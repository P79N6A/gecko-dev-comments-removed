









#include "AccessCheck.h"
#include "jsfriendapi.h"
#include "jsproxy.h"
#include "js/OldDebugAPI.h"
#include "js/StructuredClone.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsGlobalWindow.h"
#include "nsIScriptContext.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsIURI.h"
#include "nsJSUtils.h"
#include "nsNetUtil.h"
#include "nsPrincipal.h"
#include "nsXMLHttpRequest.h"
#include "WrapperFactory.h"
#include "xpcprivate.h"
#include "XPCQuickStubs.h"
#include "XPCWrapper.h"
#include "XrayWrapper.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/TextDecoderBinding.h"
#include "mozilla/dom/TextEncoderBinding.h"

using namespace mozilla;
using namespace JS;
using namespace js;
using namespace xpc;

using mozilla::dom::DestroyProtoAndIfaceCache;

NS_IMPL_ISUPPORTS3(SandboxPrivate,
                   nsIScriptObjectPrincipal,
                   nsIGlobalObject,
                   nsISupportsWeakReference)

const char kScriptSecurityManagerContractID[] = NS_SCRIPTSECURITYMANAGER_CONTRACTID;

class nsXPCComponents_utils_Sandbox : public nsIXPCComponents_utils_Sandbox,
                                      public nsIXPCScriptable
{
public:
    
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_UTILS_SANDBOX
    NS_DECL_NSIXPCSCRIPTABLE

public:
    nsXPCComponents_utils_Sandbox();
    virtual ~nsXPCComponents_utils_Sandbox();

private:
    static nsresult CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                    JSContext *cx, HandleObject obj,
                                    const CallArgs &args, bool *_retval);
};

already_AddRefed<nsIXPCComponents_utils_Sandbox>
xpc::NewSandboxConstructor()
{
    nsCOMPtr<nsIXPCComponents_utils_Sandbox> sbConstructor =
        new nsXPCComponents_utils_Sandbox();
    return sbConstructor.forget();
}

static bool
SandboxDump(JSContext *cx, unsigned argc, jsval *vp)
{
    JSString *str;
    if (!argc)
        return true;

    str = JS_ValueToString(cx, JS_ARGV(cx, vp)[0]);
    if (!str)
        return false;

    size_t length;
    const jschar *chars = JS_GetStringCharsZAndLength(cx, str, &length);
    if (!chars)
        return false;

    nsDependentString wstr(chars, length);
    char *cstr = ToNewUTF8String(wstr);
    if (!cstr)
        return false;

#if defined(XP_MACOSX)
    
    char *c = cstr, *cEnd = cstr + strlen(cstr);
    while (c < cEnd) {
        if (*c == '\r')
            *c = '\n';
        c++;
    }
#endif

    fputs(cstr, stdout);
    fflush(stdout);
    NS_Free(cstr);
    JS_SET_RVAL(cx, vp, JSVAL_TRUE);
    return true;
}

static bool
SandboxDebug(JSContext *cx, unsigned argc, jsval *vp)
{
#ifdef DEBUG
    return SandboxDump(cx, argc, vp);
#else
    return true;
#endif
}

static bool
SandboxImport(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() < 1 || args[0].isPrimitive()) {
        XPCThrower::Throw(NS_ERROR_INVALID_ARG, cx);
        return false;
    }

    RootedString funname(cx);
    if (args.length() > 1) {
        
        funname = JS_ValueToString(cx, args[1]);
        if (!funname)
            return false;
    } else {
        
        RootedObject funobj(cx, &args[0].toObject());
        if (js::IsProxy(funobj)) {
            funobj = XPCWrapper::UnsafeUnwrapSecurityWrapper(funobj);
        }

        JSAutoCompartment ac(cx, funobj);

        RootedValue funval(cx, ObjectValue(*funobj));
        JSFunction *fun = JS_ValueToFunction(cx, funval);
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
    if (!JS_ValueToId(cx, StringValue(funname), id.address()))
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
CreateXMLHttpRequest(JSContext *cx, unsigned argc, jsval *vp)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (!ssm)
        return false;

    nsIPrincipal *subjectPrincipal = ssm->GetCxSubjectPrincipal(cx);
    if (!subjectPrincipal)
        return false;

    RootedObject global(cx, JS::CurrentGlobalOrNull(cx));
    MOZ_ASSERT(global);

    nsIScriptObjectPrincipal *sop =
        static_cast<nsIScriptObjectPrincipal *>(xpc_GetJSPrivate(global));
    nsCOMPtr<nsIGlobalObject> iglobal = do_QueryInterface(sop);

    nsCOMPtr<nsIXMLHttpRequest> xhr = new nsXMLHttpRequest();
    nsresult rv = xhr->Init(subjectPrincipal, nullptr, iglobal, nullptr);
    if (NS_FAILED(rv))
        return false;

    rv = nsContentUtils::WrapNative(cx, global, xhr, vp);
    if (NS_FAILED(rv))
        return false;

    return true;
}





















static bool
ExportFunction(JSContext *cx, unsigned argc, jsval *vp)
{
    MOZ_ASSERT(cx);
    if (argc < 3) {
        JS_ReportError(cx, "Function requires at least 3 arguments");
        return false;
    }

    CallArgs args = CallArgsFromVp(argc, vp);
    if (!args[0].isObject() || !args[1].isObject() || !args[2].isString()) {
        JS_ReportError(cx, "Invalid argument");
        return false;
    }

    RootedObject funObj(cx, &args[0].toObject());
    RootedObject targetScope(cx, &args[1].toObject());
    RootedString funName(cx, args[2].toString());

    
    
    targetScope = CheckedUnwrap(targetScope);
    if (!targetScope) {
        JS_ReportError(cx, "Permission denied to export function into scope");
        return false;
    }

    if (JS_GetStringLength(funName) == 0) {
        JS_ReportError(cx, "3rd argument should be a non-empty string");
        return false;
    }

    {
        
        
        JSAutoCompartment ac(cx, targetScope);

        
        funObj = UncheckedUnwrap(funObj);
        if (!JS_ObjectIsCallable(cx, funObj)) {
            JS_ReportError(cx, "First argument must be a function");
            return false;
        }

        
        
        
        if (!JS_WrapObject(cx, funObj.address()))
            return false;

        RootedId id(cx);
        if (!JS_ValueToId(cx, args[2], id.address()))
            return false;

        
        
        if (!NewFunctionForwarder(cx, id, funObj,  true, args.rval())) {
            JS_ReportError(cx, "Exporting function failed");
            return false;
        }

        
        
        if (!JS_DefinePropertyById(cx, targetScope, id, args.rval(),
                                   JS_PropertyStub, JS_StrictPropertyStub,
                                   JSPROP_ENUMERATE))
            return false;
    }

    
    if (!JS_WrapValue(cx, args.rval().address()))
        return false;

    return true;
}

