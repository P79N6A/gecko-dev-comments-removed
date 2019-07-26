






#include "WaiveXrayWrapper.h"
#include "FilteringWrapper.h"
#include "XrayWrapper.h"
#include "AccessCheck.h"
#include "XPCWrapper.h"
#include "ChromeObjectWrapper.h"

#include "xpcprivate.h"
#include "XPCMaps.h"
#include "mozilla/dom/BindingUtils.h"
#include "jsfriendapi.h"
#include "mozilla/Likely.h"

using namespace js;
using namespace mozilla;

namespace xpc {








Wrapper XrayWaiver(WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG);





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
    XPCWrappedNativeScope *scope = GetObjectScope(obj);
    MOZ_ASSERT(scope);

    if (!scope->mWaiverWrapperMap)
        return NULL;
    return xpc_UnmarkGrayObject(scope->mWaiverWrapperMap->Find(obj));
}

JSObject *
WrapperFactory::CreateXrayWaiver(JSContext *cx, JSObject *obj)
{
    
    
    MOZ_ASSERT(!GetXrayWaiver(obj));
    XPCWrappedNativeScope *scope = GetObjectScope(obj);

    
    JSObject *proto;
    if (!js::GetObjectProto(cx, obj, &proto))
        return nullptr;
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

    
    
    if (!scope->mWaiverWrapperMap) {
        scope->mWaiverWrapperMap =
          JSObject2JSObjectMap::newMap(XPC_WRAPPER_MAP_SIZE);
        MOZ_ASSERT(scope->mWaiverWrapperMap);
    }
    if (!scope->mWaiverWrapperMap->Add(obj, waiver))
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

    
    
    MOZ_ASSERT(!IsWrapper(obj));

    
    
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
                 AccessCheck::subsumesIgnoringDomain(js::GetObjectCompartment(scope),
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

#ifdef DEBUG
static void
DEBUG_CheckUnwrapSafety(JSObject *obj, js::Wrapper *handler,
                        JSCompartment *origin, JSCompartment *target)
{
    typedef FilteringWrapper<CrossCompartmentSecurityWrapper, OnlyIfSubjectIsSystem> XSOW;

    if (AccessCheck::isChrome(target) || xpc::IsUniversalXPConnectEnabled(target)) {
        
        MOZ_ASSERT(handler->isSafeToUnwrap());
    } else if (WrapperFactory::IsComponentsObject(obj) ||
               handler == &XSOW::singleton)
    {
        
        MOZ_ASSERT(!handler->isSafeToUnwrap());
    } else {
        
        MOZ_ASSERT(handler->isSafeToUnwrap() == AccessCheck::subsumes(target, origin));
    }
}
#else
#define DEBUG_CheckUnwrapSafety(obj, handler, origin, target) {}
#endif

JSObject *
WrapperFactory::Rewrap(JSContext *cx, JSObject *existing, JSObject *obj,
                       JSObject *wrappedProto, JSObject *parent,
                       unsigned flags)
{
    MOZ_ASSERT(!IsWrapper(obj) ||
               GetProxyHandler(obj) == &XrayWaiver ||
               js::GetObjectClass(obj)->ext.innerObject,
               "wrapped object passed to rewrap");
    MOZ_ASSERT(JS_GetClass(obj) != &XrayUtils::HolderClass, "trying to wrap a holder");

    JSCompartment *origin = js::GetObjectCompartment(obj);
    JSCompartment *target = js::GetContextCompartment(cx);

    
    
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
                    wrapper = &PermissiveXrayDOM::singleton;
                } else if (type == XrayForWrappedNative) {
                    wrapper = &PermissiveXrayXPCWN::singleton;
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
            wrapper = &FilteringWrapper<SecurityXrayXPCWN, CrossOriginAccessiblePropertiesOnly>::singleton;
        } else if (mozilla::dom::UseDOMXray(obj)) {
            wrapper = &FilteringWrapper<SecurityXrayDOM, CrossOriginAccessiblePropertiesOnly>::singleton;
        } else if (IsComponentsObject(obj)) {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                        ComponentsObjectPolicy>::singleton;
        } else {
            wrapper = &ChromeObjectWrapper::singleton;

            
            
            
            
            
            
            
            
            
            
            
            JSProtoKey key = JSProto_Null;
            {
                JSAutoCompartment ac(cx, obj);
                JSObject *unwrappedProto;
                if (!js::GetObjectProto(cx, obj, &unwrappedProto))
                    return NULL;
                if (unwrappedProto && IsCrossCompartmentWrapper(unwrappedProto))
                    unwrappedProto = Wrapper::wrappedObject(unwrappedProto);
                if (unwrappedProto) {
                    JSAutoCompartment ac2(cx, unwrappedProto);
                    key = JS_IdentifyClassPrototype(cx, unwrappedProto);
                }
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
        } else if (IsComponentsObject(obj)) {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                        ComponentsObjectPolicy>::singleton;
        } else if (!targetdata || !targetdata->wantXrays ||
                   (type = GetXrayType(obj)) == NotXray) {
            wrapper = &CrossCompartmentWrapper::singleton;
        } else if (type == XrayForDOMObject) {
            wrapper = &PermissiveXrayDOM::singleton;
        } else {
            wrapper = &PermissiveXrayXPCWN::singleton;
        }
    } else {
        NS_ASSERTION(!AccessCheck::needsSystemOnlyWrapper(obj),
                     "bad object exposed across origins");

        
        
        XrayType type = GetXrayType(obj);
        if (type == NotXray) {
            wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper, Opaque>::singleton;
        } else if (type == XrayForDOMObject) {
            wrapper = &FilteringWrapper<SecurityXrayDOM,
                                        CrossOriginAccessiblePropertiesOnly>::singleton;
        } else {
            wrapper = &FilteringWrapper<SecurityXrayXPCWN,
                                        CrossOriginAccessiblePropertiesOnly>::singleton;
        }
    }

    DEBUG_CheckUnwrapSafety(obj, wrapper, origin, target);

    if (existing && proxyProto == wrappedProto)
        return Wrapper::Renew(cx, existing, obj, wrapper);

    return Wrapper::New(cx, obj, proxyProto, parent, wrapper);
}

JSObject *
WrapperFactory::WrapForSameCompartment(JSContext *cx, JSObject *obj)
{
    
    
    

    if (dom::GetSameCompartmentWrapperForDOMBinding(obj)) {
        return obj;
    }

    MOZ_ASSERT(!dom::IsDOMObject(obj));

    if (!IS_WN_WRAPPER(obj))
        return obj;

    
    XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));
    MOZ_ASSERT(wn, "Trying to wrap a dead WN!");

    
    JSObject *wrapper = wn->GetSameCompartmentSecurityWrapper(cx);
    MOZ_ASSERT_IF(wrapper != obj, !Wrapper::wrapperHandler(wrapper)->isSafeToUnwrap());
    return wrapper;
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
    JSObject *proto;
    if (!JS_GetPrototype(cx, obj, &proto))
        return NULL;
    JSObject *wrapperObj =
        Wrapper::New(cx, obj, proto, JS_GetGlobalForObject(cx, obj),
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
    JSObject *proto;
    if (!JS_GetPrototype(cx, obj, &proto))
        return NULL;
    JSObject *wrapperObj =
        Wrapper::New(cx, obj, proto, JS_GetGlobalForObject(cx, obj),
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
        wrapper = &SCPermissiveXrayXPCWN::singleton;
    else if (type == XrayForDOMObject)
        wrapper = &SCPermissiveXrayDOM::singleton;
    else
        MOZ_NOT_REACHED("Bad Xray type");

    
    JSObject *parent = JS_GetGlobalForObject(cx, obj);
    return Wrapper::New(cx, obj, NULL, parent, wrapper);
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

    
    
    
    XPCWrappedNativeScope *scope = GetObjectScope(oldWaiver);
    JSObject *key = Wrapper::wrappedObject(oldWaiver);
    MOZ_ASSERT(scope->mWaiverWrapperMap->Find(key));
    scope->mWaiverWrapperMap->Remove(key);
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
    MOZ_ASSERT(js::IsWrapper(newIdentity));
    if (!FixWaiverAfterTransplant(cx, oldWaiver, newIdentity))
        return NULL;
    return newSameCompartmentWrapper;
}

}
