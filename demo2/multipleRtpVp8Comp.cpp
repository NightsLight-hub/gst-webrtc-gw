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
static const string codecName = "vp8";

MultipleRtpVp8Sink::MultipleRtpVp8Sink(string targetAddress, int targetPort) {
	this->targetAddress = targetAddress;
	this->targetPort = targetPort;
}

void MultipleRtpVp8Sink::receivePushBuffer(void* buffer, int len, char type) {
	GstBuffer* gbuffer = gst_buffer_new_memdup(buffer, len);
	switch (type) {
	case '0':
		gst_app_src_push_buffer((GstAppSrc*)appSrcs[0], gbuffer);
		break;
	case '1':
		gst_app_src_push_buffer((GstAppSrc*)appSrcs[1], gbuffer);
		break;
	case '2':
		gst_app_src_push_buffer((GstAppSrc*)appSrcs[2], gbuffer);
		break;
	case '3':
		gst_app_src_push_buffer((GstAppSrc*)appSrcs[3], gbuffer);
		break;
	default:
		g_printerr("unknown buffer with type %c", type);
	}
}

static void handleMessage(GstBus* bus, GstMessage* msg, MultipleRtpVp8Sink* main) {
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

RtpVp8Decoder* MultipleRtpVp8Sink::addRtpVp8Deocoders() {
	stringstream inputName;
	inputName << "rtpvp8decoder_" << rtpvp8DecodersCount;
	RtpVp8Decoder* rtpVp8Decoder = new RtpVp8Decoder(inputName.str());
	rtpVp8Decoders[rtpvp8DecodersCount] = rtpVp8Decoder;
	/*gst_bin_add_many(GST_BIN(pipeline), rtpVp8Decoder->queue, rtpVp8Decoder->capsFilter, rtpVp8Decoder->rtpvp8depay, rtpVp8Decoder->vp8dec, rtpVp8Decoder->videoConvert, NULL);
	if (!gst_element_link_many(rtpVp8Decoder->queue, rtpVp8Decoder->capsFilter, rtpVp8Decoder->rtpvp8depay, rtpVp8Decoder->vp8dec, rtpVp8Decoder->videoConvert, NULL)) {
		g_printerr("Elements could not be linked.\n");
		exit(1);
	}*/
	gst_bin_add_many(GST_BIN(pipeline), rtpVp8Decoder->queue, rtpVp8Decoder->capsFilter, rtpVp8Decoder->rtpvp8depay, rtpVp8Decoder->vp8dec, NULL);
	if (!gst_element_link_many(rtpVp8Decoder->queue, rtpVp8Decoder->capsFilter, rtpVp8Decoder->rtpvp8depay, rtpVp8Decoder->vp8dec, NULL)) {
		g_printerr("Elements could not be linked.\n");
		exit(1);
	}
	return rtpVp8Decoders[rtpvp8DecodersCount++];
}

RtpVp9Decoder* MultipleRtpVp8Sink::addRtpVp9Deocoders() {
	stringstream inputName;
	inputName << "rtpvp9decoder_" << rtpvp9DecodersCount;
	RtpVp9Decoder* rtpVp9Decoder = new RtpVp9Decoder(inputName.str());
	rtpVp9Decoders[rtpvp9DecodersCount] = rtpVp9Decoder;
	gst_bin_add_many(GST_BIN(pipeline), rtpVp9Decoder->queue, rtpVp9Decoder->capsFilter, rtpVp9Decoder->rtpvp9depay, rtpVp9Decoder->vp9dec, rtpVp9Decoder->videoConvert, NULL);
	if (!gst_element_link_many(rtpVp9Decoder->queue, rtpVp9Decoder->capsFilter, rtpVp9Decoder->rtpvp9depay, rtpVp9Decoder->vp9dec, rtpVp9Decoder->videoConvert, NULL)) {
		g_printerr("Elements could not be linked.\n");
		exit(1);
	}
	return rtpVp9Decoders[rtpvp9DecodersCount++];
}

/* Create a GLib Main Loop and set it to run */
void MultipleRtpVp8Sink::mainLoop() {
	this->gstreamer_receive_main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gstreamer_receive_main_loop);
}

