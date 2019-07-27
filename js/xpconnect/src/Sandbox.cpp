









#include "AccessCheck.h"
#include "jsfriendapi.h"
#include "js/Proxy.h"
#include "js/StructuredClone.h"
#include "nsContentUtils.h"
#include "nsGlobalWindow.h"
#include "nsIScriptContext.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsIURI.h"
#include "nsJSUtils.h"
#include "nsNetUtil.h"
#include "nsNullPrincipal.h"
#include "nsPrincipal.h"
#include "nsXMLHttpRequest.h"
#include "WrapperFactory.h"
#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "XrayWrapper.h"
#include "Crypto.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/BlobBinding.h"
#include "mozilla/dom/CSSBinding.h"
#include "mozilla/dom/indexedDB/IndexedDatabaseManager.h"
#include "mozilla/dom/FileBinding.h"
#include "mozilla/dom/PromiseBinding.h"
#include "mozilla/dom/RTCIdentityProviderRegistrar.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/TextDecoderBinding.h"
#include "mozilla/dom/TextEncoderBinding.h"
#include "mozilla/dom/URLBinding.h"
#include "mozilla/dom/URLSearchParamsBinding.h"

using namespace mozilla;
using namespace JS;
using namespace js;
using namespace xpc;

using mozilla::dom::DestroyProtoAndIfaceCache;
using mozilla::dom::indexedDB::IndexedDatabaseManager;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(SandboxPrivate)
NS_IMPL_CYCLE_COLLECTING_ADDREF(SandboxPrivate)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SandboxPrivate)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SandboxPrivate)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptObjectPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsIGlobalObject)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

const char kScriptSecurityManagerContractID[] = NS_SCRIPTSECURITYMANAGER_CONTRACTID;

class nsXPCComponents_utils_Sandbox : public nsIXPCComponents_utils_Sandbox,
                                      public nsIXPCScriptable
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_UTILS_SANDBOX
    NS_DECL_NSIXPCSCRIPTABLE

public:
    nsXPCComponents_utils_Sandbox();

private:
    virtual ~nsXPCComponents_utils_Sandbox();

    static nsresult CallOrConstruct(nsIXPConnectWrappedNative* wrapper,
                                    JSContext* cx, HandleObject obj,
                                    const CallArgs& args, bool* _retval);
};

already_AddRefed<nsIXPCComponents_utils_Sandbox>
xpc::NewSandboxConstructor()
{
    nsCOMPtr<nsIXPCComponents_utils_Sandbox> sbConstructor =
        new nsXPCComponents_utils_Sandbox();
    return sbConstructor.forget();
}

static bool
SandboxDump(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0)
        return true;

    RootedString str(cx, ToString(cx, args[0]));
    if (!str)
        return false;

    JSAutoByteString utf8str;
    char* cstr = utf8str.encodeUtf8(cx, str);
    if (!cstr)
        return false;

#if defined(XP_MACOSX)
    
    char* c = cstr;
    char* cEnd = cstr + strlen(cstr);
    while (c < cEnd) {
        if (*c == '\r')
            *c = '\n';
        c++;
    }
#endif
#ifdef ANDROID
    __android_log_write(ANDROID_LOG_INFO, "GeckoDump", cstr);
#endif

    fputs(cstr, stdout);
    fflush(stdout);
    args.rval().setBoolean(true);
    return true;
}

static bool
SandboxDebug(JSContext* cx, unsigned argc, jsval* vp)
{
#ifdef DEBUG
    return SandboxDump(cx, argc, vp);
#else
    return true;
#endif
}

static bool
SandboxImport(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() < 1 || args[0].isPrimitive()) {
        XPCThrower::Throw(NS_ERROR_INVALID_ARG, cx);
        return false;
    }

    RootedString funname(cx);
    if (args.length() > 1) {
        
        funname = ToString(cx, args[1]);
        if (!funname)
            return false;
    } else {
        
        RootedObject funobj(cx, &args[0].toObject());
        if (js::IsProxy(funobj)) {
            funobj = XPCWrapper::UnsafeUnwrapSecurityWrapper(funobj);
        }

        JSAutoCompartment ac(cx, funobj);

        RootedValue funval(cx, ObjectValue(*funobj));
        JSFunction* fun = JS_ValueToFunction(cx, funval);
        if (!fun) {
            XPCThrower::Throw(NS_ERROR_INVALID_ARG, cx);
            return false;
        }

        
        funname = JS_GetFunctionId(fun);
        if (!funname) {
            XPCThrower::Throw(NS_ERROR_INVALID_ARG, cx);
            return false;
        }
    }

    RootedId id(cx);
    if (!JS_StringToId(cx, funname, &id))
        return false;

    
    
    RootedObject thisObject(cx, JS_THIS_OBJECT(cx, vp));
    if (!thisObject) {
        XPCThrower::Throw(NS_ERROR_UNEXPECTED, cx);
        return false;
    }
    if (!JS_SetPropertyById(cx, thisObject, id, args[0]))
        return false;

    args.rval().setUndefined();
    return true;
}

static bool
SandboxCreateCrypto(JSContext* cx, JS::HandleObject obj)
{
    MOZ_ASSERT(JS_IsGlobalObject(obj));

    nsIGlobalObject* native = xpc::NativeGlobal(obj);
    MOZ_ASSERT(native);

    dom::Crypto* crypto = new dom::Crypto();
    crypto->Init(native);
    JS::RootedObject wrapped(cx, crypto->WrapObject(cx, JS::NullPtr()));
    return JS_DefineProperty(cx, obj, "crypto", wrapped, JSPROP_ENUMERATE);
}

static bool
SandboxCreateRTCIdentityProvider(JSContext* cx, JS::HandleObject obj)
{
    MOZ_ASSERT(JS_IsGlobalObject(obj));

    nsCOMPtr<nsIGlobalObject> nativeGlobal = xpc::NativeGlobal(obj);
    MOZ_ASSERT(nativeGlobal);

    dom::RTCIdentityProviderRegistrar* registrar =
            new dom::RTCIdentityProviderRegistrar(nativeGlobal);
    JS::RootedObject wrapped(cx, registrar->WrapObject(cx, JS::NullPtr()));
    return JS_DefineProperty(cx, obj, "rtcIdentityProvider", wrapped, JSPROP_ENUMERATE);
}

