






































#include "XrayWrapper.h"
#include "AccessCheck.h"
#include "FilteringWrapper.h"
#include "CrossOriginWrapper.h"
#include "WrapperFactory.h"

#include "jscntxt.h"

#include "nsINode.h"
#include "nsIDocument.h"

#include "XPCWrapper.h"
#include "xpcprivate.h"

namespace xpc {

using namespace js;

static const uint32 JSSLOT_WN = 0;
static const uint32 JSSLOT_RESOLVING = 1;
static const uint32 JSSLOT_EXPANDO = 2;

class ResolvingId
{
  public:
    ResolvingId(JSObject *holder, jsid id)
      : mId(id),
        mPrev(getResolvingId(holder)),
        mHolder(holder)
    {
        js::SetReservedSlot(holder, JSSLOT_RESOLVING, PrivateValue(this));
    }

    ~ResolvingId() {
        NS_ASSERTION(getResolvingId(mHolder) == this, "unbalanced ResolvingIds");
        js::SetReservedSlot(mHolder, JSSLOT_RESOLVING, PrivateValue(mPrev));
    }

    static ResolvingId *getResolvingId(JSObject *holder) {
        return (ResolvingId *)js::GetReservedSlot(holder, JSSLOT_RESOLVING).toPrivate();
    }

    jsid mId;
    ResolvingId *mPrev;

  private:
    JSObject *mHolder;
};

static bool
IsResolving(JSObject *holder, jsid id)
{
    for (ResolvingId *cur = ResolvingId::getResolvingId(holder); cur; cur = cur->mPrev) {
        if (cur->mId == id)
            return true;
    }

    return false;
}

static JSBool
holder_get(JSContext *cx, JSObject *holder, jsid id, jsval *vp);

static JSBool
holder_set(JSContext *cx, JSObject *holder, jsid id, JSBool strict, jsval *vp);

namespace XrayUtils {

JSClass HolderClass = {
    "NativePropertyHolder",
    JSCLASS_HAS_RESERVED_SLOTS(3),
    JS_PropertyStub,        JS_PropertyStub, holder_get,      holder_set,
    JS_EnumerateStub,       JS_ResolveStub,  JS_ConvertStub,  NULL,
    NULL,                   NULL,            NULL,            NULL,
    NULL,                   NULL,            NULL,            NULL
};

}

using namespace XrayUtils;

static JSObject *
GetHolder(JSObject *obj)
{
    return &js::GetProxyExtra(obj, 0).toObject();
}

static XPCWrappedNative *
GetWrappedNative(JSObject *obj)
{
    NS_ASSERTION(IS_WN_WRAPPER_OBJECT(obj), "expected a wrapped native here");
    return static_cast<XPCWrappedNative *>(js::GetObjectPrivate(obj));
}

static XPCWrappedNative *
GetWrappedNativeFromHolder(JSObject *holder)
{
    NS_ASSERTION(js::GetObjectJSClass(holder) == &HolderClass, "expected a native property holder object");
    return static_cast<XPCWrappedNative *>(js::GetReservedSlot(holder, JSSLOT_WN).toPrivate());
}

static JSObject *
GetWrappedNativeObjectFromHolder(JSObject *holder)
{
    NS_ASSERTION(js::GetObjectJSClass(holder) == &HolderClass, "expected a native property holder object");
    return GetWrappedNativeFromHolder(holder)->GetFlatJSObject();
}

static JSObject *
GetExpandoObject(JSObject *holder)
{
    NS_ASSERTION(js::GetObjectJSClass(holder) == &HolderClass, "expected a native property holder object");
    return js::GetReservedSlot(holder, JSSLOT_EXPANDO).toObjectOrNull();
}

static JSObject *
EnsureExpandoObject(JSContext *cx, JSObject *holder)
{
    NS_ASSERTION(js::GetObjectJSClass(holder) == &HolderClass, "expected a native property holder object");
    JSObject *expando = GetExpandoObject(holder);
    if (expando)
        return expando;
    CompartmentPrivate *priv =
        (CompartmentPrivate *)JS_GetCompartmentPrivate(cx, js::GetObjectCompartment(holder));
    XPCWrappedNative *wn = GetWrappedNativeFromHolder(holder);
    expando = priv->LookupExpandoObject(wn);
    if (!expando) {
        expando = JS_NewObjectWithGivenProto(cx, nsnull, nsnull, js::GetObjectParent(holder));
        if (!expando)
            return NULL;
        
        if (!priv->RegisterExpandoObject(wn, expando)) {
            JS_ReportOutOfMemory(cx);
            return NULL;
        }
        
        nsRefPtr<nsXPCClassInfo> ci;
        CallQueryInterface(wn->Native(), getter_AddRefs(ci));
        if (ci)
            ci->PreserveWrapper(wn->Native());
    }
    js::SetReservedSlot(holder, JSSLOT_EXPANDO, ObjectValue(*expando));
    return expando;
}

static inline JSObject *
FindWrapper(JSObject *wrapper)
{
    while (!js::IsWrapper(wrapper) ||
           !(Wrapper::wrapperHandler(wrapper)->flags() & WrapperFactory::IS_XRAY_WRAPPER_FLAG)) {
        wrapper = js::GetObjectProto(wrapper);
        
    }

    return wrapper;
}




static JSBool
holder_get(JSContext *cx, JSObject *wrapper, jsid id, jsval *vp)
{
    wrapper = FindWrapper(wrapper);

    JSObject *holder = GetHolder(wrapper);

    XPCWrappedNative *wn = GetWrappedNativeFromHolder(holder);
    if (NATIVE_HAS_FLAG(wn, WantGetProperty)) {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, holder))
            return false;
        bool retval = true;
        nsresult rv = wn->GetScriptableCallback()->GetProperty(wn, cx, wrapper, id, vp, &retval);
        if (NS_FAILED(rv) || !retval) {
            if (retval)
                XPCThrower::Throw(rv, cx);
            return false;
        }
    }
    return true;
}

