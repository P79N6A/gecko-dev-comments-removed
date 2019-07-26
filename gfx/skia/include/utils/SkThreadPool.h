






#ifndef SkThreadPool_DEFINED
#define SkThreadPool_DEFINED

#include "SkCondVar.h"
#include "SkTDArray.h"
#include "SkTInternalLList.h"

class SkRunnable;
class SkThread;

class SkThreadPool {

public:
    


    explicit SkThreadPool(int count);
    ~SkThreadPool();

    



    void add(SkRunnable*);

 private:
    struct LinkedRunnable {
        
        SkRunnable* fRunnable;

    private:
        SK_DECLARE_INTERNAL_LLIST_INTERFACE(LinkedRunnable);
    };

    SkTInternalLList<LinkedRunnable>    fQueue;
    SkCondVar                           fReady;
    SkTDArray<SkThread*>                fThreads;
    bool                            fDone;

    static void Loop(void*);  
};

#endif
