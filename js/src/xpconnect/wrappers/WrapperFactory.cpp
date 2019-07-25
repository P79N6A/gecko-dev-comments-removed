






































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





CrossOriginWrapper XrayWrapperWaivedWrapper(WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG);

JSObject *
WrapperFactory::PrepareForWrapping(JSContext *cx, JSObject *scope, JSObject *obj, uintN flags)
{
    
    
    JS_ASSERT(!obj->isWrapper() || obj->getClass()->ext.innerObject);

    
    
    if (IS_SLIM_WRAPPER(obj) && !MorphSlimWrapper(cx, obj))
        return nsnull;

    
    OBJ_TO_OUTER_OBJECT(cx, obj);

    
    
    
    
    if (!IS_WN_WRAPPER(obj))
        return obj;

    XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(xpc_GetJSPrivate(obj));

    
    if (wn->GetProto()->ClassIsDOMObject())
        return obj;

    XPCCallContext ccx(JS_CALLER, cx, obj);
    if (NATIVE_HAS_FLAG(&ccx, WantPreCreate)) {
        
        
        JSObject *originalScope = scope;
        nsresult rv = wn->GetScriptableInfo()->GetCallback()->
            PreCreate(wn->Native(), cx, scope, &scope);
        NS_ENSURE_SUCCESS(rv, obj);

        
        
        
        
        if (originalScope->getCompartment() != scope->getCompartment())
            return obj;

        
        
        
        
    }

    
    
    
    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, scope))
        return obj;
    jsval v;
    nsresult rv =
        nsXPConnect::FastGetXPConnect()->WrapNativeToJSVal(cx, scope, wn->Native(), nsnull,
                                                           &NS_GET_IID(nsISupports), PR_FALSE,
                                                           &v, nsnull);
    NS_ENSURE_SUCCESS(rv, obj);
    return JSVAL_TO_OBJECT(v);
}

JSObject *
WrapperFactory::Rewrap(JSContext *cx, JSObject *obj, JSObject *wrappedProto, JSObject *parent,
                       uintN flags)
{
    NS_ASSERTION(!obj->isWrapper() || obj->getClass()->ext.innerObject,
                 "wrapped object passed to rewrap");
    NS_ASSERTION(JS_GET_CLASS(cx, obj) != &XrayUtils::HolderClass, "trying to wrap a holder");

    JSCompartment *origin = obj->getCompartment();
    JSCompartment *target = cx->compartment;
    JSObject *xrayHolder = nsnull;

    JSWrapper *wrapper;
    if (AccessCheck::isChrome(target)) {
        if (AccessCheck::isChrome(origin)) {
            
            
            if (static_cast<xpc::CompartmentPrivate*>(target->data)->preferXrays &&
                IS_WN_WRAPPER(obj)) {
                typedef XrayWrapper<JSCrossCompartmentWrapper, CrossCompartmentXray> Xray;
                wrapper = &Xray::singleton;
                xrayHolder = Xray::createHolder(cx, obj, parent);
                if (!xrayHolder)
                    return nsnull;
            } else {
                wrapper = &JSCrossCompartmentWrapper::singleton;
            }
        } else if (flags & WAIVE_XRAY_WRAPPER_FLAG) {
            
            
            wrapper = &XrayWrapperWaivedWrapper;
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
        
        
        
        
        if (AccessCheck::needsSystemOnlyWrapper(obj)) {
            wrapper = &FilteringWrapper<JSCrossCompartmentWrapper,
                                        OnlyIfSubjectIsSystem>::singleton;
        } else {
            wrapper = &FilteringWrapper<JSCrossCompartmentWrapper,
                                        ExposedPropertiesOnly>::singleton;
        }
    } else if (AccessCheck::isSameOrigin(origin, target)) {
        
        
        if (static_cast<xpc::CompartmentPrivate*>(target->data)->preferXrays &&
            IS_WN_WRAPPER(obj)) {
            typedef XrayWrapper<JSCrossCompartmentWrapper, CrossCompartmentXray> Xray;
            wrapper = &Xray::singleton;
            xrayHolder = Xray::createHolder(cx, obj, parent);
            if (!xrayHolder)
                return nsnull;
        } else {
            wrapper = &JSCrossCompartmentWrapper::singleton;
        }
    } else {
        
        
        
        
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

}
