





#ifndef jsproxy_h
#define jsproxy_h

#include "mozilla/Maybe.h"

#include "jsfriendapi.h"

#include "js/CallNonGenericMethod.h"
#include "js/Class.h"

namespace js {

using JS::AutoIdVector;
using JS::CallArgs;
using JS::HandleId;
using JS::HandleObject;
using JS::HandleValue;
using JS::IsAcceptableThis;
using JS::MutableHandle;
using JS::MutableHandleObject;
using JS::MutableHandleValue;
using JS::NativeImpl;
using JS::PrivateValue;
using JS::Value;

class RegExpGuard;
class JS_FRIEND_API(Wrapper);


































class JS_FRIEND_API(BaseProxyHandler)
{
    const void *mFamily;
    bool mHasPrototype;
    bool mHasPolicy;
  protected:
    
    void setHasPrototype(bool aHasPrototype) { mHasPrototype = aHasPrototype; }
    void setHasPolicy(bool aHasPolicy) { mHasPolicy = aHasPolicy; }

  public:
    explicit BaseProxyHandler(const void *family);
    virtual ~BaseProxyHandler();

    bool hasPrototype() {
        return mHasPrototype;
    }

    bool hasPolicy() {
        return mHasPolicy;
    }

    inline const void *family() {
        return mFamily;
    }
    static size_t offsetOfFamily() {
        return offsetof(BaseProxyHandler, mFamily);
    }

    virtual bool isOuterWindow() {
        return false;
    }

    virtual bool finalizeInBackground(Value priv) {
        



        return true;
    }

    









    enum Action {
        GET,
        SET,
        CALL
    };
    virtual bool enter(JSContext *cx, HandleObject wrapper, HandleId id, Action act,
                       bool *bp);

    
    virtual bool preventExtensions(JSContext *cx, HandleObject proxy) = 0;
    virtual bool getPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc,
                                       unsigned flags) = 0;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, HandleObject proxy,
                                          HandleId id, MutableHandle<JSPropertyDescriptor> desc,
                                          unsigned flags) = 0;
    virtual bool defineProperty(JSContext *cx, HandleObject proxy, HandleId id,
                                MutableHandle<JSPropertyDescriptor> desc) = 0;
    virtual bool getOwnPropertyNames(JSContext *cx, HandleObject proxy,
                                     AutoIdVector &props) = 0;
    virtual bool delete_(JSContext *cx, HandleObject proxy, HandleId id, bool *bp) = 0;
    virtual bool enumerate(JSContext *cx, HandleObject proxy, AutoIdVector &props) = 0;

    
    virtual bool has(JSContext *cx, HandleObject proxy, HandleId id, bool *bp);
    virtual bool hasOwn(JSContext *cx, HandleObject proxy, HandleId id, bool *bp);
    virtual bool get(JSContext *cx, HandleObject proxy, HandleObject receiver,
                     HandleId id, MutableHandleValue vp);
    virtual bool set(JSContext *cx, HandleObject proxy, HandleObject receiver,
                     HandleId id, bool strict, MutableHandleValue vp);
    virtual bool keys(JSContext *cx, HandleObject proxy, AutoIdVector &props);
    virtual bool iterate(JSContext *cx, HandleObject proxy, unsigned flags,
                         MutableHandleValue vp);

    
    virtual bool isExtensible(JSContext *cx, HandleObject proxy, bool *extensible) = 0;
    virtual bool call(JSContext *cx, HandleObject proxy, const CallArgs &args);
    virtual bool construct(JSContext *cx, HandleObject proxy, const CallArgs &args);
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl, CallArgs args);
    virtual bool hasInstance(JSContext *cx, HandleObject proxy, MutableHandleValue v, bool *bp);
    virtual bool objectClassIs(HandleObject obj, ESClassValue classValue, JSContext *cx);
    virtual const char *className(JSContext *cx, HandleObject proxy);
    virtual JSString *fun_toString(JSContext *cx, HandleObject proxy, unsigned indent);
    virtual bool regexp_toShared(JSContext *cx, HandleObject proxy, RegExpGuard *g);
    virtual bool defaultValue(JSContext *cx, HandleObject obj, JSType hint, MutableHandleValue vp);
    virtual void finalize(JSFreeOp *fop, JSObject *proxy);
    virtual bool getElementIfPresent(JSContext *cx, HandleObject obj, HandleObject receiver,
                                     uint32_t index, MutableHandleValue vp, bool *present);
    virtual bool getPrototypeOf(JSContext *cx, HandleObject proxy, MutableHandleObject protop);

    
    virtual JSObject *weakmapKeyDelegate(JSObject *proxy);
};







