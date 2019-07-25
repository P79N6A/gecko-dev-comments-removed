







































#ifndef CallObject_h___
#define CallObject_h___

namespace js {

class CallObject : public ::JSObject
{
    














    static const uint32 CALLEE_SLOT = 1;
    static const uint32 ARGUMENTS_SLOT = 2;

  public:
    
    static CallObject *
    create(JSContext *cx, JSScript *script, JSObject &scopeChain, JSObject *callee);

    static const uint32 RESERVED_SLOTS = 3;
    static const uint32 DECL_ENV_RESERVED_SLOTS = 1;

    
    inline bool isForEval() const;

    
    inline js::StackFrame *maybeStackFrame() const;
    inline void setStackFrame(js::StackFrame *frame);

    



    inline JSObject *getCallee() const;
    inline JSFunction *getCalleeFunction() const; 
    inline void setCallee(JSObject *callee);

    
    inline const js::Value &getArguments() const;
    inline void setArguments(const js::Value &v);

    
    inline const js::Value &arg(uintN i) const;
    inline void setArg(uintN i, const js::Value &v);

    
    inline const js::Value &var(uintN i) const;
    inline void setVar(uintN i, const js::Value &v);

    




    inline js::Value *argArray();
    inline js::Value *varArray();

    inline void copyValues(uintN nargs, Value *argv, uintN nvars, Value *slots);
};

}

js::CallObject &
JSObject::asCall()
{
    JS_ASSERT(isCall());
    return *reinterpret_cast<js::CallObject *>(this);
}

#endif 
