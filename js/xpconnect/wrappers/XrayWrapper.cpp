





#include "XrayWrapper.h"
#include "AccessCheck.h"
#include "WrapperFactory.h"

#include "nsIContent.h"
#include "nsIControllers.h"
#include "nsIScriptError.h"
#include "mozilla/dom/Element.h"
#include "nsContentUtils.h"

#include "XPCWrapper.h"
#include "xpcprivate.h"

#include "jsapi.h"
#include "jsprf.h"
#include "nsJSUtils.h"
#include "nsPrintfCString.h"

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/WindowBinding.h"
#include "nsGlobalWindow.h"

using namespace mozilla::dom;
using namespace JS;
using namespace mozilla;

using js::Wrapper;
using js::BaseProxyHandler;
using js::IsCrossCompartmentWrapper;
using js::UncheckedUnwrap;
using js::CheckedUnwrap;

namespace xpc {

using namespace XrayUtils;

#define Between(x, a, b) (a <= x && x <= b)

static_assert(JSProto_URIError - JSProto_Error == 7, "New prototype added in error object range");
#define AssertErrorObjectKeyInBounds(key) \
    static_assert(Between(key, JSProto_Error, JSProto_URIError), "We depend on jsprototypes.h ordering here");
MOZ_FOR_EACH(AssertErrorObjectKeyInBounds, (),
             (JSProto_Error, JSProto_InternalError, JSProto_EvalError, JSProto_RangeError,
              JSProto_ReferenceError, JSProto_SyntaxError, JSProto_TypeError, JSProto_URIError));

static_assert(JSProto_Uint8ClampedArray - JSProto_Int8Array == 8, "New prototype added in typed array range");
#define AssertTypedArrayKeyInBounds(key) \
    static_assert(Between(key, JSProto_Int8Array, JSProto_Uint8ClampedArray), "We depend on jsprototypes.h ordering here");
MOZ_FOR_EACH(AssertTypedArrayKeyInBounds, (),
             (JSProto_Int8Array, JSProto_Uint8Array, JSProto_Int16Array, JSProto_Uint16Array,
              JSProto_Int32Array, JSProto_Uint32Array, JSProto_Float32Array, JSProto_Float64Array, JSProto_Uint8ClampedArray));

#undef Between

inline bool
IsErrorObjectKey(JSProtoKey key)
{
    return key >= JSProto_Error && key <= JSProto_URIError;
}

inline bool
IsTypedArrayKey(JSProtoKey key)
{
    return key >= JSProto_Int8Array && key <= JSProto_Uint8ClampedArray;
}

bool SilentFailure(JSContext *cx, JS::HandleId id, const char *reason);


static bool
IsJSXraySupported(JSProtoKey key)
{
    if (IsTypedArrayKey(key))
        return true;
    if (IsErrorObjectKey(key))
        return true;
    switch (key) {
      case JSProto_Date:
      case JSProto_Object:
      case JSProto_Array:
      case JSProto_Function:
        return true;
      default:
        return false;
    }
}

XrayType
GetXrayType(JSObject *obj)
{
    obj = js::UncheckedUnwrap(obj,  false);
    if (mozilla::dom::UseDOMXray(obj))
        return XrayForDOMObject;

    const js::Class* clasp = js::GetObjectClass(obj);
    if (IS_WN_CLASS(clasp) || clasp->ext.innerObject)
        return XrayForWrappedNative;

    JSProtoKey standardProto = IdentifyStandardInstanceOrPrototype(obj);
    if (IsJSXraySupported(standardProto))
        return XrayForJSObject;

    
    
    
    
    if (IsSandbox(obj))
        return NotXray;

    return XrayForOpaqueObject;
}

JSObject *
XrayAwareCalleeGlobal(JSObject *fun)
{
  MOZ_ASSERT(js::IsFunctionObject(fun));
  JSObject *scope = js::GetObjectParent(fun);
  if (IsXrayWrapper(scope))
    scope = js::UncheckedUnwrap(scope);
  return js::GetGlobalForObjectCrossCompartment(scope);
}

const uint32_t JSSLOT_RESOLVING = 0;
ResolvingId::ResolvingId(JSContext *cx, HandleObject wrapper, HandleId id)
  : mId(id),
    mHolder(cx, getHolderObject(wrapper)),
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

class MOZ_STACK_CLASS ResolvingIdDummy
{
public:
    ResolvingIdDummy(JSContext *cx, HandleObject wrapper, HandleId id)
    {
    }
};

class XrayTraits
{
public:
    XrayTraits() {}

    static JSObject* getTargetObject(JSObject *wrapper) {
        return js::UncheckedUnwrap(wrapper,  false);
    }

    virtual bool resolveNativeProperty(JSContext *cx, HandleObject wrapper,
                                       HandleObject holder, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc) = 0;
    
    
    
    
    virtual bool resolveOwnProperty(JSContext *cx, const Wrapper &jsWrapper,
                                    HandleObject wrapper, HandleObject holder,
                                    HandleId id, MutableHandle<JSPropertyDescriptor> desc);

    bool delete_(JSContext *cx, HandleObject wrapper, HandleId id, bool *bp) {
        *bp = true;
        return true;
    }

    static const char *className(JSContext *cx, HandleObject wrapper, const js::Wrapper& baseInstance) {
        return baseInstance.className(cx, wrapper);
    }

    virtual void preserveWrapper(JSObject *target) = 0;

    static bool set(JSContext *cx, HandleObject wrapper, HandleObject receiver, HandleId id,
                    bool strict, MutableHandleValue vp);

    JSObject* getExpandoObject(JSContext *cx, HandleObject target,
                               HandleObject consumer);
    JSObject* ensureExpandoObject(JSContext *cx, HandleObject wrapper,
                                  HandleObject target);

    JSObject* getHolder(JSObject *wrapper);
    JSObject* ensureHolder(JSContext *cx, HandleObject wrapper);
    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper) = 0;

    JSObject* getExpandoChain(HandleObject obj) {
      return ObjectScope(obj)->GetExpandoChain(obj);
    }

    bool setExpandoChain(JSContext *cx, HandleObject obj, HandleObject chain) {
      return ObjectScope(obj)->SetExpandoChain(cx, obj, chain);
    }
    bool cloneExpandoChain(JSContext *cx, HandleObject dst, HandleObject src);

private:
    bool expandoObjectMatchesConsumer(JSContext *cx, HandleObject expandoObject,
                                      nsIPrincipal *consumerOrigin,
                                      HandleObject exclusiveGlobal);
    JSObject* getExpandoObjectInternal(JSContext *cx, HandleObject target,
                                       nsIPrincipal *origin,
                                       JSObject *exclusiveGlobal);
    JSObject* attachExpandoObject(JSContext *cx, HandleObject target,
                                  nsIPrincipal *origin,
                                  HandleObject exclusiveGlobal);

    XrayTraits(XrayTraits &) MOZ_DELETE;
    const XrayTraits& operator=(XrayTraits &) MOZ_DELETE;
};

class XPCWrappedNativeXrayTraits : public XrayTraits
{
public:
    enum {
        HasPrototype = 0
    };

    static const XrayType Type = XrayForWrappedNative;

