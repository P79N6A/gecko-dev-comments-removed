





#ifndef jswrapper_h
#define jswrapper_h

#include "mozilla/Attributes.h"

#include "jsproxy.h"

namespace js {

class DummyFrameGuard;







class MOZ_STACK_CLASS WrapperOptions : public ProxyOptions {
  public:
    WrapperOptions() : ProxyOptions(false, nullptr),
                       proto_()
    {}

    explicit WrapperOptions(JSContext *cx) : ProxyOptions(false, nullptr),
                                             proto_()
    {
        proto_.construct(cx);
    }

    inline JSObject *proto() const;
    WrapperOptions &setProto(JSObject *protoArg) {
        JS_ASSERT(!proto_.empty());
        proto_.ref() = protoArg;
        return *this;
    }

  private:
    mozilla::Maybe<JS::RootedObject> proto_;
};










class JS_FRIEND_API(Wrapper) : public DirectProxyHandler
{
    unsigned mFlags;

  public:
    using BaseProxyHandler::Action;

    enum Flags {
        CROSS_COMPARTMENT = 1 << 0,
        LAST_USED_FLAG = CROSS_COMPARTMENT
    };

    virtual bool defaultValue(JSContext *cx, HandleObject obj, JSType hint,
                              MutableHandleValue vp) MOZ_OVERRIDE;

    static JSObject *New(JSContext *cx, JSObject *obj, JSObject *parent, Wrapper *handler,
                         const WrapperOptions *options = nullptr);

    static JSObject *Renew(JSContext *cx, JSObject *existing, JSObject *obj, Wrapper *handler);

    static Wrapper *wrapperHandler(JSObject *wrapper);

    static JSObject *wrappedObject(JSObject *wrapper);

    unsigned flags() const {
        return mFlags;
    }

    explicit Wrapper(unsigned flags, bool hasPrototype = false);

    virtual bool finalizeInBackground(Value priv) MOZ_OVERRIDE;

    static Wrapper singleton;
    static Wrapper singletonWithPrototype;

    static JSObject *defaultProto;
};

inline JSObject *
WrapperOptions::proto() const
{
    return proto_.empty() ? Wrapper::defaultProto : proto_.ref();
}


class JS_FRIEND_API(CrossCompartmentWrapper) : public Wrapper
{
  public:
    explicit CrossCompartmentWrapper(unsigned flags, bool hasPrototype = false);

    
    virtual bool preventExtensions(JSContext *cx, HandleObject wrapper) MOZ_OVERRIDE;
    virtual bool getPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                          MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                                MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, HandleObject wrapper,
                                     AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, HandleObject wrapper, HandleId id, bool *bp) MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, HandleObject wrapper, AutoIdVector &props) MOZ_OVERRIDE;

    
    virtual bool has(JSContext *cx, HandleObject wrapper, HandleId id, bool *bp) MOZ_OVERRIDE;
    virtual bool hasOwn(JSContext *cx, HandleObject wrapper, HandleId id, bool *bp) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, HandleObject wrapper, HandleObject receiver,
                     HandleId id, MutableHandleValue vp) MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, HandleObject wrapper, HandleObject receiver,
                     HandleId id, bool strict, MutableHandleValue vp) MOZ_OVERRIDE;
    virtual bool keys(JSContext *cx, HandleObject wrapper, AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool iterate(JSContext *cx, HandleObject wrapper, unsigned flags,
                         MutableHandleValue vp) MOZ_OVERRIDE;

    
    virtual bool isExtensible(JSContext *cx, HandleObject wrapper, bool *extensible) MOZ_OVERRIDE;
    virtual bool call(JSContext *cx, HandleObject wrapper, const CallArgs &args) MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, HandleObject wrapper, const CallArgs &args) MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) MOZ_OVERRIDE;
    virtual bool hasInstance(JSContext *cx, HandleObject wrapper, MutableHandleValue v,
                             bool *bp) MOZ_OVERRIDE;
    virtual const char *className(JSContext *cx, HandleObject proxy) MOZ_OVERRIDE;
    virtual JSString *fun_toString(JSContext *cx, HandleObject wrapper,
                                   unsigned indent) MOZ_OVERRIDE;
    virtual bool regexp_toShared(JSContext *cx, HandleObject proxy, RegExpGuard *g) MOZ_OVERRIDE;
    virtual bool defaultValue(JSContext *cx, HandleObject wrapper, JSType hint,
                              MutableHandleValue vp) MOZ_OVERRIDE;
    virtual bool getPrototypeOf(JSContext *cx, HandleObject proxy,
                                MutableHandleObject protop) MOZ_OVERRIDE;
    virtual bool setPrototypeOf(JSContext *cx, HandleObject proxy, HandleObject proto,
                                bool *bp) MOZ_OVERRIDE;

    static CrossCompartmentWrapper singleton;
    static CrossCompartmentWrapper singletonWithPrototype;
};










