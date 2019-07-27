





#ifndef jsfun_h
#define jsfun_h





#include "jsobj.h"
#include "jsscript.h"
#include "jstypes.h"

namespace js {
class FunctionExtended;

typedef JSNative           Native;
typedef JSParallelNative   ParallelNative;
typedef JSThreadSafeNative ThreadSafeNative;
}

struct JSAtomState;

class JSFunction : public JSObject
{
  public:
    static const js::Class class_;

    enum Flags {
        INTERPRETED      = 0x0001,  
        NATIVE_CTOR      = 0x0002,  
        EXTENDED         = 0x0004,  
        IS_FUN_PROTO     = 0x0010,  
        EXPR_CLOSURE     = 0x0020,  
        HAS_GUESSED_ATOM = 0x0040,  

        LAMBDA           = 0x0080,  


        SELF_HOSTED      = 0x0100,  

        SELF_HOSTED_CTOR = 0x0200,  

        HAS_REST         = 0x0400,  
        ASMJS            = 0x0800,  
        INTERPRETED_LAZY = 0x1000,  
        ARROW            = 0x2000,  

        
        NATIVE_FUN = 0,
        ASMJS_CTOR = ASMJS | NATIVE_CTOR,
        ASMJS_LAMBDA_CTOR = ASMJS | NATIVE_CTOR | LAMBDA,
        INTERPRETED_LAMBDA = INTERPRETED | LAMBDA,
        INTERPRETED_LAMBDA_ARROW = INTERPRETED | LAMBDA | ARROW
    };

    static void staticAsserts() {
        JS_STATIC_ASSERT(INTERPRETED == JS_FUNCTION_INTERPRETED_BIT);
        static_assert(sizeof(JSFunction) == sizeof(js::shadow::Function),
                      "shadow interface must match actual interface");
    }

  private:
    uint16_t        nargs_;       

    uint16_t        flags_;       
    union U {
        class Native {
            friend class JSFunction;
            js::Native          native;       
            const JSJitInfo     *jitinfo;     


        } n;
        struct Scripted {
            union {
                JSScript *script_; 

                js::LazyScript *lazy_; 
            } s;
            JSObject    *env_;    

        } i;
        void            *nativeOrScript;
    } u;
    js::HeapPtrAtom  atom_;       

  public:

    
    bool isHeavyweight() const {
        JS_ASSERT(!isInterpretedLazy());

        if (isNative())
            return false;

        
        return nonLazyScript()->hasAnyAliasedBindings() ||
               nonLazyScript()->funHasExtensibleScope() ||
               nonLazyScript()->funNeedsDeclEnvObject() ||
               isGenerator();
    }

    size_t nargs() const {
        return nargs_;
    }

    uint16_t flags() const {
        return flags_;
    }

    
    bool isInterpreted()            const { return flags() & (INTERPRETED | INTERPRETED_LAZY); }
    bool isNative()                 const { return !isInterpreted(); }

    
    bool isNativeConstructor()      const { return flags() & NATIVE_CTOR; }
    bool isAsmJSNative()            const { return flags() & ASMJS; }

    
    bool isFunctionPrototype()      const { return flags() & IS_FUN_PROTO; }
    bool isExprClosure()            const { return flags() & EXPR_CLOSURE; }
    bool hasGuessedAtom()           const { return flags() & HAS_GUESSED_ATOM; }
    bool isLambda()                 const { return flags() & LAMBDA; }
    bool isSelfHostedBuiltin()      const { return flags() & SELF_HOSTED; }
    bool isSelfHostedConstructor()  const { return flags() & SELF_HOSTED_CTOR; }
    bool hasRest()                  const { return flags() & HAS_REST; }

    bool isInterpretedLazy()        const {
        return flags() & INTERPRETED_LAZY;
    }
    bool hasScript()                const {
        return flags() & INTERPRETED;
    }

    bool hasJITCode() const {
        if (!hasScript())
            return false;

        return nonLazyScript()->hasBaselineScript() || nonLazyScript()->hasIonScript();
    }

    
    bool isArrow()                  const { return flags() & ARROW; }

    
    bool isBuiltin() const {
        return (isNative() && !isAsmJSNative()) || isSelfHostedBuiltin();
    }
    bool isInterpretedConstructor() const {
        
        
        return isInterpreted() && !isFunctionPrototype() && !isArrow() &&
               (!isSelfHostedBuiltin() || isSelfHostedConstructor());
    }
    bool isNamedLambda() const {
        return isLambda() && displayAtom() && !hasGuessedAtom();
    }
    bool hasParallelNative() const {
        return isNative() && jitInfo() && jitInfo()->hasParallelNative();
    }

