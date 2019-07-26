






#include "XrayWrapper.h"
#include "AccessCheck.h"
#include "FilteringWrapper.h"
#include "WaiveXrayWrapper.h"
#include "WrapperFactory.h"

#include "nsINode.h"
#include "nsIDocument.h"

#include "XPCWrapper.h"
#include "xpcprivate.h"

#include "jsapi.h"
#include "nsJSUtils.h"

#include "mozilla/dom/BindingUtils.h"

using namespace mozilla::dom;

namespace xpc {

using namespace js;

static const uint32_t JSSLOT_RESOLVING = 0;

static XPCWrappedNative *GetWrappedNative(JSObject *obj);

namespace XrayUtils {

JSClass HolderClass = {
    "NativePropertyHolder",
    JSCLASS_HAS_RESERVED_SLOTS(2),
    JS_PropertyStub,        JS_PropertyStub, holder_get,      holder_set,
    JS_EnumerateStub,       JS_ResolveStub,  JS_ConvertStub
};
















enum ExpandoSlots {
    JSSLOT_EXPANDO_NEXT = 0,
    JSSLOT_EXPANDO_ORIGIN,
    JSSLOT_EXPANDO_EXCLUSIVE_GLOBAL,
    JSSLOT_EXPANDO_COUNT
};

static nsIPrincipal*
ObjectPrincipal(JSObject *obj)
{
    return GetCompartmentPrincipal(js::GetObjectCompartment(obj));
}

static nsIPrincipal*
GetExpandoObjectPrincipal(JSObject *expandoObject)
{
    JS::Value v = JS_GetReservedSlot(expandoObject, JSSLOT_EXPANDO_ORIGIN);
    return static_cast<nsIPrincipal*>(v.toPrivate());
}

static void
ExpandoObjectFinalize(JSFreeOp *fop, JSObject *obj)
{
    
    nsIPrincipal *principal = GetExpandoObjectPrincipal(obj);
    NS_RELEASE(principal);
}

JSClass ExpandoObjectClass = {
    "XrayExpandoObject",
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_EXPANDO_COUNT),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, ExpandoObjectFinalize
};

bool
ExpandoObjectMatchesConsumer(JSContext *cx,
                             JSObject *expandoObject,
                             nsIPrincipal *consumerOrigin,
                             JSObject *exclusiveGlobal)
{
    MOZ_ASSERT(js::IsObjectInContextCompartment(expandoObject, cx));

    
    nsIPrincipal *o = GetExpandoObjectPrincipal(expandoObject);
    bool equal;
    
    
    
    
    
    
    
    nsresult rv = consumerOrigin->EqualsIgnoringDomain(o, &equal);
    if (NS_FAILED(rv) || !equal)
        return false;

    
    JSObject *owner = JS_GetReservedSlot(expandoObject,
                                         JSSLOT_EXPANDO_EXCLUSIVE_GLOBAL)
                                        .toObjectOrNull();
    if (!owner && !exclusiveGlobal)
        return true;

    
    MOZ_ASSERT(!exclusiveGlobal || js::IsObjectInContextCompartment(exclusiveGlobal, cx));
    MOZ_ASSERT(!owner || js::IsObjectInContextCompartment(owner, cx));
    return owner == exclusiveGlobal;
}

JSObject *
LookupExpandoObject(JSContext *cx, JSObject *target, nsIPrincipal *origin,
                    JSObject *exclusiveGlobal)
{
    
    
    JSAutoCompartment ac(cx, target);
    if (!JS_WrapObject(cx, &exclusiveGlobal))
        return NULL;

    
    JSObject *head = GetExpandoChain(target);
    while (head) {
        if (ExpandoObjectMatchesConsumer(cx, head, origin, exclusiveGlobal))
            return head;
        head = JS_GetReservedSlot(head, JSSLOT_EXPANDO_NEXT).toObjectOrNull();
    }

    
    return nullptr;
}


JSObject *
LookupExpandoObject(JSContext *cx, JSObject *target, JSObject *consumer)
{
    JSObject *consumerGlobal = js::GetGlobalForObjectCrossCompartment(consumer);
    bool isSandbox = !strcmp(js::GetObjectJSClass(consumerGlobal)->name, "Sandbox");
    return LookupExpandoObject(cx, target, ObjectPrincipal(consumer),
                               isSandbox ? consumerGlobal : nullptr);
}

JSObject *
AttachExpandoObject(JSContext *cx, JSObject *target, nsIPrincipal *origin,
                    JSObject *exclusiveGlobal)
{
    
    MOZ_ASSERT(js::IsObjectInContextCompartment(target, cx));
    MOZ_ASSERT(!exclusiveGlobal || js::IsObjectInContextCompartment(exclusiveGlobal, cx));

    
    MOZ_ASSERT(IS_WN_WRAPPER(target));

    
    MOZ_ASSERT(!LookupExpandoObject(cx, target, origin, exclusiveGlobal));

    
    JSObject *expandoObject = JS_NewObjectWithGivenProto(cx, &ExpandoObjectClass,
                                                         nullptr, target);
    if (!expandoObject)
        return nullptr;

    
    NS_ADDREF(origin);
    JS_SetReservedSlot(expandoObject, JSSLOT_EXPANDO_ORIGIN, PRIVATE_TO_JSVAL(origin));

    
    JS_SetReservedSlot(expandoObject, JSSLOT_EXPANDO_EXCLUSIVE_GLOBAL,
                       OBJECT_TO_JSVAL(exclusiveGlobal));

    
    
    
    JSObject *chain = GetExpandoChain(target);
    if (!chain) {
        XPCWrappedNative *wn =
          static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(target));
        nsRefPtr<nsXPCClassInfo> ci;
        CallQueryInterface(wn->Native(), getter_AddRefs(ci));
        if (ci)
            ci->PreserveWrapper(wn->Native());
    }

    
    JS_SetReservedSlot(expandoObject, JSSLOT_EXPANDO_NEXT, OBJECT_TO_JSVAL(chain));
    SetExpandoChain(target, expandoObject);

