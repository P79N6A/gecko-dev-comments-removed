







#include "xpcprivate.h"
#include "jsprf.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/Preferences.h"
#include "nsIAddonInterposition.h"
#include "AddonWrapper.h"
#include "js/Class.h"

using namespace mozilla;
using namespace JS;






static bool Throw(nsresult errNum, JSContext* cx)
{
    XPCThrower::Throw(errNum, cx);
    return false;
}



#define THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper)                          \
    PR_BEGIN_MACRO                                                            \
    if (!wrapper)                                                             \
        return Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);                    \
    if (!wrapper->IsValid())                                                  \
        return Throw(NS_ERROR_XPC_HAS_BEEN_SHUTDOWN, cx);                     \
    PR_END_MACRO



static bool
ToStringGuts(XPCCallContext& ccx)
{
    char* sz;
    XPCWrappedNative* wrapper = ccx.GetWrapper();

    if (wrapper)
        sz = wrapper->ToString(ccx.GetTearOff());
    else
        sz = JS_smprintf("[xpconnect wrapped native prototype]");

    if (!sz) {
        JS_ReportOutOfMemory(ccx);
        return false;
    }

    JSString* str = JS_NewStringCopyZ(ccx, sz);
    JS_smprintf_free(sz);
    if (!str)
        return false;

    ccx.SetRetVal(STRING_TO_JSVAL(str));
    return true;
}



static bool
XPC_WN_Shared_ToString(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
    if (!obj)
        return false;

    XPCCallContext ccx(JS_CALLER, cx, obj);
    if (!ccx.IsValid())
        return Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);
    ccx.SetName(ccx.GetRuntime()->GetStringID(XPCJSRuntime::IDX_TO_STRING));
    ccx.SetArgsAndResultPtr(args.length(), args.array(), vp);
    return ToStringGuts(ccx);
}

static bool
XPC_WN_Shared_ToSource(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    static const char empty[] = "({})";
    JSString* str = JS_NewStringCopyN(cx, empty, sizeof(empty)-1);
    if (!str)
        return false;
    args.rval().setString(str);

    return true;
}













static JSObject*
GetDoubleWrappedJSObject(XPCCallContext& ccx, XPCWrappedNative* wrapper)
{
    RootedObject obj(ccx);
    nsCOMPtr<nsIXPConnectWrappedJS>
        underware = do_QueryInterface(wrapper->GetIdentityObject());
    if (underware) {
        RootedObject mainObj(ccx, underware->GetJSObject());
        if (mainObj) {
            RootedId id(ccx, ccx.GetRuntime()->
                            GetStringID(XPCJSRuntime::IDX_WRAPPED_JSOBJECT));

            JSAutoCompartment ac(ccx, mainObj);

            RootedValue val(ccx);
            if (JS_GetPropertyById(ccx, mainObj, id, &val) &&
                !val.isPrimitive()) {
                obj = val.toObjectOrNull();
            }
        }
    }
    return obj;
}




static bool
XPC_WN_DoubleWrappedGetter(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
    if (!obj)
        return false;

    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    MOZ_ASSERT(JS_TypeOfValue(cx, args.calleev()) == JSTYPE_FUNCTION, "bad function");

    RootedObject realObject(cx, GetDoubleWrappedJSObject(ccx, wrapper));
    if (!realObject) {
        
        
        
        args.rval().setNull();
        return true;
    }

    
    
    if (MOZ_UNLIKELY(!nsContentUtils::IsCallerChrome())) {
        JS_ReportError(cx, "Attempt to use .wrappedJSObject in untrusted code");
        return false;
    }
    args.rval().setObject(*realObject);
    return JS_WrapValue(cx, args.rval());
}











