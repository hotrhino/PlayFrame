/**
 * @file logic_login.h
 * @brief 登录逻辑
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-11-13
 */
#ifndef _LOGIC_LOGIN_H_
#define _LOGIC_LOGIN_H_

#include "app_def.h"
#include "logic_base.h"
#include "obj_mgr_module.h"

typedef struct {
    LogicDataHead   head;
    uint64_t        client_seq;
    int32_t         player_idx;
    uint64_t        uid;
    uint64_t        password_hash;
} LogicDataLogin;

class LogicLogin : public LogicBase
{
    public:
        LogicLogin()
        {
            logic_func_vec_.push_back(&LogicLogin::OnStep0);
            logic_func_vec_.push_back(&LogicLogin::OnStep1);
            logic_func_vec_.push_back(&LogicLogin::OnStep2);
        }

        virtual const char* LogicName() const
        {
            static const std::string logic_name = "LogicLogin";
            return logic_name.c_str();
        }

        static int32_t OnStep0(void* logic_data_void, void* msg_void, void* args_void)
        {
            LogicDataLogin* logic_data =
                (LogicDataLogin*)logic_data_void;
            ProtoCs::Msg* msg =
                (ProtoCs::Msg*)msg_void;
            ConnData* conn_data = (ConnData*)args_void;

            logic_data->client_seq = msg->head().seq();
            logic_data->player_idx = conn_data->player_idx;

            const ProtoCs::LoginReq& login_req = msg->login_req();

            ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(App::GetInstance());
            Player* player = obj_mgr_module->GetPlayer(logic_data->player_idx);
            if (player == NULL) {
                LOG(ERROR) << "get player failed!"; 
                return LOGIC_FAILED;
            }

            int32_t ret = 0;
            ret = Utils::CheckAccount(login_req.account().c_str(),
                    login_req.account().size());
            if (ret != 0) {
                LOG(ERROR) << "check account failed!";
                player->SendFailedCsRes(logic_data->client_seq,
                        ProtoCs::Msg::kLoginResFieldNumber, -1);
                return LOGIC_FAILED;
            }

            if (ret == 0) {
                logic_data->uid = Utils::Fnv64aHash(login_req.account().c_str(),
                        login_req.account().size());
            } else {
                logic_data->uid = Utils::ToNumber<uint64_t>(login_req.account());
            }

            ret = Utils::CheckPassword(login_req.password().c_str(),
                    login_req.password().size());
            if (ret != 0) {
                LOG(ERROR) << "check password failed!"; 
                player->SendFailedCsRes(logic_data->client_seq, 
                        ProtoCs::Msg::kLoginResFieldNumber, -1);
                return LOGIC_FAILED;
            }

            logic_data->password_hash = Utils::Fnv64aHash(login_req.password().c_str(), 
                    login_req.password().size());

            LOG(INFO)
                << "account[" << login_req.account()
                << "] uid[" << logic_data->uid 
                << "] password_hash[" << logic_data->password_hash
                << "]";

            int32_t old_player_idx = obj_mgr_module->FindPlayerIdxByUid(logic_data->uid);
            if (old_player_idx > 0) {
                Player* old_player = obj_mgr_module->GetPlayer(logic_data->uid);
                if (old_player == NULL) {
                    LOG(ERROR) << "get old_player failed!"; 
                    return LOGIC_FAILED;
                }
                old_player->SendServerKickOffNotify();
                old_player->DoUpdatePlayerData(logic_data->head.logic_id);
                ChangeStep(logic_data, 1);
                return LOGIC_YIELD;
            }

            player->DoGetPlayerData(logic_data->head.logic_id, logic_data->uid,
                    logic_data->password_hash);
            ChangeStep(logic_data, 2);

            return LOGIC_YIELD;
        }

        static int32_t OnStep1(void* logic_data_void, void* msg_void, void* args_void)
        {
            (void)args_void;
            LogicDataLogin* logic_data =
                (LogicDataLogin*)logic_data_void;
            ProtoSs::Msg* msg =
                (ProtoSs::Msg*)msg_void;

            int32_t update_player_data_flag = msg->head().ret();
            int32_t player_idx = msg->head().player_idx();
            LOG(INFO)
                << "get player_data result[" << update_player_data_flag
                << "] player_idx[" << player_idx
                << "]";

            ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(App::GetInstance());
            Player* player = obj_mgr_module->GetPlayer(logic_data->player_idx);
            if (player == NULL) {
                LOG(ERROR) << "get player failed!"; 
                return LOGIC_FAILED;
            }

            if (update_player_data_flag != ProtoSs::RET_SET_PLAYER_DATA_OK) {
                player->SendFailedCsRes(logic_data->client_seq, 
                        ProtoCs::Msg::kLoginResFieldNumber, -1);
                return LOGIC_FAILED;
            }

            player->DoGetPlayerData(logic_data->head.logic_id, logic_data->uid,
                    logic_data->password_hash);
            ChangeStep(logic_data, 2);

            return LOGIC_YIELD;
        }

        static int32_t OnStep2(void* logic_data_void, void* msg_void, void* args_void)
        {
            (void)args_void;
            LogicDataLogin* logic_data =
                (LogicDataLogin*)logic_data_void;
            ProtoSs::Msg* msg =
                (ProtoSs::Msg*)msg_void;

            // 从数据库初始化对象
            ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(App::GetInstance());
            Player* player = obj_mgr_module->GetPlayer(logic_data->player_idx); 

            int32_t ret = msg->head().ret();
            if (ret == ProtoSs::RET_GET_PLAYER_DATA_OK) {
                const ProtoSs::GetPlayerDataRes& get_player_data_res =
                    msg->get_player_data_res();
                Database::PlayerData player_data;
                player_data.ParseFromArray(get_player_data_res.player_data().c_str(),
                        get_player_data_res.player_data().size());
                LOG(INFO) << "\nplayer_data:\n" << player_data.Utf8DebugString();

                const Database::RoleInfo& role_info 
                    = player_data.role_info();
                player->SetPlayerByDb(&role_info);

                for (int32_t i = 0; i < player_data.city_list_size(); i++) {
                    const Database::CityInfo& city_info = player_data.city_list(i);
                    int32_t city_idx = obj_mgr_module->AddCity(logic_data->player_idx);
                    City* city = obj_mgr_module->GetCity(city_idx);
                    city->SetCityByDb(&city_info);
                    if (city != NULL) {
                        city->set_player_idx(logic_data->player_idx);
                    }
                }
                player->set_uid(logic_data->uid);
                player->set_password_hash(logic_data->password_hash);

                player->SendOkCsLoginRes(logic_data->client_seq);
                player->SendRoleInfoNotify();
            } else {
                player->SendFailedCsRes(logic_data->client_seq,
                        ProtoCs::Msg::kLoginResFieldNumber, -1);
            }

            return LOGIC_FINISH;
        }
};

#endif
