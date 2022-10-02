#include "videoCompositor.h"
#include "helper.h"
#include <sstream>
using namespace std;

//class RtpVp8Encoder {
//public:
//	RtpVp8Encoder(std::string name);
//	std::string name;
//	GstElement* vp8enc;
//	GstElement* rtpvp8pay;
//	GstElement* rtpbin;
//	GstPad* srcPad;
//	GstPad* sinkPad;
//};

// more bitrate, more bandwith, lower cpu usage
static const int BITRATE = 2048 * 5;
// 10ms buffer 
static const int LATENCY = 10;

RtpVp8Encoder::RtpVp8Encoder(string inputName) {
	name = inputName;
	gchar videoConvertName[100];
	buildName(name, "videoconvert", videoConvertName, 100);
	videoConvert = gst_element_factory_make("videoconvert", videoConvertName);
	gst_object_ref(videoConvert);

	gchar queueName[100];
	buildName(name, "queue", queueName, 100);
	queue = gst_element_factory_make("queue", queueName);
	gst_object_ref(queue);

	gchar vp8EncName[100];
	buildName(name, "vp8enc", vp8EncName, 100);
	vp8enc = gst_element_factory_make("vp8enc", vp8EncName);
	//必须设置以下参数，否则无法让浏览器识别正确的vp8流
	g_object_set(G_OBJECT(vp8enc), "error-resilient", 2, NULL);
	g_object_set(G_OBJECT(vp8enc), "keyframe-max-dist", 10, NULL);
	g_object_set(G_OBJECT(vp8enc), "auto-alt-ref", TRUE, NULL);
	g_object_set(G_OBJECT(vp8enc), "deadline", 1, NULL);

	gst_object_ref(vp8enc);

	gchar rtpvp8payName[100];
	buildName(name, "rtpvp8pay", rtpvp8payName, 100);
	rtpvp8pay = gst_element_factory_make("rtpvp8pay", rtpvp8payName);
	gst_object_ref(rtpvp8pay);

	gchar rtpbinName[100];
	buildName(name, "rtpbin", rtpbinName, 100);
	rtpbin = gst_element_factory_make("rtpbin", rtpbinName);
	gst_object_ref(rtpbin);

	g_object_set(G_OBJECT(rtpbin), "latency", LATENCY, NULL);
	g_object_set(G_OBJECT(rtpvp8pay), "pt", 96, NULL);
	g_object_set(G_OBJECT(rtpvp8pay), "mtu", 1400, NULL);

}

void RtpVp8Encoder::addToPipeline(GstElement** pipeline) {
	gst_bin_add_many(GST_BIN(*pipeline), videoConvert, queue, vp8enc, rtpvp8pay, rtpbin, NULL);
	if (!gst_element_link_many(videoConvert, queue, vp8enc, NULL)) {
		g_printerr("Elements could not be linked.\n");
		exit(1);
	}
	GstPad* encSrcP = gst_element_get_static_pad(vp8enc, "src");
	GstPad* rtpvp8paySinkP = gst_element_get_static_pad(rtpvp8pay, "sink");
	if (gst_pad_link(encSrcP, rtpvp8paySinkP) != GST_PAD_LINK_OK) {
		g_printerr("RtpVp8Encoder:: encSrcP, rtpvp8paySinkP could not be linked.\n");
		exit(1);
	}
	GstPad* rtpvp8paySrcPad = gst_element_get_static_pad(rtpvp8pay, "src");
	GstPad* rtpbinSinkPad = gst_element_request_pad_simple(rtpbin, "send_rtp_sink_%u");

	//GstPad* rtpbinsrcRtcpPad = gst_element_request_pad_simple(rtpbin, "send_rtcp_src_%u");
	if (gst_pad_link(rtpvp8paySrcPad, rtpbinSinkPad) != GST_PAD_LINK_OK) {
		g_printerr("rtph264paySrcPad and rtpbinSinkPad could not be linked.\n");
		exit(1);
	}
}