static bool
DefinePropertyIfFound(XPCCallContext& ccx,
                      HandleObject obj,
                      HandleId idArg,
                      XPCNativeSet* set,
                      XPCNativeInterface* iface,
                      XPCNativeMember* member,
                      XPCWrappedNativeScope* scope,
                      bool reflectToStringAndToSource,
                      XPCWrappedNative* wrapperToReflectInterfaceNames,
                      XPCWrappedNative* wrapperToReflectDoubleWrap,
                      XPCNativeScriptableInfo* scriptableInfo,
                      unsigned propFlags,
                      bool* resolved)
{
    RootedId id(ccx, idArg);
    XPCJSRuntime* rt = ccx.GetRuntime();
    bool found;
    const char* name;

    if (set) {
        if (iface)
            found = true;
        else
            found = set->FindMember(id, &member, &iface);
    } else
        found = (nullptr != (member = iface->FindMember(id)));

    if (!found) {
        if (reflectToStringAndToSource) {
            JSNative call;
            uint32_t flags = 0;

            if (scriptableInfo) {
                nsCOMPtr<nsIClassInfo> classInfo = do_QueryInterface(
                    scriptableInfo->GetCallback());

                if (classInfo) {
                    nsresult rv = classInfo->GetFlags(&flags);
                    if (NS_FAILED(rv))
                        return Throw(rv, ccx);
                }
            }

            bool overwriteToString = !(flags & nsIClassInfo::DOM_OBJECT)
                || Preferences::GetBool("dom.XPCToStringForDOMClasses", false);

            if(id == rt->GetStringID(XPCJSRuntime::IDX_TO_STRING)
                && overwriteToString)
            {
                call = XPC_WN_Shared_ToString;
                name = rt->GetStringName(XPCJSRuntime::IDX_TO_STRING);
                id   = rt->GetStringID(XPCJSRuntime::IDX_TO_STRING);
            } else if (id == rt->GetStringID(XPCJSRuntime::IDX_TO_SOURCE)) {
                call = XPC_WN_Shared_ToSource;
                name = rt->GetStringName(XPCJSRuntime::IDX_TO_SOURCE);
                id   = rt->GetStringID(XPCJSRuntime::IDX_TO_SOURCE);
            }

            else
                call = nullptr;

            if (call) {
                RootedFunction fun(ccx, JS_NewFunction(ccx, call, 0, 0, name));
                if (!fun) {
                    JS_ReportOutOfMemory(ccx);
                    return false;
                }

                AutoResolveName arn(ccx, id);
                if (resolved)
                    *resolved = true;
                RootedObject value(ccx, JS_GetFunctionObject(fun));
                return JS_DefinePropertyById(ccx, obj, id, value,
                                             propFlags & ~JSPROP_ENUMERATE);
            }
        }
        
        
        
        

        if (wrapperToReflectInterfaceNames) {
            JSAutoByteString name;
            AutoMarkingNativeInterfacePtr iface2(ccx);
            XPCWrappedNativeTearOff* to;
            RootedObject jso(ccx);
            nsresult rv = NS_OK;

            if (JSID_IS_STRING(id) &&
                name.encodeLatin1(ccx, JSID_TO_STRING(id)) &&
                (iface2 = XPCNativeInterface::GetNewOrUsed(name.ptr()), iface2) &&
                nullptr != (to = wrapperToReflectInterfaceNames->
                           FindTearOff(iface2, true, &rv)) &&
                nullptr != (jso = to->GetJSObject()))

            {
                AutoResolveName arn(ccx, id);
                if (resolved)
                    *resolved = true;
                return JS_DefinePropertyById(ccx, obj, id, jso,
                                             propFlags & ~JSPROP_ENUMERATE);
            } else if (NS_FAILED(rv) && rv != NS_ERROR_NO_INTERFACE) {
                return Throw(rv, ccx);
            }
        }

        
        if (wrapperToReflectDoubleWrap &&
            id == rt->GetStringID(XPCJSRuntime::IDX_WRAPPED_JSOBJECT) &&
            GetDoubleWrappedJSObject(ccx, wrapperToReflectDoubleWrap)) {
            
            

            JSFunction* fun;

            id = rt->GetStringID(XPCJSRuntime::IDX_WRAPPED_JSOBJECT);
            name = rt->GetStringName(XPCJSRuntime::IDX_WRAPPED_JSOBJECT);

            fun = JS_NewFunction(ccx, XPC_WN_DoubleWrappedGetter,
                                 0, 0, name);

            if (!fun)
                return false;

            RootedObject funobj(ccx, JS_GetFunctionObject(fun));
            if (!funobj)
                return false;

            propFlags |= JSPROP_GETTER | JSPROP_SHARED;
            propFlags &= ~JSPROP_ENUMERATE;

            AutoResolveName arn(ccx, id);
            if (resolved)
                *resolved = true;
            return JS_DefinePropertyById(ccx, obj, id, UndefinedHandleValue, propFlags,
                                         JS_DATA_TO_FUNC_PTR(JSNative, funobj.get()),
                                         nullptr);
        }

        if (resolved)
            *resolved = false;
        return true;
    }

    if (!member) {
        if (wrapperToReflectInterfaceNames) {
            XPCWrappedNativeTearOff* to =
              wrapperToReflectInterfaceNames->FindTearOff(iface, true);

            if (!to)
                return false;
            RootedObject jso(ccx, to->GetJSObject());
            if (!jso)
                return false;

            AutoResolveName arn(ccx, id);
            if (resolved)
                *resolved = true;
            return JS_DefinePropertyById(ccx, obj, id, jso,
                                         propFlags & ~JSPROP_ENUMERATE);
        }
        if (resolved)
            *resolved = false;
        return true;
    }

    if (member->IsConstant()) {
        RootedValue val(ccx);
        AutoResolveName arn(ccx, id);
        if (resolved)
            *resolved = true;
        return member->GetConstantValue(ccx, iface, val.address()) &&
               JS_DefinePropertyById(ccx, obj, id, val, propFlags);
    }

    if (scope->HasInterposition()) {
        Rooted<JSPropertyDescriptor> desc(ccx);
        if (!xpc::Interpose(ccx, obj, iface->GetIID(), id, &desc))
            return false;

        if (desc.object()) {
            AutoResolveName arn(ccx, id);
            if (resolved)
                *resolved = true;
            return JS_DefinePropertyById(ccx, obj, id, desc);
        }
    }

    if (id == rt->GetStringID(XPCJSRuntime::IDX_TO_STRING) ||
        id == rt->GetStringID(XPCJSRuntime::IDX_TO_SOURCE) ||
        (scriptableInfo &&
         scriptableInfo->GetFlags().DontEnumQueryInterface() &&
         id == rt->GetStringID(XPCJSRuntime::IDX_QUERY_INTERFACE)))
        propFlags &= ~JSPROP_ENUMERATE;

    RootedValue funval(ccx);
    if (!member->NewFunctionObject(ccx, iface, obj, funval.address()))
        return false;

    if (member->IsMethod()) {
        AutoResolveName arn(ccx, id);
        if (resolved)
            *resolved = true;
        return JS_DefinePropertyById(ccx, obj, id, funval, propFlags);
    }

    

    MOZ_ASSERT(member->IsAttribute(), "way broken!");

    propFlags |= JSPROP_GETTER | JSPROP_SHARED;
    propFlags &= ~JSPROP_READONLY;
    JSObject* funobj = funval.toObjectOrNull();
    JSNative getter = JS_DATA_TO_FUNC_PTR(JSNative, funobj);
    JSNative setter;
    if (member->IsWritableAttribute()) {
        propFlags |= JSPROP_SETTER;
        setter = JS_DATA_TO_FUNC_PTR(JSNative, funobj);
    } else {
        setter = nullptr;
    }

    AutoResolveName arn(ccx, id);
    if (resolved)
        *resolved = true;

    return JS_DefinePropertyById(ccx, obj, id, UndefinedHandleValue, propFlags, getter, setter);
}




