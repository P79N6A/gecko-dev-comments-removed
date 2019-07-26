






#ifndef _xpc_WRAPPERFACTORY_H
#define _xpc_WRAPPERFACTORY_H

#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class WrapperFactory {
  public:
    enum { WAIVE_XRAY_WRAPPER_FLAG = js::Wrapper::LAST_USED_FLAG << 1,
           IS_XRAY_WRAPPER_FLAG    = WAIVE_XRAY_WRAPPER_FLAG << 1,
           SCRIPT_ACCESS_ONLY_FLAG = IS_XRAY_WRAPPER_FLAG << 1,
           SOW_FLAG                = SCRIPT_ACCESS_ONLY_FLAG << 1 };

    
    static bool HasWrapperFlag(JSObject *wrapper, unsigned flag) {
        unsigned flags = 0;
        js::UnwrapObject(wrapper, true, &flags);
        return !!(flags & flag);
    }

    static bool IsXrayWrapper(JSObject *wrapper) {
        return HasWrapperFlag(wrapper, IS_XRAY_WRAPPER_FLAG);
    }

    static bool HasWaiveXrayFlag(JSObject *wrapper) {
        return HasWrapperFlag(wrapper, WAIVE_XRAY_WRAPPER_FLAG);
    }

    static bool IsSecurityWrapper(JSObject *obj) {
        return !js::UnwrapObjectChecked(obj);
    }

    static JSObject *GetXrayWaiver(JSObject *obj);
    static JSObject *CreateXrayWaiver(JSContext *cx, JSObject *obj);
    static JSObject *WaiveXray(JSContext *cx, JSObject *obj);

    static JSObject *DoubleWrap(JSContext *cx, JSObject *obj, unsigned flags);

    
    static JSObject *PrepareForWrapping(JSContext *cx,
                                        JSObject *scope,
                                        JSObject *obj,
                                        unsigned flags);

    
    static JSObject *Rewrap(JSContext *cx,
                            JSObject *existing,
                            JSObject *obj,
                            JSObject *wrappedProto,
                            JSObject *parent,
                            unsigned flags);

    
    static JSObject *WrapForSameCompartment(JSContext *cx,
                                            JSObject *obj);

    
    static bool WaiveXrayAndWrap(JSContext *cx, jsval *vp);

    
    static JSObject *WrapSOWObject(JSContext *cx, JSObject *obj);

    
    static bool IsComponentsObject(JSObject *obj);

    
    static JSObject *WrapComponentsObject(JSContext *cx, JSObject *obj);

    
    static JSObject *WrapForSameCompartmentXray(JSContext *cx, JSObject *obj);

    
    static bool XrayWrapperNotShadowing(JSObject *wrapper, jsid id);
};

extern js::Wrapper XrayWaiver;

}

#endif
