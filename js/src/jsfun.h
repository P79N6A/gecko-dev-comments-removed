






































#ifndef jsfun_h___
#define jsfun_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsobj.h"
#include "jsatom.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsopcode.h"

#include "gc/Barrier.h"



































#define JSFUN_JOINABLE      0x0001  /* function is null closure that does not
                                       appear to call itself via its own name
                                       or arguments.callee */

#define JSFUN_PROTOTYPE     0x0800  /* function is Function.prototype for some
                                       global object */

#define JSFUN_EXPR_CLOSURE  0x1000  /* expression closure: function(x) x*x */
#define JSFUN_EXTENDED      0x2000  /* structure is FunctionExtended */
#define JSFUN_INTERPRETED   0x4000  /* use u.i if kind >= this value else u.n */
#define JSFUN_FLAT_CLOSURE  0x8000  /* flat (aka "display") closure */
#define JSFUN_NULL_CLOSURE  0xc000  /* null closure entrains no scope chain */
#define JSFUN_KINDMASK      0xc000  /* encode interp vs. native and closure
                                       optimization level -- see above */

namespace js { class FunctionExtended; }

struct JSFunction : public JSObject
{
    uint16          nargs;        

    uint16          flags;        
    union U {
        struct Native {
            js::Native  native;   
            js::Class   *clasp;   

        } n;
        struct Scripted {
            JSScript    *script_; 

            JSObject    *env;     
        } i;
        void            *nativeOrScript;
    } u;
    JSAtom          *atom;        

    bool optimizedClosure()  const { return kind() > JSFUN_INTERPRETED; }
    bool isInterpreted()     const { return kind() >= JSFUN_INTERPRETED; }
    bool isNative()          const { return !isInterpreted(); }
    bool isConstructor()     const { return flags & JSFUN_CONSTRUCTOR; }
    bool isHeavyweight()     const { return JSFUN_HEAVYWEIGHT_TEST(flags); }
    bool isNullClosure()     const { return kind() == JSFUN_NULL_CLOSURE; }
    bool isFlatClosure()     const { return kind() == JSFUN_FLAT_CLOSURE; }
    bool isFunctionPrototype() const { return flags & JSFUN_PROTOTYPE; }
    bool isInterpretedConstructor() const { return isInterpreted() && !isFunctionPrototype(); }

    uint16 kind()            const { return flags & JSFUN_KINDMASK; }
    void setKind(uint16 k) {
        JS_ASSERT(!(k & ~JSFUN_KINDMASK));
        flags = (flags & ~JSFUN_KINDMASK) | k;
    }

    
    inline bool inStrictMode() const;

    void setArgCount(uint16 nargs) {
        JS_ASSERT(this->nargs == 0);
        this->nargs = nargs;
    }

    
    enum { MAX_ARGS_AND_VARS = 2 * ((1U << 16) - 1) };

#define JS_LOCAL_NAME_TO_ATOM(nameWord)  ((JSAtom *) ((nameWord) & ~(jsuword) 1))
#define JS_LOCAL_NAME_IS_CONST(nameWord) ((((nameWord) & (jsuword) 1)) != 0)

    bool mightEscape() const {
        return isInterpreted() && (isFlatClosure() || !script()->bindings.hasUpvars());
    }

    bool joinable() const {
        return flags & JSFUN_JOINABLE;
    }

    



    inline JSObject *environment() const;
    inline void setEnvironment(JSObject *obj);

    static inline size_t offsetOfEnvironment() { return offsetof(JSFunction, u.i.env); }

    inline void setJoinable();

    js::HeapPtrScript &script() const {
        JS_ASSERT(isInterpreted());
        return *(js::HeapPtrScript *)&u.i.script_;
    }

    inline void setScript(JSScript *script_);
    inline void initScript(JSScript *script_);

    JSScript *maybeScript() const {
        return isInterpreted() ? script().get() : NULL;
    }

    JSNative native() const {
        JS_ASSERT(isNative());
        return u.n.native;
    }

    JSNative maybeNative() const {
        return isInterpreted() ? NULL : native();
    }

