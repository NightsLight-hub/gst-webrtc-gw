#pragma once
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <string>
using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;
using namespace std;

class NegotiateListener {
public:
	NegotiateListener(wstring uristring);
	void start();
public:
	http_listener listener;
};