static bool
GetFilenameAndLineNumber(JSContext *cx, nsACString &filename, unsigned &lineno)
{
    JS::RootedScript script(cx);
    if (JS_DescribeScriptedCaller(cx, &script, &lineno)) {
        if (const char *cfilename = JS_GetScriptFilename(cx, script)) {
            filename.Assign(nsDependentCString(cfilename));
            return true;
        }
    }
    return false;
}

bool
xpc::IsReflector(JSObject *obj)
{
    return IS_WN_REFLECTOR(obj) || dom::IsDOMObject(obj);
}

enum ForwarderCloneTags {
    SCTAG_BASE = JS_SCTAG_USER_MIN,
    SCTAG_REFLECTOR
};

static JSObject *
CloneNonReflectorsRead(JSContext *cx, JSStructuredCloneReader *reader, uint32_t tag,
                       uint32_t data, void *closure)
{
    MOZ_ASSERT(closure, "Null pointer!");
    AutoObjectVector *reflectors = static_cast<AutoObjectVector *>(closure);
    if (tag == SCTAG_REFLECTOR) {
        MOZ_ASSERT(!data);

        size_t idx;
        if (JS_ReadBytes(reader, &idx, sizeof(size_t))) {
            RootedObject reflector(cx, reflectors->handleAt(idx));
            MOZ_ASSERT(reflector, "No object pointer?");
            MOZ_ASSERT(IsReflector(reflector), "Object pointer must be a reflector!");

            JS_WrapObject(cx, reflector.address());
            JS_ASSERT(WrapperFactory::IsXrayWrapper(reflector) ||
                      IsReflector(reflector));

            return reflector;
        }
    }

    JS_ReportError(cx, "CloneNonReflectorsRead error");
    return nullptr;
}

static bool
CloneNonReflectorsWrite(JSContext *cx, JSStructuredCloneWriter *writer,
                        Handle<JSObject *> obj, void *closure)
{
    MOZ_ASSERT(closure, "Null pointer!");

    
    
    AutoObjectVector *reflectors = static_cast<AutoObjectVector *>(closure);
    if (IsReflector(obj)) {
        if (!reflectors->append(obj))
            return false;

        size_t idx = reflectors->length()-1;
        if (JS_WriteUint32Pair(writer, SCTAG_REFLECTOR, 0) &&
            JS_WriteBytes(writer, &idx, sizeof(size_t))) {
            return true;
        }
    }

    JS_ReportError(cx, "CloneNonReflectorsWrite error");
    return false;
}

JSStructuredCloneCallbacks gForwarderStructuredCloneCallbacks = {
    CloneNonReflectorsRead,
    CloneNonReflectorsWrite,
    nullptr
};







bool
CloneNonReflectors(JSContext *cx, MutableHandleValue val)
{
    JSAutoStructuredCloneBuffer buffer;
    AutoObjectVector rootedReflectors(cx);
    {
        
        
        Maybe<JSAutoCompartment> ac;
        if (val.isObject()) {
            ac.construct(cx, &val.toObject());
        }

        if (!buffer.write(cx, val,
            &gForwarderStructuredCloneCallbacks,
            &rootedReflectors))
        {
            return false;
        }
    }

    
    RootedValue rval(cx);
    if (!buffer.read(cx, val.address(),
        &gForwarderStructuredCloneCallbacks,
        &rootedReflectors))
    {
        return false;
    }

    return true;
}












static bool
EvalInWindow(JSContext *cx, unsigned argc, jsval *vp)
{
    MOZ_ASSERT(cx);
    if (argc < 2) {
        JS_ReportError(cx, "Function requires two arguments");
        return false;
    }

    CallArgs args = CallArgsFromVp(argc, vp);
    if (!args[0].isString() || !args[1].isObject()) {
        JS_ReportError(cx, "Invalid arguments");
        return false;
    }

    RootedString srcString(cx, args[0].toString());
    RootedObject targetScope(cx, &args[1].toObject());

    
    targetScope = CheckedUnwrap(targetScope);
    if (!targetScope) {
        JS_ReportError(cx, "Permission denied to eval in target scope");
        return false;
    }

    
    RootedObject inner(cx, CheckedUnwrap(targetScope,  false));
    nsCOMPtr<nsIGlobalObject> global;
    nsCOMPtr<nsPIDOMWindow> window;
    if (!JS_IsGlobalObject(inner) ||
        !(global = GetNativeForGlobal(inner)) ||
        !(window = do_QueryInterface(global)))
    {
        JS_ReportError(cx, "Second argument must be a window");
        return false;
    }

    nsCOMPtr<nsIScriptContext> context =
        (static_cast<nsGlobalWindow*>(window.get()))->GetScriptContext();
    if (!context) {
        JS_ReportError(cx, "Script context needed");
        return false;
    }

    if (!context->GetScriptsEnabled()) {
        JS_ReportError(cx, "Scripts are disabled in this window");
        return false;
    }

    nsCString filename;
    unsigned lineNo;
    if (!GetFilenameAndLineNumber(cx, filename, lineNo)) {
        
        filename.Assign("Unknown");
        lineNo = 0;
    }

    nsDependentJSString srcDepString;
    srcDepString.init(cx, srcString);

    {
        
        
        JSContext *wndCx = context->GetNativeContext();
        AutoCxPusher pusher(wndCx);
        JS::CompileOptions compileOptions(wndCx);
        compileOptions.setFileAndLine(filename.get(), lineNo);

        
        
        nsJSUtils::EvaluateOptions evaluateOptions;
        evaluateOptions.setReportUncaught(false);

        nsresult rv = nsJSUtils::EvaluateString(wndCx,
                                                srcDepString,
                                                targetScope,
                                                compileOptions,
                                                evaluateOptions,
                                                args.rval().address());

        if (NS_FAILED(rv)) {
            
            
            
            MOZ_ASSERT(!JS_IsExceptionPending(wndCx),
                       "Exception should be delivered as return value.");
            if (args.rval().isUndefined()) {
                MOZ_ASSERT(rv == NS_ERROR_OUT_OF_MEMORY);
                return false;
            }

            
            
            RootedValue exn(wndCx, args.rval());
            
            args.rval().set(UndefinedValue());

            
            if (CloneNonReflectors(cx, &exn))
                JS_SetPendingException(cx, exn);

            return false;
        }
    }

    
    if (!CloneNonReflectors(cx, args.rval())) {
        args.rval().set(UndefinedValue());
        return false;
    }

    return true;
}

