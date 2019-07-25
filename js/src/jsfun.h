






































#ifndef jsfun_h___
#define jsfun_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsobj.h"

typedef struct JSLocalNameMap JSLocalNameMap;








typedef union JSLocalNames {
    jsuword         taggedAtom;
    jsuword         *array;
    JSLocalNameMap  *map;
} JSLocalNames;



































#define JSFUN_EXPR_CLOSURE  0x1000  /* expression closure: function(x) x*x */
#define JSFUN_TRCINFO       0x2000  /* when set, u.n.trcinfo is non-null,
                                       JSFunctionSpec::call points to a
                                       JSNativeTraceInfo. */
#define JSFUN_INTERPRETED   0x4000  /* use u.i if kind >= this value else u.n */
#define JSFUN_FLAT_CLOSURE  0x8000  /* flag (aka "display") closure */
#define JSFUN_NULL_CLOSURE  0xc000  /* null closure entrains no scope chain */
#define JSFUN_KINDMASK      0xc000  /* encode interp vs. native and closure
                                       optimization level -- see above */

#define FUN_OBJECT(fun)      (static_cast<JSObject *>(fun))
#define FUN_KIND(fun)        ((fun)->flags & JSFUN_KINDMASK)
#define FUN_SET_KIND(fun,k)  ((fun)->flags = ((fun)->flags & ~JSFUN_KINDMASK) | (k))
#define FUN_INTERPRETED(fun) (FUN_KIND(fun) >= JSFUN_INTERPRETED)
#define FUN_FLAT_CLOSURE(fun)(FUN_KIND(fun) == JSFUN_FLAT_CLOSURE)
#define FUN_NULL_CLOSURE(fun)(FUN_KIND(fun) == JSFUN_NULL_CLOSURE)
#define FUN_SLOW_NATIVE(fun) (!FUN_INTERPRETED(fun) && !((fun)->flags & JSFUN_FAST_NATIVE))
#define FUN_SCRIPT(fun)      (FUN_INTERPRETED(fun) ? (fun)->u.i.script : NULL)
#define FUN_NATIVE(fun)      (FUN_SLOW_NATIVE(fun) ? (fun)->u.n.native : NULL)
#define FUN_FAST_NATIVE(fun) (((fun)->flags & JSFUN_FAST_NATIVE)              \
                              ? (js::FastNative) (fun)->u.n.native            \
                              : NULL)
#define FUN_MINARGS(fun)     (((fun)->flags & JSFUN_FAST_NATIVE)              \
                              ? 0                                             \
                              : (fun)->nargs)
#define FUN_CLASP(fun)       (JS_ASSERT(!FUN_INTERPRETED(fun)),               \
                              fun->u.n.clasp)
#define FUN_TRCINFO(fun)     (JS_ASSERT(!FUN_INTERPRETED(fun)),               \
                              JS_ASSERT((fun)->flags & JSFUN_TRCINFO),        \
                              fun->u.n.trcinfo)

struct JSFunction : public JSObject
{
    uint16          nargs;        

    uint16          flags;        
    union {
        struct {
            uint16      extra;    
            uint16      spare;    
            js::Native  native;   
            js::Class   *clasp;   

            JSNativeTraceInfo *trcinfo;
        } n;
        struct {
            uint16      nvars;    
            uint16      nupvars;  

            uint16       skipmin; 


            JSPackedBool wrapper; 





            JSScript    *script;  
            JSLocalNames names;   
        } i;
    } u;
    JSAtom          *atom;        

    bool optimizedClosure() const { return FUN_KIND(this) > JSFUN_INTERPRETED; }
    bool needsWrapper()     const { return FUN_NULL_CLOSURE(this) && u.i.skipmin != 0; }
    bool isInterpreted()    const { return FUN_INTERPRETED(this); }
    bool isFastNative()     const { return !!(flags & JSFUN_FAST_NATIVE); }
    bool isHeavyweight()    const { return JSFUN_HEAVYWEIGHT_TEST(flags); }
    unsigned minArgs()      const { return FUN_MINARGS(this); }

