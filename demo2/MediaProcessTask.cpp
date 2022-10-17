#include "mediaProcessTask.h"
#include <iostream>
#include <functional>

using namespace std;

const int videoSrcPortRangeBegin = 50001;
const int videoSrcPortRangeEnd = 50500;
const int audioSrcPortRangeBegin = 50501;
const int audioSrcPortRangeEnd = 51000;

MediaProcessTask::MediaProcessTask(string name) {
	this->name = name;
}

/// <summary>
/// 初始化，此函数会选择空闲端口占用
/// </summary>
/// <returns></returns>
int MediaProcessTask::init() {
	rtpVideoProcessor = new MultipleRtpVp8Compositor(sinkIP, videoSinkPort);
	rtpAudioProcessor = new MultipleRtpOpusCompositor(sinkIP, audioSinkPort);
	rtpVideoReceiver = new UdpReceiver("videoReceiver", rtpVideoProcessor);
	rtpAudioReceiver = new UdpReceiver("audioReceiver", rtpAudioProcessor);
	cout << "trying to bind a idle port for receiving A/V rtp " << endl;
	int ret = -1;
	bool flag = false;
	for (int i = videoSrcPortRangeBegin; i < videoSrcPortRangeEnd; i++) {
		ret = rtpVideoReceiver->tryInit(i);
		if (ret == 0) {
			cout << "use " << i << " port for receiveing video!" << endl;
			flag = true;
			break;
		}
	}
	if (!flag) {
		cout << "get available port for receiveing video rtp failed!" << endl;
		return 1;
	}
	flag = false;
	for (int i = audioSrcPortRangeBegin; i < audioSrcPortRangeEnd; i++) {
		ret = rtpAudioReceiver->tryInit(i);
		if (ret == 0) {
			cout << "use " << i << " port for receiveing audio!" << endl;
			flag = true;
			break;
		}
	}
	if (!flag) {
		cout << "get available port for receiveing audio rtp failed!" << endl;
		return 1;
	}
	return 0;
}

/// <summary>
/// Rtp 收包线程
/// </summary>
void MediaProcessTask::runRtpVideoReceiver() {
	printf("rtpVideoReceiver started...\n");
	rtpVideoReceiver->start();
}

/// <summary>
/// 启动Gstreamer Rtp Video 解码、编码流水线
/// </summary>
/// <param name="flag"></param>
void MediaProcessTask::runRtpVideoProcessor(atomic<bool>* flag) {
	printf("rtpVideoProcessor started...\n");
	if (0 != rtpVideoProcessor->entrypoint(flag)) {
		cerr << "rtpVideoProcessor stoped abnormally!" << endl;
	}
}

/// <summary>
/// Rtp 收包线程
/// </summary>
void MediaProcessTask::runRtpAudioReceiver() {
	printf("rtpAudioReceiver started...\n");
	rtpAudioReceiver->start();
}
/// <summary>
/// 启动Gstreamer Rtp Audio 解码、编码流水线
/// </summary>
/// <param name="flag"></param>
void MediaProcessTask::runRtpAudioProcessor(atomic<bool>* flag) {
	printf("rtpAudioProcessor started...\n");
	rtpAudioProcessor->entrypoint(flag);
}

/// <summary>
/// 启动任务线程干活
/// </summary>
void MediaProcessTask::start() {
	udpAudioReceiveT = thread(std::bind(&MediaProcessTask::runRtpAudioReceiver, this));
	udpVideoReceiveT = thread(std::bind(&MediaProcessTask::runRtpVideoReceiver, this));
	rtpVideoProcessT = thread(std::bind(&MediaProcessTask::runRtpVideoProcessor, this, &(rtpVideoReceiver->syncFlag)));
	rtpAudioProcessT = thread(std::bind(&MediaProcessTask::runRtpAudioProcessor, this, &(rtpAudioReceiver->syncFlag)));
}

/// <summary>
/// 停止任务，此函数会触发所有任务相关工作线程的退出
/// </summary>
void MediaProcessTask::stop() {
	rtpVideoProcessor->quit();
	rtpVideoReceiver->quit();
	rtpAudioProcessor->quit();
	rtpAudioReceiver->quit();
}
