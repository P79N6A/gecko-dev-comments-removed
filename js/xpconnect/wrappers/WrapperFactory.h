






#ifndef _xpc_WRAPPERFACTORY_H
#define _xpc_WRAPPERFACTORY_H

#include "jswrapper.h"

namespace xpc {

class WrapperFactory {
  public:
    enum { WAIVE_XRAY_WRAPPER_FLAG = js::Wrapper::LAST_USED_FLAG << 1,
           IS_XRAY_WRAPPER_FLAG    = WAIVE_XRAY_WRAPPER_FLAG << 1 };

    
    static bool HasWrapperFlag(JSObject *wrapper, unsigned flag) {
        unsigned flags = 0;
        js::UncheckedUnwrap(wrapper, true, &flags);
        return !!(flags & flag);
    }

    static bool IsXrayWrapper(JSObject *wrapper) {
        return HasWrapperFlag(wrapper, IS_XRAY_WRAPPER_FLAG);
    }

    static bool HasWaiveXrayFlag(JSObject *wrapper) {
        return HasWrapperFlag(wrapper, WAIVE_XRAY_WRAPPER_FLAG);
    }

    static bool IsSecurityWrapper(JSObject *obj) {
        return !js::CheckedUnwrap(obj);
    }

    static bool IsCOW(JSObject *wrapper);

    static JSObject *GetXrayWaiver(JS::HandleObject obj);
    static JSObject *CreateXrayWaiver(JSContext *cx, JS::HandleObject obj);
    static JSObject *WaiveXray(JSContext *cx, JSObject *obj);

    static JSObject *DoubleWrap(JSContext *cx, JS::HandleObject obj, unsigned flags);

    
    static JSObject *PrepareForWrapping(JSContext *cx,
                                        JS::HandleObject scope,
                                        JS::HandleObject obj,
                                        unsigned flags);

    
    static JSObject *Rewrap(JSContext *cx,
                            JS::HandleObject existing,
                            JS::HandleObject obj,
                            JS::HandleObject wrappedProto,
                            JS::HandleObject parent,
                            unsigned flags);

    
    static JSObject *WrapForSameCompartment(JSContext *cx,
                                            JS::HandleObject obj);

    
    static bool WaiveXrayAndWrap(JSContext *cx, JS::MutableHandleValue vp);

    
    static JSObject *WrapSOWObject(JSContext *cx, JSObject *obj);

    
    static bool IsComponentsObject(JSObject *obj);

    
    static JSObject *WrapComponentsObject(JSContext *cx, JS::HandleObject obj);

    
    static bool XrayWrapperNotShadowing(JSObject *wrapper, jsid id);
};

extern js::Wrapper XrayWaiver;

}

#endif
