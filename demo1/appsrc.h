#pragma once

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <Windows.h>
#include <atomic>

int tutorial_8(std::atomic<bool>* flag);
void gstreamer_receive_push_buffer(void* buffer, int len);
void exitP();