static JSBool
holder_set(JSContext *cx, JSObject *wrapper, jsid id, JSBool strict, jsval *vp)
{
    wrapper = FindWrapper(wrapper);

    JSObject *holder = GetHolder(wrapper);
    if (IsResolving(holder, id)) {
        return true;
    }

    XPCWrappedNative *wn = GetWrappedNativeFromHolder(holder);
    if (NATIVE_HAS_FLAG(wn, WantSetProperty)) {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, holder))
            return false;
        bool retval = true;
        nsresult rv = wn->GetScriptableCallback()->SetProperty(wn, cx, wrapper, id, vp, &retval);
        if (NS_FAILED(rv) || !retval) {
            if (retval)
                XPCThrower::Throw(rv, cx);
            return false;
        }
    }
    return true;
}

static bool
ResolveNativeProperty(JSContext *cx, JSObject *wrapper, JSObject *holder, jsid id, bool set,
                      JSPropertyDescriptor *desc)
{
    desc->obj = NULL;

    NS_ASSERTION(js::GetObjectJSClass(holder) == &HolderClass, "expected a native property holder object");
    JSObject *wnObject = GetWrappedNativeObjectFromHolder(holder);
    XPCWrappedNative *wn = GetWrappedNative(wnObject);

    
    XPCCallContext ccx(JS_CALLER, cx, wnObject, nsnull, id);

    
    
    if (!JSID_IS_ATOM(id)) {
        
        return true;
    }

    XPCNativeInterface *iface;
    XPCNativeMember *member;
    if (ccx.GetWrapper() != wn ||
        !wn->IsValid()  ||
        !(iface = ccx.GetInterface()) ||
        !(member = ccx.GetMember())) {
        
        return true;
    }

    desc->obj = holder;
    desc->attrs = JSPROP_ENUMERATE;
    desc->getter = NULL;
    desc->setter = NULL;
    desc->shortid = 0;
    desc->value = JSVAL_VOID;

    jsval fval = JSVAL_VOID;
    if (member->IsConstant()) {
        if (!member->GetConstantValue(ccx, iface, &desc->value)) {
            JS_ReportError(cx, "Failed to convert constant native property to JS value");
            return false;
        }
    } else if (member->IsAttribute()) {
        
        if (!member->NewFunctionObject(ccx, iface, wrapper, &fval)) {
            JS_ReportError(cx, "Failed to clone function object for native getter/setter");
            return false;
        }

        desc->attrs |= JSPROP_GETTER;
        if (member->IsWritableAttribute())
            desc->attrs |= JSPROP_SETTER;

        
        
        desc->attrs |= JSPROP_SHARED;
    } else {
        
        if (!member->NewFunctionObject(ccx, iface, wrapper, &desc->value)) {
            JS_ReportError(cx, "Failed to clone function object for native function");
            return false;
        }

        
        
        
        
        desc->getter = JS_PropertyStub;
        desc->setter = JS_StrictPropertyStub;
    }

    if (!JS_WrapValue(cx, &desc->value) || !JS_WrapValue(cx, &fval))
        return false;

    if (desc->attrs & JSPROP_GETTER)
        desc->getter = js::CastAsJSPropertyOp(JSVAL_TO_OBJECT(fval));
    if (desc->attrs & JSPROP_SETTER)
        desc->setter = js::CastAsJSStrictPropertyOp(JSVAL_TO_OBJECT(fval));

    
    return JS_DefinePropertyById(cx, holder, id, desc->value,
                                 desc->getter, desc->setter, desc->attrs);
}

