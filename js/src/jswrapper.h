





#ifndef jswrapper_h
#define jswrapper_h

#include "mozilla/Attributes.h"

#include "js/Proxy.h"

namespace js {







class MOZ_STACK_CLASS WrapperOptions : public ProxyOptions {
  public:
    WrapperOptions() : ProxyOptions(false),
                       proto_()
    {}

    explicit WrapperOptions(JSContext* cx) : ProxyOptions(false),
                                             proto_()
    {
        proto_.emplace(cx);
    }

    inline JSObject* proto() const;
    WrapperOptions& setProto(JSObject* protoArg) {
        MOZ_ASSERT(proto_);
        *proto_ = protoArg;
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

    virtual bool defaultValue(JSContext* cx, HandleObject obj, JSType hint,
                              MutableHandleValue vp) const override;

    static JSObject* New(JSContext* cx, JSObject* obj, const Wrapper* handler,
                         const WrapperOptions& options = WrapperOptions());

    static JSObject* Renew(JSContext* cx, JSObject* existing, JSObject* obj, const Wrapper* handler);

    static const Wrapper* wrapperHandler(JSObject* wrapper);

    static JSObject* wrappedObject(JSObject* wrapper);

    unsigned flags() const {
        return mFlags;
    }

    explicit MOZ_CONSTEXPR Wrapper(unsigned aFlags, bool aHasPrototype = false,
                                   bool aHasSecurityPolicy = false)
      : DirectProxyHandler(&family, aHasPrototype, aHasSecurityPolicy),
        mFlags(aFlags)
    { }

    virtual bool finalizeInBackground(Value priv) const override;
    virtual bool isConstructor(JSObject* obj) const override;

    static const char family;
    static const Wrapper singleton;
    static const Wrapper singletonWithPrototype;

    static JSObject* defaultProto;
};

inline JSObject*
WrapperOptions::proto() const
{
    return proto_ ? *proto_ : Wrapper::defaultProto;
}


class JS_FRIEND_API(CrossCompartmentWrapper) : public Wrapper
{
  public:
    explicit MOZ_CONSTEXPR CrossCompartmentWrapper(unsigned aFlags, bool aHasPrototype = false,
                                                   bool aHasSecurityPolicy = false)
      : Wrapper(CROSS_COMPARTMENT | aFlags, aHasPrototype, aHasSecurityPolicy)
    { }

    
    virtual bool getOwnPropertyDescriptor(JSContext* cx, HandleObject wrapper, HandleId id,
                                          MutableHandle<JSPropertyDescriptor> desc) const override;
    virtual bool defineProperty(JSContext* cx, HandleObject wrapper, HandleId id,
                                Handle<JSPropertyDescriptor> desc,
                                ObjectOpResult& result) const override;
    virtual bool ownPropertyKeys(JSContext* cx, HandleObject wrapper,
                                 AutoIdVector& props) const override;
    virtual bool delete_(JSContext* cx, HandleObject wrapper, HandleId id,
                         ObjectOpResult& result) const override;
    virtual bool enumerate(JSContext* cx, HandleObject wrapper, MutableHandleObject objp) const override;
    virtual bool getPrototype(JSContext* cx, HandleObject proxy,
                              MutableHandleObject protop) const override;
    virtual bool setPrototype(JSContext* cx, HandleObject proxy, HandleObject proto,
                              ObjectOpResult& result) const override;

    virtual bool setImmutablePrototype(JSContext* cx, HandleObject proxy,
                                       bool* succeeded) const override;
    virtual bool preventExtensions(JSContext* cx, HandleObject wrapper,
                                   ObjectOpResult& result) const override;
    virtual bool isExtensible(JSContext* cx, HandleObject wrapper, bool* extensible) const override;
    virtual bool has(JSContext* cx, HandleObject wrapper, HandleId id, bool* bp) const override;
    virtual bool get(JSContext* cx, HandleObject wrapper, HandleObject receiver,
                     HandleId id, MutableHandleValue vp) const override;
    virtual bool set(JSContext* cx, HandleObject wrapper, HandleId id, HandleValue v,
                     HandleValue receiver, ObjectOpResult& result) const override;
    virtual bool call(JSContext* cx, HandleObject wrapper, const CallArgs& args) const override;
    virtual bool construct(JSContext* cx, HandleObject wrapper, const CallArgs& args) const override;

    
    virtual bool getPropertyDescriptor(JSContext* cx, HandleObject wrapper, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc) const override;
    virtual bool hasOwn(JSContext* cx, HandleObject wrapper, HandleId id, bool* bp) const override;
    virtual bool getOwnEnumerablePropertyKeys(JSContext* cx, HandleObject wrapper,
                                              AutoIdVector& props) const override;
    virtual bool nativeCall(JSContext* cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) const override;
    virtual bool hasInstance(JSContext* cx, HandleObject wrapper, MutableHandleValue v,
                             bool* bp) const override;
    virtual const char* className(JSContext* cx, HandleObject proxy) const override;
    virtual JSString* fun_toString(JSContext* cx, HandleObject wrapper,
                                   unsigned indent) const override;
    virtual bool regexp_toShared(JSContext* cx, HandleObject proxy, RegExpGuard* g) const override;
    virtual bool boxedValue_unbox(JSContext* cx, HandleObject proxy, MutableHandleValue vp) const override;
    virtual bool defaultValue(JSContext* cx, HandleObject wrapper, JSType hint,
                              MutableHandleValue vp) const override;

