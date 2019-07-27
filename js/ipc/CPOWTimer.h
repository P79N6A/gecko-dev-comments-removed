






#ifndef CPOWTIMER_H
#define CPOWTIMER_H

#include "prinrval.h"
#include "jsapi.h"










class MOZ_STACK_CLASS CPOWTimer final {
  public:
    explicit inline CPOWTimer(JSContext* cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
    ~CPOWTimer();

  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    



    JSContext* cx_;

    



    PRIntervalTime startInterval_;
};

#endif