    bool isBuiltinFunctionConstructor();

    
    bool strict() const {
        MOZ_ASSERT(isInterpreted());
        return isInterpretedLazy() ? lazyScript()->strict() : nonLazyScript()->strict();
    }

    void setFlags(uint16_t flags) {
        this->flags_ = flags;
    }

    
    void setArgCount(uint16_t nargs) {
        this->nargs_ = nargs;
    }

    
    void setHasRest() {
        flags_ |= HAS_REST;
    }

    void setIsSelfHostedBuiltin() {
        JS_ASSERT(!isSelfHostedBuiltin());
        flags_ |= SELF_HOSTED;
    }

    void setIsSelfHostedConstructor() {
        JS_ASSERT(!isSelfHostedConstructor());
        flags_ |= SELF_HOSTED_CTOR;
    }

    void setIsFunctionPrototype() {
        JS_ASSERT(!isFunctionPrototype());
        flags_ |= IS_FUN_PROTO;
    }

    
    void setIsExprClosure() {
        flags_ |= EXPR_CLOSURE;
    }

    void setArrow() {
        flags_ |= ARROW;
    }

    JSAtom *atom() const { return hasGuessedAtom() ? nullptr : atom_.get(); }
    js::PropertyName *name() const { return hasGuessedAtom() || !atom_ ? nullptr : atom_->asPropertyName(); }
    void initAtom(JSAtom *atom) { atom_.init(atom); }

    JSAtom *displayAtom() const {
        return atom_;
    }

    void setGuessedAtom(JSAtom *atom) {
        JS_ASSERT(!atom_);
        JS_ASSERT(atom);
        JS_ASSERT(!hasGuessedAtom());
        atom_ = atom;
        flags_ |= HAS_GUESSED_ATOM;
    }

    
    enum { MAX_ARGS_AND_VARS = 2 * ((1U << 16) - 1) };

    



    JSObject *environment() const {
        JS_ASSERT(isInterpreted());
        return u.i.env_;
    }

    void setEnvironment(JSObject *obj) {
        JS_ASSERT(isInterpreted());
        *(js::HeapPtrObject *)&u.i.env_ = obj;
    }

    void initEnvironment(JSObject *obj) {
        JS_ASSERT(isInterpreted());
        ((js::HeapPtrObject *)&u.i.env_)->init(obj);
    }

    static inline size_t offsetOfNargs() { return offsetof(JSFunction, nargs_); }
    static inline size_t offsetOfFlags() { return offsetof(JSFunction, flags_); }
    static inline size_t offsetOfEnvironment() { return offsetof(JSFunction, u.i.env_); }
    static inline size_t offsetOfAtom() { return offsetof(JSFunction, atom_); }

    static bool createScriptForLazilyInterpretedFunction(JSContext *cx, js::HandleFunction fun);
    void relazify(JSTracer *trc);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JSScript *getOrCreateScript(JSContext *cx) {
        JS_ASSERT(isInterpreted());
        JS_ASSERT(cx);
        if (isInterpretedLazy()) {
            JS::RootedFunction self(cx, this);
            if (!createScriptForLazilyInterpretedFunction(cx, self))
                return nullptr;
            return self->nonLazyScript();
        }
        return nonLazyScript();
    }

    JSScript *existingScriptForInlinedFunction() {
        MOZ_ASSERT(isInterpreted());
        if (isInterpretedLazy()) {
            
            
            
            
            
            js::LazyScript *lazy = lazyScript();
            JSFunction *fun = lazy->functionNonDelazifying();
            MOZ_ASSERT(fun);
            JSScript *script = fun->nonLazyScript();

            if (shadowZone()->needsIncrementalBarrier())
                js::LazyScript::writeBarrierPre(lazy);

            flags_ &= ~INTERPRETED_LAZY;
            flags_ |= INTERPRETED;
            initScript(script);
        }
        return nonLazyScript();
    }

    JSScript *nonLazyScript() const {
        JS_ASSERT(hasScript());
        JS_ASSERT(u.i.s.script_);
        return u.i.s.script_;
    }

    
    
    JSFunction* originalFunction() {
        if (this->hasScript() && this->nonLazyScript()->isCallsiteClone()) {
            return this->nonLazyScript()->donorFunction();
        } else {
            return this;
        }
    }

    js::HeapPtrScript &mutableScript() {
        JS_ASSERT(isInterpreted());
        return *(js::HeapPtrScript *)&u.i.s.script_;
    }

