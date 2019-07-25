








































#ifndef jswrapper_h___
#define jswrapper_h___

#include "jsapi.h"
#include "jsproxy.h"

namespace js {


class JSWrapper : public JSProxyHandler {
    JSObject *mWrappedObject;

    JS_FRIEND_API(JSWrapper(JSObject *));

  public:
    JS_FRIEND_API(virtual ~JSWrapper());

    
    virtual JS_FRIEND_API(bool) getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                                      PropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                                         PropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                                               PropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) getOwnPropertyNames(JSContext *cx, JSObject *proxy,
                                                    JSIdArray **idap);
    virtual JS_FRIEND_API(bool) delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) enumerate(JSContext *cx, JSObject *proxy, JSIdArray **idap);
    virtual JS_FRIEND_API(bool) fix(JSContext *cx, JSObject *proxy, Value *vp);

    
    virtual JS_FRIEND_API(bool) has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id,
                                    Value *vp);
    virtual JS_FRIEND_API(bool) set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id,
                                    Value *vp);
    virtual JS_FRIEND_API(bool) enumerateOwn(JSContext *cx, JSObject *proxy, JSIdArray **idap);
    virtual JS_FRIEND_API(bool) iterate(JSContext *cx, JSObject *proxy, uintN flags, Value *vp);

    
    virtual JS_FRIEND_API(void) finalize(JSContext *cx, JSObject *proxy);
    virtual JS_FRIEND_API(void) trace(JSTracer *trc, JSObject *proxy);
    virtual JS_FRIEND_API(const void *) family();

    static JS_FRIEND_API(JSWrapper) singleton;

    static JS_FRIEND_API(JSObject *) wrap(JSContext *cx, JSObject *obj,
                                          JSObject *proto, JSObject *parent,
                                          JSString *className);

    inline JSObject *wrappedObject(JSObject *proxy) {
        return mWrappedObject ? mWrappedObject : proxy->getProxyPrivate().asObjectOrNull();
    }
};

}

#endif