template <class Base>
class JS_FRIEND_API(SecurityWrapper) : public Base
{
  public:
    explicit SecurityWrapper(unsigned flags);

    virtual bool isExtensible(JSContext *cx, HandleObject wrapper, bool *extensible) MOZ_OVERRIDE;
    virtual bool preventExtensions(JSContext *cx, HandleObject wrapper) MOZ_OVERRIDE;
    virtual bool enter(JSContext *cx, HandleObject wrapper, HandleId id, Wrapper::Action act,
                       bool *bp) MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) MOZ_OVERRIDE;
    virtual bool defaultValue(JSContext *cx, HandleObject wrapper, JSType hint,
                              MutableHandleValue vp) MOZ_OVERRIDE;
    virtual bool objectClassIs(HandleObject obj, ESClassValue classValue,
                               JSContext *cx) MOZ_OVERRIDE;
    virtual bool regexp_toShared(JSContext *cx, HandleObject proxy, RegExpGuard *g) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                                MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;

    virtual bool setPrototypeOf(JSContext *cx, HandleObject proxy, HandleObject proto,
                                bool *bp) MOZ_OVERRIDE;

    virtual bool watch(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
                       JS::HandleObject callable) MOZ_OVERRIDE;
    virtual bool unwatch(JSContext *cx, JS::HandleObject proxy, JS::HandleId id) MOZ_OVERRIDE;

    



    typedef Base Permissive;
    typedef SecurityWrapper<Base> Restrictive;
};

typedef SecurityWrapper<Wrapper> SameCompartmentSecurityWrapper;
typedef SecurityWrapper<CrossCompartmentWrapper> CrossCompartmentSecurityWrapper;

class JS_FRIEND_API(DeadObjectProxy) : public BaseProxyHandler
{
  public:
    
    static const char sDeadObjectFamily;

    explicit DeadObjectProxy();

    
    virtual bool preventExtensions(JSContext *cx, HandleObject proxy) MOZ_OVERRIDE;
    virtual bool getPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                          MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                                MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, HandleObject wrapper,
                                     AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, HandleObject wrapper, HandleId id, bool *bp) MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, HandleObject wrapper, AutoIdVector &props) MOZ_OVERRIDE;

    
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
    virtual JSString *fun_toString(JSContext *cx, HandleObject proxy, unsigned indent) MOZ_OVERRIDE;
    virtual bool regexp_toShared(JSContext *cx, HandleObject proxy, RegExpGuard *g) MOZ_OVERRIDE;
    virtual bool defaultValue(JSContext *cx, HandleObject obj, JSType hint,
                              MutableHandleValue vp) MOZ_OVERRIDE;
    virtual bool getPrototypeOf(JSContext *cx, HandleObject proxy,
                                MutableHandleObject protop) MOZ_OVERRIDE;

    static DeadObjectProxy singleton;
};

extern JSObject *
TransparentObjectWrapper(JSContext *cx, HandleObject existing, HandleObject obj,
                         HandleObject wrappedProto, HandleObject parent,
                         unsigned flags);




extern JS_FRIEND_DATA(const char) sWrapperFamily;

inline bool
IsWrapper(JSObject *obj)
{
    return IsProxy(obj) && GetProxyHandler(obj)->family() == &sWrapperFamily;
}





JS_FRIEND_API(JSObject *)
UncheckedUnwrap(JSObject *obj, bool stopAtOuter = true, unsigned *flagsp = nullptr);





JS_FRIEND_API(JSObject *)
CheckedUnwrap(JSObject *obj, bool stopAtOuter = true);



JS_FRIEND_API(JSObject *)
UnwrapOneChecked(JSObject *obj, bool stopAtOuter = true);

JS_FRIEND_API(bool)
IsCrossCompartmentWrapper(JSObject *obj);

bool
IsDeadProxyObject(JSObject *obj);

JSObject *
NewDeadProxyObject(JSContext *cx, JSObject *parent,
                   const ProxyOptions &options = ProxyOptions());

void
NukeCrossCompartmentWrapper(JSContext *cx, JSObject *wrapper);

bool
RemapWrapper(JSContext *cx, JSObject *wobj, JSObject *newTarget);

JS_FRIEND_API(bool)
RemapAllWrappersForObject(JSContext *cx, JSObject *oldTarget,
                          JSObject *newTarget);



JS_FRIEND_API(bool)
RecomputeWrappers(JSContext *cx, const CompartmentFilter &sourceFilter,
                  const CompartmentFilter &targetFilter);















struct JS_FRIEND_API(AutoMaybeTouchDeadZones)
{
    
    explicit AutoMaybeTouchDeadZones(JSContext *cx);
    explicit AutoMaybeTouchDeadZones(JSObject *obj);
    ~AutoMaybeTouchDeadZones();

  private:
    JSRuntime *runtime;
    unsigned markCount;
    bool inIncremental;
    bool manipulatingDeadZones;
};

} 

#endif 