    return expandoObject;
}

JSObject *
EnsureExpandoObject(JSContext *cx, JSObject *wrapper, JSObject *target)
{
    
    JSAutoCompartment ac(cx, target);
    JSObject *expandoObject = LookupExpandoObject(cx, target, wrapper);
    if (!expandoObject) {
        
        
        
        
        
        JSObject *consumerGlobal = js::GetGlobalForObjectCrossCompartment(wrapper);
        bool isSandbox = !strcmp(js::GetObjectJSClass(consumerGlobal)->name, "Sandbox");
        if (!JS_WrapObject(cx, &consumerGlobal))
            return NULL;
        expandoObject = AttachExpandoObject(cx, target, ObjectPrincipal(wrapper),
                                            isSandbox ? consumerGlobal : nullptr);
    }
    return expandoObject;
}

bool
CloneExpandoChain(JSContext *cx, JSObject *dst, JSObject *src)
{
    MOZ_ASSERT(js::IsObjectInContextCompartment(dst, cx));
    MOZ_ASSERT(GetExpandoChain(dst) == nullptr);

    JSObject *oldHead = GetExpandoChain(src);
    while (oldHead) {
        JSObject *exclusive = JS_GetReservedSlot(oldHead,
                                                 JSSLOT_EXPANDO_EXCLUSIVE_GLOBAL)
                                                .toObjectOrNull();
        if (!JS_WrapObject(cx, &exclusive))
            return false;
        JSObject *newHead =
          AttachExpandoObject(cx, dst, GetExpandoObjectPrincipal(oldHead), exclusive);
        if (!JS_CopyPropertiesFrom(cx, newHead, oldHead))
            return false;
        oldHead = JS_GetReservedSlot(oldHead, JSSLOT_EXPANDO_NEXT).toObjectOrNull();
    }
    return true;
}

JSObject *
createHolder(JSContext *cx, JSObject *wrapper)
{
    JSObject *global = JS_GetGlobalForObject(cx, wrapper);
    JSObject *holder = JS_NewObjectWithGivenProto(cx, &HolderClass, nullptr,
                                                  global);
    if (!holder)
        return nullptr;

    js::SetReservedSlot(holder, JSSLOT_RESOLVING, PrivateValue(NULL));
    return holder;
}

}

using namespace XrayUtils;

ResolvingId::ResolvingId(JSObject *wrapper, jsid id)
    : mId(id),
    mHolder(getHolderObject(wrapper)),
    mPrev(getResolvingId(mHolder)),
    mXrayShadowing(false)
{
    js::SetReservedSlot(mHolder, JSSLOT_RESOLVING, js::PrivateValue(this));
}

ResolvingId::~ResolvingId()
{
    MOZ_ASSERT(getResolvingId(mHolder) == this, "unbalanced ResolvingIds");
    js::SetReservedSlot(mHolder, JSSLOT_RESOLVING, js::PrivateValue(mPrev));
}

bool
ResolvingId::isXrayShadowing(jsid id)
{
    if (!mXrayShadowing)
        return false;

    return mId == id;
}

bool
ResolvingId::isResolving(jsid id)
{
    for (ResolvingId *cur = this; cur; cur = cur->mPrev) {
        if (cur->mId == id)
            return true;
    }

    return false;
}

ResolvingId *
ResolvingId::getResolvingId(JSObject *holder)
{
    MOZ_ASSERT(strcmp(JS_GetClass(holder)->name, "NativePropertyHolder") == 0);
    return (ResolvingId *)js::GetReservedSlot(holder, JSSLOT_RESOLVING).toPrivate();
}

JSObject *
ResolvingId::getHolderObject(JSObject *wrapper)
{
    return &js::GetProxyExtra(wrapper, 0).toObject();
}

ResolvingId *
ResolvingId::getResolvingIdFromWrapper(JSObject *wrapper)
{
    return getResolvingId(getHolderObject(wrapper));
}

class ResolvingIdDummy
{
public:
    ResolvingIdDummy(JSObject *wrapper, jsid id)
    {
    }
};

class XrayTraits
{
public:
    static JSObject* getTargetObject(JSObject *wrapper) {
        return js::UnwrapObject(wrapper,  false);
    }


    static bool call(JSContext *cx, JSObject *wrapper, unsigned argc, Value *vp)
    {
        MOZ_NOT_REACHED("Call trap currently implemented only for XPCWNs");
    }
    static bool construct(JSContext *cx, JSObject *wrapper, unsigned argc,
                          Value *argv, Value *rval)
    {
        MOZ_NOT_REACHED("Call trap currently implemented only for XPCWNs");
    }
};

class XPCWrappedNativeXrayTraits : public XrayTraits
{
public:
    static bool resolveNativeProperty(JSContext *cx, JSObject *wrapper, JSObject *holder, jsid id,
                                      bool set, JSPropertyDescriptor *desc);
    static bool resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper, JSObject *wrapper,
                                   JSObject *holder, jsid id, bool set,
                                   JSPropertyDescriptor *desc);
    static bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                               JSPropertyDescriptor *desc);
    static bool delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    static bool enumerateNames(JSContext *cx, JSObject *wrapper, unsigned flags,
                               JS::AutoIdVector &props);
    static bool call(JSContext *cx, JSObject *wrapper, unsigned argc, Value *vp);
    static bool construct(JSContext *cx, JSObject *wrapper, unsigned argc,
                          Value *argv, Value *rval);

    static JSObject* getHolderObject(JSContext *cx, JSObject *wrapper)
    {
        return getHolderObject(wrapper);
    }

    static bool isResolving(JSContext *cx, JSObject *holder, jsid id);

    static bool resolveDOMCollectionProperty(JSContext *cx, JSObject *wrapper, JSObject *holder,
                                             jsid id, bool set, PropertyDescriptor *desc);

    static XPCWrappedNative* getWN(JSObject *wrapper) {
        return GetWrappedNative(getTargetObject(wrapper));
    }

    typedef ResolvingId ResolvingIdImpl;

    static XPCWrappedNativeXrayTraits singleton;

