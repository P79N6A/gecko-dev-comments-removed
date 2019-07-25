






































#ifndef jsfun_h___
#define jsfun_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsobj.h"
#include "jsatom.h"
#include "jsstr.h"
#include "jsopcode.h"



































#define JSFUN_JOINABLE      0x0001  /* function is null closure that does not
                                       appear to call itself via its own name
                                       or arguments.callee */

#define JSFUN_PROTOTYPE     0x0800  /* function is Function.prototype for some
                                       global object */

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
#define FUN_SCRIPT(fun)      (FUN_INTERPRETED(fun) ? (fun)->u.i.script : NULL)
#define FUN_CLASP(fun)       (JS_ASSERT(!FUN_INTERPRETED(fun)),               \
                              fun->u.n.clasp)
#define FUN_TRCINFO(fun)     (JS_ASSERT(!FUN_INTERPRETED(fun)),               \
                              JS_ASSERT((fun)->flags & JSFUN_TRCINFO),        \
                              fun->u.n.trcinfo)
















enum JSLocalKind {
    JSLOCAL_NONE,
    JSLOCAL_ARG,
    JSLOCAL_VAR,
    JSLOCAL_CONST,
    JSLOCAL_UPVAR
};

struct JSFunction : public JSObject_Slots2
{
    

    uint16          nargs;        

    uint16          flags;        
    union U {
        struct {
            js::Native  native;   
            js::Class   *clasp;   

            JSNativeTraceInfo *trcinfo;
        } n;
        struct Scripted {
            JSScript    *script;  
            uint16      nvars;    
            uint16      nupvars;  

            uint16       skipmin; 


            JSPackedBool wrapper; 





            js::Shape   *names;   
        } i;
        void            *nativeOrScript;
    } u;
    JSAtom          *atom;        

    bool optimizedClosure()  const { return FUN_KIND(this) > JSFUN_INTERPRETED; }
    bool needsWrapper()      const { return FUN_NULL_CLOSURE(this) && u.i.skipmin != 0; }
    bool isInterpreted()     const { return FUN_INTERPRETED(this); }
    bool isNative()          const { return !FUN_INTERPRETED(this); }
    bool isConstructor()     const { return flags & JSFUN_CONSTRUCTOR; }
    bool isHeavyweight()     const { return JSFUN_HEAVYWEIGHT_TEST(flags); }
    bool isFlatClosure()     const { return FUN_KIND(this) == JSFUN_FLAT_CLOSURE; }

    bool isFunctionPrototype() const { return flags & JSFUN_PROTOTYPE; }

    
    inline bool inStrictMode() const;

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

    uint32 countUpvarSlots() const;

    const js::Shape *lastArg() const;
    const js::Shape *lastVar() const;
    const js::Shape *lastUpvar() const { return u.i.names; }

    




    bool addLocal(JSContext *cx, JSAtom *atom, JSLocalKind kind);

    





    JSLocalKind lookupLocal(JSContext *cx, JSAtom *atom, uintN *indexp);

    
















    jsuword *getLocalNameArray(JSContext *cx, struct JSArenaPool *pool);

    void freezeLocalNames(JSContext *cx);

    



    JSAtom *findDuplicateFormal() const;

#define JS_LOCAL_NAME_TO_ATOM(nameWord)  ((JSAtom *) ((nameWord) & ~(jsuword) 1))
#define JS_LOCAL_NAME_IS_CONST(nameWord) ((((nameWord) & (jsuword) 1)) != 0)

    bool mightEscape() const {
        return FUN_INTERPRETED(this) && (FUN_FLAT_CLOSURE(this) || u.i.nupvars == 0);
    }

    bool joinable() const {
        return flags & JSFUN_JOINABLE;
    }

    JSObject &compiledFunObj() {
        return *this;
    }

  private:
    





    enum {
        METHOD_ATOM_SLOT  = JSSLOT_FUN_METHOD_ATOM
    };

  public:
    void setJoinable() {
        JS_ASSERT(FUN_INTERPRETED(this));
        getSlotRef(METHOD_ATOM_SLOT).setNull();
        flags |= JSFUN_JOINABLE;
    }

    




    JSAtom *methodAtom() const {
        return (joinable() && getSlot(METHOD_ATOM_SLOT).isString())
               ? STRING_TO_ATOM(getSlot(METHOD_ATOM_SLOT).toString())
               : NULL;
    }

    void setMethodAtom(JSAtom *atom) {
        JS_ASSERT(joinable());
        getSlotRef(METHOD_ATOM_SLOT).setString(ATOM_TO_STRING(atom));
    }

    js::Native maybeNative() const {
        return isInterpreted() ? NULL : u.n.native;
    }

    JSScript *script() const {
        JS_ASSERT(isInterpreted());
        return u.i.script;
    }

    static uintN offsetOfNativeOrScript() {
        JS_STATIC_ASSERT(offsetof(U, n.native) == offsetof(U, i.script));
        JS_STATIC_ASSERT(offsetof(U, n.native) == offsetof(U, nativeOrScript));
        return offsetof(JSFunction, u.nativeOrScript);
    }

    
    static const uint32 CLASS_RESERVED_SLOTS = JSObject::FUN_CLASS_RESERVED_SLOTS;
};






#ifdef JS_TRACER

# define JS_TN(name,fastcall,nargs,flags,trcinfo)                             \
    JS_FN(name, JS_DATA_TO_FUNC_PTR(Native, trcinfo), nargs,                  \
          (flags) | JSFUN_STUB_GSOPS | JSFUN_TRCINFO)
