






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
#include "mozilla/dom/DOMJSClass.h"
#include "nsMathUtils.h"

class nsIPrincipal;
class nsIXPConnectWrappedJS;
class nsScriptNameSpaceManager;

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
    JSCLASS_DOM_GLOBAL | JSCLASS_XPCONNECT_GLOBAL | JSCLASS_HAS_PRIVATE |     \
    JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_IMPLEMENTS_BARRIERS |            \
    JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(3)

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













#define WRAPPER_MULTISLOT 0


#define IS_WN_WRAPPER_OBJECT(obj)                                             \
    (DebugCheckWrapperClass(obj) && !js::GetReservedSlot(obj, WRAPPER_MULTISLOT).isDouble())
#define IS_SLIM_WRAPPER_OBJECT(obj)                                           \
    (DebugCheckWrapperClass(obj) && js::GetReservedSlot(obj, WRAPPER_MULTISLOT).isDouble())




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
                     !cache->IsDOMBinding() ||
                     !IS_SLIM_WRAPPER(wrapper),
                     "Should never have a slim wrapper when IsDOMBinding()");
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




inline JSBool
xpc_IsGrayGCThing(void *thing)
{
    return js::GCThingIsMarkedGray(thing);
}



extern JSBool
xpc_GCThingIsGrayCCThing(void *thing);


extern void
xpc_UnmarkGrayGCThingRecursive(void *thing, JSGCTraceKind kind);



inline JSObject *
xpc_UnmarkGrayObject(JSObject *obj)
{
    if (obj) {
        if (xpc_IsGrayGCThing(obj))
            xpc_UnmarkGrayGCThingRecursive(obj, JSTRACE_OBJECT);
        else if (js::IsIncrementalBarrierNeededOnObject(obj))
            js::IncrementalReferenceBarrier(obj);
    }
    return obj;
}

inline JSScript *
xpc_UnmarkGrayScript(JSScript *script)
{
    if (script) {
        if (xpc_IsGrayGCThing(script))
            xpc_UnmarkGrayGCThingRecursive(script, JSTRACE_SCRIPT);
        else if (js::IsIncrementalBarrierNeededOnScript(script))
            js::IncrementalReferenceBarrier(script);
    }
    return script;
}

inline JSContext *
xpc_UnmarkGrayContext(JSContext *cx)
{
    if (cx) {
        JSObject *global = JS_GetGlobalObject(cx);
        xpc_UnmarkGrayObject(global);
        if (global && JS_IsInRequest(JS_GetRuntime(cx))) {
            JSObject *scope = JS_GetGlobalForScopeChain(cx);
            if (scope != global)
                xpc_UnmarkGrayObject(scope);
        }
    }
    return cx;
}

#ifdef __cplusplus
class XPCAutoRequest : public JSAutoRequest {
public:
    XPCAutoRequest(JSContext *cx) : JSAutoRequest(cx) {
        xpc_UnmarkGrayContext(cx);
    }
};
#endif



extern void
xpc_MarkInCCGeneration(nsISupports* aVariant, PRUint32 aGeneration);


extern void
xpc_TryUnmarkWrappedGrayObject(nsISupports* aWrappedJS);

extern void
xpc_UnmarkSkippableJSHolders();



NS_EXPORT_(void)
xpc_ActivateDebugMode();

class nsIMemoryMultiReporterCallback;

namespace xpc {

bool DeferredRelease(nsISupports *obj);


bool Base64Encode(JSContext *cx, JS::Value val, JS::Value *out);
bool Base64Decode(JSContext *cx, JS::Value val, JS::Value *out);






bool StringToJsval(JSContext *cx, nsAString &str, JS::Value *rval);
bool NonVoidStringToJsval(JSContext *cx, nsAString &str, JS::Value *rval);

nsIPrincipal *GetCompartmentPrincipal(JSCompartment *compartment);

void DumpJSHeap(FILE* file);

void SetLocationForGlobal(JSObject *global, const nsACString& location);
void SetLocationForGlobal(JSObject *global, nsIURI *locationURI);


















bool
DOM_DefineQuickStubs(JSContext *cx, JSObject *proto, PRUint32 flags,
                     PRUint32 interfaceCount, const nsIID **interfaceArray);



nsresult
ReportJSRuntimeExplicitTreeStats(const JS::RuntimeStats &rtStats,
                                 const nsACString &rtPath,
                                 nsIMemoryMultiReporterCallback *cb,
                                 nsISupports *closure, size_t *rtTotal = NULL);




inline bool
ValueToInt64(JSContext *cx, JS::Value v, int64_t *result)
{
    if (JSVAL_IS_INT(v)) {
        int32_t intval;
        if (!JS_ValueToECMAInt32(cx, v, &intval))
            return false;
        *result = static_cast<int64_t>(intval);
    } else {
        double doubleval;
        if (!JS_ValueToNumber(cx, v, &doubleval))
            return false;
        
        if (NS_finite(doubleval))
            
            *result = static_cast<int64_t>(doubleval);
        else
            *result = 0;
    }
    return true;
}




inline bool
ValueToUint64(JSContext *cx, JS::Value v, uint64_t *result)
{
    if (JSVAL_IS_INT(v)) {
        uint32_t intval;
        if (!JS_ValueToECMAUint32(cx, v, &intval))
            return false;
        *result = static_cast<uint64_t>(intval);
    } else {
        double doubleval;
        if (!JS_ValueToNumber(cx, v, &doubleval))
            return false;
        
        if (NS_finite(doubleval))
            
            *result = static_cast<uint64_t>(doubleval);
        else
            *result = 0;
    }
    return true;
}











JSObject *
Unwrap(JSContext *cx, JSObject *wrapper, bool stopAtOuter = true);




bool
Throw(JSContext *cx, nsresult rv);

} 

nsCycleCollectionParticipant *
xpc_JSCompartmentParticipant();

namespace mozilla {
namespace dom {
namespace binding {

extern int HandlerFamily;
inline void* ProxyFamily() { return &HandlerFamily; }
inline bool instanceIsProxy(JSObject *obj)
{
    return js::IsProxy(obj) &&
           js::GetProxyHandler(obj)->family() == ProxyFamily();
}

typedef bool
(*DefineInterface)(JSContext *cx, JSObject *global, bool *enabled);

extern bool
DefineStaticJSVals(JSContext *cx);
void
Register(nsScriptNameSpaceManager* aNameSpaceManager);
extern bool
DefineConstructor(JSContext *cx, JSObject *obj, DefineInterface aDefine,
                  nsresult *aResult);

} 
} 
} 

#endif
