





#ifndef xpcpublic_h
#define xpcpublic_h

#include "jsapi.h"
#include "jsproxy.h"
#include "js/HeapAPI.h"
#include "js/GCAPI.h"

#include "nsISupports.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"
#include "nsIGlobalObject.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "mozilla/dom/JSSlots.h"
#include "nsMathUtils.h"
#include "nsStringBuffer.h"
#include "mozilla/dom/BindingDeclarations.h"

class nsGlobalWindow;
class nsIPrincipal;
class nsScriptNameSpaceManager;
class nsIMemoryReporterCallback;

#ifndef BAD_TLS_INDEX
#define BAD_TLS_INDEX ((uint32_t) -1)
#endif

namespace xpc {

class Scriptability {
public:
    explicit Scriptability(JSCompartment *c);
    bool Allowed();
    bool IsImmuneToScriptPolicy();

    void Block();
    void Unblock();
    void SetDocShellAllowsScript(bool aAllowed);

    static Scriptability& Get(JSObject *aScope);

private:
    
    
    
    
    uint32_t mScriptBlocks;

    
    
    bool mDocShellAllowsScript;

    
    
    bool mImmuneToScriptPolicy;

    
    
    bool mScriptBlockedByPolicy;
};

JSObject *
TransplantObject(JSContext *cx, JS::HandleObject origobj, JS::HandleObject target);

bool IsContentXBLScope(JSCompartment *compartment);
bool IsInContentXBLScope(JSObject *obj);














JSObject *
GetXBLScope(JSContext *cx, JSObject *contentScope);

inline JSObject *
GetXBLScopeOrGlobal(JSContext *cx, JSObject *obj)
{
    if (IsInContentXBLScope(obj))
        return js::GetGlobalForObjectCrossCompartment(obj);
    return GetXBLScope(cx, obj);
}





JSObject *
GetScopeForXBLExecution(JSContext *cx, JS::HandleObject obj, JSAddonId *addonId);



bool
AllowContentXBLScope(JSCompartment *c);





bool
UseContentXBLScope(JSCompartment *c);

bool
IsInAddonScope(JSObject *obj);

JSObject *
GetAddonScope(JSContext *cx, JS::HandleObject contentScope, JSAddonId *addonId);

bool
IsSandboxPrototypeProxy(JSObject *obj);

bool
IsReflector(JSObject *obj);

bool
IsXrayWrapper(JSObject *obj);






JSObject *
XrayAwareCalleeGlobal(JSObject *fun);

void
TraceXPCGlobal(JSTracer *trc, JSObject *obj);

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

inline JSObject*
xpc_FastGetCachedWrapper(JSContext *cx, nsWrapperCache *cache, JS::MutableHandleValue vp)
{
    if (cache) {
        JSObject* wrapper = cache->GetWrapper();
        if (wrapper &&
            js::GetObjectCompartment(wrapper) == js::GetContextCompartment(cx))
        {
            vp.setObject(*wrapper);
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

inline JSScript *
xpc_UnmarkGrayScript(JSScript *script)
{
    if (script)
        JS::ExposeScriptToActiveJS(script);

    return script;
}



extern void
xpc_MarkInCCGeneration(nsISupports* aVariant, uint32_t aGeneration);


extern void
xpc_TryUnmarkWrappedGrayObject(nsISupports* aWrappedJS);

extern void
xpc_UnmarkSkippableJSHolders();


class XPCStringConvert
{
    
    
    
    
    
    struct ZoneStringCache
    {
        nsStringBuffer* mBuffer;
        JSString* mString;
    };

public:

    
    
    
    static bool ReadableToJSVal(JSContext *cx, const nsAString &readable,
                                nsStringBuffer** sharedBuffer,
                                JS::MutableHandleValue vp);

    
    static MOZ_ALWAYS_INLINE bool
    StringBufferToJSVal(JSContext* cx, nsStringBuffer* buf, uint32_t length,
                        JS::MutableHandleValue rval, bool* sharedBuffer)
    {
        JS::Zone *zone = js::GetContextZone(cx);
        ZoneStringCache *cache = static_cast<ZoneStringCache*>(JS_GetZoneUserData(zone));
        if (cache && buf == cache->mBuffer) {
            MOZ_ASSERT(JS::GetTenuredGCThingZone(cache->mString) == zone);
            JS::MarkStringAsLive(zone, cache->mString);
            rval.setString(cache->mString);
            *sharedBuffer = false;
            return true;
        }

        JSString *str = JS_NewExternalString(cx,
                                             static_cast<jschar*>(buf->Data()),
                                             length, &sDOMStringFinalizer);
        if (!str) {
            return false;
        }
        rval.setString(str);
        if (!cache) {
            cache = new ZoneStringCache();
            JS_SetZoneUserData(zone, cache);
        }
        cache->mBuffer = buf;
        cache->mString = str;
        *sharedBuffer = true;
        return true;
    }

    static void FreeZoneCache(JS::Zone *zone);
    static void ClearZoneCache(JS::Zone *zone);

    static MOZ_ALWAYS_INLINE bool IsLiteral(JSString *str)
    {
        return JS_IsExternalString(str) &&
               JS_GetExternalStringFinalizer(str) == &sLiteralFinalizer;
    }

    static MOZ_ALWAYS_INLINE bool IsDOMString(JSString *str)
    {
        return JS_IsExternalString(str) &&
               JS_GetExternalStringFinalizer(str) == &sDOMStringFinalizer;
    }

private:
    static const JSStringFinalizer sLiteralFinalizer, sDOMStringFinalizer;

    static void FinalizeLiteral(const JSStringFinalizer *fin, jschar *chars);

    static void FinalizeDOMString(const JSStringFinalizer *fin, jschar *chars);

    XPCStringConvert();         
};

class nsIAddonInterposition;

namespace xpc {


bool Base64Encode(JSContext *cx, JS::HandleValue val, JS::MutableHandleValue out);
bool Base64Decode(JSContext *cx, JS::HandleValue val, JS::MutableHandleValue out);






bool NonVoidStringToJsval(JSContext *cx, nsAString &str, JS::MutableHandleValue rval);
inline bool StringToJsval(JSContext *cx, nsAString &str, JS::MutableHandleValue rval)
{
    
    if (str.IsVoid()) {
        rval.setNull();
        return true;
    }
    return NonVoidStringToJsval(cx, str, rval);
}

inline bool
NonVoidStringToJsval(JSContext* cx, const nsAString& str, JS::MutableHandleValue rval)
{
    nsString mutableCopy(str);
    return NonVoidStringToJsval(cx, mutableCopy, rval);
}

inline bool
StringToJsval(JSContext* cx, const nsAString& str, JS::MutableHandleValue rval)
{
    nsString mutableCopy(str);
    return StringToJsval(cx, mutableCopy, rval);
}




MOZ_ALWAYS_INLINE
bool NonVoidStringToJsval(JSContext* cx, mozilla::dom::DOMString& str,
                          JS::MutableHandleValue rval)
{
    if (!str.HasStringBuffer()) {
        
        return NonVoidStringToJsval(cx, str.AsAString(), rval);
    }

    uint32_t length = str.StringBufferLength();
    if (length == 0) {
        rval.set(JS_GetEmptyStringValue(cx));
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
                   JS::MutableHandleValue rval)
{
    if (str.IsNull()) {
        rval.setNull();
        return true;
    }
    return NonVoidStringToJsval(cx, str, rval);
}

nsIPrincipal *GetCompartmentPrincipal(JSCompartment *compartment);

void SetLocationForGlobal(JSObject *global, const nsACString& location);
void SetLocationForGlobal(JSObject *global, nsIURI *locationURI);



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
                                 nsIMemoryReporterCallback *cb,
                                 nsISupports *closure,
                                 bool anonymize,
                                 size_t *rtTotal = nullptr);




bool
Throw(JSContext *cx, nsresult rv);




nsIGlobalObject *
GetNativeForGlobal(JSObject *global);





nsISupports *
UnwrapReflectorToISupports(JSObject *reflector);








JSObject *
UnprivilegedJunkScope();

JSObject *
PrivilegedJunkScope();






JSObject *
CompilationScope();





nsGlobalWindow*
WindowOrNull(JSObject *aObj);





nsGlobalWindow*
WindowGlobalOrNull(JSObject *aObj);



void
SystemErrorReporter(JSContext *cx, const char *message, JSErrorReport *rep);

void
SimulateActivityCallback(bool aActive);



bool
ShouldDiscardSystemSource();

bool
SetAddonInterposition(const nsACString &addonId, nsIAddonInterposition *interposition);

bool
ExtraWarningsForSystemJS();

class ErrorReport {
  public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ErrorReport);

    ErrorReport() : mIsChrome(false)
                  , mLineNumber(0)
                  , mColumn(0)
                  , mFlags(0)
    {}

    void Init(JSErrorReport *aReport, const char *aFallbackMessage,
              nsIGlobalObject *aGlobal);
    void InitOnWorkerThread(JSErrorReport *aReport, const char *aFallbackMessage,
                            bool aIsChrome);

  private:
    void InitInternal(JSErrorReport *aReport, const char *aFallbackMessage);
    bool mIsChrome;

  public:
    const nsCString Category() {
        return mIsChrome ? NS_LITERAL_CSTRING("chrome javascript")
                         : NS_LITERAL_CSTRING("content javascript");
    }

    nsString mErrorMsg;
    nsString mFileName;
    nsString mSourceLine;
    uint32_t mLineNumber;
    uint32_t mColumn;
    uint32_t mFlags;

    
    nsCOMPtr<nsIGlobalObject> mGlobal;
    nsCOMPtr<nsPIDOMWindow> mWindow;

  private:
    ~ErrorReport() {}
};

} 

namespace mozilla {
namespace dom {

typedef JSObject*
(*DefineInterface)(JSContext *cx, JS::Handle<JSObject*> global,
                   JS::Handle<jsid> id, bool defineOnGlobal);

typedef JSObject*
(*ConstructNavigatorProperty)(JSContext *cx, JS::Handle<JSObject*> naviObj);







typedef bool
(ConstructorEnabled)(JSContext* cx, JS::Handle<JSObject*> obj);

void
Register(nsScriptNameSpaceManager* aNameSpaceManager);





bool IsChromeOrXBL(JSContext* cx, JSObject* );

} 
} 

#endif
