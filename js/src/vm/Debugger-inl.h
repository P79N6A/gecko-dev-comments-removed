





#ifndef vm_Debugger_inl_h
#define vm_Debugger_inl_h

#include "vm/Debugger.h"

#include "vm/Stack-inl.h"

 inline bool
js::Debugger::onLeaveFrame(JSContext* cx, AbstractFramePtr frame, bool ok)
{
    MOZ_ASSERT_IF(frame.isInterpreterFrame(), frame.asInterpreterFrame() == cx->interpreterFrame());
    MOZ_ASSERT_IF(frame.script()->isDebuggee(), frame.isDebuggee());
    
    mozilla::DebugOnly<bool> evalTraps = frame.isEvalFrame() &&
                                         frame.script()->hasAnyBreakpointsOrStepMode();
    MOZ_ASSERT_IF(evalTraps, frame.isDebuggee());
    if (frame.isDebuggee())
        ok = slowPathOnLeaveFrame(cx, frame, ok);
    MOZ_ASSERT(!inFrameMaps(frame));
    return ok;
}

 inline js::Debugger*
js::Debugger::fromJSObject(JSObject* obj)
{
    MOZ_ASSERT(js::GetObjectClass(obj) == &jsclass);
    return (Debugger*) obj->as<NativeObject>().getPrivate();
}

 JSTrapStatus
js::Debugger::onEnterFrame(JSContext* cx, AbstractFramePtr frame)
{
    MOZ_ASSERT_IF(frame.script()->isDebuggee(), frame.isDebuggee());
    if (!frame.isDebuggee())
        return JSTRAP_CONTINUE;
    return slowPathOnEnterFrame(cx, frame);
}

 JSTrapStatus
js::Debugger::onDebuggerStatement(JSContext* cx, AbstractFramePtr frame)
{
    if (!cx->compartment()->isDebuggee())
        return JSTRAP_CONTINUE;
    return slowPathOnDebuggerStatement(cx, frame);
}

 JSTrapStatus
js::Debugger::onExceptionUnwind(JSContext* cx, AbstractFramePtr frame)
{
    if (!cx->compartment()->isDebuggee())
        return JSTRAP_CONTINUE;
    return slowPathOnExceptionUnwind(cx, frame);
}

#endif 
