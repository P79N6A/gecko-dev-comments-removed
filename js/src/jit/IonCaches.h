





#ifndef jit_IonCaches_h
#define jit_IonCaches_h

#if defined(JS_CODEGEN_ARM)
# include "jit/arm/Assembler-arm.h"
#elif defined(JS_CODEGEN_MIPS)
# include "jit/mips/Assembler-mips.h"
#endif
#include "jit/Registers.h"
#include "jit/shared/Assembler-shared.h"
#include "vm/TypedArrayCommon.h"

namespace js {
namespace jit {

class LInstruction;

#define IONCACHE_KIND_LIST(_)                                   \
    _(GetProperty)                                              \
    _(SetProperty)                                              \
    _(GetElement)                                               \
    _(SetElement)                                               \
    _(BindName)                                                 \
    _(Name)


#define FORWARD_DECLARE(kind) class kind##IC;
IONCACHE_KIND_LIST(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class IonCacheVisitor
{
  public:
#define VISIT_INS(op)                                               \
    virtual void visit##op##IC(CodeGenerator* codegen) {            \
        MOZ_CRASH("NYI: " #op "IC");                                \
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
    inline ickind##IC& to##ickind();                                    \
    inline const ickind##IC& to##ickind() const;
    IONCACHE_KIND_LIST(CACHEKIND_CASTS)
#   undef CACHEKIND_CASTS

    virtual Kind kind() const = 0;

    virtual void accept(CodeGenerator* codegen, IonCacheVisitor* visitor) = 0;

  public:

    static const char* CacheName(Kind kind);

  protected:
    bool pure_ : 1;
    bool idempotent_ : 1;
    bool disabled_ : 1;
    size_t stubCount_ : 5;

    CodeLocationLabel fallbackLabel_;

    
    JSScript* script_;
    jsbytecode* pc_;

    
    
    jsbytecode* profilerLeavePc_;

  private:
    static const size_t MAX_STUBS;
    void incrementStubCount() {
        
        stubCount_++;
        MOZ_ASSERT(stubCount_);
    }

  public:

    IonCache()
      : pure_(false),
        idempotent_(false),
        disabled_(false),
        stubCount_(0),
        fallbackLabel_(),
        script_(nullptr),
        pc_(nullptr),
        profilerLeavePc_(nullptr)
    {
    }

    virtual void disable();
    inline bool isDisabled() const {
        return disabled_;
    }

    
    
    
    void setFallbackLabel(CodeOffsetLabel fallbackLabel) {
        fallbackLabel_ = fallbackLabel;
    }

    void setProfilerLeavePC(jsbytecode* pc) {
        MOZ_ASSERT(pc != nullptr);
        profilerLeavePc_ = pc;
    }

    
    virtual void* rejoinAddress() = 0;

    virtual void emitInitialJump(MacroAssembler& masm, AddCacheState& addState) = 0;
    virtual void bindInitialJump(MacroAssembler& masm, AddCacheState& addState) = 0;
    virtual void updateBaseAddress(JitCode* code, MacroAssembler& masm);

    
    
    virtual void initializeAddCacheState(LInstruction* ins, AddCacheState* addState);

    
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

    
    
    
    
    LinkStatus linkCode(JSContext* cx, MacroAssembler& masm, IonScript* ion, JitCode** code);
    
    
    void attachStub(MacroAssembler& masm, StubAttacher& attacher, Handle<JitCode*> code);

    
    
    bool linkAndAttachStub(JSContext* cx, MacroAssembler& masm, StubAttacher& attacher,
                           IonScript* ion, const char* attachKind);

#ifdef DEBUG
    bool isAllocated() {
        return fallbackLabel_.isSet();
    }
#endif

    bool pure() const {
        return pure_;
    }
    bool idempotent() const {
        return idempotent_;
    }
    void setIdempotent() {
        MOZ_ASSERT(!idempotent_);
        MOZ_ASSERT(!script_);
        MOZ_ASSERT(!pc_);
        idempotent_ = true;
    }

    void setScriptedLocation(JSScript* script, jsbytecode* pc) {
        MOZ_ASSERT(!idempotent_);
        script_ = script;
        pc_ = pc;
    }

    void getScriptedLocation(MutableHandleScript pscript, jsbytecode** ppc) const {
        pscript.set(script_);
        *ppc = pc_;
    }

    jsbytecode* pc() const {
        MOZ_ASSERT(pc_);
        return pc_;
    }
};































































class RepatchIonCache : public IonCache
{
  protected:
    class RepatchStubAppender;

    CodeLocationJump initialJump_;
    CodeLocationJump lastJump_;

    
#ifdef JS_CODEGEN_ARM
    static const size_t REJOIN_LABEL_OFFSET = 4;
#elif defined(JS_CODEGEN_MIPS)
    
    static const size_t REJOIN_LABEL_OFFSET = 4 * sizeof(void*);
#else
    static const size_t REJOIN_LABEL_OFFSET = 0;
#endif

    CodeLocationLabel rejoinLabel() const {
        uint8_t* ptr = initialJump_.raw();
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
        uint32_t i = 0;
        while (i < REJOIN_LABEL_OFFSET)
            ptr = Assembler::NextInstruction(ptr, &i);
#endif
        return CodeLocationLabel(ptr);
    }

  public:
    RepatchIonCache()
      : initialJump_(),
        lastJump_()
    {
    }

    virtual void reset() override;

    
    
    
    
    void emitInitialJump(MacroAssembler& masm, AddCacheState& addState) override;
    void bindInitialJump(MacroAssembler& masm, AddCacheState& addState) override;

    
    void updateBaseAddress(JitCode* code, MacroAssembler& masm) override;

    virtual void* rejoinAddress() override {
        return rejoinLabel().raw();
    }
};









































































class DispatchIonCache : public IonCache
{
  protected:
    class DispatchStubPrepender;

    uint8_t* firstStub_;
    CodeLocationLabel rejoinLabel_;
    CodeOffsetLabel dispatchLabel_;

  public:
    DispatchIonCache()
      : firstStub_(nullptr),
        rejoinLabel_(),
        dispatchLabel_()
    {
    }

    virtual void reset() override;
    virtual void initializeAddCacheState(LInstruction* ins, AddCacheState* addState) override;

    void emitInitialJump(MacroAssembler& masm, AddCacheState& addState) override;
    void bindInitialJump(MacroAssembler& masm, AddCacheState& addState) override;

    
    void updateBaseAddress(JitCode* code, MacroAssembler& masm) override;

    virtual void* rejoinAddress() override {
        return rejoinLabel_.raw();
    }
};



#define CACHE_HEADER(ickind)                                        \
    Kind kind() const {                                             \
        return IonCache::Cache_##ickind;                            \
    }                                                               \
                                                                    \
    void accept(CodeGenerator* codegen, IonCacheVisitor* visitor) { \
        visitor->visit##ickind##IC(codegen);                        \
    }                                                               \
                                                                    \
    static const VMFunction UpdateInfo;













struct CacheLocation {
    jsbytecode* pc;
    JSScript* script;

    CacheLocation(jsbytecode* pcin, JSScript* scriptin)
        : pc(pcin), script(scriptin)
    { }
};

class GetPropertyIC : public RepatchIonCache
{
  protected:
    
    
    LiveRegisterSet liveRegs_;

    Register object_;
    PropertyName* name_;
    TypedOrValueRegister output_;

    
    size_t locationsIndex_;
    size_t numLocations_;

    bool monitoredResult_ : 1;
    bool hasTypedArrayLengthStub_ : 1;
    bool hasSharedTypedArrayLengthStub_ : 1;
    bool hasStrictArgumentsLengthStub_ : 1;
    bool hasNormalArgumentsLengthStub_ : 1;
    bool hasGenericProxyStub_ : 1;

  public:
    GetPropertyIC(LiveRegisterSet liveRegs,
                  Register object, PropertyName* name,
                  TypedOrValueRegister output,
                  bool monitoredResult)
      : liveRegs_(liveRegs),
        object_(object),
        name_(name),
        output_(output),
        locationsIndex_(0),
        numLocations_(0),
        monitoredResult_(monitoredResult),
        hasTypedArrayLengthStub_(false),
        hasSharedTypedArrayLengthStub_(false),
        hasStrictArgumentsLengthStub_(false),
        hasNormalArgumentsLengthStub_(false),
        hasGenericProxyStub_(false)
    {
    }

    CACHE_HEADER(GetProperty)

    void reset();

    Register object() const {
        return object_;
    }
    PropertyName* name() const {
        return name_;
    }
    TypedOrValueRegister output() const {
        return output_;
    }
    bool monitoredResult() const {
        return monitoredResult_;
    }
    bool hasAnyTypedArrayLengthStub(HandleObject obj) const {
        return obj->is<TypedArrayObject>() ? hasTypedArrayLengthStub_ : hasSharedTypedArrayLengthStub_;
    }
    bool hasArgumentsLengthStub(bool strict) const {
        return strict ? hasStrictArgumentsLengthStub_ : hasNormalArgumentsLengthStub_;
    }
    bool hasGenericProxyStub() const {
        return hasGenericProxyStub_;
    }

    void setHasTypedArrayLengthStub(HandleObject obj) {
        if (obj->is<TypedArrayObject>()) {
            MOZ_ASSERT(!hasTypedArrayLengthStub_);
            hasTypedArrayLengthStub_ = true;
        } else {
            MOZ_ASSERT(!hasSharedTypedArrayLengthStub_);
            hasSharedTypedArrayLengthStub_ = true;
        }
    }

    void setLocationInfo(size_t locationsIndex, size_t numLocations) {
        MOZ_ASSERT(idempotent());
        MOZ_ASSERT(!numLocations_);
        MOZ_ASSERT(numLocations);
        locationsIndex_ = locationsIndex;
        numLocations_ = numLocations;
    }
    void getLocationInfo(uint32_t* index, uint32_t* num) const {
        MOZ_ASSERT(idempotent());
        *index = locationsIndex_;
        *num = numLocations_;
    }

    enum NativeGetPropCacheability {
        CanAttachNone,
        CanAttachReadSlot,
        CanAttachArrayLength,
        CanAttachCallGetter
    };

    
    bool allowArrayLength(JSContext* cx, HandleObject obj) const;
    bool allowGetters() const {
        return monitoredResult() && !idempotent();
    }

    
    bool tryAttachStub(JSContext* cx, HandleScript outerScript, IonScript* ion,
                       HandleObject obj, HandlePropertyName name, bool* emitted);

    bool tryAttachProxy(JSContext* cx, HandleScript outerScript, IonScript* ion,
                        HandleObject obj, HandlePropertyName name,
                        void* returnAddr, bool* emitted);

    bool tryAttachGenericProxy(JSContext* cx, HandleScript outerScript, IonScript* ion,
                               HandleObject obj, HandlePropertyName name,
                               void* returnAddr, bool* emitted);

    bool tryAttachDOMProxyShadowed(JSContext* cx, HandleScript outerScript, IonScript* ion,
                                   HandleObject obj, void* returnAddr, bool* emitted);

    bool tryAttachDOMProxyUnshadowed(JSContext* cx, HandleScript outerScript, IonScript* ion,
                                     HandleObject obj, HandlePropertyName name, bool resetNeeded,
                                     void* returnAddr, bool* emitted);

    bool tryAttachNative(JSContext* cx, HandleScript outerScript, IonScript* ion,
                         HandleObject obj, HandlePropertyName name,
                         void* returnAddr, bool* emitted);

    bool tryAttachUnboxed(JSContext* cx, HandleScript outerScript, IonScript* ion,
                          HandleObject obj, HandlePropertyName name,
                          void* returnAddr, bool* emitted);

    bool tryAttachUnboxedExpando(JSContext* cx, HandleScript outerScript, IonScript* ion,
                                 HandleObject obj, HandlePropertyName name,
                                 void* returnAddr, bool* emitted);

    bool tryAttachTypedArrayLength(JSContext* cx, HandleScript outerScript, IonScript* ion,
                                   HandleObject obj, HandlePropertyName name, bool* emitted);

    bool tryAttachArgumentsLength(JSContext* cx, HandleScript outerScript, IonScript* ion,
                                  HandleObject obj, HandlePropertyName name, bool* emitted);

    static bool update(JSContext* cx, HandleScript outerScript, size_t cacheIndex,
                       HandleObject obj, MutableHandleValue vp);
};

class SetPropertyIC : public RepatchIonCache
{
  protected:
    
    
    LiveRegisterSet liveRegs_;

    Register object_;
    PropertyName* name_;
    ConstantOrRegister value_;
    bool strict_;
    bool needsTypeBarrier_;

    bool hasGenericProxyStub_;

  public:
    SetPropertyIC(LiveRegisterSet liveRegs, Register object, PropertyName* name,
                  ConstantOrRegister value, bool strict, bool needsTypeBarrier)
      : liveRegs_(liveRegs),
        object_(object),
        name_(name),
        value_(value),
        strict_(strict),
        needsTypeBarrier_(needsTypeBarrier),
        hasGenericProxyStub_(false)
    {
    }

    CACHE_HEADER(SetProperty)

    void reset();

    Register object() const {
        return object_;
    }
    PropertyName* name() const {
        return name_;
    }
    ConstantOrRegister value() const {
        return value_;
    }
    bool strict() const {
        return strict_;
    }
    bool needsTypeBarrier() const {
        return needsTypeBarrier_;
    }
    bool hasGenericProxyStub() const {
        return hasGenericProxyStub_;
    }

    enum NativeSetPropCacheability {
        CanAttachNone,
        CanAttachSetSlot,
        MaybeCanAttachAddSlot,
        CanAttachCallSetter
    };

    bool attachSetSlot(JSContext* cx, HandleScript outerScript, IonScript* ion,
                       HandleObject obj, HandleShape shape, bool checkTypeset);

    bool attachCallSetter(JSContext* cx, HandleScript outerScript, IonScript* ion,
                          HandleObject obj, HandleObject holder, HandleShape shape,
                          void* returnAddr);

    bool attachAddSlot(JSContext* cx, HandleScript outerScript, IonScript* ion,
                       HandleObject obj, HandleShape oldShape, HandleObjectGroup oldGroup,
                       bool checkTypeset);

    bool attachSetUnboxed(JSContext* cx, HandleScript outerScript, IonScript* ion,
                          HandleObject obj, HandleId id,
                          uint32_t unboxedOffset, JSValueType unboxedType,
                          bool checkTypeset);

    bool attachGenericProxy(JSContext* cx, HandleScript outerScript, IonScript* ion,
                            void* returnAddr);

    bool attachDOMProxyShadowed(JSContext* cx, HandleScript outerScript, IonScript* ion,
                                HandleObject obj, void* returnAddr);

    bool attachDOMProxyUnshadowed(JSContext* cx, HandleScript outerScript, IonScript* ion,
                                  HandleObject obj, void* returnAddr);

    static bool update(JSContext* cx, HandleScript outerScript, size_t cacheIndex,
                       HandleObject obj, HandleValue value);
};

class GetElementIC : public RepatchIonCache
{
  protected:
    LiveRegisterSet liveRegs_;

    Register object_;
    ConstantOrRegister index_;
    TypedOrValueRegister output_;

    bool monitoredResult_ : 1;
    bool allowDoubleResult_ : 1;
    bool hasDenseStub_ : 1;
    bool hasStrictArgumentsStub_ : 1;
    bool hasNormalArgumentsStub_ : 1;

    size_t failedUpdates_;

    static const size_t MAX_FAILED_UPDATES;

  public:
    GetElementIC(LiveRegisterSet liveRegs, Register object, ConstantOrRegister index,
                 TypedOrValueRegister output, bool monitoredResult, bool allowDoubleResult)
      : liveRegs_(liveRegs),
        object_(object),
        index_(index),
        output_(output),
        monitoredResult_(monitoredResult),
        allowDoubleResult_(allowDoubleResult),
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
    bool allowDoubleResult() const {
        return allowDoubleResult_;
    }
    bool hasDenseStub() const {
        return hasDenseStub_;
    }
    bool hasArgumentsStub(bool strict) const {
        return strict ? hasStrictArgumentsStub_ : hasNormalArgumentsStub_;
    }
    void setHasDenseStub() {
        MOZ_ASSERT(!hasDenseStub());
        hasDenseStub_ = true;
    }

    
    typedef JSContext * Context;
    bool allowGetters() const { MOZ_ASSERT(!idempotent()); return true; }
    bool allowArrayLength(Context, HandleObject) const { return false; }
    bool canMonitorSingletonUndefinedSlot(HandleObject holder, HandleShape shape) const {
        return monitoredResult();
    }

    static bool canAttachGetProp(JSObject* obj, const Value& idval, jsid id);
    static bool canAttachDenseElement(JSObject* obj, const Value& idval);
    static bool canAttachDenseElementHole(JSObject* obj, const Value& idval,
                                          TypedOrValueRegister output);
    static bool canAttachTypedArrayElement(JSObject* obj, const Value& idval,
                                           TypedOrValueRegister output);

    bool attachGetProp(JSContext* cx, HandleScript outerScript, IonScript* ion,
                       HandleObject obj, const Value& idval, HandlePropertyName name);

    bool attachDenseElement(JSContext* cx, HandleScript outerScript, IonScript* ion,
                            HandleObject obj, const Value& idval);

    bool attachDenseElementHole(JSContext* cx, HandleScript outerScript, IonScript* ion,
                                HandleObject obj, const Value& idval);

    bool attachTypedArrayElement(JSContext* cx, HandleScript outerScript, IonScript* ion,
                                 HandleObject tarr, const Value& idval);

    bool attachArgumentsElement(JSContext* cx, HandleScript outerScript, IonScript* ion,
                                HandleObject obj);

    static bool
    update(JSContext* cx, HandleScript outerScript, size_t cacheIndex, HandleObject obj,
           HandleValue idval, MutableHandleValue vp);

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
    Register tempToUnboxIndex_;
    Register temp_;
    FloatRegister tempDouble_;
    FloatRegister tempFloat32_;
    ValueOperand index_;
    ConstantOrRegister value_;
    bool strict_;
    bool guardHoles_;

    bool hasDenseStub_ : 1;

  public:
    SetElementIC(Register object, Register tempToUnboxIndex, Register temp,
                 FloatRegister tempDouble, FloatRegister tempFloat32,
                 ValueOperand index, ConstantOrRegister value,
                 bool strict, bool guardHoles)
      : object_(object),
        tempToUnboxIndex_(tempToUnboxIndex),
        temp_(temp),
        tempDouble_(tempDouble),
        tempFloat32_(tempFloat32),
        index_(index),
        value_(value),
        strict_(strict),
        guardHoles_(guardHoles),
        hasDenseStub_(false)
    {
    }

    CACHE_HEADER(SetElement)

    void reset();

    Register object() const {
        return object_;
    }
    Register tempToUnboxIndex() const {
        return tempToUnboxIndex_;
    }
    Register temp() const {
        return temp_;
    }
    FloatRegister tempDouble() const {
        return tempDouble_;
    }
    FloatRegister tempFloat32() const {
        return tempFloat32_;
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
    bool guardHoles() const {
        return guardHoles_;
    }

    bool hasDenseStub() const {
        return hasDenseStub_;
    }
    void setHasDenseStub() {
        MOZ_ASSERT(!hasDenseStub());
        hasDenseStub_ = true;
    }

    bool attachDenseElement(JSContext* cx, HandleScript outerScript, IonScript* ion,
                            HandleObject obj, const Value& idval);

    bool attachTypedArrayElement(JSContext* cx, HandleScript outerScript, IonScript* ion,
                                 HandleObject tarr);

    static bool
    update(JSContext* cx, HandleScript outerScript, size_t cacheIndex, HandleObject obj,
           HandleValue idval, HandleValue value);
};

class BindNameIC : public RepatchIonCache
{
  protected:
    Register scopeChain_;
    PropertyName* name_;
    Register output_;

  public:
    BindNameIC(Register scopeChain, PropertyName* name, Register output)
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

    bool attachGlobal(JSContext* cx, HandleScript outerScript, IonScript* ion,
                      HandleObject scopeChain);

    bool attachNonGlobal(JSContext* cx, HandleScript outerScript, IonScript* ion,
                         HandleObject scopeChain, HandleObject holder);

    static JSObject*
    update(JSContext* cx, HandleScript outerScript, size_t cacheIndex, HandleObject scopeChain);
};

class NameIC : public RepatchIonCache
{
  protected:
    
    
    LiveRegisterSet liveRegs_;

    bool typeOf_;
    Register scopeChain_;
    PropertyName* name_;
    TypedOrValueRegister output_;

  public:
    NameIC(LiveRegisterSet liveRegs, bool typeOf,
           Register scopeChain, PropertyName* name,
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

    bool attachReadSlot(JSContext* cx, HandleScript outerScript, IonScript* ion,
                        HandleObject scopeChain, HandleObject holderBase,
                        HandleNativeObject holder, HandleShape shape);

    bool attachCallGetter(JSContext* cx, HandleScript outerScript, IonScript* ion,
                          HandleObject scopeChain, HandleObject obj, HandleObject holder,
                          HandleShape shape, void* returnAddr);

    static bool
    update(JSContext* cx, HandleScript outerScript, size_t cacheIndex, HandleObject scopeChain,
           MutableHandleValue vp);
};

#undef CACHE_HEADER


#define CACHE_CASTS(ickind)                                             \
    ickind##IC& IonCache::to##ickind()                                  \
    {                                                                   \
        MOZ_ASSERT(is##ickind());                                       \
        return *static_cast<ickind##IC*>(this);                        \
    }                                                                   \
    const ickind##IC& IonCache::to##ickind() const                      \
    {                                                                   \
        MOZ_ASSERT(is##ickind());                                       \
        return *static_cast<const ickind##IC*>(this);                  \
    }
IONCACHE_KIND_LIST(CACHE_CASTS)
#undef OPCODE_CASTS

} 
} 

#endif 
