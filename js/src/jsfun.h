






































#ifndef jsfun_h___
#define jsfun_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsobj.h"
#include "jsatom.h"
#include "jsscript.h"
#include "jsstr.h"

#include "gc/Barrier.h"

















#define JSFUN_JOINABLE      0x0001  /* function is null closure that does not
                                       appear to call itself via its own name
                                       or arguments.callee */

#define JSFUN_PROTOTYPE     0x0800  /* function is Function.prototype for some
                                       global object */

#define JSFUN_EXPR_CLOSURE  0x1000  /* expression closure: function(x) x*x */
#define JSFUN_EXTENDED      0x2000  /* structure is FunctionExtended */
#define JSFUN_INTERPRETED   0x4000  /* use u.i if kind >= this value else u.n */
#define JSFUN_NULL_CLOSURE  0x8000  /* null closure entrains no scope chain */
#define JSFUN_KINDMASK      0xc000  /* encode interp vs. native and closure
                                       optimization level -- see above */

namespace js { class FunctionExtended; }

struct JSFunction : public JSObject
{
    uint16_t        nargs;        

    uint16_t        flags;        
    union U {
        struct Native {
            js::Native  native;   
            js::Class   *clasp;   

        } n;
        struct Scripted {
            JSScript    *script_; 

            JSObject    *env_;    

        } i;
        void            *nativeOrScript;
    } u;
    js::HeapPtrAtom  atom;        

    bool optimizedClosure()  const { return kind() > JSFUN_INTERPRETED; }
    bool isInterpreted()     const { return kind() >= JSFUN_INTERPRETED; }
    bool isNative()          const { return !isInterpreted(); }
    bool isNativeConstructor() const { return flags & JSFUN_CONSTRUCTOR; }
    bool isHeavyweight()     const { return JSFUN_HEAVYWEIGHT_TEST(flags); }
    bool isNullClosure()     const { return kind() == JSFUN_NULL_CLOSURE; }
    bool isFunctionPrototype() const { return flags & JSFUN_PROTOTYPE; }
    bool isInterpretedConstructor() const { return isInterpreted() && !isFunctionPrototype(); }

    uint16_t kind()          const { return flags & JSFUN_KINDMASK; }
    void setKind(uint16_t k) {
        JS_ASSERT(!(k & ~JSFUN_KINDMASK));
        flags = (flags & ~JSFUN_KINDMASK) | k;
    }

    
    inline bool inStrictMode() const;

    void setArgCount(uint16_t nargs) {
        JS_ASSERT(this->nargs == 0);
        this->nargs = nargs;
    }

    
    enum { MAX_ARGS_AND_VARS = 2 * ((1U << 16) - 1) };

#define JS_LOCAL_NAME_TO_ATOM(nameWord)  ((JSAtom *) ((nameWord) & ~uintptr_t(1)))
#define JS_LOCAL_NAME_IS_CONST(nameWord) ((((nameWord) & uintptr_t(1))) != 0)

    bool mightEscape() const {
        return isInterpreted() && isNullClosure();
    }

    bool joinable() const {
        return flags & JSFUN_JOINABLE;
    }

    



    inline JSObject *environment() const;
    inline void setEnvironment(JSObject *obj);
    inline void initEnvironment(JSObject *obj);

    static inline size_t offsetOfEnvironment() { return offsetof(JSFunction, u.i.env_); }

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

    static unsigned offsetOfNativeOrScript() {
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
# ifdef JS_THREADSAFE
    static const js::gc::AllocKind FinalizeKind = js::gc::FINALIZE_OBJECT2_BACKGROUND;
    static const js::gc::AllocKind ExtendedFinalizeKind = js::gc::FINALIZE_OBJECT4_BACKGROUND;
# else
    static const js::gc::AllocKind FinalizeKind = js::gc::FINALIZE_OBJECT2;
    static const js::gc::AllocKind ExtendedFinalizeKind = js::gc::FINALIZE_OBJECT4;
# endif
#else
# ifdef JS_THREADSAFE
    static const js::gc::AllocKind FinalizeKind = js::gc::FINALIZE_OBJECT4_BACKGROUND;
    static const js::gc::AllocKind ExtendedFinalizeKind = js::gc::FINALIZE_OBJECT8_BACKGROUND;
# else
    static const js::gc::AllocKind FinalizeKind = js::gc::FINALIZE_OBJECT4;
    static const js::gc::AllocKind ExtendedFinalizeKind = js::gc::FINALIZE_OBJECT8;
# endif
#endif

    inline void trace(JSTracer *trc);

    

    inline bool initBoundFunction(JSContext *cx, const js::Value &thisArg,
                                  const js::Value *args, unsigned argslen);

    inline JSObject *getBoundFunctionTarget() const;
    inline const js::Value &getBoundFunctionThis() const;
    inline const js::Value &getBoundFunctionArgument(unsigned which) const;
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

    
    static const uint32_t METHOD_PROPERTY_SLOT = 0;

    
    static const uint32_t METHOD_OBJECT_SLOT = 1;

    
    inline bool isClonedMethod() const;

    
    inline JSObject *methodObj() const;
    inline void setMethodObj(JSObject& obj);

    




    inline JSAtom *methodAtom() const;
    inline void setMethodAtom(JSAtom *atom);

  private:
    



    inline JSFunction *toFunction() MOZ_DELETE;
    inline const JSFunction *toFunction() const MOZ_DELETE;
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

extern JSString *
fun_toStringHelper(JSContext *cx, JSObject *obj, unsigned indent);

extern JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, JSNative native, unsigned nargs,
               unsigned flags, js::HandleObject parent, JSAtom *atom,
               js::gc::AllocKind kind = JSFunction::FinalizeKind);

extern JSFunction * JS_FASTCALL
js_CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent, JSObject *proto,
                       js::gc::AllocKind kind = JSFunction::FinalizeKind);

extern JSFunction *
js_DefineFunction(JSContext *cx, js::HandleObject obj, jsid id, JSNative native,
                  unsigned nargs, unsigned flags,
                  js::gc::AllocKind kind = JSFunction::FinalizeKind);




#define JSV2F_CONSTRUCT         INITIAL_CONSTRUCT
#define JSV2F_SEARCH_STACK      0x10000

extern JSFunction *
js_ValueToFunction(JSContext *cx, const js::Value *vp, unsigned flags);

extern JSObject *
js_ValueToCallableObject(JSContext *cx, js::Value *vp, unsigned flags);

extern void
js_ReportIsNotFunction(JSContext *cx, const js::Value *vp, unsigned flags);

extern void
js_PutCallObject(js::StackFrame *fp);

namespace js {






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

extern void
js_PutArgsObject(js::StackFrame *fp);

inline bool
js_IsNamedLambda(JSFunction *fun) { return (fun->flags & JSFUN_LAMBDA) && fun->atom; }

namespace js {

template<XDRMode mode>
bool
XDRInterpretedFunction(XDRState<mode> *xdr, JSObject **objp, JSScript *parentScript);

} 

extern JSBool
js_fun_apply(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_fun_call(JSContext *cx, unsigned argc, js::Value *vp);

#endif 
