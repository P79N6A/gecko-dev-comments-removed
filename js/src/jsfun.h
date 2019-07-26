





#ifndef jsfun_h___
#define jsfun_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsobj.h"
#include "jsatom.h"
#include "jsstr.h"

#include "gc/Barrier.h"

ForwardDeclareJS(Atom);

namespace js { class FunctionExtended; }

class JSFunction : public JSObject
{
  public:
    enum Flags {
        INTERPRETED      = 0x0001,  
        NATIVE_CTOR      = 0x0002,  
        EXTENDED         = 0x0004,  
        HEAVYWEIGHT      = 0x0008,  
        IS_FUN_PROTO     = 0x0010,  
        EXPR_CLOSURE     = 0x0020,  
        HAS_GUESSED_ATOM = 0x0040,  

        LAMBDA           = 0x0080,  

        SELF_HOSTED      = 0x0100,  

        SELF_HOSTED_CTOR = 0x0200,  

        HAS_REST         = 0x0400,  
        HAS_DEFAULTS     = 0x0800,  
        INTERPRETED_LAZY = 0x1000,  

        
        NATIVE_FUN = 0,
        INTERPRETED_LAMBDA = INTERPRETED | LAMBDA
    };

    static void staticAsserts() {
        JS_STATIC_ASSERT(INTERPRETED == JS_FUNCTION_INTERPRETED_BIT);
        MOZ_STATIC_ASSERT(sizeof(JSFunction) == sizeof(js::shadow::Function),
                          "shadow interface must match actual interface");
    }

    uint16_t        nargs;        

    uint16_t        flags;        
    union U {
        class Native {
            friend class JSFunction;
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

    
    bool isInterpreted()            const { return flags & (INTERPRETED | INTERPRETED_LAZY); }
    bool isNative()                 const { return !isInterpreted(); }

    
    bool isNativeConstructor()      const { return flags & NATIVE_CTOR; }

    
    bool isHeavyweight()            const { return flags & HEAVYWEIGHT; }
    bool isFunctionPrototype()      const { return flags & IS_FUN_PROTO; }
    bool isInterpretedLazy()        const { return flags & INTERPRETED_LAZY; }
    bool hasScript()                const { return isInterpreted() && u.i.script_; }
    bool isExprClosure()            const { return flags & EXPR_CLOSURE; }
    bool hasGuessedAtom()           const { return flags & HAS_GUESSED_ATOM; }
    bool isLambda()                 const { return flags & LAMBDA; }
    bool isSelfHostedBuiltin()      const { return flags & SELF_HOSTED; }
    bool isSelfHostedConstructor()  const { return flags & SELF_HOSTED_CTOR; }
    bool hasRest()                  const { return flags & HAS_REST; }
    bool hasDefaults()              const { return flags & HAS_DEFAULTS; }

    
    bool isBuiltin() const {
        return isNative() || isSelfHostedBuiltin();
    }
    bool isInterpretedConstructor() const {
        return isInterpreted() && !isFunctionPrototype() &&
               (!isSelfHostedBuiltin() || isSelfHostedConstructor());
    }
    bool isNamedLambda()     const {
        return isLambda() && atom_ && !hasGuessedAtom();
    }

    
    inline bool strict() const;

    
    void setArgCount(uint16_t nargs) {
        JS_ASSERT(this->nargs == 0 || this->nargs == nargs);
        this->nargs = nargs;
    }

    
    void setHasRest() {
        flags |= HAS_REST;
    }

    
    void setHasDefaults() {
        flags |= HAS_DEFAULTS;
    }

    void setIsSelfHostedBuiltin() {
        JS_ASSERT(!isSelfHostedBuiltin());
        flags |= SELF_HOSTED;
    }

    void setIsSelfHostedConstructor() {
        JS_ASSERT(!isSelfHostedConstructor());
        flags |= SELF_HOSTED_CTOR;
    }

    void setIsFunctionPrototype() {
        JS_ASSERT(!isFunctionPrototype());
        flags |= IS_FUN_PROTO;
    }

    void setIsHeavyweight() {
        flags |= HEAVYWEIGHT;
    }

    
    void setIsExprClosure() {
        flags |= EXPR_CLOSURE;
    }

    void markNotLazy() {
        JS_ASSERT(isInterpretedLazy());
        JS_ASSERT(hasScript());
        flags |= INTERPRETED;
        flags &= ~INTERPRETED_LAZY;
    }

