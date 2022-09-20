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

void RtpVp8AppSink::gstreamer_receive_push_buffer(void* buffer, int len) {
	GstElement* src = gst_bin_get_by_name(GST_BIN(this->pipeline), "src");
	if (src != NULL) {
		/*gpointer p = g_memdup(buffer, len);
		GstBuffer* buffer = gst_buffer_new_wrapped(p, len);*/
		GstBuffer* gbuffer = gst_buffer_new_memdup(buffer, len);
		gst_app_src_push_buffer((GstAppSrc*)src, gbuffer);
		gst_object_unref(src);
	}
}

static void handleMessage(GstBus* bus, GstMessage* msg, RtpVp8AppSink* main) {
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
void RtpVp8AppSink::mainLoop() {
	this->gstreamer_receive_main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gstreamer_receive_main_loop);
}

/// <summary>
/// appsrc -> capsfilter -> rtpvp8depay -> vp8dec -> videoconvert -> tee -> queue -> compositor -> vp8enc -> rtpvp8pay -> rtpbin -> udpsink
///																	        queue -> 
/// </summary>

int RtpVp8AppSink::entrypoint(atomic<bool>* flag) {
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
	GstElement* videoConvert2 = gst_element_factory_make("videoconvert", "vd1");
	GstElement* x264enc = gst_element_factory_make("x264enc", "x264enc");
	GstElement* rtph264pay1 = gst_element_factory_make("rtph264pay", "rtph264pay");
	//GstElement* vp8enc1 = gst_element_factory_make("vp8enc", "vp8enc");
	//GstElement* rtpvp8pay1 = gst_element_factory_make("rtpvp8pay", "vp8inrpt");
	GstElement* rtpbin = gst_element_factory_make("rtpbin", "rtpbin1");
	GstElement* udpsink = gst_element_factory_make("udpsink", "udpsink1");
	GstElement* udpsinkRTCP = gst_element_factory_make("udpsink", "udpsink2");
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
	g_object_set(G_OBJECT(rtph264pay1), "config-interval", 10, NULL);
	g_object_set(G_OBJECT(rtph264pay1), "pt", 96, NULL);
	g_object_set(G_OBJECT(udpsink), "host", "172.18.39.162", NULL);
	g_object_set(G_OBJECT(udpsink), "port", 5000, NULL);
	g_object_set(G_OBJECT(udpsink), "async", FALSE, NULL);
	g_object_set(G_OBJECT(udpsink), "sync", FALSE, NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "host", "172.18.39.162", NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "port", 5001, NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "async", FALSE, NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "sync", FALSE, NULL);
	g_object_set(G_OBJECT(rtpbin), "latency", 50, NULL);

	gst_bin_add_many(GST_BIN(pipeline), appsrc, rtpvp8depay, vp8dec, videoConvert,
		capsFilter, videoQueue1, videoQueue2, tee, comp, videoConvert2, /*vp8enc1, rtpvp8pay1,*/ rtph264pay1, x264enc, rtpbin, udpsink, udpsinkRTCP, NULL);
	if (!gst_element_link_many(appsrc, capsFilter, rtpvp8depay, vp8dec, videoConvert, tee, NULL)) {
		g_printerr("Elements could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	if (!gst_element_link_many(comp, videoConvert2, x264enc, rtph264pay1, NULL)) {
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
	// link rtpvp8pay to rtpbin and updsink
	GstPad* rtpvp8paySrcPad = gst_element_get_static_pad(rtph264pay1, "src");
	GstPad* rtpbinSinkPad = gst_element_request_pad_simple(rtpbin, "send_rtp_sink_%u");
	GstPad* rtpbinsrcPad = gst_element_get_static_pad(rtpbin, "send_rtp_src_0");
	//GstPad* rtpbinsrcRtcpPad = gst_element_request_pad_simple(rtpbin, "send_rtcp_src_%u");
	GstPad* updsinkSinkPad = gst_element_get_static_pad(udpsink, "sink");
	//GstPad* updsinkSinkRTCPPad = gst_element_get_static_pad(udpsinkRTCP, "sink");
	if (gst_pad_link(rtpvp8paySrcPad, rtpbinSinkPad) != GST_PAD_LINK_OK) {
		g_printerr("Tee could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	if (gst_pad_link(rtpbinsrcPad, updsinkSinkPad) != GST_PAD_LINK_OK) {
		g_printerr("Tee could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	/*if (gst_pad_link(rtpbinsrcRtcpPad, updsinkSinkRTCPPad) != GST_PAD_LINK_OK) {
		g_printerr("Tee could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}*/

	// 处理appsink 收到的数据
	// use appsink action signal "try-pull-sample " to retrive GstSample
	//thread sampleThread(handleSinkSmaple);

	bus = gst_element_get_bus(pipeline);
	//gst_bus_add_watch(bus, (GstBusFunc)handleMessage, this);
	gst_bus_add_signal_watch(bus);
	g_signal_connect(bus, "message", (GCallback)handleMessage, this);
	/* Start playing */
	GstStateChangeReturn stateChangeResult = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	printf("%d", stateChangeResult);
	flag->store(true);

	this->mainLoop();

	gst_object_unref(queue_vd1_pad_sink);
	gst_object_unref(queue_vd2_pad_sink);
	gst_object_unref(queue_vd1_pad_src);
	gst_object_unref(queue_vd2_pad_src);
	gst_object_unref(rtpvp8paySrcPad);
	gst_object_unref(updsinkSinkPad);
	gst_element_release_request_pad(rtpbin, rtpbinSinkPad);
	gst_object_unref(rtpbinsrcPad);

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
