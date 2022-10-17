#include "mediaProcessTask.h"
#include <iostream>

MediaProcessManager* MediaProcessManager::instance = new(std::nothrow) MediaProcessManager();

MediaProcessManager::MediaProcessManager() {

}

MediaProcessManager::~MediaProcessManager() {

}

MediaProcessManager* MediaProcessManager::getInstance() {
	return instance;
}

/// <summary>
/// ���һ���հ�����������롢���롢������ȫ��������, ������Ҫ�������init����ʼ��
/// </summary>
/// <param name="name">�������ƣ�ͨ���ǻ���������</param>
/// <param name="sinkIP">Ҫ��RTP���͵�Ŀ���ַ</param>
/// <param name="audioSinkPort">Ҫ��RTP���͵�Ŀ���ַ����Ƶ�˿�</param>
/// <param name="videoSinkPort">Ҫ��RTP���͵�Ŀ���ַ����Ƶ�˿�</param>
/// <returns></returns>
MediaProcessTask* MediaProcessManager::addNewTask(
	string name, string sinkIP, int audioSinkPort, int videoSinkPort) {
	MediaProcessTask* task = new MediaProcessTask(name);
	task->audioSinkPort = audioSinkPort;
	task->videoSinkPort = videoSinkPort;
	this->tasks.push_back(task);
	return task;
}

/// <summary>
/// ɾ������
/// </summary>
/// <param name="name"></param>
void MediaProcessManager::removeTask(string name) {
	vector<MediaProcessTask*>::iterator iter = tasks.begin();
	while (iter != tasks.end())
	{
		if ((*iter)->name == name) {
			try {
				(*iter)->stop();
			}
			catch (exception e) {
				// ɾ��ʧ��Ҫ��Ҫ��������ɾ���˰ɣ�������Ҫǿ�ƻ�����Դʱ���Ż�
				cerr << "stop task failed, " << e.what() << endl;
			}
			tasks.erase(iter);
			break;
		}
	}
}