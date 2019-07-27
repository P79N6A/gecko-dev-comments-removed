





#ifndef jit_SharedIC_h
#define jit_SharedIC_h

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsgc.h"

#include "jit/BaselineICList.h"
#include "jit/MacroAssembler.h"
#include "jit/SharedICList.h"
#include "jit/SharedICRegisters.h"

namespace js {
namespace jit {










































































































































































class ICStub;
class ICFallbackStub;

#define FORWARD_DECLARE_STUBS(kindName) class IC##kindName;
    IC_BASELINE_STUB_KIND_LIST(FORWARD_DECLARE_STUBS)
    IC_SHARED_STUB_KIND_LIST(FORWARD_DECLARE_STUBS)
#undef FORWARD_DECLARE_STUBS

#ifdef DEBUG
void FallbackICSpew(JSContext* cx, ICFallbackStub* stub, const char* fmt, ...);
void TypeFallbackICSpew(JSContext* cx, ICTypeMonitor_Fallback* stub, const char* fmt, ...);
#else
#define FallbackICSpew(...)
#define TypeFallbackICSpew(...)
#endif




class ICEntry
{
  private:
    
    ICStub* firstStub_;

    
    
    uint32_t returnOffset_;

    
    uint32_t pcOffset_ : 28;

  public:
    enum Kind {
        
        Kind_Op = 0,

        
        Kind_NonOp,

        
        Kind_CallVM,

        
        
        Kind_NonOpCallVM,

        
        
        Kind_StackCheck,

        
        Kind_EarlyStackCheck,

        
        Kind_DebugTrap,

        
        
        Kind_DebugPrologue,
        Kind_DebugEpilogue,

        Kind_Invalid
    };

  private:
    
    Kind kind_ : 4;

    
    void setKind(Kind kind) {
        MOZ_ASSERT(kind < Kind_Invalid);
        kind_ = kind;
        MOZ_ASSERT(this->kind() == kind);
    }

  public:
    ICEntry(uint32_t pcOffset, Kind kind)
      : firstStub_(nullptr), returnOffset_(), pcOffset_(pcOffset)
    {
        
        
        MOZ_ASSERT(pcOffset_ == pcOffset);
        JS_STATIC_ASSERT(BaselineScript::MAX_JSSCRIPT_LENGTH <= (1u << 28) - 1);
        MOZ_ASSERT(pcOffset <= BaselineScript::MAX_JSSCRIPT_LENGTH);
        setKind(kind);
    }

    CodeOffsetLabel returnOffset() const {
        return CodeOffsetLabel(returnOffset_);
    }

    void setReturnOffset(CodeOffsetLabel offset) {
        MOZ_ASSERT(offset.offset() <= (size_t) UINT32_MAX);
        returnOffset_ = (uint32_t) offset.offset();
    }

    void fixupReturnOffset(MacroAssembler& masm) {
        CodeOffsetLabel offset = returnOffset();
        offset.fixup(&masm);
        MOZ_ASSERT(offset.offset() <= UINT32_MAX);
        returnOffset_ = (uint32_t) offset.offset();
    }

    uint32_t pcOffset() const {
        return pcOffset_;
    }

    jsbytecode* pc(JSScript* script) const {
        return script->offsetToPC(pcOffset_);
    }

    Kind kind() const {
        
        return Kind(kind_ & 0xf);
    }
    bool isForOp() const {
        return kind() == Kind_Op;
    }

    void setFakeKind(Kind kind) {
        MOZ_ASSERT(kind != Kind_Op && kind != Kind_NonOp);
        setKind(kind);
    }

    bool hasStub() const {
        return firstStub_ != nullptr;
    }
    ICStub* firstStub() const {
        MOZ_ASSERT(hasStub());
        return firstStub_;
    }

    ICFallbackStub* fallbackStub() const;

    void setFirstStub(ICStub* stub) {
        firstStub_ = stub;
    }

    static inline size_t offsetOfFirstStub() {
        return offsetof(ICEntry, firstStub_);
    }

