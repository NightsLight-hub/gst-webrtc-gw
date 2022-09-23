#pragma once
#include <atomic>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include "compositor.h"
#include <string>
using namespace std;

/// <summary>
/// queue ->capsfilter -> rtpopusdepay -> opusdec
/// </summary>
class RtpOpusDecoder {
public:
	string name;
	GstElement* queue, * capsfilter, * rtpOpusDepay, * opusDec;
	RtpOpusDecoder(string name);
};

/// <summary>
/// opusenc -> rtpopuspay -> rtpbin
/// </summary>
class RtpOpusEncoder {
	string name;
	GstElement* opusenc, * rtpopuspay, * rtpbin;
	RtpOpusEncoder(string name);
};

/// <summary>
/// appsrc1 -> queue ->capsfilter -> rtpopusdepay -> opusdec -> videomixer ->opusenc -> rtpopuspay -> rtpbin
/// appsrc1 -> queue ->capsfilter -> rtpopusdepay -> opusdec /
/// </summary>
class MultipleRtpOpusCompositor :public RtpProcessor {
public:
	MultipleRtpOpusCompositor(std::string targetAddress, int targetPort);
	string name;
	GMainLoop* gstreamer_receive_main_loop = NULL;
	GstElement* pipeline;
	GstBus* bus;
	GstElement* appSrcs[10];
	int appSrcCount;
	GstElement* udpsink;
	RtpOpusDecoder* decoders[10];
	int decodersCount = 0;
	RtpOpusEncoder* encoder;
	GstElement* audioMixer;
	std::string targetAddress;
	int targetPort;
	void receivePushBuffer(void* buffer, int len, char type);
	int entrypoint(std::atomic<bool>* flag);
private:
	void addDecoders();
	void mainLoop();
	void createAppSrc();
};