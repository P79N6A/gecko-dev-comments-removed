






#include "CrossOriginWrapper.h"
#include "FilteringWrapper.h"
#include "XrayWrapper.h"
#include "AccessCheck.h"
#include "XPCWrapper.h"

#include "xpcprivate.h"
#include "dombindings.h"
#include "XPCMaps.h"
#include "mozilla/dom/BindingUtils.h"
#include "jsfriendapi.h"
#include "mozilla/Likely.h"

using namespace js;

namespace xpc {








DirectWrapper WaiveXrayWrapperWrapper(WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG);






NoWaiverWrapper NoWaiverWrapper::singleton(0);





CrossOriginWrapper CrossOriginWrapper::singleton(0);

static JSObject *
GetCurrentOuter(JSContext *cx, JSObject *obj)
{
    obj = JS_ObjectToOuterObject(cx, obj);
    if (!obj)
        return nsnull;

    if (IsWrapper(obj) && !js::GetObjectClass(obj)->ext.innerObject) {
        obj = UnwrapObject(obj);
        NS_ASSERTION(js::GetObjectClass(obj)->ext.innerObject,
                     "weird object, expecting an outer window proxy");
    }

    return obj;
}

JSObject *
WrapperFactory::WaiveXray(JSContext *cx, JSObject *obj)
{
    obj = UnwrapObject(obj);

    
    
    obj = GetCurrentOuter(cx, obj);

    {
        
        CompartmentPrivate *priv = GetCompartmentPrivate(obj);
        JSObject *wobj = nsnull;
        if (priv && priv->waiverWrapperMap) {
            wobj = priv->waiverWrapperMap->Find(obj);
            xpc_UnmarkGrayObject(wobj);
        }

        
        if (!wobj) {
            JSObject *proto = js::GetObjectProto(obj);
            if (proto && !(proto = WaiveXray(cx, proto)))
                return nsnull;

            JSAutoEnterCompartment ac;
            if (!ac.enter(cx, obj) || !JS_WrapObject(cx, &proto))
                return nsnull;
            wobj = Wrapper::New(cx, obj, proto, JS_GetGlobalForObject(cx, obj),
                                &WaiveXrayWrapperWrapper);
            if (!wobj)
                return nsnull;

            
            if (priv) {
                if (!priv->waiverWrapperMap) {
                    priv->waiverWrapperMap = JSObject2JSObjectMap::newMap(XPC_WRAPPER_MAP_SIZE);
                    if (!priv->waiverWrapperMap)
                        return nsnull;
                }
                if (!priv->waiverWrapperMap->Add(obj, wobj))
                    return nsnull;
            }
        }

        obj = wobj;
    }

    return obj;
}





JSObject *
WrapperFactory::DoubleWrap(JSContext *cx, JSObject *obj, unsigned flags)
{
    if (flags & WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG) {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, obj))
            return nsnull;

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
        return nsnull;

    
    obj = GetCurrentOuter(cx, obj);
    if (!obj)
        return nsnull;

    if (js::GetObjectClass(obj)->ext.innerObject)
        return DoubleWrap(cx, obj, flags);

    
    
    
    
    
    if (!IS_WN_WRAPPER(obj) || !js::GetObjectParent(obj))
        return DoubleWrap(cx, obj, flags);

    XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));

    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, obj))
        return nsnull;
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
        }
    }

    
    
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

    
    
    jsval v;
    nsresult rv =
        nsXPConnect::FastGetXPConnect()->WrapNativeToJSVal(cx, scope, wn->Native(), nsnull,
                                                           &NS_GET_IID(nsISupports), false,
                                                           &v, getter_AddRefs(holder));
    if (NS_SUCCEEDED(rv)) {
        obj = JSVAL_TO_OBJECT(v);
        NS_ASSERTION(IS_WN_WRAPPER(obj), "bad object");

        
        
        
        
        
        
        
        
        
        XPCWrappedNative *newwn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));
        XPCNativeSet *unionSet = XPCNativeSet::GetNewOrUsed(ccx, newwn->GetSet(),
                                                            wn->GetSet(), false);
        if (!unionSet)
            return nsnull;
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
           : nsnull;
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
    js::Class* clasp = js::GetObjectClass(obj);
    if (mozilla::dom::IsDOMClass(Jsvalify(clasp))) {
        return XrayForDOMObject;
    }
    if (mozilla::dom::binding::instanceIsProxy(obj)) {
        return XrayForDOMProxyObject;
    }
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
                 GetProxyHandler(obj) == &WaiveXrayWrapperWrapper ||
                 js::GetObjectClass(obj)->ext.innerObject,
                 "wrapped object passed to rewrap");
    NS_ASSERTION(JS_GetClass(obj) != &XrayUtils::HolderClass, "trying to wrap a holder");

    JSCompartment *origin = js::GetObjectCompartment(obj);
    JSCompartment *target = js::GetContextCompartment(cx);
    bool usingXray = false;

    Wrapper *wrapper;
    CompartmentPrivate *targetdata = GetCompartmentPrivate(target);
    if (AccessCheck::isChrome(target)) {
        if (AccessCheck::isChrome(origin)) {
            wrapper = &CrossCompartmentWrapper::singleton;
        } else {
            bool isSystem;
            {
                JSAutoEnterCompartment ac;
                if (!ac.enter(cx, obj))
                    return nsnull;
                JSObject *globalObj = JS_GetGlobalForObject(cx, obj);
                JS_ASSERT(globalObj);
                isSystem = JS_IsSystemObject(cx, globalObj);
            }

            if (isSystem) {
                wrapper = &CrossCompartmentWrapper::singleton;
            } else if (flags & WAIVE_XRAY_WRAPPER_FLAG) {
                
                
                wrapper = &CrossOriginWrapper::singleton;
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
                    wrapper = &NoWaiverWrapper::singleton;
                }
            }
        }
    } else if (AccessCheck::isChrome(origin)) {
        JSFunction *fun = JS_GetObjectFunction(obj);
        if (fun) {
            if (JS_IsBuiltinEvalFunction(fun) || JS_IsBuiltinFunctionConstructor(fun)) {
                JS_ReportError(cx, "Not allowed to access chrome eval or Function from content");
                return nsnull;
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
        } else if (mozilla::dom::binding::instanceIsProxy(obj)) {
            wrapper = &FilteringWrapper<XrayProxy, CrossOriginAccessiblePropertiesOnly>::singleton;
        } else if (mozilla::dom::IsDOMClass(JS_GetClass(obj))) {
            wrapper = &FilteringWrapper<XrayDOM, CrossOriginAccessiblePropertiesOnly>::singleton;
        } else if (IsComponentsObject(obj)) {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                        ComponentsObjectPolicy>::singleton;
        } else {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                        ExposedPropertiesOnly>::singleton;
        }
    } else if (AccessCheck::isSameOrigin(origin, target)) {
        
        
        
        
        
        
        
        
        
        
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

    JSObject *wrapperObj = Wrapper::New(cx, obj, wrappedProto, parent, wrapper);
    if (!wrapperObj || !usingXray)
        return wrapperObj;

    JSObject *xrayHolder = XrayUtils::createHolder(cx, obj, parent);
    if (!xrayHolder)
        return nsnull;
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
        return nsnull;
    JSObject *wrapperObj = Wrapper::New(cx, obj, js::GetObjectProto(obj), js::GetObjectParent(obj),
                                        &LW::singleton);
    if (!wrapperObj)
        return nsnull;
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

}