class JS_PUBLIC_API(DirectProxyHandler) : public BaseProxyHandler
{
  public:
    explicit DirectProxyHandler(const void *family);

    
    virtual bool preventExtensions(JSContext *cx, HandleObject proxy) MOZ_OVERRIDE;
    virtual bool getPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc, unsigned flags) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, HandleObject proxy,
                                          HandleId id, MutableHandle<JSPropertyDescriptor> desc,
                                          unsigned flags) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, HandleObject proxy, HandleId id,
                                MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, HandleObject proxy,
                                     AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, HandleObject proxy, HandleId id,
                         bool *bp) MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, HandleObject proxy,
                           AutoIdVector &props) MOZ_OVERRIDE;

    
    virtual bool has(JSContext *cx, HandleObject proxy, HandleId id,
                     bool *bp) MOZ_OVERRIDE;
    virtual bool hasOwn(JSContext *cx, HandleObject proxy, HandleId id,
                        bool *bp) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, HandleObject proxy, HandleObject receiver,
                     HandleId id, MutableHandleValue vp) MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, HandleObject proxy, HandleObject receiver,
                     HandleId id, bool strict, MutableHandleValue vp) MOZ_OVERRIDE;
    virtual bool keys(JSContext *cx, HandleObject proxy,
                      AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool iterate(JSContext *cx, HandleObject proxy, unsigned flags,
                         MutableHandleValue vp) MOZ_OVERRIDE;

    
    virtual bool isExtensible(JSContext *cx, HandleObject proxy, bool *extensible) MOZ_OVERRIDE;
    virtual bool call(JSContext *cx, HandleObject proxy, const CallArgs &args) MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, HandleObject proxy, const CallArgs &args) MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) MOZ_OVERRIDE;
    virtual bool hasInstance(JSContext *cx, HandleObject proxy, MutableHandleValue v,
                             bool *bp) MOZ_OVERRIDE;
    virtual bool objectClassIs(HandleObject obj, ESClassValue classValue,
                               JSContext *cx) MOZ_OVERRIDE;
    virtual const char *className(JSContext *cx, HandleObject proxy) MOZ_OVERRIDE;
    virtual JSString *fun_toString(JSContext *cx, HandleObject proxy,
                                   unsigned indent) MOZ_OVERRIDE;
    virtual bool regexp_toShared(JSContext *cx, HandleObject proxy,
                                 RegExpGuard *g) MOZ_OVERRIDE;
    virtual JSObject *weakmapKeyDelegate(JSObject *proxy);
};


class Proxy
{
  public:
    