static JSBool
wrappedJSObject_getter(JSContext *cx, JSObject *wrapper, jsid id, jsval *vp)
{
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    *vp = OBJECT_TO_JSVAL(wrapper);

    return WrapperFactory::WaiveXrayAndWrap(cx, vp);
}

template <typename T>
static bool
Is(JSObject *wrapper)
{
    JSObject *holder = GetHolder(wrapper);
    XPCWrappedNative *wn = GetWrappedNativeFromHolder(holder);
    nsCOMPtr<T> native = do_QueryWrappedNative(wn);
    return !!native;
}

static JSBool
WrapURI(JSContext *cx, nsIURI *uri, jsval *vp)
{
    JSObject *scope = JS_GetGlobalForScopeChain(cx);
    nsresult rv =
        nsXPConnect::FastGetXPConnect()->WrapNativeToJSVal(cx, scope, uri, nsnull,
                                                           &NS_GET_IID(nsIURI), PR_TRUE,
                                                           vp, nsnull);
    if (NS_FAILED(rv)) {
        XPCThrower::Throw(rv, cx);
        return false;
    }
    return true;
}

static JSBool
documentURIObject_getter(JSContext *cx, JSObject *wrapper, jsid id, jsval *vp)
{
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    JSObject *holder = GetHolder(wrapper);
    XPCWrappedNative *wn = GetWrappedNativeFromHolder(holder);
    nsCOMPtr<nsIDocument> native = do_QueryWrappedNative(wn);
    if (!native) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    nsCOMPtr<nsIURI> uri = native->GetDocumentURI();
    if (!uri) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    return WrapURI(cx, uri, vp);
}

static JSBool
baseURIObject_getter(JSContext *cx, JSObject *wrapper, jsid id, jsval *vp)
{
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    JSObject *holder = GetHolder(wrapper);
    XPCWrappedNative *wn = GetWrappedNativeFromHolder(holder);
    nsCOMPtr<nsINode> native = do_QueryWrappedNative(wn);
    if (!native) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }
    nsCOMPtr<nsIURI> uri = native->GetBaseURI();
    if (!uri) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    return WrapURI(cx, uri, vp);
}

static JSBool
nodePrincipal_getter(JSContext *cx, JSObject *wrapper, jsid id, jsval *vp)
{
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    JSObject *holder = GetHolder(wrapper);
    XPCWrappedNative *wn = GetWrappedNativeFromHolder(holder);
    nsCOMPtr<nsINode> node = do_QueryWrappedNative(wn);
    if (!node) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    JSObject *scope = JS_GetGlobalForScopeChain(cx);
    nsresult rv =
        nsXPConnect::FastGetXPConnect()->WrapNativeToJSVal(cx, scope, node->NodePrincipal(), nsnull,
                                                           &NS_GET_IID(nsIPrincipal), PR_TRUE,
                                                           vp, nsnull);
    if (NS_FAILED(rv)) {
        XPCThrower::Throw(rv, cx);
        return false;
    }
    return true;
}

static JSBool
XrayToString(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *wrapper = JS_THIS_OBJECT(cx, vp);
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "XrayToString called on an incompatible object");
        return false;
    }

    nsAutoString result(NS_LITERAL_STRING("[object XrayWrapper "));
    if (mozilla::dom::binding::instanceIsProxy(&js::GetProxyPrivate(wrapper).toObject())) {
        JSString *wrapperStr = js::GetProxyHandler(wrapper)->obj_toString(cx, wrapper);
        size_t length;
        const jschar* chars = JS_GetStringCharsAndLength(cx, wrapperStr, &length);
        if (!chars) {
            JS_ReportOutOfMemory(cx);
            return false;
        }
        result.Append(chars, length);
    } else {
        JSObject *holder = GetHolder(wrapper);
        XPCWrappedNative *wn = GetWrappedNativeFromHolder(holder);
        JSObject *wrappednative = wn->GetFlatJSObject();

        XPCCallContext ccx(JS_CALLER, cx, wrappednative);
        char *wrapperStr = wn->ToString(ccx);
        if (!wrapperStr) {
            JS_ReportOutOfMemory(cx);
            return false;
        }
        result.AppendASCII(wrapperStr);
        JS_smprintf_free(wrapperStr);
    }

    result.Append(']');

    JSString *str = JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar *>(result.get()),
                                        result.Length());
    if (!str)
        return false;

    *vp = STRING_TO_JSVAL(str);
    return true;
}

template <typename Base>
XrayWrapper<Base>::XrayWrapper(uintN flags)
  : Base(flags | WrapperFactory::IS_XRAY_WRAPPER_FLAG)
{
}