    virtual bool resolveNativeProperty(JSContext *cx, HandleObject wrapper,
                                       HandleObject holder, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool resolveOwnProperty(JSContext *cx, const Wrapper &jsWrapper, HandleObject wrapper,
                                    HandleObject holder, HandleId id,
                                    MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    bool defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                        MutableHandle<JSPropertyDescriptor> desc,
                        Handle<JSPropertyDescriptor> existingDesc, bool *defined);
    virtual bool enumerateNames(JSContext *cx, HandleObject wrapper, unsigned flags,
                                AutoIdVector &props);
    static bool call(JSContext *cx, HandleObject wrapper,
                     const JS::CallArgs &args, const js::Wrapper& baseInstance);
    static bool construct(JSContext *cx, HandleObject wrapper,
                          const JS::CallArgs &args, const js::Wrapper& baseInstance);

    static bool isResolving(JSContext *cx, JSObject *holder, jsid id);

    static bool resolveDOMCollectionProperty(JSContext *cx, HandleObject wrapper,
                                             HandleObject holder, HandleId id,
                                             MutableHandle<JSPropertyDescriptor> desc);

    static XPCWrappedNative* getWN(JSObject *wrapper) {
        return XPCWrappedNative::Get(getTargetObject(wrapper));
    }

    virtual void preserveWrapper(JSObject *target) MOZ_OVERRIDE;

    typedef ResolvingId ResolvingIdImpl;

    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE;

    static const JSClass HolderClass;
    static XPCWrappedNativeXrayTraits singleton;
};

const JSClass XPCWrappedNativeXrayTraits::HolderClass = {
    "NativePropertyHolder", JSCLASS_HAS_RESERVED_SLOTS(2),
    JS_PropertyStub, JS_DeletePropertyStub, holder_get, holder_set,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

class DOMXrayTraits : public XrayTraits
{
public:
    enum {
        HasPrototype = 0
    };

    static const XrayType Type = XrayForDOMObject;

    virtual bool resolveNativeProperty(JSContext *cx, HandleObject wrapper,
                                       HandleObject holder, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool resolveOwnProperty(JSContext *cx, const Wrapper &jsWrapper, HandleObject wrapper,
                                    HandleObject holder, HandleId id,
                                    MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    bool defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                        MutableHandle<JSPropertyDescriptor> desc,
                        Handle<JSPropertyDescriptor> existingDesc, bool *defined);
    static bool set(JSContext *cx, HandleObject wrapper, HandleObject receiver, HandleId id,
                    bool strict, MutableHandleValue vp);
    virtual bool enumerateNames(JSContext *cx, HandleObject wrapper, unsigned flags,
                                AutoIdVector &props);
    static bool call(JSContext *cx, HandleObject wrapper,
                     const JS::CallArgs &args, const js::Wrapper& baseInstance);
    static bool construct(JSContext *cx, HandleObject wrapper,
                          const JS::CallArgs &args, const js::Wrapper& baseInstance);

    static bool isResolving(JSContext *cx, JSObject *holder, jsid id)
    {
        return false;
    }

    typedef ResolvingIdDummy ResolvingIdImpl;

    virtual void preserveWrapper(JSObject *target) MOZ_OVERRIDE;

    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE;

    static DOMXrayTraits singleton;
};

class JSXrayTraits : public XrayTraits
{
public:
    enum {
        HasPrototype = 1
    };
    static const XrayType Type = XrayForJSObject;

    virtual bool resolveNativeProperty(JSContext *cx, HandleObject wrapper,
                                       HandleObject holder, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE
    {
        MOZ_CRASH("resolveNativeProperty hook should never be called with HasPrototype = 1");
    }

    virtual bool resolveOwnProperty(JSContext *cx, const Wrapper &jsWrapper, HandleObject wrapper,
                                    HandleObject holder, HandleId id,
                                    MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;

    bool delete_(JSContext *cx, HandleObject wrapper, HandleId id, bool *bp);

    bool defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                        MutableHandle<JSPropertyDescriptor> desc,
                        Handle<JSPropertyDescriptor> existingDesc, bool *defined);

    virtual bool enumerateNames(JSContext *cx, HandleObject wrapper, unsigned flags,
                                AutoIdVector &props);

    static bool call(JSContext *cx, HandleObject wrapper,
                     const JS::CallArgs &args, const js::Wrapper& baseInstance)
    {
        JSXrayTraits &self = JSXrayTraits::singleton;
        RootedObject holder(cx, self.ensureHolder(cx, wrapper));
        if (self.getProtoKey(holder) == JSProto_Function)
            return baseInstance.call(cx, wrapper, args);

        RootedValue v(cx, ObjectValue(*wrapper));
        js_ReportIsNotFunction(cx, v);
        return false;
    }

    static bool construct(JSContext *cx, HandleObject wrapper,
                          const JS::CallArgs &args, const js::Wrapper& baseInstance)
    {
        JSXrayTraits &self = JSXrayTraits::singleton;
        RootedObject holder(cx, self.ensureHolder(cx, wrapper));
        if (self.getProtoKey(holder) == JSProto_Function)
            return baseInstance.construct(cx, wrapper, args);

        RootedValue v(cx, ObjectValue(*wrapper));
        js_ReportIsNotFunction(cx, v);
        return false;
    }

    static bool isResolving(JSContext *cx, JSObject *holder, jsid id)
    {
        return false;
    }

    typedef ResolvingIdDummy ResolvingIdImpl;

    bool getPrototypeOf(JSContext *cx, JS::HandleObject wrapper,
                               JS::HandleObject target,
                               JS::MutableHandleObject protop)
    {
        RootedObject holder(cx, ensureHolder(cx, wrapper));
        JSProtoKey key = getProtoKey(holder);
        if (isPrototype(holder)) {
            JSProtoKey parentKey = js::ParentKeyForStandardClass(key);
            if (parentKey == JSProto_Null) {
                protop.set(nullptr);
                return true;
            }
            key = parentKey;
        }

        {
            JSAutoCompartment ac(cx, target);
            if (!JS_GetClassPrototype(cx, key, protop))
                return false;
        }
        return JS_WrapObject(cx, protop);
    }

    virtual void preserveWrapper(JSObject *target) MOZ_OVERRIDE {
        
        
        
        
    }

    enum {
        SLOT_PROTOKEY = 0,
        SLOT_ISPROTOTYPE,
        SLOT_CONSTRUCTOR_FOR,
        SLOT_COUNT
    };
    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE;

    static JSProtoKey getProtoKey(JSObject *holder) {
        int32_t key = js::GetReservedSlot(holder, SLOT_PROTOKEY).toInt32();
        return static_cast<JSProtoKey>(key);
    }

    static bool isPrototype(JSObject *holder) {
        return js::GetReservedSlot(holder, SLOT_ISPROTOTYPE).toBoolean();
    }

    static JSProtoKey constructorFor(JSObject *holder) {
        int32_t key = js::GetReservedSlot(holder, SLOT_CONSTRUCTOR_FOR).toInt32();
        return static_cast<JSProtoKey>(key);
    }

    static bool getOwnPropertyFromTargetIfSafe(JSContext *cx,
                                               HandleObject target,
                                               HandleObject wrapper,
                                               HandleId id,
                                               MutableHandle<JSPropertyDescriptor> desc);

    static const JSClass HolderClass;
    static JSXrayTraits singleton;
};

const JSClass JSXrayTraits::HolderClass = {
    "JSXrayHolder", JSCLASS_HAS_RESERVED_SLOTS(SLOT_COUNT),
    JS_PropertyStub, JS_DeletePropertyStub,
    JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};




class OpaqueXrayTraits : public XrayTraits
{
public:
    enum {
        HasPrototype = 1
    };
    static const XrayType Type = XrayForOpaqueObject;

    virtual bool resolveNativeProperty(JSContext *cx, HandleObject wrapper,
                                       HandleObject holder, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE
    {
        MOZ_CRASH("resolveNativeProperty hook should never be called with HasPrototype = 1");
    }

    virtual bool resolveOwnProperty(JSContext *cx, const Wrapper &jsWrapper, HandleObject wrapper,
                                    HandleObject holder, HandleId id,
                                    MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE
    {
        bool ok = XrayTraits::resolveOwnProperty(cx, jsWrapper, wrapper, holder, id, desc);
        if (!ok || desc.object())
            return ok;

        return SilentFailure(cx, id, "object is not safely Xrayable");
    }

    bool defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                        MutableHandle<JSPropertyDescriptor> desc,
                        Handle<JSPropertyDescriptor> existingDesc, bool *defined)
    {
        *defined = false;
        return true;
    }

    virtual bool enumerateNames(JSContext *cx, HandleObject wrapper, unsigned flags,
                                AutoIdVector &props)
    {
        return true;
    }

    static bool call(JSContext *cx, HandleObject wrapper,
                     const JS::CallArgs &args, const js::Wrapper& baseInstance)
    {
        RootedValue v(cx, ObjectValue(*wrapper));
        js_ReportIsNotFunction(cx, v);
        return false;
    }

    static bool construct(JSContext *cx, HandleObject wrapper,
                          const JS::CallArgs &args, const js::Wrapper& baseInstance)
    {
        RootedValue v(cx, ObjectValue(*wrapper));
        js_ReportIsNotFunction(cx, v);
        return false;
    }

    static bool isResolving(JSContext *cx, JSObject *holder, jsid id)
    {
        return false;
    }

    typedef ResolvingIdDummy ResolvingIdImpl;

    bool getPrototypeOf(JSContext *cx, JS::HandleObject wrapper,
                               JS::HandleObject target,
                               JS::MutableHandleObject protop)
    {
        
        
        
        {
            JSAutoCompartment ac(cx, target);
            if (!JS_GetClassPrototype(cx, JSProto_Object, protop))
                return false;
        }
        return JS_WrapObject(cx, protop);
    }

    static const char *className(JSContext *cx, HandleObject wrapper, const js::Wrapper& baseInstance) {
        return "Opaque";
    }

    virtual void preserveWrapper(JSObject *target) MOZ_OVERRIDE { }

    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE
    {
        RootedObject global(cx, JS_GetGlobalForObject(cx, wrapper));
        return JS_NewObjectWithGivenProto(cx, nullptr, JS::NullPtr(), global);
    }

    static OpaqueXrayTraits singleton;
};

bool
SilentFailure(JSContext *cx, HandleId id, const char *reason)
{
    CompartmentPrivate *priv = CompartmentPrivate::Get(CurrentGlobalOrNull(cx));
    bool alreadyWarnedOnce = priv->warnedAboutXrays;
    priv->warnedAboutXrays = true;

    
    
#ifndef DEBUG
    if (alreadyWarnedOnce)
        return true;
#endif

    nsAutoJSString propertyName;
    if (!propertyName.init(cx, id))
        return false;
    AutoFilename filename;
    unsigned line = 0;
    DescribeScriptedCaller(cx, &filename, &line);

    
    NS_WARNING(nsPrintfCString("Silently denied access to property |%s|: %s (@%s:%u)",
                               NS_LossyConvertUTF16toASCII(propertyName).get(), reason,
                               filename.get(), line).get());

    
    
    
    if (alreadyWarnedOnce)
        return true;

    
    
    

    
    nsCOMPtr<nsIConsoleService> consoleService = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    NS_ENSURE_TRUE(consoleService, true);
    nsCOMPtr<nsIScriptError> errorObject = do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);
    NS_ENSURE_TRUE(errorObject, true);

    
    uint64_t windowId = 0;
    nsGlobalWindow *win = WindowGlobalOrNull(CurrentGlobalOrNull(cx));
    if (win)
      windowId = win->WindowID();

    nsPrintfCString errorMessage("XrayWrapper denied access to property %s (reason: %s). "
                                 "See https://developer.mozilla.org/en-US/docs/Xray_vision "
                                 "for more information. Note that only the first denied "
                                 "property access from a given global object will be reported.",
                                 NS_LossyConvertUTF16toASCII(propertyName).get(),
                                 reason);
    nsString filenameStr(NS_ConvertASCIItoUTF16(filename.get()));
    nsresult rv = errorObject->InitWithWindowID(NS_ConvertASCIItoUTF16(errorMessage),
                                                filenameStr,
                                                EmptyString(),
                                                line, 0,
                                                nsIScriptError::warningFlag,
                                                "XPConnect",
                                                windowId);
    NS_ENSURE_SUCCESS(rv, true);
    rv = consoleService->LogMessage(errorObject);
    NS_ENSURE_SUCCESS(rv, true);

    return true;
}

bool JSXrayTraits::getOwnPropertyFromTargetIfSafe(JSContext *cx,
                                                  HandleObject target,
                                                  HandleObject wrapper,
                                                  HandleId id,
                                                  MutableHandle<JSPropertyDescriptor> outDesc)
{
    
    
    MOZ_ASSERT(getTargetObject(wrapper) == target);
    MOZ_ASSERT(js::IsObjectInContextCompartment(target, cx));
    MOZ_ASSERT(WrapperFactory::IsXrayWrapper(wrapper));
    MOZ_ASSERT(outDesc.object() == nullptr);

    Rooted<JSPropertyDescriptor> desc(cx);
    if (!JS_GetOwnPropertyDescriptorById(cx, target, id, &desc))
        return false;

    
    if (!desc.object())
        return true;

    
    if (desc.hasGetterOrSetter()) {
        JSAutoCompartment ac(cx, wrapper);
        return SilentFailure(cx, id, "property has accessor");
    }

    
    if (desc.value().isObject()) {
        RootedObject propObj(cx, js::UncheckedUnwrap(&desc.value().toObject()));
        JSAutoCompartment ac(cx, propObj);

        
        if (!AccessCheck::subsumes(target, propObj)) {
            JSAutoCompartment ac(cx, wrapper);
            return SilentFailure(cx, id, "value not same-origin with target");
        }

        
        XrayType xrayType = GetXrayType(propObj);
        if (xrayType == NotXray || xrayType == XrayForOpaqueObject) {
            JSAutoCompartment ac(cx, wrapper);
            return SilentFailure(cx, id, "value not Xrayable");
        }

        
        if (JS_ObjectIsCallable(cx, propObj)) {
            JSAutoCompartment ac(cx, wrapper);
            return SilentFailure(cx, id, "value is callable");
        }
    }

    
    
    JSAutoCompartment ac2(cx, wrapper);
    RootedObject proto(cx);
    bool foundOnProto = false;
    if (!JS_GetPrototype(cx, wrapper, &proto) ||
        (proto && !JS_HasPropertyById(cx, proto, id, &foundOnProto)))
    {
        return false;
    }
    if (foundOnProto)
        return SilentFailure(cx, id, "value shadows a property on the standard prototype");

    
    outDesc.assign(desc.get());
    return true;
}

bool
JSXrayTraits::resolveOwnProperty(JSContext *cx, const Wrapper &jsWrapper,
                                 HandleObject wrapper, HandleObject holder,
                                 HandleId id,
                                 MutableHandle<JSPropertyDescriptor> desc)
{
    
    bool ok = XrayTraits::resolveOwnProperty(cx, jsWrapper, wrapper, holder,
                                             id, desc);
    if (!ok || desc.object())
        return ok;

    RootedObject target(cx, getTargetObject(wrapper));
    JSProtoKey key = getProtoKey(holder);
    if (!isPrototype(holder)) {
        
        
        
        
        
        
        
        
        
        if (key == JSProto_Object || key == JSProto_Array) {
            {
                JSAutoCompartment ac(cx, target);
                if (!getOwnPropertyFromTargetIfSafe(cx, target, wrapper, id, desc))
                    return false;
            }
            return JS_WrapPropertyDescriptor(cx, desc);
        } else if (IsTypedArrayKey(key)) {
            if (IsArrayIndex(GetArrayIndexFromId(cx, id))) {
                JS_ReportError(cx, "Accessing TypedArray data over Xrays is slow, and forbidden "
                                   "in order to encourage performant code. To copy TypedArrays "
                                   "across origin boundaries, consider using Components.utils.cloneInto().");
                return false;
            }
        } else if (key == JSProto_Function) {
            if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_LENGTH)) {
                FillPropertyDescriptor(desc, wrapper, JSPROP_PERMANENT | JSPROP_READONLY,
                                       NumberValue(JS_GetFunctionArity(JS_GetObjectFunction(target))));
                return true;
            } else if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_NAME)) {
                RootedString fname(cx, JS_GetFunctionId(JS_GetObjectFunction(target)));
                FillPropertyDescriptor(desc, wrapper, JSPROP_PERMANENT | JSPROP_READONLY,
                                       fname ? StringValue(fname) : JS_GetEmptyStringValue(cx));
            } else if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_PROTOTYPE)) {
                
                JSProtoKey standardConstructor = constructorFor(holder);
                if (standardConstructor != JSProto_Null) {
                    RootedObject standardProto(cx);
                    {
                        JSAutoCompartment ac(cx, target);
                        if (!JS_GetClassPrototype(cx, standardConstructor, &standardProto))
                            return false;
                        MOZ_ASSERT(standardProto);
                    }
                    if (!JS_WrapObject(cx, &standardProto))
                        return false;
                    FillPropertyDescriptor(desc, wrapper, JSPROP_PERMANENT | JSPROP_READONLY,
                                           ObjectValue(*standardProto));
                    return true;
                }
            }
        } else if (IsErrorObjectKey(key)) {
            
            
            
            
            
            
            
            bool isErrorIntProperty =
                id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_LINENUMBER) ||
                id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_COLUMNNUMBER);
            bool isErrorStringProperty =
                id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_FILENAME) ||
                id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_STACK) ||
                id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_MESSAGE);
            if (isErrorIntProperty || isErrorStringProperty) {
                RootedObject waiver(cx, wrapper);
                if (!WrapperFactory::WaiveXrayAndWrap(cx, &waiver))
                    return false;
                if (!JS_GetOwnPropertyDescriptorById(cx, waiver, id, desc))
                    return false;
                bool valueMatchesType = (isErrorIntProperty && desc.value().isInt32()) ||
                                        (isErrorStringProperty && desc.value().isString());
                if (desc.hasGetterOrSetter() || !valueMatchesType)
                    FillPropertyDescriptor(desc, nullptr, 0, UndefinedValue());
                return true;
            }
        }

        
        return true;
    }

    
    
    
    
    
    if (!JS_GetPropertyDescriptorById(cx, holder, id, desc))
        return false;
    if (desc.object()) {
        desc.object().set(wrapper);
        return true;
    }

    
    const js::Class *clasp = js::GetObjectClass(target);
    MOZ_ASSERT(clasp->spec.defined());
    JSProtoKey protoKey = getProtoKey(holder);

    
    if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_CONSTRUCTOR)) {
        RootedObject constructor(cx);
        {
            JSAutoCompartment ac(cx, target);
            if (!JS_GetClassObject(cx, protoKey, &constructor))
                return false;
        }
        if (!JS_WrapObject(cx, &constructor))
            return false;
        desc.object().set(wrapper);
        desc.setAttributes(0);
        desc.setGetter(nullptr);
        desc.setSetter(nullptr);
        desc.value().setObject(*constructor);
        return true;
    }

    
    if (IsErrorObjectKey(key) && id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_NAME)) {
        RootedId className(cx);
        ProtoKeyToId(cx, key, &className);
        FillPropertyDescriptor(desc, wrapper, 0, UndefinedValue());
        return JS_IdToValue(cx, className, desc.value());
    }

    
    
    if (js::StandardClassIsDependent(protoKey))
        return true;

    
    
    
    if (!JSID_IS_STRING(id))
        return true;
    Rooted<JSFlatString*> str(cx, JSID_TO_FLAT_STRING(id));

    
    const JSFunctionSpec *fsMatch = nullptr;
    for (const JSFunctionSpec *fs = clasp->spec.prototypeFunctions; fs && fs->name; ++fs) {
        if (JS_FlatStringEqualsAscii(str, fs->name)) {
            fsMatch = fs;
            break;
        }
    }
    if (fsMatch) {
        
        RootedFunction fun(cx);
        if (fsMatch->selfHostedName) {
            fun = JS::GetSelfHostedFunction(cx, fsMatch->selfHostedName, id, fsMatch->nargs);
        } else {
            fun = JS_NewFunctionById(cx, fsMatch->call.op, fsMatch->nargs,
                                     0, wrapper, id);
        }
        if (!fun)
            return false;

        
        
        
        
        RootedObject funObj(cx, JS_GetFunctionObject(fun));
        return JS_DefinePropertyById(cx, holder, id, funObj, 0) &&
               JS_GetPropertyDescriptorById(cx, holder, id, desc);
    }

    
    const JSPropertySpec *psMatch = nullptr;
    for (const JSPropertySpec *ps = clasp->spec.prototypeProperties; ps && ps->name; ++ps) {
        if (JS_FlatStringEqualsAscii(str, ps->name)) {
            psMatch = ps;
            break;
        }
    }
    if (psMatch) {
        desc.value().setUndefined();
        
        
        RootedFunction getterObj(cx);
        RootedFunction setterObj(cx);
        unsigned flags = psMatch->flags;
        if (flags & JSPROP_NATIVE_ACCESSORS) {
            desc.setGetter(psMatch->getter.propertyOp.op);
            desc.setSetter(psMatch->setter.propertyOp.op);
        } else {
            MOZ_ASSERT(flags & JSPROP_GETTER);
            getterObj = JS::GetSelfHostedFunction(cx, psMatch->getter.selfHosted.funname, id, 0);
            if (!getterObj)
                return false;
            desc.setGetterObject(JS_GetFunctionObject(getterObj));
            if (psMatch->setter.selfHosted.funname) {
                MOZ_ASSERT(flags & JSPROP_SETTER);
                setterObj = JS::GetSelfHostedFunction(cx, psMatch->setter.selfHosted.funname, id, 0);
                if (!setterObj)
                    return false;
                desc.setSetterObject(JS_GetFunctionObject(setterObj));
            }
        }
        desc.setAttributes(flags);

        
        
        
        
        
        
        
        
        return JS_DefinePropertyById(cx, holder, id,
                                     UndefinedHandleValue, desc.attributes(),
                                     desc.getter(), desc.setter()) &&
               JS_GetPropertyDescriptorById(cx, holder, id, desc);
    }

    return true;
}