static bool
XPC_WN_OnlyIWrite_AddPropertyStub(JSContext* cx, HandleObject obj, HandleId id, HandleValue v)
{
    XPCCallContext ccx(JS_CALLER, cx, obj, NullPtr(), id);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    
    if (ccx.GetResolveName() == id)
        return true;

    return Throw(NS_ERROR_XPC_CANT_MODIFY_PROP_ON_WN, cx);
}

static bool
XPC_WN_CannotModifyPropertyStub(JSContext* cx, HandleObject obj, HandleId id,
                                HandleValue v)
{
    return Throw(NS_ERROR_XPC_CANT_MODIFY_PROP_ON_WN, cx);
}

static bool
XPC_WN_CantDeletePropertyStub(JSContext* cx, HandleObject obj, HandleId id,
                              ObjectOpResult& result)
{
    return Throw(NS_ERROR_XPC_CANT_MODIFY_PROP_ON_WN, cx);
}

static bool
XPC_WN_CannotModifySetPropertyStub(JSContext* cx, HandleObject obj, HandleId id,
                                   MutableHandleValue vp, ObjectOpResult& result)
{
    return Throw(NS_ERROR_XPC_CANT_MODIFY_PROP_ON_WN, cx);
}

static bool
XPC_WN_Shared_Convert(JSContext* cx, HandleObject obj, JSType type, MutableHandleValue vp)
{
    if (type == JSTYPE_OBJECT) {
        vp.set(OBJECT_TO_JSVAL(obj));
        return true;
    }

    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    switch (type) {
        case JSTYPE_FUNCTION:
            {
                if (!ccx.GetTearOff()) {
                    XPCNativeScriptableInfo* si = wrapper->GetScriptableInfo();
                    if (si && (si->GetFlags().WantCall() ||
                               si->GetFlags().WantConstruct())) {
                        vp.set(OBJECT_TO_JSVAL(obj));
                        return true;
                    }
                }
            }
            return Throw(NS_ERROR_XPC_CANT_CONVERT_WN_TO_FUN, cx);
        case JSTYPE_NUMBER:
            vp.set(JS_GetNaNValue(cx));
            return true;
        case JSTYPE_BOOLEAN:
            vp.set(JSVAL_TRUE);
            return true;
        case JSTYPE_VOID:
        case JSTYPE_STRING:
        {
            ccx.SetName(ccx.GetRuntime()->GetStringID(XPCJSRuntime::IDX_TO_STRING));
            ccx.SetArgsAndResultPtr(0, nullptr, vp.address());

            XPCNativeMember* member = ccx.GetMember();
            if (member && member->IsMethod()) {
                if (!XPCWrappedNative::CallMethod(ccx))
                    return false;

                if (vp.isPrimitive())
                    return true;
            }

            
            return ToStringGuts(ccx);
        }
        default:
            NS_ERROR("bad type in conversion");
            return false;
    }
    NS_NOTREACHED("huh?");
    return false;
}

static bool
XPC_WN_Shared_Enumerate(JSContext* cx, HandleObject obj)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    
    
    if (!wrapper->HasMutatedSet())
        return true;

    XPCNativeSet* set = wrapper->GetSet();
    XPCNativeSet* protoSet = wrapper->HasProto() ?
                                wrapper->GetProto()->GetSet() : nullptr;

    uint16_t interface_count = set->GetInterfaceCount();
    XPCNativeInterface** interfaceArray = set->GetInterfaceArray();
    for (uint16_t i = 0; i < interface_count; i++) {
        XPCNativeInterface* iface = interfaceArray[i];
        uint16_t member_count = iface->GetMemberCount();
        for (uint16_t k = 0; k < member_count; k++) {
            XPCNativeMember* member = iface->GetMemberAt(k);
            jsid name = member->GetName();

            
            uint16_t index;
            if (protoSet &&
                protoSet->FindMember(name, nullptr, &index) && index == i)
                continue;
            if (!xpc_ForcePropertyResolve(cx, obj, name))
                return false;
        }
    }
    return true;
}



enum WNHelperType {
    WN_NOHELPER,
    WN_HELPER
};

static void
WrappedNativeFinalize(js::FreeOp* fop, JSObject* obj, WNHelperType helperType)
{
    const js::Class* clazz = js::GetObjectClass(obj);
    if (clazz->flags & JSCLASS_DOM_GLOBAL) {
        mozilla::dom::DestroyProtoAndIfaceCache(obj);
    }
    nsISupports* p = static_cast<nsISupports*>(xpc_GetJSPrivate(obj));
    if (!p)
        return;

    XPCWrappedNative* wrapper = static_cast<XPCWrappedNative*>(p);
    if (helperType == WN_HELPER)
        wrapper->GetScriptableCallback()->Finalize(wrapper, js::CastToJSFreeOp(fop), obj);
    wrapper->FlatJSObjectFinalized();
}

static void
WrappedNativeObjectMoved(JSObject* obj, const JSObject* old)
{
    nsISupports* p = static_cast<nsISupports*>(xpc_GetJSPrivate(obj));
    if (!p)
        return;

    XPCWrappedNative* wrapper = static_cast<XPCWrappedNative*>(p);
    wrapper->FlatJSObjectMoved(obj, old);
}

static void
XPC_WN_NoHelper_Finalize(js::FreeOp* fop, JSObject* obj)
{
    WrappedNativeFinalize(fop, obj, WN_NOHELPER);
}









static void
MarkWrappedNative(JSTracer* trc, JSObject* obj)
{
    const js::Class* clazz = js::GetObjectClass(obj);
    if (clazz->flags & JSCLASS_DOM_GLOBAL) {
        mozilla::dom::TraceProtoAndIfaceCache(trc, obj);
    }
    MOZ_ASSERT(IS_WN_CLASS(clazz));

    XPCWrappedNative* wrapper = XPCWrappedNative::Get(obj);
    if (wrapper && wrapper->IsValid())
        wrapper->TraceInside(trc);
}

 void
XPCWrappedNative::Trace(JSTracer* trc, JSObject* obj)
{
    MarkWrappedNative(trc, obj);
}

