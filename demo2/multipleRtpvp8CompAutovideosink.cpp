#include <stdio.h>
#include <atomic>
#include "videoCompositor.h"
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
static const string TARGET_HOST = "172.18.39.162";

void MultipleRtpVp8AutoVideoSink::receivePushBuffer(void* buffer, int len, char type) {
	GstBuffer* gbuffer = gst_buffer_new_memdup(buffer, len);
	if (type == '0') {
		gst_app_src_push_buffer((GstAppSrc*)appsrc1, gbuffer);
	}
	else {
		gst_app_src_push_buffer((GstAppSrc*)appsrc2, gbuffer);
	}
}

static void handleMessage(GstBus* bus, GstMessage* msg, MultipleRtpVp8AutoVideoSink* main) {
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
	case GST_MESSAGE_ELEMENT:
		g_print("get GST_MESSAGE_ELEMENT message\n");
		break;
	default:
		/* We should not reach here because we only asked for ERRORs and EOS */
		g_printerr("Unexpected message received. type:%d\n", GST_MESSAGE_TYPE(msg));
		break;
	}
}

RtpVp8Decoder* MultipleRtpVp8AutoVideoSink::addRtpVp8Deocoders() {
	stringstream inputName;
	inputName << "rtpvp8decoder_" << rtpvp8DecodersCount;
	RtpVp8Decoder* rtpVp8Decoder = new RtpVp8Decoder(inputName.str());
	rtpVp8Decoders[rtpvp8DecodersCount] = rtpVp8Decoder;
	gst_bin_add_many(GST_BIN(pipeline), rtpVp8Decoder->queue, rtpVp8Decoder->capsFilter, rtpVp8Decoder->rtpvp8depay, rtpVp8Decoder->vp8dec, rtpVp8Decoder->videoConvert, NULL);
	if (!gst_element_link_many(rtpVp8Decoder->queue, rtpVp8Decoder->capsFilter, rtpVp8Decoder->rtpvp8depay, rtpVp8Decoder->vp8dec, rtpVp8Decoder->videoConvert, NULL)) {
		g_printerr("Elements could not be linked.\n");
		exit(1);
	}
	return rtpVp8Decoders[rtpvp8DecodersCount++];
}


/* Create a GLib Main Loop and set it to run */
void MultipleRtpVp8AutoVideoSink::mainLoop() {
	this->gstreamer_receive_main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gstreamer_receive_main_loop);
}

/// <summary>
// appsrc1 -> rtpvp8Decoder[0] -> compositor -> videoplay
// appsrc2 -> rtpvp8Decoder[1] /
/// </summary>

