







































#ifndef CallObject_h___
#define CallObject_h___

namespace js {

class CallObject : public ::JSObject
{
    










    static const uintN CALLEE_SLOT = 0;
    static const uintN ARGUMENTS_SLOT = 1;

  public:
    static const uintN RESERVED_SLOTS = 2;

    
    static CallObject *
    create(JSContext *cx, JSScript *script, JSObject &scopeChain, JSObject *callee);

    
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