void MultipleRtpVp8Sink::createAppSrc() {
	char name[10];
	sprintf_s(name, 10, "%s_%d", "src", appSrcCount);
	GstElement* appsrc;
	appsrc = gst_element_factory_make("appsrc", name);
	g_object_set(G_OBJECT(appsrc), "format", GST_FORMAT_TIME, NULL);
	g_object_set(G_OBJECT(appsrc), "is-live", TRUE, NULL);
	g_object_set(G_OBJECT(appsrc), "do-timestamp", TRUE, NULL);
	appSrcs[appSrcCount++] = appsrc;
}

/// <summary>
// appsrc -> output-selector -> rtpvp8Decoder[0] -> compositor -> rptvp8Encoder -> udpsink
//                              rtpvp8Decoder[1]
/// </summary>

int MultipleRtpVp8Sink::entrypoint(atomic<bool>* flag) {
	GstPad* comp_sink0, * comp_sink1, * comp_sink2, * comp_sink3;
	/* Initialize GStreamer */
	gst_init(NULL, NULL);

	/*appsrc0 = gst_element_factory_make("appsrc", "src0");
	appsrc1 = gst_element_factory_make("appsrc", "src1");
	appsrc2 = gst_element_factory_make("appsrc", "src2");
	appsrc3 = gst_element_factory_make("appsrc", "src3");*/
	for (int i = 0; i < 4; i++) {
		createAppSrc();
	}
	compositor = gst_element_factory_make("compositor", "comp");
	udpsink = gst_element_factory_make("udpsink", "udpsink1");
	udpsinkRTCP = gst_element_factory_make("udpsink", "udpsink2");

	this->pipeline = gst_pipeline_new("test-pipeline");

	g_object_set(G_OBJECT(udpsink), "host", targetAddress.c_str(), NULL);
	g_object_set(G_OBJECT(udpsink), "port", targetPort, NULL);
	g_object_set(G_OBJECT(udpsink), "async", FALSE, NULL);
	g_object_set(G_OBJECT(udpsink), "sync", FALSE, NULL);
	//g_object_set(G_OBJECT(udpsinkRTCP), "host", targetAddress.c_str(), NULL);
	//g_object_set(G_OBJECT(udpsinkRTCP), "port", 5001, NULL);
	//g_object_set(G_OBJECT(udpsinkRTCP), "async", FALSE, NULL);
	//g_object_set(G_OBJECT(udpsinkRTCP), "sync", FALSE, NULL);

	gst_bin_add_many(GST_BIN(pipeline), appSrcs[0], appSrcs[1], appSrcs[2], appSrcs[3], compositor, udpsink, udpsinkRTCP, NULL);

	rtpH264Encoder = new RtpH264Encoder("rtph264enc");
	rtpH264Encoder->addToPipeline(&pipeline);
	GstPad* rtph264paySrcPad = gst_element_get_static_pad(rtpH264Encoder->rtph264pay, "src");
	GstPad* rtpbinSinkPad = gst_element_request_pad_simple(rtpH264Encoder->rtpbin, "send_rtp_sink_%u");

	//GstPad* rtpbinsrcRtcpPad = gst_element_request_pad_simple(rtpbin, "send_rtcp_src_%u");
	if (gst_pad_link(rtph264paySrcPad, rtpbinSinkPad) != GST_PAD_LINK_OK) {
		g_printerr("rtph264paySrcPad and rtpbinSinkPad could not be linked.\n");
	}
	GstPad* decoder0_sinkPad;
	GstPad* decoder0_srcPad;
	GstPad* decoder1_sinkPad;
	GstPad* decoder1_srcPad;
	GstPad* decoder2_sinkPad;
	GstPad* decoder2_srcPad;
	GstPad* decoder3_sinkPad;
	GstPad* decoder3_srcPad;

	if (codecName == "vp9") {
		addRtpVp9Deocoders();
		addRtpVp9Deocoders();
		addRtpVp9Deocoders();
		addRtpVp9Deocoders();
		decoder0_sinkPad = gst_element_get_static_pad(rtpVp9Decoders[0]->queue, "sink");
		decoder0_srcPad = gst_element_get_static_pad(rtpVp9Decoders[0]->videoConvert, "src");
		decoder1_sinkPad = gst_element_get_static_pad(rtpVp9Decoders[1]->queue, "sink");
		decoder1_srcPad = gst_element_get_static_pad(rtpVp9Decoders[1]->videoConvert, "src");
		decoder2_sinkPad = gst_element_get_static_pad(rtpVp9Decoders[2]->queue, "sink");
		decoder2_srcPad = gst_element_get_static_pad(rtpVp9Decoders[2]->videoConvert, "src");
		decoder3_sinkPad = gst_element_get_static_pad(rtpVp9Decoders[3]->queue, "sink");
		decoder3_srcPad = gst_element_get_static_pad(rtpVp9Decoders[3]->videoConvert, "src");
		GstCaps* caps = gst_caps_from_string("video/x-raw, format=I420, width=640, height=360, framerate=20/1");
		gst_pad_set_caps(decoder0_srcPad, caps);
		gst_pad_set_caps(decoder1_srcPad, caps);
		gst_pad_set_caps(decoder2_srcPad, caps);
		gst_pad_set_caps(decoder3_srcPad, caps);
	}
	else {
		addRtpVp8Deocoders();
		addRtpVp8Deocoders();
		addRtpVp8Deocoders();
		addRtpVp8Deocoders();
		decoder0_sinkPad = gst_element_get_static_pad(rtpVp8Decoders[0]->queue, "sink");
		decoder0_srcPad = gst_element_get_static_pad(rtpVp8Decoders[0]->vp8dec, "src");
		decoder1_sinkPad = gst_element_get_static_pad(rtpVp8Decoders[1]->queue, "sink");
		decoder1_srcPad = gst_element_get_static_pad(rtpVp8Decoders[1]->vp8dec, "src");
		decoder2_sinkPad = gst_element_get_static_pad(rtpVp8Decoders[2]->queue, "sink");
		decoder2_srcPad = gst_element_get_static_pad(rtpVp8Decoders[2]->vp8dec, "src");
		decoder3_sinkPad = gst_element_get_static_pad(rtpVp8Decoders[3]->queue, "sink");
		decoder3_srcPad = gst_element_get_static_pad(rtpVp8Decoders[3]->vp8dec, "src");
		GstCaps* caps = gst_caps_from_string("video/x-raw, format=I420, width=640, height=360, framerate=20/1");
		gst_pad_set_caps(decoder0_srcPad, caps);
		gst_pad_set_caps(decoder1_srcPad, caps);
		gst_pad_set_caps(decoder2_srcPad, caps);
		gst_pad_set_caps(decoder3_srcPad, caps);
	}

	GstPad* appsrc0Pad = gst_element_get_static_pad(appSrcs[0], "src");
	GstPad* appsrc1Pad = gst_element_get_static_pad(appSrcs[1], "src");
	GstPad* appsrc2Pad = gst_element_get_static_pad(appSrcs[2], "src");
	GstPad* appsrc3Pad = gst_element_get_static_pad(appSrcs[3], "src");
	if (gst_pad_link(appsrc0Pad, decoder0_sinkPad) != GST_PAD_LINK_OK) {
		g_printerr("outputSelectorPad1, rtpVp8Decoders[0]->sinkPad could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	if (gst_pad_link(appsrc1Pad, decoder1_sinkPad) != GST_PAD_LINK_OK) {
		g_printerr("outputSelectorPad1, rtpVp8Decoders[1]->sinkPad could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	if (gst_pad_link(appsrc2Pad, decoder2_sinkPad) != GST_PAD_LINK_OK) {
		g_printerr("outputSelectorPad1, rtpVp8Decoders[1]->sinkPad could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	if (gst_pad_link(appsrc3Pad, decoder3_sinkPad) != GST_PAD_LINK_OK) {
		g_printerr("outputSelectorPad1, rtpVp8Decoders[1]->sinkPad could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	// 连接 decoder 的src  到 compositor 上，配置compositor的布局
	comp_sink0 = gst_element_request_pad_simple(compositor, "sink_%u");
	g_object_set(comp_sink0, "width", WIDTH_360P, NULL);
	g_object_set(comp_sink0, "height", HEIGHT_360P, NULL);
	//g_object_set(comp_sink0, "zorder", 10, NULL);
	if (gst_pad_link(decoder0_srcPad, comp_sink0) != GST_PAD_LINK_OK) {
		g_printerr("rtpVp8Decoders[0]->srcPad, comp_sink1 could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	comp_sink1 = gst_element_request_pad_simple(compositor, "sink_%u");
	g_object_set(comp_sink1, "xpos", WIDTH_360P, NULL);
	g_object_set(comp_sink1, "width", WIDTH_360P, NULL);
	g_object_set(comp_sink1, "height", HEIGHT_360P, NULL);
	//g_object_set(comp_sink1, "zorder", 100, NULL);
	if (gst_pad_link(decoder1_srcPad, comp_sink1) != GST_PAD_LINK_OK) {
		g_printerr("rtpVp8Decoders[1]->srcPad, comp_sink2 could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	comp_sink2 = gst_element_request_pad_simple(compositor, "sink_%u");
	g_object_set(comp_sink2, "ypos", HEIGHT_360P, NULL);
	g_object_set(comp_sink2, "width", WIDTH_360P, NULL);
	g_object_set(comp_sink2, "height", HEIGHT_360P, NULL);
	//g_object_set(comp_sink2, "zorder", 100, NULL);
	if (gst_pad_link(decoder2_srcPad, comp_sink2) != GST_PAD_LINK_OK) {
		g_printerr("rtpVp8Decoders[1]->srcPad, comp_sink2 could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	comp_sink3 = gst_element_request_pad_simple(compositor, "sink_%u");
	g_object_set(comp_sink3, "xpos", WIDTH_360P, NULL);
	g_object_set(comp_sink3, "ypos", HEIGHT_360P, NULL);
	g_object_set(comp_sink3, "width", WIDTH_360P, NULL);
	g_object_set(comp_sink3, "height", HEIGHT_360P, NULL);
	//g_object_set(comp_sink3, "zorder", 100, NULL);
	if (gst_pad_link(decoder3_srcPad, comp_sink3) != GST_PAD_LINK_OK) {
		g_printerr("rtpVp8Decoders[1]->srcPad, comp_sink2 could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	GstPad* compsrc = gst_element_get_static_pad(compositor, "src");
	GstPad* rtpH264EncoderSrcPad = gst_element_get_static_pad(rtpH264Encoder->rtpbin, "send_rtp_src_0");
	GstPad* rtpH264EncoderSinkPad = gst_element_get_static_pad(rtpH264Encoder->videoConvert, "sink");
	GstPadLinkReturn padlinkRet = gst_pad_link(compsrc, rtpH264EncoderSinkPad);
	if (padlinkRet != GST_PAD_LINK_OK) {
		g_printerr("compsrc, rtpH264Encoder->sinkPad could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	GstPad* updsinkSinkPad = gst_element_get_static_pad(udpsink, "sink");
	padlinkRet = gst_pad_link(rtpH264EncoderSrcPad, updsinkSinkPad);
	if (GST_PAD_LINK_OK != padlinkRet) {
		g_printerr("rtpH264EncoderSrcPad, updsinkSinkPad could not be linked., ret is %d\n", padlinkRet);
		gst_object_unref(pipeline);
		return -1;
	}
	/*if (gst_pad_link(rtpbinsrcRtcpPad, updsinkSinkRTCPPad) != GST_PAD_LINK_OK) {
		g_printerr("Tee could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}*/

	bus = gst_element_get_bus(pipeline);
	//gst_bus_add_watch(bus, (GstBusFunc)handleMessage, this);
	gst_bus_add_signal_watch(bus);
	g_signal_connect(bus, "message", (GCallback)handleMessage, this);
	/* Start playing */
	GstStateChangeReturn stateChangeResult = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	printf("%d", stateChangeResult);
	flag->store(true);

	this->mainLoop();

	gst_object_unref(updsinkSinkPad);
	gst_element_release_request_pad(rtpH264Encoder->rtpbin, rtpbinSinkPad);
	gst_object_unref(rtph264paySrcPad);
	gst_object_unref(bus);
	gst_object_unref(appsrc0Pad);
	gst_object_unref(appsrc1Pad);
	gst_object_unref(appsrc2Pad);
	gst_object_unref(appsrc3Pad);
	gst_element_release_request_pad(compositor, comp_sink0);
	gst_element_release_request_pad(compositor, comp_sink1);
	gst_element_release_request_pad(compositor, comp_sink2);
	gst_element_release_request_pad(compositor, comp_sink3);
	gst_object_unref(comp_sink0);
	gst_object_unref(comp_sink1);
	gst_object_unref(comp_sink2);
	gst_object_unref(comp_sink3);
	g_main_loop_unref(gstreamer_receive_main_loop);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	flag->store(false);
	return 0;
}
