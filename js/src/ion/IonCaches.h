





#ifndef ion_IonCaches_h
#define ion_IonCaches_h

#ifdef JS_CPU_ARM
# include "ion/arm/Assembler-arm.h"
#endif
#include "ion/IonCode.h"
#include "ion/Registers.h"
#include "ion/shared/Assembler-shared.h"
#include "vm/ForkJoin.h"

class JSFunction;
class JSScript;

namespace js {

class TypedArrayObject;

namespace ion {

#define IONCACHE_KIND_LIST(_)                                   \
    _(GetProperty)                                              \
    _(SetProperty)                                              \
    _(GetElement)                                               \
    _(SetElement)                                               \
    _(BindName)                                                 \
    _(Name)                                                     \
    _(CallsiteClone)                                            \
    _(GetPropertyPar)                                           \
    _(GetElementPar)


#define FORWARD_DECLARE(kind) class kind##IC;
IONCACHE_KIND_LIST(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class IonCacheVisitor
{
  public:
#define VISIT_INS(op)                                               \
    virtual bool visit##op##IC(CodeGenerator *codegen, op##IC *) {  \
        MOZ_ASSUME_UNREACHABLE("NYI: " #op "IC");                   \
    }

    IONCACHE_KIND_LIST(VISIT_INS)
#undef VISIT_INS
};



struct AddCacheState
{
    RepatchLabel repatchEntry;
    Register dispatchScratch;
};



































































class IonCache
{
  public:
    class StubAttacher;

    enum Kind {
#   define DEFINE_CACHEKINDS(ickind) Cache_##ickind,
        IONCACHE_KIND_LIST(DEFINE_CACHEKINDS)
#   undef DEFINE_CACHEKINDS
        Cache_Invalid
    };

    
#   define CACHEKIND_CASTS(ickind)                                      \
    bool is##ickind() const {                                           \
        return kind() == Cache_##ickind;                                \
    }                                                                   \
    inline ickind##IC &to##ickind();

    IONCACHE_KIND_LIST(CACHEKIND_CASTS)
#   undef CACHEKIND_CASTS

    virtual Kind kind() const = 0;

    virtual bool accept(CodeGenerator *codegen, IonCacheVisitor *visitor) = 0;

  public:

    static const char *CacheName(Kind kind);

  protected:
    bool pure_ : 1;
    bool idempotent_ : 1;
    bool disabled_ : 1;
    size_t stubCount_ : 5;

    CodeLocationLabel fallbackLabel_;

    
    JSScript *script;
    jsbytecode *pc;

  private:
    static const size_t MAX_STUBS;
    void incrementStubCount() {
        
        stubCount_++;
        JS_ASSERT(stubCount_);
    }

  public:

    IonCache()
      : pure_(false),
        idempotent_(false),
        disabled_(false),
        stubCount_(0),
        fallbackLabel_(),
        script(NULL),
        pc(NULL)
    {
    }

    virtual void disable();
    inline bool isDisabled() const {
        return disabled_;
    }

    
    
    
    void setFallbackLabel(CodeOffsetLabel fallbackLabel) {
        fallbackLabel_ = fallbackLabel;
    }

    virtual void emitInitialJump(MacroAssembler &masm, AddCacheState &addState) = 0;
    virtual void bindInitialJump(MacroAssembler &masm, AddCacheState &addState) = 0;
    virtual void updateBaseAddress(IonCode *code, MacroAssembler &masm);

    
    
    virtual void initializeAddCacheState(LInstruction *ins, AddCacheState *addState);

    
    virtual void reset();

    
    virtual void destroy();

    bool canAttachStub() const {
        return stubCount_ < MAX_STUBS;
    }
    bool empty() const {
        return stubCount_ == 0;
    }

    enum LinkStatus {
        LINK_ERROR,
        CACHE_FLUSHED,
        LINK_GOOD
    };

    
    
    
    
    LinkStatus linkCode(JSContext *cx, MacroAssembler &masm, IonScript *ion, IonCode **code);
    
    
    void attachStub(MacroAssembler &masm, StubAttacher &attacher, Handle<IonCode *> code);

    
    
    bool linkAndAttachStub(JSContext *cx, MacroAssembler &masm, StubAttacher &attacher,
                           IonScript *ion, const char *attachKind);

#ifdef DEBUG
    bool isAllocated() {
        return fallbackLabel_.isSet();
    }
#endif

    bool pure() {
        return pure_;
    }
    bool idempotent() {
        return idempotent_;
    }
    void setIdempotent() {
        JS_ASSERT(!idempotent_);
        JS_ASSERT(!script);
        JS_ASSERT(!pc);
        idempotent_ = true;
    }

    void setScriptedLocation(JSScript *script, jsbytecode *pc) {
        JS_ASSERT(!idempotent_);
        this->script = script;
        this->pc = pc;
    }

    void getScriptedLocation(MutableHandleScript pscript, jsbytecode **ppc) {
        pscript.set(script);
        *ppc = pc;
    }
};































































class RepatchIonCache : public IonCache
{
  protected:
    class RepatchStubAppender;