template <typename Base>
XrayWrapper<Base>::~XrayWrapper()
{
}

template <typename Base>
class AutoLeaveHelper
{
  public:
    AutoLeaveHelper(XrayWrapper<Base> &xray, JSContext *cx, JSObject *wrapper)
      : xray(xray), cx(cx), wrapper(wrapper)
    {
    }
    ~AutoLeaveHelper()
    {
        xray.leave(cx, wrapper);
    }

  private:
    XrayWrapper<Base> &xray;
    JSContext *cx;
    JSObject *wrapper;
};

static bool
IsPrivilegedScript()
{
    
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (ssm) {
        bool privileged;
        if (NS_SUCCEEDED(ssm->IsCapabilityEnabled("UniversalXPConnect", &privileged)) && privileged)
            return true;
    }
    return false;
}

namespace XrayUtils {

bool
IsTransparent(JSContext *cx, JSObject *wrapper)
{
    if (WrapperFactory::HasWaiveXrayFlag(wrapper))
        return true;

    if (!WrapperFactory::IsPartiallyTransparent(wrapper))
        return false;

    
    if (IsPrivilegedScript())
        return true;

    return AccessCheck::documentDomainMakesSameOrigin(cx, UnwrapObject(wrapper));
}

}

template <typename Base>
bool
XrayWrapper<Base>::resolveOwnProperty(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                                      PropertyDescriptor *desc)
{
    
    
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    if (!WrapperFactory::IsPartiallyTransparent(wrapper) &&
        (id == rt->GetStringID(XPCJSRuntime::IDX_WRAPPED_JSOBJECT) ||
         
         
         
         
         
         ((((id == rt->GetStringID(XPCJSRuntime::IDX_BASEURIOBJECT) ||
             id == rt->GetStringID(XPCJSRuntime::IDX_NODEPRINCIPAL)) &&
            Is<nsINode>(wrapper)) ||
           (id == rt->GetStringID(XPCJSRuntime::IDX_DOCUMENTURIOBJECT) &&
            Is<nsIDocument>(wrapper))) &&
          IsPrivilegedScript()))) {
        bool status;
        Wrapper::Action action = set ? Wrapper::SET : Wrapper::GET;
        desc->obj = NULL; 
        if (!this->enter(cx, wrapper, id, action, &status))
            return status;

        AutoLeaveHelper<Base> helper(*this, cx, wrapper);

        desc->obj = wrapper;
        desc->attrs = JSPROP_ENUMERATE|JSPROP_SHARED;
        if (id == rt->GetStringID(XPCJSRuntime::IDX_WRAPPED_JSOBJECT))
            desc->getter = wrappedJSObject_getter;
        else if (id == rt->GetStringID(XPCJSRuntime::IDX_BASEURIOBJECT))
            desc->getter = baseURIObject_getter;
        else if (id == rt->GetStringID(XPCJSRuntime::IDX_DOCUMENTURIOBJECT))
            desc->getter = documentURIObject_getter;
        else
            desc->getter = nodePrincipal_getter;
        desc->setter = NULL;
        desc->shortid = 0;
        desc->value = JSVAL_VOID;
        return true;
    }

    desc->obj = NULL;

    uintN flags = (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED;
    JSObject *holder = GetHolder(wrapper);
    JSObject *expando = GetExpandoObject(holder);

    
    if (expando && !JS_GetPropertyDescriptorById(cx, expando, id, flags, desc)) {
        return false;
    }
    if (desc->obj) {
        
        desc->obj = wrapper;
        return true;
    }

    JSBool hasProp;
    if (!JS_HasPropertyById(cx, holder, id, &hasProp)) {
        return false;
    }
    if (!hasProp) {
        XPCWrappedNative *wn = GetWrappedNativeFromHolder(holder);

        
        if (!NATIVE_HAS_FLAG(wn, WantNewResolve)) {
            desc->obj = nsnull;
            return true;
        }

        bool retval = true;
        JSObject *pobj = NULL;
        nsresult rv = wn->GetScriptableInfo()->GetCallback()->NewResolve(wn, cx, wrapper, id,
                                                                         flags, &pobj, &retval);
        if (NS_FAILED(rv)) {
            if (retval)
                XPCThrower::Throw(rv, cx);
            return false;
        }

        if (!pobj) {
            desc->obj = nsnull;
            return true;
        }

#ifdef DEBUG
        NS_ASSERTION(JS_HasPropertyById(cx, holder, id, &hasProp) &&
                     hasProp, "id got defined somewhere else?");
#endif
    }

    if (!JS_GetPropertyDescriptorById(cx, holder, id, flags, desc))
        return false;

    
    desc->obj = wrapper;

    return true;
}

template <typename Base>
bool
XrayWrapper<Base>::getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                         bool set, PropertyDescriptor *desc)
{
    JSObject *holder = GetHolder(wrapper);
    if (IsResolving(holder, id)) {
        desc->obj = NULL;
        return true;
    }

    bool status;
    Wrapper::Action action = set ? Wrapper::SET : Wrapper::GET;
    desc->obj = NULL; 
    if (!this->enter(cx, wrapper, id, action, &status))
        return status;

    AutoLeaveHelper<Base> helper(*this, cx, wrapper);

    ResolvingId resolving(holder, id);

    
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSObject *wnObject = GetWrappedNativeObjectFromHolder(holder);

        {
            JSAutoEnterCompartment ac;
            if (!ac.enter(cx, wnObject))
                return false;

            if (!JS_GetPropertyDescriptorById(cx, wnObject, id,
                                              (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED,
                                              desc))
                return false;
        }

        if (desc->obj)
            desc->obj = wrapper;
        return JS_WrapPropertyDescriptor(cx, desc);
    }

    if (!this->resolveOwnProperty(cx, wrapper, id, set, desc))
        return false;

    if (desc->obj)
        return true;

    bool ok = ResolveNativeProperty(cx, wrapper, holder, id, set, desc);
    if (!ok || desc->obj)
        return ok;

    if (id == nsXPConnect::GetRuntimeInstance()->GetStringID(XPCJSRuntime::IDX_TO_STRING)) {
        desc->obj = wrapper;
        desc->attrs = 0;
        desc->getter = NULL;
        desc->setter = NULL;
        desc->shortid = 0;

        JSObject *toString = JS_GetFunctionObject(JS_NewFunction(cx, XrayToString, 0, 0, holder, "toString"));
        if (!toString)
            return false;
        desc->value = OBJECT_TO_JSVAL(toString);
        return true;
    }

    return true;
}

