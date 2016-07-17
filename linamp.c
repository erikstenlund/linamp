#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <gst/gst.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>

#define IPC_FIFO "linamp_fifo"

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = (GMainLoop *) data;

	switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_EOS:

			g_main_loop_quit(loop);
			break;
		case GST_MESSAGE_ERROR:

			g_main_loop_quit(loop);
			break;
		default:
			break;
	}

	return TRUE;
}

int play_song(char *location, GstElement *pipeline, GMainLoop *loop)
{
	GstElement *src, *mad, *sink;

	src = gst_element_factory_make("filesrc", "file-source");
	mad = gst_element_factory_make("mad", "mp3-audio-demuxer");
	sink = gst_element_factory_make("alsasink", "audio-output");

	g_object_set(G_OBJECT(src), "location", location, NULL);

	gst_bin_add_many(GST_BIN(pipeline), src, mad, sink, NULL);
	gst_element_link_many(src, mad, sink, NULL);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	return 0;
}
int play_pause(GIOChannel *src, GIOCondition cond, gpointer data)
{
	GstElement *pipeline = (GstElement *) data;
	gchar buff[4];
	gsize read;

	g_io_channel_read_chars (src, buff, 4, &read, NULL);

#ifdef DEBUG
	printf("Buff: %s\n", buff);
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
	GMainLoop *loop;
	GstElement *pipeline;
	gst_init(&argc, &argv);
	GstBus *bus;
	guint bus_watch_id;

	loop = g_main_loop_new(NULL, FALSE);

	pipeline = gst_pipeline_new("mp3-player");

	/* Message handler */
	bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
	bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
	gst_object_unref(bus);


	/* IPC using fifo */
	int fp;
	fp = open(IPC_FIFO, O_RDONLY | O_NONBLOCK);
	GIOChannel *io = g_io_channel_unix_new(fp);
	g_io_add_watch(io, G_IO_IN, play_pause, pipeline);

	play_song(argv[1], pipeline, loop);
	printf("Now playing: %s\n", argv[1]);

	g_main_loop_run(loop);

	/* Clean up */
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	g_source_remove(bus_watch_id);
	g_main_loop_unref(loop);
	return 0;
}
