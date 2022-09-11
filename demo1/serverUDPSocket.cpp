#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <iostream>
#include "appsrc.h"
#include <thread>
#include <chrono>
#include <atomic>
#pragma comment ( lib,"WS2_32.lib" )  


using namespace std;
#define RECEIVER_ADDRESS "127.0.0.1"
#define PORT 3000

SOCKET recvSocket = NULL;
atomic<bool> syncFlag = false;

void rtpvp8() {
	tutorial_8(&syncFlag);
}


int main()
{
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
	receiverAddr.sin_port = htons(PORT); // �˿ں�
	receiverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// bind����ַ��Ϣ���׽��ֹ���
	ret = bind(recvSocket, (SOCKADDR*)&receiverAddr, sizeof(receiverAddr));
	if (0 != ret) {
		printf("bind error with ErrorNum %d\n", WSAGetLastError());
		exitP();
	}

	thread t(rtpvp8);
	// �������ݱ�
	while (!syncFlag.load()) {
		this_thread::sleep_for(chrono::milliseconds(100));
	}
	
	while (syncFlag.load()) {
		ret = recvfrom(recvSocket, recvBuf, recvBufLength, 0, (SOCKADDR*)&senderAddr, &senderAddrLength);
		if (ret > 0) {
			gstreamer_receive_push_buffer(recvBuf, ret);
		}
	}
	exitP();
	return 0;
}

void exitP() {
	closesocket(recvSocket);
	system("pause");
	exit(0);
}