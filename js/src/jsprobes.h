


































#ifndef _JSPROBES_H
#define _JSPROBES_H

#ifdef INCLUDE_MOZILLA_DTRACE
#include "javascript-trace.h"
#endif
#include "jspubtd.h"
#include "jsprvtd.h"

namespace js {

class Probes {
    static bool ProfilingActive;
    static bool controlProfilers(JSContext *cx, bool toState);

    static const char nullName[];
    static const char anonymousName[];

    static const char *FunctionName(JSContext *cx, const JSFunction *fun, JSAutoByteString* bytes)
    {
        if (!fun)
            return nullName;
        JSAtom *atom = const_cast<JSAtom*>(fun->atom);
        if (!atom)
            return anonymousName;
        return bytes->encode(cx, atom) ? bytes->ptr() : nullName;
    }

    static const char *ScriptFilename(const JSScript *script) {
        if (! script)
            return "(null)";
        if (! script->filename)
            return "(anonymous)";
        return script->filename;
    }

    static const char *ObjectClassname(JSObject *obj) {
        if (! obj)
            return "(null object)";
        Class *clasp = obj->getClass();
        if (! clasp)
            return "(null)";
        const char *class_name = clasp->name;
        if (! class_name)
            return "(null class name)";
        return class_name;
    }

    static void current_location(JSContext *cx, int* lineno, char const **filename);

    static const char *FunctionClassname(const JSFunction *fun);
    static const char *ScriptFilename(JSScript *script);
    static int FunctionLineNumber(JSContext *cx, const JSFunction *fun);

    static void enterJSFunImpl(JSContext *cx, JSFunction *fun, JSScript *script);
    static void handleFunctionReturn(JSContext *cx, JSFunction *fun, JSScript *script);
    static void finalizeObjectImpl(JSObject *obj);
  public:
    static bool createRuntime(JSRuntime *rt);
    static bool destroyRuntime(JSRuntime *rt);
    static bool shutdown();

    









    static bool pauseProfilers(JSContext *cx) {
        bool prevState = ProfilingActive;
        controlProfilers(cx, false);
        return prevState;
    }
    static bool resumeProfilers(JSContext *cx) {
        bool prevState = ProfilingActive;
        controlProfilers(cx, true);
        return prevState;
    }

    static bool callTrackingActive(JSContext *);

    static bool enterJSFun(JSContext *, JSFunction *, JSScript *, int counter = 1);
    static bool exitJSFun(JSContext *, JSFunction *, JSScript *, int counter = 0);

    static bool startExecution(JSContext *cx, JSScript *script);
    static bool stopExecution(JSContext *cx, JSScript *script);

    static bool resizeHeap(JSCompartment *compartment, size_t oldSize, size_t newSize);

    
    static bool createObject(JSContext *cx, JSObject *obj);

    static bool resizeObject(JSContext *cx, JSObject *obj, size_t oldSize, size_t newSize);

    
    static bool finalizeObject(JSObject *obj);

    







    static bool createString(JSContext *cx, JSString *string, size_t length);

    


    static bool finalizeString(JSString *string);

    static bool compileScriptBegin(JSContext *cx, const char *filename, int lineno);
    static bool compileScriptEnd(JSContext *cx, JSScript *script, const char *filename, int lineno);

    static bool calloutBegin(JSContext *cx, JSFunction *fun);
    static bool calloutEnd(JSContext *cx, JSFunction *fun);

    static bool acquireMemory(JSContext *cx, void *address, size_t nbytes);
    static bool releaseMemory(JSContext *cx, void *address, size_t nbytes);

    static bool GCStart(JSCompartment *compartment);
    static bool GCEnd(JSCompartment *compartment);
    static bool GCStartMarkPhase(JSCompartment *compartment);

    static bool GCEndMarkPhase(JSCompartment *compartment);
    static bool GCStartSweepPhase(JSCompartment *compartment);
    static bool GCEndSweepPhase(JSCompartment *compartment);

    static bool CustomMark(JSString *string);
    static bool CustomMark(const char *string);
    static bool CustomMark(int marker);

