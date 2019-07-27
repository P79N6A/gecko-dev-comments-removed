





#ifndef js_Proxy_h
#define js_Proxy_h

#include "mozilla/Maybe.h"

#include "jsfriendapi.h"

#include "js/CallNonGenericMethod.h"
#include "js/Class.h"

namespace js {

using JS::AutoIdVector;
using JS::CallArgs;
using JS::Handle;
using JS::HandleId;
using JS::HandleObject;
using JS::HandleValue;
using JS::IsAcceptableThis;
using JS::MutableHandle;
using JS::MutableHandleObject;
using JS::MutableHandleValue;
using JS::NativeImpl;
using JS::ObjectOpResult;
using JS::PrivateValue;
using JS::Value;

class RegExpGuard;
class JS_FRIEND_API(Wrapper);





















































































































class JS_FRIEND_API(BaseProxyHandler)
{
    







    const void* mFamily;

    

















    bool mHasPrototype;

    





    bool mHasSecurityPolicy;

  public:
    explicit MOZ_CONSTEXPR BaseProxyHandler(const void* aFamily, bool aHasPrototype = false,
                                            bool aHasSecurityPolicy = false)
      : mFamily(aFamily),
        mHasPrototype(aHasPrototype),
        mHasSecurityPolicy(aHasSecurityPolicy)
    { }

    bool hasPrototype() const {
        return mHasPrototype;
    }

    bool hasSecurityPolicy() const {
        return mHasSecurityPolicy;
    }

    inline const void* family() const {
        return mFamily;
    }
    static size_t offsetOfFamily() {
        return offsetof(BaseProxyHandler, mFamily);
    }

    virtual bool finalizeInBackground(Value priv) const {
        



        return true;
    }

    














    typedef uint32_t Action;
    enum {
        NONE      = 0x00,
        GET       = 0x01,
        SET       = 0x02,
        CALL      = 0x04,
        ENUMERATE = 0x08,
        GET_PROPERTY_DESCRIPTOR = 0x10
    };

    virtual bool enter(JSContext* cx, HandleObject wrapper, HandleId id, Action act,
                       bool* bp) const;

    
    virtual bool getOwnPropertyDescriptor(JSContext* cx, HandleObject proxy, HandleId id,
                                          MutableHandle<JSPropertyDescriptor> desc) const = 0;
    virtual bool defineProperty(JSContext* cx, HandleObject proxy, HandleId id,
                                Handle<JSPropertyDescriptor> desc,
                                ObjectOpResult& result) const = 0;
    virtual bool ownPropertyKeys(JSContext* cx, HandleObject proxy,
                                 AutoIdVector& props) const = 0;
    virtual bool delete_(JSContext* cx, HandleObject proxy, HandleId id,
                         ObjectOpResult& result) const = 0;

    





    virtual bool enumerate(JSContext* cx, HandleObject proxy, MutableHandleObject objp) const = 0;

    





    virtual bool getPrototype(JSContext* cx, HandleObject proxy, MutableHandleObject protop) const;
    virtual bool setPrototype(JSContext* cx, HandleObject proxy, HandleObject proto,
                              ObjectOpResult& result) const;

    
    virtual bool setImmutablePrototype(JSContext* cx, HandleObject proxy, bool* succeeded) const;

    virtual bool preventExtensions(JSContext* cx, HandleObject proxy,
                                   ObjectOpResult& result) const = 0;
    virtual bool isExtensible(JSContext* cx, HandleObject proxy, bool* extensible) const = 0;

    






    virtual bool has(JSContext* cx, HandleObject proxy, HandleId id, bool* bp) const;
    virtual bool get(JSContext* cx, HandleObject proxy, HandleObject receiver,
                     HandleId id, MutableHandleValue vp) const;
    virtual bool set(JSContext* cx, HandleObject proxy, HandleId id, HandleValue v,
                     HandleValue receiver, ObjectOpResult& result) const;

    











