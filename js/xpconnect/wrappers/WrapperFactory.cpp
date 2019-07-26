





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
WrapperFactory::GetXrayWaiver(HandleObject obj)
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

    JSAutoCompartment ac(cx, obj);
    JSObject *waiver = Wrapper::New(cx, obj,
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




static bool
ForceCOWBehavior(JSObject *obj)
{
    if (IdentifyStandardInstanceOrPrototype(obj) == JSProto_Object) {
        MOZ_ASSERT(GetXrayType(obj) == XrayForJSObject,
                   "We should use XrayWrappers for standard ES Object instances "
                   "modulo this hack");
        return true;
    }
    return false;
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

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool subsumes = AccessCheck::subsumes(js::GetContextCompartment(cx),
                                          js::GetObjectCompartment(obj));
    XrayType xrayType = GetXrayType(obj);
    if (!subsumes && (xrayType == NotXray || ForceCOWBehavior(obj))) {
        JSProtoKey key = JSProto_Null;
        {
            JSAutoCompartment ac(cx, obj);
            key = IdentifyStandardPrototype(obj);
        }
        if (key != JSProto_Null) {
            RootedObject homeProto(cx);
            if (!JS_GetClassPrototype(cx, key, &homeProto))
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
                 AccessCheck::subsumes(js::GetObjectCompartment(wrapScope),
                                       js::GetObjectCompartment(obj)))
            {
                return DoubleWrap(cx, obj, flags);
            }
        }
    }

    
    
    RootedValue v(cx);
    nsresult rv =
        nsXPConnect::XPConnect()->WrapNativeToJSVal(cx, wrapScope, wn->Native(), nullptr,
                                                    &NS_GET_IID(nsISupports), false, &v);
    NS_ENSURE_SUCCESS(rv, nullptr);

    obj.set(&v.toObject());
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
        
        MOZ_ASSERT(!handler->hasSecurityPolicy());
    } else if (handler == &FilteringWrapper<CrossCompartmentSecurityWrapper, GentlyOpaque>::singleton) {
        
        
        
    } else {
        
        MOZ_ASSERT(handler->hasSecurityPolicy() == !AccessCheck::subsumesConsideringDomain(target, origin));
    }
}
#else
#define DEBUG_CheckUnwrapSafety(obj, handler, origin, target) {}
#endif