template <typename Base>
bool
XrayWrapper<Base>::getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                            bool set, PropertyDescriptor *desc)
{
    JSObject *holder = GetHolder(wrapper);
    if (IsResolving(holder, id)) {
        desc->obj = NULL;
        return true;
    }

    bool status;
    Wrapper::Action action = set ? Wrapper::SET : Wrapper::GET;
    desc->obj = NULL; 
    if (!this->enter(cx, wrapper, id, action, &status))
        return status;

    AutoLeaveHelper<Base> helper(*this, cx, wrapper);

    ResolvingId resolving(holder, id);

    
    
    
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSObject *wnObject = GetWrappedNativeObjectFromHolder(holder);

        {
            JSAutoEnterCompartment ac;
            if (!ac.enter(cx, wnObject))
                return false;

            if (!JS_GetPropertyDescriptorById(cx, wnObject, id,
                                              (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED,
                                              desc)) {
                return false;
            }
        }

        desc->obj = (desc->obj == wnObject) ? wrapper : nsnull;
        return JS_WrapPropertyDescriptor(cx, desc);
    }

    return this->resolveOwnProperty(cx, wrapper, id, set, desc);
}

template <typename Base>
bool
XrayWrapper<Base>::defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                  js::PropertyDescriptor *desc)
{
    JSObject *holder = GetHolder(wrapper);
    JSPropertyDescriptor *jsdesc = desc;

    
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSObject *wnObject = GetWrappedNativeObjectFromHolder(holder);

        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, wnObject))
            return false;

        if (!JS_WrapPropertyDescriptor(cx, desc))
            return false;

        return JS_DefinePropertyById(cx, wnObject, id, jsdesc->value, jsdesc->getter, jsdesc->setter,
                                     jsdesc->attrs);
    }

    PropertyDescriptor existing_desc;
    if (!getOwnPropertyDescriptor(cx, wrapper, id, true, &existing_desc))
        return false;

    if (existing_desc.obj && (existing_desc.attrs & JSPROP_PERMANENT))
        return true; 

    if (IsResolving(holder, id)) {
        if (!(jsdesc->attrs & (JSPROP_GETTER | JSPROP_SETTER))) {
            if (!desc->getter)
                jsdesc->getter = holder_get;
            if (!desc->setter)
                jsdesc->setter = holder_set;
        }

        return JS_DefinePropertyById(cx, holder, id, jsdesc->value, jsdesc->getter, jsdesc->setter,
                                     jsdesc->attrs);
    }

    JSObject *expando = EnsureExpandoObject(cx, holder);
    if (!expando)
        return false;

    return JS_DefinePropertyById(cx, expando, id, jsdesc->value, jsdesc->getter, jsdesc->setter,
                                 jsdesc->attrs);
}