    CodeLocationJump initialJump_;
    CodeLocationJump lastJump_;

    
#ifdef JS_CPU_ARM
    static const size_t REJOIN_LABEL_OFFSET = 4;
#else
    static const size_t REJOIN_LABEL_OFFSET = 0;
#endif

    CodeLocationLabel rejoinLabel() const {
        uint8_t *ptr = initialJump_.raw();
#ifdef JS_CPU_ARM
        uint32_t i = 0;
        while (i < REJOIN_LABEL_OFFSET)
            ptr = Assembler::nextInstruction(ptr, &i);
#endif
        return CodeLocationLabel(ptr);
    }

  public:
    RepatchIonCache()
      : initialJump_(),
        lastJump_()
    {
    }

    virtual void reset();

    
    
    
    
    void emitInitialJump(MacroAssembler &masm, AddCacheState &addState);
    void bindInitialJump(MacroAssembler &masm, AddCacheState &addState);

    
    void updateBaseAddress(IonCode *code, MacroAssembler &masm);
};








































































class DispatchIonCache : public IonCache
{
  protected:
    class DispatchStubPrepender;

    uint8_t *firstStub_;
    CodeLocationLabel rejoinLabel_;
    CodeOffsetLabel dispatchLabel_;

  public:
    DispatchIonCache()
      : firstStub_(NULL),
        rejoinLabel_(),
        dispatchLabel_()
    {
    }

    virtual void reset();
    virtual void initializeAddCacheState(LInstruction *ins, AddCacheState *addState);

    void emitInitialJump(MacroAssembler &masm, AddCacheState &addState);
    void bindInitialJump(MacroAssembler &masm, AddCacheState &addState);

    
    void updateBaseAddress(IonCode *code, MacroAssembler &masm);
};



#define CACHE_HEADER(ickind)                                        \
    Kind kind() const {                                             \
        return IonCache::Cache_##ickind;                            \
    }                                                               \
                                                                    \
    bool accept(CodeGenerator *codegen, IonCacheVisitor *visitor) { \
        return visitor->visit##ickind##IC(codegen, this);           \
    }                                                               \
                                                                    \
    static const VMFunction UpdateInfo;




class GetPropertyIC : public RepatchIonCache
{
  protected:
    
    
    RegisterSet liveRegs_;

    Register object_;
    PropertyName *name_;
    TypedOrValueRegister output_;
    bool allowGetters_ : 1;
    bool hasArrayLengthStub_ : 1;
    bool hasTypedArrayLengthStub_ : 1;
    bool hasStrictArgumentsLengthStub_ : 1;
    bool hasNormalArgumentsLengthStub_ : 1;

  public:
    GetPropertyIC(RegisterSet liveRegs,
                  Register object, PropertyName *name,
                  TypedOrValueRegister output,
                  bool allowGetters)
      : liveRegs_(liveRegs),
        object_(object),
        name_(name),
        output_(output),
        allowGetters_(allowGetters),
        hasArrayLengthStub_(false),
        hasTypedArrayLengthStub_(false),
        hasStrictArgumentsLengthStub_(false),
        hasNormalArgumentsLengthStub_(false)
    {
    }

    CACHE_HEADER(GetProperty)

    void reset();

    Register object() const {
        return object_;
    }
    PropertyName *name() const {
        return name_;
    }
    TypedOrValueRegister output() const {
        return output_;
    }
    bool allowGetters() const {
        return allowGetters_;
    }
    bool hasArrayLengthStub() const {
        return hasArrayLengthStub_;
    }
    bool hasTypedArrayLengthStub() const {
        return hasTypedArrayLengthStub_;
    }
    bool hasArgumentsLengthStub(bool strict) const {
        return strict ? hasStrictArgumentsLengthStub_ : hasNormalArgumentsLengthStub_;
    }

