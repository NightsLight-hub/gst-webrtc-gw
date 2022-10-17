#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include "rtpReceiver.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>
#pragma comment ( lib,"WS2_32.lib" )  

using namespace std;

UdpReceiver::UdpReceiver(string name, RtpProcessor* rtpProcessor) {
	this->name = name;
	this->rtpProcessor = rtpProcessor;
}

int UdpReceiver::tryInit(int port) {
	this->srcPort = port;

	WSAData wasData;
	SOCKADDR_IN receiverAddr;
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
		ret = WSAGetLastError();
		printf("bind error with ErrorNum %d\n", ret);
	}
	return ret;
}

void UdpReceiver::start() {
	char recvBuf[1501] = { 0 };
	int recvBufLength = 1500;
	SOCKADDR_IN senderAddr;
	memset(&senderAddr, 0, sizeof(senderAddr));
	int senderAddrLength = sizeof(senderAddr);
	char senderInfo[30] = { 0 };
	int ret = -1;
	while (!syncFlag.load()) {
		this_thread::sleep_for(chrono::milliseconds(100));
	}
	// rtpProcessor ������ɣ���ʼ���յ��İ��³�
	try {
		while (syncFlag.load() && active) {
			ret = recvfrom(recvSocket, recvBuf, recvBufLength, 0, (SOCKADDR*)&senderAddr, &senderAddrLength);
			if (ret > 10) {
				void* rtpStartPoint = (char*)recvBuf + 10;
				char* sp = recvBuf;
				rtpProcessor->receivePushBuffer(rtpStartPoint, ret - 10, sp[0]);
			}
		}
	}
	catch (exception e) {
		cerr << "udpReceiver return error, " << e.what() << endl;
	}
	
	closesocket(recvSocket);
}

void UdpReceiver::quit() {
	active = false;
	this_thread::sleep_for(std::chrono::milliseconds(1000));
	// ��ҪcloseSocket����Ϊudpreceiver���ܻ���recvFrom��������
	// ��Զ�޷���֪active �������ر�recvSocket����ǿ���˳�
	closesocket(recvSocket);
}
