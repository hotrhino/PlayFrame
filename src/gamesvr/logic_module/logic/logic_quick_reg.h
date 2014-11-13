/**
 * @file logic_quick_reg.h
 * @brief  快速注册逻辑
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-11-13
 */
#ifndef _LOGIC_QUICK_REG_H_
#define _LOGIC_QUICK_REG_H_

#include "app_def.h"
#include "logic_base.h"
#include "obj_mgr_module.h"

typedef struct {
    LogicDataHead   head;
    uint64_t        client_seq;
    int32_t         player_idx;
    uint64_t        uid;
    char            password[16];
} LogicDataQuickReg;

class LogicQuickReg : public LogicBase
{
    public:
        LogicQuickReg()
        {
            logic_func_vec_.push_back(&LogicQuickReg::OnStep0);
            logic_func_vec_.push_back(&LogicQuickReg::OnStep1);
        }

        virtual const char* LogicName() const
        {
            static const std::string logic_name = "LogicQuickReg";
            return logic_name.c_str();
        }

        static int32_t OnStep0(void* logic_data_void, void* msg_void, void* args_void)
        {
            LogicDataQuickReg* logic_data =
                (LogicDataQuickReg*)logic_data_void;
            ProtoCs::Msg* msg =
                (ProtoCs::Msg*)msg_void;
            ConnData* conn_data = (ConnData*)args_void;

            logic_data->uid = Utils::GenUidNum();
            std::string password;
            Utils::GenPassword8(password);
            strncpy(logic_data->password, password.c_str(), 16);
            logic_data->client_seq = msg->head().seq();
            logic_data->player_idx = conn_data->player_idx;

            ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(App::GetInstance());
            Player* player = obj_mgr_module->GetPlayer(logic_data->player_idx);
            if (player == NULL) {
                LOG(ERROR) << "get player failed!"; 
                return LOGIC_FAILED;
            }

            uint64_t password_hash = Utils::Fnv64aHash(
                    logic_data->password,
                    strlen(logic_data->password));

            player->DoAccountReg(logic_data->head.logic_id, logic_data->uid,
                    password_hash, "");

            ChangeStep(logic_data, 1);

            return LOGIC_YIELD;
        }

        static int32_t OnStep1(void* logic_data_void, void* msg_void, void* args_void)
        {
            (void)args_void;
            LogicDataQuickReg* logic_data =
                (LogicDataQuickReg*)logic_data_void;
            ProtoSs::Msg* msg =
                (ProtoSs::Msg*)msg_void;

            ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(App::GetInstance());
            Player* player = obj_mgr_module->GetPlayer(logic_data->player_idx);
            if (player == NULL) {
                LOG(ERROR) << "get player failed!"; 
                return LOGIC_FAILED;
            }

            if (msg->head().ret() == ProtoSs::RET_ACCOUNT_REG_OK) {
                player->SendOkCsQuickRegRes(logic_data->client_seq,
                   logic_data->uid, logic_data->password);
            } else {
                LOG(INFO)
                    << "normal reg failed! ret["
                    << msg->head().ret()
                    << "]";
                player->SendFailedCsRes(logic_data->client_seq, 
                        ProtoCs::Msg::kNormalRegResFieldNumber, -1);
            }
            return LOGIC_FINISH;
        }
};

#endif