static bool
SandboxIsProxy(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 1) {
        JS_ReportError(cx, "Function requires at least 1 argument");
        return false;
    }
    if (!args[0].isObject()) {
        args.rval().setBoolean(false);
        return true;
    }

    RootedObject obj(cx, &args[0].toObject());
    obj = js::CheckedUnwrap(obj);
    NS_ENSURE_TRUE(obj, false);

    args.rval().setBoolean(js::IsScriptedProxy(obj));
    return true;
}







static bool
SandboxExportFunction(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 2) {
        JS_ReportError(cx, "Function requires at least 2 arguments");
        return false;
    }

    RootedValue options(cx, args.length() > 2 ? args[2] : UndefinedValue());
    return ExportFunction(cx, args[0], args[1], options, args.rval());
}

static bool
SandboxCreateObjectIn(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 1) {
        JS_ReportError(cx, "Function requires at least 1 argument");
        return false;
    }

    RootedObject optionsObj(cx);
    bool calledWithOptions = args.length() > 1;
    if (calledWithOptions) {
        if (!args[1].isObject()) {
            JS_ReportError(cx, "Expected the 2nd argument (options) to be an object");
            return false;
        }
        optionsObj = &args[1].toObject();
    }

    CreateObjectInOptions options(cx, optionsObj);
    if (calledWithOptions && !options.Parse())
        return false;

    return xpc::CreateObjectIn(cx, args[0], options, args.rval());
}

static bool
SandboxCloneInto(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 2) {
        JS_ReportError(cx, "Function requires at least 2 arguments");
        return false;
    }

    RootedValue options(cx, args.length() > 2 ? args[2] : UndefinedValue());
    return xpc::CloneInto(cx, args[0], args[1], options, args.rval());
}

static bool
sandbox_enumerate(JSContext* cx, HandleObject obj)
{
    return JS_EnumerateStandardClasses(cx, obj);
}

static bool
sandbox_resolve(JSContext* cx, HandleObject obj, HandleId id, bool* resolvedp)
{
    return JS_ResolveStandardClass(cx, obj, id, resolvedp);
}

static void
sandbox_finalize(js::FreeOp* fop, JSObject* obj)
{
    nsIScriptObjectPrincipal* sop =
        static_cast<nsIScriptObjectPrincipal*>(xpc_GetJSPrivate(obj));
    if (!sop) {
        
        return;
    }

    static_cast<SandboxPrivate*>(sop)->ForgetGlobalObject();
    NS_RELEASE(sop);
    DestroyProtoAndIfaceCache(obj);
}

static void
sandbox_moved(JSObject* obj, const JSObject* old)
{
    
    
    
    nsIScriptObjectPrincipal* sop =
        static_cast<nsIScriptObjectPrincipal*>(xpc_GetJSPrivate(obj));
    if (sop)
        static_cast<SandboxPrivate*>(sop)->ObjectMoved(obj, old);
}

static bool
sandbox_convert(JSContext* cx, HandleObject obj, JSType type, MutableHandleValue vp)
{
    if (type == JSTYPE_OBJECT) {
        vp.set(OBJECT_TO_JSVAL(obj));
        return true;
    }

    return OrdinaryToPrimitive(cx, obj, type, vp);
}

static bool
writeToProto_setProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                         JS::MutableHandleValue vp, JS::ObjectOpResult& result)
{
    RootedObject proto(cx);
    if (!JS_GetPrototype(cx, obj, &proto))
        return false;

    RootedValue receiver(cx, ObjectValue(*proto));
    return JS_ForwardSetPropertyTo(cx, proto, id, vp, receiver, result);
}

static bool
writeToProto_getProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                    JS::MutableHandleValue vp)
{
    RootedObject proto(cx);
    if (!JS_GetPrototype(cx, obj, &proto))
        return false;

    return JS_GetPropertyById(cx, proto, id, vp);
}

struct AutoSkipPropertyMirroring
{
    explicit AutoSkipPropertyMirroring(CompartmentPrivate* priv) : priv(priv) {
        MOZ_ASSERT(!priv->skipWriteToGlobalPrototype);
        priv->skipWriteToGlobalPrototype = true;
    }
    ~AutoSkipPropertyMirroring() {
        MOZ_ASSERT(priv->skipWriteToGlobalPrototype);
        priv->skipWriteToGlobalPrototype = false;
    }

  private:
    CompartmentPrivate* priv;
};









static bool
sandbox_addProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v)
{
    CompartmentPrivate* priv = CompartmentPrivate::Get(obj);
    MOZ_ASSERT(priv->writeToGlobalPrototype);

    
    
    
    if (id == XPCJSRuntime::Get()->GetStringID(XPCJSRuntime::IDX_UNDEFINED))
        return true;

    
    
    if (priv->skipWriteToGlobalPrototype)
        return true;

    AutoSkipPropertyMirroring askip(priv);

    RootedObject proto(cx);
    if (!JS_GetPrototype(cx, obj, &proto))
        return false;

    
    RootedObject unwrappedProto(cx, js::UncheckedUnwrap(proto,  false));

    Rooted<JSPropertyDescriptor> pd(cx);
    if (!JS_GetPropertyDescriptorById(cx, proto, id, &pd))
        return false;

    
    
    
    
    
    
    
    
    
    
    
    
    
    if (pd.object() && !pd.configurable()) {
        if (!JS_SetPropertyById(cx, proto, id, v))
            return false;
    } else {
        if (!JS_CopyPropertyFrom(cx, id, unwrappedProto, obj,
                                 MakeNonConfigurableIntoConfigurable))
            return false;
    }

    if (!JS_GetPropertyDescriptorById(cx, obj, id, &pd))
        return false;
    unsigned attrs = pd.attributes() & ~(JSPROP_GETTER | JSPROP_SETTER);
    if (!JS_DefinePropertyById(cx, obj, id, v,
                               attrs | JSPROP_PROPOP_ACCESSORS | JSPROP_REDEFINE_NONCONFIGURABLE,
                               JS_PROPERTYOP_GETTER(writeToProto_getProperty),
                               JS_PROPERTYOP_SETTER(writeToProto_setProperty)))
        return false;

    return true;
}

