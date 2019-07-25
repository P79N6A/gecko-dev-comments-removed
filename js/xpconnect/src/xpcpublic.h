






































#ifndef xpcpublic_h
#define xpcpublic_h

#include "jsapi.h"
#include "js/MemoryMetrics.h"
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
class nsIXPConnectWrappedJS;
struct nsDOMClassInfoData;

#ifndef BAD_TLS_INDEX
#define BAD_TLS_INDEX ((PRUint32) -1)
#endif

nsresult
xpc_CreateGlobalObject(JSContext *cx, JSClass *clasp,
                       nsIPrincipal *principal, nsISupports *ptr,
                       bool wantXrays, JSObject **global,
                       JSCompartment **compartment);

nsresult
xpc_CreateMTGlobalObject(JSContext *cx, JSClass *clasp,
                         nsISupports *ptr, JSObject **global,
                         JSCompartment **compartment);

#define XPCONNECT_GLOBAL_FLAGS                                                \
    JSCLASS_XPCONNECT_GLOBAL | JSCLASS_HAS_PRIVATE |                          \
    JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_IMPLEMENTS_BARRIERS |            \
    JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(1)

void
TraceXPCGlobal(JSTracer *trc, JSObject *obj);


NS_EXPORT_(void)
xpc_LocalizeContext(JSContext *cx);

nsresult
xpc_MorphSlimWrapper(JSContext *cx, nsISupports *tomorph);

static inline bool IS_WRAPPER_CLASS(js::Class* clazz)
{
    return clazz->ext.isWrappedNative;
}

inline JSBool
DebugCheckWrapperClass(JSObject* obj)
{
    NS_ASSERTION(IS_WRAPPER_CLASS(js::GetObjectClass(obj)),
                 "Forgot to check if this is a wrapper?");
    return true;
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
    while (JSObject *parent = js::GetObjectParent(obj))
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
    return js::GCThingIsMarkedGray(thing);
}



extern JSBool
xpc_GCThingIsGrayCCThing(void *thing);


extern void
xpc_UnmarkGrayObjectRecursive(JSObject* obj);



inline void
xpc_UnmarkGrayObject(JSObject *obj)
{
    if (obj) {
        if (xpc_IsGrayGCThing(obj))
            xpc_UnmarkGrayObjectRecursive(obj);
        else if (js::IsIncrementalBarrierNeededOnObject(obj))
            js::IncrementalReferenceBarrier(obj);
    }
}



extern void
xpc_MarkInCCGeneration(nsISupports* aVariant, PRUint32 aGeneration);


extern void
xpc_UnmarkGrayObject(nsIXPConnectWrappedJS* aWrappedJS);

extern void
xpc_UnmarkSkippableJSHolders();



NS_EXPORT_(void)
xpc_ActivateDebugMode();

namespace xpc {


bool Base64Encode(JSContext *cx, JS::Value val, JS::Value *out);
bool Base64Decode(JSContext *cx, JS::Value val, JS::Value *out);






bool StringToJsval(JSContext *cx, nsAString &str, JS::Value *rval);
bool NonVoidStringToJsval(JSContext *cx, nsAString &str, JS::Value *rval);

void *GetCompartmentName(JSRuntime *rt, JSCompartment *c);
void DestroyCompartmentName(void *string);

#ifdef DEBUG
void DumpJSHeap(FILE* file);
#endif
} 

class nsIMemoryMultiReporterCallback;

namespace mozilla {
namespace xpconnect {
namespace memory {



void
ReportJSRuntimeExplicitTreeStats(const JS::RuntimeStats &rtStats, const nsACString &pathPrefix,
                                 nsIMemoryMultiReporterCallback *callback,
                                 nsISupports *closure);

} 
} 

namespace dom {
namespace binding {

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
} 

#endif
