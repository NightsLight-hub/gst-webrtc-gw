#include "helper.h"
#include <string>
#include <cstring>

using namespace std;
void buildName(string parentName, string elementName, gchar* target, int length) {
	string ret = parentName + "_" + elementName;
	if (ret.length() > length - 1) {
		g_printerr("target length is less than result");
		exit(1);
	}
	strcpy(target, ret.c_str());
}