static bool
sandbox_enumerate(JSContext *cx, HandleObject obj)
{
    return JS_EnumerateStandardClasses(cx, obj);
}

static bool
sandbox_resolve(JSContext *cx, HandleObject obj, HandleId id)
{
    bool resolved;
    return JS_ResolveStandardClass(cx, obj, id, &resolved);
}

static void
sandbox_finalize(JSFreeOp *fop, JSObject *obj)
{
    nsIScriptObjectPrincipal *sop =
        static_cast<nsIScriptObjectPrincipal *>(xpc_GetJSPrivate(obj));
    MOZ_ASSERT(sop);
    static_cast<SandboxPrivate *>(sop)->ForgetGlobalObject();
    NS_IF_RELEASE(sop);
    DestroyProtoAndIfaceCache(obj);
}

static bool
sandbox_convert(JSContext *cx, HandleObject obj, JSType type, MutableHandleValue vp)
{
    if (type == JSTYPE_OBJECT) {
        vp.set(OBJECT_TO_JSVAL(obj));
        return true;
    }

    return JS_ConvertStub(cx, obj, type, vp);
}

#define XPCONNECT_SANDBOX_CLASS_METADATA_SLOT (XPCONNECT_GLOBAL_EXTRA_SLOT_OFFSET)

static const JSClass SandboxClass = {
    "Sandbox",
    XPCONNECT_GLOBAL_FLAGS_WITH_EXTRA_SLOTS(1),
    JS_PropertyStub,   JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    sandbox_enumerate, sandbox_resolve, sandbox_convert,  sandbox_finalize,
    NULL, NULL, NULL, NULL, TraceXPCGlobal
};

static const JSFunctionSpec SandboxFunctions[] = {
    JS_FS("dump",    SandboxDump,    1,0),
    JS_FS("debug",   SandboxDebug,   1,0),
    JS_FS("importFunction", SandboxImport, 1,0),
    JS_FS_END
};

