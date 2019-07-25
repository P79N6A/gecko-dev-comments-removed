







































#ifndef jsinterp_h___
#define jsinterp_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsfun.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsvalue.h"

typedef struct JSFrameRegs {
    JSStackFrame    *fp;            
    jsbytecode      *pc;            
    js::Value       *sp;            
} JSFrameRegs;


enum JSFrameFlags {
    JSFRAME_CONSTRUCTING       =  0x01, 
    JSFRAME_COMPUTED_THIS      =  0x02, 

    JSFRAME_ASSIGNING          =  0x04, 

    JSFRAME_DEBUGGER           =  0x08, 
    JSFRAME_EVAL               =  0x10, 
    JSFRAME_FLOATING_GENERATOR =  0x20, 
    JSFRAME_YIELDING           =  0x40, 
    JSFRAME_GENERATOR          =  0x80, 
    JSFRAME_OVERRIDE_ARGS      = 0x100, 
    JSFRAME_DUMMY              = 0x200, 
    JSFRAME_IN_IMACRO          = 0x400, 

    JSFRAME_SPECIAL            = JSFRAME_DEBUGGER | JSFRAME_EVAL
};









struct JSStackFrame
{
  private:
    JSObject            *callobj;       
    JSObject            *argsobj;       
    JSObject            *scopeChain;    
    JSObject            *blockChain;    
    jsbytecode          *imacpc;        
    void                *annotation;    
    void                *hookData;      
    JSVersion           callerVersion;  
    JSScript            *script;        
    JSFunction          *fun;           
    js::Value           thisv;          
    js::Value           rval;           
    uintN               argc;           

  public:
    js::Value           *argv;          

    
    JSStackFrame        *down;          

    jsbytecode          *savedPC;       
#ifdef DEBUG
    static jsbytecode *const sInvalidPC;
#endif

    uint32              flags;          

    void                *padding;

    
    jsbytecode *pc(JSContext *cx) const;

    js::Value *argEnd() const {
        return (js::Value *)this;
    }

    js::Value *slots() const {
        return (js::Value *)(this + 1);
    }

    js::Value *base() const {
        return slots() + getScript()->nfixed;
    }

    

    bool hasCallObj() const {
        return callobj != NULL;
    }

    JSObject* getCallObj() const {
        JS_ASSERT(hasCallObj());
        return callobj;
    }

    JSObject* maybeCallObj() const {
        return callobj;
    }

    void setCallObj(JSObject *obj) {
        callobj = obj;
    }

    static size_t offsetCallObj() {
        return offsetof(JSStackFrame, callobj);
    }

    

    bool hasArgsObj() const {
        return argsobj != NULL;
    }

    JSObject* getArgsObj() const {
        JS_ASSERT(hasArgsObj());
        JS_ASSERT(!isEvalFrame());
        return argsobj;
    }

    JSObject* maybeArgsObj() const {
        return argsobj;
    }

    void setArgsObj(JSObject *obj) {
        argsobj = obj;
    }

    JSObject** addressArgsObj() {
        return &argsobj;
    }

    static size_t offsetArgsObj() {
        return offsetof(JSStackFrame, argsobj);
    }

    




































    

    bool hasScopeChain() const {
        return scopeChain != NULL;
    }

    JSObject* getScopeChain() const {
        JS_ASSERT(hasScopeChain());
        return scopeChain;
    }

    JSObject* maybeScopeChain() const {
        return scopeChain;
    }

    void setScopeChain(JSObject *obj) {
        scopeChain = obj;
    }

    JSObject** addressScopeChain() {
        return &scopeChain;
    }

    static size_t offsetScopeChain() {
        return offsetof(JSStackFrame, scopeChain);
    }

    

    bool hasBlockChain() const {
        return blockChain != NULL;
    }

    JSObject* getBlockChain() const {
        JS_ASSERT(hasBlockChain());
        return blockChain;
    }

    JSObject* maybeBlockChain() const {
        return blockChain;
    }

    void setBlockChain(JSObject *obj) {
        blockChain = obj;
    }

    

