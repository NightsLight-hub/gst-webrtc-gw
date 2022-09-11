#include <stdio.h>
#include <atomic>
#include "appsrc.h"
using namespace std;

static GMainLoop* gstreamer_receive_main_loop = NULL;
static GstElement* pipeline;
extern atomic<bool> syncFlag;

void gstreamer_receive_push_buffer(void* buffer, int len) {
	GstElement* src = gst_bin_get_by_name(GST_BIN(pipeline), "src");
	if (src != NULL) {
		gpointer p = g_memdup(buffer, len);
		GstBuffer* buffer = gst_buffer_new_wrapped(p, len);
		gst_app_src_push_buffer((GstAppSrc*)src, buffer);
		gst_object_unref(src);
	}
}

/* This function is called when an error message is posted on the bus */
static void error_cb(GstBus* bus, GstMessage* msg, GMainLoop* main_loop) {
	GError* err;
	gchar* debug_info;

	/* Print error details on the screen */
	gst_message_parse_error(msg, &err, &debug_info);
	g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
	g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
	g_clear_error(&err);
	g_free(debug_info);

	g_main_loop_quit(main_loop);
}

static gboolean handleMessage(GstBus* bus, GstMessage* msg, GMainLoop* main_loop) {
	GError* err;
	gchar* debug_info;
	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_ERROR:
		gst_message_parse_error(msg, &err, &debug_info);
		g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
		g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
		g_clear_error(&err);
		g_free(debug_info);
		break;
	case GST_MESSAGE_EOS:
		g_print("End-Of-Stream reached.\n");
		break;
	case GST_MESSAGE_STATE_CHANGED:
		if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
			GstState old_state, new_state, pending_state;
			gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
			g_print("Pipeline state changed from %s to %s:\n",
				gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
		}
		break;
	default:
		/* We should not reach here because we only asked for ERRORs and EOS */
		g_printerr("Unexpected message received.\n");
		break;
	}
	gst_message_unref(msg);
	return FALSE;
}

/* Create a GLib Main Loop and set it to run */
static void mainLoop() {
	gstreamer_receive_main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gstreamer_receive_main_loop);
}

int tutorial_8(atomic<bool> * flag) {
	GstBus* bus;

	/* Initialize GStreamer */
	gst_init(NULL, NULL);

	pipeline =
		gst_parse_launch
		("appsrc format=time is-live=true do-timestamp=true name=src ! application/x-rtp, payload=96, encoding-name=VP8-DRAFT-IETF-01 ! rtpvp8depay ! decodebin ! autovideosink", NULL);

	bus = gst_element_get_bus(pipeline);
	gst_bus_add_watch(bus, (GstBusFunc)handleMessage, mainLoop);
	/*gst_bus_add_signal_watch(bus);
	g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, gstreamer_receive_main_loop);*/
	gst_object_unref(bus);
	flag->store(true);
	/* Start playing */
	GstStateChangeReturn stateChangeResult = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	//printf("%d", stateChangeResult);

	mainLoop();

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	return 0;
}

