






#include "XrayWrapper.h"
#include "AccessCheck.h"
#include "FilteringWrapper.h"
#include "WaiveXrayWrapper.h"
#include "WrapperFactory.h"

#include "nsINode.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"

#include "XPCWrapper.h"
#include "xpcprivate.h"

#include "jsapi.h"
#include "nsJSUtils.h"

#include "mozilla/dom/BindingUtils.h"
#include "nsGlobalWindow.h"

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
}

using namespace XrayUtils;

XrayType
GetXrayType(JSObject *obj)
{
    obj = js::UnwrapObject(obj,  false);
    if (mozilla::dom::UseDOMXray(obj))
        return XrayForDOMObject;

    js::Class* clasp = js::GetObjectClass(obj);
    if (IS_WRAPPER_CLASS(clasp) || clasp->ext.innerObject) {
        NS_ASSERTION(clasp->ext.innerObject || IS_WN_WRAPPER_OBJECT(obj),
                     "We forgot to Morph a slim wrapper!");
        return XrayForWrappedNative;
    }
    return NotXray;
}

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

    virtual bool resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper,
                                    JSObject *wrapper, JSObject *holder,
                                    jsid id, JSPropertyDescriptor *desc, unsigned flags);

    static bool call(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned argc, Value *vp)
    {
        MOZ_NOT_REACHED("Call trap currently implemented only for XPCWNs");
    }
    static bool construct(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned argc,
                          Value *argv, JS::MutableHandle<Value> rval)
    {
        MOZ_NOT_REACHED("Call trap currently implemented only for XPCWNs");
    }

    virtual void preserveWrapper(JSObject *target) = 0;

    JSObject* getExpandoObject(JSContext *cx, JSObject *target,
                               JSObject *consumer);
    JSObject* ensureExpandoObject(JSContext *cx, JSObject *wrapper,
                                  JSObject *target);

    JSObject* getHolder(JSObject *wrapper);
    JSObject* ensureHolder(JSContext *cx, JSObject *wrapper);
    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper) = 0;

    virtual JSObject* getExpandoChain(JSObject *obj) = 0;
    virtual void setExpandoChain(JSObject *obj, JSObject *chain) = 0;
    bool cloneExpandoChain(JSContext *cx, JSObject *dst, JSObject *src);

private:
    bool expandoObjectMatchesConsumer(JSContext *cx, JSObject *expandoObject,
                                      nsIPrincipal *consumerOrigin,
                                      JSObject *exclusiveGlobal);
    JSObject* getExpandoObjectInternal(JSContext *cx, JSObject *target,
                                       nsIPrincipal *origin,
                                       JSObject *exclusiveGlobal);
    JSObject* attachExpandoObject(JSContext *cx, JSObject *target,
                                  nsIPrincipal *origin,
                                  JSObject *exclusiveGlobal);
};

class XPCWrappedNativeXrayTraits : public XrayTraits
{
public:
    static const XrayType Type = XrayForWrappedNative;

    static bool resolveNativeProperty(JSContext *cx, JSObject *wrapper, JSObject *holder, jsid id,
                                      JSPropertyDescriptor *desc, unsigned flags);
    virtual bool resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper, JSObject *wrapper,
                                    JSObject *holder, jsid id, JSPropertyDescriptor *desc,
                                    unsigned flags);
    static bool defineProperty(JSContext *cx, JS::Handle<JSObject*> wrapper,
                               JS::Handle<jsid>,
                               PropertyDescriptor *desc,
                               PropertyDescriptor &existingDesc,
                               bool *defined);
    static bool enumerateNames(JSContext *cx, JSObject *wrapper, unsigned flags,
                               JS::AutoIdVector &props);
    static bool call(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned argc, Value *vp);
    static bool construct(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned argc,
                          Value *argv, JS::MutableHandle<JS::Value> rval);

    static bool isResolving(JSContext *cx, JSObject *holder, jsid id);

    static bool resolveDOMCollectionProperty(JSContext *cx, JSObject *wrapper, JSObject *holder,
                                             jsid id, PropertyDescriptor *desc, unsigned flags);

    static XPCWrappedNative* getWN(JSObject *wrapper) {
        return GetWrappedNative(getTargetObject(wrapper));
    }

    virtual void preserveWrapper(JSObject *target);

    typedef ResolvingId ResolvingIdImpl;

    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper);
    virtual JSObject* getExpandoChain(JSObject *obj) {
        return GetWNExpandoChain(obj);
    }
    virtual void setExpandoChain(JSObject *obj, JSObject *chain) {
        SetWNExpandoChain(obj, chain);
    }

    static XPCWrappedNativeXrayTraits singleton;
};

class DOMXrayTraits : public XrayTraits
{
public:
    static const XrayType Type = XrayForDOMObject;

