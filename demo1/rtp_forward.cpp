#include <stdio.h>
#include <atomic>
#include "appsrc.h"
#include <exception>
#include <iostream>
#include <sstream>
#include <thread>
#include "gst/gstregistry.h"
/// <summary>
/// 用来将一个rtpvp8流复制成两路合并为一个rtp流，然后下沉到appsink中
/// </summary>

using namespace std;
extern atomic<bool> syncFlag;

void RtpForward::gstreamer_receive_push_buffer(void* buffer, int len) {
	GstElement* src = gst_bin_get_by_name(GST_BIN(this->pipeline), "src");
	if (src != NULL) {
		/*gpointer p = g_memdup(buffer, len);
		GstBuffer* buffer = gst_buffer_new_wrapped(p, len);*/
		GstBuffer* gbuffer = gst_buffer_new_memdup(buffer, len);
		gst_app_src_push_buffer((GstAppSrc*)src, gbuffer);
		gst_object_unref(src);
	}
}

static void handleMessage(GstBus* bus, GstMessage* msg, RtpForward* main) {
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
	case GST_MESSAGE_STREAM_STATUS:
		GstStreamStatusType type;
		GstElement* owner;
		gst_message_parse_stream_status(msg, &type, &owner);
		g_print("get GST_MESSAGE_STREAM_STATUS message, type is %d, owner is %s\n", type, owner->object.name);
		break;
	case GST_MESSAGE_NEW_CLOCK:
		g_print("get GST_MESSAGE_NEW_CLOCK message\n");
		break;

	case GST_MESSAGE_LATENCY:
		g_print("get GST_MESSAGE_LATENCY message\n");
		break;
	default:
		/* We should not reach here because we only asked for ERRORs and EOS */
		//g_printerr("Unexpected message received. type:%d\n", GST_MESSAGE_TYPE(msg));
		break;
	}
}

//void handleSinkSmaple() {
//	GstSample *sample = NULL;
//	while (true)
//	{
//		g_signal_emit_by_name(appsink1, "pull-sample", &sample);
//		//g_signal_emit_by_name(appsink1, "pull-preroll", &sample);
//		if (!sample) {
//			g_printerr("try-pull-sample return null");
//		}
//		else {
//			GstBuffer* buffer = gst_sample_get_buffer(sample);
//			gsize n = gst_buffer_get_size(buffer);
//			g_print("get buffer with size %d\n", n);
//			char recvBuf[1501] = { 0 };
//			int recvBufLength = 1500;
//			n = gst_buffer_extract(buffer, 0, recvBuf, recvBufLength);
//			g_print("extract bytes with size %d", n);
//			gst_sample_unref(sample);
//		}
//	}
//}

/* Create a GLib Main Loop and set it to run */
void RtpForward::mainLoop() {
	this->gstreamer_receive_main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gstreamer_receive_main_loop);
}

/// <summary>
/// appsrc -> capsfilter -> rtpvp8depay -> vp8dec -> videoconvert -> tee -> queue -> compositor -> vp8enc -> rtpvp8pay -> rtpbin -> udpsink
///																	        queue -> 
/// </summary>