    inline ICStub** addressOfFirstStub() {
        return &firstStub_;
    }
};

class ICMonitoredStub;
class ICMonitoredFallbackStub;
class ICUpdatedStub;








class ICStubConstIterator
{
    friend class ICStub;
    friend class ICFallbackStub;

  private:
    ICStub* currentStub_;

  public:
    explicit ICStubConstIterator(ICStub* currentStub) : currentStub_(currentStub) {}

    static ICStubConstIterator StartingAt(ICStub* stub) {
        return ICStubConstIterator(stub);
    }
    static ICStubConstIterator End(ICStub* stub) {
        return ICStubConstIterator(nullptr);
    }

    bool operator ==(const ICStubConstIterator& other) const {
        return currentStub_ == other.currentStub_;
    }
    bool operator !=(const ICStubConstIterator& other) const {
        return !(*this == other);
    }

    ICStubConstIterator& operator++();

    ICStubConstIterator operator++(int) {
        ICStubConstIterator oldThis(*this);
        ++(*this);
        return oldThis;
    }

    ICStub* operator*() const {
        MOZ_ASSERT(currentStub_);
        return currentStub_;
    }

    ICStub* operator ->() const {
        MOZ_ASSERT(currentStub_);
        return currentStub_;
    }

    bool atEnd() const {
        return currentStub_ == nullptr;
    }
};












class ICStubIterator
{
    friend class ICFallbackStub;

  private:
    ICEntry* icEntry_;
    ICFallbackStub* fallbackStub_;
    ICStub* previousStub_;
    ICStub* currentStub_;
    bool unlinked_;

    explicit ICStubIterator(ICFallbackStub* fallbackStub, bool end=false);
  public:

    bool operator ==(const ICStubIterator& other) const {
        
        MOZ_ASSERT(icEntry_ == other.icEntry_);
        MOZ_ASSERT(fallbackStub_ == other.fallbackStub_);
        return currentStub_ == other.currentStub_;
    }
    bool operator !=(const ICStubIterator& other) const {
        return !(*this == other);
    }

    ICStubIterator& operator++();

    ICStubIterator operator++(int) {
        ICStubIterator oldThis(*this);
        ++(*this);
        return oldThis;
    }

    ICStub* operator*() const {
        return currentStub_;
    }

    ICStub* operator ->() const {
        return currentStub_;
    }

    bool atEnd() const {
        return currentStub_ == (ICStub*) fallbackStub_;
    }

    void unlink(JSContext* cx);
};




class ICStub
{
    friend class ICFallbackStub;

  public:
    enum Kind {
        INVALID = 0,
#define DEF_ENUM_KIND(kindName) kindName,
        IC_BASELINE_STUB_KIND_LIST(DEF_ENUM_KIND)
        IC_SHARED_STUB_KIND_LIST(DEF_ENUM_KIND)
#undef DEF_ENUM_KIND
        LIMIT
    };

    static inline bool IsValidKind(Kind k) {
        return (k > INVALID) && (k < LIMIT);
    }

    static const char* KindString(Kind k) {
        switch(k) {
#define DEF_KIND_STR(kindName) case kindName: return #kindName;
            IC_BASELINE_STUB_KIND_LIST(DEF_KIND_STR)
            IC_SHARED_STUB_KIND_LIST(DEF_KIND_STR)
#undef DEF_KIND_STR
          default:
            MOZ_CRASH("Invalid kind.");
        }
    }

    enum Trait {
        Regular             = 0x0,
        Fallback            = 0x1,
        Monitored           = 0x2,
        MonitoredFallback   = 0x3,
        Updated             = 0x4
    };

    void markCode(JSTracer* trc, const char* name);
    void updateCode(JitCode* stubCode);
    void trace(JSTracer* trc);

    template <typename T, typename... Args>
    static T* New(JSContext* cx, ICStubSpace* space, JitCode* code, Args&&... args) {
        if (!code)
            return nullptr;
        T* result = space->allocate<T>(code, mozilla::Forward<Args>(args)...);
        if (!result)
            ReportOutOfMemory(cx);
        return result;
    }

  protected:
    
    uint8_t* stubCode_;

    
    
