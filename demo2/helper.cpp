#include "helper.h"
#include <string>
#include <cstring>
#include <comdef.h>
#include <windows.h>
#include <string.h>

using namespace std;
void buildName(string parentName, string elementName, gchar* target, int length) {
	string ret = parentName + "_" + elementName;
	if (ret.length() > length - 1) {
		g_printerr("target length is less than result");
		exit(1);
	}
	strcpy_s(target, length, ret.c_str());
}


std::string WString2String(const std::wstring& wstr) {
    std::string res;
    int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return res;
    }
    char* buffer = new char[len + 1];
    if (buffer == nullptr) {
        return res;
    }
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, nullptr, nullptr);
    buffer[len] = '\0';
    res.append(buffer);
    delete[] buffer;
    return res;
}