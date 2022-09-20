#include "compositor.h"
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

RtpH264Encoder::RtpH264Encoder(string inputName) {
	name = inputName;
	gchar videoConvertName[100];
	buildName(name, "videoconvert", videoConvertName, 100);
	videoConvert = gst_element_factory_make("videoconvert", videoConvertName);

	gchar x264EncName[100];
	buildName(name, "x264enc", x264EncName, 100);
	x264enc = gst_element_factory_make("x264enc", x264EncName);

	gchar rtph264payName[100];
	buildName(name, "rtph264pay", rtph264payName, 100);
	rtph264pay = gst_element_factory_make("rtph264pay", rtph264payName);

	gchar rtpbinName[100];
	buildName(name, "rtpbin", rtpbinName, 100);
	rtpbin = gst_element_factory_make("rtpbin", rtpbinName);
	
	g_object_set(G_OBJECT(rtpbin), "latency", 50, NULL);
	g_object_set(G_OBJECT(x264enc), "bitrate", 4096, NULL);
	g_object_set(G_OBJECT(x264enc), "tune", 4, NULL);
	g_object_set(G_OBJECT(x264enc), "speed-preset", 2, NULL);
	g_object_set(G_OBJECT(rtph264pay), "config-interval", 10, NULL);
	g_object_set(G_OBJECT(rtph264pay), "pt", 96, NULL);
}

void RtpH264Encoder::addToPipeline(GstElement* pipeline) {
	gst_bin_add_many(GST_BIN(pipeline), videoConvert, x264enc, rtph264pay, rtpbin);
}

void RtpH264Encoder::generatePad() {
	if (!gst_element_link_many(videoConvert, x264enc, rtph264pay, NULL)) {
		g_printerr("Elements could not be linked.\n");
		exit(1);
	}

	GstPad* rtph264paySrcPad = gst_element_get_static_pad(rtph264pay, "src");
	GstPad* rtpbinSinkPad = gst_element_request_pad_simple(rtpbin, "send_rtp_sink_%u");
	GstPad* rtpbinsrcPad = gst_element_get_static_pad(rtpbin, "send_rtp_src_0");
	//GstPad* rtpbinsrcRtcpPad = gst_element_request_pad_simple(rtpbin, "send_rtcp_src_%u");
	if (gst_pad_link(rtph264paySrcPad, rtpbinSinkPad) != GST_PAD_LINK_OK) {
		g_printerr("rtph264paySrcPad and rtpbinSinkPad could not be linked.\n");
	}
	srcPad = rtpbinsrcPad;
	sinkPad = gst_element_get_static_pad(videoConvert, "sink");

	gst_element_release_request_pad(rtpbin, rtpbinSinkPad);
	gst_object_unref(rtph264paySrcPad);
}
