






#include "WaiveXrayWrapper.h"
#include "FilteringWrapper.h"
#include "XrayWrapper.h"
#include "AccessCheck.h"
#include "XPCWrapper.h"
#include "ChromeObjectWrapper.h"

#include "xpcprivate.h"
#include "dombindings.h"
#include "XPCMaps.h"
#include "mozilla/dom/BindingUtils.h"
#include "jsfriendapi.h"
#include "mozilla/Likely.h"

using namespace js;

namespace xpc {








DirectWrapper XrayWaiver(WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG);





WaiveXrayWrapper WaiveXrayWrapper::singleton(0);

static JSObject *
GetCurrentOuter(JSContext *cx, JSObject *obj)
{
    obj = JS_ObjectToOuterObject(cx, obj);
    if (!obj)
        return nullptr;

    if (IsWrapper(obj) && !js::GetObjectClass(obj)->ext.innerObject) {
        obj = UnwrapObject(obj);
        NS_ASSERTION(js::GetObjectClass(obj)->ext.innerObject,
                     "weird object, expecting an outer window proxy");
    }

    return obj;
}

JSObject *
WrapperFactory::GetXrayWaiver(JSObject *obj)
{
    
    MOZ_ASSERT(obj == UnwrapObject(obj));
    MOZ_ASSERT(!js::GetObjectClass(obj)->ext.outerObject);
    CompartmentPrivate *priv = GetCompartmentPrivate(obj);
    MOZ_ASSERT(priv);

    if (!priv->waiverWrapperMap)
        return NULL;
    return xpc_UnmarkGrayObject(priv->waiverWrapperMap->Find(obj));
}

JSObject *
WrapperFactory::CreateXrayWaiver(JSContext *cx, JSObject *obj)
{
    
    
    MOZ_ASSERT(!GetXrayWaiver(obj));
    CompartmentPrivate *priv = GetCompartmentPrivate(obj);

    
    JSObject *proto = js::GetObjectProto(obj);
    if (proto && !(proto = WaiveXray(cx, proto)))
        return nullptr;

    
    JSAutoCompartment ac(cx, obj);
    if (!JS_WrapObject(cx, &proto))
        return nullptr;
    JSObject *waiver = Wrapper::New(cx, obj, proto,
                                    JS_GetGlobalForObject(cx, obj),
                                    &XrayWaiver);
    if (!waiver)
        return nullptr;

    
    
    if (!priv->waiverWrapperMap) {
        priv->waiverWrapperMap = JSObject2JSObjectMap::
                                   newMap(XPC_WRAPPER_MAP_SIZE);
        MOZ_ASSERT(priv->waiverWrapperMap);
    }
    if (!priv->waiverWrapperMap->Add(obj, waiver))
        return nullptr;
    return waiver;
}

JSObject *
WrapperFactory::WaiveXray(JSContext *cx, JSObject *obj)
{
    obj = UnwrapObject(obj);

    
    
    obj = GetCurrentOuter(cx, obj);

    JSObject *waiver = GetXrayWaiver(obj);
    if (waiver)
        return waiver;
    return CreateXrayWaiver(cx, obj);
}





JSObject *
WrapperFactory::DoubleWrap(JSContext *cx, JSObject *obj, unsigned flags)
{
    if (flags & WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG) {
        JSAutoCompartment ac(cx, obj);
        return WaiveXray(cx, obj);
    }
    return obj;
}

JSObject *
WrapperFactory::PrepareForWrapping(JSContext *cx, JSObject *scope, JSObject *obj, unsigned flags)
{
    
    if (js::GetObjectClass(obj)->ext.innerObject)
        return DoubleWrap(cx, obj, flags);

    
    
    JS_ASSERT(!IsWrapper(obj));

    
    
    if (IS_SLIM_WRAPPER(obj) && !MorphSlimWrapper(cx, obj))
        return nullptr;

    
    obj = GetCurrentOuter(cx, obj);
    if (!obj)
        return nullptr;

    if (js::GetObjectClass(obj)->ext.innerObject)
        return DoubleWrap(cx, obj, flags);

    
    
    
    
    
    if (!IS_WN_WRAPPER(obj) || !js::GetObjectParent(obj))
        return DoubleWrap(cx, obj, flags);

    XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));

    JSAutoCompartment ac(cx, obj);
    XPCCallContext ccx(JS_CALLER, cx, obj);

    {
        if (NATIVE_HAS_FLAG(&ccx, WantPreCreate)) {
            
            

            
            
            
            
            JSObject *originalScope = scope;
            nsresult rv = wn->GetScriptableInfo()->GetCallback()->
                PreCreate(wn->Native(), cx, scope, &scope);
            NS_ENSURE_SUCCESS(rv, DoubleWrap(cx, obj, flags));

            
            
            
            
            if (js::GetObjectCompartment(originalScope) != js::GetObjectCompartment(scope))
                return DoubleWrap(cx, obj, flags);

            JSObject *currentScope = JS_GetGlobalForObject(cx, obj);
            if (MOZ_UNLIKELY(scope != currentScope)) {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                JSObject *probe;
                rv = wn->GetScriptableInfo()->GetCallback()->
                    PreCreate(wn->Native(), cx, currentScope, &probe);

                
                if (probe != currentScope) {
                    MOZ_ASSERT(probe == scope);
                    return DoubleWrap(cx, obj, flags);
                }

                
            }

            
            
            if (!AccessCheck::isChrome(js::GetObjectCompartment(scope)) &&
                 AccessCheck::subsumes(js::GetObjectCompartment(scope),
                                       js::GetObjectCompartment(obj)))
            {
                return DoubleWrap(cx, obj, flags);
            }
        }
    }

    
    
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

    
    
    jsval v;
    nsresult rv =
        nsXPConnect::FastGetXPConnect()->WrapNativeToJSVal(cx, scope, wn->Native(), nullptr,
                                                           &NS_GET_IID(nsISupports), false,
                                                           &v, getter_AddRefs(holder));
    if (NS_SUCCEEDED(rv)) {
        obj = JSVAL_TO_OBJECT(v);
        NS_ASSERTION(IS_WN_WRAPPER(obj), "bad object");

        
        
        
        
        
        
        
        
        
        XPCWrappedNative *newwn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));
        XPCNativeSet *unionSet = XPCNativeSet::GetNewOrUsed(ccx, newwn->GetSet(),
                                                            wn->GetSet(), false);
        if (!unionSet)
            return nullptr;
        newwn->SetSet(unionSet);
    }

    return DoubleWrap(cx, obj, flags);
}

