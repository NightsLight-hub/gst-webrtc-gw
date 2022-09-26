#pragma once
#include <Windows.h>
#include "compositor.h"
#include <string>

class UdpReceiver :public RtpReceiver {
public:
	SOCKET recvSocket = NULL;

	void start();
	UdpReceiver(std::string name, RtpProcessor* rtpProcessor, int port);
};