    static bool resolveNativeProperty(JSContext *cx, JSObject *wrapper, JSObject *holder, jsid id,
                                      JSPropertyDescriptor *desc, unsigned flags);
    virtual bool resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper, JSObject *wrapper,
                                    JSObject *holder, jsid id, JSPropertyDescriptor *desc,
                                    unsigned flags);
    static bool defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                               PropertyDescriptor *desc,
                               PropertyDescriptor &existingDesc,
                               bool *defined);
    static bool enumerateNames(JSContext *cx, JSObject *wrapper, unsigned flags,
                               JS::AutoIdVector &props);
    static bool call(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned argc, Value *vp);
    static bool construct(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned argc,
                          Value *argv, JS::MutableHandle<JS::Value> rval);

    static bool isResolving(JSContext *cx, JSObject *holder, jsid id)
    {
        return false;
    }

    typedef ResolvingIdDummy ResolvingIdImpl;

    virtual void preserveWrapper(JSObject *target);

    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper);

    virtual JSObject* getExpandoChain(JSObject *obj) {
        return mozilla::dom::GetXrayExpandoChain(obj);
    }
    virtual void setExpandoChain(JSObject *obj, JSObject *chain) {
        mozilla::dom::SetXrayExpandoChain(obj, chain);
    }

    static DOMXrayTraits singleton;
};

XPCWrappedNativeXrayTraits XPCWrappedNativeXrayTraits::singleton;
DOMXrayTraits DOMXrayTraits::singleton;

XrayTraits*
GetXrayTraits(JSObject *obj)
{
    switch (GetXrayType(obj)) {
      case XrayForDOMObject:
        return &DOMXrayTraits::singleton;
      case XrayForWrappedNative:
        return &XPCWrappedNativeXrayTraits::singleton;
      default:
        return nullptr;
    }
}
















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
XrayTraits::expandoObjectMatchesConsumer(JSContext *cx,
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
XrayTraits::getExpandoObjectInternal(JSContext *cx, JSObject *target,
                                     nsIPrincipal *origin,
                                     JSObject *exclusiveGlobal)
{
    
    
    JSAutoCompartment ac(cx, target);
    if (!JS_WrapObject(cx, &exclusiveGlobal))
        return NULL;

    
    JSObject *head = getExpandoChain(target);
    while (head) {
        if (expandoObjectMatchesConsumer(cx, head, origin, exclusiveGlobal))
            return head;
        head = JS_GetReservedSlot(head, JSSLOT_EXPANDO_NEXT).toObjectOrNull();
    }

    
    return nullptr;
}

JSObject *
XrayTraits::getExpandoObject(JSContext *cx, JSObject *target, JSObject *consumer)
{
    JSObject *consumerGlobal = js::GetGlobalForObjectCrossCompartment(consumer);
    bool isSandbox = !strcmp(js::GetObjectJSClass(consumerGlobal)->name, "Sandbox");
    return getExpandoObjectInternal(cx, target, ObjectPrincipal(consumer),
                                    isSandbox ? consumerGlobal : nullptr);
}

JSObject *
XrayTraits::attachExpandoObject(JSContext *cx, JSObject *target,
                                nsIPrincipal *origin, JSObject *exclusiveGlobal)
{
    
    MOZ_ASSERT(js::IsObjectInContextCompartment(target, cx));
    MOZ_ASSERT(!exclusiveGlobal || js::IsObjectInContextCompartment(exclusiveGlobal, cx));

    
    MOZ_ASSERT(!getExpandoObjectInternal(cx, target, origin, exclusiveGlobal));

    
    JSObject *expandoObject = JS_NewObjectWithGivenProto(cx, &ExpandoObjectClass,
                                                         nullptr, target);
    if (!expandoObject)
        return nullptr;

    
    NS_ADDREF(origin);
    JS_SetReservedSlot(expandoObject, JSSLOT_EXPANDO_ORIGIN, PRIVATE_TO_JSVAL(origin));

    
    JS_SetReservedSlot(expandoObject, JSSLOT_EXPANDO_EXCLUSIVE_GLOBAL,
                       OBJECT_TO_JSVAL(exclusiveGlobal));

    
    
    
    JSObject *chain = getExpandoChain(target);
    if (!chain)
        preserveWrapper(target);

    
    JS_SetReservedSlot(expandoObject, JSSLOT_EXPANDO_NEXT, OBJECT_TO_JSVAL(chain));
    setExpandoChain(target, expandoObject);

    return expandoObject;
}

JSObject *
XrayTraits::ensureExpandoObject(JSContext *cx, JSObject *wrapper,
                                JSObject *target)
{
    
    JSAutoCompartment ac(cx, target);
    JSObject *expandoObject = getExpandoObject(cx, target, wrapper);
    if (!expandoObject) {
        
        
        
        
        
        JSObject *consumerGlobal = js::GetGlobalForObjectCrossCompartment(wrapper);
        bool isSandbox = !strcmp(js::GetObjectJSClass(consumerGlobal)->name, "Sandbox");
        if (!JS_WrapObject(cx, &consumerGlobal))
            return NULL;
        expandoObject = attachExpandoObject(cx, target, ObjectPrincipal(wrapper),
                                            isSandbox ? consumerGlobal : nullptr);
    }
    return expandoObject;
}

bool
XrayTraits::cloneExpandoChain(JSContext *cx, JSObject *dst, JSObject *src)
{
    MOZ_ASSERT(js::IsObjectInContextCompartment(dst, cx));
    MOZ_ASSERT(getExpandoChain(dst) == nullptr);

    JSObject *oldHead = getExpandoChain(src);
    while (oldHead) {
        JSObject *exclusive = JS_GetReservedSlot(oldHead,
                                                 JSSLOT_EXPANDO_EXCLUSIVE_GLOBAL)
                                                .toObjectOrNull();
        if (!JS_WrapObject(cx, &exclusive))
            return false;
        JSObject *newHead = attachExpandoObject(cx, dst, GetExpandoObjectPrincipal(oldHead),
                                                exclusive);
        if (!JS_CopyPropertiesFrom(cx, newHead, oldHead))
            return false;
        oldHead = JS_GetReservedSlot(oldHead, JSSLOT_EXPANDO_NEXT).toObjectOrNull();
    }
    return true;
}

namespace XrayUtils {
bool CloneExpandoChain(JSContext *cx, JSObject *dst, JSObject *src)
{
    return GetXrayTraits(src)->cloneExpandoChain(cx, dst, src);
}
}

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

JSObject*
XrayTraits::getHolder(JSObject *wrapper)
{
    MOZ_ASSERT(WrapperFactory::IsXrayWrapper(wrapper));
    js::Value v = js::GetProxyExtra(wrapper, 0);
    return v.isObject() ? &v.toObject() : nullptr;
}

JSObject*
XrayTraits::ensureHolder(JSContext *cx, JSObject *wrapper)
{
    JSObject *holder = getHolder(wrapper);
    if (holder)
        return holder;
    holder = createHolder(cx, wrapper); 
    if (holder)
        js::SetProxyExtra(wrapper, 0, ObjectValue(*holder));
    return holder;
}

bool
XPCWrappedNativeXrayTraits::isResolving(JSContext *cx, JSObject *holder,
                                        jsid id)
{
    ResolvingId *cur = ResolvingId::getResolvingId(holder);
    if (!cur)
        return false;
    return cur->isResolving(id);
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
                                                         JSObject *holder, jsid id,
                                                         PropertyDescriptor *desc, unsigned flags)
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
    nsresult rv = wn->GetScriptableInfo()->GetCallback()->NewResolve(wn, cx, wrapper, id,
                                                                     flags, &pobj, &retval);
    if (NS_FAILED(rv)) {
        if (retval)
            XPCThrower::Throw(rv, cx);
        return false;
    }

    if (pobj && !JS_GetPropertyDescriptorById(cx, holder, id, 0, desc))
        return false;

    return true;
}

