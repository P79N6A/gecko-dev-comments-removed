





#include "jsworkers.h"

#include "ion/IonBuilder.h"

using namespace js;

#ifdef JS_THREADSAFE

bool
js::StartOffThreadIonCompile(JSContext *cx, ion::IonBuilder *builder)
{
    WorkerThreadState &state = *cx->runtime->workerThreadState;

    JS_ASSERT(state.numThreads);

    AutoLockWorkerThreadState lock(cx->runtime);

    if (!state.ionWorklist.append(builder))
        return false;

    state.notify(WorkerThreadState::WORKER);

    return true;
}






static void
FinishOffThreadIonCompile(ion::IonBuilder *builder)
{
    JSCompartment *compartment = builder->script->compartment();
    JS_ASSERT(compartment->rt->workerThreadState->isLocked());

    compartment->ionCompartment()->finishedOffThreadCompilations().append(builder);
}

static inline bool
CompiledScriptMatches(JSCompartment *compartment, JSScript *script, JSScript *target)
{
    if (script)
        return target == script;
    return target->compartment() == compartment;
}

void
js::CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script)
{
    WorkerThreadState &state = *compartment->rt->workerThreadState;

    ion::IonCompartment *ion = compartment->ionCompartment();
    if (!ion)
        return;

    AutoLockWorkerThreadState lock(compartment->rt);

    
    for (size_t i = 0; i < state.ionWorklist.length(); i++) {
        ion::IonBuilder *builder = state.ionWorklist[i];
        if (CompiledScriptMatches(compartment, script, builder->script)) {
            FinishOffThreadIonCompile(builder);
            state.ionWorklist[i--] = state.ionWorklist.back();
            state.ionWorklist.popBack();
        }
    }

    
    for (size_t i = 0; i < state.numThreads; i++) {
        const WorkerThread &helper = state.threads[i];
        while (helper.ionScript && CompiledScriptMatches(compartment, script, helper.ionScript))
            state.wait(WorkerThreadState::MAIN);
    }

    ion::OffThreadCompilationVector &compilations = ion->finishedOffThreadCompilations();

    
    for (size_t i = 0; i < compilations.length(); i++) {
        ion::IonBuilder *builder = compilations[i];
        if (CompiledScriptMatches(compartment, script, builder->script)) {
            ion::FinishOffThreadBuilder(builder);
            compilations[i--] = compilations.back();
            compilations.popBack();
        }
    }
}

bool
WorkerThreadState::init(JSRuntime *rt)
{
    workerLock = PR_NewLock();
    if (!workerLock)
        return false;

    mainWakeup = PR_NewCondVar(workerLock);
    if (!mainWakeup)
        return false;

    helperWakeup = PR_NewCondVar(workerLock);
    if (!helperWakeup)
        return false;

    
    numThreads = 0;

    threads = (WorkerThread*) rt->calloc_(sizeof(WorkerThread) * numThreads);
    if (!threads) {
        numThreads = 0;
        return false;
    }

    for (size_t i = 0; i < numThreads; i++) {
        WorkerThread &helper = threads[i];
        helper.runtime = rt;
        helper.thread = PR_CreateThread(PR_USER_THREAD,
                                        WorkerThread::ThreadMain, &helper,
                                        PR_PRIORITY_NORMAL, PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);
        if (!helper.thread) {
            for (size_t j = 0; j < numThreads; j++)
                threads[j].destroy();
            Foreground::free_(threads);
            threads = NULL;
            numThreads = 0;
            return false;
        }
    }

    return true;
}

WorkerThreadState::~WorkerThreadState()
{
    



    if (threads) {
        for (size_t i = 0; i < numThreads; i++)
            threads[i].destroy();
        Foreground::free_(threads);
    }

    if (workerLock)
        PR_DestroyLock(workerLock);

    if (mainWakeup)
        PR_DestroyCondVar(mainWakeup);

    if (helperWakeup)
        PR_DestroyCondVar(helperWakeup);
}

void
WorkerThreadState::lock()
{
    JS_ASSERT(!isLocked());
    PR_Lock(workerLock);
#ifdef DEBUG
    lockOwner = PR_GetCurrentThread();
#endif
}

void
WorkerThreadState::unlock()
{
    JS_ASSERT(isLocked());
#ifdef DEBUG
    lockOwner = NULL;
#endif
    PR_Unlock(workerLock);
}

#ifdef DEBUG
bool
WorkerThreadState::isLocked()
{
    return lockOwner == PR_GetCurrentThread();
}
#endif

void
WorkerThreadState::wait(CondVar which, uint32_t millis)
{
    JS_ASSERT(isLocked());
#ifdef DEBUG
    lockOwner = NULL;
#endif
    PR_WaitCondVar((which == MAIN) ? mainWakeup : helperWakeup,
                   millis ? PR_MillisecondsToInterval(millis) : PR_INTERVAL_NO_TIMEOUT);
#ifdef DEBUG
    lockOwner = PR_GetCurrentThread();
#endif
}

void
WorkerThreadState::notify(CondVar which)
{
    JS_ASSERT(isLocked());
    PR_NotifyAllCondVar((which == MAIN) ? mainWakeup : helperWakeup);
}

void
WorkerThread::destroy()
{
    WorkerThreadState &state = *runtime->workerThreadState;

    if (!thread)
        return;

    terminate = true;
    {
        AutoLockWorkerThreadState lock(runtime);
        state.notify(WorkerThreadState::WORKER);
    }
    PR_JoinThread(thread);
}


void
WorkerThread::ThreadMain(void *arg)
{
    PR_SetCurrentThreadName("Analysis Helper");
    static_cast<WorkerThread *>(arg)->threadLoop();
}

void
WorkerThread::threadLoop()
{
    WorkerThreadState &state = *runtime->workerThreadState;
    state.lock();

    while (true) {
        JS_ASSERT(!ionScript);

        while (state.ionWorklist.empty()) {
            state.wait(WorkerThreadState::WORKER);
            if (terminate) {
                state.unlock();
                return;
            }
        }

        ion::IonBuilder *builder = state.ionWorklist.popCopy();
        ionScript = builder->script;

        JS_ASSERT(ionScript->ion == ION_COMPILING_SCRIPT);

        state.unlock();

        {
            ion::IonContext ictx(NULL, ionScript->compartment(), &builder->temp());
            ion::CompileBackEnd(builder);
        }

        state.lock();

        ionScript = NULL;
        FinishOffThreadIonCompile(builder);

        



        state.notify(WorkerThreadState::MAIN);

        



        runtime->triggerOperationCallback();
    }
}

#else 

bool
js::StartOffThreadIonCompile(JSContext *cx, ion::IonBuilder *builder)
{
    JS_NOT_REACHED("Off thread compilation not available in non-THREADSAFE builds");
    return false;
}

void
js::CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script)
{
}

#endif 