    bool hasIMacroPC() const { return flags & JSFRAME_IN_IMACRO; }

    





    jsbytecode *getIMacroPC() const {
        JS_ASSERT(flags & JSFRAME_IN_IMACRO);
        return imacpc;
    }

    
    jsbytecode *maybeIMacroPC() const { return hasIMacroPC() ? getIMacroPC() : NULL; }

    void clearIMacroPC() { flags &= ~JSFRAME_IN_IMACRO; }

    void setIMacroPC(jsbytecode *newIMacPC) {
        JS_ASSERT(newIMacPC);
        JS_ASSERT(!(flags & JSFRAME_IN_IMACRO));
        imacpc = newIMacPC;
        flags |= JSFRAME_IN_IMACRO;
    }

    

    bool hasAnnotation() const {
        return annotation != NULL;
    }

    void* getAnnotation() const {
        JS_ASSERT(hasAnnotation());
        return annotation;
    }

    void* maybeAnnotation() const {
        return annotation;
    }

    void setAnnotation(void *annot) {
        annotation = annot;
    }

    

    bool hasHookData() const {
        return hookData != NULL;
    }

    void* getHookData() const {
        JS_ASSERT(hasHookData());
        return hookData;
    }

    void* maybeHookData() const {
        return hookData;
    }

    void setHookData(void *data) {
        hookData = data;
    }

    

    JSVersion getCallerVersion() const {
        return callerVersion;
    }

    void setCallerVersion(JSVersion version) {
        callerVersion = version;
    }

    

    bool hasScript() const {
        return script != NULL;
    }

    JSScript* getScript() const {
        JS_ASSERT(hasScript());
        return script;
    }

    JSScript* maybeScript() const {
        return script;
    }

    size_t getFixedCount() const {
        return getScript()->nfixed;
    }

    size_t getSlotCount() const {
        return getScript()->nslots;
    }

    void setScript(JSScript *s) {
        script = s;
    }

    static size_t offsetScript() {
        return offsetof(JSStackFrame, script);
    }

    

    bool hasFunction() const {
        return fun != NULL;
    }

    JSFunction* getFunction() const {
        JS_ASSERT(hasFunction());
        return fun;
    }

    JSFunction* maybeFunction() const {
        return fun;
    }

    size_t numFormalArgs() const {
        JS_ASSERT(!isEvalFrame());
        return getFunction()->nargs;
    }

    void setFunction(JSFunction *f) {
        fun = f;
    }

    

    const js::Value& getThisValue() {
        return thisv;
    }

    void setThisValue(const js::Value &v) {
        thisv = v;
    }

    

    const js::Value& getReturnValue() {
        return rval;
    }

    void setReturnValue(const js::Value &v) {
        rval = v;
    }

    void clearReturnValue() {
        rval.setUndefined();
    }

    js::Value* addressReturnValue() {
        return &rval;
    }

    static size_t offsetReturnValue() {
        return offsetof(JSStackFrame, rval);
    }

    

    size_t numActualArgs() const {
        JS_ASSERT(!isEvalFrame());
        return argc;
    }

    void setNumActualArgs(size_t n) {
        argc = n;
    }

    static size_t offsetNumActualArgs() {
        return offsetof(JSStackFrame, argc);
    }

    

    void putActivationObjects(JSContext *cx) {
        



        if (hasCallObj()) {
            js_PutCallObject(cx, this);
            JS_ASSERT(!hasArgsObj());
        } else if (hasArgsObj()) {
            js_PutArgsObject(cx, this);
        }
    }

    const js::Value &calleeValue() {
        JS_ASSERT(argv);
        return argv[-2];
    }

    
    JSObject &calleeObject() const {
        JS_ASSERT(argv);
        return argv[-2].toObject();
    }

    





    bool getValidCalleeObject(JSContext *cx, js::Value *vp);

    void setCalleeObject(JSObject &callable) {
        JS_ASSERT(argv);
        argv[-2].setObject(callable);
    }

