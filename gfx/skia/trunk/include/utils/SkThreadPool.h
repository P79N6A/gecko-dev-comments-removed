






#ifndef SkThreadPool_DEFINED
#define SkThreadPool_DEFINED

#include "SkCondVar.h"
#include "SkRunnable.h"
#include "SkTDArray.h"
#include "SkTInternalLList.h"

class SkThread;

class SkThreadPool {

public:
    


    static const int kThreadPerCore = -1;
    explicit SkThreadPool(int count);
    ~SkThreadPool();

    



    void add(SkRunnable*);

    


    void wait();

 private:
    struct LinkedRunnable {
        
        SkRunnable* fRunnable;

    private:
        SK_DECLARE_INTERNAL_LLIST_INTERFACE(LinkedRunnable);
    };

    enum State {
        kRunning_State,  
        kWaiting_State,  
        kHalting_State,  
    };

    SkTInternalLList<LinkedRunnable> fQueue;
    SkCondVar                        fReady;
    SkTDArray<SkThread*>             fThreads;
    State                            fState;
    int                              fBusyThreads;

    static void Loop(void*);  
};

#endif
