#pragma once

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <Windows.h>
#include <atomic>

// rtp vp8 ֱ�Ӳ���
class RtpVp8VideoSink {
public:
	GMainLoop* gstreamer_receive_main_loop = NULL;
	GstElement* pipeline;
	void gstreamer_receive_push_buffer(void* buffer, int len);
	void mainLoop();
	int entrypoint(std::atomic<bool>* flag);
};

// rtp vp8 ת rtp h264 ����
class RtpForward {
public:
	GMainLoop* gstreamer_receive_main_loop = NULL;
	GstElement* pipeline;
	void gstreamer_receive_push_buffer(void* buffer, int len);
	void mainLoop();
	int entrypoint(std::atomic<bool>* flag);
};

// rtp vp8 ������·Ȼ�������Ȼ�󲥷�
class RtpVp8Composite {
public:
	GMainLoop* gstreamer_receive_main_loop = NULL;
	GstElement* pipeline;
	void gstreamer_receive_push_buffer(void* buffer, int len);
	void mainLoop();
	int entrypoint(std::atomic<bool>* flag);
};


// rtp vp8 ������·������תrtp  h264 ����
class RtpVp8AppSink {
public:
	GMainLoop* gstreamer_receive_main_loop = NULL;
	GstElement* pipeline;
	void gstreamer_receive_push_buffer(void* buffer, int len);
	void mainLoop();
	int entrypoint(std::atomic<bool>* flag);
};

const int WIDTH_640P = 640;
const int HEIGHT_640P = 320;
const int WIDTH_1080P = 1080;
const int HEIGHT_1080P = 720;

int tutorial_8(std::atomic<bool>* flag);
void gstreamer_receive_push_buffer(void* buffer, int len);
void exitP();