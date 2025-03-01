#ifndef __NGX_C_CONF_H__
#define __NGX_C_CONF_H__
#include <mutex>
#include <vector>
#include "ngx_global.h"
class CConfig {
public:
	static CConfig* GetInstance() {
		if (instance == nullptr) {
			std::lock_guard<std::mutex> lock(mtx);
			if (instance == nullptr) {
				instance = new CConfig();
			}
		}
		return instance;
	}
private:
	class CRelease {
	public:
		~CRelease() {
			if (CConfig::instance != nullptr) {
				delete CConfig::instance;
				CConfig::instance = nullptr;
			}
		}
	};
	CConfig() {}
	~CConfig();
	CConfig(const CConfig&) = delete;
	CConfig& operator=(const CConfig&) = delete;
	static CConfig* instance;
	static std::mutex mtx;
	static CRelease release;
public:
	bool Load(const char* pconfigName);
	int GetIntDefault(const char* pItemName,const int def);
	const char* GetString(const char* pItemName);
private:
	std::vector<PLCConfItem> confItemList;
};



#endif

