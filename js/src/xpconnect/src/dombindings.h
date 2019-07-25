






































#ifndef dombindings_h
#define dombindings_h

#include "jsapi.h"
#include "jsproxy.h"
#include "xpcpublic.h"

class nsINode;
class nsINodeList;
class nsIHTMLCollection;

namespace mozilla {
namespace dom {
namespace binding {

inline nsWrapperCache*
GetWrapperCache(nsWrapperCache *cache)
{
    return cache;
}



class nsGlobalWindow;
inline nsWrapperCache*
GetWrapperCache(nsGlobalWindow *not_allowed);

inline nsWrapperCache*
GetWrapperCache(void *p)
{
    return nsnull;
}

class ProxyHandler : public js::ProxyHandler {
protected:
    ProxyHandler() : js::ProxyHandler(ProxyFamily())
    {
    }

public:
    virtual bool isInstanceOf(JSObject *prototype) = 0;
};

class NodeListBase : public ProxyHandler {
public:
    NodeListBase() : ProxyHandler() {}

    static JSObject *create(JSContext *cx, XPCWrappedNativeScope *scope,
                            nsINodeList *aNodeList, bool *triedToWrap);
    static JSObject *create(JSContext *cx, XPCWrappedNativeScope *scope,
                            nsIHTMLCollection *aHTMLCollection,
                            nsWrapperCache *aWrapperCache, bool *triedToWrap);
};




template<class T>
class NodeList : public NodeListBase {
    friend void Register(nsDOMClassInfoData *aData);

    static NodeList instance;

    static js::Class sInterfaceClass;

    struct Properties {
        jsid &id;
        JSPropertyOp getter;
        JSStrictPropertyOp setter;
    };
    struct Methods {
        jsid &id;
        JSNative native;
        uintN nargs;
    };

    static Properties sProtoProperties[];
    static Methods sProtoMethods[];

    static bool instanceIsNodeListObject(JSContext *cx, JSObject *obj, JSObject *callee);
    static JSObject *getPrototype(JSContext *cx, XPCWrappedNativeScope *scope, bool *enabled);

    static JSObject *ensureExpandoObject(JSContext *cx, JSObject *obj);

    static uint32 getProtoShape(JSObject *obj);
    static void setProtoShape(JSObject *obj, uint32 shape);

    static JSBool length_getter(JSContext *cx, JSObject *obj, jsid id, jsval *vp);
    static JSBool item(JSContext *cx, uintN argc, jsval *vp);
    static JSBool namedItem(JSContext *cx, uintN argc, jsval *vp);

    static bool hasNamedItem(jsid id);
    static bool namedItem(JSContext *cx, JSObject *obj, jsval *name, nsISupports **result,
                          nsWrapperCache **cache, bool *hasResult);

    static nsISupports *getNamedItem(T *list, const nsAString& aName, nsWrapperCache **aCache);

    static bool cacheProtoShape(JSContext *cx, JSObject *proxy, JSObject *proto);
    static bool checkForCacheHit(JSContext *cx, JSObject *proxy, JSObject *receiver, JSObject *proto,
                                 jsid id, js::Value *vp, bool *hitp);

    static bool resolveNativeName(JSContext *cx, JSObject *proxy, jsid id,
                                  js::PropertyDescriptor *desc);
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
                            nsWrapperCache* aWrapperCache, bool *enabled);

    static bool objIsNodeList(JSObject *obj) {
        return js::IsProxy(obj) && js::GetProxyHandler(obj) == &instance;
    }
    virtual bool isInstanceOf(JSObject *prototype)
    {
        return js::GetObjectClass(prototype) == &sInterfaceClass;
    }
    static T *getNodeList(JSObject *obj);
};

}
}
}

#endif 
