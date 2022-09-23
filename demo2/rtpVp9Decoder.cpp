#include "videoCompositor.h"
#include "helper.h"
#include <sstream>

//class Rtpvp9Decoder {
//public:
//	Rtpvp9Decoder(std::string name);
//	std::string name;
//	GstElement* capsFilter;
//	GstElement* rtpvp9depay;
//	GstElement* vp9dec;
//	GstElement* videoConvert;
//	GstPad* srcPad;
//	GstPad* sinkPad;
//};
using namespace std;
RtpVp9Decoder::RtpVp9Decoder(string inputName) {
	name = inputName;
	gchar queueName[100];
	buildName(name, "queue", queueName, 100);
	queue = gst_element_factory_make("queue", queueName);
	gst_object_ref(queue);

	gchar capsFilterName[100];
	buildName(name, "cf", capsFilterName, 100);
	capsFilter = gst_element_factory_make("capsfilter", capsFilterName);
	gst_object_ref(capsFilter);

	gchar rtpvp9depayName[100];
	buildName(name, "rtpvp9depay", rtpvp9depayName, 100);
	rtpvp9depay = gst_element_factory_make("rtpvp9depay", rtpvp9depayName);
	gst_object_ref(rtpvp9depay);

	gchar vp9decName[100];
	buildName(name, "vp9dec", vp9decName, 100);
	vp9dec = gst_element_factory_make("vp9dec", vp9decName);
	gst_object_ref(vp9dec);

	gchar videoConvertName[100];
	buildName(name, "videoconvert", videoConvertName, 100);
	videoConvert = gst_element_factory_make("videoconvert", videoConvertName);
	gst_object_ref(videoConvert);

	stringstream capstr;
	capstr << "application/x-rtp, payload=96, encoding-name=VP9-DRAFT-IETF-01";
	GstCaps* caps1 = gst_caps_from_string(capstr.str().c_str());
	g_object_set(G_OBJECT(capsFilter), "caps", caps1, NULL);
}

void RtpVp9Decoder::addToPipeline(GstElement* pipeline) {
	gst_bin_add_many(GST_BIN(pipeline), queue, capsFilter, rtpvp9depay, vp9dec, videoConvert, NULL);
}

void RtpVp9Decoder::generatePad() {

}


