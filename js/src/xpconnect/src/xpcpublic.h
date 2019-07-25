






































#ifndef xpcpublic_h
#define xpcpublic_h

#include "jsapi.h"
#include "jsclass.h"
#include "jsfriendapi.h"
#include "jsgc.h"
#include "jspubtd.h"
#include "jsproxy.h"

#include "nsISupports.h"
#include "nsIPrincipal.h"
#include "nsWrapperCache.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

class nsIPrincipal;
struct nsDOMClassInfoData;

static const uint32 XPC_GC_COLOR_BLACK = 0;
static const uint32 XPC_GC_COLOR_GRAY = 1;

nsresult
xpc_CreateGlobalObject(JSContext *cx, JSClass *clasp,
                       nsIPrincipal *principal, nsISupports *ptr,
                       bool wantXrays, JSObject **global,
                       JSCompartment **compartment);

nsresult
xpc_CreateMTGlobalObject(JSContext *cx, JSClass *clasp,
                         nsISupports *ptr, JSObject **global,
                         JSCompartment **compartment);

#define XPCONNECT_GLOBAL_FLAGS \
    JSCLASS_XPCONNECT_GLOBAL | JSCLASS_HAS_PRIVATE | \
    JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(1)

void
TraceXPCGlobal(JSTracer *trc, JSObject *obj);


NS_EXPORT_(void)
xpc_LocalizeContext(JSContext *cx);

nsresult
xpc_MorphSlimWrapper(JSContext *cx, nsISupports *tomorph);

#define IS_WRAPPER_CLASS(clazz)                                               \
    ((clazz)->ext.isWrappedNative)

inline JSBool
DebugCheckWrapperClass(JSObject* obj)
{
    NS_ASSERTION(IS_WRAPPER_CLASS(js::GetObjectClass(obj)),
                 "Forgot to check if this is a wrapper?");
    return JS_TRUE;
}








#define IS_WN_WRAPPER_OBJECT(obj)                                             \
    (DebugCheckWrapperClass(obj) && js::GetReservedSlot(obj, 0).isUndefined())
#define IS_SLIM_WRAPPER_OBJECT(obj)                                           \
    (DebugCheckWrapperClass(obj) && !js::GetReservedSlot(obj, 0).isUndefined())




#define IS_WN_WRAPPER(obj)                                                    \
    (IS_WRAPPER_CLASS(js::GetObjectClass(obj)) && IS_WN_WRAPPER_OBJECT(obj))
#define IS_SLIM_WRAPPER(obj)                                                  \
    (IS_WRAPPER_CLASS(js::GetObjectClass(obj)) && IS_SLIM_WRAPPER_OBJECT(obj))

inline JSObject *
xpc_GetGlobalForObject(JSObject *obj)
{
    while(JSObject *parent = js::GetObjectParent(obj))
        obj = parent;
    return obj;
}

extern bool
xpc_OkToHandOutWrapper(nsWrapperCache *cache);

inline JSObject*
xpc_FastGetCachedWrapper(nsWrapperCache *cache, JSObject *scope, jsval *vp)
{
    if (cache) {
        JSObject* wrapper = cache->GetWrapper();
        NS_ASSERTION(!wrapper ||
                     !cache->IsProxy() ||
                     !IS_SLIM_WRAPPER(wrapper),
                     "Should never have a slim wrapper when IsProxy()");
        if (wrapper &&
            js::GetObjectCompartment(wrapper) == js::GetObjectCompartment(scope) &&
            (IS_SLIM_WRAPPER(wrapper) ||
             xpc_OkToHandOutWrapper(cache))) {
            *vp = OBJECT_TO_JSVAL(wrapper);
            return wrapper;
        }
    }

    return nsnull;
}

inline JSObject*
xpc_FastGetCachedWrapper(nsWrapperCache *cache, JSObject *scope)
{
    jsval dummy;
    return xpc_FastGetCachedWrapper(cache, scope, &dummy);
}






