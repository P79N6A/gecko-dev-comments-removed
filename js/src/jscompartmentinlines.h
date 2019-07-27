





#ifndef jscompartmentinlines_h
#define jscompartmentinlines_h

#include "jscompartment.h"

#include "gc/Barrier.h"

#include "jscntxtinlines.h"

inline void
JSCompartment::initGlobal(js::GlobalObject &global)
{
    JS_ASSERT(global.compartment() == this);
    JS_ASSERT(!global_);
    global_.set(&global);
}

js::GlobalObject *
JSCompartment::maybeGlobal() const
{
    JS_ASSERT_IF(global_, global_->compartment() == this);
    return global_;
}

js::GlobalObject *
JSCompartment::unsafeUnbarrieredMaybeGlobal() const
{
    return *global_.unsafeGet();
}

js::AutoCompartment::AutoCompartment(ExclusiveContext *cx, JSObject *target)
  : cx_(cx),
    origin_(cx->compartment_)
{
    cx_->enterCompartment(target->compartment());
}

js::AutoCompartment::AutoCompartment(ExclusiveContext *cx, JSCompartment *target)
  : cx_(cx),
    origin_(cx_->compartment_)
{
    cx_->enterCompartment(target);
}

js::AutoCompartment::~AutoCompartment()
{
    cx_->leaveCompartment(origin_);
}

inline bool
JSCompartment::wrap(JSContext *cx, JS::MutableHandleValue vp, JS::HandleObject existing)
{
    JS_ASSERT_IF(existing, vp.isObject());

    
    if (!vp.isMarkable())
        return true;

    



    if (vp.isSymbol())
        return true;

    
    if (vp.isString()) {
        JS::RootedString str(cx, vp.toString());
        if (!wrap(cx, str.address()))
            return false;
        vp.setString(str);
        return true;
    }

    JS_ASSERT(vp.isObject());

    



















#ifdef DEBUG
    JS::RootedObject cacheResult(cx);
#endif
    JS::RootedValue v(cx, vp);
    if (js::WrapperMap::Ptr p = crossCompartmentWrappers.lookup(js::CrossCompartmentKey(v))) {
#ifdef DEBUG
        cacheResult = &p->value().get().toObject();
#else
        vp.set(p->value());
        return true;
#endif
    }

    JS::RootedObject obj(cx, &vp.toObject());
    if (!wrap(cx, &obj, existing))
        return false;
    vp.setObject(*obj);
    JS_ASSERT_IF(cacheResult, obj == cacheResult);
    return true;
}

#endif 