static bool
XPC_WN_NoHelper_Resolve(JSContext* cx, HandleObject obj, HandleId id, bool* resolvedp)
{
    XPCCallContext ccx(JS_CALLER, cx, obj, NullPtr(), id);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    XPCNativeSet* set = ccx.GetSet();
    if (!set)
        return true;

    
    if (ccx.GetInterface() && !ccx.GetStaticMemberIsLocal())
        return true;

    return DefinePropertyIfFound(ccx, obj, id,
                                 set, nullptr, nullptr, wrapper->GetScope(),
                                 true, wrapper, wrapper, nullptr,
                                 JSPROP_ENUMERATE |
                                 JSPROP_READONLY |
                                 JSPROP_PERMANENT,
                                 resolvedp);
}

const XPCWrappedNativeJSClass XPC_WN_NoHelper_JSClass = {
  { 
    "XPCWrappedNative_NoHelper",    
    WRAPPER_FLAGS |
    JSCLASS_PRIVATE_IS_NSISUPPORTS, 

    
    XPC_WN_OnlyIWrite_AddPropertyStub, 
    XPC_WN_CantDeletePropertyStub,     
    nullptr,                           
    nullptr,                           

    XPC_WN_Shared_Enumerate,           
    XPC_WN_NoHelper_Resolve,           
    nullptr,                           
    XPC_WN_Shared_Convert,             
    XPC_WN_NoHelper_Finalize,          

    
    nullptr,                         
    nullptr,                         
    nullptr,                         
    XPCWrappedNative::Trace,         
    JS_NULL_CLASS_SPEC,

    
    {
        nullptr, 
        nullptr, 
        true,    
        nullptr, 
        WrappedNativeObjectMoved
    },

    
    {
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, nullptr, 
        nullptr, 
        nullptr, 
        XPC_WN_JSOp_ThisObject,
    }
  }
};




static bool
XPC_WN_MaybeResolvingPropertyStub(JSContext* cx, HandleObject obj, HandleId id, HandleValue v)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    if (ccx.GetResolvingWrapper() == wrapper)
        return true;
    return Throw(NS_ERROR_XPC_CANT_MODIFY_PROP_ON_WN, cx);
}

static bool
XPC_WN_MaybeResolvingSetPropertyStub(JSContext* cx, HandleObject obj, HandleId id,
                                     MutableHandleValue vp, ObjectOpResult& result)
{
    result.succeed();
    return XPC_WN_MaybeResolvingPropertyStub(cx, obj, id, vp);
}

static bool
XPC_WN_MaybeResolvingDeletePropertyStub(JSContext* cx, HandleObject obj, HandleId id,
                                        ObjectOpResult& result)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    if (ccx.GetResolvingWrapper() == wrapper) {
        return result.succeed();
    }
    return Throw(NS_ERROR_XPC_CANT_MODIFY_PROP_ON_WN, cx);
}


#define PRE_HELPER_STUB                                                       \
    JSObject* unwrapped = js::CheckedUnwrap(obj, false);                      \
    if (!unwrapped) {                                                         \
        JS_ReportError(cx, "Permission denied to operate on object.");        \
        return false;                                                         \
    }                                                                         \
    if (!IS_WN_REFLECTOR(unwrapped)) {                                        \
        return Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);                    \
    }                                                                         \
    XPCWrappedNative* wrapper = XPCWrappedNative::Get(unwrapped);             \
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);                             \
    bool retval = true;                                                       \
    nsresult rv = wrapper->GetScriptableCallback()->

#define POST_HELPER_STUB                                                      \
    if (NS_FAILED(rv))                                                        \
        return Throw(rv, cx);                                                 \
    return retval;

#define POST_HELPER_STUB_WITH_OBJECTOPRESULT(failMethod)                      \
    if (NS_FAILED(rv))                                                        \
        return Throw(rv, cx);                                                 \
    return retval ? result.succeed() : result.failMethod();

static bool
XPC_WN_Helper_AddProperty(JSContext* cx, HandleObject obj, HandleId id,
                          HandleValue v)
{
    PRE_HELPER_STUB
    AddProperty(wrapper, cx, obj, id, v, &retval);
    POST_HELPER_STUB
}

bool
XPC_WN_Helper_GetProperty(JSContext* cx, HandleObject obj, HandleId id,
                          MutableHandleValue vp)
{
    PRE_HELPER_STUB
    GetProperty(wrapper, cx, obj, id, vp.address(), &retval);
    POST_HELPER_STUB
}

bool
XPC_WN_Helper_SetProperty(JSContext* cx, HandleObject obj, HandleId id,
                          MutableHandleValue vp, ObjectOpResult& result)
{
    PRE_HELPER_STUB
    SetProperty(wrapper, cx, obj, id, vp.address(), &retval);
    POST_HELPER_STUB_WITH_OBJECTOPRESULT(failReadOnly)
}

static bool
XPC_WN_Helper_Call(JSContext* cx, unsigned argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    
    RootedObject obj(cx, &args.callee());

    XPCCallContext ccx(JS_CALLER, cx, obj, NullPtr(), JSID_VOIDHANDLE, args.length(),
                       args.array(), args.rval().address());
    if (!ccx.IsValid())
        return false;

    PRE_HELPER_STUB
    Call(wrapper, cx, obj, args, &retval);
    POST_HELPER_STUB
}

static bool
XPC_WN_Helper_Construct(JSContext* cx, unsigned argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    RootedObject obj(cx, &args.callee());
    if (!obj)
        return false;

    XPCCallContext ccx(JS_CALLER, cx, obj, NullPtr(), JSID_VOIDHANDLE, args.length(),
                       args.array(), args.rval().address());
    if (!ccx.IsValid())
        return false;

    PRE_HELPER_STUB
    Construct(wrapper, cx, obj, args, &retval);
    POST_HELPER_STUB
}

