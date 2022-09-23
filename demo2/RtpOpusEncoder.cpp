#include <sstream>
#include "audioCompositor.h"
#include "helper.h"

/// <summary>
/// opusenc -> rtpopuspay -> rtpbin
/// </summary>
RtpOpusEncoder::RtpOpusEncoder(string name) {
	this->name = name;
	gchar opusencName[100];
	buildName(name, "opusenc", opusencName, 100);
	opusenc = gst_element_factory_make("opusenc", opusencName);
	gst_object_ref(opusenc);

	gchar rtpopuspayName[100];
	buildName(name, "rtpopuspay", rtpopuspayName, 100);
	rtpopuspay = gst_element_factory_make("rtpopuspay", rtpopuspayName);
	gst_object_ref(rtpopuspay);

	gchar rtpbinName[100];
	buildName(name, "rtpbin", rtpbinName, 100);
	rtpbin = gst_element_factory_make("rtpbin", rtpbinName);
	gst_object_ref(rtpbin);
}