    ICStub* next_;

    
    uint16_t extra_;

    
    
    
    Trait trait_ : 3;
    Kind kind_ : 13;

    inline ICStub(Kind kind, JitCode* stubCode)
      : stubCode_(stubCode->raw()),
        next_(nullptr),
        extra_(0),
        trait_(Regular),
        kind_(kind)
    {
        MOZ_ASSERT(stubCode != nullptr);
    }

    inline ICStub(Kind kind, Trait trait, JitCode* stubCode)
      : stubCode_(stubCode->raw()),
        next_(nullptr),
        extra_(0),
        trait_(trait),
        kind_(kind)
    {
        MOZ_ASSERT(stubCode != nullptr);
    }

    inline Trait trait() const {
        
        return (Trait)(trait_ & 0x7);
    }

  public:

    inline Kind kind() const {
        return static_cast<Kind>(kind_);
    }

    inline bool isFallback() const {
        return trait() == Fallback || trait() == MonitoredFallback;
    }

    inline bool isMonitored() const {
        return trait() == Monitored;
    }

    inline bool isUpdated() const {
        return trait() == Updated;
    }

    inline bool isMonitoredFallback() const {
        return trait() == MonitoredFallback;
    }

    inline const ICFallbackStub* toFallbackStub() const {
        MOZ_ASSERT(isFallback());
        return reinterpret_cast<const ICFallbackStub*>(this);
    }

    inline ICFallbackStub* toFallbackStub() {
        MOZ_ASSERT(isFallback());
        return reinterpret_cast<ICFallbackStub*>(this);
    }

    inline const ICMonitoredStub* toMonitoredStub() const {
        MOZ_ASSERT(isMonitored());
        return reinterpret_cast<const ICMonitoredStub*>(this);
    }

    inline ICMonitoredStub* toMonitoredStub() {
        MOZ_ASSERT(isMonitored());
        return reinterpret_cast<ICMonitoredStub*>(this);
    }

    inline const ICMonitoredFallbackStub* toMonitoredFallbackStub() const {
        MOZ_ASSERT(isMonitoredFallback());
        return reinterpret_cast<const ICMonitoredFallbackStub*>(this);
    }

    inline ICMonitoredFallbackStub* toMonitoredFallbackStub() {
        MOZ_ASSERT(isMonitoredFallback());
        return reinterpret_cast<ICMonitoredFallbackStub*>(this);
    }

    inline const ICUpdatedStub* toUpdatedStub() const {
        MOZ_ASSERT(isUpdated());
        return reinterpret_cast<const ICUpdatedStub*>(this);
    }

    inline ICUpdatedStub* toUpdatedStub() {
        MOZ_ASSERT(isUpdated());
        return reinterpret_cast<ICUpdatedStub*>(this);
    }

#define KIND_METHODS(kindName)   \
    inline bool is##kindName() const { return kind() == kindName; } \
    inline const IC##kindName* to##kindName() const { \
        MOZ_ASSERT(is##kindName()); \
        return reinterpret_cast<const IC##kindName*>(this); \
    } \
    inline IC##kindName* to##kindName() { \
        MOZ_ASSERT(is##kindName()); \
        return reinterpret_cast<IC##kindName*>(this); \
    }
    IC_BASELINE_STUB_KIND_LIST(KIND_METHODS)
    IC_SHARED_STUB_KIND_LIST(KIND_METHODS)
#undef KIND_METHODS

    inline ICStub* next() const {
        return next_;
    }

    inline bool hasNext() const {
        return next_ != nullptr;
    }

    inline void setNext(ICStub* stub) {
        
        
        next_ = stub;
    }

    inline ICStub** addressOfNext() {
        return &next_;
    }

    inline JitCode* jitCode() {
        return JitCode::FromExecutable(stubCode_);
    }

    inline uint8_t* rawStubCode() const {
        return stubCode_;
    }

    
    inline ICFallbackStub* getChainFallback() {
        ICStub* lastStub = this;
        while (lastStub->next_)
            lastStub = lastStub->next_;
        MOZ_ASSERT(lastStub->isFallback());
        return lastStub->toFallbackStub();
    }

