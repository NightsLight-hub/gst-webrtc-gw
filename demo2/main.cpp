#include "rest.h"

int main()
{
	NegotiateListener listener(L"http://*:49999/api/v1/process1");
	listener.start();
	return 0;
}

