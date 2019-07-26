





#ifndef vm_Probes_h
#define vm_Probes_h

#ifdef INCLUDE_MOZILLA_DTRACE
#include "javascript-trace.h"
#endif

#include "vm/Stack.h"

namespace js {

namespace Probes {































extern bool ProfilingActive;

extern const char nullName[];
extern const char anonymousName[];






bool callTrackingActive(JSContext *);





bool wantNativeAddressInfo(JSContext *);


bool enterScript(JSContext *, JSScript *, JSFunction *, StackFrame *);


bool exitScript(JSContext *, JSScript *, JSFunction *, AbstractFramePtr);
bool exitScript(JSContext *, JSScript *, JSFunction *, StackFrame *);


bool startExecution(JSScript *script);


bool stopExecution(JSScript *script);




bool createObject(ExclusiveContext *cx, JSObject *obj);





bool finalizeObject(JSObject *obj);



enum JITReportGranularity {
    JITREPORT_GRANULARITY_NONE = 0,
    JITREPORT_GRANULARITY_FUNCTION = 1,
    JITREPORT_GRANULARITY_LINE = 2,
    JITREPORT_GRANULARITY_OP = 3
};




JITReportGranularity
JITGranularityRequested(JSContext *cx);





void
discardExecutableRegion(void *start, size_t size);







void DTraceEnterJSFun(JSContext *cx, JSFunction *fun, JSScript *script);
void DTraceExitJSFun(JSContext *cx, JSFunction *fun, JSScript *script);

} 

#ifdef INCLUDE_MOZILLA_DTRACE
static const char *ObjectClassname(JSObject *obj) {
    if (!obj)
        return "(null object)";
    const Class *clasp = obj->getClass();
    if (!clasp)
        return "(null)";
    const char *class_name = clasp->name;
    if (!class_name)
        return "(null class name)";
    return class_name;
}
#endif

inline bool
Probes::createObject(ExclusiveContext *cx, JSObject *obj)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_CREATE_ENABLED())
        JAVASCRIPT_OBJECT_CREATE(ObjectClassname(obj), (uintptr_t)obj);
#endif

    return ok;
}

inline bool
Probes::finalizeObject(JSObject *obj)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_FINALIZE_ENABLED()) {
        const Class *clasp = obj->getClass();

        
        JAVASCRIPT_OBJECT_FINALIZE(nullptr, (char *)clasp->name, (uintptr_t)obj);
    }
#endif

    return ok;
}

} 

#endif 
