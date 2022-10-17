#pragma once
#include <Windows.h>
#include "compositor.h"
#include <string>

class UdpReceiver :public RtpReceiver {
public:
	SOCKET recvSocket = NULL;
	bool active = true;
	int tryInit(int port);
	void start();
	UdpReceiver(std::string name, RtpProcessor* rtpProcessor);
	void quit();
};