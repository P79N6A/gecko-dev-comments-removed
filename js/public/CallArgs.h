




























#ifndef js_CallArgs_h___
#define js_CallArgs_h___

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

#include "jstypes.h"

#include "js/RootingAPI.h"
#include "js/Value.h"

struct JSContext;
class JSObject;


typedef JSBool
(* JSNative)(JSContext *cx, unsigned argc, JS::Value *vp);

namespace JS {



































class CallReceiver
{
  protected:
#ifdef DEBUG
    mutable bool usedRval_;
    void setUsedRval() const { usedRval_ = true; }
    void clearUsedRval() const { usedRval_ = false; }
#else
    void setUsedRval() const {}
    void clearUsedRval() const {}
#endif

    Value *argv_;

    friend CallReceiver CallReceiverFromVp(Value *vp);
    friend CallReceiver CallReceiverFromArgv(Value *argv);

  public:
    



    JSObject &callee() const {
        MOZ_ASSERT(!usedRval_);
        return argv_[-2].toObject();
    }

    



    HandleValue calleev() const {
        MOZ_ASSERT(!usedRval_);
        return HandleValue::fromMarkedLocation(&argv_[-2]);
    }

    





    HandleValue thisv() const {
        
        
        
        return HandleValue::fromMarkedLocation(&argv_[-1]);
    }

    










    MutableHandleValue rval() const {
        setUsedRval();
        return MutableHandleValue::fromMarkedLocation(&argv_[-2]);
    }

  public:
    
    

    Value *base() const { return argv_ - 2; }

    Value *spAfterCall() const {
        setUsedRval();
        return argv_ - 1;
    }

  public:
    
    
    

    void setCallee(Value aCalleev) const {
        clearUsedRval();
        argv_[-2] = aCalleev;
    }

    void setThis(Value aThisv) const {
        argv_[-1] = aThisv;
    }

    MutableHandleValue mutableThisv() const {
        return MutableHandleValue::fromMarkedLocation(&argv_[-1]);
    }
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

























class CallArgs : public CallReceiver
{
  protected:
    unsigned argc_;

    friend CallArgs CallArgsFromVp(unsigned argc, Value *vp);
    friend CallArgs CallArgsFromSp(unsigned argc, Value *sp);

    static CallArgs create(unsigned argc, Value *argv) {
        CallArgs args;
        args.clearUsedRval();
        args.argv_ = argv;
        args.argc_ = argc;
        return args;
    }

  public:
    
    unsigned length() const { return argc_; }

    
    Value &operator[](unsigned i) const {
        MOZ_ASSERT(i < argc_);
        return argv_[i];
    }

    
    MutableHandleValue handleAt(unsigned i) {
        MOZ_ASSERT(i < argc_);
        return MutableHandleValue::fromMarkedLocation(&argv_[i]);
    }

    
    HandleValue handleAt(unsigned i) const {
        MOZ_ASSERT(i < argc_);
        return HandleValue::fromMarkedLocation(&argv_[i]);
    }

    



    Value get(unsigned i) const {
        return i < length() ? argv_[i] : UndefinedValue();
    }

    



    bool hasDefined(unsigned i) const {
        return i < argc_ && !argv_[i].isUndefined();
    }

  public:
    
    
    

    Value *array() const { return argv_; }
    Value *end() const { return argv_ + argc_; }
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










extern JS_PUBLIC_API(JS::Value)
JS_ComputeThis(JSContext *cx, JS::Value *vp);








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
