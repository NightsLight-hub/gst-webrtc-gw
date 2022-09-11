#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#pragma comment(lib, "Ws2_32.lib")

constexpr static auto TCP_DEFAULT_PORT = "20000";
constexpr static auto DEFAULT_BUFLEN = 1024;
int tcp_server(int argc, char** argv)
{
	//初始化Winsock
	WSADATA wsaData;
	int iResult = 0;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
	//-----------------------
	//储存地址信息的结构
	addrinfo* result = NULL, * ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE; //声明该socket需要被bind
	//如果第一个参数为NULL，则返回本机上的地址
	iResult = getaddrinfo(NULL, TCP_DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	//-----------------------
	//建立ListenSocket
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	//将ListenSocket绑定到本机—即服务器上的地址
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
	//指示该socket为服务器的监听套接字
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	//-----------------------
	//接受客户端连接
	SOCKET ClientSocket;
	//accept会一直等待直到客户连接
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	//通常，服务器在接收到客户端连接之后，会把这个连接放到一个单独的线程中，但此处略去
	//-----------------------
	//收发消息  缓冲区长度要比定义的可接收数据长度+1，用于存储\0
	char recvbuf[DEFAULT_BUFLEN+1];
	int iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;

	//消息接收循环
	do {
		//由于收到的消息不带有字符串终止符，因此每次都要将缓冲置全0，不然无法判断字符串终止
		memset(recvbuf, 0, sizeof(recvbuf));
		//recv将阻塞等待客户端消息，客户端将发送通道关闭后，recv返回0
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
	//释放
	//服务器关闭
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