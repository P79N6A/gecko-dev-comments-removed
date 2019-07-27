





#include "XrayWrapper.h"
#include "AccessCheck.h"
#include "WrapperFactory.h"

#include "nsIControllers.h"
#include "nsDependentString.h"
#include "nsIScriptError.h"
#include "mozilla/dom/Element.h"

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
      case JSProto_TypedArray:
      case JSProto_SavedFrame:
      case JSProto_RegExp:
        return true;
      default:
        return false;
    }
}

XrayType
GetXrayType(JSObject* obj)
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

JSObject*
XrayAwareCalleeGlobal(JSObject* fun)
{
  MOZ_ASSERT(js::IsFunctionObject(fun));

  if (!js::FunctionHasNativeReserved(fun)) {
      
      return js::GetGlobalForObjectCrossCompartment(fun);
  }

  
  
  
  
  MOZ_ASSERT(&js::GetFunctionNativeReserved(fun, XRAY_DOM_FUNCTION_NATIVE_SLOT_FOR_SELF).toObject() ==
             fun);

  Value v =
      js::GetFunctionNativeReserved(fun, XRAY_DOM_FUNCTION_PARENT_WRAPPER_SLOT);
  MOZ_ASSERT(IsXrayWrapper(&v.toObject()));

  JSObject* xrayTarget = js::UncheckedUnwrap(&v.toObject());
  return js::GetGlobalForObjectCrossCompartment(xrayTarget);
}

JSObject*
XrayTraits::getExpandoChain(HandleObject obj)
{
    return ObjectScope(obj)->GetExpandoChain(obj);
}

bool
XrayTraits::setExpandoChain(JSContext* cx, HandleObject obj, HandleObject chain)
{
    return ObjectScope(obj)->SetExpandoChain(cx, obj, chain);
}


XPCWrappedNative*
XPCWrappedNativeXrayTraits::getWN(JSObject* wrapper)
{
    return XPCWrappedNative::Get(getTargetObject(wrapper));
}

const JSClass XPCWrappedNativeXrayTraits::HolderClass = {
    "NativePropertyHolder", JSCLASS_HAS_RESERVED_SLOTS(2)
};


const JSClass JSXrayTraits::HolderClass = {
    "JSXrayHolder", JSCLASS_HAS_RESERVED_SLOTS(SLOT_COUNT)
};

bool
OpaqueXrayTraits::resolveOwnProperty(JSContext* cx, const Wrapper& jsWrapper, HandleObject wrapper,
                                     HandleObject holder, HandleId id,
                                     MutableHandle<JSPropertyDescriptor> desc)
{
    bool ok = XrayTraits::resolveOwnProperty(cx, jsWrapper, wrapper, holder, id, desc);
    if (!ok || desc.object())
        return ok;

    return ReportWrapperDenial(cx, id, WrapperDenialForXray, "object is not safely Xrayable");
}