#else
# define JS_TN(name,fastcall,nargs,flags,trcinfo)                             \
    JS_FN(name, fastcall, nargs, flags)
#endif















extern js::Class js_ArgumentsClass;

namespace js {

extern Class StrictArgumentsClass;

struct ArgumentsData {
    js::Value   callee;
    js::Value   slots[1];
};

}

inline bool
JSObject::isNormalArguments() const
{
    return getClass() == &js_ArgumentsClass;
}

inline bool
JSObject::isStrictArguments() const
{
    return getClass() == &js::StrictArgumentsClass;
}

inline bool
JSObject::isArguments() const
{
    return isNormalArguments() || isStrictArguments();
}

#define JS_ARGUMENTS_OBJECT_ON_TRACE ((void *)0xa126)

extern JS_PUBLIC_DATA(js::Class) js_CallClass;
extern JS_PUBLIC_DATA(js::Class) js_FunctionClass;
extern js::Class js_DeclEnvClass;

inline bool
JSObject::isCall() const
{
    return getClass() == &js_CallClass;
}

inline bool
JSObject::isFunction() const
{
    return getClass() == &js_FunctionClass;
}

inline JSFunction *
JSObject::getFunctionPrivate() const
{
    JS_ASSERT(isFunction());
    return reinterpret_cast<JSFunction *>(getPrivate());
}

namespace js {




#define VALUE_IS_FUNCTION(cx, v)                                              \
    (!JSVAL_IS_PRIMITIVE(v) && JSVAL_TO_OBJECT(v)->isFunction())

static JS_ALWAYS_INLINE bool
IsFunctionObject(const js::Value &v)
{
    return v.isObject() && v.toObject().isFunction();
}

static JS_ALWAYS_INLINE bool
IsFunctionObject(const js::Value &v, JSObject **funobj)
{
    return v.isObject() && (*funobj = &v.toObject())->isFunction();
}

static JS_ALWAYS_INLINE bool
IsFunctionObject(const js::Value &v, JSFunction **fun)
{
    JSObject *funobj;
    bool b = IsFunctionObject(v, &funobj);
    if (b)
        *fun = funobj->getFunctionPrivate();
    return b;
}

extern JS_ALWAYS_INLINE bool
SameTraceType(const Value &lhs, const Value &rhs)
{
    return SameType(lhs, rhs) &&
           (lhs.isPrimitive() ||
            lhs.toObject().isFunction() == rhs.toObject().isFunction());
}





#define GET_FUNCTION_PRIVATE(cx, funobj)                                      \
    (JS_ASSERT((funobj)->isFunction()),                                       \
     (JSFunction *) (funobj)->getPrivate())






inline bool
IsInternalFunctionObject(JSObject *funobj)
{
    JS_ASSERT(funobj->isFunction());
    JSFunction *fun = (JSFunction *) funobj->getPrivate();
    return funobj == fun && (fun->flags & JSFUN_LAMBDA) && !funobj->getParent();
}
    

static JS_ALWAYS_INLINE bool
IsConstructing(const Value *vp)
{
#ifdef DEBUG
    JSObject *callee = &JS_CALLEE(cx, vp).toObject();
    if (callee->isFunction()) {
        JSFunction *fun = callee->getFunctionPrivate();
        JS_ASSERT((fun->flags & JSFUN_CONSTRUCTOR) != 0);
    } else {
        JS_ASSERT(callee->getClass()->construct != NULL);
    }
#endif
    return vp[1].isMagic();
}

static JS_ALWAYS_INLINE bool
IsConstructing_PossiblyWithGivenThisObject(const Value *vp, JSObject **ctorThis)
{
#ifdef DEBUG
    JSObject *callee = &JS_CALLEE(cx, vp).toObject();
    if (callee->isFunction()) {
        JSFunction *fun = callee->getFunctionPrivate();
        JS_ASSERT((fun->flags & JSFUN_CONSTRUCTOR) != 0);
    } else {
        JS_ASSERT(callee->getClass()->construct != NULL);
    }
#endif
    bool isCtor = vp[1].isMagic();
    if (isCtor)
        *ctorThis = vp[1].getMagicObjectOrNullPayload();
    return isCtor;
}

inline const char *
GetFunctionNameBytes(JSContext *cx, JSFunction *fun, JSAutoByteString *bytes)
{
    if (fun->atom)
        return bytes->encode(cx, ATOM_TO_STRING(fun->atom));
    return js_anonymous_str;
}

} 

extern JSString *
fun_toStringHelper(JSContext *cx, JSObject *obj, uintN indent);

extern JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, js::Native native, uintN nargs,
               uintN flags, JSObject *parent, JSAtom *atom);

extern JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitArgumentsClass(JSContext *cx, JSObject *obj);

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

extern JSObject * JS_FASTCALL
js_AllocFlatClosure(JSContext *cx, JSFunction *fun, JSObject *scopeChain);

extern JS_REQUIRES_STACK JSObject *
js_NewFlatClosure(JSContext *cx, JSFunction *fun, JSOp op, size_t oplen);

extern JS_REQUIRES_STACK JSObject *
js_NewDebuggableFlatClosure(JSContext *cx, JSFunction *fun);

extern JSFunction *
js_DefineFunction(JSContext *cx, JSObject *obj, jsid id, js::Native native,
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

namespace js {

extern JSBool
GetCallArg(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
GetCallVar(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);





extern JSBool
GetCallVarChecked(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
SetCallArg(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
SetCallVar(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

} 

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

extern JSBool
js_fun_apply(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_fun_call(JSContext *cx, uintN argc, js::Value *vp);

#endif 
