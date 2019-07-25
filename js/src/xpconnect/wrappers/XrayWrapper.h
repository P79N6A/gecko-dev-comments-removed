






































#include "jsapi.h"
#include "jswrapper.h"







namespace xpc {

namespace XrayUtils {

extern JSClass HolderClass;

}


template <typename Base, typename Policy>
class XrayWrapper : public Base {
  public:
    XrayWrapper(uintN flags);
    virtual ~XrayWrapper();

    bool resolveWrappedJSObject(JSContext *cx, JSObject *wrapper, jsid id,
                                bool set, js::PropertyDescriptor *desc);

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                       bool set, js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          bool set, js::PropertyDescriptor *desc);
    virtual bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper,
                                     js::AutoIdVector &props);
    virtual bool delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool enumerate(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool fix(JSContext *cx, JSObject *proxy, js::Value *vp);

    
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                     js::Value *vp);
    virtual bool set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                     js::Value *vp);
    virtual bool has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool enumerateOwn(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool iterate(JSContext *cx, JSObject *wrapper, uintN flags, js::Value *vp);

    static JSObject *createHolder(JSContext *cx, JSObject *wrappedNative, JSObject *parent);

    static XrayWrapper singleton;
};

class CrossCompartmentXray {
  public:
    static bool enter(JSContext *cx, JSObject *wrapper, jsid *idp,
                      JSWrapper::Action act, void **priv);
    static void leave(JSContext *cx, JSObject *wrapper, void *priv);
};

class SameCompartmentXray {
  public:
    static bool enter(JSContext *, JSObject *, jsid *, JSWrapper::Action, void **) {
        return true;
    }
    static void leave(JSContext *cx, JSObject *wrapper, void *priv) {
    }
};

}
