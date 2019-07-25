






































#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class WrapperFactory {
  public:
    enum { WAIVE_XRAY_WRAPPER_FLAG = (1<<0),
           IS_XRAY_WRAPPER_FLAG = (1<<1),
           SCRIPT_ACCESS_ONLY_FLAG = (1<<2),
           PARTIALLY_TRANSPARENT = (1<<3),
           SOW_FLAG = (1<<4) };

    
    static bool HasWrapperFlag(JSObject *wrapper, uintN flag) {
        uintN flags = 0;
        wrapper->unwrap(&flags);
        return !!(flags & flag);
    }

    static bool IsXrayWrapper(JSObject *wrapper) {
        return HasWrapperFlag(wrapper, IS_XRAY_WRAPPER_FLAG);
    }

    static bool IsPartiallyTransparent(JSObject *wrapper) {
        return HasWrapperFlag(wrapper, PARTIALLY_TRANSPARENT);
    }

    static bool HasWaiveXrayFlag(JSObject *wrapper) {
        return HasWrapperFlag(wrapper, WAIVE_XRAY_WRAPPER_FLAG);
    }

    
    static JSObject *PrepareForWrapping(JSContext *cx,
                                        JSObject *scope,
                                        JSObject *obj,
                                        uintN flags);

    
    static JSObject *Rewrap(JSContext *cx,
                            JSObject *obj,
                            JSObject *wrappedProto,
                            JSObject *parent,
                            uintN flags);

    
    static bool IsLocationObject(JSObject *obj);

    
    static JSObject *WrapLocationObject(JSContext *cx, JSObject *obj);

    
    static bool WaiveXrayAndWrap(JSContext *cx, jsval *vp);

    
    static JSObject *WrapSOWObject(JSContext *cx, JSObject *obj);
};

extern JSWrapper WaiveXrayWrapperWrapper;

}
