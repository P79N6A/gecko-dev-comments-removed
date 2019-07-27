





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
        ClassConstructor,
        Getter,
        Setter,
        AsmJS,                      
        FunctionKindLimit
    };

    enum Flags {
        INTERPRETED      = 0x0001,  
        CONSTRUCTOR      = 0x0002,  
        EXTENDED         = 0x0004,  
        IS_FUN_PROTO     = 0x0008,  
        EXPR_BODY        = 0x0010,  

        HAS_GUESSED_ATOM = 0x0020,  

        LAMBDA           = 0x0040,  


        SELF_HOSTED      = 0x0080,  

        HAS_REST         = 0x0100,  
        INTERPRETED_LAZY = 0x0200,  
        RESOLVED_LENGTH  = 0x0400,  
        RESOLVED_NAME    = 0x0800,  

        FUNCTION_KIND_SHIFT = 12,
        FUNCTION_KIND_MASK  = 0xf << FUNCTION_KIND_SHIFT,

        ASMJS_KIND = AsmJS << FUNCTION_KIND_SHIFT,
        ARROW_KIND = Arrow << FUNCTION_KIND_SHIFT,
        METHOD_KIND = Method << FUNCTION_KIND_SHIFT,
        CLASSCONSTRUCTOR_KIND = ClassConstructor << FUNCTION_KIND_SHIFT,
        GETTER_KIND = Getter << FUNCTION_KIND_SHIFT,
        SETTER_KIND = Setter << FUNCTION_KIND_SHIFT,

        
        NATIVE_FUN = 0,
        NATIVE_CTOR = NATIVE_FUN | CONSTRUCTOR,
        ASMJS_CTOR = ASMJS_KIND | NATIVE_CTOR,
        ASMJS_LAMBDA_CTOR = ASMJS_KIND | NATIVE_CTOR | LAMBDA,
        INTERPRETED_METHOD = INTERPRETED | METHOD_KIND,
        INTERPRETED_METHOD_GENERATOR = INTERPRETED | METHOD_KIND | CONSTRUCTOR,
        INTERPRETED_CLASS_CONSTRUCTOR = INTERPRETED | CLASSCONSTRUCTOR_KIND | CONSTRUCTOR,
        INTERPRETED_GETTER = INTERPRETED | GETTER_KIND,
        INTERPRETED_SETTER = INTERPRETED | SETTER_KIND,
        INTERPRETED_LAMBDA = INTERPRETED | LAMBDA | CONSTRUCTOR,
        INTERPRETED_LAMBDA_ARROW = INTERPRETED | LAMBDA | ARROW_KIND,
        INTERPRETED_NORMAL = INTERPRETED | CONSTRUCTOR,
        NO_XDR_FLAGS = RESOLVED_LENGTH | RESOLVED_NAME,

        STABLE_ACROSS_CLONES = IS_FUN_PROTO | CONSTRUCTOR | EXPR_BODY | HAS_GUESSED_ATOM |
                               LAMBDA | SELF_HOSTED |  HAS_REST | FUNCTION_KIND_MASK
    };

    static_assert((INTERPRETED | INTERPRETED_LAZY) == js::JS_FUNCTION_INTERPRETED_BITS,
                  "jsfriendapi.h's JSFunction::INTERPRETED-alike is wrong");
    static_assert(((FunctionKindLimit - 1) << FUNCTION_KIND_SHIFT) <= FUNCTION_KIND_MASK,
                  "FunctionKind doesn't fit into flags_");

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
               nonLazyScript()->needsHomeObject()       ||
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

    bool isConstructor()            const { return flags() & CONSTRUCTOR; }

    
    bool isAsmJSNative()            const { return kind() == AsmJS; }

    
    bool isFunctionPrototype()      const { return flags() & IS_FUN_PROTO; }
    bool isExprBody()               const { return flags() & EXPR_BODY; }
    bool hasGuessedAtom()           const { return flags() & HAS_GUESSED_ATOM; }
    bool isLambda()                 const { return flags() & LAMBDA; }
    bool isSelfHostedBuiltin()      const { return flags() & SELF_HOSTED; }
    bool hasRest()                  const { return flags() & HAS_REST; }
    bool isInterpretedLazy()        const { return flags() & INTERPRETED_LAZY; }
    bool hasScript()                const { return flags() & INTERPRETED; }

    
    bool isArrow()                  const { return kind() == Arrow; }
    
    bool isMethod()                 const { return kind() == Method || kind() == ClassConstructor; }
    bool isClassConstructor()       const { return kind() == ClassConstructor; }

    bool isGetter()                 const { return kind() == Getter; }
    bool isSetter()                 const { return kind() == Setter; }

    bool allowSuperProperty() const {
        return isMethod() || isGetter() || isSetter();
    }

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

    
    void setIsConstructor() {
        MOZ_ASSERT(!isConstructor());
        MOZ_ASSERT(isSelfHostedBuiltin());
        flags_ |= CONSTRUCTOR;
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
        
        flags_ &= ~CONSTRUCTOR;
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
    void maybeRelazify(JSRuntime* rt);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

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

    
    
    
    bool hasUncompiledScript() const {
        MOZ_ASSERT(hasScript());
        return !u.i.s.script_;
    }

    JSScript* nonLazyScript() const {
        MOZ_ASSERT(!hasUncompiledScript());
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
        bool extended = !!(flags() & EXTENDED);
        MOZ_ASSERT_IF(isTenured(),
                      extended == (asTenured().getAllocKind() == js::gc::AllocKind::FUNCTION_EXTENDED));
        return extended;
    }

    




    inline void initializeExtended();
    inline void initExtendedSlot(size_t which, const js::Value& val);
    inline void setExtendedSlot(size_t which, const js::Value& val);
    inline const js::Value& getExtendedSlot(size_t which) const;

    
    static bool setTypeForScriptedFunction(js::ExclusiveContext* cx, js::HandleFunction fun,
                                           bool singleton = false);

    
    js::gc::AllocKind getAllocKind() const {
        static_assert(js::gc::AllocKind::FUNCTION != js::gc::AllocKind::FUNCTION_EXTENDED,
                      "extended/non-extended AllocKinds have to be different "
                      "for getAllocKind() to have a reason to exist");

        js::gc::AllocKind kind = js::gc::AllocKind::FUNCTION;
        if (isExtended())
            kind = js::gc::AllocKind::FUNCTION_EXTENDED;
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
                  gc::AllocKind allocKind = gc::AllocKind::FUNCTION,
                  NewObjectKind newKind = GenericObject);


extern JSFunction*
NewNativeConstructor(ExclusiveContext* cx, JSNative native, unsigned nargs, HandleAtom atom,
                     gc::AllocKind allocKind = gc::AllocKind::FUNCTION,
                     NewObjectKind newKind = GenericObject,
                     JSFunction::Flags flags = JSFunction::NATIVE_CTOR);




extern JSFunction*
NewScriptedFunction(ExclusiveContext* cx, unsigned nargs, JSFunction::Flags flags,
                    HandleAtom atom, gc::AllocKind allocKind = gc::AllocKind::FUNCTION,
                    NewObjectKind newKind = GenericObject,
                    HandleObject enclosingDynamicScope = nullptr);





extern JSFunction*
NewFunctionWithProto(ExclusiveContext* cx, JSNative native, unsigned nargs,
                     JSFunction::Flags flags, HandleObject enclosingDynamicScope, HandleAtom atom,
                     HandleObject proto, gc::AllocKind allocKind = gc::AllocKind::FUNCTION,
                     NewObjectKind newKind = GenericObject);

extern JSAtom*
IdToFunctionName(JSContext* cx, HandleId id);

extern JSFunction*
DefineFunction(JSContext* cx, HandleObject obj, HandleId id, JSNative native,
               unsigned nargs, unsigned flags,
               gc::AllocKind allocKind = gc::AllocKind::FUNCTION,
               NewObjectKind newKind = GenericObject);

bool
FunctionHasResolveHook(const JSAtomState& atomState, jsid id);

extern bool
fun_toString(JSContext* cx, unsigned argc, Value* vp);

extern bool
fun_bind(JSContext* cx, unsigned argc, Value* vp);






class FunctionExtended : public JSFunction
{
  public:
    static const unsigned NUM_EXTENDED_SLOTS = 2;

    
    static const unsigned ARROW_THIS_SLOT = 0;
    static const unsigned ARROW_NEWTARGET_SLOT = 1;

    static const unsigned METHOD_HOMEOBJECT_SLOT = 0;

    static inline size_t offsetOfExtendedSlot(unsigned which) {
        MOZ_ASSERT(which < NUM_EXTENDED_SLOTS);
        return offsetof(FunctionExtended, extendedSlots) + which * sizeof(HeapValue);
    }
    static inline size_t offsetOfArrowThisSlot() {
        return offsetOfExtendedSlot(ARROW_THIS_SLOT);
    }
    static inline size_t offsetOfArrowNewTargetSlot() {
        return offsetOfExtendedSlot(ARROW_NEWTARGET_SLOT);
    }

  private:
    friend class JSFunction;

    
    HeapValue extendedSlots[NUM_EXTENDED_SLOTS];
};

extern bool
CanReuseScriptForClone(JSCompartment* compartment, HandleFunction fun, HandleObject newParent);

extern JSFunction*
CloneFunctionReuseScript(JSContext* cx, HandleFunction fun, HandleObject parent,
                         gc::AllocKind kind = gc::AllocKind::FUNCTION,
                         NewObjectKind newKindArg = GenericObject,
                         HandleObject proto = nullptr);


extern JSFunction*
CloneFunctionAndScript(JSContext* cx, HandleFunction fun, HandleObject parent,
                       HandleObject newStaticScope,
                       gc::AllocKind kind = gc::AllocKind::FUNCTION,
                       HandleObject proto = nullptr);

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
