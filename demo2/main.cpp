#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "videoCompositor.h"
#include "audioCompositor.h"
#include "rtpReceiver.h"
#include <thread>
#include <atomic>

using namespace std;

RtpProcessor* rtpVideoProcessor;
RtpProcessor* rtpAudioProcessor;
RtpReceiver* rtpVideoReceiver;
RtpReceiver* rtpAudioReceiver;

void static runRtpVideoReceiver() {
	printf("rtpVideoReceiver started...\n");
	rtpVideoReceiver->start();
}

void static runRtpVideoProcessor(atomic<bool>* flag) {
	printf("rtpVideoProcessor started...\n");
	rtpVideoProcessor->entrypoint(flag);
}

void static runRtpAudioReceiver() {
	printf("rtpAudioReceiver started...\n");
	rtpAudioReceiver->start();
}

void static runRtpAudioProcessor(atomic<bool>* flag) {
	printf("rtpAudioProcessor started...\n");
	rtpAudioProcessor->entrypoint(flag);
}

int main()
{
	// rtp vp8 解码 - 合流 - 下沉
	rtpVideoProcessor = new MultipleRtpVp8Compositor("127.0.0.1", 5004);
	//rtpVideoProcessor = new MultipleRtpVp8Compositor("127.0.0.1", 6666);
	// 
	// rtp opus 解码 - 合流 - 下沉
	//rtpAudioProcessor = new MultipleRtpOpusCompositor("172.18.39.162", 51001);
	// udp接收器 3000端口
	rtpVideoReceiver = new UdpReceiver("videoReceiver", rtpVideoProcessor, 30000);
	// udp接收器 3001端口
	//rtpAudioReceiver = new UdpReceiver("audioReceiver", rtpAudioProcessor, 3001);

	//thread udpAudioReceive(runRtpAudioReceiver);
	//thread rtpAudioProcess(runRtpAudioProcessor, &(rtpAudioReceiver->syncFlag));
	thread udpVideoReceive(runRtpVideoReceiver);
	thread rtpVideoProcess(runRtpVideoProcessor, &(rtpVideoReceiver->syncFlag));

	//udpAudioReceive.join();
	udpVideoReceive.join();
	//rtpAudioProcess.join();
	rtpVideoProcess.join();

	delete(rtpVideoProcessor);
	delete(rtpAudioProcessor);
	delete(rtpVideoReceiver);
	delete(rtpAudioReceiver);

	return 0;
}