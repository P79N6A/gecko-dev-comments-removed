






































#include "jsapi.h"
#include "jswrapper.h"




namespace xpc {

extern JSClass HolderClass;

template <typename Base>
class XrayWrapper : public Base {
  public:
    XrayWrapper(uintN flags);
    virtual ~XrayWrapper();

    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                       js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          js::PropertyDescriptor *desc);
    virtual bool has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);

    static XrayWrapper singleton;
};

}