static bool
EnumerateNames(JSContext *cx, JSObject *wrapper, uintN flags, js::AutoIdVector &props)
{
    JSObject *holder = GetHolder(wrapper);

    JSObject *wnObject = GetWrappedNativeObjectFromHolder(holder);

    
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, wnObject))
            return false;

        return js::GetPropertyNames(cx, wnObject, flags, &props);
    }

    if (WrapperFactory::IsPartiallyTransparent(wrapper)) {
        JS_ReportError(cx, "Not allowed to enumerate cross origin objects");
        return false;
    }

    
    JSObject *expando = GetExpandoObject(holder);
    if (expando && !js::GetPropertyNames(cx, expando, flags, &props))
        return false;

    
    js::AutoIdVector wnProps(cx);
    {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, wnObject))
            return false;
        if (!js::GetPropertyNames(cx, wnObject, flags, &wnProps))
            return false;
    }

    
    for (size_t n = 0; n < wnProps.length(); ++n) {
        jsid id = wnProps[n];
        JSBool hasProp;
        if (!JS_HasPropertyById(cx, wrapper, id, &hasProp))
            return false;
        if (hasProp)
            props.append(id);
    }
    return true;
}

template <typename Base>
bool
XrayWrapper<Base>::getOwnPropertyNames(JSContext *cx, JSObject *wrapper,
                                       js::AutoIdVector &props)
{
    return EnumerateNames(cx, wrapper, JSITER_OWNONLY | JSITER_HIDDEN, props);
}

template <typename Base>
bool
XrayWrapper<Base>::delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    JSObject *holder = GetHolder(wrapper);
    jsval v;
    JSBool b;

    
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSObject *wnObject = GetWrappedNativeObjectFromHolder(holder);

        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, wnObject))
            return false;

        if (!JS_DeletePropertyById2(cx, wnObject, id, &v) || !JS_ValueToBoolean(cx, v, &b))
            return false;
        *bp = !!b;
        return true;
    }

    JSObject *expando = GetExpandoObject(holder);
    b = true;
    if (expando &&
        (!JS_DeletePropertyById2(cx, expando, id, &v) ||
         !JS_ValueToBoolean(cx, v, &b))) {
        return false;
    }

    *bp = !!b;
    return true;
}

template <typename Base>
bool
XrayWrapper<Base>::enumerate(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props)
{
    return EnumerateNames(cx, wrapper, 0, props);
}

template <typename Base>
bool
XrayWrapper<Base>::fix(JSContext *cx, JSObject *proxy, js::Value *vp)
{
    vp->setUndefined();
    return true;
}

template <typename Base>
bool
XrayWrapper<Base>::get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                       js::Value *vp)
{
    
    
    
    return ProxyHandler::get(cx, wrapper, wrapper, id, vp);
}

template <typename Base>
bool
XrayWrapper<Base>::set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                       bool strict, js::Value *vp)
{
    
    
    
    return ProxyHandler::set(cx, wrapper, wrapper, id, strict, vp);
}

template <typename Base>
bool
XrayWrapper<Base>::has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    
    return ProxyHandler::has(cx, wrapper, id, bp);
}

template <typename Base>
bool
XrayWrapper<Base>::hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    
    return ProxyHandler::hasOwn(cx, wrapper, id, bp);
}

template <typename Base>
bool
XrayWrapper<Base>::keys(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props)
{
    
    return ProxyHandler::keys(cx, wrapper, props);
}

template <typename Base>
bool
XrayWrapper<Base>::iterate(JSContext *cx, JSObject *wrapper, uintN flags, js::Value *vp)
{
    
    return ProxyHandler::iterate(cx, wrapper, flags, vp);
}

template <typename Base>
bool
XrayWrapper<Base>::call(JSContext *cx, JSObject *wrapper, uintN argc, js::Value *vp)
{
    JSObject *holder = GetHolder(wrapper);
    XPCWrappedNative *wn = GetWrappedNativeFromHolder(holder);

    
    if (NATIVE_HAS_FLAG(wn, WantCall)) {
        XPCCallContext ccx(JS_CALLER, cx, wrapper, nsnull, JSID_VOID, argc,
                           vp + 2, vp);
        if (!ccx.IsValid())
            return false;
        bool ok = true;
        nsresult rv = wn->GetScriptableInfo()->GetCallback()->Call(wn, cx, wrapper,
                                                                   argc, vp + 2, vp, &ok);
        if (NS_FAILED(rv)) {
            if (ok)
                XPCThrower::Throw(rv, cx);
            return false;
        }
    }

    return true;
}