static bool
XPC_WN_Helper_HasInstance(JSContext* cx, HandleObject obj, MutableHandleValue valp, bool* bp)
{
    bool retval2;
    PRE_HELPER_STUB
    HasInstance(wrapper, cx, obj, valp, &retval2, &retval);
    *bp = retval2;
    POST_HELPER_STUB
}

static void
XPC_WN_Helper_Finalize(js::FreeOp* fop, JSObject* obj)
{
    WrappedNativeFinalize(fop, obj, WN_HELPER);
}

static bool
XPC_WN_Helper_Resolve(JSContext* cx, HandleObject obj, HandleId id, bool* resolvedp)
{
    nsresult rv = NS_OK;
    bool retval = true;
    bool resolved = false;
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    RootedId old(cx, ccx.SetResolveName(id));

    XPCNativeScriptableInfo* si = wrapper->GetScriptableInfo();
    if (si && si->GetFlags().WantResolve()) {
        XPCWrappedNative* oldResolvingWrapper;
        bool allowPropMods = si->GetFlags().AllowPropModsDuringResolve();

        if (allowPropMods)
            oldResolvingWrapper = ccx.SetResolvingWrapper(wrapper);

        rv = si->GetCallback()->Resolve(wrapper, cx, obj, id, &resolved, &retval);

        if (allowPropMods)
            (void)ccx.SetResolvingWrapper(oldResolvingWrapper);
    }

    old = ccx.SetResolveName(old);
    MOZ_ASSERT(old == id, "bad nest");

    if (NS_FAILED(rv)) {
        return Throw(rv, cx);
    }

    if (resolved) {
        *resolvedp = true;
    } else if (wrapper->HasMutatedSet()) {
        
        

        XPCNativeSet* set = wrapper->GetSet();
        XPCNativeSet* protoSet = wrapper->HasProto() ?
                                    wrapper->GetProto()->GetSet() : nullptr;
        XPCNativeMember* member;
        XPCNativeInterface* iface;
        bool IsLocal;

        if (set->FindMember(id, &member, &iface, protoSet, &IsLocal) &&
            IsLocal) {
            XPCWrappedNative* oldResolvingWrapper;

            XPCNativeScriptableFlags siFlags(0);
            if (si)
                siFlags = si->GetFlags();

            XPCWrappedNative* wrapperForInterfaceNames =
                siFlags.DontReflectInterfaceNames() ? nullptr : wrapper;

            oldResolvingWrapper = ccx.SetResolvingWrapper(wrapper);
            retval = DefinePropertyIfFound(ccx, obj, id,
                                           set, iface, member,
                                           wrapper->GetScope(),
                                           false,
                                           wrapperForInterfaceNames,
                                           nullptr, si,
                                           JSPROP_ENUMERATE, resolvedp);
            (void)ccx.SetResolvingWrapper(oldResolvingWrapper);
        }
    }

    return retval;
}

static bool
XPC_WN_Helper_Enumerate(JSContext* cx, HandleObject obj)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    XPCNativeScriptableInfo* si = wrapper->GetScriptableInfo();
    if (!si || !si->GetFlags().WantEnumerate())
        return Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);

    if (!XPC_WN_Shared_Enumerate(cx, obj))
        return false;

    bool retval = true;
    nsresult rv = si->GetCallback()->Enumerate(wrapper, cx, obj, &retval);
    if (NS_FAILED(rv))
        return Throw(rv, cx);
    return retval;
}



static bool
XPC_WN_JSOp_Enumerate(JSContext* cx, HandleObject obj, AutoIdVector& properties)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    XPCNativeScriptableInfo* si = wrapper->GetScriptableInfo();
    if (!si || !si->GetFlags().WantNewEnumerate())
        return Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);

    if (!XPC_WN_Shared_Enumerate(cx, obj))
        return false;

    bool retval = true;
    nsresult rv = si->GetCallback()->NewEnumerate(wrapper, cx, obj, properties, &retval);
    if (NS_FAILED(rv))
        return Throw(rv, cx);
    return retval;
}

JSObject*
XPC_WN_JSOp_ThisObject(JSContext* cx, HandleObject obj)
{
    return JS_ObjectToOuterObject(cx, obj);
}




XPCNativeScriptableInfo*
XPCNativeScriptableInfo::Construct(const XPCNativeScriptableCreateInfo* sci)
{
    MOZ_ASSERT(sci, "bad param");
    MOZ_ASSERT(sci->GetCallback(), "bad param");

    XPCNativeScriptableInfo* newObj =
        new XPCNativeScriptableInfo(sci->GetCallback());
    if (!newObj)
        return nullptr;

    char* name = nullptr;
    if (NS_FAILED(sci->GetCallback()->GetClassName(&name)) || !name) {
        delete newObj;
        return nullptr;
    }

    bool success;

    XPCJSRuntime* rt = XPCJSRuntime::Get();
    XPCNativeScriptableSharedMap* map = rt->GetNativeScriptableSharedMap();
    success = map->GetNewOrUsed(sci->GetFlags(), name, newObj);

    if (!success) {
        delete newObj;
        return nullptr;
    }

    return newObj;
}