bool
xpc::IsSandbox(JSObject *obj)
{
    return GetObjectJSClass(obj) == &SandboxClass;
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
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_ADDREF(nsXPCComponents_utils_Sandbox)
NS_IMPL_RELEASE(nsXPCComponents_utils_Sandbox)


#define XPC_MAP_CLASSNAME           nsXPCComponents_utils_Sandbox
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_utils_Sandbox"
#define                             XPC_MAP_WANT_CALL
#define                             XPC_MAP_WANT_CONSTRUCT
#define XPC_MAP_FLAGS               0
#include "xpc_map_end.h" 

xpc::SandboxProxyHandler xpc::sandboxProxyHandler;

bool
xpc::IsSandboxPrototypeProxy(JSObject *obj)
{
    return js::IsProxy(obj) &&
           js::GetProxyHandler(obj) == &xpc::sandboxProxyHandler;
}

bool
xpc::SandboxCallableProxyHandler::call(JSContext *cx, JS::Handle<JSObject*> proxy,
                                       const JS::CallArgs &args)
{
    

    
    RootedObject sandboxProxy(cx, JS_GetParent(proxy));
    MOZ_ASSERT(js::IsProxy(sandboxProxy) &&
               js::GetProxyHandler(sandboxProxy) == &xpc::sandboxProxyHandler);

    
    
    RootedObject sandboxGlobal(cx, JS_GetParent(sandboxProxy));
    MOZ_ASSERT(js::GetObjectJSClass(sandboxGlobal) == &SandboxClass);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    JS::Value thisVal =
      WrapperFactory::IsXrayWrapper(sandboxProxy) ? args.computeThis(cx) : args.thisv();
    if (thisVal == ObjectValue(*sandboxGlobal)) {
        thisVal = ObjectValue(*js::GetProxyTargetObject(sandboxProxy));
    }

    return JS::Call(cx, thisVal, js::GetProxyPrivate(proxy), args.length(), args.array(),
                    args.rval());
}

xpc::SandboxCallableProxyHandler xpc::sandboxCallableProxyHandler;





static JSObject*
WrapCallable(JSContext *cx, JSObject *callable, JSObject *sandboxProtoProxy)
{
    MOZ_ASSERT(JS_ObjectIsCallable(cx, callable));
    
    
    
    MOZ_ASSERT(js::IsProxy(sandboxProtoProxy) &&
               js::GetProxyHandler(sandboxProtoProxy) ==
                 &xpc::sandboxProxyHandler);

    RootedValue priv(cx, ObjectValue(*callable));
    return js::NewProxyObject(cx, &xpc::sandboxCallableProxyHandler,
                              priv, nullptr,
                              sandboxProtoProxy, js::ProxyIsCallable);
}

template<typename Op>
bool BindPropertyOp(JSContext *cx, Op &op, JSPropertyDescriptor *desc, HandleId id,
                    unsigned attrFlag, HandleObject sandboxProtoProxy)
{
    if (!op) {
        return true;
    }

    RootedObject func(cx);
    if (desc->attrs & attrFlag) {
        
        func = JS_FUNC_TO_DATA_PTR(JSObject *, op);
    } else {
        
        
        uint32_t args = (attrFlag == JSPROP_GETTER) ? 0 : 1;
        RootedObject obj(cx, desc->obj);
        func = GeneratePropertyOp(cx, obj, id, args, op);
        if (!func)
            return false;
    }
    func = WrapCallable(cx, func, sandboxProtoProxy);
    if (!func)
        return false;
    op = JS_DATA_TO_FUNC_PTR(Op, func.get());
    desc->attrs |= attrFlag;
    return true;
}

extern bool
XPC_WN_Helper_GetProperty(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp);
extern bool
XPC_WN_Helper_SetProperty(JSContext *cx, HandleObject obj, HandleId id, bool strict, MutableHandleValue vp);

bool
xpc::SandboxProxyHandler::getPropertyDescriptor(JSContext *cx,
                                                JS::Handle<JSObject*> proxy,
                                                JS::Handle<jsid> id,
                                                JS::MutableHandle<JSPropertyDescriptor> desc,
                                                unsigned flags)
{
    JS::RootedObject obj(cx, wrappedObject(proxy));

    MOZ_ASSERT(js::GetObjectCompartment(obj) == js::GetObjectCompartment(proxy));
    if (!JS_GetPropertyDescriptorById(cx, obj, id,
                                      flags, desc))
        return false;

    if (!desc.object())
        return true; 

    
    
    
    
    
    
    
    
    
    
    if (desc.getter() != xpc::holder_get &&
        desc.getter() != XPC_WN_Helper_GetProperty &&
        !BindPropertyOp(cx, desc.getter(), desc.address(), id, JSPROP_GETTER, proxy))
        return false;
    if (desc.setter() != xpc::holder_set &&
        desc.setter() != XPC_WN_Helper_SetProperty &&
        !BindPropertyOp(cx, desc.setter(), desc.address(), id, JSPROP_SETTER, proxy))
        return false;
    if (desc.value().isObject()) {
        JSObject* val = &desc.value().toObject();
        if (JS_ObjectIsCallable(cx, val)) {
            val = WrapCallable(cx, val, proxy);
            if (!val)
                return false;
            desc.value().setObject(*val);
        }
    }

    return true;
}

bool
xpc::SandboxProxyHandler::getOwnPropertyDescriptor(JSContext *cx,
                                                   JS::Handle<JSObject*> proxy,
                                                   JS::Handle<jsid> id,
                                                   JS::MutableHandle<JSPropertyDescriptor> desc,
                                                   unsigned flags)
{
    if (!getPropertyDescriptor(cx, proxy, id, desc, flags))
        return false;

    if (desc.object() != wrappedObject(proxy))
        desc.object().set(nullptr);

    return true;
}






bool
xpc::SandboxProxyHandler::has(JSContext *cx, JS::Handle<JSObject*> proxy,
                              JS::Handle<jsid> id, bool *bp)
{
    return BaseProxyHandler::has(cx, proxy, id, bp);
}
bool
xpc::SandboxProxyHandler::hasOwn(JSContext *cx, JS::Handle<JSObject*> proxy,
                                 JS::Handle<jsid> id, bool *bp)
{
    return BaseProxyHandler::hasOwn(cx, proxy, id, bp);
}

bool
xpc::SandboxProxyHandler::get(JSContext *cx, JS::Handle<JSObject*> proxy,
                              JS::Handle<JSObject*> receiver,
                              JS::Handle<jsid> id,
                              JS::MutableHandle<Value> vp)
{
    return BaseProxyHandler::get(cx, proxy, receiver, id, vp);
}

bool
xpc::SandboxProxyHandler::set(JSContext *cx, JS::Handle<JSObject*> proxy,
                              JS::Handle<JSObject*> receiver,
                              JS::Handle<jsid> id,
                              bool strict,
                              JS::MutableHandle<Value> vp)
{
    return BaseProxyHandler::set(cx, proxy, receiver, id, strict, vp);
}

bool
xpc::SandboxProxyHandler::keys(JSContext *cx, JS::Handle<JSObject*> proxy,
                               AutoIdVector &props)
{
    return BaseProxyHandler::keys(cx, proxy, props);
}

bool
xpc::SandboxProxyHandler::iterate(JSContext *cx, JS::Handle<JSObject*> proxy,
                                  unsigned flags, JS::MutableHandle<Value> vp)
{
    return BaseProxyHandler::iterate(cx, proxy, flags, vp);
}

bool
xpc::SandboxOptions::DOMConstructors::Parse(JSContext* cx, JS::HandleObject obj)
{
    NS_ENSURE_TRUE(JS_IsArrayObject(cx, obj), false);

    uint32_t length;
    bool ok = JS_GetArrayLength(cx, obj, &length);
    NS_ENSURE_TRUE(ok, false);
    for (uint32_t i = 0; i < length; i++) {
        RootedValue nameValue(cx);
        ok = JS_GetElement(cx, obj, i, &nameValue);
        NS_ENSURE_TRUE(ok, false);
        NS_ENSURE_TRUE(nameValue.isString(), false);
        char *name = JS_EncodeString(cx, nameValue.toString());
        NS_ENSURE_TRUE(name, false);
        if (!strcmp(name, "XMLHttpRequest")) {
            XMLHttpRequest = true;
        } else if (!strcmp(name, "TextEncoder")) {
            TextEncoder = true;
        } else if (!strcmp(name, "TextDecoder")) {
            TextDecoder = true;
        } else {
            
            return false;
        }
    }
    return true;
}

bool
xpc::SandboxOptions::DOMConstructors::Define(JSContext* cx, JS::HandleObject obj)
{
    if (XMLHttpRequest &&
        !JS_DefineFunction(cx, obj, "XMLHttpRequest", CreateXMLHttpRequest, 0, JSFUN_CONSTRUCTOR))
        return false;

    if (TextEncoder &&
        !dom::TextEncoderBinding::GetConstructorObject(cx, obj))
        return false;

    if (TextDecoder &&
        !dom::TextDecoderBinding::GetConstructorObject(cx, obj))
        return false;

    return true;
}

nsresult
xpc::CreateSandboxObject(JSContext *cx, jsval *vp, nsISupports *prinOrSop, SandboxOptions& options)
{
    
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    if (NS_FAILED(rv))
        return NS_ERROR_XPC_UNEXPECTED;

    nsCOMPtr<nsIPrincipal> principal = do_QueryInterface(prinOrSop);
    if (!principal) {
        nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(prinOrSop);
        if (sop) {
            principal = sop->GetPrincipal();
        } else {
            principal = do_CreateInstance("@mozilla.org/nullprincipal;1", &rv);
            MOZ_ASSERT(NS_FAILED(rv) || principal, "Bad return from do_CreateInstance");

            if (!principal || NS_FAILED(rv)) {
                if (NS_SUCCEEDED(rv)) {
                    rv = NS_ERROR_FAILURE;
                }

                return rv;
            }
        }
        MOZ_ASSERT(principal);
    }

    JS::CompartmentOptions compartmentOptions;
    if (options.sameZoneAs)
        compartmentOptions.setSameZoneAs(js::UncheckedUnwrap(options.sameZoneAs));
    else
        compartmentOptions.setZone(JS::SystemZone);
    RootedObject sandbox(cx, xpc::CreateGlobalObject(cx, &SandboxClass,
                                                     principal, compartmentOptions));
    if (!sandbox)
        return NS_ERROR_FAILURE;

    
    
    
    
    
    
    
    
    
    xpc::GetCompartmentPrivate(sandbox)->wantXrays =
      AccessCheck::isChrome(sandbox) ? false : options.wantXrays;

    {
        JSAutoCompartment ac(cx, sandbox);

        if (options.proto) {
            bool ok = JS_WrapObject(cx, options.proto.address());
            if (!ok)
                return NS_ERROR_XPC_UNEXPECTED;

            if (xpc::WrapperFactory::IsXrayWrapper(options.proto) && !options.wantXrays) {
                RootedValue v(cx, ObjectValue(*options.proto));
                if (!xpc::WrapperFactory::WaiveXrayAndWrap(cx, v.address()))
                    return NS_ERROR_FAILURE;
                options.proto = &v.toObject();
            }

            
            JSObject *unwrappedProto = js::UncheckedUnwrap(options.proto, false);
            const js::Class *unwrappedClass = js::GetObjectClass(unwrappedProto);
            if (IS_WN_CLASS(unwrappedClass) ||
                mozilla::dom::IsDOMClass(Jsvalify(unwrappedClass))) {
                
                
                RootedValue priv(cx, ObjectValue(*options.proto));
                options.proto = js::NewProxyObject(cx, &xpc::sandboxProxyHandler,
                                                   priv, nullptr, sandbox);
                if (!options.proto)
                    return NS_ERROR_OUT_OF_MEMORY;
            }

            ok = JS_SetPrototype(cx, sandbox, options.proto);
            if (!ok)
                return NS_ERROR_XPC_UNEXPECTED;
        }

        nsCOMPtr<nsIScriptObjectPrincipal> sbp =
            new SandboxPrivate(principal, sandbox);

        
        JS_SetPrivate(sandbox, sbp.forget().get());

        bool allowComponents = nsContentUtils::IsSystemPrincipal(principal) ||
                               nsContentUtils::IsExpandedPrincipal(principal);
        if (options.wantComponents && allowComponents &&
            !nsXPCComponents::AttachComponentsObject(cx, GetObjectScope(sandbox)))
            return NS_ERROR_XPC_UNEXPECTED;

        if (!XPCNativeWrapper::AttachNewConstructorObject(cx, sandbox))
            return NS_ERROR_XPC_UNEXPECTED;

        if (!JS_DefineFunctions(cx, sandbox, SandboxFunctions))
            return NS_ERROR_XPC_UNEXPECTED;

        if (options.wantExportHelpers &&
            (!JS_DefineFunction(cx, sandbox, "exportFunction", ExportFunction, 3, 0) ||
             !JS_DefineFunction(cx, sandbox, "evalInWindow", EvalInWindow, 2, 0)))
            return NS_ERROR_XPC_UNEXPECTED;

        if (!options.DOMConstructors.Define(cx, sandbox))
            return NS_ERROR_XPC_UNEXPECTED;
    }

    if (vp) {
        
        
        
        *vp = OBJECT_TO_JSVAL(sandbox);
        if (options.wantXrays && !JS_WrapValue(cx, vp))
            return NS_ERROR_UNEXPECTED;
        if (!options.wantXrays && !xpc::WrapperFactory::WaiveXrayAndWrap(cx, vp))
            return NS_ERROR_UNEXPECTED;
    }

    
    
    xpc::SetLocationForGlobal(sandbox, options.sandboxName);

    xpc::SetSandboxMetadata(cx, sandbox, options.metadata);

    JS_FireOnNewGlobalObject(cx, sandbox);

    return NS_OK;
}








NS_IMETHODIMP
nsXPCComponents_utils_Sandbox::Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                                    JSObject *objArg, const CallArgs &args, bool *_retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);
}