bool
ReportWrapperDenial(JSContext* cx, HandleId id, WrapperDenialType type, const char* reason)
{
    CompartmentPrivate* priv = CompartmentPrivate::Get(CurrentGlobalOrNull(cx));
    bool alreadyWarnedOnce = priv->wrapperDenialWarnings[type];
    priv->wrapperDenialWarnings[type] = true;

    
    
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
    nsGlobalWindow* win = WindowGlobalOrNull(CurrentGlobalOrNull(cx));
    if (win)
      windowId = win->WindowID();


    Maybe<nsPrintfCString> errorMessage;
    if (type == WrapperDenialForXray) {
        errorMessage.emplace("XrayWrapper denied access to property %s (reason: %s). "
                             "See https://developer.mozilla.org/en-US/docs/Xray_vision "
                             "for more information. Note that only the first denied "
                             "property access from a given global object will be reported.",
                             NS_LossyConvertUTF16toASCII(propertyName).get(),
                             reason);
    } else {
        MOZ_ASSERT(type == WrapperDenialForCOW);
        errorMessage.emplace("Security wrapper denied access to property %s on privileged "
                             "Javascript object. Support for exposing privileged objects "
                             "to untrusted content via __exposedProps__ is being gradually "
                             "removed - use WebIDL bindings or Components.utils.cloneInto "
                             "instead. Note that only the first denied property access from a "
                             "given global object will be reported.",
                             NS_LossyConvertUTF16toASCII(propertyName).get());
    }
    nsString filenameStr(NS_ConvertASCIItoUTF16(filename.get()));
    nsresult rv = errorObject->InitWithWindowID(NS_ConvertASCIItoUTF16(errorMessage.ref()),
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

bool JSXrayTraits::getOwnPropertyFromTargetIfSafe(JSContext* cx,
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
        return ReportWrapperDenial(cx, id, WrapperDenialForXray, "property has accessor");
    }

    
    if (desc.value().isObject()) {
        RootedObject propObj(cx, js::UncheckedUnwrap(&desc.value().toObject()));
        JSAutoCompartment ac(cx, propObj);

        
        if (!AccessCheck::subsumes(target, propObj)) {
            JSAutoCompartment ac(cx, wrapper);
            return ReportWrapperDenial(cx, id, WrapperDenialForXray, "value not same-origin with target");
        }

        
        XrayType xrayType = GetXrayType(propObj);
        if (xrayType == NotXray || xrayType == XrayForOpaqueObject) {
            if (IdentifyStandardInstance(propObj) == JSProto_ArrayBuffer) {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
            } else {
                JSAutoCompartment ac(cx, wrapper);
                return ReportWrapperDenial(cx, id, WrapperDenialForXray, "value not Xrayable");
            }
        }

        
        if (JS::IsCallable(propObj)) {
            JSAutoCompartment ac(cx, wrapper);
            return ReportWrapperDenial(cx, id, WrapperDenialForXray, "value is callable");
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
        return ReportWrapperDenial(cx, id, WrapperDenialForXray, "value shadows a property on the standard prototype");

    
    outDesc.assign(desc.get());
    return true;
}

bool
JSXrayTraits::resolveOwnProperty(JSContext* cx, const Wrapper& jsWrapper,
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
        } else if (key == JSProto_RegExp) {
            if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_LASTINDEX)) {
                JSAutoCompartment ac(cx, target);
                return getOwnPropertyFromTargetIfSafe(cx, target, wrapper, id, desc);
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

    
    if (id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_CONSTRUCTOR)) {
        RootedObject constructor(cx);
        {
            JSAutoCompartment ac(cx, target);
            if (!JS_GetClassObject(cx, key, &constructor))
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

    
    if (key == JSProto_RegExp && id == GetRTIdByIndex(cx, XPCJSRuntime::IDX_LASTINDEX)) {
        JSAutoCompartment ac(cx, target);
        return getOwnPropertyFromTargetIfSafe(cx, target, wrapper, id, desc);
    }

    
    const js::Class* clasp = js::GetObjectClass(target);
    MOZ_ASSERT(clasp->spec.defined());

    
    const JSFunctionSpec* fsMatch = nullptr;
    for (const JSFunctionSpec* fs = clasp->spec.prototypeFunctions; fs && fs->name; ++fs) {
        if (PropertySpecNameEqualsId(fs->name, id)) {
            fsMatch = fs;
            break;
        }
    }
    if (fsMatch) {
        
        RootedFunction fun(cx);
        if (fsMatch->selfHostedName) {
            fun = JS::GetSelfHostedFunction(cx, fsMatch->selfHostedName, id, fsMatch->nargs);
        } else {
            fun = JS_NewFunctionById(cx, fsMatch->call.op, fsMatch->nargs, 0, id);
        }
        if (!fun)
            return false;

        
        
        
        
        RootedObject funObj(cx, JS_GetFunctionObject(fun));
        return JS_DefinePropertyById(cx, holder, id, funObj, 0) &&
               JS_GetPropertyDescriptorById(cx, holder, id, desc);
    }

    
    const JSPropertySpec* psMatch = nullptr;
    for (const JSPropertySpec* ps = clasp->spec.prototypeProperties; ps && ps->name; ++ps) {
        if (PropertySpecNameEqualsId(ps->name, id)) {
            psMatch = ps;
            break;
        }
    }
    if (psMatch) {
        desc.value().setUndefined();
        RootedFunction getterObj(cx);
        RootedFunction setterObj(cx);
        unsigned flags = psMatch->flags;
        if (psMatch->isSelfHosted()) {
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
        } else {
            desc.setGetter(JS_CAST_NATIVE_TO(psMatch->getter.native.op,
                                             JSGetterOp));
            desc.setSetter(JS_CAST_NATIVE_TO(psMatch->setter.native.op,
                                             JSSetterOp));
        }
        desc.setAttributes(flags);

        
        
        
        
        
        
        
        
        return JS_DefinePropertyById(cx, holder, id,
                                     UndefinedHandleValue,
                                     
                                     
                                     
                                     
                                     desc.attributes(),
                                     JS_PROPERTYOP_GETTER(desc.getter()),
                                     JS_PROPERTYOP_SETTER(desc.setter())) &&
               JS_GetPropertyDescriptorById(cx, holder, id, desc);
    }

    return true;
}

bool
JSXrayTraits::delete_(JSContext* cx, HandleObject wrapper, HandleId id, ObjectOpResult& result)
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
            return JS_DeletePropertyById(cx, target, id, result);
    }
    return result.succeed();
}

bool
JSXrayTraits::defineProperty(JSContext* cx, HandleObject wrapper, HandleId id,
                             Handle<JSPropertyDescriptor> desc,
                             Handle<JSPropertyDescriptor> existingDesc,
                             ObjectOpResult& result,
                             bool* defined)
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

        Rooted<JSPropertyDescriptor> wrappedDesc(cx, desc);
        JSAutoCompartment ac(cx, target);
        if (!JS_WrapPropertyDescriptor(cx, &wrappedDesc) ||
            !JS_DefinePropertyById(cx, target, id, wrappedDesc, result))
        {
            return false;
        }
        *defined = true;
        return true;
    }

    return true;
}