bool
JSXrayTraits::delete_(JSContext *cx, HandleObject wrapper, HandleId id, bool *bp)
{
    RootedObject holder(cx, ensureHolder(cx, wrapper));

    
    
    
    JSProtoKey key = getProtoKey(holder);
    bool isObjectOrArrayInstance = (key == JSProto_Object || key == JSProto_Array) &&
                                   !isPrototype(holder);
    if (isObjectOrArrayInstance) {
        RootedObject target(cx, getTargetObject(wrapper));
        JSAutoCompartment ac(cx, target);
        Rooted<JSPropertyDescriptor> desc(cx);
        if (!getOwnPropertyFromTargetIfSafe(cx, target, wrapper, id, &desc))
            return false;
        if (desc.object())
            return JS_DeletePropertyById2(cx, target, id, bp);
    }
    *bp = true;
    return true;
}

bool
JSXrayTraits::defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                               MutableHandle<JSPropertyDescriptor> desc,
                               Handle<JSPropertyDescriptor> existingDesc,
                               bool *defined)
{
    *defined = false;
    RootedObject holder(cx, ensureHolder(cx, wrapper));
    if (!holder)
        return false;


    
    
    
    
    
    
    
    
    
    JSProtoKey key = getProtoKey(holder);
    bool isObjectOrArrayInstance = (key == JSProto_Object || key == JSProto_Array) &&
                                   !isPrototype(holder);
    if (isObjectOrArrayInstance) {
        RootedObject target(cx, getTargetObject(wrapper));
        if (desc.hasGetterOrSetter()) {
            JS_ReportError(cx, "Not allowed to define accessor property on [Object] or [Array] XrayWrapper");
            return false;
        }
        if (desc.value().isObject() &&
            !AccessCheck::subsumes(target, js::UncheckedUnwrap(&desc.value().toObject())))
        {
            JS_ReportError(cx, "Not allowed to define cross-origin object as property on [Object] or [Array] XrayWrapper");
            return false;
        }
        if (existingDesc.hasGetterOrSetter()) {
            JS_ReportError(cx, "Not allowed to overwrite accessor property on [Object] or [Array] XrayWrapper");
            return false;
        }
        if (existingDesc.object() && existingDesc.object() != wrapper) {
            JS_ReportError(cx, "Not allowed to shadow non-own Xray-resolved property on [Object] or [Array] XrayWrapper");
            return false;
        }

        JSAutoCompartment ac(cx, target);
        if (!JS_WrapPropertyDescriptor(cx, desc) ||
            !JS_DefinePropertyById(cx, target, id, desc.value(), desc.attributes(),
                                   JS_PropertyStub, JS_StrictPropertyStub))
        {
            return false;
        }
        *defined = true;
        return true;
    }

    return true;
}

