#pragma once
#include <Windows.h>
#include "compositor.h"

class UdpReceiver :public RtpReceiver {
public:
	SOCKET recvSocket = NULL;

	void start();
	UdpReceiver(RtpProcessor* rtpProcessor, int port);
};