NS_IMETHODIMP
nsXPCComponents_utils_Sandbox::Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                                         JSObject *objArg, const CallArgs &args, bool *_retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);
}





static nsresult
GetPrincipalFromString(JSContext *cx, HandleString codebase, nsIPrincipal **principal)
{
    MOZ_ASSERT(principal);
    MOZ_ASSERT(codebase);
    nsCOMPtr<nsIURI> uri;
    nsDependentJSString codebaseStr;
    NS_ENSURE_TRUE(codebaseStr.init(cx, codebase), NS_ERROR_FAILURE);
    nsresult rv = NS_NewURI(getter_AddRefs(uri), codebaseStr);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIScriptSecurityManager> secman =
        do_GetService(kScriptSecurityManagerContractID);
    NS_ENSURE_TRUE(secman, NS_ERROR_FAILURE);

    
    
    
    rv = secman->GetNoAppCodebasePrincipal(uri, principal);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(*principal, NS_ERROR_FAILURE);

    return NS_OK;
}





static nsresult
GetPrincipalOrSOP(JSContext *cx, HandleObject from, nsISupports **out)
{
    MOZ_ASSERT(out);
    *out = NULL;

    nsXPConnect* xpc = nsXPConnect::XPConnect();
    nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
    xpc->GetWrappedNativeOfJSObject(cx, from,
                                    getter_AddRefs(wrapper));

    NS_ENSURE_TRUE(wrapper, NS_ERROR_INVALID_ARG);

    if (nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryWrappedNative(wrapper)) {
        sop.forget(out);
        return NS_OK;
    }

    nsCOMPtr<nsIPrincipal> principal = do_QueryWrappedNative(wrapper);
    principal.forget(out);
    NS_ENSURE_TRUE(*out, NS_ERROR_INVALID_ARG);

    return NS_OK;
}





