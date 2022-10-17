#pragma once
#include <string>
#include <gst/gst.h>

void buildName(std::string parentName, std::string elementName, gchar* target, int length);
std::string WString2String(const std::wstring& wstr);