






#ifndef jswrapper_h___
#define jswrapper_h___

#include "mozilla/Attributes.h"

#include "jsapi.h"
#include "jsproxy.h"

namespace js {

class DummyFrameGuard;






















class JS_FRIEND_API(Wrapper)
{
    unsigned mFlags;

  public:
    enum Action {
        GET,
        SET,
        CALL,
        PUNCTURE
    };

    enum Flags {
        CROSS_COMPARTMENT = 1 << 0,
        LAST_USED_FLAG = CROSS_COMPARTMENT
    };

    typedef enum {
        PermitObjectAccess,
        PermitPropertyAccess,
        DenyAccess
    } Permission;

    static JSObject *New(JSContext *cx, JSObject *obj, JSObject *proto,
                         JSObject *parent, Wrapper *handler);

    static Wrapper *wrapperHandler(RawObject wrapper);

    static JSObject *wrappedObject(RawObject wrapper);

    explicit Wrapper(unsigned flags);

    unsigned flags() const {
        return mFlags;
    }

    











    virtual BaseProxyHandler *toBaseProxyHandler() = 0;

    
























    virtual bool enter(JSContext *cx, JSObject *wrapper, jsid id, Action act,
                       bool *bp);
};







class JS_FRIEND_API(IndirectWrapper) : public Wrapper,
                                       public IndirectProxyHandler
{
  public:
    explicit IndirectWrapper(unsigned flags);

    virtual BaseProxyHandler* toBaseProxyHandler() {
        return this;
    }

    virtual Wrapper *toWrapper() {
        return this;
    }

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper,
                                       jsid id, bool set,
                                       PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper,
                                          jsid id, bool set,
                                          PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper,
                                     AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, JSObject *wrapper, jsid id,
                         bool *bp) MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, JSObject *wrapper,
                           AutoIdVector &props) MOZ_OVERRIDE;
};






class JS_FRIEND_API(DirectWrapper) : public Wrapper, public DirectProxyHandler
{
  public:
    explicit DirectWrapper(unsigned flags, bool hasPrototype = false);

    virtual ~DirectWrapper();

    virtual BaseProxyHandler* toBaseProxyHandler() {
        return this;
    }

    virtual Wrapper *toWrapper() {
        return this;
    }

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper,
                                       jsid id, bool set,
                                       PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper,
                                          jsid id, bool set,
                                          PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper,
                                     AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, JSObject *wrapper, jsid id,
                         bool *bp) MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, JSObject *wrapper,
                           AutoIdVector &props) MOZ_OVERRIDE;

    
    virtual bool has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp) MOZ_OVERRIDE;
    virtual bool hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, Value *vp) MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, bool strict,
                     Value *vp) MOZ_OVERRIDE;
    virtual bool keys(JSContext *cx, JSObject *wrapper, AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool iterate(JSContext *cx, JSObject *wrapper, unsigned flags, Value *vp) MOZ_OVERRIDE;

    
    virtual bool call(JSContext *cx, JSObject *wrapper, unsigned argc, Value *vp) MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, JSObject *wrapper, unsigned argc, Value *argv, Value *rval) MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) MOZ_OVERRIDE;
    virtual bool hasInstance(JSContext *cx, JSObject *wrapper, const Value *vp, bool *bp) MOZ_OVERRIDE;
    virtual JSString *obj_toString(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE;
    virtual JSString *fun_toString(JSContext *cx, JSObject *wrapper, unsigned indent) MOZ_OVERRIDE;

    static DirectWrapper singleton;
    static DirectWrapper singletonWithPrototype;

    static void *getWrapperFamily();
};


class JS_FRIEND_API(CrossCompartmentWrapper) : public DirectWrapper
{
  public:
    CrossCompartmentWrapper(unsigned flags, bool hasPrototype = false);

