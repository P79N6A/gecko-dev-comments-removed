





#ifndef jswrapper_h
#define jswrapper_h

#include "mozilla/Attributes.h"

#include "jsproxy.h"

namespace js {

class DummyFrameGuard;










class JS_FRIEND_API(Wrapper) : public DirectProxyHandler
{
    unsigned mFlags;
    bool mSafeToUnwrap;

  public:
    using BaseProxyHandler::Action;

    enum Flags {
        CROSS_COMPARTMENT = 1 << 0,
        LAST_USED_FLAG = CROSS_COMPARTMENT
    };

    virtual bool defaultValue(JSContext *cx, HandleObject obj, JSType hint,
                              MutableHandleValue vp) MOZ_OVERRIDE;

    





    void setSafeToUnwrap(bool safe) { mSafeToUnwrap = safe; }
    bool isSafeToUnwrap() { return mSafeToUnwrap; }

    static JSObject *New(JSContext *cx, JSObject *obj, JSObject *proto,
                         JSObject *parent, Wrapper *handler);

    static JSObject *Renew(JSContext *cx, JSObject *existing, JSObject *obj, Wrapper *handler);

    static Wrapper *wrapperHandler(JSObject *wrapper);

    static JSObject *wrappedObject(JSObject *wrapper);

    unsigned flags() const {
        return mFlags;
    }

    explicit Wrapper(unsigned flags, bool hasPrototype = false);

    virtual ~Wrapper();

    static Wrapper singleton;
    static Wrapper singletonWithPrototype;
};


class JS_FRIEND_API(CrossCompartmentWrapper) : public Wrapper
{
  public:
    CrossCompartmentWrapper(unsigned flags, bool hasPrototype = false);

    virtual ~CrossCompartmentWrapper();

    virtual bool finalizeInBackground(Value priv) MOZ_OVERRIDE;

    
    virtual bool preventExtensions(JSContext *cx, HandleObject wrapper) MOZ_OVERRIDE;
    virtual bool getPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc,
                                       unsigned flags) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                          MutableHandle<JSPropertyDescriptor> desc,
                                          unsigned flags) MOZ_OVERRIDE;
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
    virtual bool getPrototypeOf(JSContext *cx, HandleObject proxy, MutableHandleObject protop);

    static CrossCompartmentWrapper singleton;
    static CrossCompartmentWrapper singletonWithPrototype;
};










template <class Base>
class JS_FRIEND_API(SecurityWrapper) : public Base
{
  public:
    SecurityWrapper(unsigned flags);

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
                                       MutableHandle<JSPropertyDescriptor> desc,
                                       unsigned flags) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                          MutableHandle<JSPropertyDescriptor> desc,
                                          unsigned flags) MOZ_OVERRIDE;
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
    virtual bool getElementIfPresent(JSContext *cx, HandleObject obj, HandleObject receiver,
                                     uint32_t index, MutableHandleValue vp,
                                     bool *present) MOZ_OVERRIDE;
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
UncheckedUnwrap(JSObject *obj, bool stopAtOuter = true, unsigned *flagsp = NULL);





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
    
    AutoMaybeTouchDeadZones(JSContext *cx);
    AutoMaybeTouchDeadZones(JSObject *obj);
    ~AutoMaybeTouchDeadZones();

  private:
    JSRuntime *runtime;
    unsigned markCount;
    bool inIncremental;
    bool manipulatingDeadZones;
};

} 

#endif 
