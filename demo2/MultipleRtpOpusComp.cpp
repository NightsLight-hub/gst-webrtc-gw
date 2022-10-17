#include <atomic>
#include <string>
#include "audioCompositor.h"
#include "helper.h"
#include <sstream>

/// <summary>
/// composite multiple opus rtp sessions to one G722 rtp session
/// 
/// test pipeline:
/// gst-launch-1.0 udpsrc port=50001 caps="application/x-rtp, payload=9" ! queue ! rtpg722depay ! avdec_g722 ! audioresample ! audioconvert ! autoaudiosink
/// </summary>

using namespace std;
static void handleMessage(GstBus* bus, GstMessage* msg, MultipleRtpOpusCompositor* main) {
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

MultipleRtpOpusCompositor::MultipleRtpOpusCompositor(string targetAddress, int targetPort) {
	this->targetAddress = targetAddress;
	this->targetPort = targetPort;
}

void MultipleRtpOpusCompositor::quit() {
	g_main_loop_quit(this->gstreamer_receive_main_loop);
}

void MultipleRtpOpusCompositor::createAppSrc() {
	char name[10];
	sprintf_s(name, 10, "%s_%d", "src", appSrcCount);
	GstElement* appsrc;
	appsrc = gst_element_factory_make("appsrc", name);
	g_object_set(G_OBJECT(appsrc), "format", GST_FORMAT_TIME, NULL);
	g_object_set(G_OBJECT(appsrc), "is-live", TRUE, NULL);
	g_object_set(G_OBJECT(appsrc), "do-timestamp", TRUE, NULL);
	if (!gst_bin_add(GST_BIN(pipeline), appsrc)) {
		g_printerr("add appsrc to pipeline failed!\n");
		gst_object_unref(pipeline);
		return;
	}
	appSrcs[appSrcCount++] = appsrc;
}

void MultipleRtpOpusCompositor::receivePushBuffer(void* buffer, int len, char type) {
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


void MultipleRtpOpusCompositor::addDecoders() {
	stringstream inputName;
	inputName << "rtpopusdecoder_" << decodersCount;
	RtpOpusDecoder* decoder = new RtpOpusDecoder(inputName.str());
	gst_bin_add_many(GST_BIN(pipeline), decoder->queue, decoder->capsfilter, decoder->rtpOpusDepay, decoder->opusDec, NULL);
	GstPad* opusDecSinkPad = gst_element_get_static_pad(decoder->opusDec, "sink");
	GstCaps* caps = gst_caps_from_string("audio/x-raw, format=S16LE, layout=interleaved, rate=48000, channels=1");
	gst_pad_set_caps(opusDecSinkPad, caps);
	GstPad* rtpOpusDepaySrcPad = gst_element_get_static_pad(decoder->rtpOpusDepay, "src");
	if (!gst_element_link_many(decoder->queue, decoder->capsfilter, decoder->rtpOpusDepay, NULL)) {
		g_printerr("Elements could not be linked.\n");
		exit(1);
	}
	GstPadLinkReturn ret = gst_pad_link(rtpOpusDepaySrcPad, opusDecSinkPad);
	if (ret != GST_PAD_LINK_OK) {
		g_printerr("rtpOpusDepaySrcPad, opusDecSinkPad could not be linked, reason is %d\n", ret);
		gst_object_unref(pipeline);
		return;
	}
	decoders[decodersCount++] = decoder;
}

int MultipleRtpOpusCompositor::entrypoint(atomic<bool>* flag) {
	/* Initialize GStreamer */
	gst_init(NULL, NULL);
	pipeline = gst_pipeline_new("test-pipeline");
	audioMixer = gst_element_factory_make("audiomixer", "audioMixer");
	udpsink = gst_element_factory_make("udpsink", "udpsink1");
	/*GstElement* audioConvert = gst_element_factory_make("audioconvert", "vc1");
	GstElement* autoAudioSink = gst_element_factory_make("autoaudiosink", "avs1");*/
	g_object_set(G_OBJECT(udpsink), "host", this->targetAddress.c_str(), NULL);
	g_object_set(G_OBJECT(udpsink), "port", this->targetPort, NULL);
	g_object_set(G_OBJECT(udpsink), "async", FALSE, NULL);
	g_object_set(G_OBJECT(udpsink), "sync", FALSE, NULL);
	encoder = new RtpG722Encoder("rtpOpusEncoder");
	gst_bin_add_many(GST_BIN(pipeline), encoder->audioConvert, encoder->rtpg722pay, encoder->avencG722, encoder->rtpbin, udpsink, NULL);
	// add appsrc 
	for (int i = 0; i < 4; i++) {
		createAppSrc();
		addDecoders();
	}
	// link appsrcs to decoders one by one
	gst_bin_add_many(GST_BIN(pipeline), audioMixer, /*audioConvert, autoAudioSink,*/ NULL);
	for (int i = 0; i < 4; i++) {
		GstPad* decoder_srcPad;
		decoder_srcPad = gst_element_get_static_pad(decoders[i]->opusDec, "src");
		if (!gst_element_link(appSrcs[i], decoders[i]->queue)) {
			g_printerr("appsrc[%d], decoders[%d]->queue could not be linked.\n", i, i);
			gst_object_unref(pipeline);
			return -1;
		}
		// link decoder to audiomixer
		GstPad* audioMixerSinkPad = gst_element_request_pad_simple(audioMixer, "sink_%u");
		GstPadLinkReturn ret = gst_pad_link(decoder_srcPad, audioMixerSinkPad);
		if (ret != GST_PAD_LINK_OK) {
			g_printerr("decoder_srcPad, audioMixerSinkPad could not be linked, reason is %d.\n", ret);
			gst_object_unref(pipeline);
			return -1;
		}
		//gst_element_release_request_pad(audioMixer, audioMixerSinkPad);
	}

	if (!gst_element_link_pads(audioMixer, "src", encoder->audioConvert, "sink")) {
		g_printerr("audioMixer, audioConvert could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	if (!gst_element_link_pads(encoder->audioConvert, "src", encoder->avencG722, "sink")) {
		g_printerr("audioMixer, audioConvert could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	if (!gst_element_link_pads(encoder->avencG722, "src", encoder->rtpg722pay, "sink")) {
		g_printerr("audioConvert, autoAudioSink could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	GstPad* rtpOupsPaySrcPad = gst_element_get_static_pad(encoder->rtpg722pay, "src");
	GstCaps* caps = gst_caps_from_string("application/x-rtp, media=audio, payload=9, clock-rate=8000, encoding-name=G722, encoding-params=1");
	gst_pad_set_caps(rtpOupsPaySrcPad, caps);

	GstPad* rtpbinSinkPad = gst_element_request_pad_simple(encoder->rtpbin, "send_rtp_sink_%u");
	//GstPad* rtpbinsrcRtcpPad = gst_element_request_pad_simple(rtpbin, "send_rtcp_src_%u");
	if (gst_pad_link(rtpOupsPaySrcPad, rtpbinSinkPad) != GST_PAD_LINK_OK) {
		g_printerr("rtph264paySrcPad and rtpbinSinkPad could not be linked.\n");
	}
	GstPad* rtpOpusEncoderSrcPad = gst_element_get_static_pad(encoder->rtpbin, "send_rtp_src_0");
	GstPad* updsinkSinkPad = gst_element_get_static_pad(udpsink, "sink");
	GstPadLinkReturn padlinkRet = gst_pad_link(rtpOpusEncoderSrcPad, updsinkSinkPad);
	if (GST_PAD_LINK_OK != padlinkRet) {
		g_printerr("rtpOpusEncoderSrcPad, updsinkSinkPad could not be linked., ret is %d\n", padlinkRet);
		gst_object_unref(pipeline);
		return -1;
	}

	/*gst_element_link_many(audioMixer, audioConvert, autoAudioSink, NULL);*/
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
	g_main_loop_unref(gstreamer_receive_main_loop);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	flag->store(false);

	return 0;
}

void MultipleRtpOpusCompositor::mainLoop() {
	this->gstreamer_receive_main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gstreamer_receive_main_loop);
}
