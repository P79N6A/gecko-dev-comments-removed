






































#include "jsapi.h"
#include "jswrapper.h"




namespace xpc {

extern JSClass HolderClass;

template <typename Base>
class XrayWrapper : public Base {
  public:
    XrayWrapper(uintN flags);
    virtual ~XrayWrapper();

    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                     js::Value *vp);
    virtual bool set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                     js::Value *vp);
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                       js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          js::PropertyDescriptor *desc);
    virtual bool has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);

    static JSObject *createHolder(JSContext *cx, JSObject *parent, JSObject *wrappedNative);
    static JSObject *unwrapHolder(JSContext *cx, JSObject *holder);

    static XrayWrapper singleton;
};

}
