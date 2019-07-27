





#ifndef jit_IonCode_h
#define jit_IonCode_h

#include "mozilla/Atomics.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"

#include "jsinfer.h"
#include "jstypes.h"

#include "gc/Heap.h"
#include "jit/ExecutableAllocator.h"
#include "jit/IonOptimizationLevels.h"
#include "jit/IonTypes.h"

namespace js {

class AsmJSModule;

namespace jit {

class MacroAssembler;
class CodeOffsetLabel;
class PatchableBackedge;

class JitCode : public gc::BarrieredCell<JitCode>
{
  protected:
    uint8_t *code_;
    ExecutablePool *pool_;
    uint32_t bufferSize_;             
    uint32_t insnSize_;               
    uint32_t dataSize_;               
    uint32_t jumpRelocTableBytes_;    
    uint32_t dataRelocTableBytes_;    
    uint32_t preBarrierTableBytes_;   
    uint8_t headerSize_ : 5;          
    uint8_t kind_ : 3;                
    bool invalidated_ : 1;            
                                      
    bool hasBytecodeMap_ : 1;         
                                      

#if JS_BITS_PER_WORD == 32
    
    uint32_t padding_;
#endif

    JitCode()
      : code_(nullptr),
        pool_(nullptr)
    { }
    JitCode(uint8_t *code, uint32_t bufferSize, uint32_t headerSize, ExecutablePool *pool,
            CodeKind kind)
      : code_(code),
        pool_(pool),
        bufferSize_(bufferSize),
        insnSize_(0),
        dataSize_(0),
        jumpRelocTableBytes_(0),
        dataRelocTableBytes_(0),
        preBarrierTableBytes_(0),
        headerSize_(headerSize),
        kind_(kind),
        invalidated_(false),
        hasBytecodeMap_(false)
    {
        MOZ_ASSERT(CodeKind(kind_) == kind);
        MOZ_ASSERT(headerSize_ == headerSize);
    }

    uint32_t dataOffset() const {
        return insnSize_;
    }
    uint32_t jumpRelocTableOffset() const {
        return dataOffset() + dataSize_;
    }
    uint32_t dataRelocTableOffset() const {
        return jumpRelocTableOffset() + jumpRelocTableBytes_;
    }
    uint32_t preBarrierTableOffset() const {
        return dataRelocTableOffset() + dataRelocTableBytes_;
    }

  public:
    uint8_t *raw() const {
        return code_;
    }
    uint8_t *rawEnd() const {
        return code_ + insnSize_;
    }
    size_t instructionsSize() const {
        return insnSize_;
    }
    void trace(JSTracer *trc);
    void finalize(FreeOp *fop);
    void setInvalidated() {
        invalidated_ = true;
    }

    void setHasBytecodeMap() {
        hasBytecodeMap_ = true;
    }

    void togglePreBarriers(bool enabled);

    
    
    
    bool invalidated() const {
        return !!invalidated_;
    }

    template <typename T> T as() const {
        return JS_DATA_TO_FUNC_PTR(T, raw());
    }

    void copyFrom(MacroAssembler &masm);

    static JitCode *FromExecutable(uint8_t *buffer) {
        JitCode *code = *(JitCode **)(buffer - sizeof(JitCode *));
        JS_ASSERT(code->raw() == buffer);
        return code;
    }

    static size_t offsetOfCode() {
        return offsetof(JitCode, code_);
    }

    uint8_t *jumpRelocTable() {
        return code_ + jumpRelocTableOffset();
    }

    
    
    
    template <AllowGC allowGC>
    static JitCode *New(JSContext *cx, uint8_t *code, uint32_t bufferSize, uint32_t headerSize,
                        ExecutablePool *pool, CodeKind kind);

  public:
    static inline ThingRootKind rootKind() { return THING_ROOT_JIT_CODE; }
};

class SnapshotWriter;
class RecoverWriter;
class SafepointWriter;
class SafepointIndex;
class OsiIndex;
class IonCache;
struct PatchableBackedgeInfo;
struct CacheLocation;



struct DependentAsmJSModuleExit
{
    const AsmJSModule *module;
    size_t exitIndex;

    DependentAsmJSModuleExit(const AsmJSModule *module, size_t exitIndex)
      : module(module),
        exitIndex(exitIndex)
    { }
};


struct IonScript
{
  private:
    
