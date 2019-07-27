






#ifndef CPOWTIMER_H
#define CPOWTIMER_H

#include "prinrval.h"

class JSObject;

class MOZ_STACK_CLASS CPOWTimer {
  public:
    CPOWTimer(): startInterval(PR_IntervalNow()) {}
    ~CPOWTimer();

  private:
    PRIntervalTime startInterval;
};

#endif