template <typename Base>
bool
XrayWrapper<Base>::construct(JSContext *cx, JSObject *wrapper, uintN argc,
                             js::Value *argv, js::Value *rval)
{
    JSObject *holder = GetHolder(wrapper);
    XPCWrappedNative *wn = GetWrappedNativeFromHolder(holder);

    
    if (NATIVE_HAS_FLAG(wn, WantConstruct)) {
        XPCCallContext ccx(JS_CALLER, cx, wrapper, nsnull, JSID_VOID, argc, argv, rval);
        if (!ccx.IsValid())
            return false;
        bool ok = true;
        nsresult rv = wn->GetScriptableInfo()->GetCallback()->Construct(wn, cx, wrapper,
                                                                        argc, argv, rval, &ok);
        if (NS_FAILED(rv)) {
            if (ok)
                XPCThrower::Throw(rv, cx);
            return false;
        }
    }

    return true;
}

template <typename Base>
JSObject *
XrayWrapper<Base>::createHolder(JSContext *cx, JSObject *wrappedNative, JSObject *parent)
{
    JSObject *holder = JS_NewObjectWithGivenProto(cx, &HolderClass, nsnull, parent);
    if (!holder)
        return nsnull;

    CompartmentPrivate *priv =
        (CompartmentPrivate *)JS_GetCompartmentPrivate(cx, js::GetObjectCompartment(holder));
    JSObject *inner = JS_ObjectToInnerObject(cx, wrappedNative);
    XPCWrappedNative *wn = GetWrappedNative(inner);
    Value expando = ObjectOrNullValue(priv->LookupExpandoObject(wn));

    
    
    
    
    
    
    
    
    JS_ASSERT(IS_WN_WRAPPER(wrappedNative) ||
              js::GetObjectClass(wrappedNative)->ext.innerObject);
    js::SetReservedSlot(holder, JSSLOT_WN, PrivateValue(wn));
    js::SetReservedSlot(holder, JSSLOT_RESOLVING, PrivateValue(NULL));
    js::SetReservedSlot(holder, JSSLOT_EXPANDO, expando);
    return holder;
}

XrayProxy::XrayProxy(uintN flags)
  : XrayWrapper<CrossCompartmentWrapper>(flags)
{
}

XrayProxy::~XrayProxy()
{
}

static JSObject *
GetHolderObject(JSContext *cx, JSObject *wrapper, bool createHolder = true)
{
    if (!js::GetProxyExtra(wrapper, 0).isUndefined())
        return &js::GetProxyExtra(wrapper, 0).toObject();

    JSObject *obj = JS_NewObjectWithGivenProto(cx, nsnull, nsnull, js::GetObjectGlobal(wrapper));
    if (!obj)
        return nsnull;
    js::SetProxyExtra(wrapper, 0, ObjectValue(*obj));
    return obj;
}

bool
XrayProxy::getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                 bool set, js::PropertyDescriptor *desc)
{
    JSObject *holder = GetHolderObject(cx, wrapper);
    if (!holder)
        return false;

    bool status;
    Wrapper::Action action = set ? Wrapper::SET : Wrapper::GET;
    desc->obj = NULL; 
    if (!this->enter(cx, wrapper, id, action, &status))
        return status;

    AutoLeaveHelper<CrossCompartmentWrapper> helper(*this, cx, wrapper);

    JSObject *obj = &js::GetProxyPrivate(wrapper).toObject();
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        {
            JSAutoEnterCompartment ac;
            if (!ac.enter(cx, obj))
                return false;
            if (!JS_GetPropertyDescriptorById(cx, obj, id,
                                              (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED,
                                              desc))
                return false;
        }

        if (desc->obj)
            desc->obj = wrapper;
        return JS_WrapPropertyDescriptor(cx, desc);
    }

    if (!JS_GetPropertyDescriptorById(cx, holder, id, JSRESOLVE_QUALIFIED, desc))
        return false;
    if (desc->obj) {
        desc->obj = wrapper;
        return true;
    }

    if (!js::GetProxyHandler(obj)->getOwnPropertyDescriptor(cx, wrapper, id, set, desc))
        return false;
    if (desc->obj) {
        desc->obj = wrapper;
        return true;
    }

    if (!js::GetProxyHandler(obj)->getPropertyDescriptor(cx, wrapper, id, set, desc))
        return false;

    if (!desc->obj) {
        if (id != nsXPConnect::GetRuntimeInstance()->GetStringID(XPCJSRuntime::IDX_TO_STRING))
            return true;

        JSFunction *toString = JS_NewFunction(cx, XrayToString, 0, 0, holder, "toString");
        if (!toString)
            return false;

        desc->attrs = 0;
        desc->getter = NULL;
        desc->setter = NULL;
        desc->shortid = 0;
        desc->value.setObject(*JS_GetFunctionObject(toString));
    }

    desc->obj = wrapper;

    return JS_DefinePropertyById(cx, holder, id, desc->value, desc->getter, desc->setter,
                                 desc->attrs);
}