    PreBarrieredJitCode method_;

    
    PreBarrieredJitCode deoptTable_;

    
    jsbytecode *osrPc_;

    
    uint32_t osrEntryOffset_;

    
    uint32_t skipArgCheckEntryOffset_;

    
    
    uint32_t invalidateEpilogueOffset_;

    
    
    
    
    uint32_t invalidateEpilogueDataOffset_;

    
    uint32_t numBailouts_;

    
    
    
    mozilla::Atomic<bool, mozilla::Relaxed> hasUncompiledCallTarget_;

    
    
    
    bool isParallelEntryScript_;

    
    bool hasSPSInstrumentation_;

    
    uint32_t recompiling_;

    
    
    uint32_t runtimeData_;
    uint32_t runtimeSize_;

    
    
    
    uint32_t cacheIndex_;
    uint32_t cacheEntries_;

    
    uint32_t safepointIndexOffset_;
    uint32_t safepointIndexEntries_;

    
    uint32_t safepointsStart_;
    uint32_t safepointsSize_;

    
    uint32_t frameSlots_;

    
    
    uint32_t frameSize_;

    
    uint32_t bailoutTable_;
    uint32_t bailoutEntries_;

    
    uint32_t osiIndexOffset_;
    uint32_t osiIndexEntries_;

    
    uint32_t snapshots_;
    uint32_t snapshotsListSize_;
    uint32_t snapshotsRVATableSize_;

    
    uint32_t recovers_;
    uint32_t recoversSize_;

    
    uint32_t constantTable_;
    uint32_t constantEntries_;

    
    
    
    uint32_t callTargetList_;
    uint32_t callTargetEntries_;

    
    uint32_t backedgeList_;
    uint32_t backedgeEntries_;

    
    uint32_t refcount_;

    
    
    
    
    
    uint32_t parallelAge_;

    
    types::RecompileInfo recompileInfo_;

    
    OptimizationLevel optimizationLevel_;

    
    
    uint32_t osrPcMismatchCounter_;

    
    
    Vector<DependentAsmJSModuleExit> *dependentAsmJSModules;

  private:
    inline uint8_t *bottomBuffer() {
        return reinterpret_cast<uint8_t *>(this);
    }
    inline const uint8_t *bottomBuffer() const {
        return reinterpret_cast<const uint8_t *>(this);
    }

  public:
    SnapshotOffset *bailoutTable() {
        return (SnapshotOffset *) &bottomBuffer()[bailoutTable_];
    }
    PreBarrieredValue *constants() {
        return (PreBarrieredValue *) &bottomBuffer()[constantTable_];
    }
    const SafepointIndex *safepointIndices() const {
        return const_cast<IonScript *>(this)->safepointIndices();
    }
    SafepointIndex *safepointIndices() {
        return (SafepointIndex *) &bottomBuffer()[safepointIndexOffset_];
    }
    const OsiIndex *osiIndices() const {
        return const_cast<IonScript *>(this)->osiIndices();
    }
    OsiIndex *osiIndices() {
        return (OsiIndex *) &bottomBuffer()[osiIndexOffset_];
    }
    uint32_t *cacheIndex() {
        return (uint32_t *) &bottomBuffer()[cacheIndex_];
    }
    uint8_t *runtimeData() {
        return  &bottomBuffer()[runtimeData_];
    }
    JSScript **callTargetList() {
        return (JSScript **) &bottomBuffer()[callTargetList_];
    }
    PatchableBackedge *backedgeList() {
        return (PatchableBackedge *) &bottomBuffer()[backedgeList_];
    }
    bool addDependentAsmJSModule(JSContext *cx, DependentAsmJSModuleExit exit);
    void removeDependentAsmJSModule(DependentAsmJSModuleExit exit) {
        if (!dependentAsmJSModules)
            return;
        for (size_t i = 0; i < dependentAsmJSModules->length(); i++) {
            if (dependentAsmJSModules->begin()[i].module == exit.module &&
                dependentAsmJSModules->begin()[i].exitIndex == exit.exitIndex)
            {
                dependentAsmJSModules->erase(dependentAsmJSModules->begin() + i);
                break;
            }
        }
    }

  private:
    void trace(JSTracer *trc);

  public:
    
    IonScript();

