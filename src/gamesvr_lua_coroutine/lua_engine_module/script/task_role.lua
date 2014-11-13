----------------------------------------
-- @file task_role.lua
-- @brief 角色相关脚本
-- @author fergus <zfengzhen@gmail.com>
-- @version 
-- @date 2014-05-03
----------------------------------------

-- 登录
function login(task_id, uid, password_hash, seq, player_idx, res_cmd)
    LOG_INFO("task_id: %s", tostring(task_id))
    LOG_INFO("player_idx: %s", tostring(player_idx))
    LOG_INFO("res_cmd: %s", tostring(res_cmd))

    -- 必须用冒号表达式
    local player = OBJ_MGR_MODULE:GetPlayer(player_idx)
    if (player == nil) then
        LOG_INFO("%s", "player is nil")
        player:SendFailedCsRes(seq, res_cmd, -1)
    end

    local old_player_idx = OBJ_MGR_MODULE:FindPlayerIdxByUid(uid)
    if (old_player_idx > 0) then
        -- 踢下线
        local old_player = OBJ_MGR_MODULE:GetPlayer(old_player_idx)
        old_player:SendServerKickOffNotify()
        local update_player_data_flag = 1
        local update_count = 0
        while (update_player_data_flag ~= 0 and update_count < 5) do
            old_player:DoUpdatePlayerData(task_id)
            update_player_data_flag = CO_YIELD(old_player:DoUpdatePlayerData(task_id))
        end

        OBJ_MGR_MODULE:DelPlayer(old_player_idx)

        if (update_count >= 5) then
            player:SendFailedCsRes(seq, res_cmd, -1)
            return
        end
    end

    local get_player_data_flag = CO_YIELD(player:DoGetPlayerData(task_id, uid, password_hash))
    if (get_player_data_flag ~= 0) then
        player:SendFailedCsRes(seq, res_cmd, -1)
        return
    end

    player:set_uid(uid)
    player:set_password_hash(password_hash)

    player:SendOkCsLoginRes(seq)
    player:SendRoleInfoNotify()

    return
end