    bool attachReadSlot(JSContext *cx, IonScript *ion, JSObject *obj, JSObject *holder,
                        HandleShape shape);
    bool attachDOMProxyShadowed(JSContext *cx, IonScript *ion, JSObject *obj, void *returnAddr);
    bool attachCallGetter(JSContext *cx, IonScript *ion, JSObject *obj, JSObject *holder,
                          HandleShape shape,
                          const SafepointIndex *safepointIndex, void *returnAddr);
    bool attachArrayLength(JSContext *cx, IonScript *ion, JSObject *obj);
    bool attachTypedArrayLength(JSContext *cx, IonScript *ion, JSObject *obj);
    bool attachArgumentsLength(JSContext *cx, IonScript *ion, JSObject *obj);

    static bool update(JSContext *cx, size_t cacheIndex, HandleObject obj, MutableHandleValue vp);
};

class SetPropertyIC : public RepatchIonCache
{
  protected:
    
    
    RegisterSet liveRegs_;

    Register object_;
    PropertyName *name_;
    ConstantOrRegister value_;
    bool isSetName_;
    bool strict_;

  public:
    SetPropertyIC(RegisterSet liveRegs, Register object, PropertyName *name,
                  ConstantOrRegister value, bool isSetName, bool strict)
      : liveRegs_(liveRegs),
        object_(object),
        name_(name),
        value_(value),
        isSetName_(isSetName),
        strict_(strict)
    {
    }

    CACHE_HEADER(SetProperty)

    Register object() const {
        return object_;
    }
    PropertyName *name() const {
        return name_;
    }
    ConstantOrRegister value() const {
        return value_;
    }
    bool isSetName() const {
        return isSetName_;
    }
    bool strict() const {
        return strict_;
    }

    bool attachNativeExisting(JSContext *cx, IonScript *ion, HandleObject obj, HandleShape shape);
    bool attachSetterCall(JSContext *cx, IonScript *ion, HandleObject obj,
                          HandleObject holder, HandleShape shape, void *returnAddr);
    bool attachNativeAdding(JSContext *cx, IonScript *ion, JSObject *obj, HandleShape oldshape,
                            HandleShape newshape, HandleShape propshape);

    static bool
    update(JSContext *cx, size_t cacheIndex, HandleObject obj, HandleValue value);
};

class GetElementIC : public RepatchIonCache
{
  protected:
    Register object_;
    ConstantOrRegister index_;
    TypedOrValueRegister output_;

    bool monitoredResult_ : 1;
    bool hasDenseStub_ : 1;
    bool hasStrictArgumentsStub_ : 1;
    bool hasNormalArgumentsStub_ : 1;

    size_t failedUpdates_;

    static const size_t MAX_FAILED_UPDATES;

  public:
    GetElementIC(Register object, ConstantOrRegister index,
                 TypedOrValueRegister output, bool monitoredResult)
      : object_(object),
        index_(index),
        output_(output),
        monitoredResult_(monitoredResult),
        hasDenseStub_(false),
        hasStrictArgumentsStub_(false),
        hasNormalArgumentsStub_(false),
        failedUpdates_(0)
    {
    }

    CACHE_HEADER(GetElement)

    void reset();

    Register object() const {
        return object_;
    }
    ConstantOrRegister index() const {
        return index_;
    }
    TypedOrValueRegister output() const {
        return output_;
    }
    bool monitoredResult() const {
        return monitoredResult_;
    }
    bool hasDenseStub() const {
        return hasDenseStub_;
    }
    bool hasArgumentsStub(bool strict) const {
        return strict ? hasStrictArgumentsStub_ : hasNormalArgumentsStub_;
    }
    void setHasDenseStub() {
        JS_ASSERT(!hasDenseStub());
        hasDenseStub_ = true;
    }

    static bool canAttachGetProp(JSObject *obj, const Value &idval, jsid id);
    static bool canAttachDenseElement(JSObject *obj, const Value &idval);
    static bool canAttachTypedArrayElement(JSObject *obj, const Value &idval,
                                           TypedOrValueRegister output);

    bool attachGetProp(JSContext *cx, IonScript *ion, HandleObject obj, const Value &idval, HandlePropertyName name);
    bool attachDenseElement(JSContext *cx, IonScript *ion, JSObject *obj, const Value &idval);
    bool attachTypedArrayElement(JSContext *cx, IonScript *ion, TypedArrayObject *tarr,
                                 const Value &idval);
    bool attachArgumentsElement(JSContext *cx, IonScript *ion, JSObject *obj);

