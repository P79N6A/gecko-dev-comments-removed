






































#ifndef dombindings_h
#define dombindings_h

#include "jsapi.h"
#include "jsproxy.h"

#include "nsINodeList.h"

namespace xpc {
namespace dom {

class NodeList : public js::ProxyHandler {
    static NodeList instance;

    static JSObject *getPrototype(JSContext *cx);

    static nsINodeList *getNodeList(JSObject *obj);
    static uint32 getProtoShape(JSObject *obj);
    static void setProtoShape(JSObject *obj, uint32 shape);

    static JSBool length_getter(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

  public:
    NodeList();

    bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                               js::PropertyDescriptor *desc);
    bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                  js::PropertyDescriptor *desc);
    bool defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                        js::PropertyDescriptor *desc);
    bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, js::AutoIdVector &props);
    bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    bool enumerate(JSContext *cx, JSObject *proxy, js::AutoIdVector &props);
    bool fix(JSContext *cx, JSObject *proxy, js::Value *vp);

    bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, js::Value *vp);
    bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, bool strict,
             js::Value *vp);
    bool keys(JSContext *cx, JSObject *proxy, js::AutoIdVector &props);
    bool iterate(JSContext *cx, JSObject *proxy, uintN flags, js::Value *vp);

    
    bool hasInstance(JSContext *cx, JSObject *proxy, const js::Value *vp, bool *bp);
    JSString *obj_toString(JSContext *cx, JSObject *proxy);
    void finalize(JSContext *cx, JSObject *proxy);

    static JSObject *create(JSContext *cx, nsINodeList *);

    static bool objIsNodeList(JSObject *obj) {
        return js::IsProxy(obj) && js::GetProxyHandler(obj) == &instance;
    }
};

}
}

#endif 
