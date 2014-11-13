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

            DeleteLogicData(logic_id);

            return -1;
        }

        void Update()
        {
            TimeValue now = TimeValue::Time();
            heap_timer_->TimerPoll(now);
        }

        int64_t CreateLogic(int32_t logic_type, time_t task_delay_secs);
        void Proc(int64_t logic_id, void* msg, void* args);
        void DeleteLogicData(int64_t logic_id);
        void* GetLogicData(int64_t logic_id);
        const char* GetLogicName(int32_t logic_type);


    private:
        typedef std::map<int64_t, void*> LogicDataMap;
        typedef std::map<int32_t, LogicBase*> LogicTypeMap;
        typedef std::map<int32_t, size_t> LogicDataSizeMap;
        HeapTimer*          heap_timer_;
        LogicDataMap        logic_data_map_;
        LogicTypeMap        logic_type_map_;
        LogicDataSizeMap    logic_data_size_map_;
};

#endif