    JSObject *callee() {
        return argv ? &argv[-2].toObject() : NULL;
    }

    




    JSObject *varobj(js::StackSegment *seg) const;

    
    JSObject *varobj(JSContext *cx) const;

    inline JSObject *getThisObject(JSContext *cx);

    bool isGenerator() const { return !!(flags & JSFRAME_GENERATOR); }
    bool isFloatingGenerator() const {
        JS_ASSERT_IF(flags & JSFRAME_FLOATING_GENERATOR, isGenerator());
        return !!(flags & JSFRAME_FLOATING_GENERATOR);
    }

    bool isDummyFrame() const { return !!(flags & JSFRAME_DUMMY); }
    bool isEvalFrame() const { return !!(flags & JSFRAME_EVAL); }

    
    inline void staticAsserts();
};

namespace js {

JS_STATIC_ASSERT(sizeof(JSStackFrame) % sizeof(Value) == 0);
static const size_t VALUES_PER_STACK_FRAME = sizeof(JSStackFrame) / sizeof(Value);

} 

inline void
JSStackFrame::staticAsserts()
{
    JS_STATIC_ASSERT(offsetof(JSStackFrame, rval) % sizeof(js::Value) == 0);
    JS_STATIC_ASSERT(offsetof(JSStackFrame, thisv) % sizeof(js::Value) == 0);
}

static JS_INLINE uintN
GlobalVarCount(JSStackFrame *fp)
{
    JS_ASSERT(!fp->hasFunction());
    return fp->getScript()->nfixed;
}








extern JSObject *
js_GetScopeChain(JSContext *cx, JSStackFrame *fp);










extern JSBool
js_GetPrimitiveThis(JSContext *cx, js::Value *vp, js::Class *clasp,
                    const js::Value **vpp);

namespace js {








extern JSObject *
ComputeThisFromArgv(JSContext *cx, js::Value *argv);

JS_ALWAYS_INLINE JSObject *
ComputeThisFromVp(JSContext *cx, js::Value *vp)
{
    return ComputeThisFromArgv(cx, vp + 2);
}

JS_ALWAYS_INLINE bool
PrimitiveThisTest(JSFunction *fun, const Value &v)
{
    uint16 flags = fun->flags;
    return (v.isString() && !!(flags & JSFUN_THISP_STRING)) ||
           (v.isNumber() && !!(flags & JSFUN_THISP_NUMBER)) ||
           (v.isBoolean() && !!(flags & JSFUN_THISP_BOOLEAN));
}





struct CallArgs
{
    Value *argv_;
    uintN argc_;
  protected:
    CallArgs() {}
    CallArgs(Value *argv, uintN argc) : argv_(argv), argc_(argc) {}
  public:
    Value *base() const { return argv_ - 2; }
    Value &callee() const { return argv_[-2]; }
    Value &thisv() const { return argv_[-1]; }
    Value &operator[](unsigned i) const { JS_ASSERT(i < argc_); return argv_[i]; }
    Value *argv() const { return argv_; }
    uintN argc() const { return argc_; }
    Value &rval() const { return argv_[-2]; }

