





#ifndef jslock_h
#define jslock_h

#ifdef JS_THREADSAFE

#ifdef JS_POSIX_NSPR

#include "vm/PosixNSPR.h"

#else 

# include "prcvar.h"
# include "prinit.h"
# include "prlock.h"
# include "prthread.h"

#endif

#else  

typedef struct PRThread PRThread;
typedef struct PRCondVar PRCondVar;
typedef struct PRLock PRLock;

#endif 

#endif 
