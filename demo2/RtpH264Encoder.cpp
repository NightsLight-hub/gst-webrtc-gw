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
	gst_object_ref(videoConvert);

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
	
	g_object_set(G_OBJECT(rtpbin), "latency", 50, NULL);
	g_object_set(G_OBJECT(x264enc), "bitrate", 4096, NULL);
	g_object_set(G_OBJECT(x264enc), "tune", 4, NULL);
	g_object_set(G_OBJECT(x264enc), "speed-preset", 2, NULL);
	g_object_set(G_OBJECT(rtph264pay), "config-interval", 10, NULL);
	g_object_set(G_OBJECT(rtph264pay), "pt", 96, NULL);
}

void RtpH264Encoder::addToPipeline(GstElement** pipeline) {
	gst_bin_add_many(GST_BIN(*pipeline), videoConvert, x264enc, rtph264pay, rtpbin, NULL);
	if (!gst_element_link_many(videoConvert, x264enc, rtph264pay, NULL)) {
		g_printerr("Elements could not be linked.\n");
		exit(1);
	}
}