bool
JSXrayTraits::enumerateNames(JSContext *cx, HandleObject wrapper, unsigned flags,
                             AutoIdVector &props)
{
    RootedObject target(cx, getTargetObject(wrapper));
    RootedObject holder(cx, ensureHolder(cx, wrapper));
    if (!holder)
        return false;

    JSProtoKey key = getProtoKey(holder);
    if (!isPrototype(holder)) {
        
        
        if (key == JSProto_Object || key == JSProto_Array) {
            MOZ_ASSERT(props.empty());
            {
                JSAutoCompartment ac(cx, target);
                AutoIdVector targetProps(cx);
                if (!js::GetPropertyNames(cx, target, flags | JSITER_OWNONLY, &targetProps))
                    return false;
                
                
                if (!props.reserve(targetProps.length()))
                    return false;
                for (size_t i = 0; i < targetProps.length(); ++i) {
                    Rooted<JSPropertyDescriptor> desc(cx);
                    RootedId id(cx, targetProps[i]);
                    if (!getOwnPropertyFromTargetIfSafe(cx, target, wrapper, id, &desc))
                        return false;
                    if (desc.object())
                        props.infallibleAppend(id);
                }
            }
            return true;
        } else if (IsTypedArrayKey(key)) {
            uint32_t length = JS_GetTypedArrayLength(target);
            
            
            if (!props.reserve(length))
                return false;
            for (int32_t i = 0; i <= int32_t(length - 1); ++i)
                props.infallibleAppend(INT_TO_JSID(i));
        } else if (key == JSProto_Function) {
            if (!props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_LENGTH)))
                return false;
            if (!props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_NAME)))
                return false;
            
            if (constructorFor(holder) != JSProto_Null) {
                if (!props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_PROTOTYPE)))
                    return false;
            }
        } else if (IsErrorObjectKey(key)) {
            if (!props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_FILENAME)) ||
                !props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_LINENUMBER)) ||
                !props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_COLUMNNUMBER)) ||
                !props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_STACK)) ||
                !props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_MESSAGE)))
            {
                return false;
            }
        }

        
        return true;
    }

    
    const js::Class *clasp = js::GetObjectClass(target);
    MOZ_ASSERT(clasp->spec.defined());
    JSProtoKey protoKey = getProtoKey(holder);

    
    if (!props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_CONSTRUCTOR)))
        return false;

    
    if (IsErrorObjectKey(key) && !props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_NAME)))
        return false;

    
    
    if (js::StandardClassIsDependent(protoKey))
        return true;

    
    for (const JSFunctionSpec *fs = clasp->spec.prototypeFunctions; fs && fs->name; ++fs) {
        RootedString str(cx, JS_InternString(cx, fs->name));
        if (!str)
            return false;
        if (!props.append(INTERNED_STRING_TO_JSID(cx, str)))
            return false;
    }
    for (const JSPropertySpec *ps = clasp->spec.prototypeProperties; ps && ps->name; ++ps) {
        
        
        
        
        MOZ_ASSERT(ps->flags & JSPROP_NATIVE_ACCESSORS,
                   "Self-hosted accessor added to Xrayable class - ping the XPConnect "
                   "module owner about adding test coverage");
        RootedString str(cx, JS_InternString(cx, ps->name));
        if (!str)
            return false;
        if (!props.append(INTERNED_STRING_TO_JSID(cx, str)))
            return false;
    }

    return true;
}

JSObject*
JSXrayTraits::createHolder(JSContext *cx, JSObject *wrapper)
{
    RootedObject global(cx, JS_GetGlobalForObject(cx, wrapper));
    RootedObject target(cx, getTargetObject(wrapper));
    RootedObject holder(cx, JS_NewObjectWithGivenProto(cx, &HolderClass,
                                                       JS::NullPtr(), global));
    if (!holder)
        return nullptr;

    
    bool isPrototype = false;
    JSProtoKey key = IdentifyStandardInstance(target);
    if (key == JSProto_Null) {
        isPrototype = true;
        key = IdentifyStandardPrototype(target);
    }
    MOZ_ASSERT(key != JSProto_Null);

    
    RootedValue v(cx);
    v.setNumber(static_cast<uint32_t>(key));
    js::SetReservedSlot(holder, SLOT_PROTOKEY, v);
    v.setBoolean(isPrototype);
    js::SetReservedSlot(holder, SLOT_ISPROTOTYPE, v);

    
    
    if (key == JSProto_Function) {
        v.setNumber(static_cast<uint32_t>(IdentifyStandardConstructor(target)));
        js::SetReservedSlot(holder, SLOT_CONSTRUCTOR_FOR, v);
    }

    return holder;
}

XPCWrappedNativeXrayTraits XPCWrappedNativeXrayTraits::singleton;
DOMXrayTraits DOMXrayTraits::singleton;
JSXrayTraits JSXrayTraits::singleton;
OpaqueXrayTraits OpaqueXrayTraits::singleton;

XrayTraits*
GetXrayTraits(JSObject *obj)
{
    switch (GetXrayType(obj)) {
      case XrayForDOMObject:
        return &DOMXrayTraits::singleton;
      case XrayForWrappedNative:
        return &XPCWrappedNativeXrayTraits::singleton;
      case XrayForJSObject:
        return &JSXrayTraits::singleton;
      case XrayForOpaqueObject:
        return &OpaqueXrayTraits::singleton;
      default:
        return nullptr;
    }
}
















enum ExpandoSlots {
    JSSLOT_EXPANDO_NEXT = 0,
    JSSLOT_EXPANDO_ORIGIN,
    JSSLOT_EXPANDO_EXCLUSIVE_GLOBAL,
    JSSLOT_EXPANDO_PROTOTYPE,
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
    Value v = JS_GetReservedSlot(expandoObject, JSSLOT_EXPANDO_ORIGIN);
    return static_cast<nsIPrincipal*>(v.toPrivate());
}

static void
ExpandoObjectFinalize(JSFreeOp *fop, JSObject *obj)
{
    
    nsIPrincipal *principal = GetExpandoObjectPrincipal(obj);
    NS_RELEASE(principal);
}

const JSClass ExpandoObjectClass = {
    "XrayExpandoObject",
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_EXPANDO_COUNT),
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, ExpandoObjectFinalize
};