    static uintN offsetOfNativeOrScript() {
        JS_STATIC_ASSERT(offsetof(U, n.native) == offsetof(U, i.script_));
        JS_STATIC_ASSERT(offsetof(U, n.native) == offsetof(U, nativeOrScript));
        return offsetof(JSFunction, u.nativeOrScript);
    }

    js::Class *getConstructorClass() const {
        JS_ASSERT(isNative());
        return u.n.clasp;
    }

    void setConstructorClass(js::Class *clasp) {
        JS_ASSERT(isNative());
        u.n.clasp = clasp;
    }

#if JS_BITS_PER_WORD == 32
    static const js::gc::AllocKind FinalizeKind = js::gc::FINALIZE_OBJECT2;
    static const js::gc::AllocKind ExtendedFinalizeKind = js::gc::FINALIZE_OBJECT4;
#else
    static const js::gc::AllocKind FinalizeKind = js::gc::FINALIZE_OBJECT4;
    static const js::gc::AllocKind ExtendedFinalizeKind = js::gc::FINALIZE_OBJECT8;
#endif

    inline void trace(JSTracer *trc);

    

    inline bool initBoundFunction(JSContext *cx, const js::Value &thisArg,
                                  const js::Value *args, uintN argslen);

    inline JSObject *getBoundFunctionTarget() const;
    inline const js::Value &getBoundFunctionThis() const;
    inline const js::Value &getBoundFunctionArgument(uintN which) const;
    inline size_t getBoundFunctionArgumentCount() const;

  private:
    inline js::FunctionExtended *toExtended();
    inline const js::FunctionExtended *toExtended() const;

    inline bool isExtended() const {
        JS_STATIC_ASSERT(FinalizeKind != ExtendedFinalizeKind);
        JS_ASSERT(!!(flags & JSFUN_EXTENDED) == (getAllocKind() == ExtendedFinalizeKind));
        return !!(flags & JSFUN_EXTENDED);
    }

  public:
    

    inline void initializeExtended();

    inline void setExtendedSlot(size_t which, const js::Value &val);
    inline const js::Value &getExtendedSlot(size_t which) const;

    




    static const uint32 FLAT_CLOSURE_UPVARS_SLOT = 0;

    static inline size_t getFlatClosureUpvarsOffset();

    inline js::Value getFlatClosureUpvar(uint32 i) const;
    inline void setFlatClosureUpvar(uint32 i, const js::Value &v);
    inline void initFlatClosureUpvar(uint32 i, const js::Value &v);

  private:
    inline bool hasFlatClosureUpvars() const;
    inline js::HeapValue *getFlatClosureUpvars() const;
  public:

    
    inline void finalizeUpvars();

    
    static const uint32 METHOD_PROPERTY_SLOT = 0;

    
    static const uint32 METHOD_OBJECT_SLOT = 0;

    
    inline bool isClonedMethod() const;

    
    inline JSObject *methodObj() const;
    inline void setMethodObj(JSObject& obj);

    




    inline JSAtom *methodAtom() const;
    inline void setMethodAtom(JSAtom *atom);
};

inline JSFunction *
JSObject::toFunction()
{
    JS_ASSERT(JS_ObjectIsFunction(NULL, this));
    return static_cast<JSFunction *>(this);
}

inline const JSFunction *
JSObject::toFunction() const
{
    JS_ASSERT(JS_ObjectIsFunction(NULL, const_cast<JSObject *>(this)));
    return static_cast<const JSFunction *>(this);
}






#ifdef JS_TRACER

# define JS_TN(name,fastcall,nargs,flags,trcinfo)                             \
    JS_FN(name, JS_DATA_TO_FUNC_PTR(Native, trcinfo), nargs,                  \
          (flags) | JSFUN_STUB_GSOPS | JSFUN_TRCINFO)
#else
# define JS_TN(name,fastcall,nargs,flags,trcinfo)                             \
    JS_FN(name, fastcall, nargs, flags)
#endif