#define XPCONNECT_SANDBOX_CLASS_METADATA_SLOT (XPCONNECT_GLOBAL_EXTRA_SLOT_OFFSET)

static const js::Class SandboxClass = {
    "Sandbox",
    XPCONNECT_GLOBAL_FLAGS_WITH_EXTRA_SLOTS(1),
    nullptr, nullptr, nullptr, nullptr,
    sandbox_enumerate, sandbox_resolve,
    nullptr,        
    sandbox_convert,  sandbox_finalize,
    nullptr, nullptr, nullptr, JS_GlobalObjectTraceHook,
    JS_NULL_CLASS_SPEC,
    {
      nullptr,      
      nullptr,      
      false,        
      nullptr,      
      sandbox_moved 
    },
    JS_NULL_OBJECT_OPS
};



static const js::Class SandboxWriteToProtoClass = {
    "Sandbox",
    XPCONNECT_GLOBAL_FLAGS_WITH_EXTRA_SLOTS(1),
    sandbox_addProperty, nullptr, nullptr, nullptr,
    sandbox_enumerate, sandbox_resolve,
    nullptr,        
    sandbox_convert,  sandbox_finalize,
    nullptr, nullptr, nullptr, JS_GlobalObjectTraceHook,
    JS_NULL_CLASS_SPEC,
    {
      nullptr,      
      nullptr,      
      false,        
      nullptr,      
      sandbox_moved 
    },
    JS_NULL_OBJECT_OPS
};

static const JSFunctionSpec SandboxFunctions[] = {
    JS_FS("dump",    SandboxDump,    1,0),
    JS_FS("debug",   SandboxDebug,   1,0),
    JS_FS("importFunction", SandboxImport, 1,0),
    JS_FS_END
};

bool
xpc::IsSandbox(JSObject* obj)
{
    const Class* clasp = GetObjectClass(obj);
    return clasp == &SandboxClass || clasp == &SandboxWriteToProtoClass;
}


nsXPCComponents_utils_Sandbox::nsXPCComponents_utils_Sandbox()
{
}

nsXPCComponents_utils_Sandbox::~nsXPCComponents_utils_Sandbox()
{
}

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_utils_Sandbox)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_utils_Sandbox)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_utils_Sandbox)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXPCComponents_utils_Sandbox)
NS_IMPL_RELEASE(nsXPCComponents_utils_Sandbox)


#define XPC_MAP_CLASSNAME           nsXPCComponents_utils_Sandbox
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_utils_Sandbox"
#define                             XPC_MAP_WANT_CALL
#define                             XPC_MAP_WANT_CONSTRUCT
#define XPC_MAP_FLAGS               0
#include "xpc_map_end.h" 

const xpc::SandboxProxyHandler xpc::sandboxProxyHandler;

bool
xpc::IsSandboxPrototypeProxy(JSObject* obj)
{
    return js::IsProxy(obj) &&
           js::GetProxyHandler(obj) == &xpc::sandboxProxyHandler;
}

bool
xpc::SandboxCallableProxyHandler::call(JSContext* cx, JS::Handle<JSObject*> proxy,
                                       const JS::CallArgs& args) const
{
    

    
    RootedObject sandboxProxy(cx, getSandboxProxy(proxy));
    MOZ_ASSERT(js::IsProxy(sandboxProxy) &&
               js::GetProxyHandler(sandboxProxy) == &xpc::sandboxProxyHandler);

    
    
    RootedObject sandboxGlobal(cx,
      js::GetGlobalForObjectCrossCompartment(sandboxProxy));
    MOZ_ASSERT(IsSandbox(sandboxGlobal));

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool isXray = WrapperFactory::IsXrayWrapper(sandboxProxy);
    RootedValue thisVal(cx, isXray ? args.computeThis(cx) : args.thisv());
    if (thisVal == ObjectValue(*sandboxGlobal)) {
        thisVal = ObjectValue(*js::GetProxyTargetObject(sandboxProxy));
    }

    RootedValue func(cx, js::GetProxyPrivate(proxy));
    return JS::Call(cx, thisVal, func, args, args.rval());
}

const xpc::SandboxCallableProxyHandler xpc::sandboxCallableProxyHandler;





static JSObject*
WrapCallable(JSContext* cx, HandleObject callable, HandleObject sandboxProtoProxy)
{
    MOZ_ASSERT(JS::IsCallable(callable));
    
    
    
    MOZ_ASSERT(js::IsProxy(sandboxProtoProxy) &&
               js::GetProxyHandler(sandboxProtoProxy) ==
                 &xpc::sandboxProxyHandler);

    RootedValue priv(cx, ObjectValue(*callable));
    JSObject* obj = js::NewProxyObject(cx, &xpc::sandboxCallableProxyHandler,
                                       priv, nullptr);
    if (obj) {
        js::SetProxyExtra(obj, SandboxCallableProxyHandler::SandboxProxySlot,
                          ObjectValue(*sandboxProtoProxy));
    }

    return obj;
}

template<typename Op>
bool WrapAccessorFunction(JSContext* cx, Op& op, JSPropertyDescriptor* desc,
                          unsigned attrFlag, HandleObject sandboxProtoProxy)
{
    if (!op) {
        return true;
    }

    if (!(desc->attrs & attrFlag)) {
        XPCThrower::Throw(NS_ERROR_UNEXPECTED, cx);
        return false;
    }

    RootedObject func(cx, JS_FUNC_TO_DATA_PTR(JSObject*, op));
    func = WrapCallable(cx, func, sandboxProtoProxy);
    if (!func)
        return false;
    op = JS_DATA_TO_FUNC_PTR(Op, func.get());
    return true;
}

