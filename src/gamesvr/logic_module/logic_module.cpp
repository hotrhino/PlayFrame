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
    logic_map_.insert(LogicMap::value_type(LOGIC_NORMAL_REG, logic));

    logic = new LogicQuickReg();
    logic_map_.insert(LogicMap::value_type(LOGIC_QUICK_REG, logic));

    logic = new LogicLogin();
    logic_map_.insert(LogicMap::value_type(LOGIC_LOGIN, logic));

    logic = new LogicUpdatePlayer();
    logic_map_.insert(LogicMap::value_type(LOGIC_UPDATE_PLAYER, logic));

    for (LogicMap::iterator it = logic_map_.begin(); it != logic_map_.end(); ++it) {
        LOG(INFO) <<  (it->second)->LogicName();
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
    logic_map_.clear();

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
    switch (logic_type) {
        case LOGIC_NORMAL_REG:
            logic_data = malloc(sizeof(LogicDataNormalReg));
            break;
        case LOGIC_QUICK_REG:
            logic_data = malloc(sizeof(LogicDataQuickReg));
            break;
        case LOGIC_LOGIN:
            logic_data = malloc(sizeof(LogicDataLogin));
            break;
        case LOGIC_UPDATE_PLAYER:
            logic_data = malloc(sizeof(LogicDataUpdatePlayer));
            break;
    }

    if (logic_data == NULL)
        return 0;

    int64_t logic_id = heap_timer_->RegisterTimer(
            TimeValue(task_delay_secs),
            TimeValue(task_delay_secs),
            this,
            NULL);
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
    LogicMap::iterator it = logic_map_.find(logic_data_head->logic_type);
    if (it == logic_map_.end()) {
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
        DeleteLogic(logic_id);
        heap_timer_->UnregisterTimer(logic_id);
    }
}
