






#ifndef SkThreadUtils_DEFINED
#define SkThreadUtils_DEFINED

#include "SkTypes.h"

class SkThread : SkNoncopyable {
public:
    typedef void (*entryPointProc)(void*);

    SkThread(entryPointProc entryPoint, void* data = NULL);

    


    ~SkThread();

    


    bool start();

    



    void join();

    




    bool setProcessorAffinity(unsigned int processor);

private:
    void* fData;
};

#endif
