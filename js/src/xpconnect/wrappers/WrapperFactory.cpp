






































#include "jsobj.h"
#include "jsvalue.h"

#include "WrapperFactory.h"
#include "CrossOriginWrapper.h"
#include "FilteringWrapper.h"
#include "XrayWrapper.h"
#include "AccessCheck.h"
#include "XPCWrapper.h"

#include "xpcprivate.h"

namespace xpc {








JSWrapper WaiveXrayWrapperWrapper(WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG);





CrossOriginWrapper CrossOriginWrapper::singleton(0);

static JSObject *
DoubleWrap(JSContext *cx, JSObject *obj, uintN flags)
{
    if (flags & WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG) {
        js::SwitchToCompartment sc(cx, obj->compartment());
        return JSWrapper::New(cx, obj, NULL, obj->getGlobal(),
                              &WaiveXrayWrapperWrapper);
    }
    return obj;
}

static JSObject *
GetCurrentOuter(JSContext *cx, JSObject *obj)
{
    OBJ_TO_OUTER_OBJECT(cx, obj);
    if (obj->isWrapper() && !obj->getClass()->ext.innerObject) {
        obj = obj->unwrap();
        NS_ASSERTION(obj->getClass()->ext.innerObject,
                     "weird object, expecting an outer window proxy");
    }

    return obj;
}

JSObject *
WrapperFactory::PrepareForWrapping(JSContext *cx, JSObject *scope, JSObject *obj, uintN flags)
{
    
    if (obj->getClass()->ext.innerObject)
        return DoubleWrap(cx, obj, flags);

    
    
    JS_ASSERT(!obj->isWrapper());

    
    
    if (IS_SLIM_WRAPPER(obj) && !MorphSlimWrapper(cx, obj))
        return nsnull;

    
    obj = GetCurrentOuter(cx, obj);
    if (obj->getClass()->ext.innerObject)
        return DoubleWrap(cx, obj, flags);

    
    
    
    
    
    if (!IS_WN_WRAPPER(obj) || !obj->getParent())
        return DoubleWrap(cx, obj, flags);

    XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));

    
    
    if (!wn->GetClassInfo())
        return DoubleWrap(cx, obj, flags);

    XPCCallContext ccx(JS_CALLER, cx, obj);
    if (NATIVE_HAS_FLAG(&ccx, WantPreCreate)) {
        
        
        JSObject *originalScope = scope;
        nsresult rv = wn->GetScriptableInfo()->GetCallback()->
            PreCreate(wn->Native(), cx, scope, &scope);
        NS_ENSURE_SUCCESS(rv, DoubleWrap(cx, obj, flags));

        
        
        
        
        if (originalScope->getCompartment() != scope->getCompartment())
            return DoubleWrap(cx, obj, flags);

        
        
        
        
    }

    
    
    
    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, scope))
        return nsnull;

    
    
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
        if (newwn->GetSet()->GetInterfaceCount() == 1) {
            
            
            
            
            

#ifdef DEBUG
            {
                XPCNativeInterface *iface = newwn->GetSet()->GetInterfaceAt(0);
                JSString *name = JSID_TO_STRING(iface->GetName());
                NS_ASSERTION(!strcmp("nsISupports", JS_GetStringBytes(name)), "weird interface");
            }
#endif

            newwn->SetSet(wn->GetSet());
        }

    }

    return DoubleWrap(cx, obj, flags);
}

