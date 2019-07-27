





#ifndef jsfun_h
#define jsfun_h





#include "jsobj.h"
#include "jsscript.h"
#include "jstypes.h"

namespace js {
class FunctionExtended;

typedef JSNative           Native;
}

struct JSAtomState;

class JSFunction : public js::NativeObject
{
  public:
    static const js::Class class_;

    enum FunctionKind {
        NormalFunction = 0,
        Arrow,                      
        Method,                     
        AsmJS                       
    };

    enum Flags {
        INTERPRETED      = 0x0001,  
        NATIVE_CTOR      = 0x0002,  
        EXTENDED         = 0x0004,  
        IS_FUN_PROTO     = 0x0008,  
        EXPR_BODY        = 0x0010,  

        HAS_GUESSED_ATOM = 0x0020,  

        LAMBDA           = 0x0040,  


        SELF_HOSTED      = 0x0080,  

        SELF_HOSTED_CTOR = 0x0100,  

        HAS_REST         = 0x0200,  
        INTERPRETED_LAZY = 0x0400,  
        RESOLVED_LENGTH  = 0x0800,  
        RESOLVED_NAME    = 0x1000,  

        FUNCTION_KIND_SHIFT = 13,
        FUNCTION_KIND_MASK  = 0x3 << FUNCTION_KIND_SHIFT,

        ASMJS_KIND = AsmJS << FUNCTION_KIND_SHIFT,
        ARROW_KIND = Arrow << FUNCTION_KIND_SHIFT,

        
        NATIVE_FUN = 0,
        ASMJS_CTOR = ASMJS_KIND | NATIVE_CTOR,
        ASMJS_LAMBDA_CTOR = ASMJS_KIND | NATIVE_CTOR | LAMBDA,
        INTERPRETED_METHOD = INTERPRETED | (Method << FUNCTION_KIND_SHIFT),
        INTERPRETED_LAMBDA = INTERPRETED | LAMBDA,
        INTERPRETED_LAMBDA_ARROW = INTERPRETED | LAMBDA | ARROW_KIND,
        STABLE_ACROSS_CLONES = NATIVE_CTOR | IS_FUN_PROTO | EXPR_BODY | HAS_GUESSED_ATOM |
                               LAMBDA | SELF_HOSTED | SELF_HOSTED_CTOR | HAS_REST |
                               FUNCTION_KIND_MASK
    };

    static_assert((INTERPRETED | INTERPRETED_LAZY) == js::JS_FUNCTION_INTERPRETED_BITS,
                  "jsfriendapi.h's JSFunction::INTERPRETED-alike is wrong");

  private:
    uint16_t        nargs_;       

    uint16_t        flags_;       
    union U {
        class Native {
            friend class JSFunction;
            js::Native          native;       
            const JSJitInfo*    jitinfo;     


        } n;
        struct Scripted {
            union {
                JSScript* script_; 

                js::LazyScript* lazy_; 
            } s;
            JSObject*   env_;    

        } i;
        void*           nativeOrScript;
    } u;
    js::HeapPtrAtom  atom_;       

