/**
 * @file config_module.cpp
 * @brief 配置模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "config_module.h"
#include "app.h"

ConfigModule::ConfigModule(App* app, const char* conf_file) :
    AppModuleBase(app),
    conf_file_(conf_file)
{
    app_ = app;
}

ConfigModule::~ConfigModule()
{}

void ConfigModule::ModuleInit()
{
    int conf_fd = open(conf_file_.c_str(), O_RDONLY);
    PCHECK(conf_fd > 0)
        << "open config file failed!";

    google::protobuf::io::FileInputStream file_input(conf_fd);
    file_input.SetCloseOnDelete(true);
    bool is_parse_succ = google::protobuf::TextFormat::Parse(&file_input, &config_);
    PCHECK(is_parse_succ == true)
        << "config: protobuf textformat parse failed!";

    LOG(INFO) << config_.Utf8DebugString();
    LOG(INFO) << ModuleName() << " init ok!";
}

void ConfigModule::ModuleFini()
{
    LOG(INFO) << ModuleName() << " fini completed!";
}

const char* ConfigModule::ModuleName() const
{
    static const std::string ModuleName = "ConfigModule";
    return ModuleName.c_str();
}

int32_t ConfigModule::ModuleId()
{
    return MODULE_ID_CONFIG;
}

AppModuleBase* ConfigModule::CreateModule(App* app, const char* conf_file)
{
    ConfigModule* module = new ConfigModule(app, conf_file);
    if (module != NULL) {
        module->ModuleInit();
    }

    return static_cast<AppModuleBase*>(module);
}

