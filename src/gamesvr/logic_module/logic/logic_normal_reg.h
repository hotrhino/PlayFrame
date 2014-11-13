/**
 * @file logic_normal_reg.h
 * @brief  正常注册逻辑
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-11-13
 */
#ifndef _LOGIC_NORMAL_REG_H_
#define _LOGIC_NORMAL_REG_H_

#include "app_def.h"
#include "logic_base.h"
#include "obj_mgr_module.h"

typedef struct {
    LogicDataHead   head;
    uint64_t        client_seq;
    int32_t         player_idx;
    char            account[32];
    char            password[16];
} LogicDataNormalReg;

class LogicNormalReg : public LogicBase
{
    public:
        LogicNormalReg()
        {
            logic_func_vec_.push_back(&LogicNormalReg::OnStep0);
            logic_func_vec_.push_back(&LogicNormalReg::OnStep1);
        }

        virtual const char* LogicName() const
        {
            static const std::string logic_name = "LogicNormalReg";
            return logic_name.c_str();
        }

        static int32_t OnStep0(void* logic_data_void, void* msg_void, void* args_void)
        {
            LogicDataNormalReg* logic_data =
                (LogicDataNormalReg*)logic_data_void;
            ProtoCs::Msg* msg =
                (ProtoCs::Msg*)msg_void;
            ConnData* conn_data = (ConnData*)args_void;

            const ProtoCs::NormalRegReq& normal_reg_req =
                msg->normal_reg_req();

            if (normal_reg_req.account().size() > 32) {
                LOG(ERROR) << "account size > 32";
                return LOGIC_FAILED;
            }

            strncpy(logic_data->account,
                    normal_reg_req.account().c_str(),
                    32);

            if (normal_reg_req.password().size() > 16) {
                LOG(ERROR) << "account size > 16"; 
                return LOGIC_FAILED;
            }

            strncpy(logic_data->password,
                    normal_reg_req.password().c_str(),
                    16);

            logic_data->client_seq = msg->head().seq();
            logic_data->player_idx = conn_data->player_idx;

            ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(App::GetInstance());
            Player* player = obj_mgr_module->GetPlayer(logic_data->player_idx);
            if (player == NULL) {
                LOG(ERROR) << "get player failed!"; 
                return LOGIC_FAILED;
            }

            int32_t ret = 0;
            ret = Utils::CheckAccount(normal_reg_req.account().c_str(),
                    normal_reg_req.account().size());
            if (ret != 0) {
                LOG(ERROR) << "check account failed!";
                player->SendFailedCsRes(logic_data->client_seq,
                        ProtoCs::Msg::kNormalRegResFieldNumber, -1);
                return LOGIC_FAILED;
            }

            ret = Utils::CheckPassword(normal_reg_req.password().c_str(),
                    normal_reg_req.password().size());
            if (ret != 0) {
                LOG(ERROR) << "check password failed!"; 
                player->SendFailedCsRes(logic_data->client_seq, 
                        ProtoCs::Msg::kNormalRegResFieldNumber, -1);
                return LOGIC_FAILED;
            }

            uint64_t uid = Utils::Fnv64aHash(
                    normal_reg_req.account().c_str(),
                    normal_reg_req.account().size());
            uint64_t password_hash = Utils::Fnv64aHash(
                    normal_reg_req.password().c_str(),
                    normal_reg_req.password().size());

            player->DoAccountReg(logic_data->head.logic_id, uid,
                    password_hash, logic_data->account);

            ChangeStep(logic_data, 1);

            return LOGIC_YIELD;
        }

        static int32_t OnStep1(void* logic_data_void, void* msg_void, void* args_void)
        {
            (void)args_void;
            LogicDataNormalReg* logic_data =
                (LogicDataNormalReg*)logic_data_void;
            ProtoSs::Msg* msg =
                (ProtoSs::Msg*)msg_void;

            ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(App::GetInstance());
            Player* player = obj_mgr_module->GetPlayer(logic_data->player_idx);
            if (player == NULL) {
                LOG(ERROR) << "get player failed!"; 
                return LOGIC_FAILED;
            }

            if (msg->head().ret() == ProtoSs::RET_ACCOUNT_REG_OK) {
                player->SendOkCsNormalRegRes(logic_data->client_seq,
                   logic_data->account, logic_data->password);
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
