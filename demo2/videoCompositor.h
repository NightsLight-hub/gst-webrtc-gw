#pragma once

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <atomic>
#include <string>
#include "compositor.h"

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

class RtpH264Decoder {
public:
	RtpH264Decoder(std::string inputName);
	std::string name;
	GstElement* queue;
	GstElement* capsFilter;
	GstElement* rtph264depay;
	GstElement* x264dec;
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

class RtpEncoder {
public:
	std::string name;
	GstElement* videoConvert;
	GstElement* queue;
	GstElement* rtpbin;
public:
	virtual void addToPipeline(GstElement** pipeline) = 0;
};

/// <summary>
/// videoconvert -> x264enc -> rtph264pay -> rtpbin
/// </summary>
class RtpH264Encoder: public RtpEncoder {
public:
	RtpH264Encoder(std::string inputName);
	GstElement* x264enc;
	GstElement* rtph264pay;
	void addToPipeline(GstElement** pipeline);
};

class RtpVp8Encoder: public RtpEncoder {
public:
	RtpVp8Encoder(std::string inputName);
	GstElement* vp8enc;
	GstElement* rtpvp8pay;
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
class MultipleRtpVp8Compositor :public RtpProcessor {
public:
	MultipleRtpVp8Compositor(std::string targetAddress, int targetPort);
	GMainLoop* gstreamer_receive_main_loop = NULL;
	GstElement* pipeline;
	GstBus* bus;
	GstElement* appSrcs[10];
	GstElement* outputSelector;
	GstElement* compositor;
	GstElement* udpsink;
	GstElement* udpsinkRTCP;
	RtpEncoder* rtpEncoder;
	RtpVp8Decoder* rtpVp8Decoders[10];
	RtpVp9Decoder* rtpVp9Decoders[10];
	RtpH264Decoder* rtpH264Decoders[10];
	// 发送的目标地址
	std::string targetAddress;
	// 目标端口
	int targetPort;
	// 插入收到的rtp包
	void receivePushBuffer(void* buffer, int len, char type);
	// 入口函数
	int entrypoint(std::atomic<bool>* flag);
private:
	int appSrcCount;
	int rtpvp8DecodersCount = 0;
	int rtpvp9DecodersCount = 0;
	int rtph264DecodersCount = 0;

	// glibc 循环
	void mainLoop();
	// 添加一个vp8解码器
	RtpVp8Decoder* addRtpVp8Deocoders();
	// 添加pv9解码器
	RtpVp9Decoder* addRtpVp9Deocoders();

	RtpH264Decoder* addRtpH264Deocoders();

	// 创建一个数据源
	void createAppSrc();
};


class MultipleRtpVp8AutoVideoSink : public RtpProcessor {
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
	void receivePushBuffer(void* buffer, int len, char type);
	int entrypoint(std::atomic<bool>* flag);
	RtpVp8Decoder* addRtpVp8Deocoders();
private:
	void mainLoop();
};

const int WIDTH_360P = 640;
const int HEIGHT_360P = 360;
const int WIDTH_1080P = 1280;
const int HEIGHT_1080P = 1080;