    virtual bool call(JSContext* cx, HandleObject proxy, const CallArgs& args) const;
    virtual bool construct(JSContext* cx, HandleObject proxy, const CallArgs& args) const;

    
    virtual bool getPropertyDescriptor(JSContext* cx, HandleObject proxy, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc) const = 0;
    virtual bool hasOwn(JSContext* cx, HandleObject proxy, HandleId id, bool* bp) const;
    virtual bool getOwnEnumerablePropertyKeys(JSContext* cx, HandleObject proxy,
                                              AutoIdVector& props) const;
    virtual bool nativeCall(JSContext* cx, IsAcceptableThis test, NativeImpl impl, CallArgs args) const;
    virtual bool hasInstance(JSContext* cx, HandleObject proxy, MutableHandleValue v, bool* bp) const;
    virtual bool objectClassIs(HandleObject obj, ESClassValue classValue, JSContext* cx) const;
    virtual const char* className(JSContext* cx, HandleObject proxy) const;
    virtual JSString* fun_toString(JSContext* cx, HandleObject proxy, unsigned indent) const;
    virtual bool regexp_toShared(JSContext* cx, HandleObject proxy, RegExpGuard* g) const;
    virtual bool boxedValue_unbox(JSContext* cx, HandleObject proxy, MutableHandleValue vp) const;
    virtual bool defaultValue(JSContext* cx, HandleObject obj, JSType hint, MutableHandleValue vp) const;
    virtual void trace(JSTracer* trc, JSObject* proxy) const;
    virtual void finalize(JSFreeOp* fop, JSObject* proxy) const;
    virtual void objectMoved(JSObject* proxy, const JSObject* old) const;

    
    
    
    
    virtual bool isCallable(JSObject* obj) const;
    virtual bool isConstructor(JSObject* obj) const;

    
    
    virtual bool watch(JSContext* cx, JS::HandleObject proxy, JS::HandleId id,
                       JS::HandleObject callable) const;
    virtual bool unwatch(JSContext* cx, JS::HandleObject proxy, JS::HandleId id) const;

    virtual bool getElements(JSContext* cx, HandleObject proxy, uint32_t begin, uint32_t end,
                             ElementAdder* adder) const;

    
    virtual JSObject* weakmapKeyDelegate(JSObject* proxy) const;
    virtual bool isScripted() const { return false; }
};











class JS_FRIEND_API(DirectProxyHandler) : public BaseProxyHandler
{
  public:
    explicit MOZ_CONSTEXPR DirectProxyHandler(const void* aFamily, bool aHasPrototype = false,
                                              bool aHasSecurityPolicy = false)
      : BaseProxyHandler(aFamily, aHasPrototype, aHasSecurityPolicy)
    { }

    
    virtual bool getOwnPropertyDescriptor(JSContext* cx, HandleObject proxy, HandleId id,
                                          MutableHandle<JSPropertyDescriptor> desc) const override;
    virtual bool defineProperty(JSContext* cx, HandleObject proxy, HandleId id,
                                Handle<JSPropertyDescriptor> desc,
                                ObjectOpResult& result) const override;
    virtual bool ownPropertyKeys(JSContext* cx, HandleObject proxy,
                                 AutoIdVector& props) const override;
    virtual bool delete_(JSContext* cx, HandleObject proxy, HandleId id,
                         ObjectOpResult& result) const override;
    virtual bool enumerate(JSContext* cx, HandleObject proxy,
                           MutableHandleObject objp) const override;
    virtual bool getPrototype(JSContext* cx, HandleObject proxy,
                              MutableHandleObject protop) const override;
    virtual bool setPrototype(JSContext* cx, HandleObject proxy, HandleObject proto,
                              ObjectOpResult& result) const override;
    virtual bool setImmutablePrototype(JSContext* cx, HandleObject proxy,
                                       bool* succeeded) const override;
    virtual bool preventExtensions(JSContext* cx, HandleObject proxy,
                                   ObjectOpResult& result) const override;
    virtual bool isExtensible(JSContext* cx, HandleObject proxy, bool* extensible) const override;
    virtual bool has(JSContext* cx, HandleObject proxy, HandleId id,
                     bool* bp) const override;
    virtual bool get(JSContext* cx, HandleObject proxy, HandleObject receiver,
                     HandleId id, MutableHandleValue vp) const override;
    virtual bool set(JSContext* cx, HandleObject proxy, HandleId id, HandleValue v,
                     HandleValue receiver, ObjectOpResult& result) const override;
    virtual bool call(JSContext* cx, HandleObject proxy, const CallArgs& args) const override;
    virtual bool construct(JSContext* cx, HandleObject proxy, const CallArgs& args) const override;

    
    virtual bool getPropertyDescriptor(JSContext* cx, HandleObject proxy, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc) const override;
    virtual bool hasOwn(JSContext* cx, HandleObject proxy, HandleId id,
                        bool* bp) const override;
    virtual bool getOwnEnumerablePropertyKeys(JSContext* cx, HandleObject proxy,
                                              AutoIdVector& props) const override;
    virtual bool nativeCall(JSContext* cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) const override;
    virtual bool hasInstance(JSContext* cx, HandleObject proxy, MutableHandleValue v,
                             bool* bp) const override;
    virtual bool objectClassIs(HandleObject obj, ESClassValue classValue,
                               JSContext* cx) const override;
    virtual const char* className(JSContext* cx, HandleObject proxy) const override;
    virtual JSString* fun_toString(JSContext* cx, HandleObject proxy,
                                   unsigned indent) const override;
    virtual bool regexp_toShared(JSContext* cx, HandleObject proxy,
                                 RegExpGuard* g) const override;
    virtual bool boxedValue_unbox(JSContext* cx, HandleObject proxy, MutableHandleValue vp) const override;
    virtual bool isCallable(JSObject* obj) const override;
    virtual JSObject* weakmapKeyDelegate(JSObject* proxy) const override;
};

extern JS_FRIEND_DATA(const js::Class* const) ProxyClassPtr;

inline bool IsProxy(const JSObject* obj)
{
    return GetObjectClass(obj)->isProxy();
}

const uint32_t PROXY_EXTRA_SLOTS = 2;






struct ProxyValueArray
{
    Value privateSlot;
    Value extraSlots[PROXY_EXTRA_SLOTS];