int MultipleRtpVp8AutoVideoSink::entrypoint(atomic<bool>* flag) {
	GstPad* comp_sink1, * comp_sink2;
	/* Initialize GStreamer */
	gst_init(NULL, NULL);

	appsrc1 = gst_element_factory_make("appsrc", "src1");
	appsrc2 = gst_element_factory_make("appsrc", "src2");
	compositor = gst_element_factory_make("compositor", "comp");
	udpsink = gst_element_factory_make("udpsink", "udpsink1");
	udpsinkRTCP = gst_element_factory_make("udpsink", "udpsink2");

	this->pipeline = gst_pipeline_new("test-pipeline");
	g_object_set(G_OBJECT(appsrc1), "format", GST_FORMAT_TIME, NULL);
	g_object_set(G_OBJECT(appsrc1), "is-live", TRUE, NULL);
	g_object_set(G_OBJECT(appsrc1), "do-timestamp", TRUE, NULL);
	g_object_set(G_OBJECT(appsrc2), "format", GST_FORMAT_TIME, NULL);
	g_object_set(G_OBJECT(appsrc2), "is-live", TRUE, NULL);
	g_object_set(G_OBJECT(appsrc2), "do-timestamp", TRUE, NULL);

	g_object_set(G_OBJECT(udpsink), "host", TARGET_HOST.c_str(), NULL);
	g_object_set(G_OBJECT(udpsink), "port", 5000, NULL);
	g_object_set(G_OBJECT(udpsink), "async", FALSE, NULL);
	g_object_set(G_OBJECT(udpsink), "sync", FALSE, NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "host", TARGET_HOST.c_str(), NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "port", 5001, NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "async", FALSE, NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "sync", FALSE, NULL);

	gst_bin_add_many(GST_BIN(pipeline), appsrc1, appsrc2, compositor, udpsink, udpsinkRTCP, NULL);

	VideoPlay* videoPlay = new VideoPlay("vp");
	//gst_bin_add_many(GST_BIN(pipeline), videoPlay->videoConvert, videoPlay->autoVideoSink, NULL);
	videoPlay->addToPipeline(&pipeline);
	videoPlay->generatePad();
	//rtpH264Encoder->addToPipeline(&pipeline);
	addRtpVp8Deocoders();
	addRtpVp8Deocoders();
	//gst_bin_add_many(GST_BIN(pipeline), rtpVp8Decoders[0]->queue, rtpVp8Decoders[0]->capsFilter, rtpVp8Decoders[0]->rtpvp8depay, rtpVp8Decoders[0]->vp8dec, rtpVp8Decoders[0]->videoConvert, NULL);
	//gst_bin_add_many(GST_BIN(pipeline), rtpVp8Decoders[1]->queue, rtpVp8Decoders[1]->capsFilter, rtpVp8Decoders[1]->rtpvp8depay, rtpVp8Decoders[1]->vp8dec, rtpVp8Decoders[1]->videoConvert, NULL);

	
	GstPad* decoder0_sinkPad = gst_element_get_static_pad(rtpVp8Decoders[0]->queue, "sink");
	GstPad* decoder0_srcPad = gst_element_get_static_pad(rtpVp8Decoders[0]->videoConvert, "src");
	GstPad* decoder1_sinkPad = gst_element_get_static_pad(rtpVp8Decoders[1]->queue, "sink");
	GstPad* decoder1_srcPad = gst_element_get_static_pad(rtpVp8Decoders[1]->videoConvert, "src");
	GstPad* appsrc1Pad = gst_element_get_static_pad(appsrc1, "src");
	GstPad* appsrc2Pad = gst_element_get_static_pad(appsrc2, "src");

	if (gst_pad_link(appsrc1Pad, decoder0_sinkPad) != GST_PAD_LINK_OK) {
		g_printerr("outputSelectorPad1, rtpVp8Decoders[0]->sinkPad could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	if (gst_pad_link(appsrc2Pad, decoder1_sinkPad) != GST_PAD_LINK_OK) {
		g_printerr("outputSelectorPad1, rtpVp8Decoders[1]->sinkPad could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	comp_sink1 = gst_element_request_pad_simple(compositor, "sink_%u");
	g_object_set(comp_sink1, "width", WIDTH_360P/2, NULL);
	g_object_set(comp_sink1, "height", HEIGHT_360P/2, NULL);
	g_object_set(comp_sink1, "zorder", 10, NULL);
	GstPadLinkReturn padlinkRet = gst_pad_link(decoder0_srcPad, comp_sink1);
	if (padlinkRet != GST_PAD_LINK_OK) {
		g_printerr("rtpVp8Decoders[0]->srcPad, comp_sink1 could not be linked. ret is %d\n", padlinkRet);
		gst_object_unref(pipeline);
		return -1;
	}

	comp_sink2 = gst_element_request_pad_simple(compositor, "sink_%u");
	g_object_set(comp_sink2, "xpos", WIDTH_360P/2, NULL);
	g_object_set(comp_sink2, "width", WIDTH_360P/2, NULL);
	g_object_set(comp_sink2, "height", HEIGHT_360P/2, NULL);
	g_object_set(comp_sink2, "zorder", 100, NULL);

	if (gst_pad_link(decoder1_srcPad, comp_sink2) != GST_PAD_LINK_OK) {
		g_printerr("rtpVp8Decoders[1]->srcPad, comp_sink2 could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	GstPad* compsrc = gst_element_get_static_pad(compositor, "src");

	gst_pad_link(compsrc, videoPlay->sinkPad);

	bus = gst_element_get_bus(pipeline);
	//gst_bus_add_watch(bus, (GstBusFunc)handleMessage, this);
	gst_bus_add_signal_watch(bus);
	g_signal_connect(bus, "message", (GCallback)handleMessage, this);
	/* Start playing */
	GstStateChangeReturn stateChangeResult = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	printf("%d", stateChangeResult);
	flag->store(true);

	this->mainLoop();

	gst_object_unref(bus);
	gst_element_release_request_pad(compositor, comp_sink1);
	gst_element_release_request_pad(compositor, comp_sink2);
	gst_object_unref(comp_sink1);
	gst_object_unref(comp_sink2);
	g_main_loop_unref(gstreamer_receive_main_loop);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	return 0;
}
