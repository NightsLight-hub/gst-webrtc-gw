#include <stdio.h>
#include <atomic>
#include "compositor.h"
#include <exception>
#include <iostream>
#include <sstream>
#include <thread>
#include "gst/gstregistry.h"
/// <summary>
/// ������һ��rtpvp8�����Ƴ���·�ϲ�Ϊһ��rtp����Ȼ���³���appsink��
/// </summary>

using namespace std;
extern atomic<bool> syncFlag;
static const string TARGET_HOST = "172.18.39.162";

void MultipleRtpVp8Sink::gstreamer_receive_push_buffer(void* buffer, int len, char type) {
	GstElement* src = gst_bin_get_by_name(GST_BIN(this->pipeline), "src");
	if (src != NULL) {
		/*gpointer p = g_memdup(buffer, len);
		GstBuffer* buffer = gst_buffer_new_wrapped(p, len);*/
		if (type == '0') {
			g_object_set(G_OBJECT(outputSelector), "active-pad", outputSelectorSrcPad[0], NULL);
		}
		else {
			g_object_set(G_OBJECT(outputSelector), "active-pad", outputSelectorSrcPad[1], NULL);
		}
		GstBuffer* gbuffer = gst_buffer_new_memdup(buffer, len);
		gst_app_src_push_buffer((GstAppSrc*)src, gbuffer);
		gst_object_unref(src);
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
	default:
		/* We should not reach here because we only asked for ERRORs and EOS */
		//g_printerr("Unexpected message received. type:%d\n", GST_MESSAGE_TYPE(msg));
		break;
	}
}

void MultipleRtpVp8Sink::addRtpVp8Deocoders() {
	stringstream capstr;
	capstr << "rtpvp8decoder_" << rtpvp8DecodersCount;
	RtpVp8Decoder* rtpVp8Decoder = new RtpVp8Decoder(capstr.str());
	rtpVp8Decoder->addToPipeline(pipeline);
	rtpVp8Decoder->generatePad();
	rtpVp8Decoders[rtpvp8DecodersCount] = rtpVp8Decoder;
}


/* Create a GLib Main Loop and set it to run */
void MultipleRtpVp8Sink::mainLoop() {
	this->gstreamer_receive_main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gstreamer_receive_main_loop);
}

/// <summary>
// appsrc -> output-selector -> rtpvp8Decoder[0] -> compositor -> rptvp8Encoder -> udpsink
//                              rtpvp8Decoder[1]
/// </summary>

int MultipleRtpVp8Sink::entrypoint(atomic<bool>* flag) {
	GstPad* comp_sink1, * comp_sink2;
	GstElement* appsrc;
	/* Initialize GStreamer */
	gst_init(NULL, NULL);

	appsrc = gst_element_factory_make("appsrc", "src");
	outputSelector = gst_element_factory_make("output-selector", "outputSelector");
	compositor = gst_element_factory_make("compositor", "comp");
	udpsink = gst_element_factory_make("udpsink", "udpsink1");
	udpsinkRTCP = gst_element_factory_make("udpsink", "udpsink2");

	this->pipeline = gst_pipeline_new("test-pipeline");
	g_object_set(G_OBJECT(appsrc), "format", GST_FORMAT_TIME, NULL);
	g_object_set(G_OBJECT(appsrc), "is-live", TRUE, NULL);
	g_object_set(G_OBJECT(appsrc), "do-timestamp", TRUE, NULL);

	g_object_set(G_OBJECT(udpsink), "host", TARGET_HOST.c_str(), NULL);
	g_object_set(G_OBJECT(udpsink), "port", 5000, NULL);
	g_object_set(G_OBJECT(udpsink), "async", FALSE, NULL);
	g_object_set(G_OBJECT(udpsink), "sync", FALSE, NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "host", TARGET_HOST.c_str(), NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "port", 5001, NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "async", FALSE, NULL);
	g_object_set(G_OBJECT(udpsinkRTCP), "sync", FALSE, NULL);

	gst_bin_add_many(GST_BIN(pipeline), appsrc, outputSelector, compositor, udpsink, udpsinkRTCP, NULL);
	if (!gst_element_link_many(appsrc, outputSelector, NULL)) {
		g_printerr("Elements appsrc and outputSelector could not be linked.\n");
		exit(1);
	}
	g_object_set(G_OBJECT(outputSelector), "pad-negotiation-mode", 2, NULL);

	rtpH264Encoder = new RtpH264Encoder("rtph264enc");
	rtpH264Encoder->addToPipeline(pipeline);
	rtpH264Encoder->generatePad();
	addRtpVp8Deocoders();
	addRtpVp8Deocoders();

	GstPad* outputSelectorPad1 = gst_element_request_pad_simple(outputSelector, "src_%u");
	GstPad* outputSelectorPad2 = gst_element_request_pad_simple(outputSelector, "src_%u");
	outputSelectorSrcPad[0] = outputSelectorPad1;
	outputSelectorSrcPad[1] = outputSelectorPad2;

	if (gst_pad_link(outputSelectorPad1, rtpVp8Decoders[0]->sinkPad) != GST_PAD_LINK_OK) {
		g_printerr("outputSelectorPad1, rtpVp8Decoders[0]->sinkPad could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	if (gst_pad_link(outputSelectorPad2, rtpVp8Decoders[1]->sinkPad) != GST_PAD_LINK_OK) {
		g_printerr("outputSelectorPad1, rtpVp8Decoders[1]->sinkPad could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}


	comp_sink1 = gst_element_request_pad_simple(compositor, "sink_%u");
	g_object_set(comp_sink1, "width", WIDTH_640P, NULL);
	g_object_set(comp_sink1, "height", HEIGHT_640P, NULL);
	g_object_set(comp_sink1, "zorder", 10, NULL);

	if (gst_pad_link(rtpVp8Decoders[0]->srcPad, comp_sink1) != GST_PAD_LINK_OK) {
		g_printerr("rtpVp8Decoders[0]->srcPad, comp_sink1 could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	comp_sink2 = gst_element_request_pad_simple(compositor, "sink_%u");
	g_object_set(comp_sink2, "xpos", WIDTH_640P, NULL);
	g_object_set(comp_sink2, "width", WIDTH_640P, NULL);
	g_object_set(comp_sink2, "height", HEIGHT_640P, NULL);
	g_object_set(comp_sink2, "zorder", 100, NULL);

	if (gst_pad_link(rtpVp8Decoders[1]->srcPad, comp_sink2) != GST_PAD_LINK_OK) {
		g_printerr("rtpVp8Decoders[1]->srcPad, comp_sink2 could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	GstPad *compsrc = gst_element_get_static_pad(compositor, "src");

	gst_pad_link(compsrc, rtpH264Encoder->sinkPad);
	
	GstPad* updsinkSinkPad = gst_element_get_static_pad(udpsink, "sink");
	//GstPad* updsinkSinkRTCPPad = gst_element_get_static_pad(udpsinkRTCP, "sink");
	if (gst_pad_link(rtpH264Encoder->srcPad, updsinkSinkPad) != GST_PAD_LINK_OK) {
		g_printerr("rtpH264Encoder->srcPad, updsinkSinkPad could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	/*if (gst_pad_link(rtpbinsrcRtcpPad, updsinkSinkRTCPPad) != GST_PAD_LINK_OK) {
		g_printerr("Tee could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}*/

	// ����appsink �յ�������
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

	gst_object_unref(updsinkSinkPad);

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