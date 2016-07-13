#include <stdio.h>
#include <gst/gst.h>
#include <glib.h>

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

int play_song(char *location)
{
	GMainLoop *loop;

	GstElement *pipeline, *src, *mad, *sink;
	GstBus *bus;
	guint bus_watch_id;
	

	loop = g_main_loop_new(NULL, FALSE);

	// Init mp3 pipeline
	pipeline = gst_pipeline_new("mp3-player");
	src = gst_element_factory_make("filesrc", "file-source");
	mad = gst_element_factory_make("mad", "mp3-audio-demuxer");
	sink = gst_element_factory_make("alsasink", "audio-output");

	// Load song
	g_object_set(G_OBJECT(src), "location", location, NULL);

	// Message handler
	bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
	bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
	gst_object_unref(bus);

	// Setup mp3 pipeline
	gst_bin_add_many(GST_BIN(pipeline), src, mad, sink, NULL);
	gst_element_link_many(src, mad, sink, NULL);

	// Play
	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	g_main_loop_run(loop);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	g_source_remove(bus_watch_id);
	g_main_loop_unref(loop);

	return 0;
}

int main(int argc, char *argv[])
{
	gst_init(&argc, &argv);
	while (1) {
		printf("Now playing: %s\n", argv[1]);
		play_song(argv[1]);


	}
	return 0;
}
