






































#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class WrapperFactory {
  public:
    enum { WAIVE_XRAY_WRAPPER_FLAG = js::Wrapper::LAST_USED_FLAG << 1,
           IS_XRAY_WRAPPER_FLAG    = WAIVE_XRAY_WRAPPER_FLAG << 1,
           SCRIPT_ACCESS_ONLY_FLAG = IS_XRAY_WRAPPER_FLAG << 1,
           PARTIALLY_TRANSPARENT   = SCRIPT_ACCESS_ONLY_FLAG << 1,
           SOW_FLAG                = PARTIALLY_TRANSPARENT << 1 };

    
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

    static JSObject *WaiveXray(JSContext *cx, JSObject *obj);

    static JSObject *DoubleWrap(JSContext *cx, JSObject *obj, uintN flags);

    
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

extern js::Wrapper WaiveXrayWrapperWrapper;

}
