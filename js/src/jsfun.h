






































#ifndef jsfun_h___
#define jsfun_h___



#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C

typedef struct JSLocalNameMap JSLocalNameMap;








typedef union JSLocalNames {
    jsuword         taggedAtom;
    jsuword         *array;
    JSLocalNameMap  *map;
} JSLocalNames;

struct JSFunction {
    JSObject        *object;    
    uint16          nargs;      

    uint16          flags;      
    union {
        struct {
            uint16      extra;  
            uint16      minargs;

            JSNative    native; 
            JSClass     *clasp; 
        } n;
        struct {
            uint16      nvars;  
            uint16      spare;  
            JSScript    *script;
            JSLocalNames names; 
        } i;
    } u;
    JSAtom          *atom;      
};

#define JSFUN_EXPR_CLOSURE   0x4000 /* expression closure: function(x)x*x */
#define JSFUN_INTERPRETED    0x8000 /* use u.i if set, u.n if unset */

#define JSFUN_SCRIPT_OR_FAST_NATIVE (JSFUN_INTERPRETED | JSFUN_FAST_NATIVE)

#define FUN_INTERPRETED(fun) ((fun)->flags & JSFUN_INTERPRETED)
#define FUN_SLOW_NATIVE(fun) (!((fun)->flags & JSFUN_SCRIPT_OR_FAST_NATIVE))
#define FUN_SCRIPT(fun)      (FUN_INTERPRETED(fun) ? (fun)->u.i.script : NULL)
#define FUN_NATIVE(fun)      (FUN_SLOW_NATIVE(fun) ? (fun)->u.n.native : NULL)
#define FUN_FAST_NATIVE(fun) (((fun)->flags & JSFUN_FAST_NATIVE)              \
                              ? (JSFastNative) (fun)->u.n.native              \
                              : NULL)
#define FUN_MINARGS(fun)     (((fun)->flags & JSFUN_FAST_NATIVE)              \
                              ? (fun)->u.n.minargs                            \
                              : (fun)->nargs)

extern JSClass js_ArgumentsClass;
extern JS_FRIEND_DATA(JSClass) js_CallClass;


extern JS_FRIEND_DATA(JSClass) js_FunctionClass;




#define VALUE_IS_FUNCTION(cx, v)                                              \
    (!JSVAL_IS_PRIMITIVE(v) &&                                                \
     OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(v)) == &js_FunctionClass)





#define GET_FUNCTION_PRIVATE(cx, funobj)                                      \
    (JS_ASSERT(OBJ_GET_CLASS(cx, funobj) == &js_FunctionClass),               \
     (JSFunction *) OBJ_GET_PRIVATE(cx, funobj))

extern JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitArgumentsClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitCallClass(JSContext *cx, JSObject *obj);

extern JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, JSNative native, uintN nargs,
               uintN flags, JSObject *parent, JSAtom *atom);

extern void
js_TraceFunction(JSTracer *trc, JSFunction *fun);

extern void
js_FinalizeFunction(JSContext *cx, JSFunction *fun);

extern JSObject *
js_CloneFunctionObject(JSContext *cx, JSObject *funobj, JSObject *parent);

extern JSBool
js_LinkFunctionObject(JSContext *cx, JSFunction *fun, JSObject *object);

extern JSFunction *
js_DefineFunction(JSContext *cx, JSObject *obj, JSAtom *atom, JSNative native,
                  uintN nargs, uintN flags);






#define JSV2F_CONSTRUCT         JSINVOKE_CONSTRUCT
#define JSV2F_ITERATOR          JSINVOKE_ITERATOR
#define JSV2F_SEARCH_STACK      0x10000

extern JSFunction *
js_ValueToFunction(JSContext *cx, jsval *vp, uintN flags);

extern JSObject *
js_ValueToFunctionObject(JSContext *cx, jsval *vp, uintN flags);

extern JSObject *
js_ValueToCallableObject(JSContext *cx, jsval *vp, uintN flags);

extern void
js_ReportIsNotFunction(JSContext *cx, jsval *vp, uintN flags);

extern JSObject *
js_GetCallObject(JSContext *cx, JSStackFrame *fp, JSObject *parent);

extern JS_FRIEND_API(JSBool)
js_PutCallObject(JSContext *cx, JSStackFrame *fp);

extern JSBool
js_GetCallVariable(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

extern JSBool
js_SetCallVariable(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

extern JSBool
js_GetArgsValue(JSContext *cx, JSStackFrame *fp, jsval *vp);

extern JSBool
js_GetArgsProperty(JSContext *cx, JSStackFrame *fp, jsid id, jsval *vp);

extern JSObject *
js_GetArgsObject(JSContext *cx, JSStackFrame *fp);

extern JS_FRIEND_API(JSBool)
js_PutArgsObject(JSContext *cx, JSStackFrame *fp);

extern JSBool
js_XDRFunction(JSXDRState *xdr, JSObject **objp);

typedef enum JSLocalKind {
    JSLOCAL_NONE,
    JSLOCAL_ARG,
    JSLOCAL_VAR,
    JSLOCAL_CONST
} JSLocalKind;

#define JS_GET_LOCAL_NAME_COUNT(fun)    ((fun)->nargs + (fun)->u.i.nvars)

extern JSBool
js_AddLocal(JSContext *cx, JSFunction *fun, JSAtom *atom, JSLocalKind kind);







extern JSLocalKind
js_LookupLocal(JSContext *cx, JSFunction *fun, JSAtom *atom, uintN *indexp);















extern jsuword *
js_GetLocalNameArray(JSContext *cx, JSFunction *fun, JSArenaPool *pool);

#define JS_LOCAL_NAME_TO_ATOM(nameWord)                                       \
    ((JSAtom *) ((nameWord) & ~(jsuword) 1))

#define JS_LOCAL_NAME_IS_CONST(nameWord)                                      \
    ((((nameWord) & (jsuword) 1)) != 0)

extern void
js_FreezeLocalNames(JSContext *cx, JSFunction *fun);

JS_END_EXTERN_C

#endif 
