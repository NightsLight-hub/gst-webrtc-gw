#pragma once

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <Windows.h>
#include <atomic>
#include <string>

class RtpVp8Decoder {
public:
	RtpVp8Decoder(std::string inputName);
	std::string name;
	GstElement* queue;
	GstElement* capsFilter;
	GstElement* rtpvp8depay;
	GstElement* vp8dec;
	GstElement* videoConvert;
public:
	void addToPipeline(GstElement* pipeline);
	void generatePad();
};

class RtpVp9Decoder {
public:
	RtpVp9Decoder(std::string inputName);
	std::string name;
	GstElement* queue;
	GstElement* capsFilter;
	GstElement* rtpvp9depay;
	GstElement* vp9dec;
	GstElement* videoConvert;
public:
	void addToPipeline(GstElement* pipeline);
	void generatePad();
};

/// <summary>
/// videoconvert -> x264enc -> rtph264pay -> rtpbin
/// </summary>
class RtpH264Encoder {
public:
	RtpH264Encoder(std::string inputName);
	std::string name;
	GstElement* videoConvert;
	GstElement* queue;
	GstElement* x264enc;
	GstElement* rtph264pay;
	GstElement* rtpbin;
public:
	void addToPipeline(GstElement** pipeline);
};

class VideoPlay {
public:
	VideoPlay(std::string inputName);
	std::string name;
	GstElement* videoConvert;
	GstElement* autoVideoSink;
	GstPad* sinkPad;
public:
	void addToPipeline(GstElement** pipeline);
	void generatePad();
};

// 多路 rtp vp8 合流，转rtp  h264 发出
// appsrc -> output-selector -> rtpvp8Decoder[0] -> compositor -> rptvp8Encoder -> udpsink
//                              rtpvp8Decoder[1]
class MultipleRtpVp8Sink {
public:
	GMainLoop* gstreamer_receive_main_loop = NULL;
	GstElement* pipeline;
	GstBus* bus;
	GstElement* appSrcs[10];
	int appSrcCount;
	GstElement* outputSelector;
	GstElement* compositor;
	GstElement* udpsink;
	GstElement* udpsinkRTCP;
	RtpH264Encoder* rtpH264Encoder;
	RtpVp8Decoder* rtpVp8Decoders[10];
	RtpVp9Decoder* rtpVp9Decoders[10];
	GstPad** outputSelectorSrcPad[10];
	int rtpvp8DecodersCount = 0;
	int rtpvp9DecodersCount = 0;
	void gstreamer_receive_push_buffer(void* buffer, int len, char type);
	void mainLoop();
	int entrypoint(std::atomic<bool>* flag);
	void createAppSrc();
	RtpVp8Decoder* addRtpVp8Deocoders();
	RtpVp9Decoder* addRtpVp9Deocoders();
};


class MultipleRtpVp8AutoVideoSink {
public:
	GMainLoop* gstreamer_receive_main_loop = NULL;
	GstElement* pipeline;
	GstBus* bus;
	GstElement* appsrc1;
	GstElement* appsrc2;
	GstElement* outputSelector;
	GstElement* compositor;
	GstElement* udpsink;
	GstElement* udpsinkRTCP;
	RtpH264Encoder* rtpH264Encoder;
	RtpVp8Decoder* rtpVp8Decoders[10];
	GstPad** outputSelectorSrcPad[10];
	int rtpvp8DecodersCount = 0;

	void gstreamer_receive_push_buffer(void* buffer, int len, char type);
	void mainLoop();
	int entrypoint(std::atomic<bool>* flag);
	RtpVp8Decoder* addRtpVp8Deocoders();
};

const int WIDTH_360P = 640;
const int HEIGHT_360P = 360;
const int WIDTH_1080P = 1280;
const int HEIGHT_1080P = 1080;

void exitP();