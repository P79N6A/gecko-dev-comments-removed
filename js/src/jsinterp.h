






































#ifndef jsinterp_h___
#define jsinterp_h___



#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C











struct JSStackFrame {
    JSObject        *callobj;       
    JSObject        *argsobj;       
    JSObject        *varobj;        
    JSObject        *callee;        
    JSScript        *script;        
    JSFunction      *fun;           
    JSObject        *thisp;         
    uintN           argc;           
    jsval           *argv;          
    jsval           rval;           
    uintN           nvars;          
    jsval           *vars;          
    JSStackFrame    *down;          
    void            *annotation;    
    JSObject        *scopeChain;    
    jsbytecode      *pc;            
    jsval           *sp;            
    jsval           *spbase;        
    uintN           sharpDepth;     
    JSObject        *sharpArray;    
    uint32          flags;          
    JSStackFrame    *dormantNext;   
    JSObject        *xmlNamespace;  
    JSObject        *blockChain;    
};

typedef struct JSInlineFrame {
    JSStackFrame    frame;          
    jsval           *rvp;           
    void            *mark;          
    void            *hookData;      
    JSVersion       callerVersion;  
} JSInlineFrame;


#define JSFRAME_CONSTRUCTING  0x01  /* frame is for a constructor invocation */
#define JSFRAME_INTERNAL      0x02  /* internal call, not invoked by a script */
#define JSFRAME_ASSIGNING     0x04  /* a complex (not simplex JOF_ASSIGNING) op
                                       is currently assigning to a property */
#define JSFRAME_DEBUGGER      0x08  /* frame for JS_EvaluateInStackFrame */
#define JSFRAME_EVAL          0x10  /* frame for obj_eval */
#define JSFRAME_SCRIPT_OBJECT 0x20  /* compiling source for a Script object */
#define JSFRAME_YIELDING      0x40  /* js_Interpret dispatched JSOP_YIELD */
#define JSFRAME_FILTERING     0x80  /* XML filtering predicate expression */
#define JSFRAME_ITERATOR      0x100 /* trying to get an iterator for for-in */
#define JSFRAME_POP_BLOCKS    0x200 /* scope chain contains blocks to pop */
#define JSFRAME_GENERATOR     0x400 /* frame belongs to generator-iterator */
#define JSFRAME_ROOTED_ARGV   0x800 /* frame.argv is rooted by the caller */

#define JSFRAME_OVERRIDE_SHIFT 24   /* override bit-set params; see jsfun.c */
#define JSFRAME_OVERRIDE_BITS  8

#define JSFRAME_SPECIAL       (JSFRAME_DEBUGGER | JSFRAME_EVAL)

extern JS_FRIEND_API(jsval *)
js_AllocStack(JSContext *cx, uintN nslots, void **markp);

extern JS_FRIEND_API(void)
js_FreeStack(JSContext *cx, void *mark);

#ifdef DUMP_CALL_TABLE
# define JSOPTION_LOGCALL_TOSOURCE JS_BIT(15)

extern JSHashTable  *js_CallTable;
extern size_t       js_LogCallToSourceLimit;

extern void         js_DumpCallTable(JSContext *cx);
#endif








extern JSObject *
js_GetScopeChain(JSContext *cx, JSStackFrame *fp);










extern JSBool
js_GetPrimitiveThis(JSContext *cx, jsval *vp, JSClass *clasp, jsval *thisvp);








extern JSBool
js_ComputeThis(JSContext *cx, jsval *argv);










extern JS_FRIEND_API(JSBool)
js_Invoke(JSContext *cx, uintN argc, jsval *vp, uintN flags);














#define JSINVOKE_CONSTRUCT      JSFRAME_CONSTRUCTING
#define JSINVOKE_INTERNAL       JSFRAME_INTERNAL
#define JSINVOKE_ITERATOR       JSFRAME_ITERATOR




#define JSINVOKE_FUNFLAGS       (JSINVOKE_CONSTRUCT | JSINVOKE_ITERATOR)





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

extern JSBool
js_Execute(JSContext *cx, JSObject *chain, JSScript *script,
           JSStackFrame *down, uintN flags, jsval *result);

extern JSBool
js_CheckRedeclaration(JSContext *cx, JSObject *obj, jsid id, uintN attrs,
                      JSObject **objp, JSProperty **propp);

extern JSBool
js_StrictlyEqual(jsval lval, jsval rval);

extern JSBool
js_InvokeConstructor(JSContext *cx, jsval *vp, uintN argc);

extern JSBool
js_Interpret(JSContext *cx, jsbytecode *pc, jsval *result);

JS_END_EXTERN_C

#endif 
