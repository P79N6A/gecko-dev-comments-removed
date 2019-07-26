



























#ifndef js_CallArgs_h
#define js_CallArgs_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/RangedPtr.h"
#include "mozilla/TypeTraits.h"

#include "jstypes.h"

#include "js/RootingAPI.h"
#include "js/Value.h"


typedef bool
(* JSNative)(JSContext *cx, unsigned argc, JS::Value *vp);


typedef bool
(* JSParallelNative)(js::ForkJoinSlice *slice, unsigned argc, JS::Value *vp);





typedef bool
(* JSThreadSafeNative)(js::ThreadSafeContext *cx, unsigned argc, JS::Value *vp);





template <JSThreadSafeNative threadSafeNative>
inline bool
JSNativeThreadSafeWrapper(JSContext *cx, unsigned argc, JS::Value *vp);

template <JSThreadSafeNative threadSafeNative>
inline bool
JSParallelNativeThreadSafeWrapper(js::ForkJoinSlice *slice, unsigned argc, JS::Value *vp);










extern JS_PUBLIC_API(JS::Value)
JS_ComputeThis(JSContext *cx, JS::Value *vp);

namespace JS {

extern JS_PUBLIC_DATA(const HandleValue) UndefinedHandleValue;




































namespace detail {

#ifdef DEBUG
extern JS_PUBLIC_API(void)
CheckIsValidConstructible(Value v);
#endif

enum UsedRval { IncludeUsedRval, NoUsedRval };

template<UsedRval WantUsedRval>
class MOZ_STACK_CLASS UsedRvalBase;

template<>
class MOZ_STACK_CLASS UsedRvalBase<IncludeUsedRval>
{
  protected:
    mutable bool usedRval_;
    void setUsedRval() const { usedRval_ = true; }
    void clearUsedRval() const { usedRval_ = false; }
};

template<>
class MOZ_STACK_CLASS UsedRvalBase<NoUsedRval>
{
  protected:
    void setUsedRval() const {}
    void clearUsedRval() const {}
};

template<UsedRval WantUsedRval>
class MOZ_STACK_CLASS CallReceiverBase : public UsedRvalBase<
#ifdef DEBUG
        WantUsedRval
#else
        NoUsedRval
#endif
    >
{
  protected:
    Value *argv_;

  public:
    



    JSObject &callee() const {
        MOZ_ASSERT(!this->usedRval_);
        return argv_[-2].toObject();
    }

    



    HandleValue calleev() const {
        MOZ_ASSERT(!this->usedRval_);
        return HandleValue::fromMarkedLocation(&argv_[-2]);
    }

    





    HandleValue thisv() const {
        
        
        
        return HandleValue::fromMarkedLocation(&argv_[-1]);
    }

    Value computeThis(JSContext *cx) const {
        if (thisv().isObject())
            return thisv();

        return JS_ComputeThis(cx, base());
    }

    bool isConstructing() const {
#ifdef DEBUG
        if (this->usedRval_)
            CheckIsValidConstructible(calleev());
#endif
        return argv_[-1].isMagic();
    }

    










    MutableHandleValue rval() const {
        this->setUsedRval();
        return MutableHandleValue::fromMarkedLocation(&argv_[-2]);
    }

  public:
    
    

    Value *base() const { return argv_ - 2; }

    Value *spAfterCall() const {
        this->setUsedRval();
        return argv_ - 1;
    }

  public:
    
    
    

    void setCallee(Value aCalleev) const {
        this->clearUsedRval();
        argv_[-2] = aCalleev;
    }

    void setThis(Value aThisv) const {
        argv_[-1] = aThisv;
    }

    MutableHandleValue mutableThisv() const {
        return MutableHandleValue::fromMarkedLocation(&argv_[-1]);
    }
};

} 

class MOZ_STACK_CLASS CallReceiver : public detail::CallReceiverBase<detail::IncludeUsedRval>
{
  private:
    friend CallReceiver CallReceiverFromVp(Value *vp);
    friend CallReceiver CallReceiverFromArgv(Value *argv);
};

MOZ_ALWAYS_INLINE CallReceiver
CallReceiverFromArgv(Value *argv)
{
    CallReceiver receiver;
    receiver.clearUsedRval();
    receiver.argv_ = argv;
    return receiver;
}

MOZ_ALWAYS_INLINE CallReceiver
CallReceiverFromVp(Value *vp)
{
    return CallReceiverFromArgv(vp + 2);
}

























namespace detail {

template<UsedRval WantUsedRval>
class MOZ_STACK_CLASS CallArgsBase :
        public mozilla::Conditional<WantUsedRval == detail::IncludeUsedRval,
                                    CallReceiver,
                                    CallReceiverBase<NoUsedRval> >::Type
{
  protected:
    unsigned argc_;

  public:
    
    unsigned length() const { return argc_; }

    
    MutableHandleValue operator[](unsigned i) const {
        MOZ_ASSERT(i < argc_);
        return MutableHandleValue::fromMarkedLocation(&this->argv_[i]);
    }

    



    HandleValue get(unsigned i) const {
        return i < length()
               ? HandleValue::fromMarkedLocation(&this->argv_[i])
               : UndefinedHandleValue;
    }

    



    bool hasDefined(unsigned i) const {
        return i < argc_ && !this->argv_[i].isUndefined();
    }

    



    mozilla::RangedPtr<const Value> thisAndArgs() const {
        return mozilla::RangedPtr<const Value>(this->argv_ - 1, argc_ + 1);
    }

  public:
    
    
    

    Value *array() const { return this->argv_; }
    Value *end() const { return this->argv_ + argc_; }
};

} 

class MOZ_STACK_CLASS CallArgs : public detail::CallArgsBase<detail::IncludeUsedRval>
{
  private:
    friend CallArgs CallArgsFromVp(unsigned argc, Value *vp);
    friend CallArgs CallArgsFromSp(unsigned argc, Value *sp);

    static CallArgs create(unsigned argc, Value *argv) {
        CallArgs args;
        args.clearUsedRval();
        args.argv_ = argv;
        args.argc_ = argc;
        return args;
    }

};

MOZ_ALWAYS_INLINE CallArgs
CallArgsFromVp(unsigned argc, Value *vp)
{
    return CallArgs::create(argc, vp + 2);
}




MOZ_ALWAYS_INLINE CallArgs
CallArgsFromSp(unsigned argc, Value *sp)
{
    return CallArgs::create(argc, sp - argc);
}

} 








#define JS_CALLEE(cx,vp)        ((vp)[0])
#define JS_THIS_OBJECT(cx,vp)   (JSVAL_TO_OBJECT(JS_THIS(cx,vp)))
#define JS_ARGV(cx,vp)          ((vp) + 2)
#define JS_RVAL(cx,vp)          (*(vp))
#define JS_SET_RVAL(cx,vp,v)    (*(vp) = (v))





MOZ_ALWAYS_INLINE JS::Value
JS_THIS(JSContext *cx, JS::Value *vp)
{
    return JSVAL_IS_PRIMITIVE(vp[1]) ? JS_ComputeThis(cx, vp) : vp[1];
}














#define JS_THIS_VALUE(cx,vp)    ((vp)[1])

#endif 
