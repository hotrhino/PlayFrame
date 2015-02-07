/**
 * @file coroutine_module.cpp
 * @brief  协程框架
 * @author fergus <zfengzhen@gmail.com>
 * @version
 * @date 2014-02-07
 */
#include "app.h"
#include <ucontext.h>
#include "coroutine_module.h"
#include "config_module.h"

CoroutineModule::CoroutineModule(App* app) :
    AppModuleBase(app),
    heap_timer_(NULL),
    stack_size_(8192),
    running_(-1)
{}

CoroutineModule::~CoroutineModule()
{}

void CoroutineModule::ModuleInit()
{
    ConfigModule* conf_module = FindModule<ConfigModule>(app_);
    heap_timer_ = new HeapTimer(conf_module->config().logic_timer_init_size());

    CHECK(heap_timer_ != NULL)
        << "timer heap init error!";
    SetHandler(this, &CoroutineModule::OnTimer);

    stack_size_ = 8192;

    LOG(INFO) << ModuleName() << " init ok!";
}

void CoroutineModule::ModuleFini()
{
    if (heap_timer_ != NULL) {
        delete heap_timer_;
        heap_timer_ = NULL;
    }

    for (CoroutineMap::iterator it = coroutine_map_.begin();
            it != coroutine_map_.end(); ++it) {
        if (it->second)
            free (it->second);
    }
    coroutine_map_.clear();

    LOG(INFO) << ModuleName() << " fini completed!";
}

const char* CoroutineModule::ModuleName() const
{
    static const std::string ModuleName = "CoroutineModule";
    return ModuleName.c_str();
}

int32_t CoroutineModule::ModuleId()
{
    return MODULE_ID_COROUTINE;
}

AppModuleBase* CoroutineModule::CreateModule(App* app)
{
    CoroutineModule* module = new CoroutineModule(app);
    if (module != NULL) {
        module->ModuleInit();
    }

    return static_cast<AppModuleBase*>(module);
}

void CoroutineModule::Schedule(uint32_t low32, uint32_t hi32)
{
    LOG(INFO) << __FUNCTION__ << " start";
    uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
    CoroutineModule* co_module = (CoroutineModule*) ptr;

    int64_t running = co_module->running_;

    Coroutine* co = co_module->GetCoroutine(running);
    assert(co);

    co->yield = co->func(co->args);

    co_module->running_ = -1;
    co->status = CO_FINISHED;
    LOG(INFO) << __FUNCTION__ << " end";
}

int64_t CoroutineModule::CreateCoroutine(CoFunc func, void* args, time_t task_delay_secs)
{
    LOG(INFO) << __FUNCTION__ << " start";
    // 等待时间为0为即时任务, 不需要注册定时器
    int64_t coroutine_id = 0;
    if (task_delay_secs > 0) {
        coroutine_id = heap_timer_->RegisterTimer(
                TimeValue(task_delay_secs),
                TimeValue(task_delay_secs),
                this,
                NULL);
    }

    Coroutine* co = (Coroutine*)malloc(sizeof(Coroutine) + stack_size_);
    if (co == NULL) {
        LOG(ERROR) << "malloc Coroutine ERROR";
        return -1;
    }

    co->args  = args;
    co->func = func;
    co->yield = 0;
    co->status = CO_READY;

    coroutine_map_.insert(CoroutineMap::value_type(coroutine_id, co));
    LOG(INFO) << "insert coroutine_id: " << coroutine_id;

    LOG(INFO) << __FUNCTION__ << " end";
    return coroutine_id;
}

uintptr_t CoroutineModule::Resume(int64_t coroutine_id, uintptr_t y)
{
    LOG(INFO) << __FUNCTION__ << " start";
    Coroutine* co = GetCoroutine(coroutine_id);
    if (co == NULL || co->status == CO_RUNNING)
        return NULL;

    co->yield = y;
    switch (co->status) {
        case CO_READY: {
                getcontext(&co->ctx);

                co->status = CO_RUNNING;
                co->ctx.uc_stack.ss_sp = co->stack;
                co->ctx.uc_stack.ss_size = stack_size_;
                co->ctx.uc_link = &main_context_;

                running_ = coroutine_id;
                uintptr_t ptr = (uintptr_t)this;
                makecontext(&co->ctx, (void (*)())Schedule, 2,
                        (uint32_t)ptr, (uint32_t)(ptr>>32));
                swapcontext(&main_context_, &co->ctx);
            }
            break;
        case CO_SUSPENDED:
            {
                running_ = coroutine_id;
                co->status = CO_RUNNING;
                swapcontext(&main_context_, &co->ctx);
            }
            break;
        default:
            assert(0);
    }

    uintptr_t ret = co->yield;
    co->yield = 0;

    if (running_ == -1 && co->status == CO_FINISHED)
        DestroyCoroutine(coroutine_id);
    LOG(INFO) << __FUNCTION__ << " end";

    return ret;
}

uintptr_t CoroutineModule::Yield(uintptr_t y)
{
    LOG(INFO) << __FUNCTION__ << " start";
    CoroutineModule* co_module = FindModule<CoroutineModule>(App::GetInstance());
    if (co_module->running() < 0)
        return 0;

    int64_t cur = co_module->running();
    co_module->set_running(-1);

    Coroutine* co = co_module->GetCoroutine(cur);

    co->yield = y;
    co->status = CO_SUSPENDED;

    swapcontext(&co->ctx, co_module->main_context());

    uintptr_t ret = co->yield;
    co->yield = 0;
    LOG(INFO) << __FUNCTION__ << " end";
    return ret;
}

void CoroutineModule::DestroyCoroutine(int64_t coroutine_id)
{
    LOG(INFO) << __FUNCTION__ << " start";
    Coroutine* co = GetCoroutine(coroutine_id);
    if (co == NULL)
        return;

    free(co);
    coroutine_map_.erase(coroutine_id);
    if (coroutine_id != 0)
        heap_timer_->UnregisterTimer(coroutine_id);
    LOG(INFO) << __FUNCTION__ << " end";
}