    js::LazyScript *lazyScript() const {
        JS_ASSERT(isInterpretedLazy() && u.i.s.lazy_);
        return u.i.s.lazy_;
    }

    js::LazyScript *lazyScriptOrNull() const {
        JS_ASSERT(isInterpretedLazy());
        return u.i.s.lazy_;
    }

    js::GeneratorKind generatorKind() const {
        if (!isInterpreted())
            return js::NotGenerator;
        if (hasScript())
            return nonLazyScript()->generatorKind();
        if (js::LazyScript *lazy = lazyScriptOrNull())
            return lazy->generatorKind();
        JS_ASSERT(isSelfHostedBuiltin());
        return js::NotGenerator;
    }

    bool isGenerator() const { return generatorKind() != js::NotGenerator; }

    bool isLegacyGenerator() const { return generatorKind() == js::LegacyGenerator; }

    bool isStarGenerator() const { return generatorKind() == js::StarGenerator; }

    void setScript(JSScript *script_) {
        JS_ASSERT(hasScript());
        mutableScript() = script_;
    }

    void initScript(JSScript *script_) {
        JS_ASSERT(hasScript());
        mutableScript().init(script_);
    }

    void setUnlazifiedScript(JSScript *script) {
        
        
        JS_ASSERT(isInterpretedLazy());
        if (!lazyScript()->maybeScript())
            lazyScript()->initScript(script);
        flags_ &= ~INTERPRETED_LAZY;
        flags_ |= INTERPRETED;
        initScript(script);
    }

    void initLazyScript(js::LazyScript *lazy) {
        JS_ASSERT(isInterpreted());
        flags_ &= ~INTERPRETED;
        flags_ |= INTERPRETED_LAZY;
        u.i.s.lazy_ = lazy;
    }

    JSNative native() const {
        JS_ASSERT(isNative());
        return u.n.native;
    }

    JSNative maybeNative() const {
        return isInterpreted() ? nullptr : native();
    }

    JSParallelNative parallelNative() const {
        JS_ASSERT(hasParallelNative());
        return jitInfo()->parallelNative;
    }

    JSParallelNative maybeParallelNative() const {
        return hasParallelNative() ? parallelNative() : nullptr;
    }

    void initNative(js::Native native, const JSJitInfo *jitinfo) {
        JS_ASSERT(native);
        u.n.native = native;
        u.n.jitinfo = jitinfo;
    }

    const JSJitInfo *jitInfo() const {
        JS_ASSERT(isNative());
        return u.n.jitinfo;
    }

    void setJitInfo(const JSJitInfo *data) {
        JS_ASSERT(isNative());
        u.n.jitinfo = data;
    }

    static unsigned offsetOfNativeOrScript() {
        JS_STATIC_ASSERT(offsetof(U, n.native) == offsetof(U, i.s.script_));
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

    JSObject *getBoundFunctionTarget() const {
        JS_ASSERT(isBoundFunction());

        
        return getParent();
    }

    const js::Value &getBoundFunctionThis() const;
    const js::Value &getBoundFunctionArgument(unsigned which) const;
    size_t getBoundFunctionArgumentCount() const;

  private:
    inline js::FunctionExtended *toExtended();
    inline const js::FunctionExtended *toExtended() const;

  public:
    inline bool isExtended() const {
        JS_STATIC_ASSERT(FinalizeKind != ExtendedFinalizeKind);
        JS_ASSERT_IF(isTenured(), !!(flags() & EXTENDED) == (tenuredGetAllocKind() == ExtendedFinalizeKind));
        return !!(flags() & EXTENDED);
    }

    




    inline void initializeExtended();
    inline void initExtendedSlot(size_t which, const js::Value &val);
    inline void setExtendedSlot(size_t which, const js::Value &val);
    inline const js::Value &getExtendedSlot(size_t which) const;

    
    static bool setTypeForScriptedFunction(js::ExclusiveContext *cx, js::HandleFunction fun,
                                           bool singleton = false);

    
    js::gc::AllocKind getAllocKind() const {
        js::gc::AllocKind kind = FinalizeKind;
        if (isExtended())
            kind = ExtendedFinalizeKind;
        JS_ASSERT_IF(isTenured(), kind == tenuredGetAllocKind());
        return kind;
    }
};

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

extern bool
Function(JSContext *cx, unsigned argc, Value *vp);

extern bool
Generator(JSContext *cx, unsigned argc, Value *vp);

extern JSFunction *
NewFunction(ExclusiveContext *cx, HandleObject funobj, JSNative native, unsigned nargs,
            JSFunction::Flags flags, HandleObject parent, HandleAtom atom,
            gc::AllocKind allocKind = JSFunction::FinalizeKind,
            NewObjectKind newKind = GenericObject);


extern JSFunction *
NewFunctionWithProto(ExclusiveContext *cx, HandleObject funobj, JSNative native, unsigned nargs,
                     JSFunction::Flags flags, HandleObject parent, HandleAtom atom,
                     JSObject *proto, gc::AllocKind allocKind = JSFunction::FinalizeKind,
                     NewObjectKind newKind = GenericObject);

extern JSFunction *
DefineFunction(JSContext *cx, HandleObject obj, HandleId id, JSNative native,
               unsigned nargs, unsigned flags,
               gc::AllocKind allocKind = JSFunction::FinalizeKind,
               NewObjectKind newKind = GenericObject);

bool
FunctionHasResolveHook(const JSAtomState &atomState, PropertyName *name);

extern bool
fun_resolve(JSContext *cx, HandleObject obj, HandleId id, MutableHandleObject objp);

extern bool
fun_toString(JSContext *cx, unsigned argc, Value *vp);






class FunctionExtended : public JSFunction
{
  public:
    static const unsigned NUM_EXTENDED_SLOTS = 2;

    
    static const unsigned ARROW_THIS_SLOT = 0;

