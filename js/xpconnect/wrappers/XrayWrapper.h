






































#include "jsapi.h"
#include "jswrapper.h"







namespace xpc {

namespace XrayUtils {

extern JSClass HolderClass;

bool
IsTransparent(JSContext *cx, JSObject *wrapper);

}


template <typename Base>
class XrayWrapper : public Base {
  public:
    XrayWrapper(uintN flags);
    virtual ~XrayWrapper();

    
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
                     bool strict, js::Value *vp);
    virtual bool has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool keys(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool iterate(JSContext *cx, JSObject *wrapper, uintN flags, js::Value *vp);

    virtual bool call(JSContext *cx, JSObject *wrapper, uintN argc, js::Value *vp);
    virtual bool construct(JSContext *cx, JSObject *wrapper,
                           uintN argc, js::Value *argv, js::Value *rval);

    static JSObject *createHolder(JSContext *cx, JSObject *wrappedNative, JSObject *parent);

    static XrayWrapper singleton;

  private:
    bool resolveOwnProperty(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                            js::PropertyDescriptor *desc);
};

class XrayProxy : public XrayWrapper<js::CrossCompartmentWrapper> {
  public:
    XrayProxy(uintN flags);
    virtual ~XrayProxy();

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
    

    static XrayProxy singleton;
};

}