void
XPCNativeScriptableShared::PopulateJSClass()
{
    MOZ_ASSERT(mJSClass.base.name, "bad state!");

    mJSClass.base.flags = WRAPPER_FLAGS |
                          JSCLASS_PRIVATE_IS_NSISUPPORTS;

    if (mFlags.IsGlobalObject())
        mJSClass.base.flags |= XPCONNECT_GLOBAL_FLAGS;

    JSAddPropertyOp addProperty;
    if (mFlags.WantAddProperty())
        addProperty = XPC_WN_Helper_AddProperty;
    else if (mFlags.UseJSStubForAddProperty())
        addProperty = nullptr;
    else if (mFlags.AllowPropModsDuringResolve())
        addProperty = XPC_WN_MaybeResolvingPropertyStub;
    else
        addProperty = XPC_WN_CannotModifyPropertyStub;
    mJSClass.base.addProperty = addProperty;

    JSDeletePropertyOp delProperty;
    if (mFlags.UseJSStubForDelProperty())
        delProperty = nullptr;
    else if (mFlags.AllowPropModsDuringResolve())
        delProperty = XPC_WN_MaybeResolvingDeletePropertyStub;
    else
        delProperty = XPC_WN_CantDeletePropertyStub;
    mJSClass.base.delProperty = delProperty;

    if (mFlags.WantGetProperty())
        mJSClass.base.getProperty = XPC_WN_Helper_GetProperty;
    else
        mJSClass.base.getProperty = nullptr;

    JSSetterOp setProperty;
    if (mFlags.WantSetProperty())
        setProperty = XPC_WN_Helper_SetProperty;
    else if (mFlags.UseJSStubForSetProperty())
        setProperty = nullptr;
    else if (mFlags.AllowPropModsDuringResolve())
        setProperty = XPC_WN_MaybeResolvingSetPropertyStub;
    else
        setProperty = XPC_WN_CannotModifySetPropertyStub;
    mJSClass.base.setProperty = setProperty;

    MOZ_ASSERT_IF(mFlags.WantEnumerate(), !mFlags.WantNewEnumerate());
    MOZ_ASSERT_IF(mFlags.WantNewEnumerate(), !mFlags.WantEnumerate());

    
    if (mFlags.WantNewEnumerate())
        mJSClass.base.enumerate = nullptr;
    else if (mFlags.WantEnumerate())
        mJSClass.base.enumerate = XPC_WN_Helper_Enumerate;
    else
        mJSClass.base.enumerate = XPC_WN_Shared_Enumerate;

    
    mJSClass.base.resolve = XPC_WN_Helper_Resolve;

    mJSClass.base.convert = XPC_WN_Shared_Convert;

    if (mFlags.WantFinalize())
        mJSClass.base.finalize = XPC_WN_Helper_Finalize;
    else
        mJSClass.base.finalize = XPC_WN_NoHelper_Finalize;

    js::ObjectOps* ops = &mJSClass.base.ops;
    if (mFlags.WantNewEnumerate())
        ops->enumerate = XPC_WN_JSOp_Enumerate;
    ops->thisObject = XPC_WN_JSOp_ThisObject;


    if (mFlags.WantCall())
        mJSClass.base.call = XPC_WN_Helper_Call;
    if (mFlags.WantConstruct())
        mJSClass.base.construct = XPC_WN_Helper_Construct;

    if (mFlags.WantHasInstance())
        mJSClass.base.hasInstance = XPC_WN_Helper_HasInstance;

    if (mFlags.IsGlobalObject())
        mJSClass.base.trace = JS_GlobalObjectTraceHook;
    else
        mJSClass.base.trace = XPCWrappedNative::Trace;

    mJSClass.base.ext.isWrappedNative = true;
    mJSClass.base.ext.objectMovedOp = WrappedNativeObjectMoved;
}
























#define IS_NOHELPER_CLASS(clasp) (clasp == &XPC_WN_NoHelper_JSClass.base)
#define IS_CU_CLASS(clasp) (clasp->name[0] == 'n' && !strcmp(clasp->name, "nsXPCComponents_Utils"))

MOZ_ALWAYS_INLINE JSObject*
FixUpThisIfBroken(JSObject* obj, JSObject* funobj)
{
    if (funobj) {
        JSObject* parentObj =
            &js::GetFunctionNativeReserved(funobj,
                                           XPC_FUNCTION_PARENT_OBJECT_SLOT).toObject();
        const js::Class* parentClass = js::GetObjectClass(parentObj);
        if (MOZ_UNLIKELY((IS_NOHELPER_CLASS(parentClass) || IS_CU_CLASS(parentClass)) &&
                         (js::GetObjectClass(obj) != parentClass)))
        {
            return parentObj;
        }
    }
    return obj;
}

bool
XPC_WN_CallMethod(JSContext* cx, unsigned argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    MOZ_ASSERT(JS_TypeOfValue(cx, args.calleev()) == JSTYPE_FUNCTION, "bad function");
    RootedObject funobj(cx, &args.callee());

    RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
    if (!obj)
        return false;

    obj = FixUpThisIfBroken(obj, funobj);
    XPCCallContext ccx(JS_CALLER, cx, obj, funobj, JSID_VOIDHANDLE, args.length(),
                       args.array(), vp);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    XPCNativeInterface* iface;
    XPCNativeMember*    member;

    if (!XPCNativeMember::GetCallInfo(funobj, &iface, &member))
        return Throw(NS_ERROR_XPC_CANT_GET_METHOD_INFO, cx);
    ccx.SetCallInfo(iface, member, false);
    return XPCWrappedNative::CallMethod(ccx);
}

bool
XPC_WN_GetterSetter(JSContext* cx, unsigned argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    MOZ_ASSERT(JS_TypeOfValue(cx, args.calleev()) == JSTYPE_FUNCTION, "bad function");
    RootedObject funobj(cx, &args.callee());

    RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
    if (!obj)
        return false;

    obj = FixUpThisIfBroken(obj, funobj);
    XPCCallContext ccx(JS_CALLER, cx, obj, funobj, JSID_VOIDHANDLE, args.length(),
                       args.array(), vp);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    XPCNativeInterface* iface;
    XPCNativeMember*    member;

    if (!XPCNativeMember::GetCallInfo(funobj, &iface, &member))
        return Throw(NS_ERROR_XPC_CANT_GET_METHOD_INFO, cx);

    if (args.length() != 0 && member->IsWritableAttribute()) {
        ccx.SetCallInfo(iface, member, true);
        bool retval = XPCWrappedNative::SetAttribute(ccx);
        if (retval)
            args.rval().set(args[0]);
        return retval;
    }
    

    ccx.SetCallInfo(iface, member, false);
    return XPCWrappedNative::GetAttribute(ccx);
}



