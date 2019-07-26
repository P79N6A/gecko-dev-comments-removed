





#ifndef Debugger_inl_h__
#define Debugger_inl_h__

#include "vm/Debugger.h"

#include "vm/Stack-inl.h"

inline bool
js::Debugger::onLeaveFrame(JSContext *cx, AbstractFramePtr frame, bool ok)
{
    
    bool evalTraps = frame.isEvalFrame() &&
                     frame.script()->hasAnyBreakpointsOrStepMode();
    if (!cx->compartment()->getDebuggees().empty() || evalTraps)
        ok = slowPathOnLeaveFrame(cx, frame, ok);
    return ok;
}

#endif  