private:
    static JSObject* getHolderObject(JSObject *wrapper)
    {
        return &js::GetProxyExtra(wrapper, 0).toObject();
    }
};

class ProxyXrayTraits : public XrayTraits
{
public:
    static bool resolveNativeProperty(JSContext *cx, JSObject *wrapper, JSObject *holder, jsid id,
                                      bool set, JSPropertyDescriptor *desc);
    static bool resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper, JSObject *wrapper,
                                   JSObject *holder, jsid id, bool set,
                                   JSPropertyDescriptor *desc);
    static bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                               JSPropertyDescriptor *desc);
    static bool delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    static bool enumerateNames(JSContext *cx, JSObject *wrapper, unsigned flags,
                               JS::AutoIdVector &props);
    static JSObject* getHolderObject(JSContext *cx, JSObject *wrapper)
    {
        return getHolderObject(cx, wrapper, true);
    }

    static bool isResolving(JSContext *cx, JSObject *holder, jsid id)
    {
        return false;
    }

    typedef ResolvingIdDummy ResolvingIdImpl;

    static ProxyXrayTraits singleton;

private:
    static JSObject* getHolderObject(JSContext *cx, JSObject *wrapper,
                                     bool createHolder)
    {
        if (!js::GetProxyExtra(wrapper, 0).isUndefined())
            return &js::GetProxyExtra(wrapper, 0).toObject();

        if (!createHolder)
            return nullptr;

        return createHolderObject(cx, wrapper);
    }
    static JSObject* createHolderObject(JSContext *cx, JSObject *wrapper);
};

class DOMXrayTraits : public XrayTraits
{
public:
    static bool resolveNativeProperty(JSContext *cx, JSObject *wrapper, JSObject *holder, jsid id,
                                      bool set, JSPropertyDescriptor *desc);
    static bool resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper, JSObject *wrapper,
                                   JSObject *holder, jsid id, bool set,
                                   JSPropertyDescriptor *desc);
    static bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                               JSPropertyDescriptor *desc);
    static bool delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    static bool enumerateNames(JSContext *cx, JSObject *wrapper, unsigned flags,
                               JS::AutoIdVector &props);
    static JSObject* getHolderObject(JSContext *cx, JSObject *wrapper)
    {
        return getHolderObject(cx, wrapper, true);
    }

    static bool isResolving(JSContext *cx, JSObject *holder, jsid id)
    {
        return false;
    }

    typedef ResolvingIdDummy ResolvingIdImpl;

    static DOMXrayTraits singleton;

private:
    static JSObject* getHolderObject(JSContext *cx, JSObject *wrapper,
                                     bool createHolder)
    {
        if (!js::GetProxyExtra(wrapper, 0).isUndefined())
            return &js::GetProxyExtra(wrapper, 0).toObject();

        if (!createHolder)
            return nullptr;

        return createHolderObject(cx, wrapper);
    }
    static JSObject* createHolderObject(JSContext *cx, JSObject *wrapper);
};

XPCWrappedNativeXrayTraits XPCWrappedNativeXrayTraits::singleton;
ProxyXrayTraits ProxyXrayTraits::singleton;
DOMXrayTraits DOMXrayTraits::singleton;

static JSObject *
GetHolder(JSObject *obj)
{
    return &js::GetProxyExtra(obj, 0).toObject();
}

static XPCWrappedNative *
GetWrappedNative(JSObject *obj)
{
    MOZ_ASSERT(IS_WN_WRAPPER_OBJECT(obj));
    return static_cast<XPCWrappedNative *>(js::GetObjectPrivate(obj));
}

static inline JSObject *
FindWrapper(JSContext *cx, JSObject *wrapper)
{
    while (!js::IsWrapper(wrapper) ||
           !(Wrapper::wrapperHandler(wrapper)->flags() &
             WrapperFactory::IS_XRAY_WRAPPER_FLAG)) {
        if (js::IsWrapper(wrapper) &&
            js::GetProxyHandler(wrapper) == &sandboxProxyHandler) {
            wrapper = SandboxProxyHandler::wrappedObject(wrapper);
        } else {
            if (!js::GetObjectProto(cx, wrapper, &wrapper))
                return nullptr;
        }
        
    }

    return wrapper;
}

bool
XPCWrappedNativeXrayTraits::isResolving(JSContext *cx, JSObject *holder,
                                        jsid id)
{
    return ResolvingId::getResolvingId(holder)->isResolving(id);
}




JSBool
holder_get(JSContext *cx, JSHandleObject wrapper_, JSHandleId id, JSMutableHandleValue vp)
{
    JSObject *wrapper = FindWrapper(cx, wrapper_);
    if (!wrapper)
        return false;

    JSObject *holder = GetHolder(wrapper);

    XPCWrappedNative *wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
    if (NATIVE_HAS_FLAG(wn, WantGetProperty)) {
        JSAutoCompartment ac(cx, holder);
        bool retval = true;
        nsresult rv = wn->GetScriptableCallback()->GetProperty(wn, cx, wrapper,
                                                               id, vp.address(), &retval);
        if (NS_FAILED(rv) || !retval) {
            if (retval)
                XPCThrower::Throw(rv, cx);
            return false;
        }
    }
    return true;
}

JSBool
holder_set(JSContext *cx, JSHandleObject wrapper_, JSHandleId id, JSBool strict, JSMutableHandleValue vp)
{
    JSObject *wrapper = FindWrapper(cx, wrapper_);
    if (!wrapper)
        return false;

    JSObject *holder = GetHolder(wrapper);
    if (XPCWrappedNativeXrayTraits::isResolving(cx, holder, id)) {
        return true;
    }

    XPCWrappedNative *wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
    if (NATIVE_HAS_FLAG(wn, WantSetProperty)) {
        JSAutoCompartment ac(cx, holder);
        bool retval = true;
        nsresult rv = wn->GetScriptableCallback()->SetProperty(wn, cx, wrapper,
                                                               id, vp.address(), &retval);
        if (NS_FAILED(rv) || !retval) {
            if (retval)
                XPCThrower::Throw(rv, cx);
            return false;
        }
    }
    return true;
}

