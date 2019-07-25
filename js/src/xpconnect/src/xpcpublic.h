






































#ifndef xpcpublic_h
#define xpcpublic_h

#include "jsapi.h"
#include "jsobj.h"
#include "jsgc.h"
#include "jspubtd.h"

#include "nsISupports.h"
#include "nsIPrincipal.h"
#include "nsWrapperCache.h"
#include "nsStringGlue.h"

class nsIPrincipal;

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


NS_EXPORT_(void)
xpc_LocalizeContext(JSContext *cx);

nsresult
xpc_MorphSlimWrapper(JSContext *cx, nsISupports *tomorph);

extern JSBool
XPC_WN_Equality(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);

#define IS_WRAPPER_CLASS(clazz)                                               \
    (clazz->ext.equality == js::Valueify(XPC_WN_Equality))

inline JSBool
DebugCheckWrapperClass(JSObject* obj)
{
    NS_ASSERTION(IS_WRAPPER_CLASS(obj->getClass()),
                 "Forgot to check if this is a wrapper?");
    return JS_TRUE;
}








#define IS_WN_WRAPPER_OBJECT(obj)                                             \
    (DebugCheckWrapperClass(obj) && obj->getSlot(0).isUndefined())
#define IS_SLIM_WRAPPER_OBJECT(obj)                                           \
    (DebugCheckWrapperClass(obj) && !obj->getSlot(0).isUndefined())




#define IS_WN_WRAPPER(obj)                                                    \
    (IS_WRAPPER_CLASS(obj->getClass()) && IS_WN_WRAPPER_OBJECT(obj))
#define IS_SLIM_WRAPPER(obj)                                                  \
    (IS_WRAPPER_CLASS(obj->getClass()) && IS_SLIM_WRAPPER_OBJECT(obj))

inline JSObject *
xpc_GetGlobalForObject(JSObject *obj)
{
    while(JSObject *parent = obj->getParent())
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
                     !IS_SLIM_WRAPPER_OBJECT(wrapper),
                     "Should never have a slim wrapper when IsProxy()");
        if (wrapper &&
            wrapper->compartment() == scope->getCompartment() &&
            (IS_SLIM_WRAPPER_OBJECT(wrapper) ||
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

inline JSObject*
nsWrapperCache::GetWrapper() const
{
  JSObject* obj = GetWrapperPreserveColor();
  xpc_UnmarkGrayObject(obj);
  return obj;
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

    PRInt64 gcHeapObjects;
    PRInt64 gcHeapStrings;
    PRInt64 gcHeapShapes;
    PRInt64 gcHeapXml;

    PRInt64 objectSlots;
    PRInt64 stringChars;
    PRInt64 propertyTables;

    PRInt64 scripts;
#ifdef JS_METHODJIT
    PRInt64 mjitCode;
    PRInt64 mjitData;
#endif
#ifdef JS_TRACER
    PRInt64 tjitCode;
    PRInt64 tjitDataAllocatorsMain;
    PRInt64 tjitDataAllocatorsReserve;
#endif
};

struct IterateData
{
    IterateData()
    : compartmentStatsVector(), currCompartmentStats(NULL) { }

    js::Vector<CompartmentStats, 0, js::SystemAllocPolicy> compartmentStatsVector;
    CompartmentStats *currCompartmentStats;
};

JSBool
CollectCompartmentStatsForRuntime(JSRuntime *rt, IterateData *data);

void
ReportCompartmentStats(const CompartmentStats &stats,
                       const nsACString &pathPrefix,
                       nsIMemoryMultiReporterCallback *callback,
                       nsISupports *closure);

void
ReportJSStackSizeForRuntime(JSRuntime *rt, const nsACString &pathPrefix,
                            nsIMemoryMultiReporterCallback *callback,
                            nsISupports *closure);

} 
} 
} 

#endif