static nsresult
GetExpandedPrincipal(JSContext *cx, HandleObject arrayObj, nsIExpandedPrincipal **out)
{
    MOZ_ASSERT(out);
    uint32_t length;

    if (!JS_IsArrayObject(cx, arrayObj) ||
        !JS_GetArrayLength(cx, arrayObj, &length) ||
        !length)
    {
        
        
        
        return NS_ERROR_INVALID_ARG;
    }

    nsTArray< nsCOMPtr<nsIPrincipal> > allowedDomains(length);
    allowedDomains.SetLength(length);
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    NS_ENSURE_TRUE(ssm, NS_ERROR_XPC_UNEXPECTED);

    for (uint32_t i = 0; i < length; ++i) {
        RootedValue allowed(cx);
        if (!JS_GetElement(cx, arrayObj, i, &allowed))
            return NS_ERROR_INVALID_ARG;

        nsresult rv;
        nsCOMPtr<nsIPrincipal> principal;
        if (allowed.isString()) {
            
            RootedString str(cx, allowed.toString());
            rv = GetPrincipalFromString(cx, str, getter_AddRefs(principal));
            NS_ENSURE_SUCCESS(rv, rv);
        } else if (allowed.isObject()) {
            
            nsCOMPtr<nsISupports> prinOrSop;
            RootedObject obj(cx, &allowed.toObject());
            rv = GetPrincipalOrSOP(cx, obj, getter_AddRefs(prinOrSop));
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(prinOrSop));
            principal = do_QueryInterface(prinOrSop);
            if (sop) {
                principal = sop->GetPrincipal();
            }
        }
        NS_ENSURE_TRUE(principal, NS_ERROR_INVALID_ARG);

        
        bool isSystem;
        rv = ssm->IsSystemPrincipal(principal, &isSystem);
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_FALSE(isSystem, NS_ERROR_INVALID_ARG);
        allowedDomains[i] = principal;
  }

  nsCOMPtr<nsIExpandedPrincipal> result = new nsExpandedPrincipal(allowedDomains);
  result.forget(out);
  return NS_OK;
}




static nsresult
GetPropFromOptions(JSContext *cx, HandleObject from, const char *name, MutableHandleValue prop,
                   bool *found)
{
    MOZ_ASSERT(found);
    bool ok = JS_HasProperty(cx, from, name, found);
    NS_ENSURE_TRUE(ok, NS_ERROR_INVALID_ARG);

    if (!*found)
        return NS_OK;

    ok = JS_GetProperty(cx, from, name, prop);
    NS_ENSURE_TRUE(ok, NS_ERROR_INVALID_ARG);
    return NS_OK;
}




static nsresult
GetBoolPropFromOptions(JSContext *cx, HandleObject from, const char *name, bool *prop)
{
    MOZ_ASSERT(prop);

    RootedValue value(cx);
    bool found;
    nsresult rv = GetPropFromOptions(cx, from, name, &value, &found);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!found)
        return NS_OK;

    NS_ENSURE_TRUE(value.isBoolean(), NS_ERROR_INVALID_ARG);

    *prop = value.toBoolean();
    return NS_OK;
}




static nsresult
GetObjPropFromOptions(JSContext *cx, HandleObject from, const char *name, JSObject **prop)
{
    MOZ_ASSERT(prop);

    RootedValue value(cx);
    bool found;
    nsresult rv = GetPropFromOptions(cx, from, name, &value, &found);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!found) {
        *prop = NULL;
        return NS_OK;
    }

    NS_ENSURE_TRUE(value.isObject(), NS_ERROR_INVALID_ARG);
    *prop = &value.toObject();
    return NS_OK;
}




static nsresult
GetStringPropFromOptions(JSContext *cx, HandleObject from, const char *name, nsCString &prop)
{
    RootedValue value(cx);
    bool found;
    nsresult rv = GetPropFromOptions(cx, from, name, &value, &found);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!found)
        return NS_OK;

    NS_ENSURE_TRUE(value.isString(), NS_ERROR_INVALID_ARG);

    char *tmp = JS_EncodeString(cx, value.toString());
    NS_ENSURE_TRUE(tmp, NS_ERROR_INVALID_ARG);
    prop.Adopt(tmp, strlen(tmp));
    return NS_OK;
}




static nsresult
GetDOMConstructorsFromOptions(JSContext *cx, HandleObject from, SandboxOptions& options)
{
    RootedValue value(cx);
    bool found;
    nsresult rv = GetPropFromOptions(cx, from, "wantDOMConstructors", &value, &found);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!found)
        return NS_OK;

    NS_ENSURE_TRUE(value.isObject(), NS_ERROR_INVALID_ARG);
    RootedObject ctors(cx, &value.toObject());
    bool ok = options.DOMConstructors.Parse(cx, ctors);
    NS_ENSURE_TRUE(ok, NS_ERROR_INVALID_ARG);
    return NS_OK;
}