    uintN countVars() const {
        JS_ASSERT(FUN_INTERPRETED(this));
        return u.i.nvars;
    }

    
    enum { MAX_ARGS_AND_VARS = 2 * ((1U << 16) - 1) };

    uintN countArgsAndVars() const {
        JS_ASSERT(FUN_INTERPRETED(this));
        return nargs + u.i.nvars;
    }

    uintN countLocalNames() const {
        JS_ASSERT(FUN_INTERPRETED(this));
        return countArgsAndVars() + u.i.nupvars;
    }

    bool hasLocalNames() const {
        JS_ASSERT(FUN_INTERPRETED(this));
        return countLocalNames() != 0;
    }

    int sharpSlotBase(JSContext *cx);

    



    JSAtom *findDuplicateFormal() const;

    uint32 countInterpretedReservedSlots() const;

    bool mightEscape() const {
        return FUN_INTERPRETED(this) && (FUN_FLAT_CLOSURE(this) || u.i.nupvars == 0);
    }

    js::FunObjTag funObjVal() {
        return js::FunObjTag(*this);
    }
};

JS_STATIC_ASSERT(sizeof(JSFunction) % JS_GCTHING_ALIGN == 0);






#ifdef JS_TRACER

# define JS_TN(name,fastcall,nargs,flags,trcinfo)                             \
    JS_FN(name, JS_DATA_TO_FUNC_PTR(JSNative, trcinfo), nargs,                \
          (flags) | JSFUN_FAST_NATIVE | JSFUN_STUB_GSOPS | JSFUN_TRCINFO)
#else
# define JS_TN(name,fastcall,nargs,flags,trcinfo)                             \
    JS_FN(name, fastcall, nargs, flags)
#endif














extern js::Class js_ArgumentsClass;

inline bool
JSObject::isArguments() const
{
    return getClass() == &js_ArgumentsClass;
}

extern JS_FRIEND_DATA(js::Class) js_CallClass;
extern js::Class js_DeclEnvClass;
extern const uint32 CALL_CLASS_FIXED_RESERVED_SLOTS;

inline bool
JSObject::isFunction() const
{
    return getClass() == &js_FunctionClass;
}

#define VALUE_IS_FUNCTION(cx, v) (Valueify(v).isFunObj())





#define GET_FUNCTION_PRIVATE(cx, funobj)                                      \
    (JS_ASSERT((funobj)->isFunction()),                                       \
     (JSFunction *) (funobj)->getPrivate())

namespace js {






inline bool
IsInternalFunctionObject(JSObject *funobj)
{
    JS_ASSERT(funobj->isFunction());
    JSFunction *fun = (JSFunction *) funobj->getPrivate();
    return funobj == fun && (fun->flags & JSFUN_LAMBDA) && !funobj->getParent();
}
    
struct ArgsPrivateNative;

inline ArgsPrivateNative *
GetArgsPrivateNative(JSObject *argsobj)
{
    JS_ASSERT(argsobj->isArguments());
    uintptr_t p = (uintptr_t) argsobj->getPrivate();
    return (ArgsPrivateNative *) (p & 2 ? p & ~2 : NULL);
}

} 

extern JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitArgumentsClass(JSContext *cx, JSObject *obj);

extern JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, js::Native native, uintN nargs,
               uintN flags, JSObject *parent, JSAtom *atom);

extern void
js_TraceFunction(JSTracer *trc, JSFunction *fun);

extern void
js_FinalizeFunction(JSContext *cx, JSFunction *fun);

extern JSObject * JS_FASTCALL
js_CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent,
                       JSObject *proto);

inline JSObject *
CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent)
{
    JS_ASSERT(parent);
    JSObject *proto;
    if (!js_GetClassPrototype(cx, parent, JSProto_Function, &proto))
        return NULL;
    return js_CloneFunctionObject(cx, fun, parent, proto);
}

