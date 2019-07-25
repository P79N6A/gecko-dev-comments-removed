







































#ifndef jsinterp_h___
#define jsinterp_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsfun.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsvalue.h"

typedef struct JSFrameRegs {
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

    JSFRAME_SPECIAL            = JSFRAME_DEBUGGER | JSFRAME_EVAL
};









struct JSStackFrame
{
    jsbytecode          *imacpc;        
    JSObject            *callobj;       
    JSObject            *argsobj;       
    JSScript            *script;        
    js::Value           thisv;          
    JSFunction          *fun;           
    uintN               argc;           
    js::Value           *argv;          
    void                *annotation;    
    js::Value           rval;           

    
    JSStackFrame        *down;          

    jsbytecode          *savedPC;       
#ifdef DEBUG
    static jsbytecode *const sInvalidPC;
#endif

    



































    JSObject        *scopeChain;
    JSObject        *blockChain;

    uint32          flags;          
    JSStackFrame    *displaySave;   


    
    void            *hookData;      
    JSVersion       callerVersion;  

    void putActivationObjects(JSContext *cx) {
        



        if (callobj) {
            js_PutCallObject(cx, this);
            JS_ASSERT(!argsobj);
        } else if (argsobj) {
            js_PutArgsObject(cx, this);
        }
    }

    
    jsbytecode *pc(JSContext *cx) const;

    js::Value *argEnd() const {
        return (js::Value *)this;
    }

    js::Value *slots() const {
        return (js::Value *)(this + 1);
    }

    js::Value *base() const {
        return slots() + script->nfixed;
    }

    const js::Value &calleeValue() {
        JS_ASSERT(argv);
        return argv[-2];
    }

    JSObject *callee() {
        return argv ? &argv[-2].toObject() : NULL;
    }

    




    JSObject *varobj(js::CallStackSegment *css) const;

    
    JSObject *varobj(JSContext *cx) const;

    inline JSObject *getThisObject(JSContext *cx);

    bool isGenerator() const { return !!(flags & JSFRAME_GENERATOR); }
    bool isFloatingGenerator() const {
        JS_ASSERT_IF(flags & JSFRAME_FLOATING_GENERATOR, isGenerator());
        return !!(flags & JSFRAME_FLOATING_GENERATOR);
    }

    bool isDummyFrame() const { return !script && !fun; }
};

namespace js {

JS_STATIC_ASSERT(sizeof(JSStackFrame) % sizeof(Value) == 0);
static const size_t VALUES_PER_STACK_FRAME = sizeof(JSStackFrame) / sizeof(Value);

JS_STATIC_ASSERT(offsetof(JSStackFrame, rval) % sizeof(Value) == 0);
JS_STATIC_ASSERT(offsetof(JSStackFrame, thisv) % sizeof(Value) == 0);

} 

static JS_INLINE uintN
GlobalVarCount(JSStackFrame *fp)
{
    JS_ASSERT(!fp->fun);
    return fp->script->nfixed;
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









extern JS_REQUIRES_STACK bool
Invoke(JSContext *cx, const InvokeArgsGuard &args, uintN flags);

extern JS_REQUIRES_STACK JS_FRIEND_API(bool)
InvokeFriendAPI(JSContext *cx, const InvokeArgsGuard &args, uintN flags);














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
InvokeConstructor(JSContext *cx, const InvokeArgsGuard &args);

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

} 





extern const js::Value &
js_GetUpvar(JSContext *cx, uintN level, js::UpvarCookie cookie);












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
    if (flags & JSFRAME_COMPUTED_THIS)
        return &thisv.toObject();
    if (!js::ComputeThisFromArgv(cx, argv))
        return NULL;
    thisv = argv[-1];
    flags |= JSFRAME_COMPUTED_THIS;
    return &thisv.toObject();
}

#endif 