static nsresult
ParseOptionsObject(JSContext *cx, jsval from, SandboxOptions &options)
{
    NS_ENSURE_TRUE(from.isObject(), NS_ERROR_INVALID_ARG);
    RootedObject optionsObject(cx, &from.toObject());
    nsresult rv = GetObjPropFromOptions(cx, optionsObject,
                                        "sandboxPrototype", options.proto.address());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetBoolPropFromOptions(cx, optionsObject,
                                "wantXrays", &options.wantXrays);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetBoolPropFromOptions(cx, optionsObject,
                                "wantComponents", &options.wantComponents);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetBoolPropFromOptions(cx, optionsObject,
                                "wantExportHelpers", &options.wantExportHelpers);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetStringPropFromOptions(cx, optionsObject,
                                  "sandboxName", options.sandboxName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetObjPropFromOptions(cx, optionsObject,
                               "sameZoneAs", options.sameZoneAs.address());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetDOMConstructorsFromOptions(cx, optionsObject, options);

    bool found;
    rv = GetPropFromOptions(cx, optionsObject,
                            "metadata", &options.metadata, &found);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

static nsresult
AssembleSandboxMemoryReporterName(JSContext *cx, nsCString &sandboxName)
{
    
    if (sandboxName.IsEmpty())
        sandboxName = NS_LITERAL_CSTRING("[anonymous sandbox]");

    nsXPConnect* xpc = nsXPConnect::XPConnect();
    
    nsAXPCNativeCallContext *cc = nullptr;
    xpc->GetCurrentNativeCallContext(&cc);
    NS_ENSURE_TRUE(cc, NS_ERROR_INVALID_ARG);

    
    nsCOMPtr<nsIStackFrame> frame;
    xpc->GetCurrentJSStack(getter_AddRefs(frame));

    
    if (frame) {
        nsCString location;
        int32_t lineNumber = 0;
        frame->GetFilename(getter_Copies(location));
        frame->GetLineNumber(&lineNumber);

        sandboxName.AppendLiteral(" (from: ");
        sandboxName.Append(location);
        sandboxName.AppendLiteral(":");
        sandboxName.AppendInt(lineNumber);
        sandboxName.AppendLiteral(")");
    }

    return NS_OK;
}


nsresult
nsXPCComponents_utils_Sandbox::CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                               JSContext *cx, HandleObject obj,
                                               const CallArgs &args, bool *_retval)
{
    if (args.length() < 1)
        return ThrowAndFail(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx, _retval);

    nsresult rv;

    
    nsCOMPtr<nsIPrincipal> principal;
    nsCOMPtr<nsIExpandedPrincipal> expanded;
    nsCOMPtr<nsISupports> prinOrSop;

    if (args[0].isString()) {
        RootedString str(cx, args[0].toString());
        rv = GetPrincipalFromString(cx, str, getter_AddRefs(principal));
        prinOrSop = principal;
    } else if (args[0].isObject()) {
        RootedObject obj(cx, &args[0].toObject());
        if (JS_IsArrayObject(cx, obj)) {
            rv = GetExpandedPrincipal(cx, obj, getter_AddRefs(expanded));
            prinOrSop = expanded;
        } else {
            rv = GetPrincipalOrSOP(cx, obj, getter_AddRefs(prinOrSop));
        }
    } else {
        return ThrowAndFail(NS_ERROR_INVALID_ARG, cx, _retval);
    }

    if (NS_FAILED(rv))
        return ThrowAndFail(rv, cx, _retval);

    SandboxOptions options(cx);

    if (args.length() > 1 && args[1].isObject()) {
        if (NS_FAILED(ParseOptionsObject(cx, args[1], options)))
            return ThrowAndFail(NS_ERROR_INVALID_ARG, cx, _retval);
    }

    if (NS_FAILED(AssembleSandboxMemoryReporterName(cx, options.sandboxName)))
        return ThrowAndFail(NS_ERROR_INVALID_ARG, cx, _retval);

    rv = CreateSandboxObject(cx, args.rval().address(), prinOrSop, options);

    if (NS_FAILED(rv))
        return ThrowAndFail(rv, cx, _retval);

    *_retval = true;

    return rv;
}

class ContextHolder : public nsIScriptObjectPrincipal
{
public:
    ContextHolder(JSContext *aOuterCx, HandleObject aSandbox, nsIPrincipal *aPrincipal);
    virtual ~ContextHolder();

    JSContext * GetJSContext()
    {
        return mJSContext;
    }

    nsIPrincipal * GetPrincipal() { return mPrincipal; }

    NS_DECL_ISUPPORTS

private:
    JSContext* mJSContext;
    nsCOMPtr<nsIPrincipal> mPrincipal;
};

NS_IMPL_ISUPPORTS1(ContextHolder, nsIScriptObjectPrincipal)

ContextHolder::ContextHolder(JSContext *aOuterCx,
                             HandleObject aSandbox,
                             nsIPrincipal *aPrincipal)
    : mJSContext(JS_NewContext(JS_GetRuntime(aOuterCx), 1024)),
      mPrincipal(aPrincipal)
{
    if (mJSContext) {
        bool isChrome;
        DebugOnly<nsresult> rv = XPCWrapper::GetSecurityManager()->
                                   IsSystemPrincipal(mPrincipal, &isChrome);
        MOZ_ASSERT(NS_SUCCEEDED(rv));

        JS_SetOptions(mJSContext,
                      JS_GetOptions(mJSContext) |
                      JSOPTION_DONT_REPORT_UNCAUGHT |
                      JSOPTION_PRIVATE_IS_NSISUPPORTS);
        js::SetDefaultObjectForContext(mJSContext, aSandbox);
        JS_SetContextPrivate(mJSContext, this);
    }
}

ContextHolder::~ContextHolder()
{
    if (mJSContext)
        JS_DestroyContextNoGC(mJSContext);
}

nsresult
xpc::EvalInSandbox(JSContext *cx, HandleObject sandboxArg, const nsAString& source,
                   const char *filename, int32_t lineNo,
                   JSVersion jsVersion, bool returnStringOnly, MutableHandleValue rval)
{
    JS_AbortIfWrongThread(JS_GetRuntime(cx));
    rval.set(UndefinedValue());

    bool waiveXray = xpc::WrapperFactory::HasWaiveXrayFlag(sandboxArg);
    RootedObject sandbox(cx, js::CheckedUnwrap(sandboxArg));
    if (!sandbox || js::GetObjectJSClass(sandbox) != &SandboxClass) {
        return NS_ERROR_INVALID_ARG;
    }

    nsIScriptObjectPrincipal *sop =
        (nsIScriptObjectPrincipal*)xpc_GetJSPrivate(sandbox);
    MOZ_ASSERT(sop, "Invalid sandbox passed");
    nsCOMPtr<nsIPrincipal> prin = sop->GetPrincipal();
    NS_ENSURE_TRUE(prin, NS_ERROR_FAILURE);

    nsAutoCString filenameBuf;
    if (!filename) {
        
        nsJSPrincipals::get(prin)->GetScriptLocation(filenameBuf);
        filename = filenameBuf.get();
        lineNo = 1;
    }

    
    RootedValue v(cx, UndefinedValue());
    RootedValue exn(cx, UndefinedValue());
    bool ok = true;
    {
        
        
        
        nsRefPtr<ContextHolder> sandcxHolder = new ContextHolder(cx, sandbox, prin);
        JSContext *sandcx = sandcxHolder->GetJSContext();
        if (!sandcx) {
            JS_ReportError(cx, "Can't prepare context for evalInSandbox");
            return NS_ERROR_OUT_OF_MEMORY;
        }
        nsCxPusher pusher;
        pusher.Push(sandcx);
        JSAutoCompartment ac(sandcx, sandbox);

        JS::CompileOptions options(sandcx);
        options.setPrincipals(nsJSPrincipals::get(prin))
               .setFileAndLine(filename, lineNo);
        if (jsVersion != JSVERSION_DEFAULT)
               options.setVersion(jsVersion);
        JS::RootedObject rootedSandbox(sandcx, sandbox);
        ok = JS::Evaluate(sandcx, rootedSandbox, options,
                          PromiseFlatString(source).get(), source.Length(),
                          v.address());
        if (ok && returnStringOnly && !v.isUndefined()) {
            JSString *str = JS_ValueToString(sandcx, v);
            ok = !!str;
            v = ok ? JS::StringValue(str) : JS::UndefinedValue();
        }

        
        if (JS_GetPendingException(sandcx, &exn)) {
            MOZ_ASSERT(!ok);
            JS_ClearPendingException(sandcx);
            if (returnStringOnly) {
                
                
                JSString *str = JS_ValueToString(sandcx, exn);
                exn = str ? JS::StringValue(str) : JS::UndefinedValue();
            }
        }
    }

    
    
    
    

    if (!ok) {
        
        
        if (exn.isUndefined() || !JS_WrapValue(cx, exn.address()))
            return NS_ERROR_OUT_OF_MEMORY;

        
        JS_SetPendingException(cx, exn);
        return NS_ERROR_FAILURE;
    }

    
    if (waiveXray) {
        ok = xpc::WrapperFactory::WaiveXrayAndWrap(cx, v.address());
    } else {
        ok = JS_WrapValue(cx, v.address());
    }
    NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

    
    rval.set(v);
    return NS_OK;
}

bool
NonCloningFunctionForwarder(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedValue v(cx, js::GetFunctionNativeReserved(&args.callee(), 0));
    MOZ_ASSERT(v.isObject(), "weird function");

    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj) {
        return false;
    }
    return JS_CallFunctionValue(cx, obj, v, args.length(), args.array(), vp);
}





