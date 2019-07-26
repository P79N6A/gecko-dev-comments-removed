






#ifndef xpcpublic_h
#define xpcpublic_h

#include "jsproxy.h"
#include "js/HeapAPI.h"
#include "js/GCAPI.h"

#include "nsISupports.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"
#include "nsWrapperCache.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "mozilla/dom/JSSlots.h"
#include "nsMathUtils.h"
#include "nsStringBuffer.h"
#include "nsIGlobalObject.h"
#include "mozilla/dom/BindingDeclarations.h"

class nsIPrincipal;
class nsIXPConnectWrappedJS;
class nsScriptNameSpaceManager;
class nsIGlobalObject;

#ifndef BAD_TLS_INDEX
#define BAD_TLS_INDEX ((uint32_t) -1)
#endif

namespace xpc {
JSObject *
TransplantObject(JSContext *cx, JS::HandleObject origobj, JS::HandleObject target);

JSObject *
TransplantObjectWithWrapper(JSContext *cx,
                            JS::HandleObject origobj, JS::HandleObject origwrapper,
                            JS::HandleObject targetobj, JS::HandleObject targetwrapper);










JSObject *
GetXBLScope(JSContext *cx, JSObject *contentScope);



bool
AllowXBLScope(JSCompartment *c);

bool
IsSandboxPrototypeProxy(JSObject *obj);

bool
IsReflector(JSObject *obj);
} 

namespace JS {

struct RuntimeStats;

}

#define XPCONNECT_GLOBAL_FLAGS_WITH_EXTRA_SLOTS(n)                            \
    JSCLASS_DOM_GLOBAL | JSCLASS_HAS_PRIVATE |                                \
    JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_IMPLEMENTS_BARRIERS |            \
    JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(DOM_GLOBAL_SLOTS + n)

#define XPCONNECT_GLOBAL_EXTRA_SLOT_OFFSET (JSCLASS_GLOBAL_SLOT_COUNT + DOM_GLOBAL_SLOTS)

#define XPCONNECT_GLOBAL_FLAGS XPCONNECT_GLOBAL_FLAGS_WITH_EXTRA_SLOTS(0)

void
TraceXPCGlobal(JSTracer *trc, JSObject *obj);


NS_EXPORT_(bool)
xpc_LocalizeRuntime(JSRuntime *rt);
NS_EXPORT_(void)
xpc_DelocalizeRuntime(JSRuntime *rt);




static inline bool IS_WN_CLASS(js::Class* clazz)
{
    return clazz->ext.isWrappedNative;
}
static inline bool IS_WN_REFLECTOR(JSObject *obj)
{
    return IS_WN_CLASS(js::GetObjectClass(obj));
}

extern bool
xpc_OkToHandOutWrapper(nsWrapperCache *cache);

inline JSObject*
xpc_FastGetCachedWrapper(nsWrapperCache *cache, JSObject *scope, jsval *vp)
{
    if (cache) {
        JSObject* wrapper = cache->GetWrapper();
        if (wrapper &&
            js::GetObjectCompartment(wrapper) == js::GetObjectCompartment(scope) &&
            (cache->IsDOMBinding() ? !cache->HasSystemOnlyWrapper() :
                                     xpc_OkToHandOutWrapper(cache))) {
            *vp = OBJECT_TO_JSVAL(wrapper);
            return wrapper;
        }
    }

    return nullptr;
}




inline bool
xpc_IsGrayGCThing(void *thing)
{
    return JS::GCThingIsMarkedGray(thing);
}



extern bool
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
        JSObject *global = js::DefaultObjectForContextOrNull(cx);
        xpc_UnmarkGrayObject(global);
        if (global && JS_IsInRequest(JS_GetRuntime(cx))) {
            JSObject *scope = JS::CurrentGlobalOrNull(cx);
            if (scope != global)
                xpc_UnmarkGrayObject(scope);
        }
    }
    return cx;
}



extern void
xpc_MarkInCCGeneration(nsISupports* aVariant, uint32_t aGeneration);


extern void
xpc_TryUnmarkWrappedGrayObject(nsISupports* aWrappedJS);

extern void
xpc_UnmarkSkippableJSHolders();



NS_EXPORT_(void)
xpc_ActivateDebugMode();

class nsIMemoryMultiReporterCallback;


class XPCStringConvert
{
public:

    
    
    
    static jsval ReadableToJSVal(JSContext *cx, const nsAString &readable,
                                 nsStringBuffer** sharedBuffer);

    
    static MOZ_ALWAYS_INLINE bool
    StringBufferToJSVal(JSContext* cx, nsStringBuffer* buf, uint32_t length,
                        JS::Value* rval, bool* sharedBuffer)
    {
        if (buf == sCachedBuffer &&
            JS::GetGCThingZone(sCachedString) == js::GetContextZone(cx))
        {
            *rval = JS::StringValue(sCachedString);
            *sharedBuffer = false;
            return true;
        }

        JSString *str = JS_NewExternalString(cx,
                                             static_cast<jschar*>(buf->Data()),
                                             length, &sDOMStringFinalizer);
        if (!str) {
            return false;
        }
        *rval = JS::StringValue(str);
        sCachedString = str;
        sCachedBuffer = buf;
        *sharedBuffer = true;
        return true;
    }

