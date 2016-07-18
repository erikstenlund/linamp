/*
 * """"""" LINAMP """""""""""""""
 * Simple mp3 player using Gstreamer.
 *
 * Usage:
 *	./linamp <filename> - Plays the provided file
 *	./linamp -p <filename> - Plays the files in the provided file
 *	./linamp -d <filename> - Plays the files in the provided directory
 *
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <gst/gst.h>
#include <glib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define IPC_FIFO "linamp_fifo"

typedef struct playlist_t {
	char *song;
	struct playlist_t *next;
} playlist_t;

typedef struct bus_call_args_t {
	GMainLoop *loop;
	GstElement *pipeline;
	playlist_t *playlist;
} bus_call_args_t;

int play_song(char *location, GstElement *pipeline);

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
	bus_call_args_t *args = (bus_call_args_t *) data;
	playlist_t *playlist = args->playlist;
	GMainLoop *loop = args->loop;
	GstElement *pipeline = args->pipeline;

	switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_EOS:
			if (playlist->next == NULL) {
				g_main_loop_quit(loop);
			} else {
				/*
				 * ToDo:
				 * Fulhack, fixa snyggare genomgang av
				 * playlisten
				 * */
				playlist->song = playlist->next->song;
				playlist->next = playlist->next->next;
				play_song(playlist->song, pipeline);
			}
			break;
		case GST_MESSAGE_ERROR:

			g_main_loop_quit(loop);
			break;
		default:
			break;
	}

	return TRUE;
}

int init_player(GstElement *pipeline)
{
	GstElement *src, *mad, *sink;

	src = gst_element_factory_make("filesrc", "file-source");
	mad = gst_element_factory_make("mad", "mp3-audio-demuxer");
	sink = gst_element_factory_make("alsasink", "audio-output");

	gst_bin_add_many(GST_BIN(pipeline), src, mad, sink, NULL);
	gst_element_link_many(src, mad, sink, NULL);
	return 0;
}

int play_song(char *location, GstElement *pipeline)
{
	gst_element_set_state(pipeline, GST_STATE_NULL);

	GstElement *src = gst_bin_get_by_name(GST_BIN(pipeline), "file-source");
	if (!src) {
		printf("Not initialized pipeline, call init_player first");
		exit(0);
	}
	g_object_set(G_OBJECT(src), "location", location, NULL);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	printf("Now playing: %s\n", location);

	return 0;
}

gboolean play_pause(GIOChannel *src, GIOCondition cond, gpointer data)
{
	GstElement *pipeline = (GstElement *) data;
	gchar buff[4];
	gsize read;

	g_io_channel_read_chars (src, buff, 4, &read, NULL);

#ifdef DEBUG
	printf("Buff: %s, Strcmp(buff, PAUS): %d\n", buff, strcmp(buff, "PAUS"));
#endif

	if (strcmp(buff, "PAUS") == 0) {
		gst_element_set_state(pipeline, GST_STATE_PAUSED);
	} else if (strcmp(buff, "PLAY") == 0) {
		gst_element_set_state(pipeline, GST_STATE_PLAYING);
	} else {

#ifdef DEBUG
		printf("Command not found\n");
#endif

	}
	return TRUE;
}

int main(int argc, char *argv[])
{
	playlist_t playlist;
	if (argc > 2 && (strcmp(argv[1], "-d") == 0)) {
		/* Directory */

		printf("Dir\n");
		exit(0);
	} else if (argc > 2 && (strcmp(argv[1], "-p") == 0)) {
		/* File with song paths */

		/*
		 * ToDo
		 * Refactor v
		 */
		char *song = malloc(sizeof(char) * 100); // Find max filelen
		size_t len, read;
		playlist_t *prev;
		FILE* fp = fopen(argv[2], "r");
		if ((read = getline(&song, &len, fp)) != -1) {
			playlist.song = malloc(sizeof(char) * (read + 1));
			strcpy(playlist.song, song);
			playlist.song[read - 1] = '\0';
			playlist.next = NULL;
			prev = &playlist;
			while ((read = getline(&song, &len, fp)) != -1) {
				playlist_t *new = malloc(sizeof(playlist_t));
				new->song = malloc(sizeof(char) * (read + 1));
				strcpy(new->song, song);
				new->song[read - 1] = '\0';
				new->next = NULL;
				prev->next = new;
				prev = new;
			}
#ifdef DEBUG
			playlist_t *p = &playlist;
			while (p) {
				printf("%s\n", p->song);
				p = p->next;
			}
#endif
		}

	} else {
		/* Single song */
		playlist.song = argv[1];
		playlist.next = NULL;
	}

	GMainLoop *loop;
	GstElement *pipeline;
	gst_init(&argc, &argv);
	GstBus *bus;
	guint bus_watch_id;

	loop = g_main_loop_new(NULL, FALSE);

	pipeline = gst_pipeline_new("mp3-player");

	/* Message handler */
	bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
	bus_call_args_t args = {loop, pipeline, &playlist };
	bus_watch_id = gst_bus_add_watch(bus, bus_call, &args);
	gst_object_unref(bus);

	/* IPC using python gui */
	int fd[2];
	pipe(fd);
	pid_t pid = fork();
	if (pid == 0) {
		close(fd[0]);
		dup2(fd[1], 1);

		execl("./py_client.py", "./py_client.py", NULL);
	} else {
		close(fd[1]);
	}
	GIOChannel *io1 = g_io_channel_unix_new(fd[0]);
	g_io_add_watch(io1, G_IO_IN, play_pause, pipeline);


	/* IPC using fifo */
	int fp = open(IPC_FIFO, O_RDONLY | O_NONBLOCK);
	GIOChannel *io = g_io_channel_unix_new(fp);
	g_io_add_watch(io, G_IO_IN, play_pause, pipeline);

	init_player(pipeline);
	play_song(playlist.song, pipeline);

	g_main_loop_run(loop);

	/* Clean up */

	/*
	 * ToDo
	 * Minnet från playlisten måste städas upp antingen här eller i bus_call
	 * nu läcker det minne
	 */

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	g_source_remove(bus_watch_id);
	g_main_loop_unref(loop);
	return 0;
}