bool
CloningFunctionForwarder(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedValue v(cx, js::GetFunctionNativeReserved(&args.callee(), 0));
    NS_ASSERTION(v.isObject(), "weird function");
    RootedObject origFunObj(cx, UncheckedUnwrap(&v.toObject()));
    {
        JSAutoCompartment ac(cx, origFunObj);
        
        
        for (unsigned i = 0; i < args.length(); i++) {
            if (!CloneNonReflectors(cx, args[i])) {
                return false;
            }
        }

        
        
        RootedValue functionVal(cx);
        functionVal.setObject(*origFunObj);

        if (!JS_CallFunctionValue(cx, nullptr, functionVal, args.length(), args.array(), vp))
            return false;
    }

    
    return JS_WrapValue(cx, vp);
}

bool
xpc::NewFunctionForwarder(JSContext *cx, HandleId id, HandleObject callable, bool doclone,
                          MutableHandleValue vp)
{
    JSFunction *fun = js::NewFunctionByIdWithReserved(cx, doclone ? CloningFunctionForwarder :
                                                                    NonCloningFunctionForwarder,
                                                                    0,0, JS::CurrentGlobalOrNull(cx), id);

    if (!fun)
        return false;

    JSObject *funobj = JS_GetFunctionObject(fun);
    js::SetFunctionNativeReserved(funobj, 0, ObjectValue(*callable));
    vp.setObject(*funobj);
    return true;
}


nsresult
xpc::GetSandboxMetadata(JSContext *cx, HandleObject sandbox, MutableHandleValue rval)
{
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(IsSandbox(sandbox));

    RootedValue metadata(cx);
    {
      JSAutoCompartment ac(cx, sandbox);
      metadata = JS_GetReservedSlot(sandbox, XPCONNECT_SANDBOX_CLASS_METADATA_SLOT);
    }

    if (!JS_WrapValue(cx, metadata.address()))
        return NS_ERROR_UNEXPECTED;

    rval.set(metadata);
    return NS_OK;
}

nsresult
xpc::SetSandboxMetadata(JSContext *cx, HandleObject sandbox, HandleValue metadataArg)
{
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(IsSandbox(sandbox));

    RootedValue metadata(cx);

    JSAutoCompartment ac(cx, sandbox);
    if (!JS_StructuredClone(cx, metadataArg, metadata.address(), NULL, NULL))
        return NS_ERROR_UNEXPECTED;

    JS_SetReservedSlot(sandbox, XPCONNECT_SANDBOX_CLASS_METADATA_SLOT, metadata);

    return NS_OK;
}
