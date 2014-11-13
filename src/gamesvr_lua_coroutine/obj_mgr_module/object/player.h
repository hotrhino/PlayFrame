/**
 * @file player.h
 * @brief Player对象
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "app_def.h"

#define PLAYER_DATA_MAX 20480
#define MAX_CITY_NUM_PER_PLAYER 100

class Player
{
    public:
        friend class ObjMgrModule;

        Player() {}
        ~Player() {}

        void init() { memset(this, 0, sizeof(*this)); }

        void set_conn_fd(int32_t conn_fd) { conn_fd_ = conn_fd; }
        int32_t conn_fd() const { return conn_fd_; }

        int32_t GetPlayerIdx();

        // 设置了uid 才算真正建立了Player
        void set_uid(uint64_t uid);
        uint64_t uid() const { return uid_; }

        void set_password_hash(uint64_t password_hash) { password_hash_ = password_hash; }
        uint64_t password_hash() const { return password_hash_; } 

        // 通过protobuf描述的blob数据初始化Player
        void SetPlayerByDb(const Database::RoleInfo* role_info);

        void DoUpdatePlayerData(uint64_t seq = 0);

        // 客户端错误回包
        void SendFailedCsRes(uint64_t seq, int32_t cmd, int32_t err_ret);

        // notify
        void SendRoleInfoNotify();
        void SendServerKickOffNotify();

        // 客户端成功回包
        void SendOkCsQuickRegRes(uint64_t seq, uint64_t uid, const char* password);
        void SendOkCsNormalRegRes(uint64_t seq, const char* account, const char* password);
        void SendOkCsLoginRes(uint64_t seq);

        // datasvr 操作
        void DoAccountReg(uint64_t seq, uint64_t uid, uint64_t password_hash, const char* account);
        void DoAccountVerify(uint64_t seq, uint64_t uid, uint64_t password);
        void DoGetPlayerData(uint64_t seq, uint64_t uid, uint64_t password);

    private:
        int32_t         conn_fd_;
        uint64_t        uid_;
        uint64_t        password_hash_;

        int32_t         city_list_[MAX_CITY_NUM_PER_PLAYER + 1];

        // role_info 注意跟database.proto保持一致
        int32_t         money_;
        int32_t         level_;
        // ---------------------------------
};

#endif // _PLAYER_H_
