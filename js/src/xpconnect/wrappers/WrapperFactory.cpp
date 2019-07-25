






































#include "jsobj.h"

#include "WrapperFactory.h"
#include "CrossOriginWrapper.h"
#include "FilteringWrapper.h"
#include "XrayWrapper.h"
#include "AccessCheck.h"

#include "xpcprivate.h"

namespace xpc {








JSWrapper WaiveXrayWrapperWrapper(WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG);





JSCrossCompartmentWrapper XrayWrapperWaivedWrapper(WrapperFactory::WAIVE_XRAY_WRAPPER_FLAG);

JSObject *
WrapperFactory::Rewrap(JSContext *cx, JSObject *obj, JSObject *wrappedProto, JSObject *parent,
                       uintN flags)
{
    NS_ASSERTION(!obj->isWrapper() || obj->getClass()->ext.innerObject,
                 "wrapped object passed to rewrap");

    JSCompartment *origin = obj->getCompartment(cx);
    JSCompartment *target = cx->compartment;

    JSWrapper *wrapper;
    if (AccessCheck::isChrome(target)) {
        NS_ASSERTION(!AccessCheck::isChrome(origin), "we shouldn't rewrap from chrome into chrome");

        
        
        if (flags & WAIVE_XRAY_WRAPPER_FLAG) {
            wrapper = &XrayWrapperWaivedWrapper;
        } else {
            
            if (!obj->getGlobal()->isSystem() &&
                (IS_WN_WRAPPER(obj) || obj->getClass()->ext.innerObject)) {
                typedef XrayWrapper<JSCrossCompartmentWrapper> Xray;

                wrapper = &Xray::singleton;
                obj = Xray::createHolder(cx, parent, obj);
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
        
        wrapper = &JSCrossCompartmentWrapper::singleton;
    } else {
        
        
        
        
        if (!IS_WN_WRAPPER(obj)) {
            wrapper = &FilteringWrapper<JSCrossCompartmentWrapper,
                                        CrossOriginAccessiblePropertiesOnly>::singleton;
        } else {
            typedef XrayWrapper<CrossOriginWrapper> Xray;
            wrapper = &FilteringWrapper<XrayWrapper<CrossOriginWrapper>,
                                        CrossOriginAccessiblePropertiesOnly>::singleton;
            obj = Xray::createHolder(cx, parent, obj);
        }
    }
    return JSWrapper::New(cx, obj, wrappedProto, NULL, wrapper);
}

}