static bool
XPC_WN_Shared_Proto_Enumerate(JSContext* cx, HandleObject obj)
{
    MOZ_ASSERT(js::GetObjectClass(obj) == &XPC_WN_ModsAllowed_WithCall_Proto_JSClass ||
               js::GetObjectClass(obj) == &XPC_WN_ModsAllowed_NoCall_Proto_JSClass ||
               js::GetObjectClass(obj) == &XPC_WN_NoMods_WithCall_Proto_JSClass ||
               js::GetObjectClass(obj) == &XPC_WN_NoMods_NoCall_Proto_JSClass,
               "bad proto");
    XPCWrappedNativeProto* self =
        (XPCWrappedNativeProto*) xpc_GetJSPrivate(obj);
    if (!self)
        return false;

    XPCNativeSet* set = self->GetSet();
    if (!set)
        return false;

    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid())
        return false;

    uint16_t interface_count = set->GetInterfaceCount();
    XPCNativeInterface** interfaceArray = set->GetInterfaceArray();
    for (uint16_t i = 0; i < interface_count; i++) {
        XPCNativeInterface* iface = interfaceArray[i];
        uint16_t member_count = iface->GetMemberCount();

        for (uint16_t k = 0; k < member_count; k++) {
            if (!xpc_ForcePropertyResolve(cx, obj, iface->GetMemberAt(k)->GetName()))
                return false;
        }
    }

    return true;
}

static void
XPC_WN_Shared_Proto_Finalize(js::FreeOp* fop, JSObject* obj)
{
    
    XPCWrappedNativeProto* p = (XPCWrappedNativeProto*) xpc_GetJSPrivate(obj);
    if (p)
        p->JSProtoObjectFinalized(fop, obj);
}

static void
XPC_WN_Shared_Proto_ObjectMoved(JSObject* obj, const JSObject* old)
{
    
    XPCWrappedNativeProto* p = (XPCWrappedNativeProto*) xpc_GetJSPrivate(obj);
    if (p)
        p->JSProtoObjectMoved(obj, old);
}

static void
XPC_WN_Shared_Proto_Trace(JSTracer* trc, JSObject* obj)
{
    
    XPCWrappedNativeProto* p =
        (XPCWrappedNativeProto*) xpc_GetJSPrivate(obj);
    if (p)
        p->TraceInside(trc);
}



static bool
XPC_WN_ModsAllowed_Proto_Resolve(JSContext* cx, HandleObject obj, HandleId id, bool* resolvep)
{
    MOZ_ASSERT(js::GetObjectClass(obj) == &XPC_WN_ModsAllowed_WithCall_Proto_JSClass ||
               js::GetObjectClass(obj) == &XPC_WN_ModsAllowed_NoCall_Proto_JSClass,
               "bad proto");

    XPCWrappedNativeProto* self =
        (XPCWrappedNativeProto*) xpc_GetJSPrivate(obj);
    if (!self)
        return false;

    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid())
        return false;

    XPCNativeScriptableInfo* si = self->GetScriptableInfo();
    return DefinePropertyIfFound(ccx, obj, id,
                                 self->GetSet(), nullptr, nullptr,
                                 self->GetScope(),
                                 true, nullptr, nullptr, si,
                                 JSPROP_ENUMERATE, resolvep);
}

#define XPC_WN_SHARED_PROTO_CLASS_EXT                                  \
    {                                                                  \
        nullptr,    /* outerObject */                                  \
        nullptr,    /* innerObject */                                  \
        false,      /* isWrappedNative */                              \
        nullptr,    /* weakmapKeyDelegateOp */                         \
        XPC_WN_Shared_Proto_ObjectMoved                                \
    }

const js::Class XPC_WN_ModsAllowed_WithCall_Proto_JSClass = {
    "XPC_WN_ModsAllowed_WithCall_Proto_JSClass", 
    WRAPPER_FLAGS, 

    
    nullptr,                        
    nullptr,                        
    nullptr,                        
    nullptr,                        
    XPC_WN_Shared_Proto_Enumerate,  
    XPC_WN_ModsAllowed_Proto_Resolve, 
    nullptr,                        
    nullptr,                        
    XPC_WN_Shared_Proto_Finalize,   

    
    nullptr,                        
    nullptr,                        
    nullptr,                        
    XPC_WN_Shared_Proto_Trace,      

    JS_NULL_CLASS_SPEC,
    XPC_WN_SHARED_PROTO_CLASS_EXT,
    XPC_WN_WithCall_ObjectOps
};

const js::Class XPC_WN_ModsAllowed_NoCall_Proto_JSClass = {
    "XPC_WN_ModsAllowed_NoCall_Proto_JSClass", 
    WRAPPER_FLAGS,                  

    
    nullptr,                        
    nullptr,                        
    nullptr,                        
    nullptr,                        
    XPC_WN_Shared_Proto_Enumerate,  
    XPC_WN_ModsAllowed_Proto_Resolve, 
    nullptr,                        
    nullptr,                        
    XPC_WN_Shared_Proto_Finalize,   

    
    nullptr,                         
    nullptr,                         
    nullptr,                         
    XPC_WN_Shared_Proto_Trace,      

    JS_NULL_CLASS_SPEC,
    XPC_WN_SHARED_PROTO_CLASS_EXT,
    XPC_WN_NoCall_ObjectOps
};



static bool
XPC_WN_OnlyIWrite_Proto_AddPropertyStub(JSContext* cx, HandleObject obj, HandleId id,
                                        HandleValue v)
{
    MOZ_ASSERT(js::GetObjectClass(obj) == &XPC_WN_NoMods_WithCall_Proto_JSClass ||
               js::GetObjectClass(obj) == &XPC_WN_NoMods_NoCall_Proto_JSClass,
               "bad proto");

    XPCWrappedNativeProto* self =
        (XPCWrappedNativeProto*) xpc_GetJSPrivate(obj);
    if (!self)
        return false;

    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid())
        return false;

    
    if (ccx.GetResolveName() == id)
        return true;

    return Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);
}

