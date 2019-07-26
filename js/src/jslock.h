





#ifndef jslock_h
#define jslock_h

#ifdef JS_THREADSAFE

#ifdef JS_POSIX_NSPR

#include "vm/PosixNSPR.h"

#else 

# include "pratom.h"
# include "prcvar.h"
# include "prinit.h"
# include "prlock.h"
# include "prthread.h"

#endif

# define JS_ATOMIC_INCREMENT(p)      PR_ATOMIC_INCREMENT((int32_t *)(p))
# define JS_ATOMIC_DECREMENT(p)      PR_ATOMIC_DECREMENT((int32_t *)(p))
# define JS_ATOMIC_ADD(p,v)          PR_ATOMIC_ADD((int32_t *)(p), (int32_t)(v))
# define JS_ATOMIC_SET(p,v)          PR_ATOMIC_SET((int32_t *)(p), (int32_t)(v))

#else  

typedef struct PRThread PRThread;
typedef struct PRCondVar PRCondVar;
typedef struct PRLock PRLock;

# define JS_ATOMIC_INCREMENT(p)      (++*(p))
# define JS_ATOMIC_DECREMENT(p)      (--*(p))
# define JS_ATOMIC_ADD(p,v)          (*(p) += (v))
# define JS_ATOMIC_SET(p,v)          (*(p) = (v))

#endif 

#endif 
