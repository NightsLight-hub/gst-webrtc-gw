//#include <stdio.h>
//#include <atomic>
//#include "appsrc.h"
//#include <exception>
//#include <iostream>
//#include "gst/gstregistry.h"
//using namespace std;
//
//static GMainLoop* gstreamer_receive_main_loop = NULL;
//static GstElement* pipeline;
//extern atomic<bool> syncFlag;
//
//void gstreamer_receive_push_buffer(void* buffer, int len) {
//	GstElement* src = gst_bin_get_by_name(GST_BIN(pipeline), "src");
//	if (src != NULL) {
//		gpointer p = g_memdup(buffer, len);
//		GstBuffer* buffer = gst_buffer_new_wrapped(p, len);
//		gst_app_src_push_buffer((GstAppSrc*)src, buffer);
//		gst_object_unref(src);
//	}
//}
//
///* This function is called when an error message is posted on the bus */
//static void error_cb(GstBus* bus, GstMessage* msg, GMainLoop* main_loop) {
//	GError* err;
//	gchar* debug_info;
//
//	/* Print error details on the screen */
//	gst_message_parse_error(msg, &err, &debug_info);
//	g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
//	g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
//	g_clear_error(&err);
//	g_free(debug_info);
//
//	g_main_loop_quit(main_loop);
//}
//
//static gboolean handleMessage(GstBus* bus, GstMessage* msg, GMainLoop* main_loop) {
//	GError* err;
//	gchar* debug_info;
//	switch (GST_MESSAGE_TYPE(msg)) {
//	case GST_MESSAGE_ERROR:
//		gst_message_parse_error(msg, &err, &debug_info);
//		g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
//		g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
//		g_clear_error(&err);
//		g_free(debug_info);
//		break;
//	case GST_MESSAGE_EOS:
//		g_print("End-Of-Stream reached.\n");
//		break;
//	case GST_MESSAGE_STATE_CHANGED:
//		if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
//			GstState old_state, new_state, pending_state;
//			gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
//			g_print("Pipeline state changed from %s to %s:\n",
//				gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
//		}
//		break;
//	default:
//		/* We should not reach here because we only asked for ERRORs and EOS */
//		g_printerr("Unexpected message received.\n");
//		break;
//	}
//	gst_message_unref(msg);
//	return FALSE;
//}
//
///* Create a GLib Main Loop and set it to run */
//static void mainLoop() {
//	gstreamer_receive_main_loop = g_main_loop_new(NULL, FALSE);
//	g_main_loop_run(gstreamer_receive_main_loop);
//}
//
//static void cb_new_pad(GstElement* element, GstPad* pad, gpointer data)
//{
//	gchar* name;
//	GstElement* other = (GstElement*)data;
//
//	name = gst_pad_get_name(pad);
//	g_print("A new pad %s was created for %s\n", name, gst_element_get_name(element));
//	g_free(name);
//
//	g_print("element %s will be linked to %s\n",
//		gst_element_get_name(element),
//		gst_element_get_name(other));
//	gst_element_link(element, other);
//}
//
//int tutorial_8(atomic<bool>* flag) {
//	GstBus* bus;
//	GstPad* tee_vd1_pad, * tee_vd2_pad;
//	GstPad* queue_vd1_pad, * queue_vd2_pad;
//	GstElement* appsrc;
//	/* Initialize GStreamer */
//	gst_init(NULL, NULL);
//	GstElement* decodebin = gst_element_factory_make("vp8dec", "decode");
//
//	appsrc = gst_element_factory_make("appsrc", "src");
//	GstElement* tee = gst_element_factory_make("tee", "tee1");
//	GstElement* rtpvp8depay = gst_element_factory_make("rtpvp8depay", "depay");
//	GstElement* capsFilter = gst_element_factory_make("capsfilter", "cf");
//	GstElement* videoConvert = gst_element_factory_make("videoconvert", "vc");
//
//	GstElement* videoSink1 = gst_element_factory_make("autovideosink", "vd1");
//	pipeline = gst_pipeline_new("test-pipeline");
//	g_object_set(G_OBJECT(appsrc), "format", GST_FORMAT_TIME, NULL);
//	g_object_set(G_OBJECT(appsrc), "is-live", TRUE, NULL);
//	//g_object_set(G_OBJECT(appsrc), "do-timestamp", TRUE, NULL);
//
//	/*GstCaps* caps1 = gst_caps_from_string("application/x-rtp, encoding-name=VP8-DRAFT-IETF-01, clock-rate=90000, media=video");
//	g_object_set(G_OBJECT(appsrc), "caps", caps1, NULL);*/
//	GstCaps* caps1 = gst_caps_from_string("application/x-rtp, payload=96, encoding-name=VP8-DRAFT-IETF-01");
//	g_object_set(G_OBJECT(capsFilter), "caps", caps1, NULL);
//
//
//	/* Link all elements that can be automatically linked because they have "Always" pads */
//	gst_bin_add_many(GST_BIN(pipeline), appsrc, rtpvp8depay, decodebin, videoConvert, capsFilter, videoSink1, NULL);
//	if (gst_element_link_many(appsrc, capsFilter, rtpvp8depay, decodebin, videoConvert, videoSink1, NULL) != TRUE) {
//		g_printerr("Elements could not be linked.\n");
//		gst_object_unref(pipeline);
//		return -1;
//	}
//
//
//	/*GstPad *rtpvp8depaySinkPad = gst_element_get_static_pad(rtpvp8depay, "sink");
//	GstCaps* caps = gst_caps_from_string("application/x-rtp, payload=96, encoding-name=VP8-DRAFT-IETF-01 clock-rate=90000, media=video");
//	gst_pad_set_caps(rtpvp8depaySinkPad, caps);*/
//
//	/*gst_element_link_pads(appsrc, "src", rtpvp8depay, "sink");
//	gst_element_link_pads(rtpvp8depay, "src", decodebin, "sink");*/
//
//
//	/*pipeline =
//		gst_parse_launch
//		("appsrc format=time is-live=true do-timestamp=true name=src ! application/x-rtp, payload=96, encoding-name=VP8-DRAFT-IETF-01 ! rtpvp8depay ! decodebin ! tee name=tee1", NULL); */
//
//		//tee_vd1_pad = gst_element_request_pad_simple(tee, "src_%u");
//		//g_print("Obtained request pad %s for audio branch.\n", gst_pad_get_name(tee_vd1_pad));
//		//queue_vd1_pad = gst_element_get_static_pad(videoSink1, "sink");
//		//tee_vd2_pad = gst_element_request_pad_simple(tee, "src_%u");
//		//g_print("Obtained request pad %s for video branch.\n", gst_pad_get_name(tee_vd2_pad));
//		//queue_vd2_pad = gst_element_get_static_pad(videoSink2, "sink");
//		//if (gst_pad_link(tee_vd1_pad, queue_vd1_pad) != GST_PAD_LINK_OK ||
//		//	gst_pad_link(tee_vd2_pad, queue_vd2_pad) != GST_PAD_LINK_OK) {
//		//	g_printerr("Tee could not be linked.\n");
//		//	gst_object_unref(pipeline);
//		//	return -1;
//		//}
//		//gst_object_unref(queue_vd1_pad);
//		//gst_object_unref(queue_vd2_pad);
//
//		//g_signal_connect(decodebin, "pad-added", G_CALLBACK(cb_new_pad), videoSink1);
//
//	bus = gst_element_get_bus(pipeline);
//	gst_bus_add_watch(bus, (GstBusFunc)handleMessage, gstreamer_receive_main_loop);
//	/*gst_bus_add_signal_watch(bus);
//	g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, gstreamer_receive_main_loop);*/
//	gst_object_unref(bus);
//	/* Start playing */
//	GstStateChangeReturn stateChangeResult = gst_element_set_state(pipeline, GST_STATE_PLAYING);
//	printf("%d", stateChangeResult);
//	flag->store(true);
//
//	mainLoop();
//
//	gst_element_set_state(pipeline, GST_STATE_NULL);
//	gst_object_unref(pipeline);
//	return 0;
//}
//
