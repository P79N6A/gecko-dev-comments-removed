






































#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class WrapperFactory {
  public:
    enum { WAIVE_XRAY_WRAPPER_FLAG = (1<<0) };

    
    bool HasWrapperFlag(JSObject *wrapper, uintN flag) {
        uintN flags = 0;
        wrapper->unwrap(&flags);
        return !!(flags & flag);
    }

    
    static JSObject *Rewrap(JSContext *cx,
                            JSObject *obj,
                            JSObject *wrappedProto,
                            JSObject *parent,
                            uintN flags);
};

}
