#include <sstream>
#include "audioCompositor.h"
#include "helper.h"

/// <summary>
/// queue ->capsfilter -> rtpopusdepay -> opusdec
/// </summary>
RtpOpusDecoder::RtpOpusDecoder(string name) {
	this->name = name;
	gchar queueName[100];
	buildName(name, "queue", queueName, 100);
	queue = gst_element_factory_make("queue", queueName);
	gst_object_ref(queue);

	gchar capsFilterName[100];
	buildName(name, "cf", capsFilterName, 100);
	capsfilter = gst_element_factory_make("capsfilter", capsFilterName);
	gst_object_ref(capsfilter);

	gchar rtpOpusDepayName[100];
	buildName(name, "rtpopusDepay", rtpOpusDepayName, 100);
	rtpOpusDepay = gst_element_factory_make("rtpopusdepay", rtpOpusDepayName);
	gst_object_ref(rtpOpusDepay);

	gchar opusDecName[100];
	buildName(name, "opusdec", opusDecName, 100);
	opusDec = gst_element_factory_make("opusdec", opusDecName);
	gst_object_ref(opusDec);

	stringstream capstr;
	capstr << "application/x-rtp, payload=96, media=audio, encoding-name=OPUS";
	GstCaps* caps1 = gst_caps_from_string(capstr.str().c_str());
	g_object_set(G_OBJECT(capsfilter), "caps", caps1, NULL);
}
