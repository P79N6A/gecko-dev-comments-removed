






#ifndef CPOWTIMER_H
#define CPOWTIMER_H

#include "prinrval.h"

struct JSContext;
class JSObject;

class MOZ_STACK_CLASS CPOWTimer {
  public:
    CPOWTimer(JSObject *obj) : jsobj(obj), startInterval(PR_IntervalNow()) {}
    ~CPOWTimer();

  private:
    JSObject *jsobj;
    PRIntervalTime startInterval;
};

#endif