class AutoSetWrapperNotShadowing
{
public:
    AutoSetWrapperNotShadowing(JSObject *wrapper MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        MOZ_ASSERT(wrapper);
        mResolvingId = ResolvingId::getResolvingIdFromWrapper(wrapper);
        MOZ_ASSERT(mResolvingId);
        mResolvingId->mXrayShadowing = true;
    }

    ~AutoSetWrapperNotShadowing()
    {
        mResolvingId->mXrayShadowing = false;
    }

private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
    ResolvingId *mResolvingId;
};





bool
XPCWrappedNativeXrayTraits::resolveDOMCollectionProperty(JSContext *cx, JSObject *wrapper,
                                                         JSObject *holder, jsid id, bool set,
                                                         PropertyDescriptor *desc)
{
    
    
    ResolvingId *rid = ResolvingId::getResolvingId(holder);
    if (!rid || rid->mId != id)
        return true;

    XPCWrappedNative *wn = getWN(wrapper);
    if (!NATIVE_HAS_FLAG(wn, WantNewResolve))
        return true;

    
    
    AutoSetWrapperNotShadowing asw(wrapper);

    bool retval = true;
    JSObject *pobj = NULL;
    unsigned flags = (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED;
    nsresult rv = wn->GetScriptableInfo()->GetCallback()->NewResolve(wn, cx, wrapper, id,
                                                                     flags, &pobj, &retval);
    if (NS_FAILED(rv)) {
        if (retval)
            XPCThrower::Throw(rv, cx);
        return false;
    }

    if (pobj && !JS_GetPropertyDescriptorById(cx, holder, id,
                                              JSRESOLVE_QUALIFIED, desc))
    {
        return false;
    }

    return true;
}

template <typename T>
static bool
Is(JSObject *wrapper)
{
    XPCWrappedNative *wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
    nsCOMPtr<T> native = do_QueryWrappedNative(wn);
    return !!native;
}






static JSBool
mozMatchesSelectorStub(JSContext *cx, unsigned argc, jsval *vp)
{
    if (argc < 1) {
        JS_ReportError(cx, "Not enough arguments");
        return false;
    }

    JSObject *wrapper = JS_THIS_OBJECT(cx, vp);
    JSString *selector = JS_ValueToString(cx, JS_ARGV(cx, vp)[0]);
    nsDependentJSString selectorStr;
    NS_ENSURE_TRUE(selectorStr.init(cx, selector), false);

    nsCOMPtr<nsIDOMElement> element;
    if (IsWrapper(wrapper) && WrapperFactory::IsXrayWrapper(wrapper)) {       
        
        XPCWrappedNative *wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
        element = do_QueryWrappedNative(wn);
    } else {
        
        nsCOMPtr<nsIXPConnectWrappedNative> iwn;  
        nsIXPConnect *xpc = nsXPConnect::GetXPConnect();
        nsresult rv = xpc->GetWrappedNativeOfJSObject(cx, wrapper, 
                                                      getter_AddRefs(iwn));
        if (NS_FAILED(rv) || !iwn) {
            JS_ReportError(cx, "Unexpected object");
            return false;
        }
        element = do_QueryWrappedNative(iwn);
    }

    if (!element) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    bool ret;
    nsresult rv = element->MozMatchesSelector(selectorStr, &ret);
    if (NS_FAILED(rv)) {
        XPCThrower::Throw(rv, cx);
        return false;
    }

    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(ret));
    return true;
}

