





#ifndef vm_Debugger_inl_h
#define vm_Debugger_inl_h

#include "vm/Debugger.h"

#include "vm/Stack-inl.h"

inline bool
js::Debugger::onLeaveFrame(JSContext *cx, AbstractFramePtr frame, bool ok)
{
    
    bool evalTraps = frame.isEvalFrame() &&
                     frame.script()->hasAnyBreakpointsOrStepMode();
    if (cx->compartment()->debugMode() || evalTraps)
        ok = slowPathOnLeaveFrame(cx, frame, ok);
    return ok;
}

 inline js::Debugger *
js::Debugger::fromJSObject(JSObject *obj)
{
    JS_ASSERT(js::GetObjectClass(obj) == &jsclass);
    return (Debugger *) obj->getPrivate();
}

#endif 
