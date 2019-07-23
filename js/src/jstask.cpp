





































#include "jstask.h"

#ifdef JS_THREADSAFE
static void start(void* arg) {
    ((JSBackgroundThread*)arg)->work();
}

JSBackgroundThread::JSBackgroundThread()
  : thread(NULL), stack(NULL), lock(NULL), wakeup(NULL), shutdown(false)
{
}

JSBackgroundThread::~JSBackgroundThread()
{
    if (wakeup)
        PR_DestroyCondVar(wakeup);
    if (lock)
        PR_DestroyLock(lock);
    
}

bool
JSBackgroundThread::init()
{
    if (!(lock = PR_NewLock()))
        return false;
    if (!(wakeup = PR_NewCondVar(lock)))
        return false;
    thread = PR_CreateThread(PR_USER_THREAD, start, this, PR_PRIORITY_LOW,
                             PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);
    return !!thread;
}

void
JSBackgroundThread::cancel()
{
    PR_Lock(lock);
    if (shutdown) {
        PR_Unlock(lock);
        return;
    }
    shutdown = true;
    PR_NotifyCondVar(wakeup);
    PR_Unlock(lock);
    PR_JoinThread(thread);
}

void
JSBackgroundThread::work()
{
    PR_Lock(lock);
    do {
        PR_WaitCondVar(wakeup, PR_INTERVAL_NO_TIMEOUT);
        JSBackgroundTask* task;
        while ((task = stack) != NULL) {
            stack = task->next;
            PR_Unlock(lock);
            task->run();
            delete task;
            PR_Lock(lock);
        }
    } while (!shutdown);
    PR_Unlock(lock);
}

bool
JSBackgroundThread::busy()
{
    return !!stack; 
}

void
JSBackgroundThread::schedule(JSBackgroundTask* task)
{
    PR_Lock(lock);
    if (shutdown) {
        PR_Unlock(lock);
        task->run();
        delete task;
        return;
    }
    task->next = stack;
    stack = task;
    PR_NotifyCondVar(wakeup);
    PR_Unlock(lock);
}

#endif