    static IonScript *New(JSContext *cx, types::RecompileInfo recompileInfo,
                          uint32_t frameLocals, uint32_t frameSize,
                          size_t snapshotsListSize, size_t snapshotsRVATableSize,
                          size_t recoversSize, size_t bailoutEntries,
                          size_t constants, size_t safepointIndexEntries,
                          size_t osiIndexEntries, size_t cacheEntries,
                          size_t runtimeSize, size_t safepointsSize,
                          size_t callTargetEntries, size_t backedgeEntries,
                          OptimizationLevel optimizationLevel);
    static void Trace(JSTracer *trc, IonScript *script);
    static void Destroy(FreeOp *fop, IonScript *script);

    static inline size_t offsetOfMethod() {
        return offsetof(IonScript, method_);
    }
    static inline size_t offsetOfOsrEntryOffset() {
        return offsetof(IonScript, osrEntryOffset_);
    }
    static inline size_t offsetOfSkipArgCheckEntryOffset() {
        return offsetof(IonScript, skipArgCheckEntryOffset_);
    }
    static inline size_t offsetOfRefcount() {
        return offsetof(IonScript, refcount_);
    }
    static inline size_t offsetOfRecompiling() {
        return offsetof(IonScript, recompiling_);
    }

  public:
    JitCode *method() const {
        return method_;
    }
    void setMethod(JitCode *code) {
        JS_ASSERT(!invalidated());
        method_ = code;
    }
    void setDeoptTable(JitCode *code) {
        deoptTable_ = code;
    }
    void setOsrPc(jsbytecode *osrPc) {
        osrPc_ = osrPc;
    }
    jsbytecode *osrPc() const {
        return osrPc_;
    }
    void setOsrEntryOffset(uint32_t offset) {
        JS_ASSERT(!osrEntryOffset_);
        osrEntryOffset_ = offset;
    }
    uint32_t osrEntryOffset() const {
        return osrEntryOffset_;
    }
    void setSkipArgCheckEntryOffset(uint32_t offset) {
        JS_ASSERT(!skipArgCheckEntryOffset_);
        skipArgCheckEntryOffset_ = offset;
    }
    uint32_t getSkipArgCheckEntryOffset() const {
        return skipArgCheckEntryOffset_;
    }
    bool containsCodeAddress(uint8_t *addr) const {
        return method()->raw() <= addr && addr <= method()->raw() + method()->instructionsSize();
    }
    bool containsReturnAddress(uint8_t *addr) const {
        
        
        return method()->raw() <= addr && addr <= method()->raw() + method()->instructionsSize();
    }
    void setInvalidationEpilogueOffset(uint32_t offset) {
        JS_ASSERT(!invalidateEpilogueOffset_);
        invalidateEpilogueOffset_ = offset;
    }
    uint32_t invalidateEpilogueOffset() const {
        JS_ASSERT(invalidateEpilogueOffset_);
        return invalidateEpilogueOffset_;
    }
    void setInvalidationEpilogueDataOffset(uint32_t offset) {
        JS_ASSERT(!invalidateEpilogueDataOffset_);
        invalidateEpilogueDataOffset_ = offset;
    }
    uint32_t invalidateEpilogueDataOffset() const {
        JS_ASSERT(invalidateEpilogueDataOffset_);
        return invalidateEpilogueDataOffset_;
    }
    void incNumBailouts() {
        numBailouts_++;
    }
    uint32_t numBailouts() const {
        return numBailouts_;
    }
    bool bailoutExpected() const {
        return numBailouts_ > 0;
    }
    void setHasUncompiledCallTarget() {
        hasUncompiledCallTarget_ = true;
    }
    void clearHasUncompiledCallTarget() {
        hasUncompiledCallTarget_ = false;
    }
    bool hasUncompiledCallTarget() const {
        return hasUncompiledCallTarget_;
    }
    void setIsParallelEntryScript() {
        isParallelEntryScript_ = true;
    }
    void clearIsParallelEntryScript() {
        isParallelEntryScript_ = false;
    }
    bool isParallelEntryScript() const {
        return isParallelEntryScript_;
    }
    void setHasSPSInstrumentation() {
        hasSPSInstrumentation_ = true;
    }
    void clearHasSPSInstrumentation() {
        hasSPSInstrumentation_ = false;
    }
    bool hasSPSInstrumentation() const {
        return hasSPSInstrumentation_;
    }
    const uint8_t *snapshots() const {
        return reinterpret_cast<const uint8_t *>(this) + snapshots_;
    }
    size_t snapshotsListSize() const {
        return snapshotsListSize_;
    }
    size_t snapshotsRVATableSize() const {
        return snapshotsRVATableSize_;
    }
    const uint8_t *recovers() const {
        return reinterpret_cast<const uint8_t *>(this) + recovers_;
    }
    size_t recoversSize() const {
        return recoversSize_;
    }
    const uint8_t *safepoints() const {
        return reinterpret_cast<const uint8_t *>(this) + safepointsStart_;
    }
    size_t safepointsSize() const {
        return safepointsSize_;
    }
    size_t callTargetEntries() const {
        return callTargetEntries_;
    }
    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return mallocSizeOf(this);
    }
    PreBarrieredValue &getConstant(size_t index) {
        JS_ASSERT(index < numConstants());
        return constants()[index];
    }
    size_t numConstants() const {
        return constantEntries_;
    }
    uint32_t frameSlots() const {
        return frameSlots_;
    }
    uint32_t frameSize() const {
        return frameSize_;
    }
    SnapshotOffset bailoutToSnapshot(uint32_t bailoutId) {
        JS_ASSERT(bailoutId < bailoutEntries_);
        return bailoutTable()[bailoutId];
    }
    const SafepointIndex *getSafepointIndex(uint32_t disp) const;
    const SafepointIndex *getSafepointIndex(uint8_t *retAddr) const {
        JS_ASSERT(containsCodeAddress(retAddr));
        return getSafepointIndex(retAddr - method()->raw());
    }
    const OsiIndex *getOsiIndex(uint32_t disp) const;
    const OsiIndex *getOsiIndex(uint8_t *retAddr) const;
    inline IonCache &getCacheFromIndex(uint32_t index) {
        JS_ASSERT(index < cacheEntries_);
        uint32_t offset = cacheIndex()[index];
        return getCache(offset);
    }
    inline IonCache &getCache(uint32_t offset) {
        JS_ASSERT(offset < runtimeSize_);
        return *(IonCache *) &runtimeData()[offset];
    }
    size_t numCaches() const {
        return cacheEntries_;
    }
    size_t runtimeSize() const {
        return runtimeSize_;
    }
    CacheLocation *getCacheLocs(uint32_t locIndex) {
        JS_ASSERT(locIndex < runtimeSize_);
        return (CacheLocation *) &runtimeData()[locIndex];
    }
    void toggleBarriers(bool enabled);
    void purgeCaches();
    void destroyCaches();
    void unlinkFromRuntime(FreeOp *fop);
    void copySnapshots(const SnapshotWriter *writer);
    void copyRecovers(const RecoverWriter *writer);
    void copyBailoutTable(const SnapshotOffset *table);
    void copyConstants(const Value *vp);
    void copySafepointIndices(const SafepointIndex *firstSafepointIndex, MacroAssembler &masm);
    void copyOsiIndices(const OsiIndex *firstOsiIndex, MacroAssembler &masm);
    void copyRuntimeData(const uint8_t *data);
    void copyCacheEntries(const uint32_t *caches, MacroAssembler &masm);
    void copySafepoints(const SafepointWriter *writer);
    void copyCallTargetEntries(JSScript **callTargets);
    void copyPatchableBackedges(JSContext *cx, JitCode *code,
                                PatchableBackedgeInfo *backedges,
                                MacroAssembler &masm);

