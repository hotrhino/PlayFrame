/**
 * @file logic_module.h
 * @brief  异步逻辑框架
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-11-13
 */
#ifndef _LOGIC_MODULE_H_
#define _LOGIC_MODULE_H_

#include "app_def.h"
#include "logic_base.h"

class LogicModule : public AppModuleBase,
    public CallbackObject
{
    public:
        DISALLOW_COPY_AND_ASSIGN(LogicModule);

        LogicModule(App* app);
        virtual ~LogicModule();

        virtual void            ModuleInit();
        virtual void            ModuleFini();
        virtual const char*     ModuleName() const;
        static int32_t          ModuleId();
        static AppModuleBase*   CreateModule(App* app);

    public:
        int32_t OnTimer(int64_t logic_id, void* data)
        {
            (void)data;
            LOG(INFO) << "OnTimer: logic_id[" << logic_id << "] TIMEOUT!!!!";

            DeleteLogic(logic_id);

            return -1;
        }

        int64_t CreateLogic(int32_t logic_type, time_t task_delay_secs);
        void Proc(int64_t logic_id, void* msg, void* args);

        void DeleteLogic(int64_t logic_id)
        {
            LogicDataMap::iterator it = logic_data_map_.find(logic_id);
            if (it != logic_data_map_.end()) {
                LogicDataHead* head = (LogicDataHead*)(it->second);
                LOG(INFO)
                    << "delete logic_id[" << head->logic_id
                    << "] logic_type[" << head->logic_type
                    << "] step[" << head->step
                    << "]";
                free(it->second);
                it->second = NULL;
                logic_data_map_.erase(it);
            }
        }

        void Update()
        {
            TimeValue now = TimeValue::Time();
            heap_timer_->TimerPoll(now);
        }


        void* GetLogicData(int64_t logic_id)
        {
            LogicDataMap::iterator it = logic_data_map_.find(logic_id);
            if (it != logic_data_map_.end())
                return it->second;

            return NULL;
        }


    private:
        HeapTimer*          heap_timer_;
        typedef std::map<int64_t, void*> LogicDataMap;
        LogicDataMap logic_data_map_;
        typedef std::map<int32_t, LogicBase*> LogicMap;
        LogicMap logic_map_;
};

#endif

