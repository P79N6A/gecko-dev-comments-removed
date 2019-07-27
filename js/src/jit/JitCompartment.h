





#ifndef jit_JitCompartment_h
#define jit_JitCompartment_h

#include "mozilla/Array.h"
#include "mozilla/MemoryReporting.h"

#include "jsweakcache.h"

#include "builtin/TypedObject.h"
#include "jit/CompileInfo.h"
#include "jit/IonCode.h"
#include "jit/JitFrames.h"
#include "jit/shared/Assembler-shared.h"
#include "js/Value.h"
#include "vm/Stack.h"

namespace js {
namespace jit {

class FrameSizeClass;

enum EnterJitType {
    EnterJitBaseline = 0,
    EnterJitOptimized = 1
};

struct EnterJitData
{
    explicit EnterJitData(JSContext* cx)
      : scopeChain(cx),
        result(cx)
    {}

    uint8_t* jitcode;
    InterpreterFrame* osrFrame;

    void* calleeToken;

    Value* maxArgv;
    unsigned maxArgc;
    unsigned numActualArgs;
    unsigned osrNumStackValues;

    RootedObject scopeChain;
    RootedValue result;

    bool constructing;
};

typedef void (*EnterJitCode)(void* code, unsigned argc, Value* argv, InterpreterFrame* fp,
                             CalleeToken calleeToken, JSObject* scopeChain,
                             size_t numStackValues, Value* vp);

class JitcodeGlobalTable;








class ICStubSpace
{
  protected:
    LifoAlloc allocator_;

    explicit ICStubSpace(size_t chunkSize)
      : allocator_(chunkSize)
    {}

  public:
    inline void* alloc(size_t size) {
        return allocator_.alloc(size);
    }

    JS_DECLARE_NEW_METHODS(allocate, alloc, inline)

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return allocator_.sizeOfExcludingThis(mallocSizeOf);
    }
};



struct OptimizedICStubSpace : public ICStubSpace
{
    static const size_t STUB_DEFAULT_CHUNK_SIZE = 4 * 1024;

  public:
    OptimizedICStubSpace()
      : ICStubSpace(STUB_DEFAULT_CHUNK_SIZE)
    {}

    void free() {
        allocator_.freeAll();
    }
};



struct FallbackICStubSpace : public ICStubSpace
{
    static const size_t STUB_DEFAULT_CHUNK_SIZE = 256;

  public:
    FallbackICStubSpace()
      : ICStubSpace(STUB_DEFAULT_CHUNK_SIZE)
    {}

    inline void adoptFrom(FallbackICStubSpace* other) {
        allocator_.steal(&(other->allocator_));
    }
};




class PatchableBackedge : public InlineListNode<PatchableBackedge>
{
    friend class JitRuntime;

    CodeLocationJump backedge;
    CodeLocationLabel loopHeader;
    CodeLocationLabel interruptCheck;

  public:
    PatchableBackedge(CodeLocationJump backedge,
                      CodeLocationLabel loopHeader,
                      CodeLocationLabel interruptCheck)
      : backedge(backedge), loopHeader(loopHeader), interruptCheck(interruptCheck)
    {}
};

class JitRuntime
{
    friend class JitCompartment;

    
    ExecutableAllocator execAlloc_;

    
    JitCode* exceptionTail_;

    
    JitCode* bailoutTail_;

    
    JitCode* profilerExitFrameTail_;

    
    JitCode* enterJIT_;

    
    JitCode* enterBaselineJIT_;

    
    Vector<JitCode*, 4, SystemAllocPolicy> bailoutTables_;

    
    JitCode* bailoutHandler_;

    
    
