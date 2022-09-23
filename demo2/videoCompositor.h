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

// ��· rtp vp8 ������תrtp  h264 ����
// appsrc -> output-selector -> rtpvp8Decoder[0] -> compositor -> rptvp8Encoder -> udpsink
//                              rtpvp8Decoder[1]
class MultipleRtpVp8Sink :public RtpProcessor {
public:
	MultipleRtpVp8Sink(std::string targetAddress, int targetPort);
	GMainLoop* gstreamer_receive_main_loop = NULL;
	GstElement* pipeline;
	GstBus* bus;
	GstElement* appSrcs[10];
	GstElement* outputSelector;
	GstElement* compositor;
	GstElement* udpsink;
	GstElement* udpsinkRTCP;
	RtpH264Encoder* rtpH264Encoder;
	RtpVp8Decoder* rtpVp8Decoders[10];
	RtpVp9Decoder* rtpVp9Decoders[10];
	// ���͵�Ŀ���ַ
	std::string targetAddress;
	// Ŀ��˿�
	int targetPort;
	// �����յ���rtp��
	void receivePushBuffer(void* buffer, int len, char type);
	// ��ں���
	int entrypoint(std::atomic<bool>* flag);
private:
	int appSrcCount;
	int rtpvp8DecodersCount = 0;
	int rtpvp9DecodersCount = 0;
	// glibc ѭ��
	void mainLoop();
	// ����һ��vp8������
	RtpVp8Decoder* addRtpVp8Deocoders();
	// ����pv9������
	RtpVp9Decoder* addRtpVp9Deocoders();
	// ����һ������Դ
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