static XPCWrappedNative *
GetWrappedNative(JSContext *cx, JSObject *obj)
{
    obj = JS_ObjectToInnerObject(cx, obj);
    return IS_WN_WRAPPER(obj)
           ? static_cast<XPCWrappedNative *>(js::GetObjectPrivate(obj))
           : nullptr;
}

enum XrayType {
    XrayForDOMObject,
    XrayForDOMProxyObject,
    XrayForWrappedNative,
    NotXray
};

static XrayType
GetXrayType(JSObject *obj)
{
    if (mozilla::dom::IsDOMObject(obj))
        return XrayForDOMObject;

    if (mozilla::dom::oldproxybindings::instanceIsProxy(obj))
        return XrayForDOMProxyObject;

    js::Class* clasp = js::GetObjectClass(obj);
    if (IS_WRAPPER_CLASS(clasp) || clasp->ext.innerObject) {
        NS_ASSERTION(clasp->ext.innerObject || IS_WN_WRAPPER_OBJECT(obj),
                     "We forgot to Morph a slim wrapper!");
        return XrayForWrappedNative;
    }
    return NotXray;
}

JSObject *
WrapperFactory::Rewrap(JSContext *cx, JSObject *obj, JSObject *wrappedProto, JSObject *parent,
                       unsigned flags)
{
    NS_ASSERTION(!IsWrapper(obj) ||
                 GetProxyHandler(obj) == &XrayWaiver ||
                 js::GetObjectClass(obj)->ext.innerObject,
                 "wrapped object passed to rewrap");
    NS_ASSERTION(JS_GetClass(obj) != &XrayUtils::HolderClass, "trying to wrap a holder");

    JSCompartment *origin = js::GetObjectCompartment(obj);
    JSCompartment *target = js::GetContextCompartment(cx);
    bool usingXray = false;

    
    
    JSObject *proxyProto = wrappedProto;

    Wrapper *wrapper;
    CompartmentPrivate *targetdata = GetCompartmentPrivate(target);
    if (AccessCheck::isChrome(target)) {
        if (AccessCheck::isChrome(origin)) {
            wrapper = &CrossCompartmentWrapper::singleton;
        } else {
            if (flags & WAIVE_XRAY_WRAPPER_FLAG) {
                
                
                wrapper = &WaiveXrayWrapper::singleton;
            } else {
                
                XrayType type = GetXrayType(obj);
                if (type == XrayForDOMObject) {
                    wrapper = &XrayDOM::singleton;
                } else if (type == XrayForDOMProxyObject) {
                    wrapper = &XrayProxy::singleton;
                } else if (type == XrayForWrappedNative) {
                    typedef XrayWrapper<CrossCompartmentWrapper> Xray;
                    usingXray = true;
                    wrapper = &Xray::singleton;
                } else {
                    wrapper = &CrossCompartmentWrapper::singleton;
                }
            }
        }
    } else if (xpc::IsUniversalXPConnectEnabled(target)) {
        wrapper = &CrossCompartmentWrapper::singleton;
    } else if (AccessCheck::isChrome(origin)) {
        JSFunction *fun = JS_GetObjectFunction(obj);
        if (fun) {
            if (JS_IsBuiltinEvalFunction(fun) || JS_IsBuiltinFunctionConstructor(fun)) {
                JS_ReportError(cx, "Not allowed to access chrome eval or Function from content");
                return nullptr;
            }
        }

        XPCWrappedNative *wn;
        if (targetdata &&
            (wn = GetWrappedNative(cx, obj)) &&
            wn->HasProto() && wn->GetProto()->ClassIsDOMObject()) {
            typedef XrayWrapper<CrossCompartmentSecurityWrapper> Xray;
            usingXray = true;
            if (IsLocationObject(obj))
                wrapper = &FilteringWrapper<Xray, LocationPolicy>::singleton;
            else
                wrapper = &FilteringWrapper<Xray, CrossOriginAccessiblePropertiesOnly>::singleton;
        } else if (mozilla::dom::IsDOMObject(obj)) {
            wrapper = &FilteringWrapper<XrayDOM, CrossOriginAccessiblePropertiesOnly>::singleton;
        } else if (mozilla::dom::oldproxybindings::instanceIsProxy(obj)) {
            wrapper = &FilteringWrapper<XrayProxy, CrossOriginAccessiblePropertiesOnly>::singleton;
        } else if (IsComponentsObject(obj)) {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                        ComponentsObjectPolicy>::singleton;
        } else {
            wrapper = &ChromeObjectWrapper::singleton;

            
            
            
            
            
            
            
            
            
            
            
            JSProtoKey key = JSProto_Null;
            JSObject *unwrappedProto = NULL;
            if (wrappedProto && IsCrossCompartmentWrapper(wrappedProto) &&
                (unwrappedProto = Wrapper::wrappedObject(wrappedProto))) {
                JSAutoCompartment ac(cx, unwrappedProto);
                key = JS_IdentifyClassPrototype(cx, unwrappedProto);
            }
            if (key != JSProto_Null) {
                JSObject *homeProto;
                if (!JS_GetClassPrototype(cx, key, &homeProto))
                    return NULL;
                MOZ_ASSERT(homeProto);
                proxyProto = homeProto;
            }
        }
    } else if (AccessCheck::subsumes(target, origin)) {
        
        
        
        
        
        
        
        
        
        
        XrayType type;
        if (AccessCheck::needsSystemOnlyWrapper(obj)) {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                        OnlyIfSubjectIsSystem>::singleton;
        } else if (IsLocationObject(obj)) {
            typedef XrayWrapper<CrossCompartmentSecurityWrapper> Xray;
            usingXray = true;
            wrapper = &FilteringWrapper<Xray, LocationPolicy>::singleton;
        } else if (IsComponentsObject(obj)) {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                        ComponentsObjectPolicy>::singleton;
        } else if (!targetdata || !targetdata->wantXrays ||
                   (type = GetXrayType(obj)) == NotXray) {
            wrapper = &CrossCompartmentWrapper::singleton;
        } else if (type == XrayForDOMObject) {
            wrapper = &XrayDOM::singleton;
        } else if (type == XrayForDOMProxyObject) {
            wrapper = &XrayProxy::singleton;
        } else {
            typedef XrayWrapper<CrossCompartmentWrapper> Xray;
            usingXray = true;
            wrapper = &Xray::singleton;
        }
    } else {
        NS_ASSERTION(!AccessCheck::needsSystemOnlyWrapper(obj),
                     "bad object exposed across origins");

        
        
        
        
        XrayType type = GetXrayType(obj);
        if (type == NotXray) {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                        CrossOriginAccessiblePropertiesOnly>::singleton;
        } else if (type == XrayForDOMObject) {
            wrapper = &FilteringWrapper<XrayDOM,
                                        CrossOriginAccessiblePropertiesOnly>::singleton;
        } else if (type == XrayForDOMProxyObject) {
            wrapper = &FilteringWrapper<XrayProxy,
                                        CrossOriginAccessiblePropertiesOnly>::singleton;
        } else {
            typedef XrayWrapper<CrossCompartmentSecurityWrapper> Xray;
            usingXray = true;

            
            
            if (IsLocationObject(obj)) {
                wrapper = &FilteringWrapper<Xray, LocationPolicy>::singleton;
            } else {
                wrapper = &FilteringWrapper<Xray,
                    CrossOriginAccessiblePropertiesOnly>::singleton;
            }
        }
    }

    JSObject *wrapperObj = Wrapper::New(cx, obj, proxyProto, parent, wrapper);
    if (!wrapperObj || !usingXray)
        return wrapperObj;

    JSObject *xrayHolder = XrayUtils::createHolder(cx, obj, parent);
    if (!xrayHolder)
        return nullptr;
    js::SetProxyExtra(wrapperObj, 0, js::ObjectValue(*xrayHolder));
    return wrapperObj;
}

