/**
 * @file logic_update_player.h
 * @brief  更新玩家数据逻辑
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-11-13
 */
#ifndef _LOGIC_UPDATE_PLAYER_H_
#define _LOGIC_UPDATE_PLAYER_H_

#include "app_def.h"
#include "logic_base.h"
#include "obj_mgr_module.h"

typedef struct {
    LogicDataHead   head;
    int32_t         player_idx;
} LogicDataUpdatePlayer;

class LogicUpdatePlayer : public LogicBase
{
    public:
        LogicUpdatePlayer()
        {
            logic_func_vec_.push_back(&LogicUpdatePlayer::OnStep0);
            logic_func_vec_.push_back(&LogicUpdatePlayer::OnStep1);
        }

        virtual const char* LogicName() const
        {
            static const std::string logic_name = "LogicUpdatePlayer";
            return logic_name.c_str();
        }

        static int32_t OnStep0(void* logic_data_void, void* msg_void, void* args_void)
        {
            (void)msg_void;
            LogicDataUpdatePlayer* logic_data =
                (LogicDataUpdatePlayer*)logic_data_void;
            logic_data->player_idx = *(int32_t*)args_void;

            ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(App::GetInstance());
            Player* player = obj_mgr_module->GetPlayer(logic_data->player_idx);
            if (player == NULL) {
                LOG(ERROR) << "get player failed!"; 
                return LOGIC_FAILED;
            }

            player->DoUpdatePlayerData(logic_data->head.logic_id);

            ChangeStep(logic_data, 1);

            return LOGIC_YIELD;
        }

        static int32_t OnStep1(void* logic_data_void, void* msg_void, void* args_void)
        {
            (void)logic_data_void;
            (void)args_void;
            ProtoSs::Msg* msg =
                (ProtoSs::Msg*)msg_void;

            int32_t update_player_data_flag = msg->head().ret();
            int32_t player_idx = msg->head().player_idx();
            LOG(INFO)
                << "get player_data result[" << update_player_data_flag
                << "] player_idx[" << player_idx
                << "]";

            return LOGIC_FINISH;
        }
};

#endif
