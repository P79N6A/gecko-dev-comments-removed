






#ifndef xpcpublic_h
#define xpcpublic_h

#include "jsapi.h"
#include "js/MemoryMetrics.h"
#include "jsclass.h"
#include "jsfriendapi.h"
#include "jspubtd.h"
#include "jsproxy.h"
#include "js/HeapAPI.h"
#include "js/GCAPI.h"

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
#define BAD_TLS_INDEX ((uint32_t) -1)
#endif

namespace xpc {
JSObject *
TransplantObject(JSContext *cx, JSObject *origobj, JSObject *target);

JSObject *
TransplantObjectWithWrapper(JSContext *cx,
                            JSObject *origobj, JSObject *origwrapper,
                            JSObject *targetobj, JSObject *targetwrapper);
} 

#define XPCONNECT_GLOBAL_FLAGS                                                \
    JSCLASS_DOM_GLOBAL | JSCLASS_HAS_PRIVATE |                                \
    JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_IMPLEMENTS_BARRIERS |            \
    JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(2)

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













#define WRAPPER_MULTISLOT 0

static inline bool IS_WN_WRAPPER_OBJECT(JSObject *obj)
{
    MOZ_ASSERT(IS_WRAPPER_CLASS(js::GetObjectClass(obj)));
    return !js::GetReservedSlot(obj, WRAPPER_MULTISLOT).isDouble();
}
static inline bool IS_SLIM_WRAPPER_OBJECT(JSObject *obj)
{
    return !IS_WN_WRAPPER_OBJECT(obj);
}




static inline bool IS_WN_WRAPPER(JSObject *obj)
{
    return IS_WRAPPER_CLASS(js::GetObjectClass(obj)) && IS_WN_WRAPPER_OBJECT(obj);
}
static inline bool IS_SLIM_WRAPPER(JSObject *obj)
{
    return IS_WRAPPER_CLASS(js::GetObjectClass(obj)) && IS_SLIM_WRAPPER_OBJECT(obj);
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
            (cache->IsDOMBinding() ? !cache->HasSystemOnlyWrapper() :
             (IS_SLIM_WRAPPER(wrapper) || xpc_OkToHandOutWrapper(cache)))) {
            *vp = OBJECT_TO_JSVAL(wrapper);
            return wrapper;
        }
    }

    return nullptr;
}




inline JSBool
xpc_IsGrayGCThing(void *thing)
{
    return JS::GCThingIsMarkedGray(thing);
}



extern JSBool
xpc_GCThingIsGrayCCThing(void *thing);


MOZ_ALWAYS_INLINE void
xpc_UnmarkNonNullGrayObject(JSObject *obj)
{
    JS::ExposeGCThingToActiveJS(obj, JSTRACE_OBJECT);
}



MOZ_ALWAYS_INLINE JSObject *
xpc_UnmarkGrayObject(JSObject *obj)
{
    if (obj)
        xpc_UnmarkNonNullGrayObject(obj);
    return obj;
}

inline JSScript *
xpc_UnmarkGrayScript(JSScript *script)
{
    if (script)
        JS::ExposeGCThingToActiveJS(script, JSTRACE_SCRIPT);

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
xpc_MarkInCCGeneration(nsISupports* aVariant, uint32_t aGeneration);


extern void
xpc_TryUnmarkWrappedGrayObject(nsISupports* aWrappedJS);

extern void
xpc_UnmarkSkippableJSHolders();



NS_EXPORT_(void)
xpc_ActivateDebugMode();

class nsIMemoryMultiReporterCallback;

namespace xpc {

bool DeferredRelease(nsISupports *obj);


NS_EXPORT_(bool) Base64Encode(JSContext *cx, JS::Value val, JS::Value *out);
NS_EXPORT_(bool) Base64Decode(JSContext *cx, JS::Value val, JS::Value *out);






bool NonVoidStringToJsval(JSContext *cx, nsAString &str, JS::Value *rval);
inline bool StringToJsval(JSContext *cx, nsAString &str, JS::Value *rval)
{
    
    if (str.IsVoid()) {
        *rval = JSVAL_NULL;
        return true;
    }
    return NonVoidStringToJsval(cx, str, rval);
}

nsIPrincipal *GetCompartmentPrincipal(JSCompartment *compartment);

void DumpJSHeap(FILE* file);

void SetLocationForGlobal(JSObject *global, const nsACString& location);
void SetLocationForGlobal(JSObject *global, nsIURI *locationURI);


















bool
DOM_DefineQuickStubs(JSContext *cx, JSObject *proto, uint32_t flags,
                     uint32_t interfaceCount, const nsIID **interfaceArray);



nsresult
ReportJSRuntimeExplicitTreeStats(const JS::RuntimeStats &rtStats,
                                 const nsACString &rtPath,
                                 nsIMemoryMultiReporterCallback *cb,
                                 nsISupports *closure, size_t *rtTotal = NULL);











JSObject *
Unwrap(JSContext *cx, JSObject *wrapper, bool stopAtOuter = true);




bool
Throw(JSContext *cx, nsresult rv);

} 

nsCycleCollectionParticipant *
xpc_JSCompartmentParticipant();

namespace mozilla {
namespace dom {

extern int HandlerFamily;
inline void* ProxyFamily() { return &HandlerFamily; }

inline bool IsDOMProxy(JSObject *obj, const js::Class* clasp)
{
    MOZ_ASSERT(js::GetObjectClass(obj) == clasp);
    return (js::IsObjectProxyClass(clasp) || js::IsFunctionProxyClass(clasp)) &&
           js::GetProxyHandler(obj)->family() == ProxyFamily();
}

inline bool IsDOMProxy(JSObject *obj)
{
    return IsDOMProxy(obj, js::GetObjectClass(obj));
}

typedef JSObject*
(*DefineInterface)(JSContext *cx, JSObject *global, bool *enabled);

typedef bool
(*PrefEnabled)();

extern bool
DefineStaticJSVals(JSContext *cx);
void
Register(nsScriptNameSpaceManager* aNameSpaceManager);

} 
} 

#endif
