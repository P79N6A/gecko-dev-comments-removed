







#ifndef runnable_utils_h__
#define runnable_utils_h__

#include "nsThreadUtils.h"


namespace mozilla {

class runnable_args_base : public nsRunnable {
 public:

  NS_IMETHOD Run() = 0;
};


#include "runnable_utils_generated.h"


#define RUN_ON_THREAD(t, r, h) ((t && (t != nsRefPtr<nsIThread>(do_GetCurrentThread()))) ? t->Dispatch(r, h) : r->Run())
}

#endif

