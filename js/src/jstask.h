





































#ifndef jstask_h___
#define jstask_h___

class JSBackgroundTask {
    friend class JSBackgroundThread;
    JSBackgroundTask* next;
  public:
    virtual void run() = 0;
};

#ifdef JS_THREADSAFE

#include "prthread.h"
#include "prlock.h"
#include "prcvar.h"

class JSBackgroundThread {
    PRThread*         thread;
    JSBackgroundTask* stack;
    PRLock*           lock;
    PRCondVar*        wakeup;
    bool              shutdown;

  public:
    JSBackgroundThread();
    ~JSBackgroundThread();

    bool init();
    void cancel();
    void work();
    bool busy();
    void schedule(JSBackgroundTask* task);
};

#else

class JSBackgroundThread {
  public:
    void schedule(JSBackgroundTask* task) {
        task->run();
    }
};

#endif

#endif 
