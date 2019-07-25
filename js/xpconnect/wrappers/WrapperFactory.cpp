






































#include "WrapperFactory.h"
#include "CrossOriginWrapper.h"
#include "FilteringWrapper.h"
#include "XrayWrapper.h"
#include "AccessCheck.h"
#include "XPCWrapper.h"

#include "xpcprivate.h"
#include "dombindings.h"
#include "XPCMaps.h"

using namespace js;

namespace xpc {








Wrapper WaiveXrayWrapperWrapper(WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG);






NoWaiverWrapper NoWaiverWrapper::singleton(0);





CrossOriginWrapper CrossOriginWrapper::singleton(0);

static JSObject *
GetCurrentOuter(JSContext *cx, JSObject *obj)
{
    obj = JS_ObjectToOuterObject(cx, obj);
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
        
        CompartmentPrivate *priv =
            (CompartmentPrivate *)JS_GetCompartmentPrivate(cx, js::GetObjectCompartment(obj));
        JSObject *wobj = nsnull;
        if (priv && priv->waiverWrapperMap)
            wobj = priv->waiverWrapperMap->Find(obj);

        
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
WrapperFactory::DoubleWrap(JSContext *cx, JSObject *obj, uintN flags)
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
WrapperFactory::PrepareForWrapping(JSContext *cx, JSObject *scope, JSObject *obj, uintN flags)
{
    
    if (js::GetObjectClass(obj)->ext.innerObject)
        return DoubleWrap(cx, obj, flags);

    
    
    JS_ASSERT(!IsWrapper(obj));

    
    
    if (IS_SLIM_WRAPPER(obj) && !MorphSlimWrapper(cx, obj))
        return nsnull;

    
    obj = GetCurrentOuter(cx, obj);
    if (js::GetObjectClass(obj)->ext.innerObject)
        return DoubleWrap(cx, obj, flags);

    
    
    
    
    
    if (!IS_WN_WRAPPER(obj) || !js::GetObjectParent(obj))
        return DoubleWrap(cx, obj, flags);

    XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));

    
    
    if (!wn->GetClassInfo())
        return DoubleWrap(cx, obj, flags);

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

            
            
            
            
        }
    }

    
    
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

    
    
    jsval v;
    nsresult rv =
        nsXPConnect::FastGetXPConnect()->WrapNativeToJSVal(cx, scope, wn->Native(), nsnull,
                                                           &NS_GET_IID(nsISupports), PR_FALSE,
                                                           &v, getter_AddRefs(holder));
    if (NS_SUCCEEDED(rv)) {
        obj = JSVAL_TO_OBJECT(v);
        NS_ASSERTION(IS_WN_WRAPPER(obj), "bad object");

        XPCWrappedNative *newwn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));
        if (newwn->GetSet()->GetInterfaceCount() < wn->GetSet()->GetInterfaceCount())
            newwn->SetSet(wn->GetSet());
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

static bool
CanXray(JSObject *obj, bool *proxy)
{
    if (IS_WN_WRAPPER(obj) || js::GetObjectClass(obj)->ext.innerObject) {
        *proxy = false;
        return true;
    }
    return (*proxy = mozilla::dom::binding::instanceIsProxy(obj));
}

