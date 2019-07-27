






#ifndef CPOWTIMER_H
#define CPOWTIMER_H

#include "prinrval.h"










class MOZ_STACK_CLASS CPOWTimer final {
  public:
    explicit inline CPOWTimer(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
        : startInterval(PR_IntervalNow())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~CPOWTimer();

  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    


    PRIntervalTime startInterval;
};

#endif
