





#ifndef vm_Probes_h
#define vm_Probes_h

#ifdef INCLUDE_MOZILLA_DTRACE
#include "javascript-trace.h"
#endif

#include "vm/Stack.h"

namespace js {

namespace probes {































extern bool ProfilingActive;

extern const char nullName[];
extern const char anonymousName[];






bool CallTrackingActive(JSContext*);


bool EnterScript(JSContext*, JSScript*, JSFunction*, InterpreterFrame*);


void ExitScript(JSContext*, JSScript*, JSFunction*, bool popSPSFrame);


bool StartExecution(JSScript* script);


bool StopExecution(JSScript* script);




bool CreateObject(ExclusiveContext* cx, JSObject* obj);





bool FinalizeObject(JSObject* obj);







void DTraceEnterJSFun(JSContext* cx, JSFunction* fun, JSScript* script);
void DTraceExitJSFun(JSContext* cx, JSFunction* fun, JSScript* script);

} 

#ifdef INCLUDE_MOZILLA_DTRACE
static const char* ObjectClassname(JSObject* obj) {
    if (!obj)
        return "(null object)";
    const Class* clasp = obj->getClass();
    if (!clasp)
        return "(null)";
    const char* class_name = clasp->name;
    if (!class_name)
        return "(null class name)";
    return class_name;
}
#endif

inline bool
probes::CreateObject(ExclusiveContext* cx, JSObject* obj)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_CREATE_ENABLED())
        JAVASCRIPT_OBJECT_CREATE(ObjectClassname(obj), (uintptr_t)obj);
#endif

    return ok;
}

inline bool
probes::FinalizeObject(JSObject* obj)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_FINALIZE_ENABLED()) {
        const Class* clasp = obj->getClass();

        
        JAVASCRIPT_OBJECT_FINALIZE(nullptr, (char*)clasp->name, (uintptr_t)obj);
    }
#endif

    return ok;
}

} 

#endif 
