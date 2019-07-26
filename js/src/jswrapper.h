






#ifndef jswrapper_h___
#define jswrapper_h___

#include "mozilla/Attributes.h"

#include "jsapi.h"
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

    





    void setSafeToUnwrap(bool safe) { mSafeToUnwrap = safe; }
    virtual bool isSafeToUnwrap() { return mSafeToUnwrap; }

    static JSObject *New(JSContext *cx, JSObject *obj, JSObject *proto,
                         JSObject *parent, Wrapper *handler);

    static JSObject *Renew(JSContext *cx, JSObject *existing, JSObject *obj, Wrapper *handler);

    static Wrapper *wrapperHandler(RawObject wrapper);

    static JSObject *wrappedObject(RawObject wrapper);

    unsigned flags() const {
        return mFlags;
    }

    explicit Wrapper(unsigned flags, bool hasPrototype = false);

    virtual ~Wrapper();

    
    virtual bool defaultValue(JSContext *cx, HandleObject wrapper, JSType hint,
                              MutableHandleValue vp) MOZ_OVERRIDE;

    static Wrapper singleton;
    static Wrapper singletonWithPrototype;

    static void *getWrapperFamily();
};


class JS_FRIEND_API(CrossCompartmentWrapper) : public Wrapper
{
  public:
    CrossCompartmentWrapper(unsigned flags, bool hasPrototype = false);

    virtual ~CrossCompartmentWrapper();

    virtual bool finalizeInBackground(Value priv) MOZ_OVERRIDE;

    
    virtual bool getPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                       PropertyDescriptor *desc, unsigned flags) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                          PropertyDescriptor *desc, unsigned flags) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                                PropertyDescriptor *desc) MOZ_OVERRIDE;
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

    
    virtual bool call(JSContext *cx, HandleObject wrapper, unsigned argc, Value *vp) MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, HandleObject wrapper, unsigned argc, Value *argv,
                           MutableHandleValue rval) MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) MOZ_OVERRIDE;
    virtual bool hasInstance(JSContext *cx, HandleObject wrapper, MutableHandleValue v,
                             bool *bp) MOZ_OVERRIDE;
    virtual JSString *obj_toString(JSContext *cx, HandleObject wrapper) MOZ_OVERRIDE;
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

    virtual bool isExtensible(JSObject *wrapper) MOZ_OVERRIDE;
    virtual bool preventExtensions(JSContext *cx, HandleObject wrapper) MOZ_OVERRIDE;
    virtual bool enter(JSContext *cx, HandleObject wrapper, HandleId id, Wrapper::Action act,
                       bool *bp) MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) MOZ_OVERRIDE;
    virtual bool objectClassIs(HandleObject obj, ESClassValue classValue,
                               JSContext *cx) MOZ_OVERRIDE;
    virtual bool regexp_toShared(JSContext *cx, HandleObject proxy, RegExpGuard *g) MOZ_OVERRIDE;

    



    typedef Base Permissive;
    typedef SecurityWrapper<Base> Restrictive;
};

typedef SecurityWrapper<Wrapper> SameCompartmentSecurityWrapper;
typedef SecurityWrapper<CrossCompartmentWrapper> CrossCompartmentSecurityWrapper;

class JS_FRIEND_API(DeadObjectProxy) : public BaseProxyHandler
{
  public:
    static int sDeadObjectFamily;

    explicit DeadObjectProxy();

    
    virtual bool preventExtensions(JSContext *cx, HandleObject proxy) MOZ_OVERRIDE;
    virtual bool getPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                       PropertyDescriptor *desc, unsigned flags) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                          PropertyDescriptor *desc, unsigned flags) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                                PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, HandleObject wrapper,
                                     AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, HandleObject wrapper, HandleId id, bool *bp) MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, HandleObject wrapper, AutoIdVector &props) MOZ_OVERRIDE;

    
    virtual bool isExtensible(JSObject *proxy) MOZ_OVERRIDE;
    virtual bool call(JSContext *cx, HandleObject proxy, unsigned argc, Value *vp);
    virtual bool construct(JSContext *cx, HandleObject proxy, unsigned argc,
                           Value *argv, MutableHandleValue rval);
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) MOZ_OVERRIDE;
    virtual bool hasInstance(JSContext *cx, HandleObject proxy, MutableHandleValue v, bool *bp);
    virtual bool objectClassIs(HandleObject obj, ESClassValue classValue, JSContext *cx);
    virtual JSString *obj_toString(JSContext *cx, HandleObject proxy);
    virtual JSString *fun_toString(JSContext *cx, HandleObject proxy, unsigned indent);
    virtual bool regexp_toShared(JSContext *cx, HandleObject proxy, RegExpGuard *g);
    virtual bool defaultValue(JSContext *cx, HandleObject obj, JSType hint, MutableHandleValue vp);
    virtual bool getElementIfPresent(JSContext *cx, HandleObject obj, HandleObject receiver,
                                     uint32_t index, MutableHandleValue vp, bool *present);
    virtual bool getPrototypeOf(JSContext *cx, HandleObject proxy, MutableHandleObject protop);

    static DeadObjectProxy singleton;
};

extern JSObject *
TransparentObjectWrapper(JSContext *cx, JSObject *existing, JSObject *obj,
                         JSObject *wrappedProto, JSObject *parent,
                         unsigned flags);



extern JS_FRIEND_DATA(int) sWrapperFamily;

inline bool
IsWrapper(RawObject obj)
{
    return IsProxy(obj) && GetProxyHandler(obj)->family() == &sWrapperFamily;
}





JS_FRIEND_API(JSObject *)
UnwrapObject(JSObject *obj, bool stopAtOuter = true, unsigned *flagsp = NULL);





JS_FRIEND_API(JSObject *)
UnwrapObjectChecked(RawObject obj, bool stopAtOuter = true);



JS_FRIEND_API(JSObject *)
UnwrapOneChecked(RawObject obj, bool stopAtOuter = true);

JS_FRIEND_API(bool)
IsCrossCompartmentWrapper(RawObject obj);

bool
IsDeadProxyObject(RawObject obj);

JSObject *
NewDeadProxyObject(JSContext *cx, JSObject *parent);

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