    JitCode* argumentsRectifier_;
    void* argumentsRectifierReturnAddr_;

    
    JitCode* invalidator_;

    
    JitCode* valuePreBarrier_;
    JitCode* stringPreBarrier_;
    JitCode* objectPreBarrier_;
    JitCode* shapePreBarrier_;
    JitCode* objectGroupPreBarrier_;

    
    JitCode* mallocStub_;
    JitCode* freeStub_;

    
    JitCode* lazyLinkStub_;

    
    JitCode* debugTrapHandler_;

    
    JitCode* baselineDebugModeOSRHandler_;
    void* baselineDebugModeOSRHandlerNoFrameRegPopAddr_;

    
    typedef WeakCache<const VMFunction*, JitCode*> VMWrapperMap;
    VMWrapperMap* functionWrappers_;

    
    
    
    uint8_t* osrTempData_;

    
    
    
    
    volatile bool mutatingBackedgeList_;
    InlineList<PatchableBackedge> backedgeList_;

    
    
    
    
    
    
    
    
    
    
    
    
    js::Value ionReturnOverride_;

    
    JitcodeGlobalTable* jitcodeGlobalTable_;

    bool hasIonNurseryObjects_;

  private:
    JitCode* generateLazyLinkStub(JSContext* cx);
    JitCode* generateProfilerExitFrameTailStub(JSContext* cx);
    JitCode* generateExceptionTailStub(JSContext* cx, void* handler);
    JitCode* generateBailoutTailStub(JSContext* cx);
    JitCode* generateEnterJIT(JSContext* cx, EnterJitType type);
    JitCode* generateArgumentsRectifier(JSContext* cx, void** returnAddrOut);
    JitCode* generateBailoutTable(JSContext* cx, uint32_t frameClass);
    JitCode* generateBailoutHandler(JSContext* cx);
    JitCode* generateInvalidator(JSContext* cx);
    JitCode* generatePreBarrier(JSContext* cx, MIRType type);
    JitCode* generateMallocStub(JSContext* cx);
    JitCode* generateFreeStub(JSContext* cx);
    JitCode* generateDebugTrapHandler(JSContext* cx);
    JitCode* generateBaselineDebugModeOSRHandler(JSContext* cx, uint32_t* noFrameRegPopOffsetOut);
    JitCode* generateVMWrapper(JSContext* cx, const VMFunction& f);

  public:
    JitRuntime();
    ~JitRuntime();
    bool initialize(JSContext* cx);

    uint8_t* allocateOsrTempData(size_t size);
    void freeOsrTempData();

    static void Mark(JSTracer* trc);
    static bool MarkJitcodeGlobalTableIteratively(JSTracer* trc);
    static void SweepJitcodeGlobalTable(JSRuntime* rt);

    ExecutableAllocator& execAlloc() {
        return execAlloc_;
    }

    class AutoMutateBackedges
    {
        JitRuntime* jrt_;
      public:
        explicit AutoMutateBackedges(JitRuntime* jrt) : jrt_(jrt) {
            MOZ_ASSERT(!jrt->mutatingBackedgeList_);
            jrt->mutatingBackedgeList_ = true;
        }
        ~AutoMutateBackedges() {
            MOZ_ASSERT(jrt_->mutatingBackedgeList_);
            jrt_->mutatingBackedgeList_ = false;
        }
    };

    bool mutatingBackedgeList() const {
        return mutatingBackedgeList_;
    }
    void addPatchableBackedge(PatchableBackedge* backedge) {
        MOZ_ASSERT(mutatingBackedgeList_);
        backedgeList_.pushFront(backedge);
    }
    void removePatchableBackedge(PatchableBackedge* backedge) {
        MOZ_ASSERT(mutatingBackedgeList_);
        backedgeList_.remove(backedge);
    }

    enum BackedgeTarget {
        BackedgeLoopHeader,
        BackedgeInterruptCheck
    };

    void patchIonBackedges(JSRuntime* rt, BackedgeTarget target);

    JitCode* getVMWrapper(const VMFunction& f) const;
    JitCode* debugTrapHandler(JSContext* cx);
    JitCode* getBaselineDebugModeOSRHandler(JSContext* cx);
    void* getBaselineDebugModeOSRHandlerAddress(JSContext* cx, bool popFrameReg);

