/**
 * @file logic_module.cpp
 * @brief  异步逻辑框架
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-11-13
 */
#include "app.h"
#include "logic_module.h"
#include "config_module.h"
#include "logic_normal_reg.h"
#include "logic_quick_reg.h"
#include "logic_login.h"
#include "logic_update_player.h"

LogicModule::LogicModule(App* app) :
    AppModuleBase(app),
    heap_timer_(NULL)
{}

LogicModule::~LogicModule()
{}

void LogicModule::ModuleInit()
{
    ConfigModule* conf_module = FindModule<ConfigModule>(app_);
    heap_timer_ = new HeapTimer(conf_module->config().logic_timer_init_size());

    CHECK(heap_timer_ != NULL)
        << "timer heap init error!";
    SetHandler(this, &LogicModule::OnTimer);

    LogicBase* logic = NULL;

    logic = new LogicNormalReg();
    logic_type_map_.insert(LogicTypeMap::value_type(LOGIC_NORMAL_REG, logic));
    logic_data_size_map_.insert(LogicDataSizeMap::value_type(
                LOGIC_NORMAL_REG, sizeof(LogicDataNormalReg)));

    logic = new LogicQuickReg();
    logic_type_map_.insert(LogicTypeMap::value_type(LOGIC_QUICK_REG, logic));
    logic_data_size_map_.insert(LogicDataSizeMap::value_type(
                LOGIC_QUICK_REG, sizeof(LogicDataQuickReg)));

    logic = new LogicLogin();
    logic_type_map_.insert(LogicTypeMap::value_type(LOGIC_LOGIN, logic));
    logic_data_size_map_.insert(LogicDataSizeMap::value_type(
                LOGIC_LOGIN, sizeof(LogicDataLogin)));

    logic = new LogicUpdatePlayer();
    logic_type_map_.insert(LogicTypeMap::value_type(LOGIC_UPDATE_PLAYER, logic));
    logic_data_size_map_.insert(LogicDataSizeMap::value_type(
                LOGIC_UPDATE_PLAYER, sizeof(LogicDataUpdatePlayer)));

    LOG(INFO) << "All Logic:";
    for (LogicTypeMap::const_iterator it = logic_type_map_.begin(); it != logic_type_map_.end(); ++it) {
        LOG(INFO)
            << "logic_type[" << it->first
            << "] <---> [" << (it->second)->LogicName()
            << "]";
    }

    LOG(INFO) << "All LogicDataSize:";
    for (LogicDataSizeMap::const_iterator it = logic_data_size_map_.begin();
            it != logic_data_size_map_.end(); ++it) {
        LOG(INFO)
            << "logic_type[" << it->first
            << "] <---> logic_data_size[" << it->second
            << "]";
    }

    LOG(INFO) << ModuleName() << " init ok!";
}

void LogicModule::ModuleFini()
{
    if (heap_timer_ != NULL) {
        delete heap_timer_;
        heap_timer_ = NULL;
    }

    logic_data_map_.clear();
    logic_type_map_.clear();
    logic_data_size_map_.clear();

    LOG(INFO) << ModuleName() << " fini completed!";
}

const char* LogicModule::ModuleName() const
{
    static const std::string ModuleName = "LogicModule";
    return ModuleName.c_str();
}

int32_t LogicModule::ModuleId()
{
    return MODULE_ID_LOGIC;
}

AppModuleBase* LogicModule::CreateModule(App* app)
{
    LogicModule* module = new LogicModule(app);
    if (module != NULL) {
        module->ModuleInit();
    }

    return static_cast<AppModuleBase*>(module);
}


int64_t LogicModule::CreateLogic(int32_t logic_type, time_t task_delay_secs)
{
    void* logic_data = NULL;
    LogicDataSizeMap::const_iterator it = logic_data_size_map_.find(logic_type);
    if (it == logic_data_size_map_.end()) {
        LOG(ERROR) << "can't find logic_type["
            << logic_type
            << "]"; 
        return -1;
    }

    logic_data = malloc(it->second);

    if (logic_data == NULL)
        return -1;

    // 等待时间为0为即时任务, 不需要注册定时器
    int64_t logic_id = 0;
    if (task_delay_secs > 0) {
        logic_id = heap_timer_->RegisterTimer(
                TimeValue(task_delay_secs),
                TimeValue(task_delay_secs),
                this,
                NULL);
    }
    LogicDataHead* logic_data_head = (LogicDataHead*)logic_data;
    logic_data_head->logic_id = logic_id;
    logic_data_head->logic_type = logic_type;
    logic_data_head->step = 0;

    logic_data_map_.insert(LogicDataMap::value_type(logic_id, (void*)logic_data));
    return logic_id;
}

void LogicModule::Proc(int64_t logic_id, void* msg, void* args) 
{
    LOG(INFO) << "LogicModule::Proc";
    void* logic_data = GetLogicData(logic_id);
    if (logic_data == NULL) {
        LOG(ERROR) << "logic_id is not exist!"; 
        return;
    }

    LogicDataHead* logic_data_head = (LogicDataHead*)logic_data;
    LOG(INFO)
        << "logic_data_head->logic_type["
        << logic_data_head->logic_type
        << "]";
    LogicTypeMap::const_iterator it = logic_type_map_.find(logic_data_head->logic_type);
    if (it == logic_type_map_.end()) {
        LOG(ERROR) << "can't find logic_type["
            << logic_data_head->logic_type
            << "]"; 
        return;
    }

    LOG(INFO)
        << "Proc Dispatch To ["
        << (it->second)->LogicName()
        << "]";

    int32_t ret = (it->second)->Proc(logic_data, msg, args);
    if (ret == LOGIC_FINISH)
        LOG(INFO) << (it->second)->LogicName() << " finish!"; 

    if (ret != LOGIC_YIELD) {
        DeleteLogicData(logic_id);
        if (logic_id != 0)
            heap_timer_->UnregisterTimer(logic_id);
    }
}

void LogicModule::DeleteLogicData(int64_t logic_id)
{
    LogicDataMap::iterator it = logic_data_map_.find(logic_id);
    if (it != logic_data_map_.end()) {
        LogicDataHead* head = (LogicDataHead*)(it->second);
        LOG(INFO)
            << "delete logic_id[" << head->logic_id
            << "] logic_type[" << head->logic_type
            << "] step[" << head->step
            << "]";
        free(it->second);
        it->second = NULL;
        logic_data_map_.erase(it);
    }
}

void* LogicModule::GetLogicData(int64_t logic_id)
{
    LogicDataMap::iterator it = logic_data_map_.find(logic_id);
    if (it != logic_data_map_.end())
        return it->second;

    return NULL;
}

const char* LogicModule::GetLogicName(int32_t logic_type)
{
    LogicTypeMap::const_iterator it = logic_type_map_.find(logic_type);
    if (it != logic_type_map_.end()) {
        return (it->second)->LogicName();
    }
    return NULL;
}