    static void ClearCache();

private:
    static nsStringBuffer* sCachedBuffer;
    static JSString* sCachedString;
    static const JSStringFinalizer sDOMStringFinalizer;

    static void FinalizeDOMString(const JSStringFinalizer *fin, jschar *chars);

    XPCStringConvert();         
};

namespace xpc {


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

inline bool
NonVoidStringToJsval(JSContext* cx, const nsAString& str, JS::Value *rval)
{
    nsString mutableCopy(str);
    return NonVoidStringToJsval(cx, mutableCopy, rval);
}

inline bool
StringToJsval(JSContext* cx, const nsAString& str, JS::Value *rval)
{
    nsString mutableCopy(str);
    return StringToJsval(cx, mutableCopy, rval);
}




MOZ_ALWAYS_INLINE
bool NonVoidStringToJsval(JSContext* cx, mozilla::dom::DOMString& str,
                          JS::Value *rval)
{
    if (!str.HasStringBuffer()) {
        
        return NonVoidStringToJsval(cx, str.AsAString(), rval);
    }

    uint32_t length = str.StringBufferLength();
    if (length == 0) {
        *rval = JS_GetEmptyStringValue(cx);
        return true;
    }

    nsStringBuffer* buf = str.StringBuffer();
    bool shared;
    if (!XPCStringConvert::StringBufferToJSVal(cx, buf, length, rval,
                                               &shared)) {
        return false;
    }
    if (shared) {
        
        buf->AddRef();
    }
    return true;
}

MOZ_ALWAYS_INLINE
bool StringToJsval(JSContext* cx, mozilla::dom::DOMString& str,
                   JS::Value *rval)
{
    if (str.IsNull()) {
        *rval = JS::NullValue();
        return true;
    }
    return NonVoidStringToJsval(cx, str, rval);
}

nsIPrincipal *GetCompartmentPrincipal(JSCompartment *compartment);
nsIPrincipal *GetObjectPrincipal(JSObject *obj);

bool IsXBLScope(JSCompartment *compartment);

void SetLocationForGlobal(JSObject *global, const nsACString& location);
void SetLocationForGlobal(JSObject *global, nsIURI *locationURI);


















bool
DOM_DefineQuickStubs(JSContext *cx, JSObject *proto, uint32_t flags,
                     uint32_t interfaceCount, const nsIID **interfaceArray);




class ZoneStatsExtras {
public:
    ZoneStatsExtras()
    {}

    nsAutoCString pathPrefix;

private:
    ZoneStatsExtras(const ZoneStatsExtras &other) MOZ_DELETE;
    ZoneStatsExtras& operator=(const ZoneStatsExtras &other) MOZ_DELETE;
};



class CompartmentStatsExtras {
public:
    CompartmentStatsExtras()
    {}

    nsAutoCString jsPathPrefix;
    nsAutoCString domPathPrefix;
    nsCOMPtr<nsIURI> location;

private:
    CompartmentStatsExtras(const CompartmentStatsExtras &other) MOZ_DELETE;
    CompartmentStatsExtras& operator=(const CompartmentStatsExtras &other) MOZ_DELETE;
};





nsresult
ReportJSRuntimeExplicitTreeStats(const JS::RuntimeStats &rtStats,
                                 const nsACString &rtPath,
                                 nsIMemoryMultiReporterCallback *cb,
                                 nsISupports *closure, size_t *rtTotal = NULL);




bool
Throw(JSContext *cx, nsresult rv);




nsIGlobalObject *
GetNativeForGlobal(JSObject *global);












JSObject *
GetJunkScope();





nsIGlobalObject *
GetJunkScopeGlobal();



void
SystemErrorReporter(JSContext *cx, const char *message, JSErrorReport *rep);





NS_EXPORT_(void)
SystemErrorReporterExternal(JSContext *cx, const char *message,
                            JSErrorReport *rep);

NS_EXPORT_(void)
SimulateActivityCallback(bool aActive);

} 

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
(*DefineInterface)(JSContext *cx, JS::Handle<JSObject*> global,
                   JS::Handle<jsid> id, bool defineOnGlobal);

typedef JSObject*
(*ConstructNavigatorProperty)(JSContext *cx, JS::Handle<JSObject*> naviObj);







typedef bool
(ConstructorEnabled)(JSContext* cx, JS::Handle<JSObject*> obj);

extern bool
DefineStaticJSVals(JSContext *cx);
void
Register(nsScriptNameSpaceManager* aNameSpaceManager);





bool IsChromeOrXBL(JSContext* cx, JSObject* );

} 
} 

#endif