    static bool
    update(JSContext *cx, size_t cacheIndex, HandleObject obj, HandleValue idval,
                MutableHandleValue vp);

    void incFailedUpdates() {
        failedUpdates_++;
    }
    void resetFailedUpdates() {
        failedUpdates_ = 0;
    }
    bool shouldDisable() const {
        return !canAttachStub() ||
               (stubCount_ == 0 && failedUpdates_ > MAX_FAILED_UPDATES);
    }
};

class SetElementIC : public RepatchIonCache
{
  protected:
    Register object_;
    Register temp0_;
    Register temp1_;
    ValueOperand index_;
    ConstantOrRegister value_;
    bool strict_;

    bool hasDenseStub_ : 1;

  public:
    SetElementIC(Register object, Register temp0, Register temp1,
                 ValueOperand index, ConstantOrRegister value,
                 bool strict)
      : object_(object),
        temp0_(temp0),
        temp1_(temp1),
        index_(index),
        value_(value),
        strict_(strict),
        hasDenseStub_(false)
    {
    }

    CACHE_HEADER(SetElement)

    void reset();

    Register object() const {
        return object_;
    }
    Register temp0() const {
        return temp0_;
    }
    Register temp1() const {
        return temp1_;
    }
    ValueOperand index() const {
        return index_;
    }
    ConstantOrRegister value() const {
        return value_;
    }
    bool strict() const {
        return strict_;
    }

    bool hasDenseStub() const {
        return hasDenseStub_;
    }
    void setHasDenseStub() {
        JS_ASSERT(!hasDenseStub());
        hasDenseStub_ = true;
    }

    bool attachDenseElement(JSContext *cx, IonScript *ion, JSObject *obj, const Value &idval);

    static bool
    update(JSContext *cx, size_t cacheIndex, HandleObject obj, HandleValue idval,
                HandleValue value);
};

class BindNameIC : public RepatchIonCache
{
  protected:
    Register scopeChain_;
    PropertyName *name_;
    Register output_;

  public:
    BindNameIC(Register scopeChain, PropertyName *name, Register output)
      : scopeChain_(scopeChain),
        name_(name),
        output_(output)
    {
    }

    CACHE_HEADER(BindName)

    Register scopeChainReg() const {
        return scopeChain_;
    }
    HandlePropertyName name() const {
        return HandlePropertyName::fromMarkedLocation(&name_);
    }
    Register outputReg() const {
        return output_;
    }

    bool attachGlobal(JSContext *cx, IonScript *ion, JSObject *scopeChain);
    bool attachNonGlobal(JSContext *cx, IonScript *ion, JSObject *scopeChain, JSObject *holder);

    static JSObject *
    update(JSContext *cx, size_t cacheIndex, HandleObject scopeChain);
};

class NameIC : public RepatchIonCache
{
  protected:
    
    
    RegisterSet liveRegs_;

    bool typeOf_;
    Register scopeChain_;
    PropertyName *name_;
    TypedOrValueRegister output_;

  public:
    NameIC(RegisterSet liveRegs, bool typeOf,
           Register scopeChain, PropertyName *name,
           TypedOrValueRegister output)
      : liveRegs_(liveRegs),
        typeOf_(typeOf),
        scopeChain_(scopeChain),
        name_(name),
        output_(output)
    {
    }

    CACHE_HEADER(Name)

    Register scopeChainReg() const {
        return scopeChain_;
    }
    HandlePropertyName name() const {
        return HandlePropertyName::fromMarkedLocation(&name_);
    }
    TypedOrValueRegister outputReg() const {
        return output_;
    }
    bool isTypeOf() const {
        return typeOf_;
    }

    bool attachReadSlot(JSContext *cx, IonScript *ion, HandleObject scopeChain, HandleObject obj,
                        HandleShape shape);
    bool attachCallGetter(JSContext *cx, IonScript *ion, JSObject *obj, JSObject *holder,
                          HandleShape shape, const SafepointIndex *safepointIndex,
                          void *returnAddr);

    static bool
    update(JSContext *cx, size_t cacheIndex, HandleObject scopeChain, MutableHandleValue vp);
};

class CallsiteCloneIC : public RepatchIonCache
{
  protected:
    Register callee_;
    Register output_;
    JSScript *callScript_;
    jsbytecode *callPc_;

  public:
    CallsiteCloneIC(Register callee, JSScript *callScript, jsbytecode *callPc, Register output)
      : callee_(callee),
        output_(output),
        callScript_(callScript),
        callPc_(callPc)
    {
    }

