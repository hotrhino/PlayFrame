/**
 * @file logic_base.h
 * @brief  逻辑基类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-11-13
 */
#ifndef _LOGIC_BASE_H_
#define _LOGIC_BASE_H_

typedef int32_t (*LogicFunc)(void*, void*, void*);

typedef struct {
    uint64_t logic_id;      // logic唯一id
    int32_t  logic_type;    // logic类型
    int32_t  step;          // 进展到哪步(从0开始) 
} LogicDataHead;

enum ProcRet {
    LOGIC_FAILED = -1,
    LOGIC_YIELD = 0,
    LOGIC_FINISH = 99,
};

enum LogicType {
    LOGIC_NORMAL_REG = 1,
    LOGIC_QUICK_REG,
    LOGIC_LOGIN,
    LOGIC_UPDATE_PLAYER,
};

class LogicBase
{
    public:
        virtual const char* LogicName() const = 0;
        int32_t Proc(void* logic_data, void* msg, void* args)
        {
            LogicDataHead* logic_data_head = (LogicDataHead*)logic_data;
            if (logic_data_head->step < 0 ||
                logic_data_head->step >= (int32_t)logic_func_vec_.size()) {
                LOG(ERROR)
                    << "logic_data_head->step[" << logic_data_head->step
                    << "]";
                return LOGIC_FAILED;
            }

            return (*logic_func_vec_[logic_data_head->step])(logic_data, msg, args);
        }

        static void ChangeStep(void* logic_data, int32_t step)
        {
            LogicDataHead* logic_data_head = (LogicDataHead*)logic_data;
            logic_data_head->step = step;
        }

    protected:
        std::vector<LogicFunc> logic_func_vec_;
};

#endif