template <typename T>
static T*
As(JSObject *wrapper)
{
    XPCWrappedNative *wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
    nsCOMPtr<T> native = do_QueryWrappedNative(wn);
    return native;
}

template <typename T>
static bool
Is(JSObject *wrapper)
{
    return !!As<T>(wrapper);
}

static nsQueryInterface
do_QueryInterfaceNative(JSContext* cx, JSObject* wrapper);






static JSBool
mozMatchesSelectorStub(JSContext *cx, unsigned argc, jsval *vp)
{
    if (argc < 1) {
        JS_ReportError(cx, "Not enough arguments");
        return false;
    }

    JSObject *wrapper = JS_THIS_OBJECT(cx, vp);
    JSString *selector = JS_ValueToString(cx, JS_ARGV(cx, vp)[0]);
    if (!selector) {
        return false;
    }
    nsDependentJSString selectorStr;
    NS_ENSURE_TRUE(selectorStr.init(cx, selector), false);

    nsCOMPtr<nsIDOMElement> element = do_QueryInterfaceNative(cx, wrapper);
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

void
XPCWrappedNativeXrayTraits::preserveWrapper(JSObject *target)
{
    XPCWrappedNative *wn =
      static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(target));
    nsRefPtr<nsXPCClassInfo> ci;
    CallQueryInterface(wn->Native(), getter_AddRefs(ci));
    if (ci)
        ci->PreserveWrapper(wn->Native());
}

