#include <stdio.h>
#include <atomic>
#include "appsrc.h"
#include <exception>
#include <iostream>
#include <sstream>
#include <thread>
#include "gst/gstregistry.h"
/// <summary>
/// 用来将一个rtpvp8流复制成两路，然后并排播放
/// </summary>

using namespace std;
extern atomic<bool> syncFlag;

void RtpVp8Composite::gstreamer_receive_push_buffer(void* buffer, int len) {
	GstElement* src = gst_bin_get_by_name(GST_BIN(this->pipeline), "src");
	if (src != NULL) {
		gpointer p = g_memdup(buffer, len);
		GstBuffer* buffer = gst_buffer_new_wrapped(p, len);
		gst_app_src_push_buffer((GstAppSrc*)src, buffer);
		gst_object_unref(src);
	}
}

static void stop(RtpVp8Composite* main, atomic<bool>* flag) {
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	flag->store(false);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//gst_element_set_state(main->pipeline, GST_STATE_NULL);
	//g_main_loop_quit(main->gstreamer_receive_main_loop);
}

static void handleMessage(GstBus* bus, GstMessage* msg, RtpVp8Composite* main) {
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
		g_main_loop_quit(main->gstreamer_receive_main_loop);
		break;
	case GST_MESSAGE_STATE_CHANGED:
		if (GST_MESSAGE_SRC(msg) == GST_OBJECT(main->pipeline)) {
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
}

/* Create a GLib Main Loop and set it to run */
void RtpVp8Composite::mainLoop() {
	this->gstreamer_receive_main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gstreamer_receive_main_loop);
}

static void cb_new_pad(GstElement* element, GstPad* pad, gpointer data)
{
	gchar* name;
	GstElement* other = (GstElement*)data;

	name = gst_pad_get_name(pad);
	g_print("A new pad %s was created for %s\n", name, gst_element_get_name(element));
	g_free(name);

	g_print("element %s will be linked to %s\n",
		gst_element_get_name(element),
		gst_element_get_name(other));
	gst_element_link(element, other);
}