bool
xpc::SandboxProxyHandler::getPropertyDescriptor(JSContext* cx,
                                                JS::Handle<JSObject*> proxy,
                                                JS::Handle<jsid> id,
                                                JS::MutableHandle<JSPropertyDescriptor> desc) const
{
    JS::RootedObject obj(cx, wrappedObject(proxy));

    MOZ_ASSERT(js::GetObjectCompartment(obj) == js::GetObjectCompartment(proxy));
    if (!JS_GetPropertyDescriptorById(cx, obj, id, desc))
        return false;

    if (!desc.object())
        return true; 

    
    if (!WrapAccessorFunction(cx, desc.getter(), desc.address(),
                              JSPROP_GETTER, proxy))
        return false;
    if (!WrapAccessorFunction(cx, desc.setter(), desc.address(),
                              JSPROP_SETTER, proxy))
        return false;
    if (desc.value().isObject()) {
        RootedObject val (cx, &desc.value().toObject());
        if (JS::IsCallable(val)) {
            val = WrapCallable(cx, val, proxy);
            if (!val)
                return false;
            desc.value().setObject(*val);
        }
    }

    return true;
}

bool
xpc::SandboxProxyHandler::getOwnPropertyDescriptor(JSContext* cx,
                                                   JS::Handle<JSObject*> proxy,
                                                   JS::Handle<jsid> id,
                                                   JS::MutableHandle<JSPropertyDescriptor> desc)
                                                   const
{
    if (!getPropertyDescriptor(cx, proxy, id, desc))
        return false;

    if (desc.object() != wrappedObject(proxy))
        desc.object().set(nullptr);

    return true;
}






bool
xpc::SandboxProxyHandler::has(JSContext* cx, JS::Handle<JSObject*> proxy,
                              JS::Handle<jsid> id, bool* bp) const
{
    return BaseProxyHandler::has(cx, proxy, id, bp);
}
bool
xpc::SandboxProxyHandler::hasOwn(JSContext* cx, JS::Handle<JSObject*> proxy,
                                 JS::Handle<jsid> id, bool* bp) const
{
    return BaseProxyHandler::hasOwn(cx, proxy, id, bp);
}

bool
xpc::SandboxProxyHandler::get(JSContext* cx, JS::Handle<JSObject*> proxy,
                              JS::Handle<JSObject*> receiver,
                              JS::Handle<jsid> id,
                              JS::MutableHandle<Value> vp) const
{
    return BaseProxyHandler::get(cx, proxy, receiver, id, vp);
}

bool
xpc::SandboxProxyHandler::set(JSContext* cx, JS::Handle<JSObject*> proxy,
                              JS::Handle<jsid> id,
                              JS::Handle<Value> v,
                              JS::Handle<Value> receiver,
                              JS::ObjectOpResult& result) const
{
    return BaseProxyHandler::set(cx, proxy, id, v, receiver, result);
}

bool
xpc::SandboxProxyHandler::getOwnEnumerablePropertyKeys(JSContext* cx,
                                                       JS::Handle<JSObject*> proxy,
                                                       AutoIdVector& props) const
{
    return BaseProxyHandler::getOwnEnumerablePropertyKeys(cx, proxy, props);
}

bool
xpc::SandboxProxyHandler::enumerate(JSContext* cx, JS::Handle<JSObject*> proxy,
                                    JS::MutableHandle<JSObject*> objp) const
{
    return BaseProxyHandler::enumerate(cx, proxy, objp);
}

bool
xpc::GlobalProperties::Parse(JSContext* cx, JS::HandleObject obj)
{
    MOZ_ASSERT(JS_IsArrayObject(cx, obj));

    uint32_t length;
    bool ok = JS_GetArrayLength(cx, obj, &length);
    NS_ENSURE_TRUE(ok, false);
    for (uint32_t i = 0; i < length; i++) {
        RootedValue nameValue(cx);
        ok = JS_GetElement(cx, obj, i, &nameValue);
        NS_ENSURE_TRUE(ok, false);
        if (!nameValue.isString()) {
            JS_ReportError(cx, "Property names must be strings");
            return false;
        }
        JSAutoByteString name(cx, nameValue.toString());
        NS_ENSURE_TRUE(name, false);
        if (!strcmp(name.ptr(), "CSS")) {
            CSS = true;
        } else if (!strcmp(name.ptr(), "indexedDB")) {
            indexedDB = true;
        } else if (!strcmp(name.ptr(), "XMLHttpRequest")) {
            XMLHttpRequest = true;
        } else if (!strcmp(name.ptr(), "TextEncoder")) {
            TextEncoder = true;
        } else if (!strcmp(name.ptr(), "TextDecoder")) {
            TextDecoder = true;
        } else if (!strcmp(name.ptr(), "URL")) {
            URL = true;
        } else if (!strcmp(name.ptr(), "URLSearchParams")) {
            URLSearchParams = true;
        } else if (!strcmp(name.ptr(), "atob")) {
            atob = true;
        } else if (!strcmp(name.ptr(), "btoa")) {
            btoa = true;
        } else if (!strcmp(name.ptr(), "Blob")) {
            Blob = true;
        } else if (!strcmp(name.ptr(), "File")) {
            File = true;
        } else if (!strcmp(name.ptr(), "crypto")) {
            crypto = true;
        } else if (!strcmp(name.ptr(), "rtcIdentityProvider")) {
            rtcIdentityProvider = true;
        } else {
            JS_ReportError(cx, "Unknown property name: %s", name.ptr());
            return false;
        }
    }
    return true;
}

