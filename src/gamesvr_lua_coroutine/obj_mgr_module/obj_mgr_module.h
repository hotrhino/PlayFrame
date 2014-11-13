/**
 * @file obj_mgr_module.h
 * @brief 对象管理模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _OBJ_MGR_MODULE_H_
#define _OBJ_MGR_MODULE_H_

#include "app_def.h"
#include "player.h"
#include "city.h"

class ObjMgrModule : public AppModuleBase
{
    public:
        DISALLOW_COPY_AND_ASSIGN(ObjMgrModule);

        ObjMgrModule(App* app);
        virtual ~ObjMgrModule();

        virtual void            ModuleInit();
        virtual void            ModuleFini();
        virtual const char*     ModuleName() const;
        static int32_t          ModuleId();
        static AppModuleBase*   CreateModule(App* app);

    public:
        static void RestoreUidToPlayerIdxMap(Player* player, void* args);
        void RestoreUidToPlayerIdxMap(Player* player);

        int32_t FindPlayerIdxByUid(uint64_t uid);
        void BindUidToPlayerIdx(uint64_t uid, int32_t player_idx);
        void EraseUidToPlayerIdx(uint64_t uid, int32_t player_idx);

        int32_t AddPlayer();
        Player* GetPlayer(int32_t player_idx);
        int32_t GetPlayerIdx(Player* player);    
        void DelPlayer(int32_t player_idx);

        int32_t AddCity(int32_t player_idx);
        City* GetCity(int32_t city_idx);
        int32_t GetCityIdx(City* city);    
        void DelCity(int32_t city_idx);

    private:
        typedef std::map<uint64_t, int32_t> UidToPlayerIdxMap_T;
        UidToPlayerIdxMap_T uid_to_player_idx_map_;
        ShmPool<Player>     player_pool_;
        ShmPool<City>       city_pool_;
};

#endif