int RtpVp8Composite::entrypoint(atomic<bool>* flag) {
	GstBus* bus;
	GstPad* tee_vd1_pad, * tee_vd2_pad;
	GstPad* comp_sink1, * comp_sink2;
	GstPad* queue_vd1_pad_sink, * queue_vd1_pad_src, * queue_vd2_pad_sink, * queue_vd2_pad_src;
	GstElement* appsrc;
	/* Initialize GStreamer */
	gst_init(NULL, NULL);
	GstElement* vp8dec = gst_element_factory_make("vp8dec", "decode");

	appsrc = gst_element_factory_make("appsrc", "src");
	GstElement* tee = gst_element_factory_make("tee", "tee1");
	GstElement* rtpvp8depay = gst_element_factory_make("rtpvp8depay", "depay");
	GstElement* capsFilter = gst_element_factory_make("capsfilter", "cf");
	GstElement* videoConvert = gst_element_factory_make("videoconvert", "vc");
	GstElement* videoQueue1 = gst_element_factory_make("queue", "q1");
	GstElement* videoQueue2 = gst_element_factory_make("queue", "q2");
	GstElement* comp = gst_element_factory_make("compositor", "comp");
	GstElement* videoSink1 = gst_element_factory_make("autovideosink", "vd1");

	this->pipeline = gst_pipeline_new("test-pipeline");
	g_object_set(G_OBJECT(appsrc), "format", GST_FORMAT_TIME, NULL);
	g_object_set(G_OBJECT(appsrc), "is-live", TRUE, NULL);
	g_object_set(G_OBJECT(appsrc), "do-timestamp", TRUE, NULL);
	stringstream capstr;
	capstr << "application/x-rtp, payload=96, encoding-name=VP8-DRAFT-IETF-01, width=" << WIDTH_640P << ", height=" << HEIGHT_640P;
	//char* capStr = sprintf("application/x-rtp, payload=96, encoding-name=VP8-DRAFT-IETF-01, width=%d, height=%d", WIDTH_640P, HEIGHT_640P);
	GstCaps* caps1 = gst_caps_from_string(capstr.str().c_str());
	g_object_set(G_OBJECT(capsFilter), "caps", caps1, NULL);
	/* Link all elements that can be automatically linked because they have "Always" pads */
	gst_bin_add_many(GST_BIN(pipeline), appsrc, rtpvp8depay, vp8dec, videoConvert, capsFilter, videoQueue1, videoQueue2, videoSink1/*,videoSink2*/, tee, comp, NULL);
	if (!gst_element_link_many(appsrc, capsFilter, rtpvp8depay, vp8dec, videoConvert, tee, NULL)) {
		g_printerr("Elements could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	if (!gst_element_link_many(comp, videoSink1, NULL)) {
		g_printerr("vq,vd1 Elements could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	// link tee src_1 to comp sink_1
	tee_vd1_pad = gst_element_request_pad_simple(tee, "src_%u");
	queue_vd1_pad_sink = gst_element_get_static_pad(videoQueue1, "sink");
	queue_vd1_pad_src = gst_element_get_static_pad(videoQueue1, "src");
	comp_sink1 = gst_element_request_pad_simple(comp, "sink_%u");

	g_object_set(comp_sink1, "width", WIDTH_640P, NULL);
	g_object_set(comp_sink1, "height", HEIGHT_640P, NULL);
	g_object_set(comp_sink1, "zorder", 10, NULL);

	tee_vd2_pad = gst_element_request_pad_simple(tee, "src_%u");
	queue_vd2_pad_sink = gst_element_get_static_pad(videoQueue2, "sink");
	queue_vd2_pad_src = gst_element_get_static_pad(videoQueue2, "src");
	comp_sink2 = gst_element_request_pad_simple(comp, "sink_%u");
	//comp_sink2 = gst_element_get_static_pad(videoSink2, "sink");

	g_object_set(comp_sink2, "xpos", WIDTH_640P, NULL);
	g_object_set(comp_sink2, "width", WIDTH_640P, NULL);
	g_object_set(comp_sink2, "height", HEIGHT_640P, NULL);
	g_object_set(comp_sink2, "zorder", 100, NULL);

	if (gst_pad_link(tee_vd1_pad, queue_vd1_pad_sink) != GST_PAD_LINK_OK) {
		g_printerr("Tee could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	if (gst_pad_link(queue_vd1_pad_src, comp_sink1) != GST_PAD_LINK_OK) {
		g_printerr("Tee could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	if (gst_pad_link(tee_vd2_pad, queue_vd2_pad_sink) != GST_PAD_LINK_OK) {
		g_printerr("Tee could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	if (gst_pad_link(queue_vd2_pad_src, comp_sink2) != GST_PAD_LINK_OK) {
		g_printerr("Tee could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	gst_object_unref(queue_vd1_pad_sink);
	gst_object_unref(queue_vd2_pad_sink);
	gst_object_unref(queue_vd1_pad_src);
	gst_object_unref(queue_vd2_pad_src);


	bus = gst_element_get_bus(pipeline);
	//gst_bus_add_watch(bus, (GstBusFunc)handleMessage, this);
	gst_bus_add_signal_watch(bus);
	GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
	g_signal_connect(bus, "message", (GCallback)handleMessage, this);
	/* Start playing */
	GstStateChangeReturn stateChangeResult = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	printf("%d", stateChangeResult);
	flag->store(true);
	
	//thread ttt(stop, this, flag);
	this->mainLoop();
	gst_object_unref(bus);
	gst_element_release_request_pad(tee, tee_vd1_pad);
	gst_element_release_request_pad(tee, tee_vd2_pad);
	gst_element_release_request_pad(comp, comp_sink1);
	gst_element_release_request_pad(comp, comp_sink2);
	gst_object_unref(tee_vd1_pad);
	gst_object_unref(tee_vd2_pad);
	gst_object_unref(comp_sink1);
	gst_object_unref(comp_sink2);
	g_main_loop_unref(gstreamer_receive_main_loop);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	return 0;
}

