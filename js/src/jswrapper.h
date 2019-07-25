








































#ifndef jswrapper_h___
#define jswrapper_h___

#include "mozilla/Attributes.h"

#include "jsapi.h"
#include "jsproxy.h"

namespace js {


class JS_FRIEND_API(Wrapper) : public ProxyHandler
{
    uintN mFlags;
  public:
    uintN flags() const { return mFlags; }

    explicit Wrapper(uintN flags);

    typedef enum { PermitObjectAccess, PermitPropertyAccess, DenyAccess } Permission;

    virtual ~Wrapper();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                                       PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                                          PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper, AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp) MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, JSObject *wrapper, AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool fix(JSContext *cx, JSObject *wrapper, Value *vp) MOZ_OVERRIDE;

    
    virtual bool has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp) MOZ_OVERRIDE;
    virtual bool hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, Value *vp) MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, bool strict,
                     Value *vp) MOZ_OVERRIDE;
    virtual bool keys(JSContext *cx, JSObject *wrapper, AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool iterate(JSContext *cx, JSObject *wrapper, uintN flags, Value *vp) MOZ_OVERRIDE;

    
    virtual bool call(JSContext *cx, JSObject *wrapper, uintN argc, Value *vp) MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, JSObject *wrapper, uintN argc, Value *argv, Value *rval) MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, JSObject *wrapper, Class *clasp, Native native, CallArgs args) MOZ_OVERRIDE;
    virtual bool hasInstance(JSContext *cx, JSObject *wrapper, const Value *vp, bool *bp) MOZ_OVERRIDE;
    virtual JSType typeOf(JSContext *cx, JSObject *proxy) MOZ_OVERRIDE;
    virtual bool objectClassIs(JSObject *obj, ESClassValue classValue, JSContext *cx) MOZ_OVERRIDE;
    virtual JSString *obj_toString(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE;
    virtual JSString *fun_toString(JSContext *cx, JSObject *wrapper, uintN indent) MOZ_OVERRIDE;
    virtual bool defaultValue(JSContext *cx, JSObject *wrapper, JSType hint, Value *vp) MOZ_OVERRIDE;

    virtual void trace(JSTracer *trc, JSObject *wrapper) MOZ_OVERRIDE;

    
    enum Action { GET, SET, CALL };
    virtual bool enter(JSContext *cx, JSObject *wrapper, jsid id, Action act, bool *bp);
    virtual void leave(JSContext *cx, JSObject *wrapper);

    static Wrapper singleton;

    static JSObject *New(JSContext *cx, JSObject *obj, JSObject *proto, JSObject *parent,
                         Wrapper *handler);

    static JSObject *wrappedObject(const JSObject *wrapper);
    static Wrapper *wrapperHandler(const JSObject *wrapper);

    enum {
        CROSS_COMPARTMENT = 1 << 0,
        LAST_USED_FLAG = CROSS_COMPARTMENT
    };

    static void *getWrapperFamily();
};


class JS_FRIEND_API(CrossCompartmentWrapper) : public Wrapper
{
  public:
    CrossCompartmentWrapper(uintN flags);

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
    virtual bool iterate(JSContext *cx, JSObject *wrapper, uintN flags, Value *vp) MOZ_OVERRIDE;

    
    virtual bool call(JSContext *cx, JSObject *wrapper, uintN argc, Value *vp) MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, JSObject *wrapper, uintN argc, Value *argv, Value *rval) MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, JSObject *wrapper, Class *clasp, Native native, CallArgs args) MOZ_OVERRIDE;
    virtual bool hasInstance(JSContext *cx, JSObject *wrapper, const Value *vp, bool *bp) MOZ_OVERRIDE;
    virtual JSString *obj_toString(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE;
    virtual JSString *fun_toString(JSContext *cx, JSObject *wrapper, uintN indent) MOZ_OVERRIDE;
    virtual bool defaultValue(JSContext *cx, JSObject *wrapper, JSType hint, Value *vp) MOZ_OVERRIDE;

    virtual void trace(JSTracer *trc, JSObject *wrapper) MOZ_OVERRIDE;

    static CrossCompartmentWrapper singleton;
};










template <class Base>
class JS_FRIEND_API(SecurityWrapper) : public Base
{
  public:
    SecurityWrapper(uintN flags);

    virtual bool nativeCall(JSContext *cx, JSObject *wrapper, Class *clasp, Native native, CallArgs args) MOZ_OVERRIDE;
    virtual bool objectClassIs(JSObject *obj, ESClassValue classValue, JSContext *cx) MOZ_OVERRIDE;
};

typedef SecurityWrapper<Wrapper> SameCompartmentSecurityWrapper;
typedef SecurityWrapper<CrossCompartmentWrapper> CrossCompartmentSecurityWrapper;





class JS_FRIEND_API(ForceFrame)
{
  public:
    JSContext * const context;
    JSObject * const target;
  private:
    DummyFrameGuard *frame;

  public:
    ForceFrame(JSContext *cx, JSObject *target);
    ~ForceFrame();
    bool enter();
};

extern JSObject *
TransparentObjectWrapper(JSContext *cx, JSObject *obj, JSObject *wrappedProto, JSObject *parent,
                         uintN flags);

JS_FRIEND_API(bool) IsWrapper(const JSObject *obj);
JS_FRIEND_API(JSObject *) UnwrapObject(JSObject *obj, uintN *flagsp = NULL);
bool IsCrossCompartmentWrapper(const JSObject *obj);

} 

#endif
