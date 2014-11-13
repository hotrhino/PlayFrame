/**
 * @file obj_mgr_module.cpp
 * @brief 对象管理模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "obj_mgr_module.h"
#include "app.h"
#include "config_module.h"

ObjMgrModule::ObjMgrModule(App* app) :
    AppModuleBase(app)
{
}

ObjMgrModule::~ObjMgrModule()
{
}

void ObjMgrModule::ModuleInit()
{
    ConfigModule* conf_module = FindModule<ConfigModule>(app_);
    int32_t player_pool_shm_key = conf_module->config().player_pool_shm_key();
    int32_t player_pool_size = conf_module->config().player_pool_size();

    CHECK(player_pool_.Init(player_pool_size, player_pool_shm_key) == 0)
        << "player_pool init error!";

    int32_t city_pool_shm_key = conf_module->config().city_pool_shm_key();
    int32_t city_pool_size = conf_module->config().city_pool_size();

    CHECK(city_pool_.Init(city_pool_size, city_pool_shm_key) == 0)
        << "city_pool init error!";

    // 恢复uid 和 player_idx关系
    player_pool_.TravelPool(&ObjMgrModule::RestoreUidToPlayerIdxMap, this, 0);

    LOG(INFO) << ModuleName() << " init ok!";
}

int32_t ObjMgrModule::FindPlayerIdxByUid(uint64_t uid)
{
    LOG(INFO) << "FindPlayerIdxByUid[" << uid << "]";
    UidToPlayerIdxMap_T::iterator it = uid_to_player_idx_map_.find(uid);
    if (it != uid_to_player_idx_map_.end()) {
        int32_t player_idx = it->second;
        LOG(INFO)
            << "uid_to_player_idx_map_ find uid[" << uid
            << "] player_idx[" << player_idx
            << "].";
        return player_idx;
    }
    return 0;
}

void ObjMgrModule::BindUidToPlayerIdx(uint64_t uid, int32_t player_idx)
{
    uid_to_player_idx_map_.insert(UidToPlayerIdxMap_T::value_type(uid, player_idx));
    LOG(INFO)
        << "uid_to_player_idx_map_ insert uid[" << uid
        << "] player_idx[" << player_idx
        << "].";
}

void ObjMgrModule::EraseUidToPlayerIdx(uint64_t uid, int32_t player_idx)
{
    UidToPlayerIdxMap_T::iterator it = uid_to_player_idx_map_.find(uid);
    if (it != uid_to_player_idx_map_.end()) {
        if (it->second == player_idx) {
            LOG(INFO)
                << "uid_to_player_idx_map_ erase uid[" << it->first
                << "] player_idx[" << it->second
                << "].";
            uid_to_player_idx_map_.erase(it);
        } else {
            LOG(ERROR)
                << "uid [" << uid
                << "] player_idx[" << player_idx
                << "], in uid_to_player_idx_map_ uid[" << it->first
                << "] player_idx[" << it->second
                << "].";
        }
    }
}

void ObjMgrModule::RestoreUidToPlayerIdxMap(Player* player, void* args)
{
    ObjMgrModule* obj_mgr_module = (ObjMgrModule*)(args);
    obj_mgr_module->RestoreUidToPlayerIdxMap(player);
}

void ObjMgrModule::RestoreUidToPlayerIdxMap(Player* player)
{
    BindUidToPlayerIdx(player->uid(), player->GetPlayerIdx());
}

void ObjMgrModule::ModuleFini()
{
    LOG(INFO) << ModuleName() << " fini completed!";
}

const char* ObjMgrModule::ModuleName() const
{
    static const std::string ModuleName = "ObjMgrModule";
    return ModuleName.c_str();
}

int32_t ObjMgrModule::ModuleId()
{
    return MODULE_ID_PLAYER_MGR;
}

AppModuleBase* ObjMgrModule::CreateModule(App* app)
{
    ObjMgrModule* obj_mgr_module = new ObjMgrModule(app);
    if (obj_mgr_module != NULL) {
        obj_mgr_module->ModuleInit();
    }

    return static_cast<AppModuleBase*>(obj_mgr_module);
}

int32_t ObjMgrModule::AddPlayer()
{
    Player* player = (Player*)player_pool_.Alloc();
    player->init();
    if (player != NULL) {
        int32_t player_idx = player_pool_.GetPos((void*)player);
        LOG(INFO)
            << "AddPlayer mem_addr[" << (uint64_t)player
            << "] player_idx[" << player_idx
            << "]";
        return player_idx;
    }

    return -1;
}

Player* ObjMgrModule::GetPlayer(int32_t player_idx)
{
    if (player_idx <= 0) {
        LOG(ERROR) 
            << "player_idx[" << player_idx
            << "] must be > 0";
        return NULL;
    }

    LOG(INFO) << "GetPlayer[" << player_idx << "]";
    Player* player = (Player*)player_pool_.Find(player_idx);
    if (player == NULL) {
        LOG(ERROR)
            << "player_idx[" << player_idx
            << "] can't find.";
    }
    return player;
}

int32_t ObjMgrModule::GetPlayerIdx(Player* player)
{
    int32_t player_idx = player_pool_.GetPos((void*)player);
    return player_idx;
}

void ObjMgrModule::DelPlayer(int32_t player_idx)
{
    Player* player = GetPlayer(player_idx);
    if (player != NULL) {
        uint64_t uid = player->uid();
        int32_t player_idx = GetPlayerIdx(player);
        EraseUidToPlayerIdx(uid, player_idx);
        // 注意: 每添加一种Player包含的内存对象, 则需要添加相应的释放
        // 有其他内存对象
        if (player->city_list_[0] > 0) {
            for (int32_t i = 1; i < MAX_CITY_NUM_PER_PLAYER; i++) {
                if (player->city_list_[i] != 0) {
                    DelCity(player->city_list_[i]);        
                }
            }
        }
        // 释放Player
        player_pool_.Release((void*)player);
        LOG(INFO) << "DelPlayer player_idx[" << player_idx << "]";
    }
}

int32_t ObjMgrModule::AddCity(int32_t player_idx)
{
    Player* player = GetPlayer(player_idx);
    if (player == NULL) {
        LOG(ERROR)
            << "player_idx[" << player_idx
            << "] is not existed!";
        return -1;
    }

    if (player->city_list_[0] >= MAX_CITY_NUM_PER_PLAYER) {
        LOG(ERROR)
            << "player_idx[" << player_idx
            << "] city num beyond MAX_CITY_NUM_PER_PLAYER[" << MAX_CITY_NUM_PER_PLAYER
            << "]";
        return -2;
    }

    for (int32_t i = 1; i < MAX_CITY_NUM_PER_PLAYER; i++) {
        if (player->city_list_[i] == 0) {
            City* city = (City*)city_pool_.Alloc();
            city->init();
            if (city != NULL) {
                int32_t city_idx = city_pool_.GetPos((void*)city);
                LOG(INFO)
                    << "AddCity mem_addr[" << (uint64_t)city
                    << "] city_idx[" << city_idx
                    << "]";
                (player->city_list_[0])++;
                player->city_list_[i] = city_idx;
                LOG(INFO)
                    << "player->city_list_[0] = " << player->city_list_[0]
                    << ", player->city_list_[" << i
                    << "] = " << city_idx;
                city->set_player_idx(player_idx);
                return city_idx;
            }
            return -3;
        }
    }

    return -4;
}

City* ObjMgrModule::GetCity(int32_t city_idx)
{
    if (city_idx <= 0) {
        LOG(ERROR) 
            << "city_idx[" << city_idx
            << "] must be > 0";
        return NULL;
    }

    LOG(INFO) << "GetCity[" << city_idx << "]";
    City* city = (City*)city_pool_.Find(city_idx);
    if (city == NULL) {
        LOG(ERROR)
            << "city_idx[" << city_idx
            << "] can't find.";
    }
    return city;
}

int32_t ObjMgrModule::GetCityIdx(City* city)
{
    int32_t city_idx = city_pool_.GetPos((void*)city);
    return city_idx;
}

void ObjMgrModule::DelCity(int32_t city_idx)
{
    City* city = GetCity(city_idx);
    if (city != NULL) {
        city_pool_.Release((void*)city);
        LOG(INFO) << "DelCity city_idx[" << city_idx << "]";
    }
}

