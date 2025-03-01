#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "ngx_c_conf.h"
#include "ngx_global.h"
#include "ngx_func.h"
// #ifdef _MSC_VER
// #define strcasecmp _stricmp
// #define strncasecmp _strnicmp
// #endif


std::mutex CConfig::mtx;
CConfig* CConfig::instance = nullptr;
CConfig::CRelease CConfig::release;

//释放资源
CConfig::~CConfig() {
	std::vector<PLCConfItem>::iterator pos;
	for (pos = confItemList.begin(); pos != confItemList.end(); ++pos) {
		delete (*pos);
		(*pos) = nullptr;
	}
	confItemList.clear();
}
//加载配置文件
bool CConfig::Load(const char* pconfigName) {
	FILE* fp = fopen(pconfigName, "r");
	if (fp == nullptr){
		printf("open config file fail!!!\n");
		return false;
	} 
	char linebuf[501];
	memset(linebuf, 0, sizeof(linebuf));
	while (!feof(fp)) {
		if (fgets(linebuf, sizeof(linebuf), fp) == nullptr)continue;
		if (*linebuf == ';' || *linebuf == '#' || *linebuf == '\t' || *linebuf == '\n')continue;
		if (*linebuf == '\0')continue;
		while (strlen(linebuf)>0&&
			(linebuf[strlen(linebuf) - 1] == '\t' || linebuf[strlen(linebuf) - 1] == '\n' || linebuf[strlen(linebuf) - 1] == ' ')) {
			linebuf[strlen(linebuf) - 1] = '\0';
		}
		if (*linebuf=='\0') continue;
		if (*linebuf == '[')continue;
		char* ptmp = strchr(linebuf, '=');
		if (ptmp == nullptr)continue;
		PLCConfItem p_confitem = new CConfItem();
		memset(p_confitem, 0, sizeof(CConfItem));
		strncpy(p_confitem->ItemName,linebuf,ptmp-linebuf);
		strcpy(p_confitem->ItemContent, ptmp + 1);
		Rtrim(p_confitem->ItemName);
		Ltrim(p_confitem->ItemName);
		Rtrim(p_confitem->ItemContent);
		Ltrim(p_confitem->ItemContent);
		confItemList.push_back(p_confitem);
	}
	fclose(fp);
	return true;
}

//获取数字的配置项
int CConfig::GetIntDefault(const char* pItemName ,const int def) {
	std::vector<PLCConfItem>::iterator pos;
	for (pos = confItemList.begin(); pos != confItemList.end(); ++pos) {
		if (strcasecmp((*pos)->ItemName, pItemName) == 0) {
			return atoi((*pos)->ItemContent);
		}
	}
	return def;
}

//获取字符串的配置项
const char* CConfig::GetString(const char* pItemName) {
	std::vector<PLCConfItem>::iterator pos;
	for (pos = confItemList.begin(); pos != confItemList.end(); ++pos) {
		if (strcasecmp((*pos)->ItemName, pItemName) == 0) {
			return (*pos)->ItemContent;
		}
	}
	return nullptr;
}