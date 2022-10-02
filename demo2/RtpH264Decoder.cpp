#include "videoCompositor.h"
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
RtpH264Decoder::RtpH264Decoder(string inputName) {
	name = inputName;
	gchar queueName[100];
	buildName(name, "queue", queueName, 100);
	queue = gst_element_factory_make("queue", queueName);
	gst_object_ref(queue);

	gchar capsFilterName[100];
	buildName(name, "cf", capsFilterName, 100);
	capsFilter = gst_element_factory_make("capsfilter", capsFilterName);
	gst_object_ref(capsFilter);

	gchar rtpvp8depayName[100];
	buildName(name, "rtpvp8depay", rtpvp8depayName, 100);
	rtph264depay = gst_element_factory_make("rtph264depay", rtpvp8depayName);
	gst_object_ref(rtph264depay);

	gchar vp8decName[100];
	buildName(name, "vp8dec", vp8decName, 100);
	x264dec = gst_element_factory_make("avdec_h264", vp8decName);
	gst_object_ref(x264dec);

	gchar videoConvertName[100];
	buildName(name, "videoconvert", videoConvertName, 100);
	videoConvert = gst_element_factory_make("videoconvert", videoConvertName);
	gst_object_ref(videoConvert);

	stringstream capstr;
	capstr << "application/x-rtp, payload=96, encoding-name=H264";
	GstCaps* caps1 = gst_caps_from_string(capstr.str().c_str());
	g_object_set(G_OBJECT(capsFilter), "caps", caps1, NULL);
}

void RtpH264Decoder::addToPipeline(GstElement* pipeline) {
	gst_bin_add_many(GST_BIN(pipeline), queue, capsFilter, rtph264depay, x264dec, videoConvert, NULL);
}

void RtpH264Decoder::generatePad() {

}