    inline ICStubConstIterator beginHere() {
        return ICStubConstIterator::StartingAt(this);
    }

    static inline size_t offsetOfNext() {
        return offsetof(ICStub, next_);
    }

    static inline size_t offsetOfStubCode() {
        return offsetof(ICStub, stubCode_);
    }

    static inline size_t offsetOfExtra() {
        return offsetof(ICStub, extra_);
    }

    static bool CanMakeCalls(ICStub::Kind kind) {
        MOZ_ASSERT(IsValidKind(kind));
        switch (kind) {
          case Call_Fallback:
          case Call_Scripted:
          case Call_AnyScripted:
          case Call_Native:
          case Call_ClassHook:
          case Call_ScriptedApplyArray:
          case Call_ScriptedApplyArguments:
          case Call_ScriptedFunCall:
          case Call_StringSplit:
          case WarmUpCounter_Fallback:
          case GetElem_NativeSlot:
          case GetElem_NativePrototypeSlot:
          case GetElem_NativePrototypeCallNative:
          case GetElem_NativePrototypeCallScripted:
          case GetProp_CallScripted:
          case GetProp_CallNative:
          case GetProp_CallDOMProxyNative:
          case GetProp_CallDOMProxyWithGenerationNative:
          case GetProp_DOMProxyShadowed:
          case GetProp_Generic:
          case SetProp_CallScripted:
          case SetProp_CallNative:
          case RetSub_Fallback:
          
          
          
          case GetProp_Fallback:
          case SetProp_Fallback:
#if JS_HAS_NO_SUCH_METHOD
          case GetElem_Dense:
          case GetElem_Arguments:
          case GetProp_NativePrototype:
          case GetProp_Native:
#endif
            return true;
          default:
            return false;
        }
    }

    
    
    
    
    bool allocatedInFallbackSpace() const {
        MOZ_ASSERT(next());
        return CanMakeCalls(kind());
    }
};

class ICFallbackStub : public ICStub
{
    friend class ICStubConstIterator;
  protected:
    
    

    
    ICEntry* icEntry_;

    
    uint32_t numOptimizedStubs_;

    
    
    
    
    
    ICStub** lastStubPtrAddr_;

    ICFallbackStub(Kind kind, JitCode* stubCode)
      : ICStub(kind, ICStub::Fallback, stubCode),
        icEntry_(nullptr),
        numOptimizedStubs_(0),
        lastStubPtrAddr_(nullptr) {}

    ICFallbackStub(Kind kind, Trait trait, JitCode* stubCode)
      : ICStub(kind, trait, stubCode),
        icEntry_(nullptr),
        numOptimizedStubs_(0),
        lastStubPtrAddr_(nullptr)
    {
        MOZ_ASSERT(trait == ICStub::Fallback ||
                   trait == ICStub::MonitoredFallback);
    }

  public:
    inline ICEntry* icEntry() const {
        return icEntry_;
    }

    inline size_t numOptimizedStubs() const {
        return (size_t) numOptimizedStubs_;
    }

    
    
    
    
    void fixupICEntry(ICEntry* icEntry) {
        MOZ_ASSERT(icEntry_ == nullptr);
        MOZ_ASSERT(lastStubPtrAddr_ == nullptr);
        icEntry_ = icEntry;
        lastStubPtrAddr_ = icEntry_->addressOfFirstStub();
    }

    
    void addNewStub(ICStub* stub) {
        MOZ_ASSERT(*lastStubPtrAddr_ == this);
        MOZ_ASSERT(stub->next() == nullptr);
        stub->setNext(this);
        *lastStubPtrAddr_ = stub;
        lastStubPtrAddr_ = stub->addressOfNext();
        numOptimizedStubs_++;
    }

    ICStubConstIterator beginChainConst() const {
        return ICStubConstIterator(icEntry_->firstStub());
    }

    ICStubIterator beginChain() {
        return ICStubIterator(this);
    }

    bool hasStub(ICStub::Kind kind) const {
        for (ICStubConstIterator iter = beginChainConst(); !iter.atEnd(); iter++) {
            if (iter->kind() == kind)
                return true;
        }
        return false;
    }