bool
XrayProxy::getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                    bool set, js::PropertyDescriptor *desc)
{
    JSObject *holder = GetHolderObject(cx, wrapper);
    if (!holder)
        return false;

    bool status;
    Wrapper::Action action = set ? Wrapper::SET : Wrapper::GET;
    desc->obj = NULL; 
    if (!this->enter(cx, wrapper, id, action, &status))
        return status;

    AutoLeaveHelper<CrossCompartmentWrapper> helper(*this, cx, wrapper);

    JSObject *obj = &js::GetProxyPrivate(wrapper).toObject();
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        {
            JSAutoEnterCompartment ac;
            if (!ac.enter(cx, obj))
                return false;
            if (!JS_GetPropertyDescriptorById(cx, obj, id,
                                              (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED,
                                              desc))
                return false;
        }

        if (desc->obj)
            desc->obj = desc->obj == obj ? wrapper : NULL;
        return JS_WrapPropertyDescriptor(cx, desc);
    }

    if (!JS_GetPropertyDescriptorById(cx, holder, id, JSRESOLVE_QUALIFIED, desc))
        return false;
    if (desc->obj) {
        desc->obj = wrapper;
        return true;
    }

    if (!js::GetProxyHandler(obj)->getOwnPropertyDescriptor(cx, wrapper, id, set, desc))
        return false;

    desc->obj = wrapper;

    return JS_DefinePropertyById(cx, holder, id, desc->value, desc->getter, desc->setter,
                                 desc->attrs);
}

bool
XrayProxy::defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                          js::PropertyDescriptor *desc)
{
    JSObject *holder = GetHolderObject(cx, wrapper);
    if (!holder)
        return false;

    JSObject *obj = &js::GetProxyPrivate(wrapper).toObject();
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, obj) || !JS_WrapPropertyDescriptor(cx, desc))
            return false;

        return JS_DefinePropertyById(cx, obj, id, desc->value, desc->getter, desc->setter,
                                     desc->attrs);
    }

    PropertyDescriptor existing_desc;
    if (!getOwnPropertyDescriptor(cx, wrapper, id, true, &existing_desc))
        return false;

    if (existing_desc.obj && (existing_desc.attrs & JSPROP_PERMANENT))
        return true; 

    return JS_DefinePropertyById(cx, holder, id, desc->value, desc->getter, desc->setter,
                                 desc->attrs);
}

static bool
EnumerateProxyNames(JSContext *cx, JSObject *wrapper, uintN flags, js::AutoIdVector &props)
{
    JSObject *obj = &js::GetProxyPrivate(wrapper).toObject();

    
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, obj))
            return false;

        return js::GetPropertyNames(cx, obj, flags, &props);
    }

    if (WrapperFactory::IsPartiallyTransparent(wrapper)) {
        JS_ReportError(cx, "Not allowed to enumerate cross origin objects");
        return false;
    }

    if (flags & (JSITER_OWNONLY | JSITER_HIDDEN))
        return js::GetProxyHandler(obj)->getOwnPropertyNames(cx, wrapper, props);

    return js::GetProxyHandler(obj)->enumerate(cx, wrapper, props);
}

bool
XrayProxy::getOwnPropertyNames(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props)
{
    return EnumerateProxyNames(cx, wrapper, JSITER_OWNONLY | JSITER_HIDDEN, props);
}

bool
XrayProxy::delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    JSObject *obj = &js::GetProxyPrivate(wrapper).toObject();
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, obj))
            return false;

        JSBool b;
        jsval v;
        if (!JS_DeletePropertyById2(cx, obj, id, &v) || !JS_ValueToBoolean(cx, v, &b))
            return false;

        *bp = b;

        return true;
    }

    if (!js::GetProxyHandler(obj)->delete_(cx, wrapper, id, bp))
        return false;

    JSObject *holder;
    if (*bp && (holder = GetHolderObject(cx, wrapper, false)))
        JS_DeletePropertyById(cx, holder, id);

    return true;
}

bool
XrayProxy::enumerate(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props)
{
    return EnumerateProxyNames(cx, wrapper, 0, props);
}

XrayProxy
XrayProxy::singleton(0);


#define XRAY XrayWrapper<CrossCompartmentSecurityWrapper>
template <> XRAY XRAY::singleton(0);
template class XRAY;
#undef XRAY

#define XRAY XrayWrapper<SameCompartmentSecurityWrapper>
template <> XRAY XRAY::singleton(0);
template class XRAY;
#undef XRAY

#define XRAY XrayWrapper<CrossCompartmentWrapper>
template <> XRAY XRAY::singleton(0);
template class XRAY;
#undef XRAY

}