bool
xpc::GlobalProperties::Define(JSContext* cx, JS::HandleObject obj)
{
    if (CSS && !dom::CSSBinding::GetConstructorObject(cx, obj))
        return false;

    if (indexedDB && AccessCheck::isChrome(obj) &&
        !IndexedDatabaseManager::DefineIndexedDB(cx, obj))
        return false;

    if (XMLHttpRequest &&
        !dom::XMLHttpRequestBinding::GetConstructorObject(cx, obj))
        return false;

    if (TextEncoder &&
        !dom::TextEncoderBinding::GetConstructorObject(cx, obj))
        return false;

    if (TextDecoder &&
        !dom::TextDecoderBinding::GetConstructorObject(cx, obj))
        return false;

    if (URL &&
        !dom::URLBinding::GetConstructorObject(cx, obj))
        return false;

    if (URLSearchParams &&
        !dom::URLSearchParamsBinding::GetConstructorObject(cx, obj))
        return false;

    if (atob &&
        !JS_DefineFunction(cx, obj, "atob", Atob, 1, 0))
        return false;

    if (btoa &&
        !JS_DefineFunction(cx, obj, "btoa", Btoa, 1, 0))
        return false;

    if (Blob &&
        !dom::BlobBinding::GetConstructorObject(cx, obj))
        return false;

    if (File &&
        !dom::FileBinding::GetConstructorObject(cx, obj))
        return false;

    if (crypto && !SandboxCreateCrypto(cx, obj))
        return false;

    if (rtcIdentityProvider && !SandboxCreateRTCIdentityProvider(cx, obj))
        return false;

    return true;
}

nsresult
xpc::CreateSandboxObject(JSContext* cx, MutableHandleValue vp, nsISupports* prinOrSop,
                         SandboxOptions& options)
{
    
    nsCOMPtr<nsIPrincipal> principal = do_QueryInterface(prinOrSop);
    if (!principal) {
        nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(prinOrSop);
        if (sop) {
            principal = sop->GetPrincipal();
        } else {
            nsRefPtr<nsNullPrincipal> nullPrin = nsNullPrincipal::Create();
            NS_ENSURE_TRUE(nullPrin, NS_ERROR_FAILURE);
            principal = nullPrin;
        }
    }
    MOZ_ASSERT(principal);

    JS::CompartmentOptions compartmentOptions;
    if (options.sameZoneAs)
        compartmentOptions.setSameZoneAs(js::UncheckedUnwrap(options.sameZoneAs));
    else if (options.freshZone)
        compartmentOptions.setZone(JS::FreshZone);
    else
        compartmentOptions.setZone(JS::SystemZone);

    compartmentOptions.setInvisibleToDebugger(options.invisibleToDebugger)
                      .setDiscardSource(options.discardSource)
                      .setTrace(TraceXPCGlobal);

    
    
    
    JSAddonId* addonId = nullptr;
    if (options.addonId) {
        addonId = JS::NewAddonId(cx, options.addonId);
        NS_ENSURE_TRUE(addonId, NS_ERROR_FAILURE);
    } else if (JSObject* obj = JS::CurrentGlobalOrNull(cx)) {
        if (JSAddonId* id = JS::AddonIdOfObject(obj))
            addonId = id;
    }

    compartmentOptions.setAddonId(addonId);

    const Class* clasp = options.writeToGlobalPrototype
                       ? &SandboxWriteToProtoClass
                       : &SandboxClass;

    RootedObject sandbox(cx, xpc::CreateGlobalObject(cx, js::Jsvalify(clasp),
                                                     principal, compartmentOptions));
    if (!sandbox)
        return NS_ERROR_FAILURE;

    CompartmentPrivate::Get(sandbox)->writeToGlobalPrototype =
      options.writeToGlobalPrototype;

    
    
    
    
    
    
    
    
    
    CompartmentPrivate::Get(sandbox)->wantXrays =
      AccessCheck::isChrome(sandbox) ? false : options.wantXrays;

    {
        JSAutoCompartment ac(cx, sandbox);

        if (options.proto) {
            bool ok = JS_WrapObject(cx, &options.proto);
            if (!ok)
                return NS_ERROR_XPC_UNEXPECTED;

            
            JSObject* unwrappedProto = js::UncheckedUnwrap(options.proto, false);
            const js::Class* unwrappedClass = js::GetObjectClass(unwrappedProto);
            if (IS_WN_CLASS(unwrappedClass) ||
                mozilla::dom::IsDOMClass(Jsvalify(unwrappedClass))) {
                
                
                RootedValue priv(cx, ObjectValue(*options.proto));
                options.proto = js::NewProxyObject(cx, &xpc::sandboxProxyHandler,
                                                   priv, nullptr);
                if (!options.proto)
                    return NS_ERROR_OUT_OF_MEMORY;
            }

            ok = JS_SetPrototype(cx, sandbox, options.proto);
            if (!ok)
                return NS_ERROR_XPC_UNEXPECTED;
        }

        nsCOMPtr<nsIScriptObjectPrincipal> sbp =
            new SandboxPrivate(principal, sandbox);

        
        JS_SetPrivate(sandbox, sbp.forget().take());

        
        AutoSkipPropertyMirroring askip(CompartmentPrivate::Get(sandbox));

        bool allowComponents = principal == nsXPConnect::SystemPrincipal() ||
                               nsContentUtils::IsExpandedPrincipal(principal);
        if (options.wantComponents && allowComponents &&
            !ObjectScope(sandbox)->AttachComponentsObject(cx))
            return NS_ERROR_XPC_UNEXPECTED;

        if (!XPCNativeWrapper::AttachNewConstructorObject(cx, sandbox))
            return NS_ERROR_XPC_UNEXPECTED;

        if (!JS_DefineFunctions(cx, sandbox, SandboxFunctions))
            return NS_ERROR_XPC_UNEXPECTED;

        if (options.wantExportHelpers &&
            (!JS_DefineFunction(cx, sandbox, "exportFunction", SandboxExportFunction, 3, 0) ||
             !JS_DefineFunction(cx, sandbox, "createObjectIn", SandboxCreateObjectIn, 2, 0) ||
             !JS_DefineFunction(cx, sandbox, "cloneInto", SandboxCloneInto, 3, 0) ||
             !JS_DefineFunction(cx, sandbox, "isProxy", SandboxIsProxy, 1, 0)))
            return NS_ERROR_XPC_UNEXPECTED;

        if (!options.globalProperties.Define(cx, sandbox))
            return NS_ERROR_XPC_UNEXPECTED;

        
        
        if (!dom::PromiseBinding::GetConstructorObject(cx, sandbox))
            return NS_ERROR_XPC_UNEXPECTED;

        
        if (options.writeToGlobalPrototype && !JS_EnumerateStandardClasses(cx, sandbox))
            return NS_ERROR_XPC_UNEXPECTED;
    }

    
    
    vp.setObject(*sandbox);
    if (js::GetContextCompartment(cx) && !JS_WrapValue(cx, vp))
        return NS_ERROR_UNEXPECTED;

    
    
    xpc::SetLocationForGlobal(sandbox, options.sandboxName);

    xpc::SetSandboxMetadata(cx, sandbox, options.metadata);

    JS_FireOnNewGlobalObject(cx, sandbox);

    return NS_OK;
}