bool
XPCWrappedNativeXrayTraits::resolveNativeProperty(JSContext *cx, JSObject *wrapper,
                                                  JSObject *holder, jsid id, bool set,
                                                  JSPropertyDescriptor *desc)
{
    MOZ_ASSERT(js::GetObjectJSClass(holder) == &HolderClass);
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    if (!set &&
        id == rt->GetStringID(XPCJSRuntime::IDX_MOZMATCHESSELECTOR) &&
        Is<nsIDOMElement>(wrapper))
    {
        
        
        
        desc->obj = wrapper;
        desc->attrs = JSPROP_ENUMERATE;
        JSObject *proto;
        if (!JS_GetPrototype(cx, wrapper, &proto))
            return false;
        JSFunction *fun = JS_NewFunction(cx, mozMatchesSelectorStub, 
                                         1, 0, proto, 
                                         "mozMatchesSelector");
        NS_ENSURE_TRUE(fun, false);
        desc->value = OBJECT_TO_JSVAL(JS_GetFunctionObject(fun));
        desc->getter = NULL;
        desc->setter = NULL;
        desc->shortid = 0;
        return true;
    }

    desc->obj = NULL;

    
    XPCCallContext ccx(JS_CALLER, cx, getTargetObject(wrapper), nullptr, id);

    
    
    
    
    if (!JSID_IS_STRING(id)) {
        
        return resolveDOMCollectionProperty(cx, wrapper, holder, id, set, desc);
    }

    XPCNativeInterface *iface;
    XPCNativeMember *member;
    XPCWrappedNative *wn = getWN(wrapper);
    if (ccx.GetWrapper() != wn ||
        !wn->IsValid()  ||
        !(iface = ccx.GetInterface()) ||
        !(member = ccx.GetMember())) {
        
        return resolveDOMCollectionProperty(cx, wrapper, holder, id, set, desc);
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
wrappedJSObject_getter(JSContext *cx, JSHandleObject wrapper, JSHandleId id, JSMutableHandleValue vp)
{
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    vp.set(OBJECT_TO_JSVAL(wrapper));

    return WrapperFactory::WaiveXrayAndWrap(cx, vp.address());
}

static JSBool
WrapURI(JSContext *cx, nsIURI *uri, jsval *vp)
{
    JSObject *scope = JS_GetGlobalForScopeChain(cx);
    nsresult rv =
        nsXPConnect::FastGetXPConnect()->WrapNativeToJSVal(cx, scope, uri, nullptr,
                                                           &NS_GET_IID(nsIURI), true,
                                                           vp, nullptr);
    if (NS_FAILED(rv)) {
        XPCThrower::Throw(rv, cx);
        return false;
    }
    return true;
}

static JSBool
documentURIObject_getter(JSContext *cx, JSHandleObject wrapper, JSHandleId id, JSMutableHandleValue vp)
{
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    XPCWrappedNative *wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
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

    return WrapURI(cx, uri, vp.address());
}

static JSBool
baseURIObject_getter(JSContext *cx, JSHandleObject wrapper, JSHandleId id, JSMutableHandleValue vp)
{
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    XPCWrappedNative *wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
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

    return WrapURI(cx, uri, vp.address());
}

static JSBool
nodePrincipal_getter(JSContext *cx, JSHandleObject wrapper, JSHandleId id, JSMutableHandleValue vp)
{
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    XPCWrappedNative *wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
    nsCOMPtr<nsINode> node = do_QueryWrappedNative(wn);
    if (!node) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    JSObject *scope = JS_GetGlobalForScopeChain(cx);
    nsresult rv =
        nsXPConnect::FastGetXPConnect()->WrapNativeToJSVal(cx, scope, node->NodePrincipal(), nullptr,
                                                           &NS_GET_IID(nsIPrincipal), true,
                                                           vp.address(), nullptr);
    if (NS_FAILED(rv)) {
        XPCThrower::Throw(rv, cx);
        return false;
    }
    return true;
}

bool
XPCWrappedNativeXrayTraits::resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper,
                                               JSObject *wrapper, JSObject *holder, jsid id,
                                               bool set, PropertyDescriptor *desc)
{
    
    
    MOZ_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    if (AccessCheck::isChrome(wrapper) &&
        (((id == rt->GetStringID(XPCJSRuntime::IDX_BASEURIOBJECT) ||
           id == rt->GetStringID(XPCJSRuntime::IDX_NODEPRINCIPAL)) &&
          Is<nsINode>(wrapper)) ||
          (id == rt->GetStringID(XPCJSRuntime::IDX_DOCUMENTURIOBJECT) &&
          Is<nsIDocument>(wrapper)))) {
        bool status;
        Wrapper::Action action = set ? Wrapper::SET : Wrapper::GET;
        desc->obj = NULL; 
        if (!jsWrapper.enter(cx, wrapper, id, action, &status))
            return status;

        desc->obj = wrapper;
        desc->attrs = JSPROP_ENUMERATE|JSPROP_SHARED;
        if (id == rt->GetStringID(XPCJSRuntime::IDX_BASEURIOBJECT))
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

    unsigned flags = (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED;
    JSObject *target = getTargetObject(wrapper);
    JSObject *expando = LookupExpandoObject(cx, target, wrapper);

    
    
    if (expando) {
        JSAutoCompartment ac(cx, expando);
        if (!JS_GetPropertyDescriptorById(cx, expando, id, flags, desc))
            return false;
    }
    if (desc->obj) {
        if (!JS_WrapPropertyDescriptor(cx, desc))
            return false;
        
        desc->obj = wrapper;
        return true;
    }

    JSBool hasProp;
    if (!JS_HasPropertyById(cx, holder, id, &hasProp)) {
        return false;
    }
    if (!hasProp) {
        XPCWrappedNative *wn = getWN(wrapper);

        
        if (!NATIVE_HAS_FLAG(wn, WantNewResolve)) {
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

#ifdef DEBUG
        NS_ASSERTION(!pobj || (JS_HasPropertyById(cx, holder, id, &hasProp) &&
                     hasProp), "id got defined somewhere else?");
#endif
    }

    return true;
}

bool
XPCWrappedNativeXrayTraits::defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                      PropertyDescriptor *desc)
{
    JSObject *holder = getHolderObject(wrapper);
    if (isResolving(cx, holder, id)) {
        if (!(desc->attrs & (JSPROP_GETTER | JSPROP_SETTER))) {
            if (!desc->getter)
                desc->getter = holder_get;
            if (!desc->setter)
                desc->setter = holder_set;
        }

        return JS_DefinePropertyById(cx, holder, id, desc->value, desc->getter, desc->setter,
                                     desc->attrs);
    }

    
    
    JSObject *target = getTargetObject(wrapper);
    JSAutoCompartment ac(cx, target);

    
    JSObject *expandoObject = EnsureExpandoObject(cx, wrapper, target);
    if (!expandoObject)
        return false;

    
    PropertyDescriptor wrappedDesc = *desc;
    if (!JS_WrapPropertyDescriptor(cx, &wrappedDesc))
        return false;

    return JS_DefinePropertyById(cx, expandoObject, id, wrappedDesc.value,
                                 wrappedDesc.getter, wrappedDesc.setter,
                                 wrappedDesc.attrs);
}

bool
XPCWrappedNativeXrayTraits::delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    JSObject *target = getTargetObject(wrapper);
    JSObject *expando = LookupExpandoObject(cx, target, wrapper);
    JSBool b = true;
    if (expando) {
        JSAutoCompartment ac(cx, expando);
        jsval v;
        if (!JS_DeletePropertyById2(cx, expando, id, &v) ||
            !JS_ValueToBoolean(cx, v, &b))
        {
            return false;
        }
    }

    *bp = !!b;
    return true;
}

bool
XPCWrappedNativeXrayTraits::enumerateNames(JSContext *cx, JSObject *wrapper, unsigned flags,
                                           JS::AutoIdVector &props)
{
    
    
    JSObject *target = getTargetObject(wrapper);
    JSObject *expando = LookupExpandoObject(cx, target, wrapper);
    if (expando) {
        JSAutoCompartment ac(cx, expando);
        if (!js::GetPropertyNames(cx, expando, flags, &props))
            return false;
    }
    if (!JS_WrapAutoIdVector(cx, props))
        return false;

    
    JS::AutoIdVector wnProps(cx);
    {
        JSAutoCompartment ac(cx, target);
        if (!js::GetPropertyNames(cx, target, flags, &wnProps))
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

bool
XPCWrappedNativeXrayTraits::call(JSContext *cx, JSObject *wrapper,
                                 unsigned argc, Value *vp)
{
    
    XPCWrappedNative *wn = getWN(wrapper);
    if (NATIVE_HAS_FLAG(wn, WantCall)) {
        XPCCallContext ccx(JS_CALLER, cx, wrapper, nullptr, JSID_VOID, argc,
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

bool
XPCWrappedNativeXrayTraits::construct(JSContext *cx, JSObject *wrapper,
                                      unsigned argc, Value *argv, Value *rval)
{
    
    XPCWrappedNative *wn = getWN(wrapper);
    if (NATIVE_HAS_FLAG(wn, WantConstruct)) {
        XPCCallContext ccx(JS_CALLER, cx, wrapper, nullptr, JSID_VOID, argc, argv, rval);
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

bool
ProxyXrayTraits::resolveNativeProperty(JSContext *cx, JSObject *wrapper, JSObject *holder,
                                       jsid id, bool set, JSPropertyDescriptor *desc)
{
    JSObject *obj = getTargetObject(wrapper);
    return js::GetProxyHandler(obj)->getPropertyDescriptor(cx, wrapper, id, set, desc);
}

bool
ProxyXrayTraits::resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper, JSObject *wrapper,
                                    JSObject *holder, jsid id, bool set, PropertyDescriptor *desc)
{
    JSObject *obj = getTargetObject(wrapper);
    bool ok = js::GetProxyHandler(obj)->getOwnPropertyDescriptor(cx, wrapper, id, set, desc);
    if (ok) {
        
        if (desc->obj)
            desc->obj = wrapper;
    }

    
    return ok;
}

bool
ProxyXrayTraits::defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                PropertyDescriptor *desc)
{
    JSObject *holder = getHolderObject(cx, wrapper);
    if (!holder)
        return false;

    return JS_DefinePropertyById(cx, holder, id, desc->value, desc->getter, desc->setter,
                                 desc->attrs);
}

bool
ProxyXrayTraits::delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    JSObject *obj = getTargetObject(wrapper);
    if (!js::GetProxyHandler(obj)->delete_(cx, wrapper, id, bp))
        return false;

    JSObject *holder;
    if (*bp && (holder = getHolderObject(cx, wrapper, false)))
        JS_DeletePropertyById(cx, holder, id);

    return true;
}

bool
ProxyXrayTraits::enumerateNames(JSContext *cx, JSObject *wrapper, unsigned flags,
                                JS::AutoIdVector &props)
{
    JSObject *obj = getTargetObject(wrapper);
    if (flags & (JSITER_OWNONLY | JSITER_HIDDEN))
        return js::GetProxyHandler(obj)->getOwnPropertyNames(cx, wrapper, props);

    return js::GetProxyHandler(obj)->enumerate(cx, wrapper, props);
}




JSObject*
ProxyXrayTraits::createHolderObject(JSContext *cx, JSObject *wrapper)
{
    JSObject *obj = JS_NewObjectWithGivenProto(cx, nullptr, nullptr,
                                               JS_GetGlobalForObject(cx, wrapper));
    if (!obj)
        return nullptr;
    js::SetProxyExtra(wrapper, 0, ObjectValue(*obj));
    return obj;
}

bool
DOMXrayTraits::resolveNativeProperty(JSContext *cx, JSObject *wrapper, JSObject *holder, jsid id,
                                     bool set, JSPropertyDescriptor *desc)
{
    JSObject *obj = getTargetObject(wrapper);
    const NativePropertyHooks *nativeHooks = GetDOMClass(obj)->mNativeHooks;

    do {
        if (!nativeHooks->mResolveProperty(cx, wrapper, id, set, desc))
            return false;

        if (desc->obj) {
            NS_ASSERTION(desc->obj == wrapper, "What did we resolve this on?");
            return true;
        }
    } while ((nativeHooks = nativeHooks->mProtoHooks));

    return true;
}

bool
DOMXrayTraits::resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper, JSObject *wrapper,
                                  JSObject *holder, jsid id, bool set, JSPropertyDescriptor *desc)
{
    JSObject *obj = getTargetObject(wrapper);
    const NativePropertyHooks *nativeHooks = GetDOMClass(obj)->mNativeHooks;

    if (nativeHooks->mResolveOwnProperty) {
        if (!nativeHooks->mResolveOwnProperty(cx, wrapper, id, set, desc))
            return false;

        NS_ASSERTION(!desc->obj || desc->obj == wrapper,
                     "What did we resolve this on?");
    }

    return true;
}

bool
DOMXrayTraits::defineProperty(JSContext *cx, JSObject *wrapper, jsid id, PropertyDescriptor *desc)
{
    JSObject *holder = getHolderObject(cx, wrapper);
    if (!holder)
        return false;

    return JS_DefinePropertyById(cx, holder, id, desc->value, desc->getter, desc->setter,
                                 desc->attrs);
}

bool
DOMXrayTraits::delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    JSObject *holder;
    if ((holder = getHolderObject(cx, wrapper, false)))
        JS_DeletePropertyById(cx, holder, id);

    return true;
}

bool
DOMXrayTraits::enumerateNames(JSContext *cx, JSObject *wrapper, unsigned flags,
                              JS::AutoIdVector &props)
{
    JSObject *obj = getTargetObject(wrapper);
    const NativePropertyHooks *nativeHooks = GetDOMClass(obj)->mNativeHooks;

    if (nativeHooks->mEnumerateOwnProperties &&
        !nativeHooks->mEnumerateOwnProperties(cx, wrapper, props))
        return false;

    if (flags & (JSITER_OWNONLY | JSITER_HIDDEN))
        
        return true;

    do {
        if (!nativeHooks->mEnumerateProperties(cx, wrapper, props)) {
            return false;
        }
    } while ((nativeHooks = nativeHooks->mProtoHooks));

    return true;
}

JSObject*
DOMXrayTraits::createHolderObject(JSContext *cx, JSObject *wrapper)
{
    JSObject *obj = JS_NewObjectWithGivenProto(cx, nullptr, nullptr,
                                               JS_GetGlobalForObject(cx, wrapper));
    if (!obj)
        return nullptr;
    js::SetProxyExtra(wrapper, 0, ObjectValue(*obj));
    return obj;
}

template <typename Base, typename Traits>
XrayWrapper<Base, Traits>::XrayWrapper(unsigned flags)
  : Base(flags | WrapperFactory::IS_XRAY_WRAPPER_FLAG)
{
}

template <typename Base, typename Traits>
XrayWrapper<Base, Traits>::~XrayWrapper()
{
}

namespace XrayUtils {

bool
IsTransparent(JSContext *cx, JSObject *wrapper)
{
    return WrapperFactory::HasWaiveXrayFlag(wrapper);
}

JSObject *
GetNativePropertiesObject(JSContext *cx, JSObject *wrapper)
{
    NS_ASSERTION(js::IsWrapper(wrapper) && WrapperFactory::IsXrayWrapper(wrapper),
                 "bad object passed in");

    JSObject *holder = GetHolder(wrapper);
    NS_ASSERTION(holder, "uninitialized wrapper being used?");
    return holder;
}

}

static JSBool
XrayToString(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *wrapper = JS_THIS_OBJECT(cx, vp);
    if (!wrapper)
        return false;
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "XrayToString called on an incompatible object");
        return false;
    }

    nsAutoString result(NS_LITERAL_STRING("[object XrayWrapper "));
    JSObject *obj = &js::GetProxyPrivate(wrapper).toObject();
    if (IsDOMProxy(obj) || oldproxybindings::instanceIsProxy(obj)) {
        JSString *wrapperStr = js::GetProxyHandler(wrapper)->obj_toString(cx, wrapper);
        size_t length;
        const jschar* chars = JS_GetStringCharsAndLength(cx, wrapperStr, &length);
        if (!chars) {
            JS_ReportOutOfMemory(cx);
            return false;
        }
        result.Append(chars, length);
    } else if (IsDOMClass(JS_GetClass(obj))) {
        result.AppendLiteral("[Object ");
        result.AppendASCII(JS_GetClass(obj)->name);
        result.Append(']');
    } else {
        XPCCallContext ccx(JS_CALLER, cx, XrayTraits::getTargetObject(wrapper));
        XPCWrappedNative *wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
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

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                                 bool set, js::PropertyDescriptor *desc)
{
    JSObject *holder = Traits::getHolderObject(cx, wrapper);
    if (Traits::isResolving(cx, holder, id)) {
        desc->obj = NULL;
        return true;
    }

    bool status;
    Wrapper::Action action = set ? Wrapper::SET : Wrapper::GET;
    desc->obj = NULL; 
    if (!this->enter(cx, wrapper, id, action, &status))
        return status;

    typename Traits::ResolvingIdImpl resolving(wrapper, id);

    
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSObject *obj = Traits::getTargetObject(wrapper);
        {
            JSAutoCompartment ac(cx, obj);
            if (!JS_GetPropertyDescriptorById(cx, obj, id,
                                              (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED,
                                              desc)) {
                return false;
            }
        }

        if (desc->obj)
            desc->obj = wrapper;
        return JS_WrapPropertyDescriptor(cx, desc);
    }

    if (!holder)
        return false;

    
    
    
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    if (AccessCheck::wrapperSubsumes(wrapper) &&
        id == rt->GetStringID(XPCJSRuntime::IDX_WRAPPED_JSOBJECT)) {
        bool status;
        Wrapper::Action action = set ? Wrapper::SET : Wrapper::GET;
        desc->obj = NULL; 
        if (!this->enter(cx, wrapper, id, action, &status))
            return status;

        desc->obj = wrapper;
        desc->attrs = JSPROP_ENUMERATE|JSPROP_SHARED;
        desc->getter = wrappedJSObject_getter;
        desc->setter = NULL;
        desc->shortid = 0;
        desc->value = JSVAL_VOID;
        return true;
    }

    if (!Traits::resolveOwnProperty(cx, *this, wrapper, holder, id, set, desc))
        return false;

    if (desc->obj)
        return true;

    if (!JS_GetPropertyDescriptorById(cx, holder, id, JSRESOLVE_QUALIFIED, desc))
        return false;
    if (desc->obj) {
        desc->obj = wrapper;
        return true;
    }

    
    if (!Traits::resolveNativeProperty(cx, wrapper, holder, id, set, desc))
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

    unsigned flags = (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED;
    return JS_DefinePropertyById(cx, holder, id, desc->value, desc->getter, desc->setter,
                                 desc->attrs) &&
           JS_GetPropertyDescriptorById(cx, holder, id, flags, desc);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                                    bool set, PropertyDescriptor *desc)
{
    JSObject *holder = Traits::getHolderObject(cx, wrapper);
    if (Traits::isResolving(cx, holder, id)) {
        desc->obj = NULL;
        return true;
    }

    bool status;
    Wrapper::Action action = set ? Wrapper::SET : Wrapper::GET;
    desc->obj = NULL; 
    if (!this->enter(cx, wrapper, id, action, &status))
        return status;

    typename Traits::ResolvingIdImpl resolving(wrapper, id);

    
    
    
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSObject *obj = Traits::getTargetObject(wrapper);
        {
            JSAutoCompartment ac(cx, obj);
            if (!JS_GetPropertyDescriptorById(cx, obj, id,
                                              (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED,
                                              desc)) {
                return false;
            }
        }

        desc->obj = (desc->obj == obj) ? wrapper : nullptr;
        return JS_WrapPropertyDescriptor(cx, desc);
    }

    if (!Traits::resolveOwnProperty(cx, *this, wrapper, holder, id, set, desc))
        return false;

    if (desc->obj)
        return true;

    unsigned flags = (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED;
    if (!JS_GetPropertyDescriptorById(cx, holder, id, flags, desc))
        return false;

    
    if (desc->obj)
        desc->obj = wrapper;

    return true;
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                          js::PropertyDescriptor *desc)
{
    
    
    if (WrapperFactory::IsShadowingForbidden(wrapper)) {
        JSObject *holder = Traits::getHolderObject(cx, wrapper);
        js::PropertyDescriptor nativeProp;
        if (!Traits::resolveNativeProperty(cx, wrapper, holder, id, false, &nativeProp))
            return false;
        if (nativeProp.obj) {
            JS_ReportError(cx, "Permission denied to shadow native property");
            return false;
        }
    }

    
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSObject *obj = Traits::getTargetObject(wrapper);
        JSAutoCompartment ac(cx, obj);
        if (!JS_WrapPropertyDescriptor(cx, desc))
            return false;

        return JS_DefinePropertyById(cx, obj, id, desc->value, desc->getter, desc->setter,
                                     desc->attrs);
    }

    PropertyDescriptor existing_desc;
    if (!getOwnPropertyDescriptor(cx, wrapper, id, true, &existing_desc))
        return false;

    if (existing_desc.obj && (existing_desc.attrs & JSPROP_PERMANENT))
        return true; 

    return Traits::defineProperty(cx, wrapper, id, desc);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getOwnPropertyNames(JSContext *cx, JSObject *wrapper,
                                               JS::AutoIdVector &props)
{
    return enumerate(cx, wrapper, JSITER_OWNONLY | JSITER_HIDDEN, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSObject *obj = Traits::getTargetObject(wrapper);

        JSAutoCompartment ac(cx, obj);

        JSBool b;
        jsval v;
        if (!JS_DeletePropertyById2(cx, obj, id, &v) || !JS_ValueToBoolean(cx, v, &b))
            return false;
        *bp = !!b;
        return true;
    }

    return Traits::delete_(cx, wrapper, id, bp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::enumerate(JSContext *cx, JSObject *wrapper, unsigned flags,
                                     JS::AutoIdVector &props)
{
    
    if (XrayUtils::IsTransparent(cx, wrapper)) {
        JSObject *obj = Traits::getTargetObject(wrapper);
        JSAutoCompartment ac(cx, obj);
        return js::GetPropertyNames(cx, obj, flags, &props);
    }

    if (!AccessCheck::wrapperSubsumes(wrapper)) {
        JS_ReportError(cx, "Not allowed to enumerate cross origin objects");
        return false;
    }

    return Traits::enumerateNames(cx, wrapper, flags, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::enumerate(JSContext *cx, JSObject *wrapper, JS::AutoIdVector &props)
{
    return enumerate(cx, wrapper, 0, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                               js::Value *vp)
{
    
    
    
    return BaseProxyHandler::get(cx, wrapper, wrapper, id, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                               bool strict, js::Value *vp)
{
    
    
    
    return BaseProxyHandler::set(cx, wrapper, wrapper, id, strict, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    
    return BaseProxyHandler::has(cx, wrapper, id, bp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    
    return BaseProxyHandler::hasOwn(cx, wrapper, id, bp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::keys(JSContext *cx, JSObject *wrapper, JS::AutoIdVector &props)
{
    
    return BaseProxyHandler::keys(cx, wrapper, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::iterate(JSContext *cx, JSObject *wrapper, unsigned flags,
                                   js::Value *vp)
{
    
    return BaseProxyHandler::iterate(cx, wrapper, flags, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::call(JSContext *cx, JSObject *wrapper, unsigned argc, js::Value *vp)
{
    return Traits::call(cx, wrapper, argc, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::construct(JSContext *cx, JSObject *wrapper, unsigned argc,
                                     js::Value *argv, js::Value *rval)
{
    return Traits::construct(cx, wrapper, argc, argv, rval);
}


#define XRAY XrayWrapper<CrossCompartmentSecurityWrapper, XPCWrappedNativeXrayTraits >
template <> XRAY XRAY::singleton(0);
template class XRAY;
#undef XRAY

#define XRAY XrayWrapper<SameCompartmentSecurityWrapper, XPCWrappedNativeXrayTraits >
template <> XRAY XRAY::singleton(0);
template class XRAY;
#undef XRAY

#define XRAY XrayWrapper<CrossCompartmentWrapper, XPCWrappedNativeXrayTraits >
template <> XRAY XRAY::singleton(0);
template class XRAY;
#undef XRAY

#define XRAY XrayWrapper<CrossCompartmentWrapper, ProxyXrayTraits >
template <> XRAY XRAY::singleton(0);
template class XRAY;
#undef XRAY

#define XRAY XrayWrapper<CrossCompartmentWrapper, DOMXrayTraits >
template <> XRAY XRAY::singleton(0);
template class XRAY;
#undef XRAY



#define XRAY XrayWrapper<DirectWrapper, XPCWrappedNativeXrayTraits >
template <> XRAY XRAY::singleton(0);
template class XRAY;
#undef XRAY

#define XRAY XrayWrapper<DirectWrapper, ProxyXrayTraits >
template <> XRAY XRAY::singleton(0);
template class XRAY;
#undef XRAY

#define XRAY XrayWrapper<DirectWrapper, DOMXrayTraits >
template <> XRAY XRAY::singleton(0);
template class XRAY;
#undef XRAY

}
