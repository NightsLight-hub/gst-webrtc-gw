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
	// rtp vp8 ���� - ���� - �³�
	rtpVideoProcessor = new MultipleRtpVp8Sink("172.18.39.162", 51000);
	// rtp opus ���� - ���� - �³�
	rtpAudioProcessor = new MultipleRtpOpusCompositor("172.18.39.162", 51000);
	// udp������ 3000�˿�
	rtpVideoReceiver = new UdpReceiver(rtpVideoProcessor, 3000);
	// udp������ 3001�˿�
	rtpAudioReceiver = new UdpReceiver(rtpAudioProcessor, 3001);
	
	thread udpAudioReceive(runRtpAudioReceiver);
	thread udpVideoReceive(runRtpVideoReceiver);
	thread rtpAudioProcess(runRtpAudioProcessor, &(rtpAudioReceiver->syncFlag));
	thread rtpVideoProcess(runRtpVideoProcessor, &(rtpVideoReceiver->syncFlag));

	udpAudioReceive.join();
	udpVideoReceive.join();
	rtpAudioProcess.join();
	rtpVideoProcess.join();

	delete(rtpVideoProcessor);
	delete(rtpAudioProcessor);
	delete(rtpVideoReceiver);
	delete(rtpAudioReceiver);

	return 0;
}