  public:

    
    bool isHeavyweight() const {
        MOZ_ASSERT(!isInterpretedLazy());

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

    FunctionKind kind() const {
        return static_cast<FunctionKind>((flags_ & FUNCTION_KIND_MASK) >> FUNCTION_KIND_SHIFT);
    }

    
    bool isInterpreted()            const { return flags() & (INTERPRETED | INTERPRETED_LAZY); }
    bool isNative()                 const { return !isInterpreted(); }

    
    bool isNativeConstructor()      const { return flags() & NATIVE_CTOR; }
    bool isAsmJSNative()            const { return kind() == AsmJS; }

    
    bool isFunctionPrototype()      const { return flags() & IS_FUN_PROTO; }
    bool isExprBody()               const { return flags() & EXPR_BODY; }
    bool hasGuessedAtom()           const { return flags() & HAS_GUESSED_ATOM; }
    bool isLambda()                 const { return flags() & LAMBDA; }
    bool isSelfHostedBuiltin()      const { return flags() & SELF_HOSTED; }
    bool isSelfHostedConstructor()  const { return flags() & SELF_HOSTED_CTOR; }
    bool hasRest()                  const { return flags() & HAS_REST; }
    bool isInterpretedLazy()        const { return flags() & INTERPRETED_LAZY; }
    bool hasScript()                const { return flags() & INTERPRETED; }

    
    bool isArrow()                  const { return kind() == Arrow; }
    bool isMethod()                 const { return kind() == Method; }

    bool hasResolvedLength()        const { return flags() & RESOLVED_LENGTH; }
    bool hasResolvedName()          const { return flags() & RESOLVED_NAME; }

    bool hasJITCode() const {
        if (!hasScript())
            return false;

        return nonLazyScript()->hasBaselineScript() || nonLazyScript()->hasIonScript();
    }

    
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

    bool isBuiltinFunctionConstructor();

    
    bool strict() const {
        MOZ_ASSERT(isInterpreted());
        return isInterpretedLazy() ? lazyScript()->strict() : nonLazyScript()->strict();
    }

    void setFlags(uint16_t flags) {
        this->flags_ = flags;
    }
    void setKind(FunctionKind kind) {
        this->flags_ &= ~FUNCTION_KIND_MASK;
        this->flags_ |= static_cast<uint16_t>(kind) << FUNCTION_KIND_SHIFT;
    }

    
    void setArgCount(uint16_t nargs) {
        this->nargs_ = nargs;
    }

    
    void setHasRest() {
        flags_ |= HAS_REST;
    }

    void setIsSelfHostedBuiltin() {
        MOZ_ASSERT(!isSelfHostedBuiltin());
        flags_ |= SELF_HOSTED;
    }

    void setIsSelfHostedConstructor() {
        MOZ_ASSERT(!isSelfHostedConstructor());
        flags_ |= SELF_HOSTED_CTOR;
    }

    void setIsFunctionPrototype() {
        MOZ_ASSERT(!isFunctionPrototype());
        flags_ |= IS_FUN_PROTO;
    }

    
    void setIsExprBody() {
        flags_ |= EXPR_BODY;
    }

    void setArrow() {
        setKind(Arrow);
    }

    void setResolvedLength() {
        flags_ |= RESOLVED_LENGTH;
    }

    void setResolvedName() {
        flags_ |= RESOLVED_NAME;
    }

    JSAtom* atom() const { return hasGuessedAtom() ? nullptr : atom_.get(); }

    js::PropertyName* name() const {
        return hasGuessedAtom() || !atom_ ? nullptr : atom_->asPropertyName();
    }

    void initAtom(JSAtom* atom) { atom_.init(atom); }

    JSAtom* displayAtom() const {
        return atom_;
    }

    void setGuessedAtom(JSAtom* atom) {
        MOZ_ASSERT(!atom_);
        MOZ_ASSERT(atom);
        MOZ_ASSERT(!hasGuessedAtom());
        atom_ = atom;
        flags_ |= HAS_GUESSED_ATOM;
    }

    
    enum { MAX_ARGS_AND_VARS = 2 * ((1U << 16) - 1) };

    



    JSObject* environment() const {
        MOZ_ASSERT(isInterpreted());
        return u.i.env_;
    }

    void setEnvironment(JSObject* obj) {
        MOZ_ASSERT(isInterpreted());
        *(js::HeapPtrObject*)&u.i.env_ = obj;
    }

    void initEnvironment(JSObject* obj) {
        MOZ_ASSERT(isInterpreted());
        ((js::HeapPtrObject*)&u.i.env_)->init(obj);
    }

    static inline size_t offsetOfNargs() { return offsetof(JSFunction, nargs_); }
    static inline size_t offsetOfFlags() { return offsetof(JSFunction, flags_); }
    static inline size_t offsetOfEnvironment() { return offsetof(JSFunction, u.i.env_); }
    static inline size_t offsetOfAtom() { return offsetof(JSFunction, atom_); }

    static bool createScriptForLazilyInterpretedFunction(JSContext* cx, js::HandleFunction fun);
    void relazify(JSTracer* trc);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JSScript* getOrCreateScript(JSContext* cx) {
        MOZ_ASSERT(isInterpreted());
        MOZ_ASSERT(cx);
        if (isInterpretedLazy()) {
            JS::RootedFunction self(cx, this);
            if (!createScriptForLazilyInterpretedFunction(cx, self))
                return nullptr;
            return self->nonLazyScript();
        }
        return nonLazyScript();
    }

    JSScript* existingScriptForInlinedFunction() {
        MOZ_ASSERT(isInterpreted());
        if (isInterpretedLazy()) {
            
            
            
            
            
            js::LazyScript* lazy = lazyScript();
            JSFunction* fun = lazy->functionNonDelazifying();
            MOZ_ASSERT(fun);
            JSScript* script = fun->nonLazyScript();

            if (shadowZone()->needsIncrementalBarrier())
                js::LazyScript::writeBarrierPre(lazy);

            flags_ &= ~INTERPRETED_LAZY;
            flags_ |= INTERPRETED;
            initScript(script);
        }
        return nonLazyScript();
    }

    JSScript* nonLazyScript() const {
        MOZ_ASSERT(hasScript());
        MOZ_ASSERT(u.i.s.script_);
        return u.i.s.script_;
    }

    bool getLength(JSContext* cx, uint16_t* length) {
        JS::RootedFunction self(cx, this);
        if (self->isInterpretedLazy() && !self->getOrCreateScript(cx))
            return false;

        *length = self->hasScript() ? self->nonLazyScript()->funLength()
                                    : (self->nargs() - self->hasRest());
        return true;
    }

    js::LazyScript* lazyScript() const {
        MOZ_ASSERT(isInterpretedLazy() && u.i.s.lazy_);
        return u.i.s.lazy_;
    }

    js::LazyScript* lazyScriptOrNull() const {
        MOZ_ASSERT(isInterpretedLazy());
        return u.i.s.lazy_;
    }

    js::GeneratorKind generatorKind() const {
        if (!isInterpreted())
            return js::NotGenerator;
        if (hasScript())
            return nonLazyScript()->generatorKind();
        if (js::LazyScript* lazy = lazyScriptOrNull())
            return lazy->generatorKind();
        MOZ_ASSERT(isSelfHostedBuiltin());
        return js::NotGenerator;
    }

    bool isGenerator() const { return generatorKind() != js::NotGenerator; }

    bool isLegacyGenerator() const { return generatorKind() == js::LegacyGenerator; }

    bool isStarGenerator() const { return generatorKind() == js::StarGenerator; }

    void setScript(JSScript* script_) {
        mutableScript() = script_;
    }

    void initScript(JSScript* script_) {
        mutableScript().init(script_);
    }

    void setUnlazifiedScript(JSScript* script) {
        
        
        MOZ_ASSERT(isInterpretedLazy());
        if (!lazyScript()->maybeScript())
            lazyScript()->initScript(script);
        flags_ &= ~INTERPRETED_LAZY;
        flags_ |= INTERPRETED;
        initScript(script);
    }

    void initLazyScript(js::LazyScript* lazy) {
        MOZ_ASSERT(isInterpreted());
        flags_ &= ~INTERPRETED;
        flags_ |= INTERPRETED_LAZY;
        u.i.s.lazy_ = lazy;
    }

    JSNative native() const {
        MOZ_ASSERT(isNative());
        return u.n.native;
    }

    JSNative maybeNative() const {
        return isInterpreted() ? nullptr : native();
    }

    void initNative(js::Native native, const JSJitInfo* jitinfo) {
        MOZ_ASSERT(native);
        u.n.native = native;
        u.n.jitinfo = jitinfo;
    }

    const JSJitInfo* jitInfo() const {
        MOZ_ASSERT(isNative());
        return u.n.jitinfo;
    }

    void setJitInfo(const JSJitInfo* data) {
        MOZ_ASSERT(isNative());
        u.n.jitinfo = data;
    }

    static unsigned offsetOfNativeOrScript() {
        static_assert(offsetof(U, n.native) == offsetof(U, i.s.script_),
                      "native and script pointers must be in the same spot "
                      "for offsetOfNativeOrScript() have any sense");
        static_assert(offsetof(U, n.native) == offsetof(U, nativeOrScript),
                      "U::nativeOrScript must be at the same offset as "
                      "native");

        return offsetof(JSFunction, u.nativeOrScript);
    }

#if JS_BITS_PER_WORD == 32
    static const js::gc::AllocKind FinalizeKind = js::gc::AllocKind::OBJECT2_BACKGROUND;
    static const js::gc::AllocKind ExtendedFinalizeKind = js::gc::AllocKind::OBJECT4_BACKGROUND;
#else
    static const js::gc::AllocKind FinalizeKind = js::gc::AllocKind::OBJECT4_BACKGROUND;
    static const js::gc::AllocKind ExtendedFinalizeKind = js::gc::AllocKind::OBJECT8_BACKGROUND;
#endif

    inline void trace(JSTracer* trc);

    

    inline bool initBoundFunction(JSContext* cx, js::HandleObject target, js::HandleValue thisArg,
                                  const js::Value* args, unsigned argslen);

    JSObject* getBoundFunctionTarget() const;
    const js::Value& getBoundFunctionThis() const;
    const js::Value& getBoundFunctionArgument(unsigned which) const;
    size_t getBoundFunctionArgumentCount() const;

  private:
    js::HeapPtrScript& mutableScript() {
        MOZ_ASSERT(hasScript());
        return *(js::HeapPtrScript*)&u.i.s.script_;
    }

    inline js::FunctionExtended* toExtended();
    inline const js::FunctionExtended* toExtended() const;

  public:
    inline bool isExtended() const {
        MOZ_ASSERT_IF(isTenured(), !!(flags() & EXTENDED) == (asTenured().getAllocKind() == ExtendedFinalizeKind));
        return !!(flags() & EXTENDED);
    }

    




    inline void initializeExtended();
    inline void initExtendedSlot(size_t which, const js::Value& val);
    inline void setExtendedSlot(size_t which, const js::Value& val);
    inline const js::Value& getExtendedSlot(size_t which) const;

    
    static bool setTypeForScriptedFunction(js::ExclusiveContext* cx, js::HandleFunction fun,
                                           bool singleton = false);

    
    js::gc::AllocKind getAllocKind() const {
        static_assert(FinalizeKind != ExtendedFinalizeKind,
                      "extended/non-extended AllocKinds have to be different "
                      "for getAllocKind() to have a reason to exist");

        js::gc::AllocKind kind = FinalizeKind;
        if (isExtended())
            kind = ExtendedFinalizeKind;
        MOZ_ASSERT_IF(isTenured(), kind == asTenured().getAllocKind());
        return kind;
    }
};

static_assert(sizeof(JSFunction) == sizeof(js::shadow::Function),
              "shadow interface must match actual interface");

extern JSString*
fun_toStringHelper(JSContext* cx, js::HandleObject obj, unsigned indent);

namespace js {

extern bool
Function(JSContext* cx, unsigned argc, Value* vp);

extern bool
Generator(JSContext* cx, unsigned argc, Value* vp);


extern JSFunction*
NewNativeFunction(ExclusiveContext* cx, JSNative native, unsigned nargs, HandleAtom atom,
                  gc::AllocKind allocKind = JSFunction::FinalizeKind,
                  NewObjectKind newKind = GenericObject);


extern JSFunction*
NewNativeConstructor(ExclusiveContext* cx, JSNative native, unsigned nargs, HandleAtom atom,
                     gc::AllocKind allocKind = JSFunction::FinalizeKind,
                     NewObjectKind newKind = GenericObject,
                     JSFunction::Flags flags = JSFunction::NATIVE_CTOR);




extern JSFunction*
NewScriptedFunction(ExclusiveContext* cx, unsigned nargs, JSFunction::Flags flags,
                    HandleAtom atom, gc::AllocKind allocKind = JSFunction::FinalizeKind,
                    NewObjectKind newKind = GenericObject,
                    HandleObject enclosingDynamicScope = NullPtr());





extern JSFunction*
NewFunctionWithProto(ExclusiveContext* cx, JSNative native, unsigned nargs,
                     JSFunction::Flags flags, HandleObject enclosingDynamicScope, HandleAtom atom,
                     HandleObject proto, gc::AllocKind allocKind = JSFunction::FinalizeKind,
                     NewObjectKind newKind = GenericObject);

extern JSAtom*
IdToFunctionName(JSContext* cx, HandleId id);

extern JSFunction*
DefineFunction(JSContext* cx, HandleObject obj, HandleId id, JSNative native,
               unsigned nargs, unsigned flags,
               gc::AllocKind allocKind = JSFunction::FinalizeKind,
               NewObjectKind newKind = GenericObject);

bool
FunctionHasResolveHook(const JSAtomState& atomState, jsid id);

extern bool
fun_resolve(JSContext* cx, HandleObject obj, HandleId id, bool* resolvedp);

extern bool
fun_toString(JSContext* cx, unsigned argc, Value* vp);

extern bool
fun_bind(JSContext* cx, unsigned argc, Value* vp);






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

extern bool
CloneFunctionObjectUseSameScript(JSCompartment* compartment, HandleFunction fun,
                                 HandleObject newParent);

extern JSFunction*
CloneFunctionObject(JSContext* cx, HandleFunction fun, HandleObject parent,
                    gc::AllocKind kind = JSFunction::FinalizeKind,
                    NewObjectKind newKindArg = GenericObject,
                    HandleObject proto = NullPtr());

extern bool
FindBody(JSContext* cx, HandleFunction fun, HandleLinearString src, size_t* bodyStart,
         size_t* bodyEnd);

} 

inline js::FunctionExtended*
JSFunction::toExtended()
{
    MOZ_ASSERT(isExtended());
    return static_cast<js::FunctionExtended*>(this);
}

inline const js::FunctionExtended*
JSFunction::toExtended() const
{
    MOZ_ASSERT(isExtended());
    return static_cast<const js::FunctionExtended*>(this);
}

inline void
JSFunction::initializeExtended()
{
    MOZ_ASSERT(isExtended());

    MOZ_ASSERT(mozilla::ArrayLength(toExtended()->extendedSlots) == 2);
    toExtended()->extendedSlots[0].init(js::UndefinedValue());
    toExtended()->extendedSlots[1].init(js::UndefinedValue());
}

inline void
JSFunction::initExtendedSlot(size_t which, const js::Value& val)
{
    MOZ_ASSERT(which < mozilla::ArrayLength(toExtended()->extendedSlots));
    toExtended()->extendedSlots[which].init(val);
}

inline void
JSFunction::setExtendedSlot(size_t which, const js::Value& val)
{
    MOZ_ASSERT(which < mozilla::ArrayLength(toExtended()->extendedSlots));
    toExtended()->extendedSlots[which] = val;
}

inline const js::Value&
JSFunction::getExtendedSlot(size_t which) const
{
    MOZ_ASSERT(which < mozilla::ArrayLength(toExtended()->extendedSlots));
    return toExtended()->extendedSlots[which];
}

namespace js {

JSString* FunctionToString(JSContext* cx, HandleFunction fun, bool bodyOnly, bool lambdaParen);

template<XDRMode mode>
bool
XDRInterpretedFunction(XDRState<mode>* xdr, HandleObject enclosingScope,
                       HandleScript enclosingScript, MutableHandleFunction objp);

extern JSObject*
CloneFunctionAndScript(JSContext* cx, HandleObject enclosingScope, HandleFunction fun,
                       PollutedGlobalScopeOption polluted);






extern void
ReportIncompatibleMethod(JSContext* cx, CallReceiver call, const Class* clasp);





extern void
ReportIncompatible(JSContext* cx, CallReceiver call);

bool
CallOrConstructBoundFunction(JSContext*, unsigned, js::Value*);

extern const JSFunctionSpec function_methods[];

extern bool
fun_apply(JSContext* cx, unsigned argc, Value* vp);

extern bool
fun_call(JSContext* cx, unsigned argc, Value* vp);

extern JSObject*
fun_bind(JSContext* cx, HandleObject target, HandleValue thisArg,
         Value* boundArgs, unsigned argslen);

} 

#ifdef DEBUG
namespace JS {
namespace detail {

JS_PUBLIC_API(void)
CheckIsValidConstructible(Value calleev);

} 
} 
#endif

#endif 
