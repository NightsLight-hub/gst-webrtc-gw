#include <sstream>
#include "audioCompositor.h"
#include "helper.h"

static const int LATENCY = 10;
/// <summary>
/// opusenc -> rtpopuspay -> rtpbin
/// </summary>
/// 
RtpG722Encoder::RtpG722Encoder(string name) {
	this->name = name;
	gchar audioConvertName[100];
	buildName(name, "audioconvert", audioConvertName, 100);
	audioConvert = gst_element_factory_make("audioconvert", audioConvertName);
	gst_object_ref(audioConvert);

	gchar avenc_g722Name[100];
	buildName(name, "avenc_g722", avenc_g722Name, 100);
	avencG722 = gst_element_factory_make("avenc_g722", avenc_g722Name);
	gst_object_ref(avencG722);

	gchar rtpG722payName[100];
	buildName(name, "rtpg722pay", rtpG722payName, 100);
	rtpg722pay = gst_element_factory_make("rtpg722pay", rtpG722payName);
	gst_object_ref(rtpg722pay);

	//g_object_set(G_OBJECT(rtpopuspay), "config-interval", 10, NULL);
	g_object_set(G_OBJECT(rtpg722pay), "pt", 9, NULL);
	//g_object_set(G_OBJECT(rtpopuspay), "dtx", TRUE, NULL);

	gchar rtpbinName[100];
	buildName(name, "rtpbin", rtpbinName, 100);
	rtpbin = gst_element_factory_make("rtpbin", rtpbinName);
	gst_object_ref(rtpbin);

	g_object_set(G_OBJECT(rtpbin), "latency", LATENCY, NULL);
}