    ProxyValueArray()
      : privateSlot(JS::UndefinedValue())
    {
        for (size_t i = 0; i < PROXY_EXTRA_SLOTS; i++)
            extraSlots[i] = JS::UndefinedValue();
    }
};









struct ProxyDataLayout
{
    ProxyValueArray* values;
    const BaseProxyHandler* handler;
};

const uint32_t ProxyDataOffset = 2 * sizeof(void*);


inline ProxyDataLayout*
GetProxyDataLayout(JSObject* obj)
{
    MOZ_ASSERT(IsProxy(obj));
    return reinterpret_cast<ProxyDataLayout*>(reinterpret_cast<uint8_t*>(obj) + ProxyDataOffset);
}

inline const BaseProxyHandler*
GetProxyHandler(JSObject* obj)
{
    return GetProxyDataLayout(obj)->handler;
}

inline const Value&
GetProxyPrivate(JSObject* obj)
{
    return GetProxyDataLayout(obj)->values->privateSlot;
}

inline JSObject*
GetProxyTargetObject(JSObject* obj)
{
    return GetProxyPrivate(obj).toObjectOrNull();
}

inline const Value&
GetProxyExtra(JSObject* obj, size_t n)
{
    MOZ_ASSERT(n < PROXY_EXTRA_SLOTS);
    return GetProxyDataLayout(obj)->values->extraSlots[n];
}

inline void
SetProxyHandler(JSObject* obj, const BaseProxyHandler* handler)
{
    GetProxyDataLayout(obj)->handler = handler;
}

JS_FRIEND_API(void)
SetValueInProxy(Value* slot, const Value& value);

inline void
SetProxyExtra(JSObject* obj, size_t n, const Value& extra)
{
    MOZ_ASSERT(n < PROXY_EXTRA_SLOTS);
    Value* vp = &GetProxyDataLayout(obj)->values->extraSlots[n];

    
    if (vp->isMarkable() || extra.isMarkable())
        SetValueInProxy(vp, extra);
    else
        *vp = extra;
}

inline bool
IsScriptedProxy(JSObject* obj)
{
    return IsProxy(obj) && GetProxyHandler(obj)->isScripted();
}

inline const Value&
GetReservedOrProxyPrivateSlot(JSObject* obj, size_t slot)
{
    MOZ_ASSERT(slot == 0);
    MOZ_ASSERT(slot < JSCLASS_RESERVED_SLOTS(GetObjectClass(obj)) || IsProxy(obj));
    return reinterpret_cast<const shadow::Object*>(obj)->slotRef(slot);
}

inline void
SetReservedOrProxyPrivateSlot(JSObject* obj, size_t slot, const Value& value)
{
    MOZ_ASSERT(slot == 0);
    MOZ_ASSERT(slot < JSCLASS_RESERVED_SLOTS(GetObjectClass(obj)) || IsProxy(obj));
    shadow::Object* sobj = reinterpret_cast<shadow::Object*>(obj);
    if (sobj->slotRef(slot).isMarkable() || value.isMarkable())
        SetReservedOrProxyPrivateSlotWithBarrier(obj, slot, value);
    else
        sobj->slotRef(slot) = value;
}

class MOZ_STACK_CLASS ProxyOptions {
  protected:
    