bool
XPCWrappedNativeXrayTraits::resolveNativeProperty(JSContext *cx, JSObject *wrapper,
                                                  JSObject *holder, jsid id,
                                                  JSPropertyDescriptor *desc, unsigned flags)
{
    MOZ_ASSERT(js::GetObjectJSClass(holder) == &HolderClass);
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    if (id == rt->GetStringID(XPCJSRuntime::IDX_MOZMATCHESSELECTOR) &&
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
        
        return resolveDOMCollectionProperty(cx, wrapper, holder, id, desc, flags);
    }

    XPCNativeInterface *iface;
    XPCNativeMember *member;
    XPCWrappedNative *wn = getWN(wrapper);
    if (ccx.GetWrapper() != wn ||
        !wn->IsValid()  ||
        !(iface = ccx.GetInterface()) ||
        !(member = ccx.GetMember())) {
        
        return resolveDOMCollectionProperty(cx, wrapper, holder, id, desc, flags);
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
    nsCOMPtr<nsIDocument> native = do_QueryInterfaceNative(cx, wrapper);
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
    nsCOMPtr<nsINode> native = do_QueryInterfaceNative(cx, wrapper);
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
    nsCOMPtr<nsINode> node = do_QueryInterfaceNative(cx, wrapper);
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
XrayTraits::resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper,
                               JSObject *wrapper, JSObject *holder, jsid id,
                               PropertyDescriptor *desc, unsigned flags)
{
    desc->obj = NULL;
    JSObject *target = getTargetObject(wrapper);
    JSObject *expando = getExpandoObject(cx, target, wrapper);

    
    
    if (expando) {
        JSAutoCompartment ac(cx, expando);
        if (!JS_GetPropertyDescriptorById(cx, expando, id, 0, desc))
            return false;
    }
    if (desc->obj) {
        if (!JS_WrapPropertyDescriptor(cx, desc))
            return false;
        
        desc->obj = wrapper;
        return true;
    }
    return true;
}

bool
XPCWrappedNativeXrayTraits::resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper,
                                               JSObject *wrapper, JSObject *holder, jsid id,
                                               PropertyDescriptor *desc, unsigned flags)
{
    
    bool ok = XrayTraits::resolveOwnProperty(cx, jsWrapper, wrapper, holder,
                                             id, desc, flags);
    if (!ok || desc->obj)
        return ok;

    
    int32_t index = GetArrayIndexFromId(cx, id);
    if (IsArrayIndex(index)) {
        nsGlobalWindow* win =
            static_cast<nsGlobalWindow*>(As<nsPIDOMWindow>(wrapper));
        
        if (win) {
            bool unused;
            nsCOMPtr<nsIDOMWindow> subframe = win->IndexedGetter(index, unused);
            if (subframe) {
                nsGlobalWindow* global = static_cast<nsGlobalWindow*>(subframe.get());
                global->EnsureInnerWindow();
                JSObject* obj = global->FastGetGlobalJSObject();
                if (MOZ_UNLIKELY(!obj)) {
                    
                    return xpc::Throw(cx, NS_ERROR_FAILURE);
                }
                desc->value = JS::ObjectValue(*obj);
                mozilla::dom::FillPropertyDescriptor(desc, wrapper, true);
                return JS_WrapPropertyDescriptor(cx, desc);
            }
        }
    }

    
    
    MOZ_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    if (AccessCheck::isChrome(wrapper) &&
        (((id == rt->GetStringID(XPCJSRuntime::IDX_BASEURIOBJECT) ||
           id == rt->GetStringID(XPCJSRuntime::IDX_NODEPRINCIPAL)) &&
          Is<nsINode>(wrapper)) ||
          (id == rt->GetStringID(XPCJSRuntime::IDX_DOCUMENTURIOBJECT) &&
          Is<nsIDocument>(wrapper))))
    {
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
XPCWrappedNativeXrayTraits::defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                                           PropertyDescriptor *desc,
                                           PropertyDescriptor &existingDesc,
                                           bool *defined)
{
    *defined = false;
    JSObject *holder = singleton.ensureHolder(cx, wrapper);
    if (isResolving(cx, holder, id)) {
        if (!(desc->attrs & (JSPROP_GETTER | JSPROP_SETTER))) {
            if (!desc->getter)
                desc->getter = holder_get;
            if (!desc->setter)
                desc->setter = holder_set;
        }

        *defined = true;
        return JS_DefinePropertyById(cx, holder, id, desc->value, desc->getter, desc->setter,
                                     desc->attrs);
    }

    
    
    int32_t index = GetArrayIndexFromId(cx, id);
    if (IsArrayIndex(index) && Is<nsIDOMWindow>(wrapper)) {
        *defined = true;
        return true;
    }

    return true;
}

bool
XPCWrappedNativeXrayTraits::enumerateNames(JSContext *cx, JSObject *wrapper, unsigned flags,
                                           JS::AutoIdVector &props)
{
    
    JS::AutoIdVector wnProps(cx);
    {
        JSObject *target = singleton.getTargetObject(wrapper);
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

JSObject *
XPCWrappedNativeXrayTraits::createHolder(JSContext *cx, JSObject *wrapper)
{
    JSObject *global = JS_GetGlobalForObject(cx, wrapper);
    JSObject *holder = JS_NewObjectWithGivenProto(cx, &HolderClass, nullptr,
                                                  global);
    if (!holder)
        return nullptr;

    js::SetReservedSlot(holder, JSSLOT_RESOLVING, PrivateValue(NULL));
    return holder;
}

bool
XPCWrappedNativeXrayTraits::call(JSContext *cx, JS::Handle<JSObject*> wrapper,
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
XPCWrappedNativeXrayTraits::construct(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                      unsigned argc, Value *argv,
                                      JS::MutableHandle<JS::Value> rval)
{
    
    XPCWrappedNative *wn = getWN(wrapper);
    if (NATIVE_HAS_FLAG(wn, WantConstruct)) {
        XPCCallContext ccx(JS_CALLER, cx, wrapper, nullptr, JSID_VOID, argc, argv, rval.address());
        if (!ccx.IsValid())
            return false;
        bool ok = true;
        nsresult rv = wn->GetScriptableInfo()->GetCallback()->Construct(wn, cx, wrapper,
                                                                        argc, argv, rval.address(),
                                                                        &ok);
        if (NS_FAILED(rv)) {
            if (ok)
                XPCThrower::Throw(rv, cx);
            return false;
        }
    }

    return true;

}

bool
DOMXrayTraits::resolveNativeProperty(JSContext *cx, JSObject *wrapper, JSObject *holder, jsid id,
                                     JSPropertyDescriptor *desc, unsigned flags)
{
    JSObject *obj = getTargetObject(wrapper);
    if (!XrayResolveNativeProperty(cx, wrapper, obj, id, desc))
        return false;

    NS_ASSERTION(!desc->obj || desc->obj == wrapper,
                 "What did we resolve this on?");

    return true;
}

bool
DOMXrayTraits::resolveOwnProperty(JSContext *cx, js::Wrapper &jsWrapper, JSObject *wrapper,
                                  JSObject *holder, jsid id, JSPropertyDescriptor *desc,
                                  unsigned flags)
{
    
    bool ok = XrayTraits::resolveOwnProperty(cx, jsWrapper, wrapper, holder,
                                             id, desc, flags);
    if (!ok || desc->obj)
        return ok;

    JSObject *obj = getTargetObject(wrapper);
    if (!XrayResolveOwnProperty(cx, wrapper, obj, id, desc, flags))
        return false;

    NS_ASSERTION(!desc->obj || desc->obj == wrapper,
                 "What did we resolve this on?");

    return true;
}

bool
DOMXrayTraits::defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                              PropertyDescriptor *desc,
                              PropertyDescriptor &existingDesc,
                              bool *defined)
{
    if (!existingDesc.obj)
        return true;

    JSObject *obj= getTargetObject(wrapper);
    if (!js::IsProxy(obj))
        return true;

    *defined = true;
    return js::GetProxyHandler(obj)->defineProperty(cx, wrapper, id, desc);
}

bool
DOMXrayTraits::enumerateNames(JSContext *cx, JSObject *wrapper, unsigned flags,
                              JS::AutoIdVector &props)
{
    return XrayEnumerateProperties(cx, wrapper, getTargetObject(wrapper),
                                   flags, props);
}

bool
DOMXrayTraits::call(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned argc, Value *vp)
{
    JSObject *obj = getTargetObject(wrapper);
    AutoValueRooter rval(cx);
    bool ok;
    {
        JSAutoCompartment ac(cx, obj);
        if (!JS_WrapValue(cx, &vp[1]))
            return false;
        JS::Value* argv = JS_ARGV(cx, vp);
        for (unsigned i = 0; i < argc; ++i) {
            if (!JS_WrapValue(cx, &argv[i]))
                return false;
        }
        ok = JS::Call(cx, vp[1], obj, argc, argv, rval.addr());
    }
    if (!ok || !JS_WrapValue(cx, rval.addr()))
        return false;
    JS_SET_RVAL(cx, vp, rval.value());
    return true;
}

bool
DOMXrayTraits::construct(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned argc,
                         Value *argv, JS::MutableHandle<JS::Value> rval)
{
    JSObject *obj = getTargetObject(wrapper);
    MOZ_ASSERT(mozilla::dom::HasConstructor(obj));
    JSObject *newObj;
    {
        JSAutoCompartment ac(cx, obj);
        for (unsigned i = 0; i < argc; ++i) {
            if (!JS_WrapValue(cx, &argv[i]))
                return false;
        }
        newObj = JS_New(cx, obj, argc, argv);
    }
    if (!newObj || !JS_WrapObject(cx, &newObj))
        return false;
    rval.setObject(*newObj);
    return true;
}

void
DOMXrayTraits::preserveWrapper(JSObject *target)
{
    nsISupports *identity;
    if (!mozilla::dom::UnwrapDOMObjectToISupports(target, identity))
        return;
    nsWrapperCache* cache = nullptr;
    CallQueryInterface(identity, &cache);
    if (cache)
        nsContentUtils::PreserveWrapper(identity, cache);
}

JSObject*
DOMXrayTraits::createHolder(JSContext *cx, JSObject *wrapper)
{
    return JS_NewObjectWithGivenProto(cx, nullptr, nullptr,
                                      JS_GetGlobalForObject(cx, wrapper));
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
IsTransparent(JSContext *cx, JSObject *wrapper, jsid id)
{
    
    
    
    nsCOMPtr<nsIContent> content;
    if (EnsureCompartmentPrivate(wrapper)->scope->IsXBLScope() &&
        (content = do_QueryInterfaceNative(cx, wrapper)))
    {
        JS::RootedId id_(cx, id);
        if (nsContentUtils::IsBindingField(cx, content, id_))
            return true;
    }
    return false;
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

bool
IsXrayResolving(JSContext *cx, JSObject *wrapper, jsid id)
{
    if (!WrapperFactory::IsXrayWrapper(wrapper) ||
        GetXrayType(wrapper) != XrayForWrappedNative)
    {
        return false;
    }
    JSObject *holder =
      XPCWrappedNativeXrayTraits::singleton.ensureHolder(cx, wrapper);
    return XPCWrappedNativeXrayTraits::isResolving(cx, holder, id);
}

}

static JSBool
XrayToString(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *wrapper = JS_THIS_OBJECT(cx, vp);
    if (!wrapper)
        return false;
    if (IsWrapper(wrapper) &&
        GetProxyHandler(wrapper) == &sandboxCallableProxyHandler) {
        wrapper = xpc::SandboxCallableProxyHandler::wrappedObject(wrapper);
    }
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "XrayToString called on an incompatible object");
        return false;
    }

    JSObject *obj = XrayTraits::getTargetObject(wrapper);

    static const char start[] = "[object XrayWrapper ";
    static const char end[] = "]";
    if (UseDOMXray(obj))
        return NativeToString(cx, wrapper, obj, start, end, vp);

    nsAutoString result;
    result.AppendASCII(start);

    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative *wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
    char *wrapperStr = wn->ToString(ccx);
    if (!wrapperStr) {
        JS_ReportOutOfMemory(cx);
        return false;
    }
    result.AppendASCII(wrapperStr);
    JS_smprintf_free(wrapperStr);

    result.AppendASCII(end);

    JSString *str = JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar *>(result.get()),
                                        result.Length());
    if (!str)
        return false;

    *vp = STRING_TO_JSVAL(str);
    return true;
}

#ifdef DEBUG

static void
DEBUG_CheckXBLCallable(JSContext *cx, JSObject *obj)
{
    MOZ_ASSERT(!js::IsCrossCompartmentWrapper(obj));
    MOZ_ASSERT(JS_ObjectIsCallable(cx, obj));
}

static void
DEBUG_CheckXBLLookup(JSContext *cx, JSPropertyDescriptor *desc)
{
    if (!desc->obj)
        return;
    if (!desc->value.isUndefined()) {
        MOZ_ASSERT(desc->value.isObject());
        DEBUG_CheckXBLCallable(cx, &desc->value.toObject());
    }
    if (desc->getter) {
        MOZ_ASSERT(desc->attrs & JSPROP_GETTER);
        DEBUG_CheckXBLCallable(cx, JS_FUNC_TO_DATA_PTR(JSObject *, desc->getter));
    }
    if (desc->setter) {
        MOZ_ASSERT(desc->attrs & JSPROP_SETTER);
        DEBUG_CheckXBLCallable(cx, JS_FUNC_TO_DATA_PTR(JSObject *, desc->setter));
    }
}
#else
#define DEBUG_CheckXBLLookup(a, b) {}
#endif

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                                 JS::Handle<jsid> id,
                                                 js::PropertyDescriptor *desc, unsigned flags)
{
    assertEnteredPolicy(cx, wrapper, id);
    JSObject *holder = Traits::singleton.ensureHolder(cx, wrapper);
    if (Traits::isResolving(cx, holder, id)) {
        desc->obj = NULL;
        return true;
    }

    typename Traits::ResolvingIdImpl resolving(wrapper, id);

    
    if (XrayUtils::IsTransparent(cx, wrapper, id)) {
        JSObject *obj = Traits::getTargetObject(wrapper);
        {
            JSAutoCompartment ac(cx, obj);
            if (!JS_GetPropertyDescriptorById(cx, obj, id, flags, desc))
                return false;
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
        desc->obj = wrapper;
        desc->attrs = JSPROP_ENUMERATE|JSPROP_SHARED;
        desc->getter = wrappedJSObject_getter;
        desc->setter = NULL;
        desc->shortid = 0;
        desc->value = JSVAL_VOID;
        return true;
    }

    if (!Traits::singleton.resolveOwnProperty(cx, *this, wrapper, holder, id, desc, flags))
        return false;

    if (desc->obj) {
        desc->obj = wrapper;
        return true;
    }

    
    
    
    
    
    
    nsGlobalWindow *win;
    if (Traits::Type == XrayForWrappedNative && JSID_IS_STRING(id) &&
        (win = static_cast<nsGlobalWindow*>(As<nsPIDOMWindow>(wrapper))))
    {
        nsCOMPtr<nsIDOMWindow> childDOMWin = win->GetChildWindow(id);
        if (childDOMWin) {
            nsGlobalWindow *cwin = static_cast<nsGlobalWindow*>(childDOMWin.get());
            JSObject *childObj = cwin->FastGetGlobalJSObject();
            if (MOZ_UNLIKELY(!childObj))
                return xpc::Throw(cx, NS_ERROR_FAILURE);
            mozilla::dom::FillPropertyDescriptor(desc, wrapper,
                                                 ObjectValue(*childObj),
                                                  true);
            return JS_WrapPropertyDescriptor(cx, desc);
        }
    }

    if (!JS_GetPropertyDescriptorById(cx, holder, id, 0, desc))
        return false;
    if (desc->obj) {
        desc->obj = wrapper;
        return true;
    }

    
    if (!Traits::resolveNativeProperty(cx, wrapper, holder, id, desc, flags))
        return false;

    if (!desc->obj &&
        id == nsXPConnect::GetRuntimeInstance()->GetStringID(XPCJSRuntime::IDX_TO_STRING))
    {

        JSFunction *toString = JS_NewFunction(cx, XrayToString, 0, 0, holder, "toString");
        if (!toString)
            return false;

        desc->obj = wrapper;
        desc->attrs = 0;
        desc->getter = NULL;
        desc->setter = NULL;
        desc->shortid = 0;
        desc->value.setObject(*JS_GetFunctionObject(toString));
    }

    
    
    
    
    
    
    
    
    
    
    
    
    nsCOMPtr<nsIContent> content;
    if (!desc->obj &&
        EnsureCompartmentPrivate(wrapper)->scope->IsXBLScope() &&
        (content = do_QueryInterfaceNative(cx, wrapper)))
    {
        JS::RootedId id_(cx, id);
        if (!nsContentUtils::LookupBindingMember(cx, content, id_, desc))
            return false;
        DEBUG_CheckXBLLookup(cx, desc);
    }

    
    if (!desc->obj)
      return true;

    if (!JS_DefinePropertyById(cx, holder, id, desc->value, desc->getter,
                               desc->setter, desc->attrs) ||
        !JS_GetPropertyDescriptorById(cx, holder, id, flags, desc))
    {
        return false;
    }
    MOZ_ASSERT(desc->obj);
    desc->obj = wrapper;
    return true;
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                                    JS::Handle<jsid> id,
                                                    PropertyDescriptor *desc, unsigned flags)
{
    assertEnteredPolicy(cx, wrapper, id);
    JSObject *holder = Traits::singleton.ensureHolder(cx, wrapper);
    if (Traits::isResolving(cx, holder, id)) {
        desc->obj = NULL;
        return true;
    }

    typename Traits::ResolvingIdImpl resolving(wrapper, id);

    
    
    
    if (XrayUtils::IsTransparent(cx, wrapper, id)) {
        JSObject *obj = Traits::getTargetObject(wrapper);
        {
            JSAutoCompartment ac(cx, obj);
            if (!JS_GetPropertyDescriptorById(cx, obj, id, flags, desc))
                return false;
        }

        desc->obj = (desc->obj == obj) ? wrapper.get() : nullptr; 
        return JS_WrapPropertyDescriptor(cx, desc);
    }

    if (!Traits::singleton.resolveOwnProperty(cx, *this, wrapper, holder, id, desc, flags))
        return false;

    if (desc->obj) {
        desc->obj = wrapper;
        return true;
    }

    if (!JS_GetPropertyDescriptorById(cx, holder, id, flags, desc))
        return false;

    
    if (desc->obj)
        desc->obj = wrapper;

    return true;
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::defineProperty(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                          HandleId id, js::PropertyDescriptor *desc)
{
    assertEnteredPolicy(cx, wrapper, id);
    
    if (XrayUtils::IsTransparent(cx, wrapper, id)) {
        JSObject *obj = Traits::getTargetObject(wrapper);
        JSAutoCompartment ac(cx, obj);
        if (!JS_WrapPropertyDescriptor(cx, desc))
            return false;

        return JS_DefinePropertyById(cx, obj, id, desc->value, desc->getter, desc->setter,
                                     desc->attrs);
    }

    
    
    PropertyDescriptor existing_desc;
    if (!getOwnPropertyDescriptor(cx, wrapper, id, &existing_desc, JSRESOLVE_ASSIGNING))
        return false;

    if (existing_desc.obj && (existing_desc.attrs & JSPROP_PERMANENT))
        return true; 

    bool defined = false;
    if (!Traits::defineProperty(cx, wrapper, id, desc, existing_desc, &defined))
        return false;
    if (defined)
        return true;

    
    
    JSObject *target = Traits::singleton.getTargetObject(wrapper);
    JSAutoCompartment ac(cx, target);

    
    JSObject *expandoObject = Traits::singleton.ensureExpandoObject(cx, wrapper,
                                                                    target);
    if (!expandoObject)
        return false;

    
    PropertyDescriptor wrappedDesc = *desc;
    if (!JS_WrapPropertyDescriptor(cx, &wrappedDesc))
        return false;

    return JS_DefinePropertyById(cx, expandoObject, id, wrappedDesc.value,
                                 wrappedDesc.getter, wrappedDesc.setter,
                                 wrappedDesc.attrs);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getOwnPropertyNames(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                               JS::AutoIdVector &props)
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID);
    return enumerate(cx, wrapper, JSITER_OWNONLY | JSITER_HIDDEN, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::delete_(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                   JS::Handle<jsid> id, bool *bp)
{
    assertEnteredPolicy(cx, wrapper, id);
    
    if (XrayUtils::IsTransparent(cx, wrapper, id)) {
        JSObject *obj = Traits::getTargetObject(wrapper);

        JSAutoCompartment ac(cx, obj);

        JSBool b;
        jsval v;
        if (!JS_DeletePropertyById2(cx, obj, id, &v) || !JS_ValueToBoolean(cx, v, &b))
            return false;
        *bp = !!b;
        return true;
    }

    
    JSObject *target = Traits::getTargetObject(wrapper);
    JSObject *expando = Traits::singleton.getExpandoObject(cx, target, wrapper);
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

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::enumerate(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned flags,
                                     JS::AutoIdVector &props)
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID);
    
    if (XrayUtils::IsTransparent(cx, wrapper, JSID_VOID)) {
        JSObject *obj = Traits::getTargetObject(wrapper);
        JSAutoCompartment ac(cx, obj);
        return js::GetPropertyNames(cx, obj, flags, &props);
    }

    if (!AccessCheck::wrapperSubsumes(wrapper)) {
        JS_ReportError(cx, "Not allowed to enumerate cross origin objects");
        return false;
    }

    
    
    JSObject *target = Traits::singleton.getTargetObject(wrapper);
    JSObject *expando = Traits::singleton.getExpandoObject(cx, target, wrapper);
    if (expando) {
        JSAutoCompartment ac(cx, expando);
        if (!js::GetPropertyNames(cx, expando, flags, &props))
            return false;
    }
    if (!JS_WrapAutoIdVector(cx, props))
        return false;

    return Traits::enumerateNames(cx, wrapper, flags, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::enumerate(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                    JS::AutoIdVector &props)
{
    return enumerate(cx, wrapper, 0, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::get(JSContext *cx, JS::Handle<JSObject*> wrapper,
                               JS::Handle<JSObject*> receiver, JS::Handle<jsid> id,
                               JS::MutableHandle<JS::Value> vp)
{
    
    
    
    return BaseProxyHandler::get(cx, wrapper, wrapper, id, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::set(JSContext *cx, JS::Handle<JSObject*> wrapper,
                               JS::Handle<JSObject*> receiver, JS::Handle<jsid> id,
                               bool strict, JS::MutableHandle<JS::Value> vp)
{
    
    
    
    return BaseProxyHandler::set(cx, wrapper, wrapper, id, strict, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::has(JSContext *cx, JS::Handle<JSObject*> wrapper,
                               JS::Handle<jsid> id, bool *bp)
{
    
    return BaseProxyHandler::has(cx, wrapper, id, bp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::hasOwn(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                  JS::Handle<jsid> id, bool *bp)
{
    
    return BaseProxyHandler::hasOwn(cx, wrapper, id, bp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::keys(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                JS::AutoIdVector &props)
{
    
    return BaseProxyHandler::keys(cx, wrapper, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::iterate(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                   unsigned flags, JS::MutableHandle<JS::Value> vp)
{
    
    return BaseProxyHandler::iterate(cx, wrapper, flags, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::call(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                unsigned argc, js::Value *vp)
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID);
    return Traits::call(cx, wrapper, argc, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::construct(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                     unsigned argc, JS::Value *argv,
                                     JS::MutableHandle<JS::Value> rval)
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID);
    return Traits::construct(cx, wrapper, argc, argv, rval);
}








template<>
PermissiveXrayXPCWN PermissiveXrayXPCWN::singleton(0);
template class PermissiveXrayXPCWN;

template<>
SecurityXrayXPCWN SecurityXrayXPCWN::singleton(0);
template class SecurityXrayXPCWN;

template<>
PermissiveXrayDOM PermissiveXrayDOM::singleton(0);
template class PermissiveXrayDOM;

template<>
SecurityXrayDOM SecurityXrayDOM::singleton(0);
template class SecurityXrayDOM;

template<>
SCPermissiveXrayXPCWN SCPermissiveXrayXPCWN::singleton(0);
template class SCPermissiveXrayXPCWN;

template<>
SCSecurityXrayXPCWN SCSecurityXrayXPCWN::singleton(0);
template class SCSecurityXrayXPCWN;

template<>
SCPermissiveXrayDOM SCPermissiveXrayDOM::singleton(0);
template class SCPermissiveXrayDOM;

static nsQueryInterface
do_QueryInterfaceNative(JSContext* cx, JSObject* wrapper)
{
    nsISupports* nativeSupports;
    if (IsWrapper(wrapper) && WrapperFactory::IsXrayWrapper(wrapper)) {
        JSObject* target = XrayTraits::getTargetObject(wrapper);
        if (GetXrayType(target) == XrayForDOMObject) {
            if (!UnwrapDOMObjectToISupports(target, nativeSupports)) {
                nativeSupports = nullptr;
            }
        } else {
            XPCWrappedNative *wn = GetWrappedNative(target);
            nativeSupports = wn->Native();
        }
    } else {
        nsIXPConnect *xpc = nsXPConnect::GetXPConnect();
        nativeSupports = xpc->GetNativeOfWrapper(cx, wrapper);
    }

    return nsQueryInterface(nativeSupports);
}

}