    static bool startProfiling();
    static void stopProfiling();

#ifdef MOZ_ETW
    
    static bool ETWCreateRuntime(JSRuntime *rt);
    static bool ETWDestroyRuntime(JSRuntime *rt);
    static bool ETWShutdown();
    static bool ETWCallTrackingActive(JSContext *cx);
    static bool ETWEnterJSFun(JSContext *cx, JSFunction *fun, JSScript *script, int counter);
    static bool ETWExitJSFun(JSContext *cx, JSFunction *fun, JSScript *script, int counter);
    static bool ETWCreateObject(JSContext *cx, JSObject *obj);
    static bool ETWFinalizeObject(JSObject *obj);
    static bool ETWResizeObject(JSContext *cx, JSObject *obj, size_t oldSize, size_t newSize);
    static bool ETWCreateString(JSContext *cx, JSString *string, size_t length);
    static bool ETWFinalizeString(JSString *string);
    static bool ETWCompileScriptBegin(const char *filename, int lineno);
    static bool ETWCompileScriptEnd(const char *filename, int lineno);
    static bool ETWCalloutBegin(JSContext *cx, JSFunction *fun);
    static bool ETWCalloutEnd(JSContext *cx, JSFunction *fun);
    static bool ETWAcquireMemory(JSContext *cx, void *address, size_t nbytes);
    static bool ETWReleaseMemory(JSContext *cx, void *address, size_t nbytes);
    static bool ETWGCStart(JSCompartment *compartment);
    static bool ETWGCEnd(JSCompartment *compartment);
    static bool ETWGCStartMarkPhase(JSCompartment *compartment);
    static bool ETWGCEndMarkPhase(JSCompartment *compartment);
    static bool ETWGCStartSweepPhase(JSCompartment *compartment);
    static bool ETWGCEndSweepPhase(JSCompartment *compartment);
    static bool ETWCustomMark(JSString *string);
    static bool ETWCustomMark(const char *string);
    static bool ETWCustomMark(int marker);
    static bool ETWStartExecution(JSContext *cx, JSScript *script);
    static bool ETWStopExecution(JSContext *cx, JSScript *script);
    static bool ETWResizeHeap(JSCompartment *compartment, size_t oldSize, size_t newSize);
#endif
};

inline bool
Probes::createRuntime(JSRuntime *rt)
{
    bool ok = true;
#ifdef MOZ_ETW
    if (!ETWCreateRuntime(rt))
        ok = false;
#endif
    return ok;
}

inline bool
Probes::destroyRuntime(JSRuntime *rt)
{
    bool ok = true;
#ifdef MOZ_ETW
    if (!ETWDestroyRuntime(rt))
        ok = false;
#endif
    return ok;
}

inline bool
Probes::shutdown()
{
    bool ok = true;
#ifdef MOZ_ETW
    if (!ETWShutdown())
        ok = false;
#endif
    return ok;
}

inline bool
Probes::callTrackingActive(JSContext *cx)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_FUNCTION_ENTRY_ENABLED() || JAVASCRIPT_FUNCTION_RETURN_ENABLED())
        return true;
#endif
#ifdef MOZ_TRACE_JSCALLS
    if (cx->functionCallback)
        return true;
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && ETWCallTrackingActive(cx))
        return true;
#endif
    return false;
}

extern inline JS_FRIEND_API(JSBool)
js_PauseProfilers(JSContext *cx, uintN argc, jsval *vp)
{
    Probes::pauseProfilers(cx);
    return JS_TRUE;
}

extern inline JS_FRIEND_API(JSBool)
js_ResumeProfilers(JSContext *cx, uintN argc, jsval *vp)
{
    Probes::resumeProfilers(cx);
    return JS_TRUE;
}

extern JS_FRIEND_API(JSBool)
js_ResumeProfilers(JSContext *cx, uintN argc, jsval *vp);

inline bool
Probes::enterJSFun(JSContext *cx, JSFunction *fun, JSScript *script, int counter)
{
    bool ok = true;
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_FUNCTION_ENTRY_ENABLED())
        enterJSFunImpl(cx, fun, script);