static bool
MaybeAppend(jsid id, unsigned flags, AutoIdVector& props)
{
    MOZ_ASSERT(!(flags & JSITER_SYMBOLSONLY));
    if (!(flags & JSITER_SYMBOLS) && JSID_IS_SYMBOL(id))
        return true;
    return props.append(id);
}

bool
JSXrayTraits::enumerateNames(JSContext* cx, HandleObject wrapper, unsigned flags,
                             AutoIdVector& props)
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
                if (!js::GetPropertyKeys(cx, target, flags | JSITER_OWNONLY, &targetProps))
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
        } else if (key == JSProto_RegExp) {
            if (!props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_LASTINDEX)))
                return false;
        }

        
        return true;
    }

    
    if (!props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_CONSTRUCTOR)))
        return false;

    
    if (IsErrorObjectKey(key) && !props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_NAME)))
        return false;

    
    if (key == JSProto_RegExp && !props.append(GetRTIdByIndex(cx, XPCJSRuntime::IDX_LASTINDEX)))
        return false;

    
    const js::Class* clasp = js::GetObjectClass(target);
    MOZ_ASSERT(clasp->spec.defined());

    
    for (const JSFunctionSpec* fs = clasp->spec.prototypeFunctions; fs && fs->name; ++fs) {
        jsid id;
        if (!PropertySpecNameToPermanentId(cx, fs->name, &id))
            return false;
        if (!MaybeAppend(id, flags, props))
            return false;
    }
    for (const JSPropertySpec* ps = clasp->spec.prototypeProperties; ps && ps->name; ++ps) {
        jsid id;
        if (!PropertySpecNameToPermanentId(cx, ps->name, &id))
            return false;
        if (!MaybeAppend(id, flags, props))
            return false;
    }

    return true;
}

JSObject*
JSXrayTraits::createHolder(JSContext* cx, JSObject* wrapper)
{
    RootedObject target(cx, getTargetObject(wrapper));
    RootedObject holder(cx, JS_NewObjectWithGivenProto(cx, &HolderClass,
                                                       JS::NullPtr()));
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
GetXrayTraits(JSObject* obj)
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
ObjectPrincipal(JSObject* obj)
{
    return GetCompartmentPrincipal(js::GetObjectCompartment(obj));
}

static nsIPrincipal*
GetExpandoObjectPrincipal(JSObject* expandoObject)
{
    Value v = JS_GetReservedSlot(expandoObject, JSSLOT_EXPANDO_ORIGIN);
    return static_cast<nsIPrincipal*>(v.toPrivate());
}

static void
ExpandoObjectFinalize(JSFreeOp* fop, JSObject* obj)
{
    
    nsIPrincipal* principal = GetExpandoObjectPrincipal(obj);
    NS_RELEASE(principal);
}

const JSClass ExpandoObjectClass = {
    "XrayExpandoObject",
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_EXPANDO_COUNT),
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, ExpandoObjectFinalize
};

bool
XrayTraits::expandoObjectMatchesConsumer(JSContext* cx,
                                         HandleObject expandoObject,
                                         nsIPrincipal* consumerOrigin,
                                         HandleObject exclusiveGlobal)
{
    MOZ_ASSERT(js::IsObjectInContextCompartment(expandoObject, cx));

    
    nsIPrincipal* o = GetExpandoObjectPrincipal(expandoObject);
    
    
    
    
    
    
    
    if (!consumerOrigin->Equals(o))
      return false;

    
    JSObject* owner = JS_GetReservedSlot(expandoObject,
                                         JSSLOT_EXPANDO_EXCLUSIVE_GLOBAL)
                                        .toObjectOrNull();
    if (!owner && !exclusiveGlobal)
        return true;

    
    MOZ_ASSERT(!exclusiveGlobal || js::IsObjectInContextCompartment(exclusiveGlobal, cx));
    MOZ_ASSERT(!owner || js::IsObjectInContextCompartment(owner, cx));
    return owner == exclusiveGlobal;
}

bool
XrayTraits::getExpandoObjectInternal(JSContext* cx, HandleObject target,
                                     nsIPrincipal* origin,
                                     JSObject* exclusiveGlobalArg,
                                     MutableHandleObject expandoObject)
{
    MOZ_ASSERT(!JS_IsExceptionPending(cx));
    expandoObject.set(nullptr);

    
    
    RootedObject exclusiveGlobal(cx, exclusiveGlobalArg);
    JSAutoCompartment ac(cx, target);
    if (!JS_WrapObject(cx, &exclusiveGlobal))
        return false;

    
    RootedObject head(cx, getExpandoChain(target));
    while (head) {
        if (expandoObjectMatchesConsumer(cx, head, origin, exclusiveGlobal)) {
            expandoObject.set(head);
            return true;
        }
        head = JS_GetReservedSlot(head, JSSLOT_EXPANDO_NEXT).toObjectOrNull();
    }

    
    return true;
}

