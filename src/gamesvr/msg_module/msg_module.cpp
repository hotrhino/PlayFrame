/**
 * @file msg_module.cpp
 * @brief 消息模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "app.h"
#include "msg_module.h"
#include "config_module.h"
#include "obj_mgr_module.h"
#include "logic_module.h"

MsgModule::MsgModule(App* app) :
    AppModuleBase(app),
    zmq_ctx_(NULL),
    connsvr_zmq_sock_(NULL),
    datasvr_zmq_sock_(NULL)

{}

MsgModule::~MsgModule()
{}

void MsgModule::ModuleInit()
{
    ConfigModule* conf_module = FindModule<ConfigModule>(app_);
    // ZMQ初始化
    zmq_ctx_  = zmq_init(1);
    PCHECK(zmq_ctx_ != NULL)
        << "zmq_init error!";
    connsvr_zmq_sock_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
    PCHECK(connsvr_zmq_sock_ != NULL)
        << "zmq_socket error!";
    PCHECK(zmq_bind(connsvr_zmq_sock_, conf_module->config().connsvr_zmq_addr().c_str()) == 0)
        << "zmq_bind error!";

    datasvr_zmq_sock_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
    PCHECK(datasvr_zmq_sock_ != NULL)
        << "zmq_socket error!";
    PCHECK(zmq_connect(datasvr_zmq_sock_, conf_module->config().datasvr_zmq_addr().c_str()) == 0)
        << "zmq_connect error!";

    cmd_logic_type_map_.insert(CmdLogicTypeMap::value_type(
                ProtoCs::Msg::kQuickRegReqFieldNumber, LOGIC_QUICK_REG));
    cmd_logic_type_map_.insert(CmdLogicTypeMap::value_type(
                ProtoCs::Msg::kNormalRegReqFieldNumber, LOGIC_NORMAL_REG));
    cmd_logic_type_map_.insert(CmdLogicTypeMap::value_type(
                ProtoCs::Msg::kLoginReqFieldNumber, LOGIC_LOGIN));

    // 注册消息处理函数
    REGISTER_MSG_BEGIN(MsgModule, ProtoCs::Msg)
        REGISTER_MSG(this, ProtoCs::Msg::kQuickRegReqFieldNumber, &MsgModule::OnLogicMultiStepStart)
        REGISTER_MSG(this, ProtoCs::Msg::kNormalRegReqFieldNumber, &MsgModule::OnLogicMultiStepStart)
        REGISTER_MSG(this, ProtoCs::Msg::kLoginReqFieldNumber, &MsgModule::OnLogicMultiStepStart)
        REGISTER_MSG_END;

    REGISTER_MSG_BEGIN(MsgModule, ProtoSs::Msg)
        REGISTER_MSG(this, ProtoSs::Msg::kAccountRegResFieldNumber, &MsgModule::OnLogicMultiStepResume)
        REGISTER_MSG(this, ProtoSs::Msg::kGetPlayerDataResFieldNumber, &MsgModule::OnLogicMultiStepResume)
        REGISTER_MSG(this, ProtoSs::Msg::kSetPlayerDataResFieldNumber, &MsgModule::OnLogicMultiStepResume)
        REGISTER_MSG_END;

    LOG(INFO) << ModuleName() << " init ok!";
}

void MsgModule::ModuleFini()
{
    // zmq 释放
    if (connsvr_zmq_sock_ != NULL)
        zmq_close(connsvr_zmq_sock_);
    if (datasvr_zmq_sock_ != NULL)
        zmq_close(datasvr_zmq_sock_);
    if (zmq_ctx_ != NULL)
        zmq_term(zmq_ctx_);

    cmd_logic_type_map_.clear();

    LOG(INFO) << ModuleName() << " fini completed!";
}

const char* MsgModule::ModuleName() const
{
    static const std::string ModuleName = "MsgModule";
    return ModuleName.c_str();
}

int32_t MsgModule::ModuleId()
{
    return MODULE_ID_MSG;
}

AppModuleBase* MsgModule::CreateModule(App* app)
{
    MsgModule* module = new MsgModule(app);
    if (module != NULL) {
        module->ModuleInit();
    }

    return static_cast<AppModuleBase*>(module);
}

int32_t MsgModule::SendToConnsvr(ProtoCs::Msg* msg, int32_t conn_cmd, int32_t conn_fd, int32_t player_idx)
{
    static ConnData conn_data;
    conn_data.conn_cmd = conn_cmd;
    conn_data.player_idx = player_idx;
    conn_data.conn_fd = conn_fd;

    static char send_buf[PKG_BUF_SIZE + CONN_DATA_SIZE];

    // --- 组包 start ---
    char* p = send_buf;
    memcpy(p, &conn_data, CONN_DATA_SIZE);
    p += CONN_DATA_SIZE;

    if (msg != NULL) {
        if (msg->SerializeToArray(p, PKG_BUF_SIZE) == false) {
            printf("msg.SerializeToArray error.\n");
            return -1;
        }

        p += msg->ByteSize();
    }

    int32_t send_len = p - send_buf;
    // --- 组包 end --- 

    int32_t ret = zmq_send(connsvr_zmq_sock_, send_buf, send_len, ZMQ_DONTWAIT);
    if (ret < 0) {
        LOG(ERROR)
            << "zmq_send error."; 
        return -1;
    }
    return 0; 
}


void MsgModule::Run()
{
    ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(app_);

    int32_t loop_times = 1000;
    static char buf[PKG_BUF_SIZE];
    int32_t len = 0;
    while (loop_times--) {
        // 处理connsvr消息
        len = zmq_recv(connsvr_zmq_sock_, buf, 1024, ZMQ_DONTWAIT);
        if (len > 0) {
            static ConnData conn_data;
            memcpy((void*)&conn_data, buf, CONN_DATA_SIZE);

            const char* msg_buf = Utils::GetMsgFromConn(buf);
            int32_t msg_len = Utils::GetMsgLenFromConn(len);

            if (conn_data.conn_cmd == CONN_START) {
                // 分配内存对象 
                LOG(INFO) << "CONN_START";
                int32_t player_idx = obj_mgr_module->AddPlayer();
                Player* player =  obj_mgr_module->GetPlayer(player_idx);
                if (player == NULL) {
                    // 内存池满了
                    LOG(ERROR) << "create_player error!";

                    static ProtoCs::Msg msg;
                    msg.Clear();
                    if (msg.ParseFromArray(msg_buf, msg_len) == false) {
                        LOG(ERROR)
                            << "protobuf parse error!";
                        return;
                    }

                    int32_t cmd = msg.head().cmd();
                    if (cmd == ProtoCs::Msg::kLoginReqFieldNumber) {
                        msg.mutable_head()->set_cmd(ProtoCs::Msg::kLoginResFieldNumber);
                        msg.mutable_head()->set_ret(ProtoCs::RET_LOGIN_GAMESVR_FULL);
                    } else if (cmd == ProtoCs::Msg::kQuickRegReqFieldNumber) {
                        msg.mutable_head()->set_cmd(ProtoCs::Msg::kQuickRegResFieldNumber);
                        msg.mutable_head()->set_ret(ProtoCs::RET_QUICK_REG_GAMESVR_FULL);
                    } else if (cmd == ProtoCs::Msg::kNormalRegReqFieldNumber) {
                        msg.mutable_head()->set_cmd(ProtoCs::Msg::kNormalRegResFieldNumber);
                        msg.mutable_head()->set_ret(ProtoCs::RET_NORMAL_REG_GAMESVR_FULL);
                    }

                    SendCloseToConnsvr(&msg, conn_data.conn_fd, 0);
                    return;
                }
                conn_data.player_idx = player_idx;
                player->set_conn_fd(conn_data.conn_fd);
            } else if (conn_data.conn_cmd == CONN_STOP) {
                LOG(INFO) << "CONN_STOP";
                LOG(INFO)
                    << "player_idx[" << conn_data.player_idx
                    << "] client disconnected";
                Player* player =  obj_mgr_module->GetPlayer(conn_data.player_idx);
                if (player != NULL) {
                    LOG(INFO) << "update_player_data";
                    LogicModule* logic_module = FindModule<LogicModule>(app_);
                    int64_t logic_id = logic_module->CreateLogic(LOGIC_UPDATE_PLAYER, 5);
                    if (logic_id <= 0) {
                        LOG(ERROR) << "logic_id = " << logic_id << "error"; 
                    }
                    int32_t player_idx = conn_data.player_idx;
                    logic_module->Proc(logic_id, NULL, (void*)&player_idx);
                } else {
                    LOG(ERROR) << "update_player_data GetPlayer error";
                }
                obj_mgr_module->DelPlayer(conn_data.player_idx);
                return;
            } else if (conn_data.conn_cmd == CONN_PROC) {
                LOG(INFO) << "CONN_PROC";
            }

            HandleRequest<ProtoCs::Msg>(msg_buf, msg_len, &conn_data);
        }
        // 处理datasvr消息
        len = zmq_recv(datasvr_zmq_sock_, buf, 1024, ZMQ_DONTWAIT);
        if (len > 0) {
            HandleRequest<ProtoSs::Msg>(buf, len, NULL);
        }
    }
}

int32_t MsgModule::OnLogicOneStepStart(ProtoCs::Msg* msg, void* args)
{
    LogicModule* logic_module = FindModule<LogicModule>(app_);
    int32_t logic_type = cmd_logic_type_map_[msg->head().cmd()];
    LOG(INFO)
        << "logic_type[" << logic_type
        << "] name[" << logic_module->GetLogicName(logic_type)
        << "]";
    int64_t logic_id = logic_module->CreateLogic(logic_type, 0);
    if (logic_id <= 0) {
        LOG(ERROR) << "logic_id = " << logic_id << " error"; 
        return -1;
    }

    logic_module->Proc(logic_id, (void*)msg, args);

    return 0;
}

int32_t MsgModule::OnLogicMultiStepStart(ProtoCs::Msg* msg, void* args)
{
    LogicModule* logic_module = FindModule<LogicModule>(app_);
    int32_t logic_type = cmd_logic_type_map_[msg->head().cmd()];
    LOG(INFO)
        << "logic_type[" << logic_type
        << "] name[" << logic_module->GetLogicName(logic_type)
        << "]";
    int64_t logic_id = logic_module->CreateLogic(logic_type, 10);
    if (logic_id <= 0) {
        LOG(ERROR) << "logic_id = " << logic_id << " error"; 
        return -1;
    }

    logic_module->Proc(logic_id, (void*)msg, args);

    return 0;
}

int32_t MsgModule::OnLogicMultiStepResume(ProtoSs::Msg* msg, void* args)
{
    LogicModule* logic_module = FindModule<LogicModule>(app_);
    int64_t logic_id = msg->head().seq();
    logic_module->Proc(logic_id, (void*)msg, args);
    return 0;
}