    JSObject *computeThis(JSContext *cx) const {
        return ComputeThisFromArgv(cx, argv_);
    }
};









extern JS_REQUIRES_STACK bool
Invoke(JSContext *cx, const CallArgs &args, uintN flags);














#define JSINVOKE_CONSTRUCT      JSFRAME_CONSTRUCTING




#define JSINVOKE_FUNFLAGS       JSINVOKE_CONSTRUCT





extern JSBool
InternalInvoke(JSContext *cx, const Value &thisv, const Value &fval, uintN flags,
               uintN argc, Value *argv, Value *rval);

static JS_ALWAYS_INLINE bool
InternalCall(JSContext *cx, JSObject *obj, const Value &fval,
             uintN argc, Value *argv, Value *rval)
{
    return InternalInvoke(cx, ObjectOrNullValue(obj), fval, 0, argc, argv, rval);
}

static JS_ALWAYS_INLINE bool
InternalConstruct(JSContext *cx, JSObject *obj, const Value &fval,
                  uintN argc, Value *argv, Value *rval)
{
    return InternalInvoke(cx, ObjectOrNullValue(obj), fval, JSINVOKE_CONSTRUCT, argc, argv, rval);
}

extern bool
InternalGetOrSet(JSContext *cx, JSObject *obj, jsid id, const Value &fval,
                 JSAccessMode mode, uintN argc, Value *argv, Value *rval);

extern JS_FORCES_STACK bool
Execute(JSContext *cx, JSObject *chain, JSScript *script,
        JSStackFrame *down, uintN flags, Value *result);

extern JS_REQUIRES_STACK bool
InvokeConstructor(JSContext *cx, const CallArgs &args);

extern JS_REQUIRES_STACK bool
Interpret(JSContext *cx);

#define JSPROP_INITIALIZER 0x100   /* NB: Not a valid property attribute. */

extern bool
CheckRedeclaration(JSContext *cx, JSObject *obj, jsid id, uintN attrs,
                   JSObject **objp, JSProperty **propp);

extern bool
StrictlyEqual(JSContext *cx, const Value &lval, const Value &rval);


extern bool
SameValue(const Value &v1, const Value &v2, JSContext *cx);

extern JSType
TypeOfValue(JSContext *cx, const Value &v);

inline bool
InstanceOf(JSContext *cx, JSObject *obj, Class *clasp, Value *argv)
{
    if (obj && obj->getClass() == clasp)
        return true;
    extern bool InstanceOfSlow(JSContext *, JSObject *, Class *, Value *);
    return InstanceOfSlow(cx, obj, clasp, argv);
}

extern JSBool
HasInstance(JSContext *cx, JSObject *obj, const js::Value *v, JSBool *bp);

inline void *
GetInstancePrivate(JSContext *cx, JSObject *obj, Class *clasp, Value *argv)
{
    if (!InstanceOf(cx, obj, clasp, argv))
        return NULL;
    return obj->getPrivate();
}

extern bool
ValueToId(JSContext *cx, const Value &v, jsid *idp);








extern const js::Value &
GetUpvar(JSContext *cx, uintN level, js::UpvarCookie cookie);

} 












#ifndef JS_LONE_INTERPRET
# ifdef _MSC_VER
#  define JS_LONE_INTERPRET 0
# else
#  define JS_LONE_INTERPRET 1
# endif
#endif

#define JS_MAX_INLINE_CALL_COUNT 3000

#if !JS_LONE_INTERPRET
# define JS_STATIC_INTERPRET    static
#else
# define JS_STATIC_INTERPRET

extern JS_REQUIRES_STACK JSBool
js_EnterWith(JSContext *cx, jsint stackIndex);

extern JS_REQUIRES_STACK void
js_LeaveWith(JSContext *cx);

extern JS_REQUIRES_STACK js::Class *
js_IsActiveWithOrBlock(JSContext *cx, JSObject *obj, int stackDepth);





extern JS_REQUIRES_STACK JSBool
js_UnwindScope(JSContext *cx, jsint stackDepth, JSBool normalUnwind);

extern JSBool
js_OnUnknownMethod(JSContext *cx, js::Value *vp);







extern JSBool
js_DoIncDec(JSContext *cx, const JSCodeSpec *cs, js::Value *vp, js::Value *vp2);





extern JS_REQUIRES_STACK void
js_TraceOpcode(JSContext *cx);




extern void
js_MeterOpcodePair(JSOp op1, JSOp op2);

extern void
js_MeterSlotOpcode(JSOp op, uint32 slot);

#endif 

inline JSObject *
JSStackFrame::getThisObject(JSContext *cx)
{
    JS_ASSERT(!isDummyFrame());
    if (flags & JSFRAME_COMPUTED_THIS)
        return &thisv.toObject();
    if (!js::ComputeThisFromArgv(cx, argv))
        return NULL;
    setThisValue(argv[-1]);
    flags |= JSFRAME_COMPUTED_THIS;
    return &thisv.toObject();
}

#endif 
