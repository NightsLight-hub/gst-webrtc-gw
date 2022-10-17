#pragma once
#include "rest.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "videoCompositor.h"
#include "audioCompositor.h"
#include "rtpReceiver.h"
#include <thread>
#include <atomic>
#include "helper.h"
#include "mediaProcessTask.h"

const wstring conferenceNameStr = L"conferenceName";
const wstring sipAdaptorIPStr = L"sipAdaptorIP";
const wstring sipAdaptorVideoPortStr = L"sipAdaptorVideoPort";
const wstring sipAdaptorAudioPortStr = L"sipAdaptorAudioPort";
const wstring MediaProcessorVideoPortStr = L"MediaProcessorVideoPort";
const wstring MediaProcessorAudioPortStr = L"MediaProcessorAudioPort";

static void handleDel(http_request request) {
	wcout << L"\nhandle DEL" << endl;
	auto answer = json::value::object();

	auto queryParameterMap = uri::split_query(request.relative_uri().query());
	if (queryParameterMap.find(conferenceNameStr) == queryParameterMap.end()) {
		answer[U("result")] = json::value(U("error"));
		answer[U("message")] = json::value(U("no conferenceNameStr in queryParameters"));
		request.reply(status_codes::OK, answer);
		return;
	}
	wstring conferenceName = queryParameterMap.find(conferenceNameStr)->second;
	MediaProcessManager::getInstance()->removeTask(WString2String(conferenceName));
}

static void handleGet(http_request request) {
	wcout << L"\nhandle GET" << endl;

	auto answer = json::value::object();

	auto queryParameterMap = uri::split_query(request.relative_uri().query());
	if (queryParameterMap.find(sipAdaptorVideoPortStr) == queryParameterMap.end()) {
		answer[U("result")] = json::value(U("error"));
		answer[U("message")] = json::value(U("no sipAdaptorVideoPort in queryParameters"));
		request.reply(status_codes::OK, answer);
		return;
	}
	if (queryParameterMap.find(sipAdaptorAudioPortStr) == queryParameterMap.end()) {
		answer[U("result")] = json::value(U("error"));
		answer[U("message")] = json::value(U("no sipAdaptorAudioPort in queryParameters"));
		request.reply(status_codes::OK, answer);
		return;
	}
	if (queryParameterMap.find(conferenceNameStr) == queryParameterMap.end()) {
		answer[U("result")] = json::value(U("error"));
		answer[U("message")] = json::value(U("no conferenceName in queryParameters"));
		request.reply(status_codes::OK, answer);
		return;
	}
	if (queryParameterMap.find(sipAdaptorIPStr) == queryParameterMap.end()) {
		answer[U("result")] = json::value(U("error"));
		answer[U("message")] = json::value(U("no sipAdaptorIPStr in queryParameters"));
		request.reply(status_codes::OK, answer);
		return;
	}

	int sipAdaptorVideoPort = stoi(queryParameterMap.find(sipAdaptorVideoPortStr)->second);
	int sipAdaptorAudioPort = stoi(queryParameterMap.find(sipAdaptorAudioPortStr)->second);
	wstring conferenceName = queryParameterMap.find(conferenceNameStr)->second;
	wstring sipAdaptorIP = queryParameterMap.find(sipAdaptorIPStr)->second;
	MediaProcessTask* task = MediaProcessManager::getInstance()->addNewTask(WString2String(conferenceName), WString2String(sipAdaptorIP), sipAdaptorAudioPort, sipAdaptorVideoPort);
	if (task->init() != 0) {
		MediaProcessManager::getInstance()->removeTask(task->name);
		answer[U("result")] = json::value(U("error"));
		answer[U("message")] = json::value(U("Cannot find available port to receive A/V !!!"));
		request.reply(status_codes::OK, answer);
		return;
	}
	task->start();
	answer[U("result")] = json::value(U("success"));
	answer[U("MediaProcessorVideoPort")] = json::value(task->rtpVideoReceiver->srcPort);
	answer[U("MediaProcessorAudioPort")] = json::value(task->rtpAudioReceiver->srcPort);
	request.reply(status_codes::OK, answer);
}

NegotiateListener::NegotiateListener(wstring uristring) {
	uri_builder uri(uristring);
	listener = http_listener(uri.to_uri());
}

void NegotiateListener::start() {
	listener.support(methods::GET, handleGet);
	listener.support(methods::DEL, handleDel);
	try {
		listener.open().wait();;
	}
	catch (http_exception e) {
		cout << "httpListener open failed errcode is " << e.error_code() << ", msg is " << e.what() << endl;
		return;
	}
	// todo 先写个死循环挂住rest进程，以后改进为可关闭
	while (true) {
		this_thread::sleep_for(chrono::milliseconds(2000));
	}
}