extern JS_REQUIRES_STACK JSObject *
js_NewFlatClosure(JSContext *cx, JSFunction *fun);

extern JS_REQUIRES_STACK JSObject *
js_NewDebuggableFlatClosure(JSContext *cx, JSFunction *fun);

extern JSFunction *
js_DefineFunction(JSContext *cx, JSObject *obj, JSAtom *atom, js::Native native,
                  uintN nargs, uintN flags);






#define JSV2F_CONSTRUCT         JSINVOKE_CONSTRUCT
#define JSV2F_SEARCH_STACK      0x10000

extern JSFunction *
js_ValueToFunction(JSContext *cx, const js::Value *vp, uintN flags);

extern JSObject *
js_ValueToFunctionObject(JSContext *cx, js::Value *vp, uintN flags);

extern JSObject *
js_ValueToCallableObject(JSContext *cx, js::Value *vp, uintN flags);

extern void
js_ReportIsNotFunction(JSContext *cx, const js::Value *vp, uintN flags);

extern JSObject *
js_GetCallObject(JSContext *cx, JSStackFrame *fp);

extern JSObject * JS_FASTCALL
js_CreateCallObjectOnTrace(JSContext *cx, JSFunction *fun, JSObject *callee, JSObject *scopeChain);

extern void
js_PutCallObject(JSContext *cx, JSStackFrame *fp);

extern JSBool JS_FASTCALL
js_PutCallObjectOnTrace(JSContext *cx, JSObject *scopeChain, uint32 nargs,
                        js::Value *argv, uint32 nvars, js::Value *slots);

extern JSFunction *
js_GetCallObjectFunction(JSObject *obj);

extern JSBool
js_GetCallArg(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
js_GetCallVar(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
SetCallArg(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
SetCallVar(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);





extern JSBool
js_GetCallVarChecked(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
js_GetArgsValue(JSContext *cx, JSStackFrame *fp, js::Value *vp);

extern JSBool
js_GetArgsProperty(JSContext *cx, JSStackFrame *fp, jsid id, js::Value *vp);

extern JSObject *
js_GetArgsObject(JSContext *cx, JSStackFrame *fp);

extern void
js_PutArgsObject(JSContext *cx, JSStackFrame *fp);

inline bool
js_IsNamedLambda(JSFunction *fun) { return (fun->flags & JSFUN_LAMBDA) && fun->atom; }











const uint32 JS_ARGS_LENGTH_MAX = JS_BIT(19) - 1024;






JS_STATIC_ASSERT(JS_ARGS_LENGTH_MAX <= JS_BIT(30));
JS_STATIC_ASSERT(((JS_ARGS_LENGTH_MAX << 1) | 1) <= JSVAL_INT_MAX);

extern JSBool
js_XDRFunctionObject(JSXDRState *xdr, JSObject **objp);

typedef enum JSLocalKind {
    JSLOCAL_NONE,
    JSLOCAL_ARG,
    JSLOCAL_VAR,
    JSLOCAL_CONST,
    JSLOCAL_UPVAR
} JSLocalKind;

extern JSBool
js_AddLocal(JSContext *cx, JSFunction *fun, JSAtom *atom, JSLocalKind kind);







extern JSLocalKind
js_LookupLocal(JSContext *cx, JSFunction *fun, JSAtom *atom, uintN *indexp);



















extern jsuword *
js_GetLocalNameArray(JSContext *cx, JSFunction *fun, struct JSArenaPool *pool);

#define JS_LOCAL_NAME_TO_ATOM(nameWord)                                       \
    ((JSAtom *) ((nameWord) & ~(jsuword) 1))

#define JS_LOCAL_NAME_IS_CONST(nameWord)                                      \
    ((((nameWord) & (jsuword) 1)) != 0)

extern void
js_FreezeLocalNames(JSContext *cx, JSFunction *fun);

extern JSBool
js_fun_apply(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_fun_call(JSContext *cx, uintN argc, js::Value *vp);

#endif 
