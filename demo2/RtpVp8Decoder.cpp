#include "compositor.h"
#include "helper.h"
#include <sstream>

//class RtpVp8Decoder {
//public:
//	RtpVp8Decoder(std::string name);
//	std::string name;
//	GstElement* capsFilter;
//	GstElement* rtpvp8depay;
//	GstElement* vp8dec;
//	GstElement* videoConvert;
//	GstPad* srcPad;
//	GstPad* sinkPad;
//};
using namespace std;
RtpVp8Decoder::RtpVp8Decoder(string inputName) {
	name = inputName;
	gchar capsFilterName[100];
	buildName(name, "cf", capsFilterName, 100);
	capsFilter = gst_element_factory_make("capsfilter", capsFilterName);

	gchar rtpvp8depayName[100];
	buildName(name, "rtpvp8depay", rtpvp8depayName, 100);
	rtpvp8depay = gst_element_factory_make("rtpvp8depay", rtpvp8depayName);

	gchar vp8decName[100];
	buildName(name, "vp8dec", vp8decName, 100);
	vp8dec = gst_element_factory_make("vp8dec", vp8decName);


	gchar videoConvertName[100];
	buildName(name, "videoconvert", videoConvertName, 100);
	videoConvert = gst_element_factory_make("videoconvert", videoConvertName);

	stringstream capstr;
	capstr << "application/x-rtp, payload=96, encoding-name=VP8-DRAFT-IETF-01, width=" << WIDTH_640P << ", height=" << HEIGHT_640P;
	GstCaps* caps1 = gst_caps_from_string(capstr.str().c_str());
	g_object_set(G_OBJECT(capsFilter), "caps", caps1, NULL);
}

void RtpVp8Decoder::addToPipeline(GstElement* pipeline) {
	gst_bin_add_many(GST_BIN(pipeline), capsFilter, rtpvp8depay, vp8dec, videoConvert);
}

void RtpVp8Decoder::generatePad() {
	if (!gst_element_link_many(capsFilter, rtpvp8depay, vp8dec, videoConvert, NULL)) {
		g_printerr("Elements could not be linked.\n");
		exit(1);
	}
	sinkPad = gst_element_get_static_pad(capsFilter, "sink");
	srcPad = gst_element_get_static_pad(videoConvert, "src");
}