JSObject *
WrapperFactory::WrapForSameCompartment(JSContext *cx, JSObject *obj)
{
    
    
    
    
    
    if (!IS_WN_WRAPPER(obj))
        return obj;

    
    XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));
    MOZ_ASSERT(wn, "Trying to wrap a dead WN!");

    
    return wn->GetSameCompartmentSecurityWrapper(cx);
}

typedef FilteringWrapper<XrayWrapper<SameCompartmentSecurityWrapper>, LocationPolicy> LW;

bool
WrapperFactory::IsLocationObject(JSObject *obj)
{
    const char *name = js::GetObjectClass(obj)->name;
    return name[0] == 'L' && !strcmp(name, "Location");
}

JSObject *
WrapperFactory::WrapLocationObject(JSContext *cx, JSObject *obj)
{
    JSObject *xrayHolder = XrayUtils::createHolder(cx, obj, js::GetObjectParent(obj));
    if (!xrayHolder)
        return nullptr;
    JSObject *wrapperObj = Wrapper::New(cx, obj, js::GetObjectProto(obj), js::GetObjectParent(obj),
                                        &LW::singleton);
    if (!wrapperObj)
        return nullptr;
    js::SetProxyExtra(wrapperObj, 0, js::ObjectValue(*xrayHolder));
    return wrapperObj;
}





