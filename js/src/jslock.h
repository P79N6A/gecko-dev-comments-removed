





































#ifndef jslock_h__
#define jslock_h__

#include "jsapi.h"

#ifdef JS_THREADSAFE

# include "pratom.h"
# include "prlock.h"
# include "prcvar.h"
# include "prthread.h"
# include "prinit.h"

# define JS_ATOMIC_INCREMENT(p)      PR_ATOMIC_INCREMENT((PRInt32 *)(p))
# define JS_ATOMIC_DECREMENT(p)      PR_ATOMIC_DECREMENT((PRInt32 *)(p))
# define JS_ATOMIC_ADD(p,v)          PR_ATOMIC_ADD((PRInt32 *)(p), (PRInt32)(v))
# define JS_ATOMIC_SET(p,v)          PR_ATOMIC_SET((PRInt32 *)(p), (PRInt32)(v))

#else  

# define JS_ATOMIC_INCREMENT(p)      (++*(p))
# define JS_ATOMIC_DECREMENT(p)      (--*(p))
# define JS_ATOMIC_ADD(p,v)          (*(p) += (v))
# define JS_ATOMIC_SET(p,v)          (*(p) = (v))

#endif 

namespace js {

class AutoAtomicIncrement
{
    int32_t *p;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoAtomicIncrement(int32_t *p JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : p(p) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ATOMIC_INCREMENT(p);
    }

    ~AutoAtomicIncrement() {
        JS_ATOMIC_DECREMENT(p);
    }
};

}  

#endif 