bool
XrayTraits::expandoObjectMatchesConsumer(JSContext *cx,
                                         HandleObject expandoObject,
                                         nsIPrincipal *consumerOrigin,
                                         HandleObject exclusiveGlobal)
{
    MOZ_ASSERT(js::IsObjectInContextCompartment(expandoObject, cx));

    
    nsIPrincipal *o = GetExpandoObjectPrincipal(expandoObject);
    
    
    
    
    
    
    
    if (!consumerOrigin->Equals(o))
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
XrayTraits::getExpandoObjectInternal(JSContext *cx, HandleObject target,
                                     nsIPrincipal *origin,
                                     JSObject *exclusiveGlobalArg)
{
    
    
    RootedObject exclusiveGlobal(cx, exclusiveGlobalArg);
    JSAutoCompartment ac(cx, target);
    if (!JS_WrapObject(cx, &exclusiveGlobal))
        return nullptr;

    
    RootedObject head(cx, getExpandoChain(target));
    while (head) {
        if (expandoObjectMatchesConsumer(cx, head, origin, exclusiveGlobal))
            return head;
        head = JS_GetReservedSlot(head, JSSLOT_EXPANDO_NEXT).toObjectOrNull();
    }

    
    return nullptr;
}

JSObject *
XrayTraits::getExpandoObject(JSContext *cx, HandleObject target, HandleObject consumer)
{
    JSObject *consumerGlobal = js::GetGlobalForObjectCrossCompartment(consumer);
    bool isSandbox = !strcmp(js::GetObjectJSClass(consumerGlobal)->name, "Sandbox");
    return getExpandoObjectInternal(cx, target, ObjectPrincipal(consumer),
                                    isSandbox ? consumerGlobal : nullptr);
}

JSObject *
XrayTraits::attachExpandoObject(JSContext *cx, HandleObject target,
                                nsIPrincipal *origin, HandleObject exclusiveGlobal)
{
    
    MOZ_ASSERT(js::IsObjectInContextCompartment(target, cx));
    MOZ_ASSERT(!exclusiveGlobal || js::IsObjectInContextCompartment(exclusiveGlobal, cx));

    
    MOZ_ASSERT(!getExpandoObjectInternal(cx, target, origin, exclusiveGlobal));

    
    RootedObject expandoObject(cx, JS_NewObjectWithGivenProto(cx, &ExpandoObjectClass,
                                                              JS::NullPtr(), target));
    if (!expandoObject)
        return nullptr;

    
    NS_ADDREF(origin);
    JS_SetReservedSlot(expandoObject, JSSLOT_EXPANDO_ORIGIN, PRIVATE_TO_JSVAL(origin));

    
    JS_SetReservedSlot(expandoObject, JSSLOT_EXPANDO_EXCLUSIVE_GLOBAL,
                       OBJECT_TO_JSVAL(exclusiveGlobal));

    
    
    
    RootedObject chain(cx, getExpandoChain(target));
    if (!chain)
        preserveWrapper(target);

    
    JS_SetReservedSlot(expandoObject, JSSLOT_EXPANDO_NEXT, OBJECT_TO_JSVAL(chain));
    setExpandoChain(cx, target, expandoObject);

    return expandoObject;
}

JSObject *
XrayTraits::ensureExpandoObject(JSContext *cx, HandleObject wrapper,
                                HandleObject target)
{
    
    JSAutoCompartment ac(cx, target);
    JSObject *expandoObject = getExpandoObject(cx, target, wrapper);
    if (!expandoObject) {
        
        
        
        
        
        RootedObject consumerGlobal(cx, js::GetGlobalForObjectCrossCompartment(wrapper));
        bool isSandbox = !strcmp(js::GetObjectJSClass(consumerGlobal)->name, "Sandbox");
        if (!JS_WrapObject(cx, &consumerGlobal))
            return nullptr;
        expandoObject = attachExpandoObject(cx, target, ObjectPrincipal(wrapper),
                                            isSandbox ? (HandleObject)consumerGlobal : NullPtr());
    }
    return expandoObject;
}

bool
XrayTraits::cloneExpandoChain(JSContext *cx, HandleObject dst, HandleObject src)
{
    MOZ_ASSERT(js::IsObjectInContextCompartment(dst, cx));
    MOZ_ASSERT(getExpandoChain(dst) == nullptr);

    RootedObject oldHead(cx, getExpandoChain(src));
    while (oldHead) {
        RootedObject exclusive(cx, JS_GetReservedSlot(oldHead,
                                                      JSSLOT_EXPANDO_EXCLUSIVE_GLOBAL)
                                                     .toObjectOrNull());
        if (!JS_WrapObject(cx, &exclusive))
            return false;
        RootedObject newHead(cx, attachExpandoObject(cx, dst, GetExpandoObjectPrincipal(oldHead),
                                                     exclusive));
        if (!JS_CopyPropertiesFrom(cx, newHead, oldHead))
            return false;
        oldHead = JS_GetReservedSlot(oldHead, JSSLOT_EXPANDO_NEXT).toObjectOrNull();
    }
    return true;
}

namespace XrayUtils {
bool CloneExpandoChain(JSContext *cx, JSObject *dstArg, JSObject *srcArg)
{
    RootedObject dst(cx, dstArg);
    RootedObject src(cx, srcArg);
    return GetXrayTraits(src)->cloneExpandoChain(cx, dst, src);
}
}

static JSObject *
GetHolder(JSObject *obj)
{
    return &js::GetProxyExtra(obj, 0).toObject();
}

JSObject*
XrayTraits::getHolder(JSObject *wrapper)
{
    MOZ_ASSERT(WrapperFactory::IsXrayWrapper(wrapper));
    js::Value v = js::GetProxyExtra(wrapper, 0);
    return v.isObject() ? &v.toObject() : nullptr;
}

JSObject*
XrayTraits::ensureHolder(JSContext *cx, HandleObject wrapper)
{
    RootedObject holder(cx, getHolder(wrapper));
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

namespace XrayUtils {

bool
IsXPCWNHolderClass(const JSClass *clasp)
{
  return clasp == &XPCWrappedNativeXrayTraits::HolderClass;
}

}





bool
holder_get(JSContext *cx, HandleObject wrapper, HandleId id, MutableHandleValue vp)
{
    
    
    
    NS_ENSURE_TRUE(WrapperFactory::IsXrayWrapper(wrapper), true);
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

bool
holder_set(JSContext *cx, HandleObject wrapper, HandleId id, bool strict, MutableHandleValue vp)
{
    
    
    
    NS_ENSURE_TRUE(WrapperFactory::IsXrayWrapper(wrapper), true);
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
    AutoSetWrapperNotShadowing(ResolvingId *resolvingId MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        MOZ_ASSERT(resolvingId);
        mResolvingId = resolvingId;
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
XPCWrappedNativeXrayTraits::resolveDOMCollectionProperty(JSContext *cx, HandleObject wrapper,
                                                         HandleObject holder, HandleId id,
                                                         MutableHandle<JSPropertyDescriptor> desc)
{
    
    
    ResolvingId *rid = ResolvingId::getResolvingId(holder);
    if (!rid || rid->mId != id)
        return true;

    XPCWrappedNative *wn = getWN(wrapper);
    if (!wn) {
        
        
        XPCThrower::Throw(NS_ERROR_UNEXPECTED, cx);
        return false;
    }
    if (!NATIVE_HAS_FLAG(wn, WantNewResolve))
        return true;

    ResolvingId *resolvingId = ResolvingId::getResolvingIdFromWrapper(wrapper);
    if (!resolvingId) {
        
        
        XPCThrower::Throw(NS_ERROR_UNEXPECTED, cx);
        return false;
    }

    
    
    AutoSetWrapperNotShadowing asw(resolvingId);

    bool retval = true;
    RootedObject pobj(cx);
    nsresult rv = wn->GetScriptableInfo()->GetCallback()->NewResolve(wn, cx, wrapper, id,
                                                                     pobj.address(), &retval);
    if (NS_FAILED(rv)) {
        if (retval)
            XPCThrower::Throw(rv, cx);
        return false;
    }

    if (pobj && !JS_GetPropertyDescriptorById(cx, holder, id, desc))
        return false;

    return true;
}

static nsGlobalWindow*
AsWindow(JSContext *cx, JSObject *wrapper)
{
  nsGlobalWindow* win;
  
  
  JSObject* target = XrayTraits::getTargetObject(wrapper);
  nsresult rv = UNWRAP_OBJECT(Window, target, win);
  if (NS_SUCCEEDED(rv))
      return win;

  nsCOMPtr<nsPIDOMWindow> piWin = do_QueryInterface(
      nsContentUtils::XPConnect()->GetNativeOfWrapper(cx, target));
  return static_cast<nsGlobalWindow*>(piWin.get());
}

static bool
IsWindow(JSContext *cx, JSObject *wrapper)
{
    return !!AsWindow(cx, wrapper);
}

static nsQueryInterface
do_QueryInterfaceNative(JSContext* cx, HandleObject wrapper);

void
XPCWrappedNativeXrayTraits::preserveWrapper(JSObject *target)
{
    XPCWrappedNative *wn = XPCWrappedNative::Get(target);
    nsRefPtr<nsXPCClassInfo> ci;
    CallQueryInterface(wn->Native(), getter_AddRefs(ci));
    if (ci)
        ci->PreserveWrapper(wn->Native());
}

bool
XPCWrappedNativeXrayTraits::resolveNativeProperty(JSContext *cx, HandleObject wrapper,
                                                  HandleObject holder, HandleId id,
                                                  MutableHandle<JSPropertyDescriptor> desc)
{
    MOZ_ASSERT(js::GetObjectJSClass(holder) == &HolderClass);

    desc.object().set(nullptr);

    
    RootedObject target(cx, getTargetObject(wrapper));
    XPCCallContext ccx(JS_CALLER, cx, target, NullPtr(), id);

    
    
    
    
    if (!JSID_IS_STRING(id)) {
        
        return resolveDOMCollectionProperty(cx, wrapper, holder, id, desc);
    }


    
    
    
    nsGlobalWindow *win = nullptr;
    if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_CONTROLLERS) &&
        AccessCheck::isChrome(wrapper) &&
        (win = AsWindow(cx, wrapper)))
    {
        nsCOMPtr<nsIControllers> c;
        nsresult rv = win->GetControllers(getter_AddRefs(c));
        if (NS_SUCCEEDED(rv) && c) {
            rv = nsXPConnect::XPConnect()->WrapNativeToJSVal(cx, CurrentGlobalOrNull(cx),
                                                             c, nullptr, nullptr, true,
                                                             desc.value());
        }

        if (NS_FAILED(rv) || !c) {
            JS_ReportError(cx, "Failed to invoke GetControllers via Xrays");
            return false;
        }

        desc.object().set(wrapper);
        return true;
    }

    
    
    
    if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_REALFRAMEELEMENT) &&
        AccessCheck::isChrome(wrapper) &&
        (win = AsWindow(cx, wrapper)))
    {
        ErrorResult rv;
        Element* f = win->GetRealFrameElement(rv);
        if (!f) {
          desc.object().set(nullptr);
          return true;
        }

        if (!WrapNewBindingObject(cx, f, desc.value())) {
          return false;
        }

        desc.object().set(wrapper);
        return true;
    }

    XPCNativeInterface *iface;
    XPCNativeMember *member;
    XPCWrappedNative *wn = getWN(wrapper);

    if (ccx.GetWrapper() != wn || !wn->IsValid()) {
        
        
        return true;
    } else if (!(iface = ccx.GetInterface()) ||
               !(member = ccx.GetMember())) {
        
        return resolveDOMCollectionProperty(cx, wrapper, holder, id, desc);
    }

    desc.object().set(holder);
    desc.setAttributes(JSPROP_ENUMERATE);
    desc.setGetter(nullptr);
    desc.setSetter(nullptr);
    desc.value().set(JSVAL_VOID);

    RootedValue fval(cx, JSVAL_VOID);
    if (member->IsConstant()) {
        if (!member->GetConstantValue(ccx, iface, desc.value().address())) {
            JS_ReportError(cx, "Failed to convert constant native property to JS value");
            return false;
        }
    } else if (member->IsAttribute()) {
        
        if (!member->NewFunctionObject(ccx, iface, wrapper, fval.address())) {
            JS_ReportError(cx, "Failed to clone function object for native getter/setter");
            return false;
        }

        unsigned attrs = desc.attributes();
        attrs |= JSPROP_GETTER;
        if (member->IsWritableAttribute())
            attrs |= JSPROP_SETTER;

        
        
        attrs |= JSPROP_SHARED;
        desc.setAttributes(attrs);
    } else {
        
        if (!member->NewFunctionObject(ccx, iface, wrapper, desc.value().address())) {
            JS_ReportError(cx, "Failed to clone function object for native function");
            return false;
        }

        
        
        
        
        desc.setGetter(JS_PropertyStub);
        desc.setSetter(JS_StrictPropertyStub);
    }

    if (!JS_WrapValue(cx, desc.value()) || !JS_WrapValue(cx, &fval))
        return false;

    if (desc.hasGetterObject())
        desc.setGetterObject(&fval.toObject());
    if (desc.hasSetterObject())
        desc.setSetterObject(&fval.toObject());

    
    return JS_DefinePropertyById(cx, holder, id, desc.value(), desc.attributes(),
                                 desc.getter(), desc.setter());
}

static bool
wrappedJSObject_getter(JSContext *cx, HandleObject wrapper, HandleId id, MutableHandleValue vp)
{
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper)) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    vp.set(OBJECT_TO_JSVAL(wrapper));

    return WrapperFactory::WaiveXrayAndWrap(cx, vp);
}

