#pragma once
#include <atomic>
#include <string>

class RtpProcessor {
public:
	virtual void receivePushBuffer(void* buffer, int len, char type) = 0;
	virtual int entrypoint(std::atomic<bool>* flag) = 0;
};

class RtpReceiver {
public:
	std::string name;
	RtpProcessor* rtpProcessor;
	std::atomic<bool> syncFlag = false;
	int srcPort = 3000;
	virtual void start() = 0;
};
