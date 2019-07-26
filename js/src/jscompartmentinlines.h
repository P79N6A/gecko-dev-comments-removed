





#ifndef jscompartmentinlines_h
#define jscompartmentinlines_h

#include "jscompartment.h"

#include "jscntxtinlines.h"

inline void
JSCompartment::initGlobal(js::GlobalObject &global)
{
    JS_ASSERT(global.compartment() == this);
    JS_ASSERT(!global_);
    global_ = &global;
}

js::GlobalObject *
JSCompartment::maybeGlobal() const
{
    JS_ASSERT_IF(global_, global_->compartment() == this);
    return global_;
}

js::AutoCompartment::AutoCompartment(JSContext *cx, JSObject *target)
  : cx_(cx),
    origin_(cx->compartment())
{
    cx_->enterCompartment(target->compartment());
}

js::AutoCompartment::AutoCompartment(ExclusiveContext *cx, JSCompartment *target)
  : cx_(cx->asJSContext()),
    origin_(cx_->compartment())
{
    cx_->enterCompartment(target);
}

js::AutoCompartment::~AutoCompartment()
{
    cx_->leaveCompartment(origin_);
}

#endif 
