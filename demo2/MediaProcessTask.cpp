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
/// ��ʼ�����˺�����ѡ����ж˿�ռ��
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
/// Rtp �հ��߳�
/// </summary>
void MediaProcessTask::runRtpVideoReceiver() {
	printf("rtpVideoReceiver started...\n");
	rtpVideoReceiver->start();
}

/// <summary>
/// ����Gstreamer Rtp Video ���롢������ˮ��
/// </summary>
/// <param name="flag"></param>
void MediaProcessTask::runRtpVideoProcessor(atomic<bool>* flag) {
	printf("rtpVideoProcessor started...\n");
	if (0 != rtpVideoProcessor->entrypoint(flag)) {
		cerr << "rtpVideoProcessor stoped abnormally!" << endl;
	}
}

/// <summary>
/// Rtp �հ��߳�
/// </summary>
void MediaProcessTask::runRtpAudioReceiver() {
	printf("rtpAudioReceiver started...\n");
	rtpAudioReceiver->start();
}
/// <summary>
/// ����Gstreamer Rtp Audio ���롢������ˮ��
/// </summary>
/// <param name="flag"></param>
void MediaProcessTask::runRtpAudioProcessor(atomic<bool>* flag) {
	printf("rtpAudioProcessor started...\n");
	rtpAudioProcessor->entrypoint(flag);
}

/// <summary>
/// ���������̸߳ɻ�
/// </summary>
void MediaProcessTask::start() {
	udpAudioReceiveT = thread(std::bind(&MediaProcessTask::runRtpAudioReceiver, this));
	udpVideoReceiveT = thread(std::bind(&MediaProcessTask::runRtpVideoReceiver, this));
	rtpVideoProcessT = thread(std::bind(&MediaProcessTask::runRtpVideoProcessor, this, &(rtpVideoReceiver->syncFlag)));
	rtpAudioProcessT = thread(std::bind(&MediaProcessTask::runRtpAudioProcessor, this, &(rtpAudioReceiver->syncFlag)));
}

/// <summary>
/// ֹͣ���񣬴˺����ᴥ������������ع����̵߳��˳�
/// </summary>
void MediaProcessTask::stop() {
	rtpVideoProcessor->quit();
	rtpVideoReceiver->quit();
	rtpAudioProcessor->quit();
	rtpAudioReceiver->quit();
}