bool
XrayTraits::resolveOwnProperty(JSContext *cx, const Wrapper &jsWrapper,
                               HandleObject wrapper, HandleObject holder, HandleId id,
                               MutableHandle<JSPropertyDescriptor> desc)
{
    desc.object().set(nullptr);
    RootedObject target(cx, getTargetObject(wrapper));
    RootedObject expando(cx, getExpandoObject(cx, target, wrapper));

    
    
    bool found = false;
    if (expando) {
        JSAutoCompartment ac(cx, expando);
        if (!JS_GetPropertyDescriptorById(cx, expando, id, desc))
            return false;
        found = !!desc.object();
    }

    
    if (!found && JS_IsGlobalObject(target)) {
        JSProtoKey key = JS_IdToProtoKey(cx, id);
        JSAutoCompartment ac(cx, target);
        if (key != JSProto_Null) {
            MOZ_ASSERT(key < JSProto_LIMIT);
            RootedObject constructor(cx);
            if (!JS_GetClassObject(cx, key, &constructor))
                return false;
            MOZ_ASSERT(constructor);
            desc.value().set(ObjectValue(*constructor));
            found = true;
        } else if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_EVAL)) {
            RootedObject eval(cx);
            if (!js::GetOriginalEval(cx, target, &eval))
                return false;
            desc.value().set(ObjectValue(*eval));
            found = true;
        }
    }

    if (found) {
        if (!JS_WrapPropertyDescriptor(cx, desc))
            return false;
        
        desc.object().set(wrapper);
        return true;
    }

    
    
    if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_WRAPPED_JSOBJECT) &&
        AccessCheck::wrapperSubsumes(wrapper))
    {
        if (!JS_AlreadyHasOwnPropertyById(cx, holder, id, &found))
            return false;
        if (!found && !JS_DefinePropertyById(cx, holder, id, UndefinedHandleValue,
                                             JSPROP_ENUMERATE | JSPROP_SHARED,
                                             wrappedJSObject_getter)) {
            return false;
        }
        if (!JS_GetPropertyDescriptorById(cx, holder, id, desc))
            return false;
        desc.object().set(wrapper);
        return true;
    }

    return true;
}

bool
XrayTraits::set(JSContext *cx, HandleObject wrapper, HandleObject receiver, HandleId id,
                bool strict, MutableHandleValue vp)
{
    
    const js::BaseProxyHandler *handler = js::GetProxyHandler(wrapper);
    return handler->js::BaseProxyHandler::set(cx, wrapper, receiver, id, strict, vp);
}

bool
XPCWrappedNativeXrayTraits::resolveOwnProperty(JSContext *cx, const Wrapper &jsWrapper,
                                               HandleObject wrapper, HandleObject holder,
                                               HandleId id,
                                               MutableHandle<JSPropertyDescriptor> desc)
{
    
    bool ok = XrayTraits::resolveOwnProperty(cx, jsWrapper, wrapper, holder,
                                             id, desc);
    if (!ok || desc.object())
        return ok;

    
    int32_t index = GetArrayIndexFromId(cx, id);
    if (IsArrayIndex(index)) {
        nsGlobalWindow* win = AsWindow(cx, wrapper);
        
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
                desc.value().setObject(*obj);
                FillPropertyDescriptor(desc, wrapper, true);
                return JS_WrapPropertyDescriptor(cx, desc);
            }
        }
    }

    
    
    MOZ_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));

    bool hasProp;
    if (!JS_HasPropertyById(cx, holder, id, &hasProp)) {
        return false;
    }
    if (!hasProp) {
        XPCWrappedNative *wn = getWN(wrapper);

        
        if (!NATIVE_HAS_FLAG(wn, WantNewResolve)) {
            return true;
        }

        bool retval = true;
        RootedObject pobj(cx);
        nsIXPCScriptable *callback = wn->GetScriptableInfo()->GetCallback();
        nsresult rv = callback->NewResolve(wn, cx, wrapper, id, pobj.address(),
                                           &retval);
        if (NS_FAILED(rv)) {
            if (retval)
                XPCThrower::Throw(rv, cx);
            return false;
        }

        MOZ_ASSERT(!pobj || (JS_HasPropertyById(cx, holder, id, &hasProp) &&
                             hasProp), "id got defined somewhere else?");
    }

    
    
    
    
    
    
    
    
    
    
    
    
    return JS_GetPropertyDescriptorById(cx, holder, id, desc);
}

bool
XPCWrappedNativeXrayTraits::defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                                           MutableHandle<JSPropertyDescriptor> desc,
                                           Handle<JSPropertyDescriptor> existingDesc, bool *defined)
{
    *defined = false;
    RootedObject holder(cx, singleton.ensureHolder(cx, wrapper));
    if (isResolving(cx, holder, id)) {
        if (!desc.hasAttributes(JSPROP_GETTER | JSPROP_SETTER)) {
            if (!desc.getter())
                desc.setGetter(holder_get);
            if (!desc.setter())
                desc.setSetter(holder_set);
        }

        *defined = true;
        return JS_DefinePropertyById(cx, holder, id, desc.value(), desc.attributes(),
                                     desc.getter(), desc.setter());
    }

    
    
    int32_t index = GetArrayIndexFromId(cx, id);
    if (IsArrayIndex(index) && IsWindow(cx, wrapper)) {
        *defined = true;
        return true;
    }

    return true;
}

bool
XPCWrappedNativeXrayTraits::enumerateNames(JSContext *cx, HandleObject wrapper, unsigned flags,
                                           AutoIdVector &props)
{
    
    AutoIdVector wnProps(cx);
    {
        RootedObject target(cx, singleton.getTargetObject(wrapper));
        JSAutoCompartment ac(cx, target);
        if (!js::GetPropertyNames(cx, target, flags, &wnProps))
            return false;
    }

    
    
    
    MOZ_ASSERT(!JS_IsExceptionPending(cx));
    if (!props.reserve(wnProps.length()))
        return false;
    for (size_t n = 0; n < wnProps.length(); ++n) {
        RootedId id(cx, wnProps[n]);
        bool hasProp;
        if (JS_HasPropertyById(cx, wrapper, id, &hasProp) && hasProp)
            props.infallibleAppend(id);
        JS_ClearPendingException(cx);
    }
    return true;
}

JSObject *
XPCWrappedNativeXrayTraits::createHolder(JSContext *cx, JSObject *wrapper)
{
    RootedObject global(cx, JS_GetGlobalForObject(cx, wrapper));
    JSObject *holder = JS_NewObjectWithGivenProto(cx, &HolderClass, JS::NullPtr(),
                                                  global);
    if (!holder)
        return nullptr;

    js::SetReservedSlot(holder, JSSLOT_RESOLVING, PrivateValue(nullptr));
    return holder;
}

bool
XPCWrappedNativeXrayTraits::call(JSContext *cx, HandleObject wrapper,
                                 const JS::CallArgs &args,
                                 const js::Wrapper& baseInstance)
{
    
    XPCWrappedNative *wn = getWN(wrapper);
    if (NATIVE_HAS_FLAG(wn, WantCall)) {
        XPCCallContext ccx(JS_CALLER, cx, wrapper, NullPtr(), JSID_VOIDHANDLE, args.length(),
                           args.array(), args.rval().address());
        if (!ccx.IsValid())
            return false;
        bool ok = true;
        nsresult rv = wn->GetScriptableInfo()->GetCallback()->Call(
            wn, cx, wrapper, args, &ok);
        if (NS_FAILED(rv)) {
            if (ok)
                XPCThrower::Throw(rv, cx);
            return false;
        }
    }

    return true;

}

bool
XPCWrappedNativeXrayTraits::construct(JSContext *cx, HandleObject wrapper,
                                      const JS::CallArgs &args,
                                      const js::Wrapper& baseInstance)
{
    
    XPCWrappedNative *wn = getWN(wrapper);
    if (NATIVE_HAS_FLAG(wn, WantConstruct)) {
        XPCCallContext ccx(JS_CALLER, cx, wrapper, NullPtr(), JSID_VOIDHANDLE, args.length(),
                           args.array(), args.rval().address());
        if (!ccx.IsValid())
            return false;
        bool ok = true;
        nsresult rv = wn->GetScriptableInfo()->GetCallback()->Construct(
            wn, cx, wrapper, args, &ok);
        if (NS_FAILED(rv)) {
            if (ok)
                XPCThrower::Throw(rv, cx);
            return false;
        }
    }

    return true;

}

bool
DOMXrayTraits::resolveNativeProperty(JSContext *cx, HandleObject wrapper,
                                     HandleObject holder, HandleId id,
                                     MutableHandle<JSPropertyDescriptor> desc)
{
    bool unused;
    RootedObject obj(cx, getTargetObject(wrapper));
    if (!XrayResolveNativeProperty(cx, wrapper, obj, id, desc, unused))
        return false;

    MOZ_ASSERT(!desc.object() || desc.object() == wrapper, "What did we resolve this on?");

    return true;
}

bool
DOMXrayTraits::resolveOwnProperty(JSContext *cx, const Wrapper &jsWrapper, HandleObject wrapper,
                                  HandleObject holder, HandleId id,
                                  MutableHandle<JSPropertyDescriptor> desc)
{
    
    bool ok = XrayTraits::resolveOwnProperty(cx, jsWrapper, wrapper, holder, id, desc);
    if (!ok || desc.object())
        return ok;

    
    int32_t index = GetArrayIndexFromId(cx, id);
    if (IsArrayIndex(index)) {
        nsGlobalWindow* win = AsWindow(cx, wrapper);
        
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
                desc.value().setObject(*obj);
                FillPropertyDescriptor(desc, wrapper, true);
                return JS_WrapPropertyDescriptor(cx, desc);
            }
        }
    }

    if (!JS_GetPropertyDescriptorById(cx, holder, id, desc))
        return false;
    if (desc.object()) {
        desc.object().set(wrapper);
        return true;
    }

    RootedObject obj(cx, getTargetObject(wrapper));
    bool cacheOnHolder;
    if (!XrayResolveOwnProperty(cx, wrapper, obj, id, desc, cacheOnHolder))
        return false;

    MOZ_ASSERT(!desc.object() || desc.object() == wrapper, "What did we resolve this on?");

    if (!desc.object() || !cacheOnHolder)
        return true;

    return JS_DefinePropertyById(cx, holder, id, desc.value(), desc.attributes(),
                                 desc.getter(), desc.setter()) &&
           JS_GetPropertyDescriptorById(cx, holder, id, desc);
}

bool
DOMXrayTraits::defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                              MutableHandle<JSPropertyDescriptor> desc,
                              Handle<JSPropertyDescriptor> existingDesc, bool *defined)
{
    
    
    if (IsWindow(cx, wrapper)) {
        int32_t index = GetArrayIndexFromId(cx, id);
        if (IsArrayIndex(index)) {
            *defined = true;
            return true;
        }
    }

    JS::Rooted<JSObject*> obj(cx, getTargetObject(wrapper));
    return XrayDefineProperty(cx, wrapper, obj, id, desc, defined);
}

