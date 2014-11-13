/**
 * @file app_def.h
 * @brief App��������
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _APP_DEF_H_
#define _APP_DEF_H_
// base
#include "comm_def.h"
#include "utils.h"
#include "shm_pool.h"
#include "time_value.h"
#include "heap_timer.h"
#include "server_frame.h"
#include "msg_module_base.h"

// protocol
#include "database.pb.h"
#include "cs_msg.pb.h"
#include "ss_msg.pb.h"

#define PKG_BUF_SIZE 10240

// ǰ������
class App;

enum {
    MODULE_ID_CONFIG = 0x9fff,
    MODULE_ID_TIMER_MGR,
    MODULE_ID_MSG,
    MODULE_ID_PLAYER_MGR,
    MODULE_ID_LUA_ENGINE,
    MODULE_ID_LOGIC,
};

#endif
