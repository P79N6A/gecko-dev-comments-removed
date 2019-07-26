





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
        
        INTERPRETED_LAZY = 0x1000,  
        ARROW            = 0x2000,  
        SH_WRAPPABLE     = 0x4000,  

        
        NATIVE_FUN = 0,
        INTERPRETED_LAMBDA = INTERPRETED | LAMBDA,
        INTERPRETED_LAMBDA_ARROW = INTERPRETED | LAMBDA | ARROW
    };

    static void staticAsserts() {
        JS_STATIC_ASSERT(INTERPRETED == JS_FUNCTION_INTERPRETED_BIT);
        static_assert(sizeof(JSFunction) == sizeof(js::shadow::Function),
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
            union {
                JSScript *script_; 

                js::LazyScript *lazy_; 
            } s;
            JSObject    *env_;    

        } i;
        void            *nativeOrScript;
    } u;
  private:
    js::HeapPtrAtom  atom_;       

  public:

    
    bool isHeavyweight() const {
        JS_ASSERT(!isInterpretedLazy());

        if (isNative())
            return false;

        
        return nonLazyScript()->bindings.hasAnyAliasedBindings() ||
               nonLazyScript()->funHasExtensibleScope ||
               nonLazyScript()->funNeedsDeclEnvObject;
    }

    
    bool isInterpreted()            const { return flags & (INTERPRETED | INTERPRETED_LAZY); }
    bool isNative()                 const { return !isInterpreted(); }

    
    bool isNativeConstructor()      const { return flags & NATIVE_CTOR; }

    
    bool isFunctionPrototype()      const { return flags & IS_FUN_PROTO; }
    bool isInterpretedLazy()        const { return flags & INTERPRETED_LAZY; }
    bool hasScript()                const { return flags & INTERPRETED; }
    bool isExprClosure()            const { return flags & EXPR_CLOSURE; }
    bool hasGuessedAtom()           const { return flags & HAS_GUESSED_ATOM; }
    bool isLambda()                 const { return flags & LAMBDA; }
    bool isSelfHostedBuiltin()      const { return flags & SELF_HOSTED; }
    bool isSelfHostedConstructor()  const { return flags & SELF_HOSTED_CTOR; }
    bool hasRest()                  const { return flags & HAS_REST; }
    bool isWrappable()              const {
        JS_ASSERT_IF(flags & SH_WRAPPABLE, isSelfHostedBuiltin());
        return flags & SH_WRAPPABLE;
    }

    bool hasJITCode() const {
        if (!hasScript())
            return false;

        return nonLazyScript()->hasBaselineScript() || nonLazyScript()->hasIonScript();
    }

    
    
    
    
    
    
    
    
    
    
    
    bool isArrow()                  const { return flags & ARROW; }

    
    bool isBuiltin() const {
        return isNative() || isSelfHostedBuiltin();
    }
    bool isInterpretedConstructor() const {
        
        
        return isInterpreted() && !isFunctionPrototype() &&
               (!isSelfHostedBuiltin() || isSelfHostedConstructor());
    }
    bool isNamedLambda() const {
        return isLambda() && atom_ && !hasGuessedAtom();
    }
    bool hasParallelNative() const {
        return isNative() && jitInfo() && !!jitInfo()->parallelNative;
    }

    bool isBuiltinFunctionConstructor();

    
    bool strict() const {
        return nonLazyScript()->strict;
    }

    
    void setArgCount(uint16_t nargs) {
        this->nargs = nargs;
    }

    
    void setHasRest() {
        flags |= HAS_REST;
    }

    void setIsSelfHostedBuiltin() {
        JS_ASSERT(!isSelfHostedBuiltin());
        flags |= SELF_HOSTED;
    }

    void setIsSelfHostedConstructor() {
        JS_ASSERT(!isSelfHostedConstructor());
        flags |= SELF_HOSTED_CTOR;
    }

    void makeWrappable() {
        JS_ASSERT(isSelfHostedBuiltin());
        JS_ASSERT(!isWrappable());
        flags |= SH_WRAPPABLE;
    }

    void setIsFunctionPrototype() {
        JS_ASSERT(!isFunctionPrototype());
        flags |= IS_FUN_PROTO;
    }

    
    void setIsExprClosure() {
        flags |= EXPR_CLOSURE;
    }

    JSAtom *atom() const { return hasGuessedAtom() ? nullptr : atom_.get(); }
    js::PropertyName *name() const { return hasGuessedAtom() || !atom_ ? nullptr : atom_->asPropertyName(); }
    void initAtom(JSAtom *atom) { atom_.init(atom); }
    JSAtom *displayAtom() const { return atom_; }

    void setGuessedAtom(JSAtom *atom) {
        JS_ASSERT(atom_ == nullptr);
        JS_ASSERT(atom != nullptr);
        JS_ASSERT(!hasGuessedAtom());
        atom_ = atom;
        flags |= HAS_GUESSED_ATOM;
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

    static inline size_t offsetOfEnvironment() { return offsetof(JSFunction, u.i.env_); }
    static inline size_t offsetOfAtom() { return offsetof(JSFunction, atom_); }

    static bool createScriptForLazilyInterpretedFunction(JSContext *cx, js::HandleFunction fun);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JSScript *getOrCreateScript(JSContext *cx) {
        JS_ASSERT(isInterpreted());
        JS_ASSERT(cx);
        if (isInterpretedLazy()) {
            JS::RootedFunction self(cx, this);
            if (!createScriptForLazilyInterpretedFunction(cx, self))
                return nullptr;
            JS_ASSERT(self->hasScript());
            return self->u.i.s.script_;
        }
        JS_ASSERT(hasScript());
        return u.i.s.script_;
    }

    JSScript *existingScript() {
        JS_ASSERT(isInterpreted());
        if (isInterpretedLazy()) {
            js::LazyScript *lazy = lazyScript();
            JSScript *script = lazy->maybeScript();
            JS_ASSERT(script);

            if (shadowZone()->needsBarrier())
                js::LazyScript::writeBarrierPre(lazy);

            flags &= ~INTERPRETED_LAZY;
            flags |= INTERPRETED;
            initScript(script);
        }
        JS_ASSERT(hasScript());
        return u.i.s.script_;
    }

    JSScript *nonLazyScript() const {
        JS_ASSERT(hasScript());
        return u.i.s.script_;
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
        JS_ASSERT(isInterpreted());
        mutableScript() = script_;
    }

    void initScript(JSScript *script_) {
        JS_ASSERT(isInterpreted());
        mutableScript().init(script_);
    }

    void initLazyScript(js::LazyScript *lazy) {
        JS_ASSERT(isInterpreted());
        flags &= ~INTERPRETED;
        flags |= INTERPRETED_LAZY;
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

    inline const js::Value &getBoundFunctionThis() const;
    inline const js::Value &getBoundFunctionArgument(unsigned which) const;
    inline size_t getBoundFunctionArgumentCount() const;

  private:
    inline js::FunctionExtended *toExtended();
    inline const js::FunctionExtended *toExtended() const;

  public:
    inline bool isExtended() const {
        JS_STATIC_ASSERT(FinalizeKind != ExtendedFinalizeKind);
        JS_ASSERT_IF(isTenured(), !!(flags & EXTENDED) == (tenuredGetAllocKind() == ExtendedFinalizeKind));
        return !!(flags & EXTENDED);
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

extern bool
fun_resolve(JSContext *cx, js::HandleObject obj, js::HandleId id,
            unsigned flags, js::MutableHandleObject objp);


bool IsConstructor(const Value &v);






class FunctionExtended : public JSFunction
{
  public:
    static const unsigned NUM_EXTENDED_SLOTS = 2;

  private:
    friend class JSFunction;

    
    HeapValue extendedSlots[NUM_EXTENDED_SLOTS];
};

extern JSFunction *
CloneFunctionObject(JSContext *cx, HandleFunction fun, HandleObject parent,
                    gc::AllocKind kind = JSFunction::FinalizeKind,
                    NewObjectKind newKindArg = GenericObject);

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
