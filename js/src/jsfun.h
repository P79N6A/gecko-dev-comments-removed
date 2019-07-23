






































#ifndef jsfun_h___
#define jsfun_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsobj.h"

JS_BEGIN_EXTERN_C

typedef struct JSLocalNameMap JSLocalNameMap;








typedef union JSLocalNames {
    jsuword         taggedAtom;
    jsuword         *array;
    JSLocalNameMap  *map;
} JSLocalNames;

struct JSNativeFunction {
    JSObject        object;
    uint16          flags;      
    uint16          nargs;      

    uint16          extra;      
    uint16          minargs;    

    JSNative        native;     
    JSClass         *clasp;     
    JSAtom          *atom;      
};

struct JSScriptedFunction {
    JSObject        *object;    
    uint16          flags;      
    uint16          nargs;      
    uint16          nvars;      
    uint16          spare;      
    JSScript        *script;    
    JSLocalNames    names;      
    JSAtom          *atom;      
};

#define JSFUN_EXPR_CLOSURE   0x4000 /* expression closure: function(x)x*x */
#define JSFUN_SCRIPTED       0x8000 /* use FUN_TO_SCRIPTED if set,
                                       FUN_TO_NATIVE if unset */






#define FUN_FLAGS(fun)       (*(uint16 *) (fun))

#define FUN_IS_SCRIPTED(fun) (FUN_FLAGS(fun) & JSFUN_SCRIPTED)

#define FUN_TO_SCRIPTED(fun)                                                  \
    (JS_ASSERT(FUN_IS_SCRIPTED(fun)),                                         \
     (JSScriptedFunction *)((uint8 *)fun - offsetof(JSScriptedFunction, flags)))

#define FUN_TO_NATIVE(fun)                                                    \
    (JS_ASSERT(!FUN_IS_SCRIPTED(fun)),                                        \
     (JSNativeFunction *)((uint8 *)fun - offsetof(JSNativeFunction, flags)))

#define NATIVE_TO_FUN(nfun)                                                   \
    (JS_ASSERT(((nfun)->flags & JSFUN_SCRIPTED) == 0),                        \
     (JSFunction *) (void *) &(nfun)->flags)

#define SCRIPTED_TO_FUN(sfun)                                                 \
    (JS_ASSERT((sfun)->flags & JSFUN_SCRIPTED),                               \
     (JSFunction *) (void *) &(sfun)->flags)

#define FUN_NARGS(fun)       (FUN_IS_SCRIPTED(fun)                            \
                              ? FUN_TO_SCRIPTED(fun)->nargs                   \
                              : FUN_TO_NATIVE(fun)->nargs)

#define FUN_OBJECT(fun)      (FUN_IS_SCRIPTED(fun)                            \
                              ? FUN_TO_SCRIPTED(fun)->object                  \
                              : &FUN_TO_NATIVE(fun)->object)

#define FUN_ATOM(fun)        (FUN_IS_SCRIPTED(fun)                            \
                              ? FUN_TO_SCRIPTED(fun)->atom                    \
                              : FUN_TO_NATIVE(fun)->atom)

#define NATIVE_FUN_MINARGS(nfun)                                              \
    (((nfun)->flags & JSFUN_FAST_NATIVE) ? (nfun)->minargs : (nfun)->nargs)

extern JSClass js_ArgumentsClass;
extern JS_FRIEND_DATA(JSClass) js_CallClass;


extern JS_FRIEND_DATA(JSClass) js_FunctionClass;

#define HAS_FUNCTION_CLASS(obj) (STOBJ_GET_CLASS(obj) == &js_FunctionClass)




#define VALUE_IS_FUNCTION(cx, v)                                              \
    (!JSVAL_IS_PRIMITIVE(v) && HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(v)))





#define GET_FUNCTION_PRIVATE(cx, funobj)                                      \
    (JS_ASSERT(HAS_FUNCTION_CLASS(funobj)),                                   \
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

extern JSNativeFunction *
js_NewNativeFunction(JSContext *cx, JSNative native, uintN nargs, uintN flags,
                     JSObject *parent, JSAtom *atom);

extern JSScriptedFunction *
js_NewScriptedFunction(JSContext *cx, JSObject *funobj, uintN flags,
                       JSObject *parent, JSAtom *atom);

extern void
js_TraceFunction(JSTracer *trc, JSScriptedFunction *fun);

extern void
js_FinalizeFunction(JSContext *cx, JSScriptedFunction *fun);

extern JSObject *
js_CloneFunctionObject(JSContext *cx, JSObject *funobj, JSObject *parent);

extern JSNativeFunction *
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

#define JS_GET_LOCAL_NAME_COUNT(fun)    ((fun)->nargs + (fun)->nvars)

extern JSBool
js_AddLocal(JSContext *cx, JSScriptedFunction *fun, JSAtom *atom,
            JSLocalKind kind);







extern JSLocalKind
js_LookupLocal(JSContext *cx, JSScriptedFunction *fun, JSAtom *atom,
               uintN *indexp);















extern jsuword *
js_GetLocalNameArray(JSContext *cx, JSScriptedFunction *fun, JSArenaPool *pool);

#define JS_LOCAL_NAME_TO_ATOM(nameWord)                                       \
    ((JSAtom *) ((nameWord) & ~(jsuword) 1))

#define JS_LOCAL_NAME_IS_CONST(nameWord)                                      \
    ((((nameWord) & (jsuword) 1)) != 0)

extern void
js_FreezeLocalNames(JSContext *cx, JSScriptedFunction *fun);

JS_END_EXTERN_C

#endif 
