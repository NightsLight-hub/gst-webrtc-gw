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
/// 添加一个收包、解包、解码、编码、发包的全流程任务, 任务需要额外调用init来初始化
/// </summary>
/// <param name="name">任务名称，通常是会议室名称</param>
/// <param name="sinkIP">要把RTP发送的目标地址</param>
/// <param name="audioSinkPort">要把RTP发送的目标地址，音频端口</param>
/// <param name="videoSinkPort">要把RTP发送的目标地址，视频端口</param>
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
/// 删除任务
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
				// 删除失败要不要继续？先删除了吧，等有需要强制回收资源时再优化
				cerr << "stop task failed, " << e.what() << endl;
			}
			tasks.erase(iter);
			break;
		}
	}
}