bool
DOMXrayTraits::set(JSContext *cx, HandleObject wrapper, HandleObject receiver, HandleId id,
                   bool strict, MutableHandleValue vp)
{
    MOZ_ASSERT(xpc::WrapperFactory::IsXrayWrapper(wrapper));
    RootedObject obj(cx, getTargetObject(wrapper));
    if (IsDOMProxy(obj)) {
        const DOMProxyHandler* handler = GetDOMProxyHandler(obj);

        bool done;
        if (!handler->setCustom(cx, obj, id, vp, &done))
            return false;
        if (done)
            return true;
    }
    return XrayTraits::set(cx, wrapper, receiver, id, strict, vp);
}

bool
DOMXrayTraits::enumerateNames(JSContext *cx, HandleObject wrapper, unsigned flags,
                              AutoIdVector &props)
{
    JS::Rooted<JSObject*> obj(cx, getTargetObject(wrapper));
    return XrayEnumerateProperties(cx, wrapper, obj, flags, props);
}

bool
DOMXrayTraits::call(JSContext *cx, HandleObject wrapper,
                    const JS::CallArgs &args, const js::Wrapper& baseInstance)
{
    RootedObject obj(cx, getTargetObject(wrapper));
    const js::Class* clasp = js::GetObjectClass(obj);
    
    
    
    
    
    
    if (clasp->flags & JSCLASS_IS_DOMIFACEANDPROTOJSCLASS) {
        if (!clasp->call) {
            RootedValue v(cx, ObjectValue(*wrapper));
            js_ReportIsNotFunction(cx, v);
            return false;
        }
        
        if (!clasp->call(cx, args.length(), args.base()))
            return false;
    } else {
        
        
        if (!baseInstance.call(cx, wrapper, args))
            return false;
    }
    return JS_WrapValue(cx, args.rval());
}

bool
DOMXrayTraits::construct(JSContext *cx, HandleObject wrapper,
                         const JS::CallArgs &args, const js::Wrapper& baseInstance)
{
    RootedObject obj(cx, getTargetObject(wrapper));
    MOZ_ASSERT(mozilla::dom::HasConstructor(obj));
    const js::Class* clasp = js::GetObjectClass(obj);
    
    if (clasp->flags & JSCLASS_IS_DOMIFACEANDPROTOJSCLASS) {
        if (!clasp->construct) {
            RootedValue v(cx, ObjectValue(*wrapper));
            js_ReportIsNotFunction(cx, v);
            return false;
        }
        if (!clasp->construct(cx, args.length(), args.base()))
            return false;
    } else {
        if (!baseInstance.construct(cx, wrapper, args))
            return false;
    }
    if (!args.rval().isObject() || !JS_WrapValue(cx, args.rval()))
        return false;
    return true;
}

void
DOMXrayTraits::preserveWrapper(JSObject *target)
{
    nsISupports *identity = mozilla::dom::UnwrapDOMObjectToISupports(target);
    if (!identity)
        return;
    nsWrapperCache* cache = nullptr;
    CallQueryInterface(identity, &cache);
    if (cache)
        cache->PreserveWrapper(identity);
}

JSObject*
DOMXrayTraits::createHolder(JSContext *cx, JSObject *wrapper)
{
    RootedObject global(cx, JS_GetGlobalForObject(cx, wrapper));
    return JS_NewObjectWithGivenProto(cx, nullptr, JS::NullPtr(), global);
}

template <typename Base, typename Traits>
XrayWrapper<Base, Traits>::XrayWrapper(unsigned flags)
  : Base(flags | WrapperFactory::IS_XRAY_WRAPPER_FLAG,  Traits::HasPrototype)
{
}

namespace XrayUtils {

JSObject *
GetNativePropertiesObject(JSContext *cx, JSObject *wrapper)
{
    MOZ_ASSERT(js::IsWrapper(wrapper) && WrapperFactory::IsXrayWrapper(wrapper),
               "bad object passed in");

    JSObject *holder = GetHolder(wrapper);
    MOZ_ASSERT(holder, "uninitialized wrapper being used?");
    return holder;
}

bool
IsXrayResolving(JSContext *cx, HandleObject wrapper, HandleId id)
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

bool
HasNativeProperty(JSContext *cx, HandleObject wrapper, HandleId id, bool *hasProp)
{
    MOZ_ASSERT(WrapperFactory::IsXrayWrapper(wrapper));
    XrayTraits *traits = GetXrayTraits(wrapper);
    MOZ_ASSERT(traits);
    RootedObject holder(cx, traits->ensureHolder(cx, wrapper));
    NS_ENSURE_TRUE(holder, false);
    *hasProp = false;
    Rooted<JSPropertyDescriptor> desc(cx);
    const Wrapper *handler = Wrapper::wrapperHandler(wrapper);

    
    Maybe<ResolvingId> resolvingId;
    if (traits == &XPCWrappedNativeXrayTraits::singleton)
        resolvingId.emplace(cx, wrapper, id);
    if (!traits->resolveOwnProperty(cx, *handler, wrapper, holder, id, &desc))
        return false;
    if (desc.object()) {
        *hasProp = true;
        return true;
    }

    
    bool found = false;
    if (!JS_AlreadyHasOwnPropertyById(cx, holder, id, &found))
        return false;
    if (found) {
        *hasProp = true;
        return true;
    }

    
    if (!traits->resolveNativeProperty(cx, wrapper, holder, id, &desc))
        return false;
    *hasProp = !!desc.object();
    return true;
}

} 

static bool
XrayToString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!args.thisv().isObject()) {
        JS_ReportError(cx, "XrayToString called on an incompatible object");
        return false;
    }

    RootedObject wrapper(cx, &args.thisv().toObject());
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

    RootedObject obj(cx, XrayTraits::getTargetObject(wrapper));

    if (UseDOMXray(obj))
        return NativeToString(cx, wrapper, obj, args.rval());

    static const char start[] = "[object XrayWrapper ";
    static const char end[] = "]";
    nsAutoString result;
    result.AppendASCII(start);

    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative *wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
    char *wrapperStr = wn->ToString();
    if (!wrapperStr) {
        JS_ReportOutOfMemory(cx);
        return false;
    }
    result.AppendASCII(wrapperStr);
    JS_smprintf_free(wrapperStr);

    result.AppendASCII(end);

    JSString *str = JS_NewUCStringCopyN(cx, result.get(), result.Length());
    if (!str)
        return false;

    args.rval().setString(str);
    return true;
}

#ifdef DEBUG

static void
DEBUG_CheckXBLCallable(JSContext *cx, JSObject *obj)
{
    
    
    
    
    
    MOZ_ASSERT_IF(js::IsCrossCompartmentWrapper(obj),
                  xpc::IsContentXBLScope(js::GetObjectCompartment(js::UncheckedUnwrap(obj))));
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
XrayWrapper<Base, Traits>::isExtensible(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                        bool *extensible) const
{
    
    
    
    
    
    *extensible = true;
    return true;
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::preventExtensions(JSContext *cx, HandleObject wrapper) const
{
    
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CHANGE_EXTENSIBILITY);
    return false;
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                                 JS::MutableHandle<JSPropertyDescriptor> desc)
                                                 const
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::GET | BaseProxyHandler::SET |
                                         BaseProxyHandler::GET_PROPERTY_DESCRIPTOR);
    RootedObject holder(cx, Traits::singleton.ensureHolder(cx, wrapper));
    if (Traits::isResolving(cx, holder, id)) {
        desc.object().set(nullptr);
        return true;
    }

    typename Traits::ResolvingIdImpl resolving(cx, wrapper, id);

    if (!holder)
        return false;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    if (!Traits::singleton.resolveOwnProperty(cx, *this, wrapper, holder, id, desc))
        return false;

    
    if (!desc.object() && !JS_GetPropertyDescriptorById(cx, holder, id, desc))
        return false;
    if (desc.object()) {
        desc.object().set(wrapper);
        return true;
    }

    
    if (!Traits::singleton.resolveNativeProperty(cx, wrapper, holder, id, desc))
        return false;

    
    
    
    
    
    
    nsGlobalWindow *win = nullptr;
    if (!desc.object() &&
        JSID_IS_STRING(id) &&
        (win = AsWindow(cx, wrapper)))
    {
        nsAutoJSString name;
        if (!name.init(cx, JSID_TO_STRING(id)))
            return false;
        nsCOMPtr<nsIDOMWindow> childDOMWin = win->GetChildWindow(name);
        if (childDOMWin) {
            nsGlobalWindow *cwin = static_cast<nsGlobalWindow*>(childDOMWin.get());
            JSObject *childObj = cwin->FastGetGlobalJSObject();
            if (MOZ_UNLIKELY(!childObj))
                return xpc::Throw(cx, NS_ERROR_FAILURE);
            FillPropertyDescriptor(desc, wrapper, ObjectValue(*childObj),
                                    true);
            return JS_WrapPropertyDescriptor(cx, desc);
        }
    }

    if (!desc.object() &&
        id == nsXPConnect::GetRuntimeInstance()->GetStringID(XPCJSRuntime::IDX_TO_STRING))
    {

        JSFunction *toString = JS_NewFunction(cx, XrayToString, 0, 0, wrapper, "toString");
        if (!toString)
            return false;

        desc.object().set(wrapper);
        desc.setAttributes(0);
        desc.setGetter(nullptr);
        desc.setSetter(nullptr);
        desc.value().setObject(*JS_GetFunctionObject(toString));
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsCOMPtr<nsIContent> content;
    if (!desc.object() && ObjectScope(wrapper)->IsContentXBLScope() &&
        (content = do_QueryInterfaceNative(cx, wrapper)))
    {
        if (!nsContentUtils::LookupBindingMember(cx, content, id, desc))
            return false;
        DEBUG_CheckXBLLookup(cx, desc.address());
    }

    
    if (!desc.object())
        return true;

    if (!JS_DefinePropertyById(cx, holder, id, desc.value(), desc.attributes(),
                               desc.getter(), desc.setter()) ||
        !JS_GetPropertyDescriptorById(cx, holder, id, desc))
    {
        return false;
    }
    MOZ_ASSERT(desc.object());
    desc.object().set(wrapper);
    return true;
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                                    JS::MutableHandle<JSPropertyDescriptor> desc)
                                                    const
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::GET | BaseProxyHandler::SET |
                                         BaseProxyHandler::GET_PROPERTY_DESCRIPTOR);
    RootedObject holder(cx, Traits::singleton.ensureHolder(cx, wrapper));
    if (Traits::isResolving(cx, holder, id)) {
        desc.object().set(nullptr);
        return true;
    }

    typename Traits::ResolvingIdImpl resolving(cx, wrapper, id);

    
    

    if (!Traits::singleton.resolveOwnProperty(cx, *this, wrapper, holder, id, desc))
        return false;
    if (desc.object())
        desc.object().set(wrapper);
    return true;
}