    bool invalidated() const {
        return refcount_ != 0;
    }
    size_t refcount() const {
        return refcount_;
    }
    void incref() {
        refcount_++;
    }
    void decref(FreeOp *fop) {
        JS_ASSERT(refcount_);
        refcount_--;
        if (!refcount_)
            Destroy(fop, this);
    }
    const types::RecompileInfo& recompileInfo() const {
        return recompileInfo_;
    }
    types::RecompileInfo& recompileInfoRef() {
        return recompileInfo_;
    }
    OptimizationLevel optimizationLevel() const {
        return optimizationLevel_;
    }
    uint32_t incrOsrPcMismatchCounter() {
        return ++osrPcMismatchCounter_;
    }
    void resetOsrPcMismatchCounter() {
        osrPcMismatchCounter_ = 0;
    }

    void setRecompiling() {
        recompiling_ = true;
    }

    bool isRecompiling() const {
        return recompiling_;
    }

    void clearRecompiling() {
        recompiling_ = false;
    }

    static const uint32_t MAX_PARALLEL_AGE = 5;

    enum ShouldIncreaseAge {
        IncreaseAge = true,
        KeepAge = false
    };

    void resetParallelAge() {
        MOZ_ASSERT(isParallelEntryScript());
        parallelAge_ = 0;
    }
    uint32_t parallelAge() const {
        return parallelAge_;
    }
    uint32_t shouldPreserveParallelCode(ShouldIncreaseAge increaseAge = KeepAge) {
        MOZ_ASSERT(isParallelEntryScript());
        return (increaseAge ? ++parallelAge_ : parallelAge_) < MAX_PARALLEL_AGE;
    }

