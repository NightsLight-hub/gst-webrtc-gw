#pragma once

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <Windows.h>
#include <atomic>
#include <string>

// 多路 rtp vp8 合流，转rtp  h264 发出
// appsrc -> output-selector -> rtpvp8Decoder[0] -> compositor -> rptvp8Encoder -> udpsink
//                              rtpvp8Decoder[1]
class MultipleRtpVp8Sink {
public:
	GMainLoop* gstreamer_receive_main_loop = NULL;
	GstElement* pipeline;
	GstBus* bus;
	GstElement* appsrc, *outputSelector, *compositor, *udpsink, *udpsinkRTCP;
	RtpH264Encoder* rtpH264Encoder;
	RtpVp8Decoder* rtpVp8Decoders[10] = { NULL };
	GstPad* outputSelectorSrcPad[10];
	int rtpvp8DecodersCount = 0;

	void gstreamer_receive_push_buffer(void* buffer, int len, char type);
	void mainLoop();
	int entrypoint(std::atomic<bool>* flag);
	void addRtpVp8Deocoders();
};

class RtpVp8Decoder {
public:
	RtpVp8Decoder(std::string inputName);
	std::string name;
	GstElement* capsFilter;
	GstElement* rtpvp8depay;
	GstElement* vp8dec;
	GstElement* videoConvert;
	GstPad *srcPad;
	GstPad *sinkPad;
public:
	void addToPipeline(GstElement* pipeline);
	void generatePad();
};

class RtpH264Encoder {
public:
	RtpH264Encoder(std::string inputName);
	std::string name;
	GstElement* videoConvert;
	GstElement* x264enc;
	GstElement* rtph264pay;
	GstElement* rtpbin;
	GstPad* srcPad;
	GstPad* sinkPad;
public:
	void addToPipeline(GstElement* pipeline);
	void generatePad();
};

const int WIDTH_640P = 640;
const int HEIGHT_640P = 320;
const int WIDTH_1080P = 1080;
const int HEIGHT_1080P = 720;

void exitP();