    static bool preventExtensions(JSContext *cx, HandleObject proxy);
    static bool getPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                      MutableHandle<JSPropertyDescriptor> desc, unsigned flags);
    static bool getPropertyDescriptor(JSContext *cx, HandleObject proxy, unsigned flags, HandleId id,
                                      MutableHandleValue vp);
    static bool getOwnPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                         MutableHandle<JSPropertyDescriptor> desc, unsigned flags);
    static bool getOwnPropertyDescriptor(JSContext *cx, HandleObject proxy, unsigned flags, HandleId id,
                                         MutableHandleValue vp);
    static bool defineProperty(JSContext *cx, HandleObject proxy, HandleId id,
                               MutableHandle<JSPropertyDescriptor> desc);
    static bool defineProperty(JSContext *cx, HandleObject proxy, HandleId id, HandleValue v);
    static bool getOwnPropertyNames(JSContext *cx, HandleObject proxy, AutoIdVector &props);
    static bool delete_(JSContext *cx, HandleObject proxy, HandleId id, bool *bp);
    static bool enumerate(JSContext *cx, HandleObject proxy, AutoIdVector &props);

    
    static bool has(JSContext *cx, HandleObject proxy, HandleId id, bool *bp);
    static bool hasOwn(JSContext *cx, HandleObject proxy, HandleId id, bool *bp);
    static bool get(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id,
                    MutableHandleValue vp);
    static bool getElementIfPresent(JSContext *cx, HandleObject proxy, HandleObject receiver,
                                    uint32_t index, MutableHandleValue vp, bool *present);
    static bool set(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id,
                    bool strict, MutableHandleValue vp);
    static bool keys(JSContext *cx, HandleObject proxy, AutoIdVector &props);
    static bool iterate(JSContext *cx, HandleObject proxy, unsigned flags, MutableHandleValue vp);

    
    static bool isExtensible(JSContext *cx, HandleObject proxy, bool *extensible);
    static bool call(JSContext *cx, HandleObject proxy, const CallArgs &args);
    static bool construct(JSContext *cx, HandleObject proxy, const CallArgs &args);
    static bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl, CallArgs args);
    static bool hasInstance(JSContext *cx, HandleObject proxy, MutableHandleValue v, bool *bp);
    static bool objectClassIs(HandleObject obj, ESClassValue classValue, JSContext *cx);
    static const char *className(JSContext *cx, HandleObject proxy);
    static JSString *fun_toString(JSContext *cx, HandleObject proxy, unsigned indent);
    static bool regexp_toShared(JSContext *cx, HandleObject proxy, RegExpGuard *g);
    static bool defaultValue(JSContext *cx, HandleObject obj, JSType hint, MutableHandleValue vp);
    static bool getPrototypeOf(JSContext *cx, HandleObject proxy, MutableHandleObject protop);

    static JSObject * const LazyProto;
};


extern JS_FRIEND_DATA(const js::Class* const) CallableProxyClassPtr;
extern JS_FRIEND_DATA(const js::Class* const) UncallableProxyClassPtr;
extern JS_FRIEND_DATA(const js::Class* const) OuterWindowProxyClassPtr;

inline bool IsProxyClass(const Class *clasp)
{
    return clasp == CallableProxyClassPtr ||
           clasp == UncallableProxyClassPtr ||
           clasp == OuterWindowProxyClassPtr;
}

inline bool IsProxy(JSObject *obj)
{
    return IsProxyClass(GetObjectClass(obj));
}









const uint32_t PROXY_PRIVATE_SLOT = 0;
const uint32_t PROXY_HANDLER_SLOT = 1;
const uint32_t PROXY_EXTRA_SLOT   = 2;

inline BaseProxyHandler *
GetProxyHandler(JSObject *obj)
{
    JS_ASSERT(IsProxy(obj));
    return (BaseProxyHandler *) GetReservedSlot(obj, PROXY_HANDLER_SLOT).toPrivate();
}

inline const Value &
GetProxyPrivate(JSObject *obj)
{
    JS_ASSERT(IsProxy(obj));
    return GetReservedSlot(obj, PROXY_PRIVATE_SLOT);
}

inline JSObject *
GetProxyTargetObject(JSObject *obj)
{
    JS_ASSERT(IsProxy(obj));
    return GetProxyPrivate(obj).toObjectOrNull();
}

inline const Value &
GetProxyExtra(JSObject *obj, size_t n)
{
    JS_ASSERT(IsProxy(obj));
    return GetReservedSlot(obj, PROXY_EXTRA_SLOT + n);
}

