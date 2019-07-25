







































#ifndef CallObject_inl_h___
#define CallObject_inl_h___

#include "CallObject.h"

namespace js {

inline bool
CallObject::isForEval() const
{
    JS_ASSERT(getFixedSlot(CALLEE_SLOT).isObjectOrNull());
    JS_ASSERT_IF(getFixedSlot(CALLEE_SLOT).isObject(),
                 getFixedSlot(CALLEE_SLOT).toObject().isFunction());
    return getFixedSlot(CALLEE_SLOT).isNull();
}

inline js::StackFrame *
CallObject::maybeStackFrame() const
{
    return reinterpret_cast<js::StackFrame *>(getPrivate());
}

inline void
CallObject::setStackFrame(StackFrame *frame)
{
    return setPrivate(frame);
}

inline void
CallObject::setCallee(JSObject *callee)
{
    JS_ASSERT_IF(callee, callee->isFunction());
    setFixedSlot(CALLEE_SLOT, js::ObjectOrNullValue(callee));
}

inline JSObject *
CallObject::getCallee() const
{
    return getFixedSlot(CALLEE_SLOT).toObjectOrNull();
}

inline JSFunction *
CallObject::getCalleeFunction() const
{
    return getFixedSlot(CALLEE_SLOT).toObject().toFunction();
}

inline const js::Value &
CallObject::getArguments() const
{
    JS_ASSERT(!isForEval());
    return getFixedSlot(ARGUMENTS_SLOT);
}

inline void
CallObject::setArguments(const js::Value &v)
{
    JS_ASSERT(!isForEval());
    setFixedSlot(ARGUMENTS_SLOT, v);
}

inline const js::Value &
CallObject::arg(uintN i) const
{
    JS_ASSERT(i < getCalleeFunction()->nargs);
    return getSlot(RESERVED_SLOTS + i);
}

inline void
CallObject::setArg(uintN i, const js::Value &v)
{
    JS_ASSERT(i < getCalleeFunction()->nargs);
    setSlot(RESERVED_SLOTS + i, v);
}

inline const js::Value &
CallObject::var(uintN i) const
{
    JSFunction *fun = getCalleeFunction();
    JS_ASSERT(fun->nargs == fun->script()->bindings.countArgs());
    JS_ASSERT(i < fun->script()->bindings.countVars());
    return getSlot(RESERVED_SLOTS + fun->nargs + i);
}

inline void
CallObject::setVar(uintN i, const js::Value &v)
{
    JSFunction *fun = getCalleeFunction();
    JS_ASSERT(fun->nargs == fun->script()->bindings.countArgs());
    JS_ASSERT(i < fun->script()->bindings.countVars());
    setSlot(RESERVED_SLOTS + fun->nargs + i, v);
}

inline void
CallObject::copyValues(uintN nargs, Value *argv, uintN nvars, Value *slots)
{
    JS_ASSERT(slotInRange(RESERVED_SLOTS + nargs + nvars, SENTINEL_ALLOWED));
    copySlotRange(RESERVED_SLOTS, argv, nargs);
    copySlotRange(RESERVED_SLOTS + nargs, slots, nvars);
}

inline js::Value *
CallObject::argArray()
{
    js::DebugOnly<JSFunction*> fun = getCalleeFunction();
    JS_ASSERT(hasContiguousSlots(RESERVED_SLOTS, fun->nargs));
    return getSlotAddress(RESERVED_SLOTS);
}

inline js::Value *
CallObject::varArray()
{
    JSFunction *fun = getCalleeFunction();
    JS_ASSERT(hasContiguousSlots(RESERVED_SLOTS + fun->nargs,
                                 fun->script()->bindings.countVars()));
    return getSlotAddress(RESERVED_SLOTS + fun->nargs);
}

}

#endif 