bool
WrapperFactory::WaiveXrayAndWrap(JSContext *cx, jsval *vp)
{
    if (JSVAL_IS_PRIMITIVE(*vp))
        return JS_WrapValue(cx, vp);

    JSObject *obj = js::UnwrapObject(JSVAL_TO_OBJECT(*vp));
    obj = GetCurrentOuter(cx, obj);
    if (js::IsObjectInContextCompartment(obj, cx)) {
        *vp = OBJECT_TO_JSVAL(obj);
        return true;
    }

    obj = WaiveXray(cx, obj);
    if (!obj)
        return false;

    *vp = OBJECT_TO_JSVAL(obj);
    return JS_WrapValue(cx, vp);
}

JSObject *
WrapperFactory::WrapSOWObject(JSContext *cx, JSObject *obj)
{
    JSObject *wrapperObj =
        Wrapper::New(cx, obj, JS_GetPrototype(obj), JS_GetGlobalForObject(cx, obj),
                     &FilteringWrapper<SameCompartmentSecurityWrapper,
                     OnlyIfSubjectIsSystem>::singleton);
    return wrapperObj;
}

bool
WrapperFactory::IsComponentsObject(JSObject *obj)
{
    const char *name = js::GetObjectClass(obj)->name;
    return name[0] == 'n' && !strcmp(name, "nsXPCComponents");
}

