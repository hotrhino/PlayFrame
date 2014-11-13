----------------------------------------
-- @file task_reg.lua
-- @brief 注册相关脚本
-- @author fergus <zfengzhen@gmail.com>
-- @version 
-- @date 2014-05-03
----------------------------------------

-- 产生字符串
function rand_str(num)
    local str = "abcdefghijklmnhopqrstuvwxyz0123456789"
    local ret = ""
    for i=1 ,num do --根据长度生成字符串
        local rchr = math.random(1,string.len(str))
        ret = ret .. string.sub(str, rchr, rchr)
    end
 
    return ret
end

-- 快速注册
function quick_reg(task_id, seq, player_idx, res_cmd)
    local player = OBJ_MGR_MODULE:GetPlayer(player_idx)
    if (player == nil) then
        LOG_INFO("%s", "player is nil")
        player:SendFailedCsRes(seq, res_cmd, -1)
        return
    end

    local uid = 0
    local password = ""
    local password_len = 0
    local password_hash = 1121

    for i=1, 3 do
        uid = gen_uid_num()
        password = rand_str(8)
        password_len = #password 
        password_hash = fnv_64a_hash(password, password_len)
        LOG_INFO("%s", tostring(password_hash))
        local account_reg_flag = CO_YIELD(player:DoAccountReg(task_id, uid, password_hash, ""))

        if (account_reg_flag == 0) then
            player:SendOkCsQuickRegRes(seq, uid, password)
            return
        end
    end

    player:SendFailedCsRes(seq, res_cmd, -1)

end

-- 正常注册
function normal_reg(task_id, account, password, seq, player_idx, res_cmd)
    local ret = 0
    LOG_INFO("task_id: %s ", tostring(task_id))
    LOG_INFO("account: %s ", tostring(account))
    LOG_INFO("password: %s ", tostring(password))
    LOG_INFO("seq: %s ", tostring(seq))
    LOG_INFO("player_idx: %s ", tostring(player_idx))
    LOG_INFO("res_cmd: %s ", tostring(res_cmd))

    local player = OBJ_MGR_MODULE:GetPlayer(player_idx)
    if (player == nil) then
        LOG_INFO("%s", "player is nil")
        player:SendFailedCsRes(seq, -1)
        return
    end

    ret = check_account(account, #account)
    if (ret ~= 0) then
        LOG_INFO("account not vaild [%d]", ret)
        player:SendFailedCsRes(seq, res_cmd, -1)
        return
    end    

    ret = check_password(password, #password)
    if (ret ~= 0) then
        LOG_INFO("passowd not vaild [%d]", ret)
        player:SendFailedCsRes(seq, res_cmd, -1)
        return
    end    

    local uid = fnv_64a_hash(account, #account)
    local password_hash = fnv_64a_hash(password, #password)
    local account_reg_flag = CO_YIELD(player:DoAccountReg(task_id, uid, password_hash, account))

    if (account_reg_flag == 0) then
        player:SendOkCsNormalRegRes(seq, account, password)
        return
    end

    player:SendFailedCsRes(seq, res_cmd, -1)

end