extern JSString *
fun_toStringHelper(JSContext *cx, JSObject *obj, uintN indent);

extern JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, JSNative native, uintN nargs,
               uintN flags, JSObject *parent, JSAtom *atom,
               js::gc::AllocKind kind = JSFunction::FinalizeKind);

extern void
js_FinalizeFunction(JSContext *cx, JSFunction *fun);

extern JSFunction * JS_FASTCALL
js_CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent, JSObject *proto,
                       js::gc::AllocKind kind = JSFunction::FinalizeKind);

extern JSFunction * JS_FASTCALL
js_AllocFlatClosure(JSContext *cx, JSFunction *fun, JSObject *scopeChain);

extern JSFunction *
js_NewFlatClosure(JSContext *cx, JSFunction *fun, JSOp op, size_t oplen);

extern JSFunction *
js_DefineFunction(JSContext *cx, JSObject *obj, jsid id, JSNative native,
                  uintN nargs, uintN flags,
                  js::gc::AllocKind kind = JSFunction::FinalizeKind);




#define JSV2F_CONSTRUCT         INITIAL_CONSTRUCT
#define JSV2F_SEARCH_STACK      0x10000

extern JSFunction *
js_ValueToFunction(JSContext *cx, const js::Value *vp, uintN flags);

extern JSObject *
js_ValueToCallableObject(JSContext *cx, js::Value *vp, uintN flags);

extern void
js_ReportIsNotFunction(JSContext *cx, const js::Value *vp, uintN flags);

extern JSObject * JS_FASTCALL
js_CreateCallObjectOnTrace(JSContext *cx, JSFunction *fun, JSObject *callee, JSObject *scopeChain);

extern void
js_PutCallObject(js::StackFrame *fp);

extern JSBool JS_FASTCALL
js_PutCallObjectOnTrace(JSObject *scopeChain, uint32 nargs, js::Value *argv,
                        uint32 nvars, js::Value *slots);

namespace js {

CallObject *
CreateFunCallObject(JSContext *cx, StackFrame *fp);

CallObject *
CreateEvalCallObject(JSContext *cx, StackFrame *fp);

extern JSBool
GetCallArg(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
GetCallVar(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
GetCallUpvar(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
SetCallArg(JSContext *cx, JSObject *obj, jsid id, JSBool strict, js::Value *vp);

extern JSBool
SetCallVar(JSContext *cx, JSObject *obj, jsid id, JSBool strict, js::Value *vp);

extern JSBool
SetCallUpvar(JSContext *cx, JSObject *obj, jsid id, JSBool strict, js::Value *vp);






class FunctionExtended : public JSFunction
{
    friend struct JSFunction;

    
    HeapValue extendedSlots[2];
};

} 

inline js::FunctionExtended *
JSFunction::toExtended()
{
    JS_ASSERT(isExtended());
    return static_cast<js::FunctionExtended *>(this);
}

inline const js::FunctionExtended *
JSFunction::toExtended() const
{
    JS_ASSERT(isExtended());
    return static_cast<const js::FunctionExtended *>(this);
}

inline void
JSFunction::initializeExtended()
{
    JS_ASSERT(isExtended());

    JS_STATIC_ASSERT(JS_ARRAY_LENGTH(toExtended()->extendedSlots) == 2);
    toExtended()->extendedSlots[0].init(js::UndefinedValue());
    toExtended()->extendedSlots[1].init(js::UndefinedValue());
}

extern JSBool
js_GetArgsValue(JSContext *cx, js::StackFrame *fp, js::Value *vp);











extern JSObject *
js_GetArgsObject(JSContext *cx, js::StackFrame *fp);

extern void
js_PutArgsObject(js::StackFrame *fp);

inline bool
js_IsNamedLambda(JSFunction *fun) { return (fun->flags & JSFUN_LAMBDA) && fun->atom; }

extern JSBool
js_XDRFunctionObject(JSXDRState *xdr, JSObject **objp);

extern JSBool
js_fun_apply(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_fun_call(JSContext *cx, uintN argc, js::Value *vp);

#endif 
