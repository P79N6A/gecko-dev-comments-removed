






































#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class WrapperFactory {
  public:
    enum { WAIVE_XRAY_WRAPPER_FLAG = (1<<0),
           IS_XRAY_WRAPPER_FLAG = (1<<1) };

    
    static bool HasWrapperFlag(JSObject *wrapper, uintN flag) {
        uintN flags = 0;
        wrapper->unwrap(&flags);
        return !!(flags & flag);
    }

    static bool IsXrayWrapper(JSObject *wrapper) {
        return HasWrapperFlag(wrapper, IS_XRAY_WRAPPER_FLAG);
    }

    
    static JSObject *Rewrap(JSContext *cx,
                            JSObject *obj,
                            JSObject *wrappedProto,
                            JSObject *parent,
                            uintN flags);

    
    static bool IsLocationObject(JSObject *obj);

    
    static JSObject *WrapLocationObject(JSContext *cx, JSObject *obj);
};

}
