






































#ifndef jsfuninlines_h___
#define jsfuninlines_h___

#include "jsfun.h"
#include "jsscript.h"

inline bool
js::IsConstructing(CallReceiver call)
{
    return IsConstructing(call.base());
}

inline bool
JSFunction::inStrictMode() const
{
    return script()->strictModeCode;
}

inline void
JSFunction::setJoinable()
{
    JS_ASSERT(isInterpreted());
    setSlot(METHOD_ATOM_SLOT, js::NullValue());
    flags |= JSFUN_JOINABLE;
}

inline void
JSFunction::setMethodAtom(JSAtom *atom)
{
    JS_ASSERT(joinable());
    setSlot(METHOD_ATOM_SLOT, js::StringValue(atom));
}

#endif 