    explicit ProxyOptions(bool singletonArg, bool lazyProtoArg = false)
      : singleton_(singletonArg),
        lazyProto_(lazyProtoArg),
        clasp_(ProxyClassPtr)
    {}

  public:
    ProxyOptions() : singleton_(false),
                     lazyProto_(false),
                     clasp_(ProxyClassPtr)
    {}

    bool singleton() const { return singleton_; }
    ProxyOptions& setSingleton(bool flag) {
        singleton_ = flag;
        return *this;
    }

    bool lazyProto() const { return lazyProto_; }
    ProxyOptions& setLazyProto(bool flag) {
        lazyProto_ = flag;
        return *this;
    }

    const Class* clasp() const {
        return clasp_;
    }
    ProxyOptions& setClass(const Class* claspArg) {
        clasp_ = claspArg;
        return *this;
    }

  private:
    bool singleton_;
    bool lazyProto_;
    const Class* clasp_;
};

JS_FRIEND_API(JSObject*)
NewProxyObject(JSContext* cx, const BaseProxyHandler* handler, HandleValue priv,
               JSObject* proto, const ProxyOptions& options = ProxyOptions());

JSObject*
RenewProxyObject(JSContext* cx, JSObject* obj, BaseProxyHandler* handler, Value priv);

class JS_FRIEND_API(AutoEnterPolicy)
{
  public:
    typedef BaseProxyHandler::Action Action;
    AutoEnterPolicy(JSContext* cx, const BaseProxyHandler* handler,
                    HandleObject wrapper, HandleId id, Action act, bool mayThrow)
#ifdef JS_DEBUG
        : context(nullptr)
#endif
    {
        allow = handler->hasSecurityPolicy() ? handler->enter(cx, wrapper, id, act, &rv)
                                             : true;
        recordEnter(cx, wrapper, id, act);
        
        
        
        
        
        if (!allow && !rv && mayThrow)
            reportErrorIfExceptionIsNotPending(cx, id);
    }

    virtual ~AutoEnterPolicy() { recordLeave(); }
    inline bool allowed() { return allow; }
    inline bool returnValue() { MOZ_ASSERT(!allowed()); return rv; }

  protected:
    
    AutoEnterPolicy()
#ifdef JS_DEBUG
        : context(nullptr)
        , enteredAction(BaseProxyHandler::NONE)
#endif
        {}
    void reportErrorIfExceptionIsNotPending(JSContext* cx, jsid id);
    bool allow;
    bool rv;

#ifdef JS_DEBUG
    JSContext* context;
    mozilla::Maybe<HandleObject> enteredProxy;
    mozilla::Maybe<HandleId> enteredId;
    Action                   enteredAction;

    
    
    
    AutoEnterPolicy* prev;
    void recordEnter(JSContext* cx, HandleObject proxy, HandleId id, Action act);
    void recordLeave();

    friend JS_FRIEND_API(void) assertEnteredPolicy(JSContext* cx, JSObject* proxy, jsid id, Action act);
#else
    inline void recordEnter(JSContext* cx, JSObject* proxy, jsid id, Action act) {}
    inline void recordLeave() {}
#endif

};

#ifdef JS_DEBUG
class JS_FRIEND_API(AutoWaivePolicy) : public AutoEnterPolicy {
public:
    AutoWaivePolicy(JSContext* cx, HandleObject proxy, HandleId id,
                    BaseProxyHandler::Action act)
    {
        allow = true;
        recordEnter(cx, proxy, id, act);
    }
};
#else
class JS_FRIEND_API(AutoWaivePolicy) {
  public:
    AutoWaivePolicy(JSContext* cx, HandleObject proxy, HandleId id,
                    BaseProxyHandler::Action act)
    {}
};
#endif

#ifdef JS_DEBUG
extern JS_FRIEND_API(void)
assertEnteredPolicy(JSContext* cx, JSObject* obj, jsid id,
                    BaseProxyHandler::Action act);
#else
inline void assertEnteredPolicy(JSContext* cx, JSObject* obj, jsid id,
                                BaseProxyHandler::Action act)
{}
#endif

extern JS_FRIEND_API(JSObject*)
InitProxyClass(JSContext* cx, JS::HandleObject obj);

} 

#endif 