JSObject *
WrapperFactory::Rewrap(JSContext *cx, JSObject *obj, JSObject *wrappedProto, JSObject *parent,
                       uintN flags)
{
    NS_ASSERTION(!obj->isWrapper() ||
                 (obj->isWrapper() &&
                  obj->getProxyHandler() == &WaiveXrayWrapperWrapper) ||
                 obj->getClass()->ext.innerObject,
                 "wrapped object passed to rewrap");
    NS_ASSERTION(JS_GET_CLASS(cx, obj) != &XrayUtils::HolderClass, "trying to wrap a holder");

    JSCompartment *origin = obj->getCompartment();
    JSCompartment *target = cx->compartment;
    JSObject *xrayHolder = nsnull;

    JSWrapper *wrapper;
    CompartmentPrivate *targetdata = static_cast<CompartmentPrivate *>(target->data);
    if (AccessCheck::isChrome(target)) {
        if (AccessCheck::isChrome(origin)) {
            wrapper = &JSCrossCompartmentWrapper::singleton;
        } else if (flags & WAIVE_XRAY_WRAPPER_FLAG) {
            
            
            wrapper = &CrossOriginWrapper::singleton;
        } else {
            
            if (!obj->getGlobal()->isSystem() &&
                (IS_WN_WRAPPER(obj) || obj->getClass()->ext.innerObject)) {
                typedef XrayWrapper<JSCrossCompartmentWrapper, CrossCompartmentXray> Xray;
                wrapper = &Xray::singleton;
                xrayHolder = Xray::createHolder(cx, obj, parent);
                if (!xrayHolder)
                    return nsnull;
            } else {
                wrapper = &JSCrossCompartmentWrapper::singleton;
            }
        }
    } else if (AccessCheck::isChrome(origin)) {
        wrapper = &FilteringWrapper<JSCrossCompartmentWrapper,
                                    ExposedPropertiesOnly>::singleton;
    } else if (AccessCheck::isSameOrigin(origin, target)) {
        
        
        if (AccessCheck::needsSystemOnlyWrapper(obj)) {
            wrapper = &FilteringWrapper<JSCrossCompartmentWrapper,
                                        OnlyIfSubjectIsSystem>::singleton;
        } else if (targetdata && targetdata->wantXrays && IS_WN_WRAPPER(obj)) {
            typedef XrayWrapper<JSCrossCompartmentWrapper, CrossCompartmentXray> Xray;
            wrapper = &Xray::singleton;
            xrayHolder = Xray::createHolder(cx, obj, parent);
            if (!xrayHolder)
                return nsnull;
        } else {
            wrapper = &JSCrossCompartmentWrapper::singleton;
        }
    } else {
        NS_ASSERTION(!AccessCheck::needsSystemOnlyWrapper(obj),
                     "bad object exposed across origins");

        
        
        
        
        if (!IS_WN_WRAPPER(obj) && !obj->getClass()->ext.innerObject) {
            wrapper = &FilteringWrapper<JSCrossCompartmentWrapper,
                                        CrossOriginAccessiblePropertiesOnly>::singleton;
        } else {
            typedef XrayWrapper<JSCrossCompartmentWrapper, CrossCompartmentXray> Xray;

            
            
            if (IsLocationObject(obj)) {
                wrapper = &FilteringWrapper<Xray,
                    SameOriginOrCrossOriginAccessiblePropertiesOnly>::singleton;
            } else {
                wrapper= &FilteringWrapper<Xray,
                    CrossOriginAccessiblePropertiesOnly>::singleton;
            }

            xrayHolder = Xray::createHolder(cx, obj, parent);
            if (!xrayHolder)
                return nsnull;
        }
    }

    JSObject *wrapperObj = JSWrapper::New(cx, obj, wrappedProto, parent, wrapper);
    if (!wrapperObj || !xrayHolder)
        return wrapperObj;

    
    
    wrapperObj->setProxyExtra(js::ObjectValue(*xrayHolder));
    return wrapperObj;
}

typedef FilteringWrapper<XrayWrapper<JSWrapper, SameCompartmentXray>,
                         SameOriginOrCrossOriginAccessiblePropertiesOnly> LW;

bool
WrapperFactory::IsLocationObject(JSObject *obj)
{
    const char *name = obj->getClass()->name;
    return name[0] == 'L' && !strcmp(name, "Location");
}

JSObject *
WrapperFactory::WrapLocationObject(JSContext *cx, JSObject *obj)
{
    JSObject *xrayHolder = LW::createHolder(cx, obj, obj->getParent());
    if (!xrayHolder)
        return NULL;
    JSObject *wrapperObj = JSWrapper::New(cx, obj, obj->getProto(), NULL, &LW::singleton);
    if (!wrapperObj)
        return NULL;
    wrapperObj->setProxyExtra(js::ObjectValue(*xrayHolder));
    return wrapperObj;
}

bool
WrapperFactory::WaiveXrayAndWrap(JSContext *cx, jsval *vp)
{
    if (JSVAL_IS_PRIMITIVE(*vp))
        return JS_WrapValue(cx, vp);

    JSObject *obj = JSVAL_TO_OBJECT(*vp)->unwrap();

    
    
    obj = GetCurrentOuter(cx, obj);

    {
        js::SwitchToCompartment sc(cx, obj->compartment());
        obj = JSWrapper::New(cx, obj, NULL, obj->getGlobal(), &WaiveXrayWrapperWrapper);
        if (!obj)
            return false;
    }

    *vp = OBJECT_TO_JSVAL(obj);
    return JS_WrapValue(cx, vp);
}

JSObject *
WrapperFactory::WrapSOWObject(JSContext *cx, JSObject *obj)
{
    JSObject *wrapperObj =
        JSWrapper::New(cx, obj, obj->getProto(), NULL,
                       &FilteringWrapper<JSWrapper,
                                         OnlyIfSubjectIsSystem>::singleton);
    return wrapperObj;
}

}
