





#ifndef jsfun_h___
#define jsfun_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsobj.h"
#include "jsatom.h"
#include "jsscript.h"
#include "jsstr.h"

#include "gc/Barrier.h"

















#define JSFUN_PROTOTYPE     0x0800  /* function is Function.prototype for some
                                       global object */

#define JSFUN_EXPR_CLOSURE  0x1000  /* expression closure: function(x) x*x */
#define JSFUN_EXTENDED      0x2000  /* structure is FunctionExtended */




#define JSFUN_INTERPRETED   0x4000  /* use u.i if kind >= this value else u.native */

#define JSFUN_HAS_GUESSED_ATOM  0x8000  /* function had no explicit name, but a
                                           name was guessed for it anyway */

namespace js { class FunctionExtended; }

struct JSFunction : public JSObject
{
    uint16_t        nargs;        

    uint16_t        flags;        
    union U {
        class Native {
            friend struct JSFunction;
            js::Native          native;       
            const JSJitInfo     *jitinfo;     


        } n;
        struct Scripted {
            JSScript    *script_; 

            JSObject    *env_;    

        } i;
        void            *nativeOrScript;
    } u;
  private:
    js::HeapPtrAtom  atom_;       
  public:

    bool hasDefaults()       const { return flags & JSFUN_HAS_DEFAULTS; }
    bool hasRest()           const { return flags & JSFUN_HAS_REST; }
    bool hasGuessedAtom()    const { return flags & JSFUN_HAS_GUESSED_ATOM; }
    bool isInterpreted()     const { return flags & JSFUN_INTERPRETED; }
    bool isNative()          const { return !isInterpreted(); }
    bool isSelfHostedBuiltin() const { return flags & JSFUN_SELF_HOSTED; }
    bool isSelfHostedConstructor() const { return flags & JSFUN_SELF_HOSTED_CTOR; }
    bool isNativeConstructor() const { return flags & JSFUN_CONSTRUCTOR; }
    bool isHeavyweight()     const { return JSFUN_HEAVYWEIGHT_TEST(flags); }
    bool isFunctionPrototype() const { return flags & JSFUN_PROTOTYPE; }
    bool isInterpretedConstructor() const {
        return isInterpreted() && !isFunctionPrototype() &&
               (!isSelfHostedBuiltin() || isSelfHostedConstructor());
    }
    bool isNamedLambda()     const {
        return (flags & JSFUN_LAMBDA) && atom_ && !hasGuessedAtom();
    }

    
    inline bool inStrictMode() const;

    void setArgCount(uint16_t nargs) {
        JS_ASSERT(this->nargs == 0);
        this->nargs = nargs;
    }

    void setHasRest() {
        JS_ASSERT(!hasRest());
        this->flags |= JSFUN_HAS_REST;
    }

    void setHasDefaults() {
        JS_ASSERT(!hasDefaults());
        this->flags |= JSFUN_HAS_DEFAULTS;
    }

    JSAtom *atom() const { return hasGuessedAtom() ? NULL : atom_.get(); }
    void initAtom(JSAtom *atom) { atom_.init(atom); }
    JSAtom *displayAtom() const { return atom_; }

    void setGuessedAtom(JSAtom *atom) {
        JS_ASSERT(this->atom_ == NULL);
        JS_ASSERT(atom != NULL);
        JS_ASSERT(!hasGuessedAtom());
        this->atom_ = atom;
        this->flags |= JSFUN_HAS_GUESSED_ATOM;
    }

    
    enum { MAX_ARGS_AND_VARS = 2 * ((1U << 16) - 1) };

    



    inline JSObject *environment() const;
    inline void setEnvironment(JSObject *obj);
    inline void initEnvironment(JSObject *obj);

    static inline size_t offsetOfEnvironment() { return offsetof(JSFunction, u.i.env_); }

    JSScript *script() const {
        JS_ASSERT(isInterpreted());
        return *(js::HeapPtrScript *)&u.i.script_;
    }

    js::HeapPtrScript &mutableScript() {
        JS_ASSERT(isInterpreted());
        return *(js::HeapPtrScript *)&u.i.script_;
    }

    inline void setScript(JSScript *script_);
    inline void initScript(JSScript *script_);

    JSScript *maybeScript() const {
        return isInterpreted() ? script() : NULL;
    }

    JSNative native() const {
        JS_ASSERT(isNative());
        return u.n.native;
    }

    JSNative maybeNative() const {
        return isInterpreted() ? NULL : native();
    }

    inline void initNative(js::Native native, const JSJitInfo *jitinfo);
    inline const JSJitInfo *jitInfo() const;
    inline void setJitInfo(const JSJitInfo *data);

    static unsigned offsetOfNativeOrScript() {
        JS_STATIC_ASSERT(offsetof(U, n.native) == offsetof(U, i.script_));
        JS_STATIC_ASSERT(offsetof(U, n.native) == offsetof(U, nativeOrScript));
        return offsetof(JSFunction, u.nativeOrScript);
    }

#if JS_BITS_PER_WORD == 32
    static const js::gc::AllocKind FinalizeKind = js::gc::FINALIZE_OBJECT2_BACKGROUND;
    static const js::gc::AllocKind ExtendedFinalizeKind = js::gc::FINALIZE_OBJECT4_BACKGROUND;
#else
    static const js::gc::AllocKind FinalizeKind = js::gc::FINALIZE_OBJECT4_BACKGROUND;
    static const js::gc::AllocKind ExtendedFinalizeKind = js::gc::FINALIZE_OBJECT8_BACKGROUND;
#endif

    inline void trace(JSTracer *trc);

    

    inline bool initBoundFunction(JSContext *cx, js::HandleValue thisArg,
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

    
    static bool setTypeForScriptedFunction(JSContext *cx, js::HandleFunction fun,
                                           bool singleton = false);

  private:
    static void staticAsserts() {
        MOZ_STATIC_ASSERT(sizeof(JSFunction) == sizeof(js::shadow::Function),
                          "shadow interface must match actual interface");
    }
    



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
js_CloneFunctionObject(JSContext *cx, js::HandleFunction fun,
                       js::HandleObject parent, js::HandleObject proto,
                       js::gc::AllocKind kind = JSFunction::FinalizeKind);

extern JSFunction *
js_DefineFunction(JSContext *cx, js::HandleObject obj, js::HandleId id, JSNative native,
                  unsigned nargs, unsigned flags, const char *selfHostedName = NULL,
                  js::gc::AllocKind kind = JSFunction::FinalizeKind);

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

namespace js {

JSString *FunctionToString(JSContext *cx, HandleFunction fun, bool bodyOnly, bool lambdaParen);

template<XDRMode mode>
bool
XDRInterpretedFunction(XDRState<mode> *xdr, HandleObject enclosingScope,
                       HandleScript enclosingScript, MutableHandleObject objp);

extern JSObject *
CloneInterpretedFunction(JSContext *cx, HandleObject enclosingScope, HandleFunction fun);






extern void
ReportIncompatibleMethod(JSContext *cx, CallReceiver call, Class *clasp);





extern void
ReportIncompatible(JSContext *cx, CallReceiver call);

} 

extern JSBool
js_fun_apply(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_fun_call(JSContext *cx, unsigned argc, js::Value *vp);

extern JSObject*
js_fun_bind(JSContext *cx, js::HandleObject target, js::HandleValue thisArg,
            js::Value *boundArgs, unsigned argslen);

#endif 
