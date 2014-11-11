/**
 * @file config_module.h
 * @brief 配置模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _CONFIG_MODULE_H_
#define _CONFIG_MODULE_H_

#include "app_def.h"
#include "gamesvr_config.pb.h"

class ConfigModule : public AppModuleBase
{
public:
	ConfigModule(App* app, const char* conf_file);
	virtual ~ConfigModule();

	virtual void            ModuleInit();
	virtual void            ModuleFini();
	virtual const char*     ModuleName() const;
	static int32_t          ModuleId();
	static AppModuleBase*   CreateModule(App* app, const char* conf_file);

public:
    inline const Config::GameSvr& config() const
    {
        return config_; 
    }

private:
    Config::GameSvr config_;
    std::string     conf_file_;
};

#endif