JSObject *
WrapperFactory::WrapComponentsObject(JSContext *cx, JSObject *obj)
{
    JSObject *wrapperObj =
        Wrapper::New(cx, obj, JS_GetPrototype(obj), JS_GetGlobalForObject(cx, obj),
                     &FilteringWrapper<SameCompartmentSecurityWrapper, ComponentsObjectPolicy>::singleton);

    return wrapperObj;
}

JSObject *
WrapperFactory::WrapForSameCompartmentXray(JSContext *cx, JSObject *obj)
{
    
    MOZ_ASSERT(js::IsObjectInContextCompartment(obj, cx));

    
    XrayType type = GetXrayType(obj);
    if (type == NotXray)
        return NULL;

    
    Wrapper *wrapper = NULL;
    if (type == XrayForWrappedNative)
        wrapper = &XrayWrapper<DirectWrapper>::singleton;
    else if (type == XrayForDOMProxyObject)
        wrapper = &XrayWrapper<DirectWrapper, ProxyXrayTraits>::singleton;
    else if (type == XrayForDOMObject)
        wrapper = &XrayWrapper<DirectWrapper, DOMXrayTraits>::singleton;
    else
        MOZ_NOT_REACHED("Bad Xray type");

    
    JSObject *parent = JS_GetGlobalForObject(cx, obj);
    JSObject *wrapperObj = Wrapper::New(cx, obj, NULL, parent, wrapper);
    if (!wrapperObj)
        return NULL;

    
    
    if (type == XrayForWrappedNative) {
        JSObject *xrayHolder = XrayUtils::createHolder(cx, obj, parent);
        if (!xrayHolder)
            return nullptr;
        js::SetProxyExtra(wrapperObj, 0, js::ObjectValue(*xrayHolder));
    }
    return wrapperObj;
}


bool
WrapperFactory::XrayWrapperNotShadowing(JSObject *wrapper, jsid id)
{
    ResolvingId *rid = ResolvingId::getResolvingIdFromWrapper(wrapper);
    return rid->isXrayShadowing(id);
}






static bool
FixWaiverAfterTransplant(JSContext *cx, JSObject *oldWaiver, JSObject *newobj)
{
    MOZ_ASSERT(Wrapper::wrapperHandler(oldWaiver) == &XrayWaiver);
    MOZ_ASSERT(!js::IsCrossCompartmentWrapper(newobj));

    
    
    
    
    JSObject *newWaiver = WrapperFactory::CreateXrayWaiver(cx, newobj);
    if (!newWaiver)
        return false;

    
    
    if (!js::RemapAllWrappersForObject(cx, oldWaiver, newWaiver))
        return false;

    
    
    
    CompartmentPrivate *priv = GetCompartmentPrivate(oldWaiver);
    JSObject *key = Wrapper::wrappedObject(oldWaiver);
    MOZ_ASSERT(priv->waiverWrapperMap->Find(key));
    priv->waiverWrapperMap->Remove(key);
    return true;
}

JSObject *
TransplantObject(JSContext *cx, JSObject *origobj, JSObject *target)
{
    JSObject *oldWaiver = WrapperFactory::GetXrayWaiver(origobj);
    JSObject *newIdentity = JS_TransplantObject(cx, origobj, target);
    if (!newIdentity || !oldWaiver)
       return newIdentity;

    if (!FixWaiverAfterTransplant(cx, oldWaiver, newIdentity))
        return NULL;
    return newIdentity;
}

JSObject *
TransplantObjectWithWrapper(JSContext *cx,
                            JSObject *origobj, JSObject *origwrapper,
                            JSObject *targetobj, JSObject *targetwrapper)
{
    JSObject *oldWaiver = WrapperFactory::GetXrayWaiver(origobj);
    JSObject *newSameCompartmentWrapper =
      js_TransplantObjectWithWrapper(cx, origobj, origwrapper, targetobj,
                                     targetwrapper);
    if (!newSameCompartmentWrapper || !oldWaiver)
        return newSameCompartmentWrapper;

    JSObject *newIdentity = Wrapper::wrappedObject(newSameCompartmentWrapper);
    JS_ASSERT(js::IsWrapper(newIdentity));
    if (!FixWaiverAfterTransplant(cx, oldWaiver, newIdentity))
        return NULL;
    return newSameCompartmentWrapper;
}

}