NS_IMETHODIMP
nsXPCComponents_utils_Sandbox::Call(nsIXPConnectWrappedNative* wrapper, JSContext* cx,
                                    JSObject* objArg, const CallArgs& args, bool* _retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);
}








NS_IMETHODIMP
nsXPCComponents_utils_Sandbox::Construct(nsIXPConnectWrappedNative* wrapper, JSContext* cx,
                                         JSObject* objArg, const CallArgs& args, bool* _retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);
}





bool
ParsePrincipal(JSContext* cx, HandleString codebase, nsIPrincipal** principal)
{
    MOZ_ASSERT(principal);
    MOZ_ASSERT(codebase);
    nsCOMPtr<nsIURI> uri;
    nsAutoJSString codebaseStr;
    NS_ENSURE_TRUE(codebaseStr.init(cx, codebase), false);
    nsresult rv = NS_NewURI(getter_AddRefs(uri), codebaseStr);
    if (NS_FAILED(rv)) {
        JS_ReportError(cx, "Creating URI from string failed");
        return false;
    }

    nsCOMPtr<nsIScriptSecurityManager> secman =
        do_GetService(kScriptSecurityManagerContractID);
    NS_ENSURE_TRUE(secman, false);

    
    
    
    rv = secman->GetNoAppCodebasePrincipal(uri, principal);
    if (NS_FAILED(rv) || !*principal) {
        JS_ReportError(cx, "Creating Principal from URI failed");
        return false;
    }
    return true;
}





static bool
GetPrincipalOrSOP(JSContext* cx, HandleObject from, nsISupports** out)
{
    MOZ_ASSERT(out);
    *out = nullptr;

    nsXPConnect* xpc = nsXPConnect::XPConnect();
    nsISupports* native = xpc->GetNativeOfWrapper(cx, from);

    if (nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(native)) {
        sop.forget(out);
        return true;
    }

    nsCOMPtr<nsIPrincipal> principal = do_QueryInterface(native);
    principal.forget(out);
    NS_ENSURE_TRUE(*out, false);

    return true;
}





static bool
GetExpandedPrincipal(JSContext* cx, HandleObject arrayObj, nsIExpandedPrincipal** out)
{
    MOZ_ASSERT(out);
    uint32_t length;

    if (!JS_IsArrayObject(cx, arrayObj) ||
        !JS_GetArrayLength(cx, arrayObj, &length) ||
        !length)
    {
        
        
        
        JS_ReportError(cx, "Expected an array of URI strings");
        return false;
    }

    nsTArray< nsCOMPtr<nsIPrincipal> > allowedDomains(length);
    allowedDomains.SetLength(length);

    for (uint32_t i = 0; i < length; ++i) {
        RootedValue allowed(cx);
        if (!JS_GetElement(cx, arrayObj, i, &allowed))
            return false;

        nsresult rv;
        nsCOMPtr<nsIPrincipal> principal;
        if (allowed.isString()) {
            
            RootedString str(cx, allowed.toString());
            if (!ParsePrincipal(cx, str, getter_AddRefs(principal)))
                return false;

        } else if (allowed.isObject()) {
            
            nsCOMPtr<nsISupports> prinOrSop;
            RootedObject obj(cx, &allowed.toObject());
            if (!GetPrincipalOrSOP(cx, obj, getter_AddRefs(prinOrSop)))
                return false;

            nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(prinOrSop));
            principal = do_QueryInterface(prinOrSop);
            if (sop)
                principal = sop->GetPrincipal();
        }
        NS_ENSURE_TRUE(principal, false);

        
        bool isSystem;
        rv = nsXPConnect::SecurityManager()->IsSystemPrincipal(principal, &isSystem);
        NS_ENSURE_SUCCESS(rv, false);
        if (isSystem) {
            JS_ReportError(cx, "System principal is not allowed in an expanded principal");
            return false;
        }
        allowedDomains[i] = principal;
  }

  nsCOMPtr<nsIExpandedPrincipal> result = new nsExpandedPrincipal(allowedDomains);
  result.forget(out);
  return true;
}




bool
OptionsBase::ParseValue(const char* name, MutableHandleValue prop, bool* aFound)
{
    bool found;
    bool ok = JS_HasProperty(mCx, mObject, name, &found);
    NS_ENSURE_TRUE(ok, false);

    if (aFound)
        *aFound = found;

    if (!found)
        return true;

    return JS_GetProperty(mCx, mObject, name, prop);
}




bool
OptionsBase::ParseBoolean(const char* name, bool* prop)
{
    MOZ_ASSERT(prop);
    RootedValue value(mCx);
    bool found;
    bool ok = ParseValue(name, &value, &found);
    NS_ENSURE_TRUE(ok, false);

    if (!found)
        return true;

    if (!value.isBoolean()) {
        JS_ReportError(mCx, "Expected a boolean value for property %s", name);
        return false;
    }

    *prop = value.toBoolean();
    return true;
}




bool
OptionsBase::ParseObject(const char* name, MutableHandleObject prop)
{
    RootedValue value(mCx);
    bool found;
    bool ok = ParseValue(name, &value, &found);
    NS_ENSURE_TRUE(ok, false);

    if (!found)
        return true;

    if (!value.isObject()) {
        JS_ReportError(mCx, "Expected an object value for property %s", name);
        return false;
    }
    prop.set(&value.toObject());
    return true;
}