JSObject *
WrapperFactory::Rewrap(JSContext *cx, JSObject *obj, JSObject *wrappedProto, JSObject *parent,
                       uintN flags)
{
    NS_ASSERTION(!IsWrapper(obj) ||
                 GetProxyHandler(obj) == &WaiveXrayWrapperWrapper ||
                 js::GetObjectClass(obj)->ext.innerObject,
                 "wrapped object passed to rewrap");
    NS_ASSERTION(JS_GET_CLASS(cx, obj) != &XrayUtils::HolderClass, "trying to wrap a holder");

    JSCompartment *origin = js::GetObjectCompartment(obj);
    JSCompartment *target = cx->compartment;
    JSObject *xrayHolder = nsnull;

    Wrapper *wrapper;
    CompartmentPrivate *targetdata =
        static_cast<CompartmentPrivate *>(JS_GetCompartmentPrivate(cx, target));
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
                
                bool proxy;
                if (CanXray(obj, &proxy)) {
                    if (proxy) {
                        wrapper = &XrayProxy::singleton;
                    } else {
                        typedef XrayWrapper<CrossCompartmentSecurityWrapper> Xray;
                        wrapper = &Xray::singleton;
                        xrayHolder = Xray::createHolder(cx, obj, parent);
                        if (!xrayHolder)
                            return nsnull;
                    }
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
            wrapper = &FilteringWrapper<Xray,
                                        CrossOriginAccessiblePropertiesOnly>::singleton;
            xrayHolder = Xray::createHolder(cx, obj, parent);
            if (!xrayHolder)
                return nsnull;
        } else {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                        ExposedPropertiesOnly>::singleton;
        }
    } else if (AccessCheck::isSameOrigin(origin, target)) {
        
        
        bool proxy;
        if (AccessCheck::needsSystemOnlyWrapper(obj)) {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                        OnlyIfSubjectIsSystem>::singleton;
        } else if (targetdata && targetdata->wantXrays && CanXray(obj, &proxy)) {
            if (proxy) {
                wrapper = &XrayProxy::singleton;
            } else {
                typedef XrayWrapper<CrossCompartmentWrapper> Xray;
                wrapper = &Xray::singleton;
                xrayHolder = Xray::createHolder(cx, obj, parent);
                if (!xrayHolder)
                    return nsnull;
            }
        } else {
            wrapper = &CrossCompartmentWrapper::singleton;
        }
    } else {
        NS_ASSERTION(!AccessCheck::needsSystemOnlyWrapper(obj),
                     "bad object exposed across origins");

        
        
        
        
        bool proxy;
        if (!CanXray(obj, &proxy)) {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                        CrossOriginAccessiblePropertiesOnly>::singleton;
        } else {
            if (proxy) {
                wrapper = &FilteringWrapper<XrayProxy,
                                            CrossOriginAccessiblePropertiesOnly>::singleton;
            } else {
                typedef XrayWrapper<CrossCompartmentSecurityWrapper> Xray;

                
                
                if (IsLocationObject(obj)) {
                    wrapper = &FilteringWrapper<Xray,
                        SameOriginOrCrossOriginAccessiblePropertiesOnly>::singleton;
                } else {
                    wrapper = &FilteringWrapper<Xray,
                        CrossOriginAccessiblePropertiesOnly>::singleton;
                }

                xrayHolder = Xray::createHolder(cx, obj, parent);
                if (!xrayHolder)
                    return nsnull;
            }
        }
    }

    JSObject *wrapperObj = Wrapper::New(cx, obj, wrappedProto, parent, wrapper);
    if (!wrapperObj || !xrayHolder)
        return wrapperObj;

    
    
    js::SetProxyExtra(wrapperObj, 0, js::ObjectValue(*xrayHolder));
    return wrapperObj;
}

typedef FilteringWrapper<XrayWrapper<SameCompartmentSecurityWrapper>,
                         SameOriginOrCrossOriginAccessiblePropertiesOnly> LW;

bool
WrapperFactory::IsLocationObject(JSObject *obj)
{
    const char *name = js::GetObjectClass(obj)->name;
    return name[0] == 'L' && !strcmp(name, "Location");
}

JSObject *
WrapperFactory::WrapLocationObject(JSContext *cx, JSObject *obj)
{
    JSObject *xrayHolder = LW::createHolder(cx, obj, js::GetObjectParent(obj));
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
    if (js::GetObjectCompartment(obj) == cx->compartment) {
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
        Wrapper::New(cx, obj, JS_GetPrototype(cx, obj), JS_GetGlobalForObject(cx, obj),
                     &FilteringWrapper<SameCompartmentSecurityWrapper,
                                       OnlyIfSubjectIsSystem>::singleton);
    return wrapperObj;
}

}