static Wrapper *
SelectWrapper(bool securityWrapper, bool wantXrays, XrayType xrayType,
              bool waiveXrays, bool originIsXBLScope)
{
    
    
    if (waiveXrays) {
        MOZ_ASSERT(!securityWrapper);
        return &WaiveXrayWrapper::singleton;
    }

    
    
    if (!wantXrays || xrayType == NotXray) {
        if (!securityWrapper)
            return &CrossCompartmentWrapper::singleton;
        
        
        
        
        
        else if (originIsXBLScope)
            return &FilteringWrapper<CrossCompartmentSecurityWrapper, OpaqueWithCall>::singleton;
        return &FilteringWrapper<CrossCompartmentSecurityWrapper, Opaque>::singleton;
    }

    
    
    if (!securityWrapper) {
        if (xrayType == XrayForWrappedNative)
            return &PermissiveXrayXPCWN::singleton;
        else if (xrayType == XrayForDOMObject)
            return &PermissiveXrayDOM::singleton;
        MOZ_ASSERT(xrayType == XrayForJSObject);
        return &PermissiveXrayJS::singleton;
    }

    
    if (xrayType == XrayForWrappedNative)
        return &FilteringWrapper<SecurityXrayXPCWN,
                                 CrossOriginAccessiblePropertiesOnly>::singleton;
    else if (xrayType == XrayForDOMObject)
        return &FilteringWrapper<SecurityXrayDOM,
                                 CrossOriginAccessiblePropertiesOnly>::singleton;
    
    
    MOZ_ASSERT(xrayType == XrayForJSObject);
    return &FilteringWrapper<CrossCompartmentSecurityWrapper, Opaque>::singleton;
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
    MOZ_ASSERT(!XrayUtils::IsXPCWNHolderClass(JS_GetClass(obj)), "trying to wrap a holder");
    MOZ_ASSERT(!js::IsInnerObject(obj));
    
    
    MOZ_ASSERT(XPCJSRuntime::Get()->GetJSContextStack()->Peek() == cx);

    
    JSCompartment *origin = js::GetObjectCompartment(obj);
    JSCompartment *target = js::GetContextCompartment(cx);
    bool originIsChrome = AccessCheck::isChrome(origin);
    bool targetIsChrome = AccessCheck::isChrome(target);
    bool originSubsumesTarget = AccessCheck::subsumesConsideringDomain(origin, target);
    bool targetSubsumesOrigin = AccessCheck::subsumesConsideringDomain(target, origin);
    bool sameOrigin = targetSubsumesOrigin && originSubsumesTarget;
    XrayType xrayType = GetXrayType(obj);
    bool waiveXrayFlag = flags & WAIVE_XRAY_WRAPPER_FLAG;

    Wrapper *wrapper;
    CompartmentPrivate *targetdata = EnsureCompartmentPrivate(target);

    
    
    

    
    
    if (xpc::IsUniversalXPConnectEnabled(target)) {
        wrapper = &CrossCompartmentWrapper::singleton;
    }

    
    
    
    
    
    else if (originIsChrome && !targetIsChrome &&
             (xrayType == NotXray || ForceCOWBehavior(obj)))
    {
        wrapper = &ChromeObjectWrapper::singleton;
    }

    
    
    
    
    
    
    
    else if (targetSubsumesOrigin && !originSubsumesTarget &&
             !waiveXrayFlag && xrayType == NotXray &&
             IsContentXBLScope(target))
    {
        wrapper = &FilteringWrapper<CrossCompartmentSecurityWrapper, GentlyOpaque>::singleton;
    }

    
    
    
    
    
    
    else {

        
        
        bool securityWrapper = !targetSubsumesOrigin;

        
        
        
        
        
        
        bool wantXrays = !(sameOrigin && !targetdata->wantXrays);

        
        
        bool waiveXrays = wantXrays && !securityWrapper && waiveXrayFlag;

        
        
        bool originIsContentXBLScope = IsContentXBLScope(origin);

        wrapper = SelectWrapper(securityWrapper, wantXrays, xrayType, waiveXrays,
                                originIsContentXBLScope);
    }

    if (!targetSubsumesOrigin) {
        
        
        JSFunction *fun = JS_GetObjectFunction(obj);
        if (fun) {
            if (JS_IsBuiltinEvalFunction(fun) || JS_IsBuiltinFunctionConstructor(fun)) {
                JS_ReportError(cx, "Permission denied to expose eval or Function to non-subsuming content");
                return nullptr;
            }
        }
    }

    DEBUG_CheckUnwrapSafety(obj, wrapper, origin, target);

    if (existing)
        return Wrapper::Renew(cx, existing, obj, wrapper);

    return Wrapper::New(cx, obj, parent, wrapper);
}





bool
WrapperFactory::WaiveXrayAndWrap(JSContext *cx, MutableHandleValue vp)
{
    if (vp.isPrimitive())
        return JS_WrapValue(cx, vp);

    RootedObject obj(cx, &vp.toObject());
    if (!WaiveXrayAndWrap(cx, &obj))
        return false;

    vp.setObject(*obj);
    return true;
}

bool
WrapperFactory::WaiveXrayAndWrap(JSContext *cx, MutableHandleObject argObj)
{
    MOZ_ASSERT(argObj);
    RootedObject obj(cx, js::UncheckedUnwrap(argObj));
    MOZ_ASSERT(!js::IsInnerObject(obj));
    if (js::IsObjectInContextCompartment(obj, cx)) {
        argObj.set(obj);
        return true;
    }

    
    
    
    
    
    
    
    
    JSCompartment *target = js::GetContextCompartment(cx);
    JSCompartment *origin = js::GetObjectCompartment(obj);
    obj = AccessCheck::subsumes(target, origin) ? WaiveXray(cx, obj) : obj;
    if (!obj)
        return false;

    if (!JS_WrapObject(cx, &obj))
        return false;
    argObj.set(obj);
    return true;
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

nsIGlobalObject *
GetNativeForGlobal(JSObject *obj)
{
    MOZ_ASSERT(JS_IsGlobalObject(obj));
    if (!MaybeGetObjectScope(obj))
        return nullptr;

    
    
    MOZ_ASSERT((GetObjectClass(obj)->flags & (JSCLASS_PRIVATE_IS_NSISUPPORTS |
                                             JSCLASS_HAS_PRIVATE)) ||
               dom::UnwrapDOMObjectToISupports(obj));

    nsISupports *native = dom::UnwrapDOMObjectToISupports(obj);
    if (!native) {
        native = static_cast<nsISupports *>(js::GetObjectPrivate(obj));
        MOZ_ASSERT(native);

        
        
        
        
        if (nsCOMPtr<nsIXPConnectWrappedNative> wn = do_QueryInterface(native)) {
            native = wn->Native();
        }
    }

    nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(native);
    MOZ_ASSERT(global, "Native held by global needs to implement nsIGlobalObject!");

    return global;
}

}