int RtpForward::entrypoint(atomic<bool>* flag) {
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
	GstElement* x264enc = gst_element_factory_make("x264enc", "x264enc");
	GstElement* rtph264pay1 = gst_element_factory_make("rtph264pay", "rtph264pay");
	//GstElement* vp8enc1 = gst_element_factory_make("vp8enc", "vp8enc");
	//GstElement* rtpvp8pay1 = gst_element_factory_make("rtpvp8pay", "vp8inrpt");
	GstElement* rtpbin = gst_element_factory_make("rtpbin", "rtpbin1");
	GstElement* udpsink = gst_element_factory_make("udpsink", "udpsink1");
	this->pipeline = gst_pipeline_new("test-pipeline");
	g_object_set(G_OBJECT(appsrc), "format", GST_FORMAT_TIME, NULL);
	g_object_set(G_OBJECT(appsrc), "is-live", TRUE, NULL);
	g_object_set(G_OBJECT(appsrc), "do-timestamp", TRUE, NULL);
	stringstream capstr;
	capstr << "application/x-rtp, payload=96, encoding-name=VP8-DRAFT-IETF-01, width=" << WIDTH_640P << ", height=" << HEIGHT_640P;
	//char* capStr = sprintf("application/x-rtp, payload=96, encoding-name=VP8-DRAFT-IETF-01, width=%d, height=%d", WIDTH_640P, HEIGHT_640P);
	GstCaps* caps1 = gst_caps_from_string(capstr.str().c_str());
	g_object_set(G_OBJECT(capsFilter), "caps", caps1, NULL);
	g_object_set(G_OBJECT(x264enc), "bitrate", 4096, NULL);
	g_object_set(G_OBJECT(x264enc), "tune", 4, NULL);
	g_object_set(G_OBJECT(x264enc), "speed-preset", 2, NULL);
	//g_object_set(G_OBJECT(rtph264pay1), "config-interval", 20, NULL);
	g_object_set(G_OBJECT(rtph264pay1), "pt", 96, NULL);
	g_object_set(G_OBJECT(rtph264pay1), "aggregate-mode", 1, NULL);
	g_object_set(G_OBJECT(udpsink), "host", "172.15.1.112", NULL);
	g_object_set(G_OBJECT(udpsink), "port", 5000, NULL);
	g_object_set(G_OBJECT(udpsink), "async", FALSE, NULL);
	g_object_set(G_OBJECT(udpsink), "sync", FALSE, NULL);
	//g_object_set(G_OBJECT(rtpbin), "drop-on-latency", TRUE, NULL);

	gst_bin_add_many(GST_BIN(pipeline), appsrc, rtpvp8depay, vp8dec, videoConvert,
		capsFilter, tee, rtph264pay1, x264enc, rtpbin, udpsink, NULL);
	if (!gst_element_link_many(appsrc, capsFilter, rtpvp8depay, vp8dec, videoConvert, x264enc, rtph264pay1, udpsink, NULL)) {
		g_printerr("Elements could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	// link rtpvp8pay to rtpbin and updsink
	//GstPad* rtpvp8paySrcPad = gst_element_get_static_pad(rtph264pay1, "src");
	//GstPad* rtpbinSinkPad = gst_element_request_pad_simple(rtpbin, "send_rtp_sink_%u");
	//GstPad* rtpbinsrcPad = gst_element_get_static_pad(rtpbin, "send_rtp_src_0");
	////GstPad* rtpbinsrcRtcpPad = gst_element_request_pad_simple(rtpbin, "send_rtcp_src_%u");
	//GstPad* updsinkSinkPad = gst_element_get_static_pad(udpsink, "sink");
	////GstPad* updsinkSinkRTCPPad = gst_element_get_static_pad(udpsinkRTCP, "sink");
	//if (gst_pad_link(rtpvp8paySrcPad, rtpbinSinkPad) != GST_PAD_LINK_OK) {
	//	g_printerr("Tee could not be linked.\n");
	//	gst_object_unref(pipeline);
	//	return -1;
	//}
	//if (gst_pad_link(rtpbinsrcPad, updsinkSinkPad) != GST_PAD_LINK_OK) {
	//	g_printerr("Tee could not be linked.\n");
	//	gst_object_unref(pipeline);
	//	return -1;
	//}

	bus = gst_element_get_bus(pipeline);
	//gst_bus_add_watch(bus, (GstBusFunc)handleMessage, this);
	gst_bus_add_signal_watch(bus);
	g_signal_connect(bus, "message", (GCallback)handleMessage, this);
	/* Start playing */
	GstStateChangeReturn stateChangeResult = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	printf("%d", stateChangeResult);
	flag->store(true);

	this->mainLoop();

	//gst_object_unref(rtpvp8paySrcPad);
	//gst_object_unref(updsinkSinkPad);
	//gst_element_release_request_pad(rtpbin, rtpbinSinkPad);
	//gst_object_unref(rtpbinsrcPad);

	gst_object_unref(bus);
	g_main_loop_unref(gstreamer_receive_main_loop);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	return 0;
}