    JSAtom *atom() const { return hasGuessedAtom() ? NULL : atom_.get(); }
    inline void initAtom(JSAtom *atom);
    JSAtom *displayAtom() const { return atom_; }

    inline void setGuessedAtom(js::UnrootedAtom atom);

    
    enum { MAX_ARGS_AND_VARS = 2 * ((1U << 16) - 1) };

    



    inline JSObject *environment() const;
    inline void setEnvironment(JSObject *obj);
    inline void initEnvironment(JSObject *obj);

    static inline size_t offsetOfEnvironment() { return offsetof(JSFunction, u.i.env_); }
    static inline size_t offsetOfAtom() { return offsetof(JSFunction, atom_); }

    bool initializeLazyScript(JSContext *cx);

    js::UnrootedScript getOrCreateScript(JSContext *cx) {
        JS_ASSERT(isInterpreted());
        if (isInterpretedLazy()) {
            js::RootedFunction self(cx, this);
            js::MaybeCheckStackRoots(cx);
            if (!self->initializeLazyScript(cx))
                return js::UnrootedScript(NULL);
            return self->u.i.script_;
        }
        JS_ASSERT(hasScript());
        return u.i.script_;
    }

    static bool maybeGetOrCreateScript(JSContext *cx, js::HandleFunction fun,
                                       js::MutableHandle<JSScript*> script)
    {
        if (fun->isNative()) {
            script.set(NULL);
            return true;
        }
        script.set(fun->getOrCreateScript(cx));
        return fun->hasScript();
    }

    js::UnrootedScript nonLazyScript() const {
        JS_ASSERT(hasScript());
        return JS::HandleScript::fromMarkedLocation(&u.i.script_);
    }

    js::UnrootedScript maybeNonLazyScript() const {
        return isInterpreted() ? nonLazyScript() : js::UnrootedScript(NULL);
    }

    js::HeapPtrScript &mutableScript() {
        JS_ASSERT(isInterpreted());
        return *(js::HeapPtrScript *)&u.i.script_;
    }

    inline void setScript(JSScript *script_);
    inline void initScript(JSScript *script_);

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
        JS_ASSERT(!!(flags & EXTENDED) == (getAllocKind() == ExtendedFinalizeKind));
        return !!(flags & EXTENDED);
    }

  public:
    
    inline void initializeExtended();
    inline void setExtendedSlot(size_t which, const js::Value &val);
    inline const js::Value &getExtendedSlot(size_t which) const;

    
    static bool setTypeForScriptedFunction(JSContext *cx, js::HandleFunction fun,
                                           bool singleton = false);

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
fun_toStringHelper(JSContext *cx, js::HandleObject obj, unsigned indent);

inline JSFunction::Flags
JSAPIToJSFunctionFlags(unsigned flags)
{
    return (flags & JSFUN_CONSTRUCTOR)
           ? JSFunction::NATIVE_CTOR
           : JSFunction::NATIVE_FUN;
}

namespace js {

extern JSFunction *
NewFunction(JSContext *cx, HandleObject funobj, JSNative native, unsigned nargs,
            JSFunction::Flags flags, HandleObject parent, HandleAtom atom,
            gc::AllocKind allocKind = JSFunction::FinalizeKind,
            NewObjectKind newKind = GenericObject);

extern JSFunction *
DefineFunction(JSContext *cx, HandleObject obj, HandleId id, JSNative native,
               unsigned nargs, unsigned flags,
               gc::AllocKind allocKind = JSFunction::FinalizeKind,
               NewObjectKind newKind = GenericObject);






class FunctionExtended : public JSFunction
{
    friend class JSFunction;

    
    HeapValue extendedSlots[2];
};

extern JSFunction *
CloneFunctionObject(JSContext *cx, HandleFunction fun, HandleObject parent,
                    gc::AllocKind kind = JSFunction::FinalizeKind);

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

JSBool
CallOrConstructBoundFunction(JSContext *, unsigned, js::Value *);

extern JSFunctionSpec function_methods[];

} 

extern JSBool
js_fun_apply(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_fun_call(JSContext *cx, unsigned argc, js::Value *vp);

extern JSObject*
js_fun_bind(JSContext *cx, js::HandleObject target, js::HandleValue thisArg,
            js::Value *boundArgs, unsigned argslen);

#endif 
