#pragma once
#include <string>
#include <vector>
#include "rtpReceiver.h"
#include <thread>
#include <stdlib.h>
#include "videoCompositor.h"
#include "audioCompositor.h"
#include "rtpReceiver.h"

using namespace std;

class MediaProcessTask {
public:
	MediaProcessTask(std::string name);
	int init();
	void start();
	void stop();
public:
	std::string name;
	string sinkIP;
	int audioSinkPort;
	int videoSinkPort;
	RtpProcessor* rtpVideoProcessor;
	RtpProcessor* rtpAudioProcessor;
	RtpReceiver* rtpVideoReceiver;
	RtpReceiver* rtpAudioReceiver;

	thread udpAudioReceiveT;
	thread rtpAudioProcessT;
	thread udpVideoReceiveT;
	thread rtpVideoProcessT;

private:
	void runRtpVideoReceiver();
	void runRtpVideoProcessor(atomic<bool>* flag);
	void runRtpAudioReceiver();
	void runRtpAudioProcessor(atomic<bool>* flag);
};

class MediaProcessManager {
public:
	static MediaProcessManager* getInstance();
public:
	vector<MediaProcessTask*> tasks;
	MediaProcessTask* addNewTask(string name, string sinkIP, int audioSinkPort, int videoSinkPort);
	void removeTask(string name);

private:
	MediaProcessManager();
	~MediaProcessManager();
	MediaProcessManager(const MediaProcessManager& mpm);
	const MediaProcessManager& operator=(const MediaProcessManager& mpm);
	static MediaProcessManager *instance;
};