    JitCode* getGenericBailoutHandler() const {
        return bailoutHandler_;
    }

    JitCode* getExceptionTail() const {
        return exceptionTail_;
    }

    JitCode* getBailoutTail() const {
        return bailoutTail_;
    }

    JitCode* getProfilerExitFrameTail() const {
        return profilerExitFrameTail_;
    }

    JitCode* getBailoutTable(const FrameSizeClass& frameClass) const;

    JitCode* getArgumentsRectifier() const {
        return argumentsRectifier_;
    }

    void* getArgumentsRectifierReturnAddr() const {
        return argumentsRectifierReturnAddr_;
    }

    JitCode* getInvalidationThunk() const {
        return invalidator_;
    }

    EnterJitCode enterIon() const {
        return enterJIT_->as<EnterJitCode>();
    }

    EnterJitCode enterBaseline() const {
        return enterBaselineJIT_->as<EnterJitCode>();
    }

    JitCode* preBarrier(MIRType type) const {
        switch (type) {
          case MIRType_Value: return valuePreBarrier_;
          case MIRType_String: return stringPreBarrier_;
          case MIRType_Object: return objectPreBarrier_;
          case MIRType_Shape: return shapePreBarrier_;
          case MIRType_ObjectGroup: return objectGroupPreBarrier_;
          default: MOZ_CRASH();
        }
    }

    JitCode* mallocStub() const {
        return mallocStub_;
    }

    JitCode* freeStub() const {
        return freeStub_;
    }

    JitCode* lazyLinkStub() const {
        return lazyLinkStub_;
    }

    bool hasIonReturnOverride() const {
        return !ionReturnOverride_.isMagic(JS_ARG_POISON);
    }
    js::Value takeIonReturnOverride() {
        js::Value v = ionReturnOverride_;
        ionReturnOverride_ = js::MagicValue(JS_ARG_POISON);
        return v;
    }
    void setIonReturnOverride(const js::Value& v) {
        MOZ_ASSERT(!hasIonReturnOverride());
        MOZ_ASSERT(!v.isMagic());
        ionReturnOverride_ = v;
    }

    bool hasIonNurseryObjects() const {
        return hasIonNurseryObjects_;
    }
    void setHasIonNurseryObjects(bool b)  {
        hasIonNurseryObjects_ = b;
    }

    bool hasJitcodeGlobalTable() const {
        return jitcodeGlobalTable_ != nullptr;
    }

    JitcodeGlobalTable* getJitcodeGlobalTable() {
        MOZ_ASSERT(hasJitcodeGlobalTable());
        return jitcodeGlobalTable_;
    }

    bool isProfilerInstrumentationEnabled(JSRuntime* rt) {
        return rt->spsProfiler.enabled();
    }

    bool isOptimizationTrackingEnabled(JSRuntime* rt) {
        return isProfilerInstrumentationEnabled(rt);
    }
};

class JitZone
{
    
    OptimizedICStubSpace optimizedStubSpace_;

  public:
    OptimizedICStubSpace* optimizedStubSpace() {
        return &optimizedStubSpace_;
    }
};

class JitCompartment
{
    friend class JitActivation;

    
    typedef WeakValueCache<uint32_t, ReadBarrieredJitCode> ICStubCodeMap;
    ICStubCodeMap* stubCodes_;

    
    
    void* baselineCallReturnAddr_;
    void* baselineGetPropReturnAddr_;
    void* baselineSetPropReturnAddr_;

    
    
    
    
    
    
    JitCode* stringConcatStub_;
    JitCode* regExpExecStub_;
    JitCode* regExpTestStub_;

    mozilla::Array<ReadBarrieredObject, SimdTypeDescr::LAST_TYPE + 1> simdTemplateObjects_;

    JitCode* generateStringConcatStub(JSContext* cx);
    JitCode* generateRegExpExecStub(JSContext* cx);
    JitCode* generateRegExpTestStub(JSContext* cx);

