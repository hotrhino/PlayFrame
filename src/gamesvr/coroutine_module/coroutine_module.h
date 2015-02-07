/**
 * @file coroutine_module.h
 * @brief  协程框架
 * @author fergus <zfengzhen@gmail.com>
 * @version
 * @date 2015-02-07
 */
#ifndef _COROUTINE_MODULE_H_
#define _COROUTINE_MODULE_H_

#include "app_def.h"

typedef uintptr_t (*CoFunc)(void* args);

enum CoroutineStatus
{
    CO_READY,
    CO_SUSPENDED,
    CO_RUNNING,
    CO_FINISHED
};

typedef struct {
    void*       args;
    int32_t     status;
    ucontext_t  ctx;
    uintptr_t   yield;
    CoFunc      func;
    char        stack[0];
} Coroutine;

class CoroutineModule : public AppModuleBase,
    public CallbackObject
{
    public:
        DISALLOW_COPY_AND_ASSIGN(CoroutineModule);

        CoroutineModule(App* app);
        virtual ~CoroutineModule();

        virtual void            ModuleInit();
        virtual void            ModuleFini();
        virtual const char*     ModuleName() const;
        static int32_t          ModuleId();
        static AppModuleBase*   CreateModule(App* app);

    public:
        int32_t OnTimer(int64_t coroutine_id, void* data)
        {
            (void)data;
            LOG(INFO) << "OnTimer: coroutine_id[" << coroutine_id << "] TIMEOUT!!!!";

            DestroyCoroutine(coroutine_id);

            return -1;
        }

        void Update()
        {
            TimeValue now = TimeValue::Time();
            heap_timer_->TimerPoll(now);
        }

        Coroutine* GetCoroutine(int64_t coroutine_id)
        {
            CoroutineMap::const_iterator it = coroutine_map_.find(coroutine_id);
            if (it == coroutine_map_.end()) {
                LOG(ERROR) << "can't find coroutine_id["
                    << coroutine_id
                    << "]";
                return NULL;
            }

            return it->second;
        }

        inline void set_running(int64_t running)
        {
            running_ = running;
        }

        inline int64_t running() const
        {
            return running_;
        }

        inline const ucontext_t* main_context() const
        {
            return &main_context_;
        }

        static void Schedule(uint32_t low32, uint32_t hi32);
        int64_t CreateCoroutine(CoFunc func, void* args, time_t task_delay_secs);
        uintptr_t Resume(int64_t coroutine_id, uintptr_t y = NULL);
        static uintptr_t Yield(uintptr_t y = NULL);
        void DestroyCoroutine(int64_t coroutine_id);


    private:
        typedef std::map<int64_t, Coroutine*> CoroutineMap;

        HeapTimer*          heap_timer_;
        int32_t             stack_size_;
        int64_t             running_;
        ucontext_t          main_context_;
        CoroutineMap        coroutine_map_;
};

#endif

