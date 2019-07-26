





#ifndef js_Tracer_h
#define js_Tracer_h

#include "mozilla/NullPtr.h"
 
#include "jspubtd.h"

struct JSTracer;

namespace JS {
template <typename T> class Heap;
template <typename T> class TenuredHeap;
}















typedef void
(* JSTraceCallback)(JSTracer *trc, void **thingp, JSGCTraceKind kind);



typedef void
(* JSTraceNamePrinter)(JSTracer *trc, char *buf, size_t bufsize);

enum WeakMapTraceKind {
    DoNotTraceWeakMaps = 0,
    TraceWeakMapValues = 1,
    TraceWeakMapKeysValues = 2
};

struct JSTracer {
    JSRuntime           *runtime;
    JSTraceCallback     callback;
    JSTraceNamePrinter  debugPrinter;
    const void          *debugPrintArg;
    size_t              debugPrintIndex;
    WeakMapTraceKind    eagerlyTraceWeakMaps;
#ifdef JS_GC_ZEAL
    void                *realLocation;
#endif
};
















# define JS_SET_TRACING_DETAILS(trc, printer, arg, index)                     \
    JS_BEGIN_MACRO                                                            \
        (trc)->debugPrinter = (printer);                                      \
        (trc)->debugPrintArg = (arg);                                         \
        (trc)->debugPrintIndex = (index);                                     \
    JS_END_MACRO








#ifdef JS_GC_ZEAL
# define JS_SET_TRACING_LOCATION(trc, location)                               \
    JS_BEGIN_MACRO                                                            \
        if (!(trc)->realLocation || !(location))                              \
            (trc)->realLocation = (location);                                 \
    JS_END_MACRO
# define JS_UNSET_TRACING_LOCATION(trc)                                       \
    JS_BEGIN_MACRO                                                            \
        (trc)->realLocation = nullptr;                                        \
    JS_END_MACRO
#else
# define JS_SET_TRACING_LOCATION(trc, location)                               \
    JS_BEGIN_MACRO                                                            \
    JS_END_MACRO
# define JS_UNSET_TRACING_LOCATION(trc)                                       \
    JS_BEGIN_MACRO                                                            \
    JS_END_MACRO
#endif



# define JS_SET_TRACING_INDEX(trc, name, index)                               \
    JS_SET_TRACING_DETAILS(trc, nullptr, name, index)


# define JS_SET_TRACING_NAME(trc, name)                                       \
    JS_SET_TRACING_DETAILS(trc, nullptr, name, (size_t)-1)













extern JS_PUBLIC_API(void)
JS_CallValueTracer(JSTracer *trc, JS::Value *valuep, const char *name);

extern JS_PUBLIC_API(void)
JS_CallIdTracer(JSTracer *trc, jsid *idp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallObjectTracer(JSTracer *trc, JSObject **objp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallStringTracer(JSTracer *trc, JSString **strp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallScriptTracer(JSTracer *trc, JSScript **scriptp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallHeapValueTracer(JSTracer *trc, JS::Heap<JS::Value> *valuep, const char *name);

extern JS_PUBLIC_API(void)
JS_CallHeapIdTracer(JSTracer *trc, JS::Heap<jsid> *idp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallHeapObjectTracer(JSTracer *trc, JS::Heap<JSObject *> *objp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallHeapStringTracer(JSTracer *trc, JS::Heap<JSString *> *strp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallHeapScriptTracer(JSTracer *trc, JS::Heap<JSScript *> *scriptp, const char *name);

template <typename HashSetEnum>
inline void
JS_CallHashSetObjectTracer(JSTracer *trc, HashSetEnum &e, JSObject *const &key, const char *name)
{
    JSObject *updated = key;
    JS_SET_TRACING_LOCATION(trc, reinterpret_cast<void *>(&const_cast<JSObject *&>(key)));
    JS_CallObjectTracer(trc, &updated, name);
    if (updated != key)
        e.rekeyFront(key, updated);
}



extern JS_PUBLIC_API(void)
JS_CallTenuredObjectTracer(JSTracer *trc, JS::TenuredHeap<JSObject *> *objp, const char *name);


extern JS_PUBLIC_API(void)
JS_TracerInit(JSTracer *trc, JSRuntime *rt, JSTraceCallback callback);

extern JS_PUBLIC_API(void)
JS_TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind);

extern JS_PUBLIC_API(void)
JS_TraceRuntime(JSTracer *trc);

extern JS_PUBLIC_API(void)
JS_GetTraceThingInfo(char *buf, size_t bufsize, JSTracer *trc,
                     void *thing, JSGCTraceKind kind, bool includeDetails);

extern JS_PUBLIC_API(const char *)
JS_GetTraceEdgeName(JSTracer *trc, char *buffer, int bufferSize);

#endif 