bool
OptionsBase::ParseJSString(const char* name, MutableHandleString prop)
{
    RootedValue value(mCx);
    bool found;
    bool ok = ParseValue(name, &value, &found);
    NS_ENSURE_TRUE(ok, false);

    if (!found)
        return true;

    if (!value.isString()) {
        JS_ReportError(mCx, "Expected a string value for property %s", name);
        return false;
    }
    prop.set(value.toString());
    return true;
}




bool
OptionsBase::ParseString(const char* name, nsCString& prop)
{
    RootedValue value(mCx);
    bool found;
    bool ok = ParseValue(name, &value, &found);
    NS_ENSURE_TRUE(ok, false);

    if (!found)
        return true;

    if (!value.isString()) {
        JS_ReportError(mCx, "Expected a string value for property %s", name);
        return false;
    }

    char* tmp = JS_EncodeString(mCx, value.toString());
    NS_ENSURE_TRUE(tmp, false);
    prop.Assign(tmp, strlen(tmp));
    js_free(tmp);
    return true;
}




bool
OptionsBase::ParseString(const char* name, nsString& prop)
{
    RootedValue value(mCx);
    bool found;
    bool ok = ParseValue(name, &value, &found);
    NS_ENSURE_TRUE(ok, false);

    if (!found)
        return true;

    if (!value.isString()) {
        JS_ReportError(mCx, "Expected a string value for property %s", name);
        return false;
    }

    nsAutoJSString strVal;
    if (!strVal.init(mCx, value.toString()))
        return false;

    prop = strVal;
    return true;
}




bool
OptionsBase::ParseId(const char* name, MutableHandleId prop)
{
    RootedValue value(mCx);
    bool found;
    bool ok = ParseValue(name, &value, &found);
    NS_ENSURE_TRUE(ok, false);

    if (!found)
        return true;

    return JS_ValueToId(mCx, value, prop);
}




bool
SandboxOptions::ParseGlobalProperties()
{
    RootedValue value(mCx);
    bool found;
    bool ok = ParseValue("wantGlobalProperties", &value, &found);
    NS_ENSURE_TRUE(ok, false);
    if (!found)
        return true;

    if (!value.isObject()) {
        JS_ReportError(mCx, "Expected an array value for wantGlobalProperties");
        return false;
    }

    RootedObject ctors(mCx, &value.toObject());
    if (!JS_IsArrayObject(mCx, ctors)) {
        JS_ReportError(mCx, "Expected an array value for wantGlobalProperties");
        return false;
    }

    return globalProperties.Parse(mCx, ctors);
}




bool
SandboxOptions::Parse()
{
    bool ok = ParseObject("sandboxPrototype", &proto) &&
              ParseBoolean("wantXrays", &wantXrays) &&
              ParseBoolean("wantComponents", &wantComponents) &&
              ParseBoolean("wantExportHelpers", &wantExportHelpers) &&
              ParseString("sandboxName", sandboxName) &&
              ParseObject("sameZoneAs", &sameZoneAs) &&
              ParseBoolean("freshZone", &freshZone) &&
              ParseBoolean("invisibleToDebugger", &invisibleToDebugger) &&
              ParseBoolean("discardSource", &discardSource) &&
              ParseJSString("addonId", &addonId) &&
              ParseBoolean("writeToGlobalPrototype", &writeToGlobalPrototype) &&
              ParseGlobalProperties() &&
              ParseValue("metadata", &metadata);
    if (!ok)
        return false;

    if (freshZone && sameZoneAs) {
        JS_ReportError(mCx, "Cannot use both sameZoneAs and freshZone");
        return false;
    }

    return true;
}

static nsresult
AssembleSandboxMemoryReporterName(JSContext* cx, nsCString& sandboxName)
{
    
    if (sandboxName.IsEmpty())
        sandboxName = NS_LITERAL_CSTRING("[anonymous sandbox]");

    nsXPConnect* xpc = nsXPConnect::XPConnect();
    
    nsAXPCNativeCallContext* cc = nullptr;
    xpc->GetCurrentNativeCallContext(&cc);
    NS_ENSURE_TRUE(cc, NS_ERROR_INVALID_ARG);

    
    nsCOMPtr<nsIStackFrame> frame;
    xpc->GetCurrentJSStack(getter_AddRefs(frame));

    
    if (frame) {
        nsString location;
        int32_t lineNumber = 0;
        frame->GetFilename(location);
        frame->GetLineNumber(&lineNumber);

        sandboxName.AppendLiteral(" (from: ");
        sandboxName.Append(NS_ConvertUTF16toUTF8(location));
        sandboxName.Append(':');
        sandboxName.AppendInt(lineNumber);
        sandboxName.Append(')');
    }

    return NS_OK;
}