    unsigned numStubsWithKind(ICStub::Kind kind) const {
        unsigned count = 0;
        for (ICStubConstIterator iter = beginChainConst(); !iter.atEnd(); iter++) {
            if (iter->kind() == kind)
                count++;
        }
        return count;
    }

    void unlinkStub(Zone* zone, ICStub* prev, ICStub* stub);
    void unlinkStubsWithKind(JSContext* cx, ICStub::Kind kind);
};



class ICMonitoredStub : public ICStub
{
  protected:
    
    ICStub* firstMonitorStub_;

    ICMonitoredStub(Kind kind, JitCode* stubCode, ICStub* firstMonitorStub);

  public:
    inline void updateFirstMonitorStub(ICStub* monitorStub) {
        
        
        MOZ_ASSERT(firstMonitorStub_ && firstMonitorStub_->isTypeMonitor_Fallback());
        firstMonitorStub_ = monitorStub;
    }
    inline void resetFirstMonitorStub(ICStub* monitorFallback) {
        MOZ_ASSERT(monitorFallback->isTypeMonitor_Fallback());
        firstMonitorStub_ = monitorFallback;
    }
    inline ICStub* firstMonitorStub() const {
        return firstMonitorStub_;
    }

    static inline size_t offsetOfFirstMonitorStub() {
        return offsetof(ICMonitoredStub, firstMonitorStub_);
    }
};


class ICMonitoredFallbackStub : public ICFallbackStub
{
  protected:
    
    ICTypeMonitor_Fallback* fallbackMonitorStub_;

    ICMonitoredFallbackStub(Kind kind, JitCode* stubCode)
      : ICFallbackStub(kind, ICStub::MonitoredFallback, stubCode),
        fallbackMonitorStub_(nullptr) {}

  public:
    bool initMonitoringChain(JSContext* cx, ICStubSpace* space);
    bool addMonitorStubForValue(JSContext* cx, JSScript* script, HandleValue val);

    inline ICTypeMonitor_Fallback* fallbackMonitorStub() const {
        return fallbackMonitorStub_;
    }

    static inline size_t offsetOfFallbackMonitorStub() {
        return offsetof(ICMonitoredFallbackStub, fallbackMonitorStub_);
    }
};



class ICUpdatedStub : public ICStub
{
  protected:
    
    ICStub* firstUpdateStub_;

    static const uint32_t MAX_OPTIMIZED_STUBS = 8;
    uint32_t numOptimizedStubs_;

    ICUpdatedStub(Kind kind, JitCode* stubCode)
      : ICStub(kind, ICStub::Updated, stubCode),
        firstUpdateStub_(nullptr),
        numOptimizedStubs_(0)
    {}

  public:
    bool initUpdatingChain(JSContext* cx, ICStubSpace* space);

    bool addUpdateStubForValue(JSContext* cx, HandleScript script, HandleObject obj, HandleId id,
                               HandleValue val);

    void addOptimizedUpdateStub(ICStub* stub) {
        if (firstUpdateStub_->isTypeUpdate_Fallback()) {
            stub->setNext(firstUpdateStub_);
            firstUpdateStub_ = stub;
        } else {
            ICStub* iter = firstUpdateStub_;
            MOZ_ASSERT(iter->next() != nullptr);
            while (!iter->next()->isTypeUpdate_Fallback())
                iter = iter->next();
            MOZ_ASSERT(iter->next()->next() == nullptr);
            stub->setNext(iter->next());
            iter->setNext(stub);
        }

        numOptimizedStubs_++;
    }

    inline ICStub* firstUpdateStub() const {
        return firstUpdateStub_;
    }

    bool hasTypeUpdateStub(ICStub::Kind kind) {
        ICStub* stub = firstUpdateStub_;
        do {
            if (stub->kind() == kind)
                return true;

            stub = stub->next();
        } while (stub);

        return false;
    }

    inline uint32_t numOptimizedStubs() const {
        return numOptimizedStubs_;
    }

