






































#ifndef dombindings_h
#define dombindings_h

#include "jsapi.h"
#include "jsproxy.h"
#include "xpcpublic.h"

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

class NoType;
class NoOp {
public:
    typedef NoType* T;
    enum {
        hasOp = 0
    };
};

template<typename Type>
class Op {
public:
    typedef Type T;
    enum {
        hasOp = 1
    };
};

template<typename Type>
class Getter : public Op<Type>
{
};

template<typename Type>
class Setter : public Op<Type>
{
};

template<class Getter, class Setter=NoOp>
class Ops
{
public:
    typedef Getter G;
    typedef Setter S;
};

typedef Ops<NoOp, NoOp> NoOps;

template<class ListType, class Base, class IndexOps, class NameOps=NoOps>
class DerivedListClass {
public:
    typedef ListType LT;
    typedef Base B;
    typedef IndexOps IO;
    typedef NameOps NO;
};

class NoBase {
public:
    static JSObject *getPrototype(JSContext *cx, XPCWrappedNativeScope *scope);
    static bool shouldCacheProtoShape(JSContext *cx, JSObject *proto, bool *shouldCache)
    {
        *shouldCache = true;
        return true;
    }
    static bool resolveNativeName(JSContext *cx, JSObject *proxy, jsid id, JSPropertyDescriptor *desc)
    {
        return true;
    }
    static bool nativeGet(JSContext *cx, JSObject *proxy, JSObject *proto, jsid id, bool *found,
                          JS::Value *vp)
    {
        *found = false;
        return true;
    }
};

template<class ListType, class IndexOps, class NameOps=NoOps>
class ListClass : public DerivedListClass<ListType, NoBase, IndexOps, NameOps> {
};

template<class LC>
class ListBase : public ProxyHandler {
protected:
    typedef typename LC::LT ListType;
    typedef typename LC::B Base;
    typedef typename LC::IO::G::T IndexGetterType;
    typedef typename LC::IO::S::T IndexSetterType;
    typedef typename LC::NO::G::T NameGetterType;
    typedef typename LC::NO::S::T NameSetterType;
    enum {
        hasIndexGetter = LC::IO::G::hasOp,
        hasIndexSetter = LC::IO::S::hasOp,
        hasNameGetter = LC::NO::G::hasOp,
        hasNameSetter = LC::NO::S::hasOp
    };

private:
    friend void Register(nsDOMClassInfoData *aData);

    static ListBase<LC> instance;

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
    static size_t sProtoPropertiesCount;
    static Methods sProtoMethods[];
    static size_t sProtoMethodsCount;

    static JSObject *ensureExpandoObject(JSContext *cx, JSObject *obj);

    static js::Shape *getProtoShape(JSObject *obj);
    static void setProtoShape(JSObject *obj, js::Shape *shape);

    static JSBool length_getter(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

    static inline bool getItemAt(ListType *list, uint32_t i, IndexGetterType &item);
    static inline bool setItemAt(JSContext *cx, ListType *list, uint32_t i, IndexSetterType item);

    static inline bool namedItem(JSContext *cx, JSObject *obj, jsval *name, NameGetterType &result,
                                 bool *hasResult);

    static inline bool getNamedItem(ListType *list, const nsAString& aName, NameGetterType &item);
    static inline bool setNamedItem(JSContext *cx, ListType *list, const nsAString& aName,
                                    NameSetterType item);

    static bool getPropertyOnPrototype(JSContext *cx, JSObject *proxy, jsid id, bool *found,
                                       JS::Value *vp);
    static bool hasPropertyOnPrototype(JSContext *cx, JSObject *proxy, jsid id);

public:
    static JSObject *create(JSContext *cx, XPCWrappedNativeScope *scope, ListType *list,
                            nsWrapperCache* cache, bool *triedToWrap);

    static JSObject *getPrototype(JSContext *cx, XPCWrappedNativeScope *scope, bool *enabled);

    bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                               JSPropertyDescriptor *desc);
    bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                  JSPropertyDescriptor *desc);
    bool defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                        JSPropertyDescriptor *desc);
    bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, JS::AutoIdVector &props);
    bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    bool enumerate(JSContext *cx, JSObject *proxy, JS::AutoIdVector &props);
    bool fix(JSContext *cx, JSObject *proxy, JS::Value *vp);

    bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, JS::Value *vp);
    bool getElementIfPresent(JSContext *cx, JSObject *proxy, JSObject *receiver,
                             uint32_t index, JS::Value *vp, bool *present);
    bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, bool strict,
             JS::Value *vp);
    bool keys(JSContext *cx, JSObject *proxy, JS::AutoIdVector &props);
    bool iterate(JSContext *cx, JSObject *proxy, uintN flags, JS::Value *vp);

    
    bool hasInstance(JSContext *cx, JSObject *proxy, const JS::Value *vp, bool *bp);
    JSString *obj_toString(JSContext *cx, JSObject *proxy);
    void finalize(JSContext *cx, JSObject *proxy);

    static bool proxyHandlerIsList(js::ProxyHandler *handler) {
        return handler == &instance;
    }
    static bool objIsList(JSObject *obj) {
        return js::IsProxy(obj) && proxyHandlerIsList(js::GetProxyHandler(obj));
    }
    static inline bool instanceIsListObject(JSContext *cx, JSObject *obj, JSObject *callee);
    virtual bool isInstanceOf(JSObject *prototype)
    {
        return js::GetObjectClass(prototype) == &sInterfaceClass;
    }
    static inline ListType *getListObject(JSObject *obj);

    static JSObject *getPrototype(JSContext *cx, XPCWrappedNativeScope *scope);
    static inline bool protoIsClean(JSContext *cx, JSObject *proto, bool *isClean);
    static bool shouldCacheProtoShape(JSContext *cx, JSObject *proto, bool *shouldCache);
    static bool resolveNativeName(JSContext *cx, JSObject *proxy, jsid id,
                                  JSPropertyDescriptor *desc);
    static bool nativeGet(JSContext *cx, JSObject *proxy, JSObject *proto, jsid id, bool *found,
                          JS::Value *vp);
    static ListType *getNative(JSObject *proxy);
};

struct nsISupportsResult
{
    nsISupportsResult()
    {
    }
    nsISupports *mResult;
    nsWrapperCache *mCache;
};

}
}
}

#include "dombindings_gen.h"

#endif 
