#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include "rtpReceiver.h"
#include <thread>
#include <chrono>
#include <atomic>
#pragma comment ( lib,"WS2_32.lib" )  

using namespace std;

static void exitP(SOCKET recvSocket);

UdpReceiver::UdpReceiver(string name, RtpProcessor* rtpProcessor, int port) {
	this->name = name;
	this->rtpProcessor = rtpProcessor;
	this->srcPort = port;
}

void UdpReceiver::start() {
	WSAData wasData;
	SOCKADDR_IN receiverAddr;
	char recvBuf[1501] = { 0 };
	int recvBufLength = 1500;

	SOCKADDR_IN senderAddr;
	memset(&senderAddr, 0, sizeof(senderAddr));
	int senderAddrLength = sizeof(senderAddr);
	char senderInfo[30] = { 0 };

	int ret = -1;

	// ��ʼ��
	WSAStartup(MAKEWORD(2, 2), &wasData);

	// �����׽���
	// �׽������͡���UDP/IP-SOCK_DGRAM
	// Э�飺UDP-IPPROTO_UDP
	recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (SOCKET_ERROR == recvSocket) {
		printf("Create Socket Error!");

	}

	// ����һ��SOCKADDR_IN�ṹ��ָ�����ն˵�ַ��Ϣ
	receiverAddr.sin_family = AF_INET;   // ʹ��IP��ַ��
	receiverAddr.sin_port = htons(this->srcPort); // �˿ں�
	receiverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// bind����ַ��Ϣ���׽��ֹ���
	ret = bind(recvSocket, (SOCKADDR*)&receiverAddr, sizeof(receiverAddr));
	if (0 != ret) {
		printf("bind error with ErrorNum %d\n", WSAGetLastError());
		exitP(recvSocket);
	}

	// �ȴ�rtpProcessor ��ʼ������
	while (!syncFlag.load()) {
		this_thread::sleep_for(chrono::milliseconds(100));
	}
	// rtpProcessor ������ɣ���ʼ���յ��İ��³�
	while (syncFlag.load()) {
		ret = recvfrom(recvSocket, recvBuf, recvBufLength, 0, (SOCKADDR*)&senderAddr, &senderAddrLength);
		if (ret > 10) {
			void* rtpStartPoint = (char*)recvBuf + 10;
			char* sp = recvBuf;
			rtpProcessor->receivePushBuffer(rtpStartPoint, ret - 10, sp[0]);
		}
	}
	exitP(recvSocket);
}

static void exitP(SOCKET recvSocket) {
	closesocket(recvSocket);
	system("pause");
	exit(0);
}