inline void
SetProxyHandler(JSObject *obj, BaseProxyHandler *handler)
{
    JS_ASSERT(IsProxy(obj));
    SetReservedSlot(obj, PROXY_HANDLER_SLOT, PrivateValue(handler));
}

inline void
SetProxyExtra(JSObject *obj, size_t n, const Value &extra)
{
    JS_ASSERT(IsProxy(obj));
    JS_ASSERT(n <= 1);
    SetReservedSlot(obj, PROXY_EXTRA_SLOT + n, extra);
}

class MOZ_STACK_CLASS ProxyOptions {
  public:
    ProxyOptions() : callable_(false),
                     singleton_(false),
                     forceForegroundFinalization_(false)
    {}

    bool callable() const { return callable_; }
    ProxyOptions &setCallable(bool flag) {
        callable_ = flag;
        return *this;
    }

    bool singleton() const { return singleton_; }
    ProxyOptions &setSingleton(bool flag) {
        singleton_ = flag;
        return *this;
    }

    bool forceForegroundFinalization() const {
        return forceForegroundFinalization_;
    }
    ProxyOptions &setForceForegroundFinalization(bool flag) {
        forceForegroundFinalization_ = true;
        return *this;
    }

  private:
    bool callable_;
    bool singleton_;
    bool forceForegroundFinalization_;
};

JS_FRIEND_API(JSObject *)
NewProxyObject(JSContext *cx, BaseProxyHandler *handler, HandleValue priv,
               JSObject *proto, JSObject *parent, const ProxyOptions &options = ProxyOptions());

JSObject *
RenewProxyObject(JSContext *cx, JSObject *obj, BaseProxyHandler *handler, Value priv);

class JS_FRIEND_API(AutoEnterPolicy)
{
  public:
    typedef BaseProxyHandler::Action Action;
    AutoEnterPolicy(JSContext *cx, BaseProxyHandler *handler,
                    HandleObject wrapper, HandleId id, Action act, bool mayThrow)
#ifdef DEBUG
        : context(NULL)
#endif
    {
        allow = handler->hasPolicy() ? handler->enter(cx, wrapper, id, act, &rv)
                                     : true;
        recordEnter(cx, wrapper, id);
        
        
        
        
        
        if (!allow && !rv && mayThrow)
            reportErrorIfExceptionIsNotPending(cx, id);
    }

    virtual ~AutoEnterPolicy() { recordLeave(); }
    inline bool allowed() { return allow; }
    inline bool returnValue() { JS_ASSERT(!allowed()); return rv; }

  protected:
    
    AutoEnterPolicy()
#ifdef DEBUG
        : context(NULL)
#endif
        {};
    void reportErrorIfExceptionIsNotPending(JSContext *cx, jsid id);
    bool allow;
    bool rv;

#ifdef DEBUG
    JSContext *context;
    mozilla::Maybe<HandleObject> enteredProxy;
    mozilla::Maybe<HandleId> enteredId;
    
    
    
    AutoEnterPolicy *prev;
    void recordEnter(JSContext *cx, HandleObject proxy, HandleId id);
    void recordLeave();

    friend JS_FRIEND_API(void) assertEnteredPolicy(JSContext *cx, JSObject *proxy, jsid id);
#else
    inline void recordEnter(JSContext *cx, JSObject *proxy, jsid id) {}
    inline void recordLeave() {}
#endif

};

#ifdef DEBUG
class JS_FRIEND_API(AutoWaivePolicy) : public AutoEnterPolicy {
public:
    AutoWaivePolicy(JSContext *cx, HandleObject proxy, HandleId id)
    {
        allow = true;
        recordEnter(cx, proxy, id);
    }
};
#else
class JS_FRIEND_API(AutoWaivePolicy) {
    public: AutoWaivePolicy(JSContext *cx, HandleObject proxy, HandleId id) {};
};
#endif

} 

extern JS_FRIEND_API(JSObject *)
js_InitProxyClass(JSContext *cx, JS::HandleObject obj);

#endif 