    static inline size_t offsetOfExtendedSlot(unsigned which) {
        MOZ_ASSERT(which < NUM_EXTENDED_SLOTS);
        return offsetof(FunctionExtended, extendedSlots) + which * sizeof(HeapValue);
    }
    static inline size_t offsetOfArrowThisSlot() {
        return offsetOfExtendedSlot(ARROW_THIS_SLOT);
    }

  private:
    friend class JSFunction;

    
    HeapValue extendedSlots[NUM_EXTENDED_SLOTS];
};

extern JSFunction *
CloneFunctionObject(JSContext *cx, HandleFunction fun, HandleObject parent,
                    gc::AllocKind kind = JSFunction::FinalizeKind,
                    NewObjectKind newKindArg = GenericObject);


extern bool
FindBody(JSContext *cx, HandleFunction fun, HandleLinearString src, size_t *bodyStart,
         size_t *bodyEnd);

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

    JS_ASSERT(mozilla::ArrayLength(toExtended()->extendedSlots) == 2);
    toExtended()->extendedSlots[0].init(js::UndefinedValue());
    toExtended()->extendedSlots[1].init(js::UndefinedValue());
}

inline void
JSFunction::initExtendedSlot(size_t which, const js::Value &val)
{
    JS_ASSERT(which < mozilla::ArrayLength(toExtended()->extendedSlots));
    toExtended()->extendedSlots[which].init(val);
}

inline void
JSFunction::setExtendedSlot(size_t which, const js::Value &val)
{
    JS_ASSERT(which < mozilla::ArrayLength(toExtended()->extendedSlots));
    toExtended()->extendedSlots[which] = val;
}

inline const js::Value &
JSFunction::getExtendedSlot(size_t which) const
{
    JS_ASSERT(which < mozilla::ArrayLength(toExtended()->extendedSlots));
    return toExtended()->extendedSlots[which];
}

namespace js {

JSString *FunctionToString(JSContext *cx, HandleFunction fun, bool bodyOnly, bool lambdaParen);

template<XDRMode mode>
bool
XDRInterpretedFunction(XDRState<mode> *xdr, HandleObject enclosingScope,
                       HandleScript enclosingScript, MutableHandleObject objp);

extern JSObject *
CloneFunctionAndScript(JSContext *cx, HandleObject enclosingScope, HandleFunction fun);






extern void
ReportIncompatibleMethod(JSContext *cx, CallReceiver call, const Class *clasp);





extern void
ReportIncompatible(JSContext *cx, CallReceiver call);

bool
CallOrConstructBoundFunction(JSContext *, unsigned, js::Value *);

extern const JSFunctionSpec function_methods[];

} 

extern bool
js_fun_apply(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
js_fun_call(JSContext *cx, unsigned argc, js::Value *vp);

extern JSObject*
js_fun_bind(JSContext *cx, js::HandleObject target, js::HandleValue thisArg,
            js::Value *boundArgs, unsigned argslen);

#ifdef DEBUG
namespace JS {
namespace detail {

JS_PUBLIC_API(void)
CheckIsValidConstructible(Value calleev);

} 
} 
#endif

#endif 