    static void writeBarrierPre(Zone *zone, IonScript *ionScript);
};



struct IonBlockCounts
{
  private:
    uint32_t id_;

    
    
    uint32_t offset_;

    
    char *description_;

    
    uint32_t numSuccessors_;
    uint32_t *successors_;

    
    uint64_t hitCount_;

    
    char *code_;

  public:

    bool init(uint32_t id, uint32_t offset, char *description, uint32_t numSuccessors) {
        id_ = id;
        offset_ = offset;
        description_ = description;
        numSuccessors_ = numSuccessors;
        if (numSuccessors) {
            successors_ = js_pod_calloc<uint32_t>(numSuccessors);
            if (!successors_)
                return false;
        }
        return true;
    }

    void destroy() {
        js_free(description_);
        js_free(successors_);
        js_free(code_);
    }

    uint32_t id() const {
        return id_;
    }

    uint32_t offset() const {
        return offset_;
    }

    const char *description() const {
        return description_;
    }

    size_t numSuccessors() const {
        return numSuccessors_;
    }

    void setSuccessor(size_t i, uint32_t id) {
        JS_ASSERT(i < numSuccessors_);
        successors_[i] = id;
    }

    uint32_t successor(size_t i) const {
        JS_ASSERT(i < numSuccessors_);
        return successors_[i];
    }

    uint64_t *addressOfHitCount() {
        return &hitCount_;
    }

    uint64_t hitCount() const {
        return hitCount_;
    }

    void setCode(const char *code) {
        char *ncode = (char *) js_malloc(strlen(code) + 1);
        if (ncode) {
            strcpy(ncode, code);
            code_ = ncode;
        }
    }

    const char *code() const {
        return code_;
    }
};



struct IonScriptCounts
{
  private:
    
    IonScriptCounts *previous_;

    
    size_t numBlocks_;
    IonBlockCounts *blocks_;

  public:

    IonScriptCounts() {
        mozilla::PodZero(this);
    }

    ~IonScriptCounts() {
        for (size_t i = 0; i < numBlocks_; i++)
            blocks_[i].destroy();
        js_free(blocks_);
        js_delete(previous_);
    }

    bool init(size_t numBlocks) {
        numBlocks_ = numBlocks;
        blocks_ = js_pod_calloc<IonBlockCounts>(numBlocks);
        return blocks_ != nullptr;
    }

    size_t numBlocks() const {
        return numBlocks_;
    }

    IonBlockCounts &block(size_t i) {
        JS_ASSERT(i < numBlocks_);
        return blocks_[i];
    }

    void setPrevious(IonScriptCounts *previous) {
        previous_ = previous;
    }

    IonScriptCounts *previous() const {
        return previous_;
    }
};

struct VMFunction;

class JitCompartment;
class JitRuntime;

struct AutoFlushICache
{
  private:
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
    uintptr_t start_;
    uintptr_t stop_;
    const char *name_;
    bool inhibit_;
    AutoFlushICache *prev_;
#endif

  public:
    static void setRange(uintptr_t p, size_t len);
    static void flush(uintptr_t p, size_t len);
    static void setInhibit();
    ~AutoFlushICache();
    explicit AutoFlushICache(const char *nonce, bool inhibit=false);
};

} 

namespace gc {

inline bool
IsMarked(const jit::VMFunction *)
{
    
    
    return true;
}

} 

} 

#endif 
