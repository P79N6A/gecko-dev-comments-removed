






#include "WaiveXrayWrapper.h"
#include "FilteringWrapper.h"
#include "XrayWrapper.h"
#include "AccessCheck.h"
#include "XPCWrapper.h"
#include "ChromeObjectWrapper.h"
#include "WrapperFactory.h"

#include "xpcprivate.h"
#include "XPCMaps.h"
#include "mozilla/dom/BindingUtils.h"
#include "jsfriendapi.h"
#include "mozilla/Likely.h"
#include "nsContentUtils.h"

using namespace JS;
using namespace js;
using namespace mozilla;

namespace xpc {








Wrapper XrayWaiver(WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG);





WaiveXrayWrapper WaiveXrayWrapper::singleton(0);

bool
WrapperFactory::IsCOW(JSObject *obj)
{
    return IsWrapper(obj) &&
           Wrapper::wrapperHandler(obj) == &ChromeObjectWrapper::singleton;
}

JSObject *
WrapperFactory::GetXrayWaiver(JSObject *obj)
{
    
    MOZ_ASSERT(obj == UncheckedUnwrap(obj));
    MOZ_ASSERT(!js::GetObjectClass(obj)->ext.outerObject);
    XPCWrappedNativeScope *scope = GetObjectScope(obj);
    MOZ_ASSERT(scope);

    if (!scope->mWaiverWrapperMap)
        return nullptr;

    JSObject* xrayWaiver = scope->mWaiverWrapperMap->Find(obj);
    if (xrayWaiver)
        JS::ExposeObjectToActiveJS(xrayWaiver);

    return xrayWaiver;
}

JSObject *
WrapperFactory::CreateXrayWaiver(JSContext *cx, HandleObject obj)
{
    
    
    MOZ_ASSERT(!GetXrayWaiver(obj));
    XPCWrappedNativeScope *scope = GetObjectScope(obj);

    
    RootedObject proto(cx);
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
    if (!scope->mWaiverWrapperMap->Add(cx, obj, waiver))
        return nullptr;
    return waiver;
}

JSObject *
WrapperFactory::WaiveXray(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    obj = UncheckedUnwrap(obj);
    MOZ_ASSERT(!js::IsInnerObject(obj));

    JSObject *waiver = GetXrayWaiver(obj);
    if (waiver)
        return waiver;
    return CreateXrayWaiver(cx, obj);
}





JSObject *
WrapperFactory::DoubleWrap(JSContext *cx, HandleObject obj, unsigned flags)
{
    if (flags & WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG) {
        JSAutoCompartment ac(cx, obj);
        return WaiveXray(cx, obj);
    }
    return obj;
}

JSObject *
WrapperFactory::PrepareForWrapping(JSContext *cx, HandleObject scope,
                                   HandleObject objArg, unsigned flags)
{
    RootedObject obj(cx, objArg);
    
    
    if (js::IsInnerObject(obj)) {
        JSAutoCompartment ac(cx, obj);
        obj = JS_ObjectToOuterObject(cx, obj);
        NS_ENSURE_TRUE(obj, nullptr);
        
        
        obj = js::UncheckedUnwrap(obj);
        MOZ_ASSERT(js::IsOuterObject(obj));
    }

    
    
    
    if (js::IsOuterObject(obj))
        return DoubleWrap(cx, obj, flags);

    
    
    MOZ_ASSERT(!IsWrapper(obj));

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool subsumes = AccessCheck::subsumesIgnoringDomain(js::GetContextCompartment(cx),
                                                        js::GetObjectCompartment(obj));
    XrayType xrayType = GetXrayType(obj);
    if (!subsumes && xrayType == NotXray) {
        JSProtoKey key = JSProto_Null;
        {
            JSAutoCompartment ac(cx, obj);
            key = JS_IdentifyClassPrototype(cx, obj);
        }
        if (key != JSProto_Null) {
            RootedObject homeProto(cx);
            if (!JS_GetClassPrototype(cx, key, homeProto.address()))
                return nullptr;
            MOZ_ASSERT(homeProto);
            
            
            return homeProto;
        }
    }

    
    
    
    
    
    if (!IS_WN_REFLECTOR(obj) || !js::GetObjectParent(obj))
        return DoubleWrap(cx, obj, flags);

    XPCWrappedNative *wn = XPCWrappedNative::Get(obj);

    JSAutoCompartment ac(cx, obj);
    XPCCallContext ccx(JS_CALLER, cx, obj);
    RootedObject wrapScope(cx, scope);

    {
        if (NATIVE_HAS_FLAG(&ccx, WantPreCreate)) {
            
            

            
            
            
            
            nsresult rv = wn->GetScriptableInfo()->GetCallback()->
                PreCreate(wn->Native(), cx, scope, wrapScope.address());
            NS_ENSURE_SUCCESS(rv, DoubleWrap(cx, obj, flags));

            
            
            
            
            if (js::GetObjectCompartment(scope) != js::GetObjectCompartment(wrapScope))
                return DoubleWrap(cx, obj, flags);

            RootedObject currentScope(cx, JS_GetGlobalForObject(cx, obj));
            if (MOZ_UNLIKELY(wrapScope != currentScope)) {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                RootedObject probe(cx);
                rv = wn->GetScriptableInfo()->GetCallback()->
                    PreCreate(wn->Native(), cx, currentScope, probe.address());

                
                if (probe != currentScope) {
                    MOZ_ASSERT(probe == wrapScope);
                    return DoubleWrap(cx, obj, flags);
                }

                
            }

            
            
            
            
            
            
            
            
            
            
            
            
            if (!AccessCheck::isChrome(js::GetObjectCompartment(wrapScope)) &&
                 AccessCheck::subsumesIgnoringDomain(js::GetObjectCompartment(wrapScope),
                                                     js::GetObjectCompartment(obj)))
            {
                return DoubleWrap(cx, obj, flags);
            }
        }
    }

    
    
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

    
    
    RootedValue v(cx);
    nsresult rv =
        nsXPConnect::XPConnect()->WrapNativeToJSVal(cx, wrapScope, wn->Native(), nullptr,
                                                    &NS_GET_IID(nsISupports), false,
                                                    v.address(), getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, nullptr);

    obj = JSVAL_TO_OBJECT(v);
    MOZ_ASSERT(IS_WN_REFLECTOR(obj), "bad object");

    
    
    
    
    
    
    
    
    
    XPCWrappedNative *newwn = XPCWrappedNative::Get(obj);
    XPCNativeSet *unionSet = XPCNativeSet::GetNewOrUsed(newwn->GetSet(),
                                                        wn->GetSet(), false);
    if (!unionSet)
        return nullptr;
    newwn->SetSet(unionSet);

    return DoubleWrap(cx, obj, flags);
}

#ifdef DEBUG
static void
DEBUG_CheckUnwrapSafety(HandleObject obj, js::Wrapper *handler,
                        JSCompartment *origin, JSCompartment *target)
{
    if (AccessCheck::isChrome(target) || xpc::IsUniversalXPConnectEnabled(target)) {
        
        MOZ_ASSERT(handler->isSafeToUnwrap());
    } else if (WrapperFactory::IsComponentsObject(obj)) {
        
        MOZ_ASSERT(!handler->isSafeToUnwrap());
    } else if (AccessCheck::needsSystemOnlyWrapper(obj)) {
        
    } else if (handler == &FilteringWrapper<CrossCompartmentSecurityWrapper, GentlyOpaque>::singleton) {
        
        
        
    } else {
        
        MOZ_ASSERT(handler->isSafeToUnwrap() == AccessCheck::subsumes(target, origin));
    }
}
#else
#define DEBUG_CheckUnwrapSafety(obj, handler, origin, target) {}
#endif

static Wrapper *
SelectWrapper(bool securityWrapper, bool wantXrays, XrayType xrayType,
              bool waiveXrays)
{
    
    
    if (waiveXrays) {
        MOZ_ASSERT(!securityWrapper);
        return &WaiveXrayWrapper::singleton;
    }

    
    
    if (!wantXrays || xrayType == NotXray) {
        if (!securityWrapper)
            return &CrossCompartmentWrapper::singleton;
        return &FilteringWrapper<CrossCompartmentSecurityWrapper, Opaque>::singleton;
    }

    
    
    if (!securityWrapper) {
        if (xrayType == XrayForWrappedNative)
            return &PermissiveXrayXPCWN::singleton;
        return &PermissiveXrayDOM::singleton;
    }

    
    if (xrayType == XrayForWrappedNative)
        return &FilteringWrapper<SecurityXrayXPCWN,
                                 CrossOriginAccessiblePropertiesOnly>::singleton;
    return &FilteringWrapper<SecurityXrayDOM,
                             CrossOriginAccessiblePropertiesOnly>::singleton;
}

JSObject *
WrapperFactory::Rewrap(JSContext *cx, HandleObject existing, HandleObject obj,
                       HandleObject wrappedProto, HandleObject parent,
                       unsigned flags)
{
    MOZ_ASSERT(!IsWrapper(obj) ||
               GetProxyHandler(obj) == &XrayWaiver ||
               js::GetObjectClass(obj)->ext.innerObject,
               "wrapped object passed to rewrap");
    MOZ_ASSERT(JS_GetClass(obj) != &XrayUtils::HolderClass, "trying to wrap a holder");
    MOZ_ASSERT(!js::IsInnerObject(obj));
    
    
    MOZ_ASSERT(XPCJSRuntime::Get()->GetJSContextStack()->Peek() == cx);

    
    JSCompartment *origin = js::GetObjectCompartment(obj);
    JSCompartment *target = js::GetContextCompartment(cx);
    bool originIsChrome = AccessCheck::isChrome(origin);
    bool targetIsChrome = AccessCheck::isChrome(target);
    bool originSubsumesTarget = AccessCheck::subsumes(origin, target);
    bool targetSubsumesOrigin = AccessCheck::subsumes(target, origin);
    bool sameOrigin = targetSubsumesOrigin && originSubsumesTarget;
    XrayType xrayType = GetXrayType(obj);
    bool waiveXrayFlag = flags & WAIVE_XRAY_WRAPPER_FLAG;

    
    
    RootedObject proxyProto(cx, wrappedProto);

    Wrapper *wrapper;
    CompartmentPrivate *targetdata = EnsureCompartmentPrivate(target);

    
    
    

    
    
    if (xpc::IsUniversalXPConnectEnabled(target)) {
        wrapper = &CrossCompartmentWrapper::singleton;

    
    
    } else if (originIsChrome && !targetIsChrome && xrayType == NotXray) {
        wrapper = &ChromeObjectWrapper::singleton;

    
    
    
    } else if (IsComponentsObject(obj) && !AccessCheck::isChrome(target)) {
        wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper,
                                    ComponentsObjectPolicy>::singleton;
    } else if (AccessCheck::needsSystemOnlyWrapper(obj) &&
               xpc::AllowXBLScope(target) &&
               !(targetIsChrome || (targetSubsumesOrigin && nsContentUtils::IsCallerXBL())))
    {
        wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper, Opaque>::singleton;
    }

    
    
    
    
    
    
    
    else if (targetSubsumesOrigin && !originSubsumesTarget &&
             !waiveXrayFlag && xrayType == NotXray &&
             IsXBLScope(target))
    {
        wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper, GentlyOpaque>::singleton;
    }

    
    
    
    
    
    
    else {

        
        
        bool securityWrapper = !targetSubsumesOrigin;

        
        
        
        
        
        
        bool wantXrays = !(sameOrigin && !targetdata->wantXrays);

        
        
        bool waiveXrays = wantXrays && !securityWrapper && waiveXrayFlag;

        wrapper = SelectWrapper(securityWrapper, wantXrays, xrayType, waiveXrays);
    }

    if (wrapper == &ChromeObjectWrapper::singleton) {
        
        
        JSFunction *fun = JS_GetObjectFunction(obj);
        if (fun) {
            if (JS_IsBuiltinEvalFunction(fun) || JS_IsBuiltinFunctionConstructor(fun)) {
                JS_ReportError(cx, "Not allowed to access chrome eval or Function from content");
                return nullptr;
            }
        }
    }

    DEBUG_CheckUnwrapSafety(obj, wrapper, origin, target);

    if (existing && proxyProto == wrappedProto)
        return Wrapper::Renew(cx, existing, obj, wrapper);

    return Wrapper::New(cx, obj, proxyProto, parent, wrapper);
}

JSObject *
WrapperFactory::WrapForSameCompartment(JSContext *cx, HandleObject objArg)
{
    RootedObject obj(cx, objArg);
    MOZ_ASSERT(js::IsObjectInContextCompartment(obj, cx));

    
    
    

    
    
    obj = JS_ObjectToOuterObject(cx, obj);
    NS_ENSURE_TRUE(obj, nullptr);

    if (dom::GetSameCompartmentWrapperForDOMBinding(*obj.address())) {
        return obj;
    }

    MOZ_ASSERT(!dom::IsDOMObject(obj));

    if (!IS_WN_REFLECTOR(obj))
        return obj;

    
    XPCWrappedNative *wn = XPCWrappedNative::Get(obj);
    MOZ_ASSERT(wn, "Trying to wrap a dead WN!");

    
    RootedObject wrapper(cx, wn->GetSameCompartmentSecurityWrapper(cx));
    MOZ_ASSERT_IF(wrapper != obj && IsComponentsObject(js::UncheckedUnwrap(obj)),
                  !Wrapper::wrapperHandler(wrapper)->isSafeToUnwrap());
    return wrapper;
}





bool
WrapperFactory::WaiveXrayAndWrap(JSContext *cx, jsval *vp)
{
    if (JSVAL_IS_PRIMITIVE(*vp))
        return JS_WrapValue(cx, vp);

    JSObject *obj = js::UncheckedUnwrap(JSVAL_TO_OBJECT(*vp));
    MOZ_ASSERT(!js::IsInnerObject(obj));
    if (js::IsObjectInContextCompartment(obj, cx)) {
        *vp = OBJECT_TO_JSVAL(obj);
        return true;
    }

    
    
    
    
    
    
    
    
    JSCompartment *target = js::GetContextCompartment(cx);
    JSCompartment *origin = js::GetObjectCompartment(obj);
    obj = AccessCheck::subsumes(target, origin) ? WaiveXray(cx, obj) : obj;
    if (!obj)
        return false;

    *vp = OBJECT_TO_JSVAL(obj);
    return JS_WrapValue(cx, vp);
}

JSObject *
WrapperFactory::WrapSOWObject(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    RootedObject proto(cx);

    
    
    
    MOZ_ASSERT(xpc::AllowXBLScope(js::GetContextCompartment(cx)));
    if (!JS_GetPrototype(cx, obj, &proto))
        return nullptr;
    JSObject *wrapperObj =
        Wrapper::New(cx, obj, proto, JS_GetGlobalForObject(cx, obj),
                     &FilteringWrapper<SameCompartmentSecurityWrapper,
                     Opaque>::singleton);
    return wrapperObj;
}

bool
WrapperFactory::IsComponentsObject(JSObject *obj)
{
    const char *name = js::GetObjectClass(obj)->name;
    return name[0] == 'n' && !strcmp(name, "nsXPCComponents");
}

JSObject *
WrapperFactory::WrapComponentsObject(JSContext *cx, HandleObject obj)
{
    RootedObject proto(cx);
    if (!JS_GetPrototype(cx, obj, &proto))
        return nullptr;
    JSObject *wrapperObj =
        Wrapper::New(cx, obj, proto, JS_GetGlobalForObject(cx, obj),
                     &FilteringWrapper<SameCompartmentSecurityWrapper, ComponentsObjectPolicy>::singleton);

    return wrapperObj;
}

bool
WrapperFactory::XrayWrapperNotShadowing(JSObject *wrapper, jsid id)
{
    ResolvingId *rid = ResolvingId::getResolvingIdFromWrapper(wrapper);
    return rid->isXrayShadowing(id);
}






static bool
FixWaiverAfterTransplant(JSContext *cx, HandleObject oldWaiver, HandleObject newobj)
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
TransplantObject(JSContext *cx, JS::HandleObject origobj, JS::HandleObject target)
{
    RootedObject oldWaiver(cx, WrapperFactory::GetXrayWaiver(origobj));
    RootedObject newIdentity(cx, JS_TransplantObject(cx, origobj, target));
    if (!newIdentity || !oldWaiver)
       return newIdentity;

    if (!FixWaiverAfterTransplant(cx, oldWaiver, newIdentity))
        return nullptr;
    return newIdentity;
}

JSObject *
TransplantObjectWithWrapper(JSContext *cx,
                            HandleObject origobj, HandleObject origwrapper,
                            HandleObject targetobj, HandleObject targetwrapper)
{
    RootedObject oldWaiver(cx, WrapperFactory::GetXrayWaiver(origobj));
    RootedObject newSameCompartmentWrapper(cx,
      js_TransplantObjectWithWrapper(cx, origobj, origwrapper, targetobj,
                                     targetwrapper));
    if (!newSameCompartmentWrapper || !oldWaiver)
        return newSameCompartmentWrapper;

    RootedObject newIdentity(cx, Wrapper::wrappedObject(newSameCompartmentWrapper));
    MOZ_ASSERT(!js::IsWrapper(newIdentity));
    if (!FixWaiverAfterTransplant(cx, oldWaiver, newIdentity))
        return nullptr;
    return newSameCompartmentWrapper;
}

nsIGlobalObject *
GetNativeForGlobal(JSObject *obj)
{
    MOZ_ASSERT(JS_IsGlobalObject(obj));
    if (!EnsureCompartmentPrivate(obj)->scope)
        return nullptr;

    
    MOZ_ASSERT(GetObjectClass(obj)->flags & (JSCLASS_PRIVATE_IS_NSISUPPORTS |
                                             JSCLASS_HAS_PRIVATE));
    nsISupports *native =
        static_cast<nsISupports *>(js::GetObjectPrivate(obj));
    MOZ_ASSERT(native);

    
    
    
    
    if (nsCOMPtr<nsIXPConnectWrappedNative> wn = do_QueryInterface(native)) {
        native = wn->Native();
    }

    nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(native);
    MOZ_ASSERT(global, "Native held by global needs to implement nsIGlobalObject!");

    return global;
}

}
