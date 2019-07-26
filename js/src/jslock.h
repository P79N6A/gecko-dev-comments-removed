





#ifndef jslock_h
#define jslock_h

#include "jsapi.h"

#ifdef JS_THREADSAFE

# include "prlock.h"
# include "prcvar.h"
# include "prthread.h"
# include "prinit.h"

namespace js {
    
    unsigned GetCPUCount();
}

#else  

typedef struct PRThread PRThread;
typedef struct PRCondVar PRCondVar;
typedef struct PRLock PRLock;

#endif 

#endif 