    CACHE_HEADER(CallsiteClone)

    Register calleeReg() const {
        return callee_;
    }
    HandleScript callScript() const {
        return HandleScript::fromMarkedLocation(&callScript_);
    }
    jsbytecode *callPc() const {
        return callPc_;
    }
    Register outputReg() const {
        return output_;
    }

    bool attach(JSContext *cx, IonScript *ion, HandleFunction original, HandleFunction clone);

    static JSObject *update(JSContext *cx, size_t cacheIndex, HandleObject callee);
};

class ParallelIonCache : public DispatchIonCache
{
  protected:
    
    
    ShapeSet *stubbedShapes_;

    ParallelIonCache()
      : stubbedShapes_(NULL)
    {
    }

    bool initStubbedShapes(JSContext *cx);

  public:
    void reset();
    void destroy();

    bool hasOrAddStubbedShape(LockedJSContext &cx, Shape *shape, bool *alreadyStubbed);
};

class GetPropertyParIC : public ParallelIonCache
{
  protected:
    Register object_;
    PropertyName *name_;
    TypedOrValueRegister output_;
    bool hasTypedArrayLengthStub_ : 1;

   public:
    GetPropertyParIC(Register object, PropertyName *name, TypedOrValueRegister output)
      : object_(object),
        name_(name),
        output_(output),
        hasTypedArrayLengthStub_(false)
    {
    }

    CACHE_HEADER(GetPropertyPar)

#ifdef JS_CPU_X86
    
    
    void initializeAddCacheState(LInstruction *ins, AddCacheState *addState);
#endif

    void reset();

    Register object() const {
        return object_;
    }
    PropertyName *name() const {
        return name_;
    }
    TypedOrValueRegister output() const {
        return output_;
    }
    bool hasTypedArrayLengthStub() const {
        return hasTypedArrayLengthStub_;
    }

    static bool canAttachReadSlot(LockedJSContext &cx, IonCache &cache,
                                  TypedOrValueRegister output, JSObject *obj,
                                  PropertyName *name, MutableHandleObject holder,
                                  MutableHandleShape shape);

    bool attachReadSlot(LockedJSContext &cx, IonScript *ion, JSObject *obj, JSObject *holder,
                        Shape *shape);
    bool attachArrayLength(LockedJSContext &cx, IonScript *ion, JSObject *obj);
    bool attachTypedArrayLength(LockedJSContext &cx, IonScript *ion, JSObject *obj);

    static ParallelResult update(ForkJoinSlice *slice, size_t cacheIndex, HandleObject obj,
                                 MutableHandleValue vp);
};

class GetElementParIC : public ParallelIonCache
{
  protected:
    Register object_;
    ConstantOrRegister index_;
    TypedOrValueRegister output_;

    bool monitoredResult_ : 1;

  public:
    GetElementParIC(Register object, ConstantOrRegister index,
                    TypedOrValueRegister output, bool monitoredResult)
      : object_(object),
        index_(index),
        output_(output),
        monitoredResult_(monitoredResult)
    {
    }

    CACHE_HEADER(GetElementPar)

#ifdef JS_CPU_X86
    
    
    void initializeAddCacheState(LInstruction *ins, AddCacheState *addState);
#endif

    Register object() const {
        return object_;
    }
    ConstantOrRegister index() const {
        return index_;
    }
    TypedOrValueRegister output() const {
        return output_;
    }
    bool monitoredResult() const {
        return monitoredResult_;
    }

    bool attachReadSlot(LockedJSContext &cx, IonScript *ion, JSObject *obj, const Value &idval,
                        PropertyName *name, JSObject *holder, Shape *shape);
    bool attachDenseElement(LockedJSContext &cx, IonScript *ion, JSObject *obj, const Value &idval);
    bool attachTypedArrayElement(LockedJSContext &cx, IonScript *ion, TypedArrayObject *tarr,
                                 const Value &idval);

    static ParallelResult update(ForkJoinSlice *slice, size_t cacheIndex, HandleObject obj, HandleValue idval,
                                 MutableHandleValue vp);

};

#undef CACHE_HEADER


#define CACHE_CASTS(ickind)                                             \
    ickind##IC &IonCache::to##ickind()                                  \
    {                                                                   \
        JS_ASSERT(is##ickind());                                        \
        return *static_cast<ickind##IC *>(this);                        \
    }
IONCACHE_KIND_LIST(CACHE_CASTS)
#undef OPCODE_CASTS

} 
} 

#endif 
