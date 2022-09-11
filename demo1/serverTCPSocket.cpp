#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#pragma comment(lib, "Ws2_32.lib")

constexpr static auto TCP_DEFAULT_PORT = "20000";
constexpr static auto DEFAULT_BUFLEN = 1024;
int tcp_server(int argc, char** argv)
{
	//��ʼ��Winsock
	WSADATA wsaData;
	int iResult = 0;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
	//-----------------------
	//�����ַ��Ϣ�Ľṹ
	addrinfo* result = NULL, * ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE; //������socket��Ҫ��bind
	//�����һ������ΪNULL���򷵻ر����ϵĵ�ַ
	iResult = getaddrinfo(NULL, TCP_DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	//-----------------------
	//����ListenSocket
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	//��ListenSocket�󶨵����������������ϵĵ�ַ
	//sockaddr_in service;
	//service.sin_family = AF_INET;
	//service.sin_addr.s_addr = ADDR_ANY;
	//service.sin_port = htons(27015);
	//iResult = bind(ListenSocket, (SOCKADDR*)& service, sizeof(service));
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);
	//ָʾ��socketΪ�������ļ����׽���
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	//-----------------------
	//���ܿͻ�������
	SOCKET ClientSocket;
	//accept��һֱ�ȴ�ֱ���ͻ�����
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	//ͨ�����������ڽ��յ��ͻ�������֮�󣬻��������ӷŵ�һ���������߳��У����˴���ȥ
	//-----------------------
	//�շ���Ϣ  ����������Ҫ�ȶ���Ŀɽ������ݳ���+1�����ڴ洢\0
	char recvbuf[DEFAULT_BUFLEN+1];
	int iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;

	//��Ϣ����ѭ��
	do {
		//�����յ�����Ϣ�������ַ�����ֹ�������ÿ�ζ�Ҫ��������ȫ0����Ȼ�޷��ж��ַ�����ֹ
		memset(recvbuf, 0, sizeof(recvbuf));
		//recv�������ȴ��ͻ�����Ϣ���ͻ��˽�����ͨ���رպ�recv����0
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("%s\n", recvbuf);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	} while (iResult > 0);
	//-----------------------
	//�ͷ�
	//�������ر�
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}