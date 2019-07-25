






































#ifndef dombindings_h
#define dombindings_h

#include "jsapi.h"
#include "jsproxy.h"

class nsINodeList;
class nsIHTMLCollection;

namespace xpc {
namespace dom {

class NodeListBase : public js::ProxyHandler {
public:
    NodeListBase() : js::ProxyHandler(ProxyFamily()) {}

    static void* ProxyFamily() { return &NodeListFamily; }

    static JSObject *create(JSContext *cx, XPCWrappedNativeScope *scope,
                            nsINodeList *aNodeList);
    static JSObject *create(JSContext *cx, XPCWrappedNativeScope *scope,
                            nsIHTMLCollection *aHTMLCollection,
                            nsWrapperCache *aWrapperCache);
private:
    static int NodeListFamily;
};




template<class T>
class NodeList : public NodeListBase {
    static NodeList instance;

    static js::Class sProtoClass;

    struct Methods {
        jsid &id;
        JSNative native;
        uintN nargs;
    };

    static Methods sProtoMethods[];

    static bool instanceIsNodeListObject(JSContext *cx, JSObject *obj);
    static JSObject *getPrototype(JSContext *cx, XPCWrappedNativeScope *scope);

    static T *getNodeList(JSObject *obj);

    static uint32 getProtoShape(JSObject *obj);
    static void setProtoShape(JSObject *obj, uint32 shape);

    static JSBool length_getter(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);
    static JSBool item(JSContext *cx, uintN argc, jsval *vp);
    static JSBool namedItem(JSContext *cx, uintN argc, jsval *vp);

    static bool cacheProtoShape(JSContext *cx, JSObject *proxy, JSObject *proto);
    static bool checkForCacheHit(JSContext *cx, JSObject *proxy, JSObject *receiver, JSObject *proto,
                                 jsid id, js::Value *vp, bool *hitp);
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

    static JSObject *create(JSContext *cx, XPCWrappedNativeScope *scope, T *,
                            nsWrapperCache* aWrapperCache);

    static bool objIsNodeList(JSObject *obj) {
        return js::IsProxy(obj) && js::GetProxyHandler(obj) == &instance;
    }
};

}
}

#endif 
