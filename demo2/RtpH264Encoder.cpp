#include "videoCompositor.h"
#include "helper.h"
#include <sstream>
using namespace std;

//class RtpH264Encoder {
//public:
//	RtpH264Encoder(std::string name);
//	std::string name;
//	GstElement* x264enc;
//	GstElement* rtpvp8pay;
//	GstElement* rtpbin;
//	GstPad* srcPad;
//	GstPad* sinkPad;
//};

// more bitrate, more bandwith, lower cpu usage
static const int BITRATE = 2048 * 5;
// 10ms buffer 
static const int LATENCY = 10;

RtpH264Encoder::RtpH264Encoder(string inputName) {
	name = inputName;
	gchar videoConvertName[100];
	buildName(name, "videoconvert", videoConvertName, 100);
	videoConvert = gst_element_factory_make("videoconvert", videoConvertName);
	gst_object_ref(videoConvert);

	gchar queueName[100];
	buildName(name, "queue", queueName, 100);
	queue = gst_element_factory_make("queue", queueName);
	gst_object_ref(queue);

	gchar x264EncName[100];
	buildName(name, "x264enc", x264EncName, 100);
	x264enc = gst_element_factory_make("x264enc", x264EncName);
	gst_object_ref(x264enc);

	gchar rtph264payName[100];
	buildName(name, "rtph264pay", rtph264payName, 100);
	rtph264pay = gst_element_factory_make("rtph264pay", rtph264payName);
	gst_object_ref(rtph264pay);

	gchar rtpbinName[100];
	buildName(name, "rtpbin", rtpbinName, 100);
	rtpbin = gst_element_factory_make("rtpbin", rtpbinName);
	gst_object_ref(rtpbin);

	g_object_set(G_OBJECT(rtpbin), "latency", LATENCY, NULL);
	g_object_set(G_OBJECT(x264enc), "bitrate", BITRATE, NULL);
	g_object_set(G_OBJECT(x264enc), "tune", 0x00000004, NULL);
	//ultra fast speed!
	g_object_set(G_OBJECT(x264enc), "speed-preset", 1, NULL);
	g_object_set(G_OBJECT(rtph264pay), "aggregate-mode", 1, NULL);
	g_object_set(G_OBJECT(rtph264pay), "config-interval", 10, NULL);
	g_object_set(G_OBJECT(rtph264pay), "pt", 111, NULL);
	g_object_set(G_OBJECT(rtph264pay), "mtu", 1400, NULL);
}

void RtpH264Encoder::addToPipeline(GstElement** pipeline) {
	gst_bin_add_many(GST_BIN(*pipeline), videoConvert, queue, x264enc, rtph264pay, rtpbin, NULL);
	if (!gst_element_link_many(videoConvert, queue, x264enc, NULL)) {
		g_printerr("Elements could not be linked.\n");
		exit(1);
	}
	GstPad* encSrcP = gst_element_get_static_pad(x264enc, "src");
	GstPad* rtph264paySinkP = gst_element_get_static_pad(rtph264pay, "sink");
	//GstCaps* caps = gst_caps_from_string("video/x-h264,  framerate=20/1, width=540, height=360, profile=baseline, alignment=au, stream-format=byte-stream");
	GstCaps* caps = gst_caps_from_string("video/x-h264,  framerate=20/1, width=540, height=360, profile=baseline, stream-format=byte-stream");
	gst_pad_set_caps(encSrcP, caps);
	//GstPad* rtpbinsrcRtcpPad = gst_element_request_pad_simple(rtpbin, "send_rtcp_src_%u");
	if (gst_pad_link(encSrcP, rtph264paySinkP) != GST_PAD_LINK_OK) {
		g_printerr("RtpH264Encoder:: encSrcP, rtph264paySinkP could not be linked.\n");
		exit(1);
	}
	GstPad* rtph264paySrcPad = gst_element_get_static_pad(rtph264pay, "src");
	GstPad* rtpbinSinkPad = gst_element_request_pad_simple(rtpbin, "send_rtp_sink_%u");

	//GstPad* rtpbinsrcRtcpPad = gst_element_request_pad_simple(rtpbin, "send_rtcp_src_%u");
	if (gst_pad_link(rtph264paySrcPad, rtpbinSinkPad) != GST_PAD_LINK_OK) {
		g_printerr("rtph264paySrcPad and rtpbinSinkPad could not be linked.\n");
		exit(1);
	}
}