bool
XrayTraits::getExpandoObject(JSContext* cx, HandleObject target, HandleObject consumer,
                             MutableHandleObject expandoObject)
{
    JSObject* consumerGlobal = js::GetGlobalForObjectCrossCompartment(consumer);
    bool isSandbox = !strcmp(js::GetObjectJSClass(consumerGlobal)->name, "Sandbox");
    return getExpandoObjectInternal(cx, target, ObjectPrincipal(consumer),
                                    isSandbox ? consumerGlobal : nullptr,
                                    expandoObject);
}

JSObject*
XrayTraits::attachExpandoObject(JSContext* cx, HandleObject target,
                                nsIPrincipal* origin, HandleObject exclusiveGlobal)
{
    
    MOZ_ASSERT(js::IsObjectInContextCompartment(target, cx));
    MOZ_ASSERT(!exclusiveGlobal || js::IsObjectInContextCompartment(exclusiveGlobal, cx));

    
#ifdef DEBUG
    {
        RootedObject existingExpandoObject(cx);
        if (getExpandoObjectInternal(cx, target, origin, exclusiveGlobal, &existingExpandoObject))
            MOZ_ASSERT(!existingExpandoObject);
        else
            JS_ClearPendingException(cx);
    }
#endif

    
    RootedObject expandoObject(cx,
      JS_NewObjectWithGivenProto(cx, &ExpandoObjectClass, JS::NullPtr()));
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

JSObject*
XrayTraits::ensureExpandoObject(JSContext* cx, HandleObject wrapper,
                                HandleObject target)
{
    
    JSAutoCompartment ac(cx, target);
    RootedObject expandoObject(cx);
    if (!getExpandoObject(cx, target, wrapper, &expandoObject))
        return nullptr;
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
XrayTraits::cloneExpandoChain(JSContext* cx, HandleObject dst, HandleObject src)
{
    MOZ_ASSERT(js::IsObjectInContextCompartment(dst, cx));
    MOZ_ASSERT(getExpandoChain(dst) == nullptr);

    RootedObject oldHead(cx, getExpandoChain(src));

#ifdef DEBUG
    
    
    
    
    
    
    if (oldHead) {
        nsISupports* identity = mozilla::dom::UnwrapDOMObjectToISupports(src);
        if (identity) {
            nsWrapperCache* cache = nullptr;
            CallQueryInterface(identity, &cache);
            MOZ_ASSERT_IF(cache, cache->PreservingWrapper());
        }
    }
#endif

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
bool CloneExpandoChain(JSContext* cx, JSObject* dstArg, JSObject* srcArg)
{
    RootedObject dst(cx, dstArg);
    RootedObject src(cx, srcArg);
    return GetXrayTraits(src)->cloneExpandoChain(cx, dst, src);
}
}

static JSObject*
GetHolder(JSObject* obj)
{
    return &js::GetProxyExtra(obj, 0).toObject();
}

JSObject*
XrayTraits::getHolder(JSObject* wrapper)
{
    MOZ_ASSERT(WrapperFactory::IsXrayWrapper(wrapper));
    js::Value v = js::GetProxyExtra(wrapper, 0);
    return v.isObject() ? &v.toObject() : nullptr;
}

JSObject*
XrayTraits::ensureHolder(JSContext* cx, HandleObject wrapper)
{
    RootedObject holder(cx, getHolder(wrapper));
    if (holder)
        return holder;
    holder = createHolder(cx, wrapper); 
    if (holder)
        js::SetProxyExtra(wrapper, 0, ObjectValue(*holder));
    return holder;
}

namespace XrayUtils {

bool
IsXPCWNHolderClass(const JSClass* clasp)
{
  return clasp == &XPCWrappedNativeXrayTraits::HolderClass;
}

}

static nsGlobalWindow*
AsWindow(JSContext* cx, JSObject* wrapper)
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
IsWindow(JSContext* cx, JSObject* wrapper)
{
    return !!AsWindow(cx, wrapper);
}

void
XPCWrappedNativeXrayTraits::preserveWrapper(JSObject* target)
{
    XPCWrappedNative* wn = XPCWrappedNative::Get(target);
    nsRefPtr<nsXPCClassInfo> ci;
    CallQueryInterface(wn->Native(), getter_AddRefs(ci));
    if (ci)
        ci->PreserveWrapper(wn->Native());
}

static bool
XrayToString(JSContext* cx, unsigned argc, JS::Value* vp);

bool
XPCWrappedNativeXrayTraits::resolveNativeProperty(JSContext* cx, HandleObject wrapper,
                                                  HandleObject holder, HandleId id,
                                                  MutableHandle<JSPropertyDescriptor> desc)
{
    MOZ_ASSERT(js::GetObjectJSClass(holder) == &HolderClass);

    desc.object().set(nullptr);

    
    RootedObject target(cx, getTargetObject(wrapper));
    XPCCallContext ccx(JS_CALLER, cx, target, NullPtr(), id);

    
    
    if (!JSID_IS_STRING(id))
        return true;

    
    
    
    nsGlobalWindow* win = nullptr;
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

        if (!GetOrCreateDOMReflector(cx, f, desc.value())) {
          return false;
        }

        desc.object().set(wrapper);
        return true;
    }

    XPCNativeInterface* iface;
    XPCNativeMember* member;
    XPCWrappedNative* wn = getWN(wrapper);

    if (ccx.GetWrapper() != wn || !wn->IsValid()) {
        return true;
    }

    if (!(iface = ccx.GetInterface()) || !(member = ccx.GetMember())) {
        if (id != nsXPConnect::GetRuntimeInstance()->GetStringID(XPCJSRuntime::IDX_TO_STRING))
            return true;

        JSFunction* toString = JS_NewFunction(cx, XrayToString, 0, 0, "toString");
        if (!toString)
            return false;

        FillPropertyDescriptor(desc, wrapper, 0,
                               ObjectValue(*JS_GetFunctionObject(toString)));

        return JS_DefinePropertyById(cx, holder, id, desc) &&
               JS_GetPropertyDescriptorById(cx, holder, id, desc);
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

        
        
        
        
        desc.setGetter(nullptr);
        desc.setSetter(nullptr);
    }

    if (!JS_WrapValue(cx, desc.value()) || !JS_WrapValue(cx, &fval))
        return false;

    if (desc.hasGetterObject())
        desc.setGetterObject(&fval.toObject());
    if (desc.hasSetterObject())
        desc.setSetterObject(&fval.toObject());

    return JS_DefinePropertyById(cx, holder, id, desc);
}

static bool
wrappedJSObject_getter(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (!args.thisv().isObject()) {
        JS_ReportError(cx, "This value not an object");
        return false;
    }
    RootedObject wrapper(cx, &args.thisv().toObject());
    if (!IsWrapper(wrapper) || !WrapperFactory::IsXrayWrapper(wrapper) ||
        !AccessCheck::wrapperSubsumes(wrapper)) {
        JS_ReportError(cx, "Unexpected object");
        return false;
    }

    args.rval().setObject(*wrapper);

    return WrapperFactory::WaiveXrayAndWrap(cx, args.rval());
}

bool
XrayTraits::resolveOwnProperty(JSContext* cx, const Wrapper& jsWrapper,
                               HandleObject wrapper, HandleObject holder, HandleId id,
                               MutableHandle<JSPropertyDescriptor> desc)
{
    desc.object().set(nullptr);
    RootedObject target(cx, getTargetObject(wrapper));
    RootedObject expando(cx);
    if (!getExpandoObject(cx, target, wrapper, &expando))
        return false;

    
    
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
XPCWrappedNativeXrayTraits::resolveOwnProperty(JSContext* cx, const Wrapper& jsWrapper,
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

    return JS_GetPropertyDescriptorById(cx, holder, id, desc);
}

bool
XPCWrappedNativeXrayTraits::defineProperty(JSContext* cx, HandleObject wrapper, HandleId id,
                                           Handle<JSPropertyDescriptor> desc,
                                           Handle<JSPropertyDescriptor> existingDesc,
                                           JS::ObjectOpResult& result, bool* defined)
{
    *defined = false;
    RootedObject holder(cx, singleton.ensureHolder(cx, wrapper));

    
    
    int32_t index = GetArrayIndexFromId(cx, id);
    if (IsArrayIndex(index) && IsWindow(cx, wrapper)) {
        *defined = true;
        return result.succeed();
    }

    return true;
}

bool
XPCWrappedNativeXrayTraits::enumerateNames(JSContext* cx, HandleObject wrapper, unsigned flags,
                                           AutoIdVector& props)
{
    
    AutoIdVector wnProps(cx);
    {
        RootedObject target(cx, singleton.getTargetObject(wrapper));
        JSAutoCompartment ac(cx, target);
        if (!js::GetPropertyKeys(cx, target, flags, &wnProps))
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

JSObject*
XPCWrappedNativeXrayTraits::createHolder(JSContext* cx, JSObject* wrapper)
{
    return JS_NewObjectWithGivenProto(cx, &HolderClass, JS::NullPtr());
}

bool
XPCWrappedNativeXrayTraits::call(JSContext* cx, HandleObject wrapper,
                                 const JS::CallArgs& args,
                                 const js::Wrapper& baseInstance)
{
    
    XPCWrappedNative* wn = getWN(wrapper);
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
XPCWrappedNativeXrayTraits::construct(JSContext* cx, HandleObject wrapper,
                                      const JS::CallArgs& args,
                                      const js::Wrapper& baseInstance)
{
    
    XPCWrappedNative* wn = getWN(wrapper);
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
DOMXrayTraits::resolveOwnProperty(JSContext* cx, const Wrapper& jsWrapper, HandleObject wrapper,
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

    return JS_DefinePropertyById(cx, holder, id, desc) &&
           JS_GetPropertyDescriptorById(cx, holder, id, desc);
}

bool
DOMXrayTraits::defineProperty(JSContext* cx, HandleObject wrapper, HandleId id,
                              Handle<JSPropertyDescriptor> desc,
                              Handle<JSPropertyDescriptor> existingDesc,
                              JS::ObjectOpResult& result, bool* defined)
{
    
    
    if (IsWindow(cx, wrapper)) {
        int32_t index = GetArrayIndexFromId(cx, id);
        if (IsArrayIndex(index)) {
            *defined = true;
            return result.succeed();
        }
    }

    JS::Rooted<JSObject*> obj(cx, getTargetObject(wrapper));
    return XrayDefineProperty(cx, wrapper, obj, id, desc, result, defined);
}

bool
DOMXrayTraits::enumerateNames(JSContext* cx, HandleObject wrapper, unsigned flags,
                              AutoIdVector& props)
{
    JS::Rooted<JSObject*> obj(cx, getTargetObject(wrapper));
    return XrayOwnPropertyKeys(cx, wrapper, obj, flags, props);
}

bool
DOMXrayTraits::call(JSContext* cx, HandleObject wrapper,
                    const JS::CallArgs& args, const js::Wrapper& baseInstance)
{
    RootedObject obj(cx, getTargetObject(wrapper));
    const js::Class* clasp = js::GetObjectClass(obj);
    
    
    
    
    
    
    if (clasp->flags & JSCLASS_IS_DOMIFACEANDPROTOJSCLASS) {
        if (!clasp->call) {
            RootedValue v(cx, ObjectValue(*wrapper));
            js::ReportIsNotFunction(cx, v);
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
DOMXrayTraits::construct(JSContext* cx, HandleObject wrapper,
                         const JS::CallArgs& args, const js::Wrapper& baseInstance)
{
    RootedObject obj(cx, getTargetObject(wrapper));
    MOZ_ASSERT(mozilla::dom::HasConstructor(obj));
    const js::Class* clasp = js::GetObjectClass(obj);
    
    if (clasp->flags & JSCLASS_IS_DOMIFACEANDPROTOJSCLASS) {
        if (!clasp->construct) {
            RootedValue v(cx, ObjectValue(*wrapper));
            js::ReportIsNotFunction(cx, v);
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

bool
DOMXrayTraits::getPrototype(JSContext* cx, JS::HandleObject wrapper,
                            JS::HandleObject target,
                            JS::MutableHandleObject protop)
{
    return mozilla::dom::XrayGetNativeProto(cx, target, protop);
}

void
DOMXrayTraits::preserveWrapper(JSObject* target)
{
    nsISupports* identity = mozilla::dom::UnwrapDOMObjectToISupports(target);
    if (!identity)
        return;
    nsWrapperCache* cache = nullptr;
    CallQueryInterface(identity, &cache);
    if (cache)
        cache->PreserveWrapper(identity);
}

JSObject*
DOMXrayTraits::createHolder(JSContext* cx, JSObject* wrapper)
{
    return JS_NewObjectWithGivenProto(cx, nullptr, JS::NullPtr());
}

namespace XrayUtils {

JSObject*
GetNativePropertiesObject(JSContext* cx, JSObject* wrapper)
{
    MOZ_ASSERT(js::IsWrapper(wrapper) && WrapperFactory::IsXrayWrapper(wrapper),
               "bad object passed in");

    JSObject* holder = GetHolder(wrapper);
    MOZ_ASSERT(holder, "uninitialized wrapper being used?");
    return holder;
}

bool
HasNativeProperty(JSContext* cx, HandleObject wrapper, HandleId id, bool* hasProp)
{
    MOZ_ASSERT(WrapperFactory::IsXrayWrapper(wrapper));
    XrayTraits* traits = GetXrayTraits(wrapper);
    MOZ_ASSERT(traits);
    RootedObject holder(cx, traits->ensureHolder(cx, wrapper));
    NS_ENSURE_TRUE(holder, false);
    *hasProp = false;
    Rooted<JSPropertyDescriptor> desc(cx);
    const Wrapper* handler = Wrapper::wrapperHandler(wrapper);

    
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
XrayToString(JSContext* cx, unsigned argc, Value* vp)
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
    if (GetXrayType(obj) != XrayForWrappedNative) {
        JS_ReportError(cx, "XrayToString called on an incompatible object");
        return false;
    }

    static const char start[] = "[object XrayWrapper ";
    static const char end[] = "]";
    nsAutoString result;
    result.AppendASCII(start);

    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wn = XPCWrappedNativeXrayTraits::getWN(wrapper);
    char* wrapperStr = wn->ToString();
    if (!wrapperStr) {
        JS_ReportOutOfMemory(cx);
        return false;
    }
    result.AppendASCII(wrapperStr);
    JS_smprintf_free(wrapperStr);

    result.AppendASCII(end);

    JSString* str = JS_NewUCStringCopyN(cx, result.get(), result.Length());
    if (!str)
        return false;

    args.rval().setString(str);
    return true;
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::preventExtensions(JSContext* cx, HandleObject wrapper,
                                             ObjectOpResult& result) const
{
    
    
    
    
    
    return result.failCantPreventExtensions();
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::isExtensible(JSContext* cx, JS::Handle<JSObject*> wrapper,
                                        bool* extensible) const
{
    
    *extensible = true;
    return true;
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getPropertyDescriptor(JSContext* cx, HandleObject wrapper, HandleId id,
                                                 JS::MutableHandle<JSPropertyDescriptor> desc)
                                                 const
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::GET | BaseProxyHandler::SET |
                                         BaseProxyHandler::GET_PROPERTY_DESCRIPTOR);
    RootedObject holder(cx, Traits::singleton.ensureHolder(cx, wrapper));

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

    
    
    
    
    
    
    nsGlobalWindow* win = nullptr;
    if (!desc.object() &&
        JSID_IS_STRING(id) &&
        (win = AsWindow(cx, wrapper)))
    {
        nsAutoJSString name;
        if (!name.init(cx, JSID_TO_STRING(id)))
            return false;
        nsCOMPtr<nsIDOMWindow> childDOMWin = win->GetChildWindow(name);
        if (childDOMWin) {
            nsGlobalWindow* cwin = static_cast<nsGlobalWindow*>(childDOMWin.get());
            JSObject* childObj = cwin->FastGetGlobalJSObject();
            if (MOZ_UNLIKELY(!childObj))
                return xpc::Throw(cx, NS_ERROR_FAILURE);
            FillPropertyDescriptor(desc, wrapper, ObjectValue(*childObj),
                                    true);
            return JS_WrapPropertyDescriptor(cx, desc);
        }
    }

    
    if (!desc.object())
        return true;

    if (!JS_DefinePropertyById(cx, holder, id, desc) ||
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
XrayWrapper<Base, Traits>::getOwnPropertyDescriptor(JSContext* cx, HandleObject wrapper, HandleId id,
                                                    JS::MutableHandle<JSPropertyDescriptor> desc)
                                                    const
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::GET | BaseProxyHandler::SET |
                                         BaseProxyHandler::GET_PROPERTY_DESCRIPTOR);
    RootedObject holder(cx, Traits::singleton.ensureHolder(cx, wrapper));

    if (!Traits::singleton.resolveOwnProperty(cx, *this, wrapper, holder, id, desc))
        return false;
    if (desc.object())
        desc.object().set(wrapper);
    return true;
}















static bool
RecreateLostWaivers(JSContext* cx, const JSPropertyDescriptor* orig,
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
XrayWrapper<Base, Traits>::defineProperty(JSContext* cx, HandleObject wrapper,
                                          HandleId id, Handle<JSPropertyDescriptor> desc,
                                          ObjectOpResult& result) const
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::SET);

    Rooted<JSPropertyDescriptor> existing_desc(cx);
    if (!JS_GetPropertyDescriptorById(cx, wrapper, id, &existing_desc))
        return false;

    
    
    
    
    
    
    if (existing_desc.object() == wrapper && !existing_desc.configurable()) {
        
        
        if (existing_desc.isAccessorDescriptor() || desc.isAccessorDescriptor() ||
            (desc.hasEnumerable() && existing_desc.enumerable() != desc.enumerable()) ||
            (desc.hasWritable() && !existing_desc.writable() && desc.writable()))
        {
            
            
            return result.succeed();
        }
        if (!existing_desc.writable()) {
            
            return result.succeed();
        }
    }

    bool defined = false;
    if (!Traits::singleton.defineProperty(cx, wrapper, id, desc, existing_desc, result, &defined))
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

    return JS_DefinePropertyById(cx, expandoObject, id, wrappedDesc, result);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::ownPropertyKeys(JSContext* cx, HandleObject wrapper,
                                           AutoIdVector& props) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);
    return getPropertyKeys(cx, wrapper, JSITER_OWNONLY | JSITER_HIDDEN | JSITER_SYMBOLS, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::delete_(JSContext* cx, HandleObject wrapper,
                                   HandleId id, ObjectOpResult& result) const
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::SET);

    
    RootedObject target(cx, Traits::getTargetObject(wrapper));
    RootedObject expando(cx);
    if (!Traits::singleton.getExpandoObject(cx, target, wrapper, &expando))
        return false;

    if (expando) {
        JSAutoCompartment ac(cx, expando);
        return JS_DeletePropertyById(cx, expando, id, result);
    }

    return Traits::singleton.delete_(cx, wrapper, id, result);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::get(JSContext* cx, HandleObject wrapper,
                               HandleObject receiver, HandleId id,
                               MutableHandleValue vp) const
{
    
    
    
    return js::BaseProxyHandler::get(cx, wrapper, Traits::HasPrototype ? receiver : wrapper, id, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::set(JSContext* cx, HandleObject wrapper, HandleId id, HandleValue v,
                               HandleValue receiver, ObjectOpResult& result) const
{
    MOZ_ASSERT(!Traits::HasPrototype);
    
    
    
    RootedValue wrapperValue(cx, ObjectValue(*wrapper));
    return js::BaseProxyHandler::set(cx, wrapper, id, v, wrapperValue, result);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::has(JSContext* cx, HandleObject wrapper,
                               HandleId id, bool* bp) const
{
    
    return js::BaseProxyHandler::has(cx, wrapper, id, bp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::hasOwn(JSContext* cx, HandleObject wrapper,
                                  HandleId id, bool* bp) const
{
    
    return js::BaseProxyHandler::hasOwn(cx, wrapper, id, bp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getOwnEnumerablePropertyKeys(JSContext* cx,
                                                        HandleObject wrapper,
                                                        AutoIdVector& props) const
{
    
    return js::BaseProxyHandler::getOwnEnumerablePropertyKeys(cx, wrapper, props);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::enumerate(JSContext* cx, HandleObject wrapper,
                                     MutableHandleObject objp) const
{
    
    return js::BaseProxyHandler::enumerate(cx, wrapper, objp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::call(JSContext* cx, HandleObject wrapper, const JS::CallArgs& args) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::CALL);
    
    return Traits::call(cx, wrapper, args, Base::singleton);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::construct(JSContext* cx, HandleObject wrapper, const JS::CallArgs& args) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::CALL);
    
    return Traits::construct(cx, wrapper, args, Base::singleton);
}

template <typename Base, typename Traits>
const char*
XrayWrapper<Base, Traits>::className(JSContext* cx, HandleObject wrapper) const
{
    return Traits::className(cx, wrapper, Base::singleton);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::defaultValue(JSContext* cx, HandleObject wrapper,
                                        JSType hint, MutableHandleValue vp) const
{
    
    
    
    
    
    return OrdinaryToPrimitive(cx, wrapper, hint, vp);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getPrototype(JSContext* cx, JS::HandleObject wrapper,
                                        JS::MutableHandleObject protop) const
{
    
    
    
    
    if (Base::hasSecurityPolicy())
        return Base::getPrototype(cx, wrapper, protop);

    RootedObject target(cx, Traits::getTargetObject(wrapper));
    RootedObject expando(cx);
    if (!Traits::singleton.getExpandoObject(cx, target, wrapper, &expando))
        return false;

    
    
    

    RootedValue v(cx);
    if (expando) {
        JSAutoCompartment ac(cx, expando);
        v = JS_GetReservedSlot(expando, JSSLOT_EXPANDO_PROTOTYPE);
    }
    if (v.isUndefined())
        return getPrototypeHelper(cx, wrapper, target, protop);

    protop.set(v.toObjectOrNull());
    return JS_WrapObject(cx, protop);
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::setPrototype(JSContext* cx, JS::HandleObject wrapper,
                                        JS::HandleObject proto, JS::ObjectOpResult& result) const
{
    
    
    if (Base::hasSecurityPolicy())
        return Base::setPrototype(cx, wrapper, proto, result);

    RootedObject target(cx, Traits::getTargetObject(wrapper));
    RootedObject expando(cx, Traits::singleton.ensureExpandoObject(cx, wrapper, target));
    if (!expando)
        return false;

    
    JSAutoCompartment ac(cx, target);

    RootedValue v(cx, ObjectOrNullValue(proto));
    if (!JS_WrapValue(cx, &v))
        return false;
    JS_SetReservedSlot(expando, JSSLOT_EXPANDO_PROTOTYPE, v);
    return result.succeed();
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::setImmutablePrototype(JSContext* cx, JS::HandleObject wrapper,
                                                 bool* succeeded) const
{
    
    
    
    
    *succeeded = false;
    return true;
}

template <typename Base, typename Traits>
bool
XrayWrapper<Base, Traits>::getPropertyKeys(JSContext* cx, HandleObject wrapper, unsigned flags,
                                           AutoIdVector& props) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);

    
    
    RootedObject target(cx, Traits::singleton.getTargetObject(wrapper));
    RootedObject expando(cx);
    if (!Traits::singleton.getExpandoObject(cx, target, wrapper, &expando))
        return false;

    if (expando) {
        JSAutoCompartment ac(cx, expando);
        if (!js::GetPropertyKeys(cx, expando, flags, &props))
            return false;
    }

    return Traits::singleton.enumerateNames(cx, wrapper, flags, props);
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

}
