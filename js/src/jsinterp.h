







































#ifndef jsinterp_h___
#define jsinterp_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsfun.h"
#include "jsopcode.h"
#include "jsscript.h"

JS_BEGIN_EXTERN_C

typedef struct JSFrameRegs {
    jsbytecode      *pc;            
    jsval           *sp;            
} JSFrameRegs;


enum JSFrameFlags {
    JSFRAME_CONSTRUCTING       =  0x01, 
    JSFRAME_OVERRIDE_ARGS      =  0x02, 
    JSFRAME_ASSIGNING          =  0x04, 

    JSFRAME_DEBUGGER           =  0x08, 
    JSFRAME_EVAL               =  0x10, 
    JSFRAME_FLOATING_GENERATOR =  0x20, 
    JSFRAME_YIELDING           =  0x40, 
    JSFRAME_GENERATOR          =  0x80, 

    JSFRAME_SPECIAL            = JSFRAME_DEBUGGER | JSFRAME_EVAL
};









struct JSStackFrame
{
    jsbytecode          *imacpc;        
    JSObject            *callobj;       
    jsval               argsobj;        

    JSScript            *script;        
    JSFunction          *fun;           

    












    jsval               thisv;

    uintN               argc;           
    jsval               *argv;          
    jsval               rval;           
    void                *annotation;    

    
    JSStackFrame        *down;          

    jsbytecode          *savedPC;       
#ifdef DEBUG
    static jsbytecode *const sInvalidPC;
#endif

    



































    union {
        JSObject    *scopeChain;
        jsval       scopeChainVal;
    };
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

    jsval *argEnd() const {
        return (jsval *)this;
    }

    jsval *slots() const {
        return (jsval *)(this + 1);
    }

    jsval calleeValue() {
        JS_ASSERT(argv);
        return argv[-2];
    }

    JSObject *calleeObject() {
        JS_ASSERT(argv);
        return JSVAL_TO_OBJECT(argv[-2]);
    }

    JSObject *callee() {
        return argv ? JSVAL_TO_OBJECT(argv[-2]) : NULL;
    }

    




    JSObject *varobj(js::CallStack *cs) const;

    
    JSObject *varobj(JSContext *cx) const;

    inline JSObject *getThisObject(JSContext *cx);

    bool isGenerator() const { return flags & JSFRAME_GENERATOR; }
    bool isFloatingGenerator() const {
        if (flags & JSFRAME_FLOATING_GENERATOR) {
            JS_ASSERT(isGenerator());
            return true;
        }
        return false;
    }

private:
    JSObject *computeThisObject(JSContext *cx);
};

namespace js {

static const size_t VALUES_PER_STACK_FRAME = sizeof(JSStackFrame) / sizeof(jsval);
JS_STATIC_ASSERT(sizeof(JSStackFrame) % sizeof(jsval) == 0);

}

static JS_INLINE jsval *
StackBase(JSStackFrame *fp)
{
    return fp->slots() + fp->script->nfixed;
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
js_GetPrimitiveThis(JSContext *cx, jsval *vp, JSClass *clasp, jsval *thisvp);







extern JSObject *
js_ComputeThis(JSContext *cx, jsval *argv);

extern const uint16 js_PrimitiveTestFlags[];

#define PRIMITIVE_THIS_TEST(fun,thisv)                                        \
    (JS_ASSERT(!JSVAL_IS_VOID(thisv)),                                        \
     JSFUN_THISP_TEST(JSFUN_THISP_FLAGS((fun)->flags),                        \
                      js_PrimitiveTestFlags[JSVAL_TAG(thisv) - 1]))









extern JS_REQUIRES_STACK JS_FRIEND_API(JSBool)
js_Invoke(JSContext *cx, const js::InvokeArgsGuard &args, uintN flags);














#define JSINVOKE_CONSTRUCT      JSFRAME_CONSTRUCTING




#define JSINVOKE_FUNFLAGS       JSINVOKE_CONSTRUCT





#define js_InternalCall(cx,obj,fval,argc,argv,rval)                           \
    js_InternalInvoke(cx, obj, fval, 0, argc, argv, rval)

#define js_InternalConstruct(cx,obj,fval,argc,argv,rval)                      \
    js_InternalInvoke(cx, obj, fval, JSINVOKE_CONSTRUCT, argc, argv, rval)

extern JSBool
js_InternalInvoke(JSContext *cx, JSObject *obj, jsval fval, uintN flags,
                  uintN argc, jsval *argv, jsval *rval);

extern JSBool
js_InternalGetOrSet(JSContext *cx, JSObject *obj, jsid id, jsval fval,
                    JSAccessMode mode, uintN argc, jsval *argv, jsval *rval);

extern JS_FORCES_STACK JSBool
js_Execute(JSContext *cx, JSObject *chain, JSScript *script,
           JSStackFrame *down, uintN flags, jsval *result);

extern JS_REQUIRES_STACK JSBool
js_InvokeConstructor(JSContext *cx, const js::InvokeArgsGuard &args, JSBool clampReturn);

extern JS_REQUIRES_STACK JSBool
js_Interpret(JSContext *cx);

#define JSPROP_INITIALIZER 0x100   /* NB: Not a valid property attribute. */

extern JSBool
js_CheckRedeclaration(JSContext *cx, JSObject *obj, jsid id, uintN attrs,
                      JSObject **objp, JSProperty **propp);

extern JSBool
js_StrictlyEqual(JSContext *cx, jsval lval, jsval rval);


extern JSBool
js_SameValue(jsval v1, jsval v2, JSContext *cx);

extern JSBool
js_InternNonIntElementId(JSContext *cx, JSObject *obj, jsval idval, jsid *idp);





extern jsval&
js_GetUpvar(JSContext *cx, uintN level, uintN cookie);












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
















extern JSObject *
js_ComputeGlobalThis(JSContext *cx, jsval *argv);

extern JS_REQUIRES_STACK JSBool
js_EnterWith(JSContext *cx, jsint stackIndex);

extern JS_REQUIRES_STACK void
js_LeaveWith(JSContext *cx);

extern JS_REQUIRES_STACK JSClass *
js_IsActiveWithOrBlock(JSContext *cx, JSObject *obj, int stackDepth);





extern JS_REQUIRES_STACK JSBool
js_UnwindScope(JSContext *cx, jsint stackDepth, JSBool normalUnwind);

extern JSBool
js_OnUnknownMethod(JSContext *cx, jsval *vp);







extern JSBool
js_DoIncDec(JSContext *cx, const JSCodeSpec *cs, jsval *vp, jsval *vp2);





extern JS_REQUIRES_STACK void
js_TraceOpcode(JSContext *cx);




extern void
js_MeterOpcodePair(JSOp op1, JSOp op2);

extern void
js_MeterSlotOpcode(JSOp op, uint32 slot);

#endif 

JS_END_EXTERN_C

inline JSObject *
JSStackFrame::getThisObject(JSContext *cx)
{
    return JSVAL_IS_PRIMITIVE(thisv) ? computeThisObject(cx) : JSVAL_TO_OBJECT(thisv);
}

#endif 