  public:
    JSObject* getSimdTemplateObjectFor(JSContext* cx, Handle<SimdTypeDescr*> descr) {
        ReadBarrieredObject& tpl = simdTemplateObjects_[descr->type()];
        if (!tpl)
            tpl.set(TypedObject::createZeroed(cx, descr, 0, gc::TenuredHeap));
        return tpl.get();
    }

    JSObject* maybeGetSimdTemplateObjectFor(SimdTypeDescr::Type type) const {
        const ReadBarrieredObject& tpl = simdTemplateObjects_[type];

        
        
        
        return tpl.unbarrieredGet();
    }

    
    
    void registerSimdTemplateObjectFor(SimdTypeDescr::Type type) {
        ReadBarrieredObject& tpl = simdTemplateObjects_[type];
        MOZ_ASSERT(tpl.unbarrieredGet());
        tpl.get();
    }

    JitCode* getStubCode(uint32_t key) {
        ICStubCodeMap::AddPtr p = stubCodes_->lookupForAdd(key);
        if (p)
            return p->value();
        return nullptr;
    }
    bool putStubCode(uint32_t key, Handle<JitCode*> stubCode) {
        
        
        
        MOZ_ASSERT(!stubCodes_->has(key));
        ICStubCodeMap::AddPtr p = stubCodes_->lookupForAdd(key);
        return stubCodes_->add(p, key, stubCode.get());
    }
    void initBaselineCallReturnAddr(void* addr) {
        MOZ_ASSERT(baselineCallReturnAddr_ == nullptr);
        baselineCallReturnAddr_ = addr;
    }
    void* baselineCallReturnAddr() {
        MOZ_ASSERT(baselineCallReturnAddr_ != nullptr);
        return baselineCallReturnAddr_;
    }
    void initBaselineGetPropReturnAddr(void* addr) {
        MOZ_ASSERT(baselineGetPropReturnAddr_ == nullptr);
        baselineGetPropReturnAddr_ = addr;
    }
    void* baselineGetPropReturnAddr() {
        MOZ_ASSERT(baselineGetPropReturnAddr_ != nullptr);
        return baselineGetPropReturnAddr_;
    }
    void initBaselineSetPropReturnAddr(void* addr) {
        MOZ_ASSERT(baselineSetPropReturnAddr_ == nullptr);
        baselineSetPropReturnAddr_ = addr;
    }
    void* baselineSetPropReturnAddr() {
        MOZ_ASSERT(baselineSetPropReturnAddr_ != nullptr);
        return baselineSetPropReturnAddr_;
    }

    void toggleBarriers(bool enabled);

  public:
    JitCompartment();
    ~JitCompartment();

    bool initialize(JSContext* cx);

    
    bool ensureIonStubsExist(JSContext* cx);

    void mark(JSTracer* trc, JSCompartment* compartment);
    void sweep(FreeOp* fop, JSCompartment* compartment);

    JitCode* stringConcatStubNoBarrier() const {
        return stringConcatStub_;
    }

    JitCode* regExpExecStubNoBarrier() const {
        return regExpExecStub_;
    }

    bool ensureRegExpExecStubExists(JSContext* cx) {
        if (regExpExecStub_)
            return true;
        regExpExecStub_ = generateRegExpExecStub(cx);
        return regExpExecStub_ != nullptr;
    }

    JitCode* regExpTestStubNoBarrier() const {
        return regExpTestStub_;
    }

    bool ensureRegExpTestStubExists(JSContext* cx) {
        if (regExpTestStub_)
            return true;
        regExpTestStub_ = generateRegExpTestStub(cx);
        return regExpTestStub_ != nullptr;
    }
};


void InvalidateAll(FreeOp* fop, JS::Zone* zone);
void FinishInvalidation(FreeOp* fop, JSScript* script);



#ifdef XP_WIN
const unsigned WINDOWS_BIG_FRAME_TOUCH_INCREMENT = 4096 - 1;
#endif

} 
} 

#endif 