    static inline size_t offsetOfFirstUpdateStub() {
        return offsetof(ICUpdatedStub, firstUpdateStub_);
    }
};


class ICStubCompiler
{
    
    js::gc::AutoSuppressGC suppressGC;

  public:
    enum class Engine {
        Baseline = 0,
        IonMonkey
    };

  protected:
    JSContext* cx;
    ICStub::Kind kind;
    Engine engine_;

#ifdef DEBUG
    bool entersStubFrame_;
#endif

    
    virtual int32_t getKey() const {
        return static_cast<int32_t>(engine_) |
              (static_cast<int32_t>(kind) << 1);
    }

    virtual bool generateStubCode(MacroAssembler& masm) = 0;
    virtual bool postGenerateStubCode(MacroAssembler& masm, Handle<JitCode*> genCode) {
        return true;
    }
    JitCode* getStubCode();

    ICStubCompiler(JSContext* cx, ICStub::Kind kind, Engine engine)
      : suppressGC(cx), cx(cx), kind(kind), engine_(engine)
#ifdef DEBUG
      , entersStubFrame_(false)
#endif
    {}

    
    bool tailCallVM(const VMFunction& fun, MacroAssembler& masm);

    
    bool callVM(const VMFunction& fun, MacroAssembler& masm);

    
    
    bool callTypeUpdateIC(MacroAssembler& masm, uint32_t objectOffset);

    
    
    
    void enterStubFrame(MacroAssembler& masm, Register scratch);
    void leaveStubFrame(MacroAssembler& masm, bool calledIntoIon = false);

    
    
    
    void guardProfilingEnabled(MacroAssembler& masm, Register scratch, Label* skip);

    inline AllocatableGeneralRegisterSet availableGeneralRegs(size_t numInputs) const {
        AllocatableGeneralRegisterSet regs(GeneralRegisterSet::All());
        MOZ_ASSERT(!regs.has(BaselineStackReg));
#if defined(JS_CODEGEN_ARM)
        MOZ_ASSERT(!regs.has(ICTailCallReg));
        regs.take(BaselineSecondScratchReg);
#elif defined(JS_CODEGEN_MIPS)
        MOZ_ASSERT(!regs.has(ICTailCallReg));
        MOZ_ASSERT(!regs.has(BaselineSecondScratchReg));
#endif
        regs.take(BaselineFrameReg);
        regs.take(ICStubReg);
#ifdef JS_CODEGEN_X64
        regs.take(ExtractTemp0);
        regs.take(ExtractTemp1);
#endif

        switch (numInputs) {
          case 0:
            break;
          case 1:
            regs.take(R0);
            break;
          case 2:
            regs.take(R0);
            regs.take(R1);
            break;
          default:
            MOZ_CRASH("Invalid numInputs");
        }

        return regs;
    }

    bool emitPostWriteBarrierSlot(MacroAssembler& masm, Register obj, ValueOperand val,
                                  Register scratch, LiveGeneralRegisterSet saveRegs);

    template <typename T, typename... Args>
    T* newStub(Args&&... args) {
        return ICStub::New<T>(cx, mozilla::Forward<Args>(args)...);
    }

  public:
    virtual ICStub* getStub(ICStubSpace* space) = 0;

    static ICStubSpace* StubSpaceForKind(ICStub::Kind kind, JSScript* script) {
        if (ICStub::CanMakeCalls(kind))
            return script->baselineScript()->fallbackStubSpace();
        return script->zone()->jitZone()->optimizedStubSpace();
    }

    ICStubSpace* getStubSpace(JSScript* script) {
        return StubSpaceForKind(kind, script);
    }
};



class ICMultiStubCompiler : public ICStubCompiler
{
  protected:
    JSOp op;

    
    
    virtual int32_t getKey() const {
        return static_cast<int32_t>(engine_) |
              (static_cast<int32_t>(kind) << 1) |
              (static_cast<int32_t>(op) << 17);
    }

    ICMultiStubCompiler(JSContext* cx, ICStub::Kind kind, JSOp op, Engine engine)
      : ICStubCompiler(cx, kind, engine), op(op) {}
};

} 
} 

#endif 
