




#ifndef jslock_h__
#define jslock_h__

#include "jsapi.h"

#ifdef JS_THREADSAFE

# include "prlock.h"
# include "prcvar.h"
# include "prthread.h"
# include "prinit.h"

#else  

typedef struct PRThread PRThread;
typedef struct PRCondVar PRCondVar;
typedef struct PRLock PRLock;

#endif 

#endif 