    virtual ~CrossCompartmentWrapper();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                                       PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                                          PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper, AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp) MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, JSObject *wrapper, AutoIdVector &props) MOZ_OVERRIDE;

    
    virtual bool has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp) MOZ_OVERRIDE;
    virtual bool hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, Value *vp) MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, bool strict,
                     Value *vp) MOZ_OVERRIDE;
    virtual bool keys(JSContext *cx, JSObject *wrapper, AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool iterate(JSContext *cx, JSObject *wrapper, unsigned flags, Value *vp) MOZ_OVERRIDE;

    
    virtual bool call(JSContext *cx, JSObject *wrapper, unsigned argc, Value *vp) MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, JSObject *wrapper, unsigned argc, Value *argv, Value *rval) MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) MOZ_OVERRIDE;
    virtual bool hasInstance(JSContext *cx, JSObject *wrapper, const Value *vp, bool *bp) MOZ_OVERRIDE;
    virtual JSString *obj_toString(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE;
    virtual JSString *fun_toString(JSContext *cx, JSObject *wrapper, unsigned indent) MOZ_OVERRIDE;
    virtual bool regexp_toShared(JSContext *cx, JSObject *proxy, RegExpGuard *g) MOZ_OVERRIDE;
    virtual bool defaultValue(JSContext *cx, JSObject *wrapper, JSType hint, Value *vp) MOZ_OVERRIDE;
    virtual bool iteratorNext(JSContext *cx, JSObject *wrapper, Value *vp);

    static CrossCompartmentWrapper singleton;
    static CrossCompartmentWrapper singletonWithPrototype;
};










template <class Base>
class JS_FRIEND_API(SecurityWrapper) : public Base
{
  public:
    SecurityWrapper(unsigned flags);

    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) MOZ_OVERRIDE;
    virtual bool objectClassIs(JSObject *obj, ESClassValue classValue, JSContext *cx) MOZ_OVERRIDE;
    virtual bool regexp_toShared(JSContext *cx, JSObject *proxy, RegExpGuard *g) MOZ_OVERRIDE;
};

typedef SecurityWrapper<DirectWrapper> SameCompartmentSecurityWrapper;
typedef SecurityWrapper<CrossCompartmentWrapper> CrossCompartmentSecurityWrapper;

class JS_FRIEND_API(DeadObjectProxy) : public BaseProxyHandler
{
  public:
    static int sDeadObjectFamily;

    explicit DeadObjectProxy();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                                       PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                                          PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper, AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp) MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, JSObject *wrapper, AutoIdVector &props) MOZ_OVERRIDE;

    
    virtual bool call(JSContext *cx, JSObject *proxy, unsigned argc, Value *vp);
    virtual bool construct(JSContext *cx, JSObject *proxy, unsigned argc, Value *argv, Value *rval);
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) MOZ_OVERRIDE;
    virtual bool hasInstance(JSContext *cx, JSObject *proxy, const Value *vp, bool *bp);
    virtual bool objectClassIs(JSObject *obj, ESClassValue classValue, JSContext *cx);
    virtual JSString *obj_toString(JSContext *cx, JSObject *proxy);
    virtual JSString *fun_toString(JSContext *cx, JSObject *proxy, unsigned indent);
    virtual bool regexp_toShared(JSContext *cx, JSObject *proxy, RegExpGuard *g);
    virtual bool defaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp);
    virtual bool iteratorNext(JSContext *cx, JSObject *proxy, Value *vp);
    virtual bool getElementIfPresent(JSContext *cx, JSObject *obj, JSObject *receiver,
                                     uint32_t index, Value *vp, bool *present);


    static DeadObjectProxy singleton;
};

extern JSObject *
TransparentObjectWrapper(JSContext *cx, JSObject *obj, JSObject *wrappedProto, JSObject *parent,
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
UnwrapObjectChecked(JSContext *cx, RawObject obj);



JS_FRIEND_API(JSObject *)
UnwrapOneChecked(JSContext *cx, HandleObject obj);

JS_FRIEND_API(bool)
IsCrossCompartmentWrapper(RawObject obj);

JSObject *
NewDeadProxyObject(JSContext *cx, JSObject *parent);

void
NukeCrossCompartmentWrapper(JSObject *wrapper);

bool
RemapWrapper(JSContext *cx, JSObject *wobj, JSObject *newTarget);

JS_FRIEND_API(bool)
RemapAllWrappersForObject(JSContext *cx, JSObject *oldTarget,
                          JSObject *newTarget);



JS_FRIEND_API(bool)
RecomputeWrappers(JSContext *cx, const CompartmentFilter &sourceFilter,
                  const CompartmentFilter &targetFilter);

} 

#endif
