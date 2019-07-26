






#include "mozilla/Attributes.h"
#include "mozilla/GuardObjects.h"

#include "jsapi.h"
#include "jswrapper.h"







class XPCWrappedNative;

namespace xpc {

JSBool
holder_get(JSContext *cx, JSHandleObject holder, JSHandleId id, JSMutableHandleValue vp);
JSBool
holder_set(JSContext *cx, JSHandleObject holder, JSHandleId id, JSBool strict, JSMutableHandleValue vp);

namespace XrayUtils {

extern JSClass HolderClass;

bool CloneExpandoChain(JSContext *cx, JSObject *src, JSObject *dst);

bool
IsTransparent(JSContext *cx, JSObject *wrapper, jsid id);

JSObject *
GetNativePropertiesObject(JSContext *cx, JSObject *wrapper);

bool
IsXrayResolving(JSContext *cx, JSObject *wrapper, jsid id);

}

class XrayTraits;
class XPCWrappedNativeXrayTraits;
class ProxyXrayTraits;
class DOMXrayTraits;


enum XrayType {
    XrayForDOMObject,
    XrayForWrappedNative,
    NotXray
};

XrayType GetXrayType(JSObject *obj);
XrayTraits* GetXrayTraits(JSObject *obj);


template <typename Base, typename Traits = XPCWrappedNativeXrayTraits >
class XrayWrapper : public Base {
  public:
    XrayWrapper(unsigned flags);
    virtual ~XrayWrapper();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                       js::PropertyDescriptor *desc, unsigned flags) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          js::PropertyDescriptor *desc,
                                          unsigned flags) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper,
                                     js::AutoIdVector &props);
    virtual bool delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool enumerate(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);

    
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                     js::Value *vp);
    virtual bool set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                     bool strict, js::Value *vp);
    virtual bool has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool keys(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool iterate(JSContext *cx, JSObject *wrapper, unsigned flags, js::Value *vp);

    virtual bool call(JSContext *cx, JSObject *wrapper, unsigned argc, js::Value *vp);
    virtual bool construct(JSContext *cx, JSObject *wrapper,
                           unsigned argc, js::Value *argv, js::Value *rval);

    static XrayWrapper singleton;

  private:
    bool enumerate(JSContext *cx, JSObject *wrapper, unsigned flags,
                   JS::AutoIdVector &props);
};

#define PermissiveXrayXPCWN xpc::XrayWrapper<js::CrossCompartmentWrapper, xpc::XPCWrappedNativeXrayTraits>
#define SecurityXrayXPCWN xpc::XrayWrapper<js::CrossCompartmentSecurityWrapper, xpc::XPCWrappedNativeXrayTraits>
#define PermissiveXrayDOM xpc::XrayWrapper<js::CrossCompartmentWrapper, xpc::DOMXrayTraits>
#define SecurityXrayDOM xpc::XrayWrapper<js::CrossCompartmentSecurityWrapper, xpc::DOMXrayTraits>
#define SCSecurityXrayXPCWN xpc::XrayWrapper<js::SameCompartmentSecurityWrapper, xpc::XPCWrappedNativeXrayTraits>
#define SCPermissiveXrayXPCWN xpc::XrayWrapper<js::Wrapper, xpc::XPCWrappedNativeXrayTraits>
#define SCPermissiveXrayDOM xpc::XrayWrapper<js::Wrapper, xpc::DOMXrayTraits>

class SandboxProxyHandler : public js::Wrapper {
public:
    SandboxProxyHandler() : js::Wrapper(0)
    {
    }

    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                       js::PropertyDescriptor *desc, unsigned flags) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy,
                                          jsid id, js::PropertyDescriptor *desc,
                                          unsigned flags) MOZ_OVERRIDE;

    
    
    virtual bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp) MOZ_OVERRIDE;
    virtual bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JSObject *proxy, JSObject *receiver,
                     jsid id, JS::Value *vp) MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, JSObject *proxy, JSObject *receiver,
                     jsid id, bool strict, JS::Value *vp) MOZ_OVERRIDE;
    virtual bool keys(JSContext *cx, JSObject *proxy, JS::AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool iterate(JSContext *cx, JSObject *proxy, unsigned flags,
                         JS::Value *vp) MOZ_OVERRIDE;
};

extern SandboxProxyHandler sandboxProxyHandler;




class SandboxCallableProxyHandler : public js::Wrapper {
public:
    SandboxCallableProxyHandler() : js::Wrapper(0)
    {
    }

    virtual bool call(JSContext *cx, JSObject *proxy, unsigned argc,
                      JS::Value *vp);
};

extern SandboxCallableProxyHandler sandboxCallableProxyHandler;

class AutoSetWrapperNotShadowing;
class XPCWrappedNativeXrayTraits;

class ResolvingId {
public:
    ResolvingId(JSObject *wrapper, jsid id);
    ~ResolvingId();

    bool isXrayShadowing(jsid id);
    bool isResolving(jsid id);
    static ResolvingId* getResolvingId(JSObject *holder);
    static JSObject* getHolderObject(JSObject *wrapper);
    static ResolvingId *getResolvingIdFromWrapper(JSObject *wrapper);

private:
    friend class AutoSetWrapperNotShadowing;
    friend class XPCWrappedNativeXrayTraits;

    jsid mId;
    JSObject *mHolder;
    ResolvingId *mPrev;
    bool mXrayShadowing;
};

}