inline JSBool
xpc_IsGrayGCThing(void *thing)
{
    return js_GCThingIsMarked(thing, XPC_GC_COLOR_GRAY);
}





extern JSBool
xpc_GCThingIsGrayCCThing(void *thing);


extern void
xpc_UnmarkGrayObjectRecursive(JSObject* obj);



inline void
xpc_UnmarkGrayObject(JSObject *obj)
{
    if(obj && xpc_IsGrayGCThing(obj))
        xpc_UnmarkGrayObjectRecursive(obj);
}

class nsIMemoryMultiReporterCallback;

namespace mozilla {
namespace xpconnect {
namespace memory {

struct CompartmentStats
{
    CompartmentStats(JSContext *cx, JSCompartment *c);

    nsCString name;
    PRInt64 gcHeapArenaHeaders;
    PRInt64 gcHeapArenaPadding;
    PRInt64 gcHeapArenaUnused;

    PRInt64 gcHeapKinds[JSTRACE_LAST + 1];

    PRInt64 objectSlots;
    PRInt64 stringChars;
    PRInt64 propertyTables;
    PRInt64 shapeKids;
    PRInt64 scriptData;

#ifdef JS_METHODJIT
    PRInt64 mjitCodeMethod;
    PRInt64 mjitCodeRegexp;
    PRInt64 mjitCodeUnused;
    PRInt64 mjitData;
#endif
#ifdef JS_TRACER
    PRInt64 tjitCode;
    PRInt64 tjitDataAllocatorsMain;
    PRInt64 tjitDataAllocatorsReserve;
    PRInt64 tjitDataNonAllocators;
#endif
    TypeInferenceMemoryStats typeInferenceMemory;
};

struct IterateData
{
    IterateData()
      : runtimeObjectSize(0),
        atomsTableSize(0),
        stackSize(0),
        gcHeapChunkTotal(0),
        gcHeapChunkCleanUnused(0),
        gcHeapChunkDirtyUnused(0),
        gcHeapArenaUnused(0),
        gcHeapChunkAdmin(0),
        gcHeapUnusedPercentage(0),
        compartmentStatsVector(),
        currCompartmentStats(NULL) { }

    PRInt64 runtimeObjectSize;
    PRInt64 atomsTableSize;
    PRInt64 stackSize;
    PRInt64 gcHeapChunkTotal;
    PRInt64 gcHeapChunkCleanUnused;
    PRInt64 gcHeapChunkDirtyUnused;
    PRInt64 gcHeapArenaUnused;
    PRInt64 gcHeapChunkAdmin;
    PRInt64 gcHeapUnusedPercentage;

    nsTArray<CompartmentStats> compartmentStatsVector;
    CompartmentStats *currCompartmentStats;
};

JSBool
CollectCompartmentStatsForRuntime(JSRuntime *rt, IterateData *data);

void
ReportJSRuntimeStats(const IterateData &data, const nsACString &pathPrefix,
                     nsIMemoryMultiReporterCallback *callback,
                     nsISupports *closure);

} 
} 
} 

namespace xpc {
namespace dom {

extern int HandlerFamily;
inline void* ProxyFamily() { return &HandlerFamily; }
inline bool instanceIsProxy(JSObject *obj)
{
    return js::IsProxy(obj) &&
           js::GetProxyHandler(obj)->family() == ProxyFamily();
}
extern JSClass ExpandoClass;
inline bool isExpandoObject(JSObject *obj)
{
    return js::GetObjectJSClass(obj) == &ExpandoClass;
}

enum {
    JSPROXYSLOT_PROTOSHAPE = 0,
    JSPROXYSLOT_EXPANDO = 1
};

typedef JSObject*
(*DefineInterface)(JSContext *cx, XPCWrappedNativeScope *scope, bool *enabled);

extern bool
DefineStaticJSVals(JSContext *cx);
void
Register(nsDOMClassInfoData *aData);
extern bool
DefineConstructor(JSContext *cx, JSObject *obj, DefineInterface aDefine,
                  nsresult *aResult);

}
}

#endif
