/**
 * @file lua_engine_module.cpp
 * @brief Lua引擎模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "lua_engine_module.h"
#include "obj_mgr_module.h"
#include "player.h"
#include "app.h"

LuaEngineModule::LuaEngineModule(App* app, const char* main_script_file) :
    AppModuleBase(app)
{
    Init(main_script_file);
}

LuaEngineModule::~LuaEngineModule()
{
    while (GetTaskSize() != 0) {
        LOG(INFO) << "wait for [" << GetTaskSize() << "] to finish!";
        Run();
        sleep(1);  
    }
}

void LuaEngineModule::ModuleInit()
{
    // 初始化要调用的类
    // ObjMgrModule
    RegClass<ObjMgrModule>("ObjMgrModule");
    REG_FUNC(ObjMgrModule, FindPlayerIdxByUid);
    REG_FUNC(ObjMgrModule, AddPlayer);
    REG_FUNC(ObjMgrModule, GetPlayer);
    REG_FUNC(ObjMgrModule, DelPlayer);

    //Utils
    RegFunc("gen_uid_num", &Utils::GenUidNum);
    RegFunc("fnv_64a_hash", &Utils::Fnv64aHash);
    RegFunc("check_account", &Utils::CheckAccount);
    RegFunc("check_password", &Utils::CheckPassword);

    // Player
    // Player 自身属性
    RegClass<Player>("Player");
    REG_FUNC(Player, SetPlayerByDb);
    REG_FUNC(Player, GetPlayerIdx);
    REG_FUNC(Player, set_uid);
    REG_FUNC(Player, uid);
    REG_FUNC(Player, set_password_hash);
    REG_FUNC(Player, password_hash);
    // Player 客户端交互
    REG_FUNC(Player, SendFailedCsRes);
    REG_FUNC(Player, SendOkCsQuickRegRes);
    REG_FUNC(Player, SendOkCsNormalRegRes);
    REG_FUNC(Player, SendOkCsLoginRes);
    REG_FUNC(Player, SendRoleInfoNotify);
    REG_FUNC(Player, SendServerKickOffNotify);
    // Player datasvr交互
    REG_FUNC(Player, DoAccountReg);
    REG_FUNC(Player, DoAccountVerify);
    REG_FUNC(Player, DoGetPlayerData);
    REG_FUNC(Player, DoUpdatePlayerData);

    // 设置要调用的module
    ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(app_);
    SetGlobal<ObjMgrModule*>("OBJ_MGR_MODULE", obj_mgr_module);

    LOG(INFO) << ModuleName() << " init ok!";
}

void LuaEngineModule::ModuleFini()
{
    LOG(INFO) << ModuleName() << " fini completed!";
}

const char* LuaEngineModule::ModuleName() const
{
    static const std::string ModuleName = "LuaEngineModule";
    return ModuleName.c_str();
}

int32_t LuaEngineModule::ModuleId()
{
    return MODULE_ID_LUA_ENGINE;
}

AppModuleBase* LuaEngineModule::CreateModule(App* app,
        const char* main_script_file)
{
    LuaEngineModule* module = new LuaEngineModule(app, main_script_file);
    if (module != NULL) {
        module->ModuleInit();
    }

    return static_cast<AppModuleBase*>(module);
}

