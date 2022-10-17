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
	// 初始化
	WSAStartup(MAKEWORD(2, 2), &wasData);

	// 创建套接字
	// 套接字类型—：UDP/IP-SOCK_DGRAM
	// 协议：UDP-IPPROTO_UDP
	recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (SOCKET_ERROR == recvSocket) {
		printf("Create Socket Error!");
	}
	// 创建一个SOCKADDR_IN结构，指定接收端地址信息
	receiverAddr.sin_family = AF_INET;   // 使用IP地址族
	receiverAddr.sin_port = htons(this->srcPort); // 端口号
	receiverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// bind将地址信息和套接字关联
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
	// rtpProcessor 启动完成，开始把收到的包下沉
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
	// 需要closeSocket，因为udpreceiver可能会在recvFrom上阻塞，
	// 永远无法感知active 条件，关闭recvSocket让它强制退出
	closesocket(recvSocket);
}
