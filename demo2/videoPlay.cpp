#include "compositor.h"
#include <string>
#include <cstring>
#include "helper.h"
using namespace std;

VideoPlay::VideoPlay(string inputName) {
	name = inputName;
	gchar videoConvertName[100];
	buildName(name, "videoconvert", videoConvertName, 100);
	videoConvert = gst_element_factory_make("videoconvert", videoConvertName);

	gchar autoVideoSinkName[100];
	buildName(name, "autovideosink", autoVideoSinkName, 100);
	autoVideoSink = gst_element_factory_make("autovideosink", autoVideoSinkName);
}

void VideoPlay::generatePad() {
	if (!gst_element_link_many(videoConvert, autoVideoSink, NULL)) {
		g_printerr("Elements could not be linked.\n");
		exit(1);
	}

	sinkPad = gst_element_get_static_pad(videoConvert, "sink");
}

void VideoPlay::addToPipeline(GstElement** pipeline) {
	gst_bin_add_many(GST_BIN(*pipeline), videoConvert, autoVideoSink, NULL);
}