#endif
#ifdef MOZ_TRACE_JSCALLS
    cx->doFunctionCallback(fun, script, counter);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWEnterJSFun(cx, fun, script, counter))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::exitJSFun(JSContext *cx, JSFunction *fun, JSScript *script, int counter)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_FUNCTION_RETURN_ENABLED())
        handleFunctionReturn(cx, fun, script);
#endif
#ifdef MOZ_TRACE_JSCALLS
    if (counter > 0)
        counter = -counter;
    cx->doFunctionCallback(fun, script, counter);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWExitJSFun(cx, fun, script, counter))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::resizeHeap(JSCompartment *compartment, size_t oldSize, size_t newSize)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWResizeHeap(compartment, oldSize, newSize))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::createObject(JSContext *cx, JSObject *obj)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_CREATE_ENABLED())
        JAVASCRIPT_OBJECT_CREATE(ObjectClassname(obj), (uintptr_t)obj);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCreateObject(cx, obj))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::finalizeObject(JSObject *obj)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_FINALIZE_ENABLED()) {
        Class *clasp = obj->getClass();

        
        JAVASCRIPT_OBJECT_FINALIZE(NULL, (char *)clasp->name, (uintptr_t)obj);
    }
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWFinalizeObject(obj))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::resizeObject(JSContext *cx, JSObject *obj, size_t oldSize, size_t newSize)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWResizeObject(cx, obj, oldSize, newSize))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::createString(JSContext *cx, JSString *string, size_t length)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCreateString(cx, string, length))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::finalizeString(JSString *string)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWFinalizeString(string))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::compileScriptBegin(JSContext *cx, const char *filename, int lineno)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCompileScriptBegin(filename, lineno))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::compileScriptEnd(JSContext *cx, JSScript *script, const char *filename, int lineno)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCompileScriptEnd(filename, lineno))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::calloutBegin(JSContext *cx, JSFunction *fun)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCalloutBegin(cx, fun))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::calloutEnd(JSContext *cx, JSFunction *fun)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCalloutEnd(cx, fun))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::acquireMemory(JSContext *cx, void *address, size_t nbytes)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWAcquireMemory(cx, address, nbytes))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::releaseMemory(JSContext *cx, void *address, size_t nbytes)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWReleaseMemory(cx, address, nbytes))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCStart(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCStart(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCEnd(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCEnd(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCStartMarkPhase(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCStartMarkPhase(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCEndMarkPhase(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCEndMarkPhase(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCStartSweepPhase(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCStartSweepPhase(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::GCEndSweepPhase(JSCompartment *compartment)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWGCEndSweepPhase(compartment))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::CustomMark(JSString *string)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCustomMark(string))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::CustomMark(const char *string)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCustomMark(string))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::CustomMark(int marker)
{
    bool ok = true;

#ifdef MOZ_ETW
    if (ProfilingActive && !ETWCustomMark(marker))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::startExecution(JSContext *cx, JSScript *script)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_START_ENABLED())
        JAVASCRIPT_EXECUTE_START((script->filename ? (char *)script->filename : nullName),
                                 script->lineno);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWStartExecution(cx, script))
        ok = false;
#endif

    return ok;
}

inline bool
Probes::stopExecution(JSContext *cx, JSScript *script)
{
    bool ok = true;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_DONE_ENABLED())
        JAVASCRIPT_EXECUTE_DONE((script->filename ? (char *)script->filename : nullName),
                                script->lineno);
#endif
#ifdef MOZ_ETW
    if (ProfilingActive && !ETWStopExecution(cx, script))
        ok = false;
#endif

    return ok;
}

struct AutoFunctionCallProbe {
    JSContext * const cx;
    JSFunction *fun;
    JSScript *script;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    AutoFunctionCallProbe(JSContext *cx, JSFunction *fun, JSScript *script
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), fun(fun), script(script)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        Probes::enterJSFun(cx, fun, script);
    }

    ~AutoFunctionCallProbe() {
        Probes::exitJSFun(cx, fun, script);
    }
};

} 
    
#endif 