static bool
RecreateLostWaivers(JSContext *cx, JSPropertyDescriptor *orig,
                    MutableHandle<JSPropertyDescriptor> wrapped)
{
    
    
    bool valueWasWaived =
        orig->value.isObject() &&
        WrapperFactory::HasWaiveXrayFlag(&orig->value.toObject());
    bool getterWasWaived =
        (orig->attrs & JSPROP_GETTER) &&
        WrapperFactory::HasWaiveXrayFlag(JS_FUNC_TO_DATA_PTR(JSObject*, orig->getter));
    bool setterWasWaived =
        (orig->attrs & JSPROP_SETTER) &&
        WrapperFactory::HasWaiveXrayFlag(JS_FUNC_TO_DATA_PTR(JSObject*, orig->setter));

    
    
    

    RootedObject rewaived(cx);
    if (valueWasWaived && !IsCrossCompartmentWrapper(&wrapped.value().toObject())) {
        rewaived = &wrapped.value().toObject();
        rewaived = WrapperFactory::WaiveXray(cx, UncheckedUnwrap(rewaived));
        NS_ENSURE_TRUE(rewaived, false);
        wrapped.value().set(ObjectValue(*rewaived));
    }
    if (getterWasWaived && !IsCrossCompartmentWrapper(wrapped.getterObject())) {
        MOZ_ASSERT(CheckedUnwrap(wrapped.getterObject()));
        rewaived = WrapperFactory::WaiveXray(cx, wrapped.getterObject());
        NS_ENSURE_TRUE(rewaived, false);
        wrapped.setGetterObject(rewaived);
    }
    if (setterWasWaived && !IsCrossCompartmentWrapper(wrapped.setterObject())) {
        MOZ_ASSERT(CheckedUnwrap(wrapped.setterObject()));
        rewaived = WrapperFactory::WaiveXray(cx, wrapped.setterObject());
        NS_ENSURE_TRUE(rewaived, false);
        wrapped.setSetterObject(rewaived);
    }

    return true;
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::defineProperty(JSContext *cx, HandleObject wrapper,
                                          HandleId id, MutableHandle<JSPropertyDescriptor> desc)
                                          const
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::SET);

    Rooted<JSPropertyDescriptor> existing_desc(cx);
    if (!JS_GetPropertyDescriptorById(cx, wrapper, id, &existing_desc))
        return false;

    
    
    
    
    
    
    if (existing_desc.object() == wrapper && existing_desc.isPermanent()) {
        
        
        if (existing_desc.hasGetterOrSetterObject() || desc.hasGetterOrSetterObject() ||
            existing_desc.isEnumerable() != desc.isEnumerable() ||
            (existing_desc.isReadonly() && !desc.isReadonly()))
        {
            
            
            return true;
        }
        if (existing_desc.isReadonly()) {
            
            return true;
        }
    }

    bool defined = false;
    if (!Traits::singleton.defineProperty(cx, wrapper, id, desc, existing_desc, &defined))
        return false;
    if (defined)
        return true;

    
    
    RootedObject target(cx, Traits::singleton.getTargetObject(wrapper));
    JSAutoCompartment ac(cx, target);

    
    RootedObject expandoObject(cx, Traits::singleton.ensureExpandoObject(cx, wrapper,
                                                                         target));
    if (!expandoObject)
        return false;

    
    Rooted<JSPropertyDescriptor> wrappedDesc(cx, desc);
    if (!JS_WrapPropertyDescriptor(cx, &wrappedDesc))
        return false;

    
    if (!RecreateLostWaivers(cx, desc.address(), &wrappedDesc))
        return false;

    return JS_DefinePropertyById(cx, expandoObject, id, wrappedDesc.value(),
                                 wrappedDesc.get().attrs,
                                 wrappedDesc.getter(), wrappedDesc.setter());
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getOwnPropertyNames(JSContext *cx, HandleObject wrapper,
                                               AutoIdVector &props) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);
    return enumerate(cx, wrapper, JSITER_OWNONLY | JSITER_HIDDEN, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::delete_(JSContext *cx, HandleObject wrapper,
                                   HandleId id, bool *bp) const
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::SET);

    
    RootedObject target(cx, Traits::getTargetObject(wrapper));
    RootedObject expando(cx, Traits::singleton.getExpandoObject(cx, target, wrapper));
    if (expando) {
        JSAutoCompartment ac(cx, expando);
        return JS_DeletePropertyById2(cx, expando, id, bp);
    }

    return Traits::singleton.delete_(cx, wrapper, id, bp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::enumerate(JSContext *cx, HandleObject wrapper, unsigned flags,
                                     AutoIdVector &props) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);

    
    
    RootedObject target(cx, Traits::singleton.getTargetObject(wrapper));
    RootedObject expando(cx, Traits::singleton.getExpandoObject(cx, target, wrapper));
    if (expando) {
        JSAutoCompartment ac(cx, expando);
        if (!js::GetPropertyNames(cx, expando, flags, &props))
            return false;
    }

    return Traits::singleton.enumerateNames(cx, wrapper, flags, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::enumerate(JSContext *cx, HandleObject wrapper,
                                    AutoIdVector &props) const
{
    return enumerate(cx, wrapper, 0, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::get(JSContext *cx, HandleObject wrapper,
                               HandleObject receiver, HandleId id,
                               MutableHandleValue vp) const
{
    
    
    
    return js::BaseProxyHandler::get(cx, wrapper, Traits::HasPrototype ? receiver : wrapper, id, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::set(JSContext *cx, HandleObject wrapper,
                               HandleObject receiver, HandleId id,
                               bool strict, MutableHandleValue vp) const
{
    
    
    
    return Traits::set(cx, wrapper, Traits::HasPrototype ? receiver : wrapper, id, strict, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::has(JSContext *cx, HandleObject wrapper,
                               HandleId id, bool *bp) const
{
    
    return js::BaseProxyHandler::has(cx, wrapper, id, bp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::hasOwn(JSContext *cx, HandleObject wrapper,
                                  HandleId id, bool *bp) const
{
    
    return js::BaseProxyHandler::hasOwn(cx, wrapper, id, bp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::keys(JSContext *cx, HandleObject wrapper,
                                AutoIdVector &props) const
{
    
    return js::BaseProxyHandler::keys(cx, wrapper, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::iterate(JSContext *cx, HandleObject wrapper,
                                   unsigned flags, MutableHandleValue vp) const
{
    
    return js::BaseProxyHandler::iterate(cx, wrapper, flags, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::call(JSContext *cx, HandleObject wrapper, const JS::CallArgs &args) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::CALL);
    
    return Traits::call(cx, wrapper, args, Base::singleton);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::construct(JSContext *cx, HandleObject wrapper, const JS::CallArgs &args) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::CALL);
    
    return Traits::construct(cx, wrapper, args, Base::singleton);
}

template <typename Base, typename Traits>
const char *
XrayWrapper<Base, Traits>::className(JSContext *cx, HandleObject wrapper) const
{
    return Traits::className(cx, wrapper, Base::singleton);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::defaultValue(JSContext *cx, HandleObject wrapper,
                                        JSType hint, MutableHandleValue vp) const
{
    
    
    
    
    
    return js::DefaultValue(cx, wrapper, hint, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getPrototypeOf(JSContext *cx, JS::HandleObject wrapper,
                                          JS::MutableHandleObject protop) const
{
    
    
    
    
    if (Base::hasSecurityPolicy())
        return Base::getPrototypeOf(cx, wrapper, protop);

    RootedObject target(cx, Traits::getTargetObject(wrapper));
    RootedObject expando(cx, Traits::singleton.getExpandoObject(cx, target, wrapper));

    
    
    

    RootedValue v(cx);
    if (expando) {
        JSAutoCompartment ac(cx, expando);
        v = JS_GetReservedSlot(expando, JSSLOT_EXPANDO_PROTOTYPE);
    }
    if (v.isUndefined())
        return getPrototypeOfHelper(cx, wrapper, target, protop);

    protop.set(v.toObjectOrNull());
    return JS_WrapObject(cx, protop);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::setPrototypeOf(JSContext *cx, JS::HandleObject wrapper,
                                          JS::HandleObject proto, bool *bp) const
{
    
    
    if (Base::hasSecurityPolicy())
        return Base::setPrototypeOf(cx, wrapper, proto, bp);

    RootedObject target(cx, Traits::getTargetObject(wrapper));
    RootedObject expando(cx, Traits::singleton.ensureExpandoObject(cx, wrapper, target));

    
    JSAutoCompartment ac(cx, target);

    RootedValue v(cx, ObjectOrNullValue(proto));
    if (!JS_WrapValue(cx, &v))
        return false;
    JS_SetReservedSlot(expando, JSSLOT_EXPANDO_PROTOTYPE, v);
    *bp = true;
    return true;
}









template<>
const PermissiveXrayXPCWN PermissiveXrayXPCWN::singleton(0);
template class PermissiveXrayXPCWN;

template<>
const SecurityXrayXPCWN SecurityXrayXPCWN::singleton(0);
template class SecurityXrayXPCWN;

template<>
const PermissiveXrayDOM PermissiveXrayDOM::singleton(0);
template class PermissiveXrayDOM;

template<>
const SecurityXrayDOM SecurityXrayDOM::singleton(0);
template class SecurityXrayDOM;

template<>
const PermissiveXrayJS PermissiveXrayJS::singleton(0);
template class PermissiveXrayJS;

template<>
const PermissiveXrayOpaque PermissiveXrayOpaque::singleton(0);
template class PermissiveXrayOpaque;

template<>
const SCSecurityXrayXPCWN SCSecurityXrayXPCWN::singleton(0);
template class SCSecurityXrayXPCWN;

static nsQueryInterface
do_QueryInterfaceNative(JSContext* cx, HandleObject wrapper)
{
    nsISupports* nativeSupports;
    if (IsWrapper(wrapper) && WrapperFactory::IsXrayWrapper(wrapper)) {
        RootedObject target(cx, XrayTraits::getTargetObject(wrapper));
        if (GetXrayType(target) == XrayForDOMObject) {
            nativeSupports = UnwrapDOMObjectToISupports(target);
        } else {
            XPCWrappedNative *wn = XPCWrappedNative::Get(target);
            nativeSupports = wn->Native();
        }
    } else {
        nsIXPConnect *xpc = nsXPConnect::XPConnect();
        nativeSupports = xpc->GetNativeOfWrapper(cx, wrapper);
    }

    return nsQueryInterface(nativeSupports);
}

}