nsresult
nsXPCComponents_utils_Sandbox::CallOrConstruct(nsIXPConnectWrappedNative* wrapper,
                                               JSContext* cx, HandleObject obj,
                                               const CallArgs& args, bool* _retval)
{
    if (args.length() < 1)
        return ThrowAndFail(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx, _retval);

    nsresult rv;
    bool ok = false;

    
    nsCOMPtr<nsIPrincipal> principal;
    nsCOMPtr<nsIExpandedPrincipal> expanded;
    nsCOMPtr<nsISupports> prinOrSop;

    if (args[0].isString()) {
        RootedString str(cx, args[0].toString());
        ok = ParsePrincipal(cx, str, getter_AddRefs(principal));
        prinOrSop = principal;
    } else if (args[0].isObject()) {
        RootedObject obj(cx, &args[0].toObject());
        if (JS_IsArrayObject(cx, obj)) {
            ok = GetExpandedPrincipal(cx, obj, getter_AddRefs(expanded));
            prinOrSop = expanded;
        } else {
            ok = GetPrincipalOrSOP(cx, obj, getter_AddRefs(prinOrSop));
        }
    } else if (args[0].isNull()) {
        
        
        ok = true;
    }

    if (!ok)
        return ThrowAndFail(NS_ERROR_INVALID_ARG, cx, _retval);

    bool calledWithOptions = args.length() > 1;
    if (calledWithOptions && !args[1].isObject())
        return ThrowAndFail(NS_ERROR_INVALID_ARG, cx, _retval);

    RootedObject optionsObject(cx, calledWithOptions ? &args[1].toObject()
                                                     : nullptr);

    SandboxOptions options(cx, optionsObject);
    if (calledWithOptions && !options.Parse())
        return ThrowAndFail(NS_ERROR_INVALID_ARG, cx, _retval);

    if (NS_FAILED(AssembleSandboxMemoryReporterName(cx, options.sandboxName)))
        return ThrowAndFail(NS_ERROR_INVALID_ARG, cx, _retval);

    if (options.metadata.isNullOrUndefined()) {
        
        RootedObject callerGlobal(cx, CurrentGlobalOrNull(cx));
        if (IsSandbox(callerGlobal)) {
            rv = GetSandboxMetadata(cx, callerGlobal, &options.metadata);
            if (NS_WARN_IF(NS_FAILED(rv)))
                return rv;
        }
    }

    rv = CreateSandboxObject(cx, args.rval(), prinOrSop, options);

    if (NS_FAILED(rv))
        return ThrowAndFail(rv, cx, _retval);

    
    
    
    if (!options.wantXrays && !xpc::WrapperFactory::WaiveXrayAndWrap(cx, args.rval()))
        return NS_ERROR_UNEXPECTED;

    *_retval = true;
    return NS_OK;
}

nsresult
xpc::EvalInSandbox(JSContext* cx, HandleObject sandboxArg, const nsAString& source,
                   const nsACString& filename, int32_t lineNo,
                   JSVersion jsVersion, MutableHandleValue rval)
{
    JS_AbortIfWrongThread(JS_GetRuntime(cx));
    rval.set(UndefinedValue());

    bool waiveXray = xpc::WrapperFactory::HasWaiveXrayFlag(sandboxArg);
    RootedObject sandbox(cx, js::CheckedUnwrap(sandboxArg));
    if (!sandbox || !IsSandbox(sandbox)) {
        return NS_ERROR_INVALID_ARG;
    }

    nsIScriptObjectPrincipal* sop =
        static_cast<nsIScriptObjectPrincipal*>(xpc_GetJSPrivate(sandbox));
    MOZ_ASSERT(sop, "Invalid sandbox passed");
    SandboxPrivate* priv = static_cast<SandboxPrivate*>(sop);
    nsCOMPtr<nsIPrincipal> prin = sop->GetPrincipal();
    NS_ENSURE_TRUE(prin, NS_ERROR_FAILURE);

    nsAutoCString filenameBuf;
    if (!filename.IsVoid() && filename.Length() != 0) {
        filenameBuf.Assign(filename);
    } else {
        
        nsJSPrincipals::get(prin)->GetScriptLocation(filenameBuf);
        lineNo = 1;
    }

    
    RootedValue v(cx, UndefinedValue());
    RootedValue exn(cx, UndefinedValue());
    bool ok = true;
    {
        
        
        mozilla::dom::AutoEntryScript aes(priv, "XPConnect sandbox evaluation");
        JSContext* sandcx = aes.cx();
        AutoSaveContextOptions savedOptions(sandcx);
        JS::ContextOptionsRef(sandcx).setDontReportUncaught(true);
        JSAutoCompartment ac(sandcx, sandbox);

        JS::CompileOptions options(sandcx);
        options.setFileAndLine(filenameBuf.get(), lineNo)
               .setVersion(jsVersion);
        MOZ_ASSERT(JS_IsGlobalObject(sandbox));
        ok = JS::Evaluate(sandcx, options,
                          PromiseFlatString(source).get(), source.Length(), &v);

        
        if (JS_GetPendingException(sandcx, &exn)) {
            MOZ_ASSERT(!ok);
            JS_ClearPendingException(sandcx);
        }
    }

    
    
    
    

    if (!ok) {
        
        
        if (exn.isUndefined() || !JS_WrapValue(cx, &exn))
            return NS_ERROR_OUT_OF_MEMORY;

        
        JS_SetPendingException(cx, exn);
        return NS_ERROR_FAILURE;
    }

    
    if (waiveXray) {
        ok = xpc::WrapperFactory::WaiveXrayAndWrap(cx, &v);
    } else {
        ok = JS_WrapValue(cx, &v);
    }
    NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

    
    rval.set(v);
    return NS_OK;
}

nsresult
xpc::GetSandboxAddonId(JSContext* cx, HandleObject sandbox, MutableHandleValue rval)
{
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(IsSandbox(sandbox));

    JSAddonId* id = JS::AddonIdOfObject(sandbox);
    if (!id) {
        rval.setNull();
        return NS_OK;
    }

    JS::RootedValue idStr(cx, StringValue(JS::StringOfAddonId(id)));
    if (!JS_WrapValue(cx, &idStr))
        return NS_ERROR_UNEXPECTED;

    rval.set(idStr);
    return NS_OK;
}

nsresult
xpc::GetSandboxMetadata(JSContext* cx, HandleObject sandbox, MutableHandleValue rval)
{
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(IsSandbox(sandbox));

    RootedValue metadata(cx);
    {
      JSAutoCompartment ac(cx, sandbox);
      metadata = JS_GetReservedSlot(sandbox, XPCONNECT_SANDBOX_CLASS_METADATA_SLOT);
    }

    if (!JS_WrapValue(cx, &metadata))
        return NS_ERROR_UNEXPECTED;

    rval.set(metadata);
    return NS_OK;
}

nsresult
xpc::SetSandboxMetadata(JSContext* cx, HandleObject sandbox, HandleValue metadataArg)
{
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(IsSandbox(sandbox));

    RootedValue metadata(cx);

    JSAutoCompartment ac(cx, sandbox);
    if (!JS_StructuredClone(cx, metadataArg, &metadata, nullptr, nullptr))
        return NS_ERROR_UNEXPECTED;

    JS_SetReservedSlot(sandbox, XPCONNECT_SANDBOX_CLASS_METADATA_SLOT, metadata);

    return NS_OK;
}