static bool
XPC_WN_NoMods_Proto_Resolve(JSContext* cx, HandleObject obj, HandleId id, bool* resolvedp)
{
    MOZ_ASSERT(js::GetObjectClass(obj) == &XPC_WN_NoMods_WithCall_Proto_JSClass ||
               js::GetObjectClass(obj) == &XPC_WN_NoMods_NoCall_Proto_JSClass,
               "bad proto");

    XPCWrappedNativeProto* self =
        (XPCWrappedNativeProto*) xpc_GetJSPrivate(obj);
    if (!self)
        return false;

    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid())
        return false;

    XPCNativeScriptableInfo* si = self->GetScriptableInfo();

    return DefinePropertyIfFound(ccx, obj, id,
                                 self->GetSet(), nullptr, nullptr,
                                 self->GetScope(),
                                 true, nullptr, nullptr, si,
                                 JSPROP_READONLY |
                                 JSPROP_PERMANENT |
                                 JSPROP_ENUMERATE, resolvedp);
}

const js::Class XPC_WN_NoMods_WithCall_Proto_JSClass = {
    "XPC_WN_NoMods_WithCall_Proto_JSClass",    
    WRAPPER_FLAGS,                             

    
    XPC_WN_OnlyIWrite_Proto_AddPropertyStub,   
    XPC_WN_CantDeletePropertyStub,             
    nullptr,                                   
    nullptr,                                   
    XPC_WN_Shared_Proto_Enumerate,             
    XPC_WN_NoMods_Proto_Resolve,               
    nullptr,                                   
    nullptr,                                   
    XPC_WN_Shared_Proto_Finalize,              

    
    nullptr,                         
    nullptr,                         
    nullptr,                         
    XPC_WN_Shared_Proto_Trace,      

    JS_NULL_CLASS_SPEC,
    XPC_WN_SHARED_PROTO_CLASS_EXT,
    XPC_WN_WithCall_ObjectOps
};

const js::Class XPC_WN_NoMods_NoCall_Proto_JSClass = {
    "XPC_WN_NoMods_NoCall_Proto_JSClass",      
    WRAPPER_FLAGS,                             

    
    XPC_WN_OnlyIWrite_Proto_AddPropertyStub,   
    XPC_WN_CantDeletePropertyStub,             
    nullptr,                                   
    nullptr,                                   
    XPC_WN_Shared_Proto_Enumerate,             
    XPC_WN_NoMods_Proto_Resolve,               
    nullptr,                                   
    nullptr,                                   
    XPC_WN_Shared_Proto_Finalize,              

    
    nullptr,                         
    nullptr,                         
    nullptr,                         
    XPC_WN_Shared_Proto_Trace,      

    JS_NULL_CLASS_SPEC,
    XPC_WN_SHARED_PROTO_CLASS_EXT,
    XPC_WN_NoCall_ObjectOps
};



static bool
XPC_WN_TearOff_Enumerate(JSContext* cx, HandleObject obj)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    XPCWrappedNativeTearOff* to = ccx.GetTearOff();
    XPCNativeInterface* iface;

    if (!to || nullptr == (iface = to->GetInterface()))
        return Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);

    uint16_t member_count = iface->GetMemberCount();
    for (uint16_t k = 0; k < member_count; k++) {
        if (!xpc_ForcePropertyResolve(cx, obj, iface->GetMemberAt(k)->GetName()))
            return false;
    }

    return true;
}

static bool
XPC_WN_TearOff_Resolve(JSContext* cx, HandleObject obj, HandleId id, bool* resolvedp)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    XPCWrappedNativeTearOff* to = ccx.GetTearOff();
    XPCNativeInterface* iface;

    if (!to || nullptr == (iface = to->GetInterface()))
        return Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);

    return DefinePropertyIfFound(ccx, obj, id, nullptr, iface, nullptr,
                                 wrapper->GetScope(),
                                 true, nullptr, nullptr, nullptr,
                                 JSPROP_READONLY |
                                 JSPROP_PERMANENT |
                                 JSPROP_ENUMERATE, resolvedp);
}

static void
XPC_WN_TearOff_Finalize(js::FreeOp* fop, JSObject* obj)
{
    XPCWrappedNativeTearOff* p = (XPCWrappedNativeTearOff*)
        xpc_GetJSPrivate(obj);
    if (!p)
        return;
    p->JSObjectFinalized();
}

static void
XPC_WN_TearOff_ObjectMoved(JSObject* obj, const JSObject* old)
{
    XPCWrappedNativeTearOff* p = (XPCWrappedNativeTearOff*)
        xpc_GetJSPrivate(obj);
    if (!p)
        return;
    p->JSObjectMoved(obj, old);
}



static_assert(((WRAPPER_FLAGS >> JSCLASS_RESERVED_SLOTS_SHIFT) &
               JSCLASS_RESERVED_SLOTS_MASK) == 0,
              "WRAPPER_FLAGS should not include any reserved slots");

const js::Class XPC_WN_Tearoff_JSClass = {
    "WrappedNative_TearOff",                   
    WRAPPER_FLAGS |
    JSCLASS_HAS_RESERVED_SLOTS(XPC_WN_TEAROFF_RESERVED_SLOTS), 
    XPC_WN_OnlyIWrite_AddPropertyStub,         
    XPC_WN_CantDeletePropertyStub,             
    nullptr,                                   
    nullptr,                                   
    XPC_WN_TearOff_Enumerate,                  
    XPC_WN_TearOff_Resolve,                    
    nullptr,                                   
    XPC_WN_Shared_Convert,                     
    XPC_WN_TearOff_Finalize,                   

    
    nullptr,                                   
    nullptr,                                   
    nullptr,                                   
    nullptr,                                   
    JS_NULL_CLASS_SPEC,

    
    {
        nullptr,                               
        nullptr,                               
        false,                                 
        nullptr,                               
        XPC_WN_TearOff_ObjectMoved
    },
};