    static const CrossCompartmentWrapper singleton;
    static const CrossCompartmentWrapper singletonWithPrototype;
};










template <class Base>
class JS_FRIEND_API(SecurityWrapper) : public Base
{
  public:
    explicit MOZ_CONSTEXPR SecurityWrapper(unsigned flags, bool hasPrototype = false)
      : Base(flags, hasPrototype,  true)
    { }

    virtual bool enter(JSContext* cx, HandleObject wrapper, HandleId id, Wrapper::Action act,
                       bool* bp) const override;

    virtual bool defineProperty(JSContext* cx, HandleObject wrapper, HandleId id,
                                Handle<JSPropertyDescriptor> desc,
                                ObjectOpResult& result) const override;
    virtual bool isExtensible(JSContext* cx, HandleObject wrapper, bool* extensible) const override;
    virtual bool preventExtensions(JSContext* cx, HandleObject wrapper,
                                   ObjectOpResult& result) const override;
    virtual bool setPrototype(JSContext* cx, HandleObject proxy, HandleObject proto,
                              ObjectOpResult& result) const override;
    virtual bool setImmutablePrototype(JSContext* cx, HandleObject proxy, bool* succeeded) const override;

    virtual bool nativeCall(JSContext* cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) const override;
    virtual bool objectClassIs(HandleObject obj, ESClassValue classValue,
                               JSContext* cx) const override;
    virtual bool regexp_toShared(JSContext* cx, HandleObject proxy, RegExpGuard* g) const override;
    virtual bool boxedValue_unbox(JSContext* cx, HandleObject proxy, MutableHandleValue vp) const override;
    virtual bool defaultValue(JSContext* cx, HandleObject wrapper, JSType hint,
                              MutableHandleValue vp) const override;

    
    

    virtual bool watch(JSContext* cx, JS::HandleObject proxy, JS::HandleId id,
                       JS::HandleObject callable) const override;
    virtual bool unwatch(JSContext* cx, JS::HandleObject proxy, JS::HandleId id) const override;

    



    typedef Base Permissive;
    typedef SecurityWrapper<Base> Restrictive;
};

typedef SecurityWrapper<Wrapper> SameCompartmentSecurityWrapper;
typedef SecurityWrapper<CrossCompartmentWrapper> CrossCompartmentSecurityWrapper;

extern JSObject*
TransparentObjectWrapper(JSContext* cx, HandleObject existing, HandleObject obj);

inline bool
IsWrapper(JSObject* obj)
{
    return IsProxy(obj) && GetProxyHandler(obj)->family() == &Wrapper::family;
}





JS_FRIEND_API(JSObject*)
UncheckedUnwrap(JSObject* obj, bool stopAtOuter = true, unsigned* flagsp = nullptr);





JS_FRIEND_API(JSObject*)
CheckedUnwrap(JSObject* obj, bool stopAtOuter = true);



JS_FRIEND_API(JSObject*)
UnwrapOneChecked(JSObject* obj, bool stopAtOuter = true);

JS_FRIEND_API(bool)
IsCrossCompartmentWrapper(JSObject* obj);

void
NukeCrossCompartmentWrapper(JSContext* cx, JSObject* wrapper);

bool
RemapWrapper(JSContext* cx, JSObject* wobj, JSObject* newTarget);

JS_FRIEND_API(bool)
RemapAllWrappersForObject(JSContext* cx, JSObject* oldTarget,
                          JSObject* newTarget);



JS_FRIEND_API(bool)
RecomputeWrappers(JSContext* cx, const CompartmentFilter& sourceFilter,
                  const CompartmentFilter& targetFilter);

} 

#endif 
