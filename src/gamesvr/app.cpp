/**
 * @file app.cpp
 * @brief App实现
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "app.h"
#include "config_module.h"
#include "timer_mgr_module.h"
#include "msg_module.h"
#include "obj_mgr_module.h"
#include "logic_module.h"
#include "coroutine_module.h"

ServerFrame g_server("0.0.1");
ServerFrame* g_svr_frame = &g_server;

int32_t main(int argc, char * argv[])
{
    return g_server.Run(argc, argv);
}

//////////////////////////////////////////////////////////////////////////
App* App::instance_ = NULL;

const char * App::AppName() const
{
    // 名字必须跟exe名字一样
    static const std::string name = "gamesvr";
    return name.c_str();
}

AppBase* AppBase::GetInstance()
{
    if (App::instance_ == NULL) {
        App::instance_ = new App();
    }
    return App::instance_;
}

int32_t App::AppInit()
{
    // 初始化module
    AppModuleBase* app_module = NULL;

    app_module = ConfigModule::CreateModule(this, g_svr_frame->conf_name_);
    AddModule(ConfigModule::ModuleId(), app_module);

    app_module = TimerMgrModule::CreateModule(this);
    AddModule(TimerMgrModule::ModuleId(), app_module);

    app_module = MsgModule::CreateModule(this);
    AddModule(MsgModule::ModuleId(), app_module);

    app_module = ObjMgrModule::CreateModule(this);
    AddModule(ObjMgrModule::ModuleId(), app_module);

    app_module = LogicModule::CreateModule(this);
    AddModule(LogicModule::ModuleId(), app_module);

    app_module = CoroutineModule::CreateModule(this);
    AddModule(CoroutineModule::ModuleId(), app_module);

    LOG(INFO) << AppName() << " init succeed!";
    return 0;
}

uintptr_t TestCo(void* str)
{
    LOG(INFO) << "test_context recv: [" << (const char*)str << "]";
    uintptr_t ret;
    ret = CoroutineModule::Yield((uintptr_t)"i want to yield! from test_context");
    LOG(INFO) << "test_context recv:" << (const char*)ret << "]";
    return (uintptr_t)"exit! from test_context";
}

int32_t App::AppRun()
{
    static MsgModule* msg_module = GetModule<MsgModule>();
    static LogicModule* logic_module = GetModule<LogicModule>();
    static CoroutineModule* coroutine_module = GetModule<CoroutineModule>();

    msg_module->Run();
    logic_module->Update();
    coroutine_module->Update();

    static int32_t test_flag = 1;
    if (test_flag == 1) {
        test_flag = 2;
        // 创建协程
        int64_t coroutine_id =
            coroutine_module->CreateCoroutine(TestCo,
                    (void*)"hi Coroutine! from main_context", 5);
        if (coroutine_id > 0) {
            uintptr_t ret;
            // 恢复协程
            ret = coroutine_module->Resume(coroutine_id);
            LOG(INFO) << "main_context recv:"  << (const char*)ret << "]";
            ret = coroutine_module->Resume(coroutine_id, (uintptr_t)"ok! from main_context");
            LOG(INFO) << "main_context recv:"  << (const char*)ret << "]";
        }
    }

    return 0;
}

void App::AppClean()
{

}

void App::AppReload()
{
}

