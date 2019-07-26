






#ifndef jsion_coderef_h__
#define jsion_coderef_h__

#include "IonTypes.h"
#include "gc/Heap.h"


#include "jsinfer.h"

namespace JSC {
    class ExecutablePool;
}

class JSScript;

namespace js {
namespace ion {




static const uint32_t MAX_BUFFER_SIZE = (1 << 30) - 1;


static const uint32_t SNAPSHOT_MAX_NARGS = 127;
static const uint32_t SNAPSHOT_MAX_STACK = 127;

class MacroAssembler;
class CodeOffsetLabel;

class IonCode : public gc::Cell
{
  protected:
    uint8_t *code_;
    JSC::ExecutablePool *pool_;
    uint32_t bufferSize_;             
    uint32_t insnSize_;               
    uint32_t dataSize_;               
    uint32_t jumpRelocTableBytes_;    
    uint32_t dataRelocTableBytes_;    
    uint32_t preBarrierTableBytes_;   
    JSBool invalidated_;            
                                    

#if JS_BITS_PER_WORD == 32
    
    uint32_t padding_;
#endif

    IonCode()
      : code_(NULL),
        pool_(NULL)
    { }
    IonCode(uint8_t *code, uint32_t bufferSize, JSC::ExecutablePool *pool)
      : code_(code),
        pool_(pool),
        bufferSize_(bufferSize),
        insnSize_(0),
        dataSize_(0),
        jumpRelocTableBytes_(0),
        dataRelocTableBytes_(0),
        preBarrierTableBytes_(0),
        invalidated_(false)
    { }

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
    size_t instructionsSize() const {
        return insnSize_;
    }
    void trace(JSTracer *trc);
    void finalize(FreeOp *fop);
    void setInvalidated() {
        invalidated_ = true;
    }

    void togglePreBarriers(bool enabled);

    
    
    
    bool invalidated() const {
        return !!invalidated_;
    }

    template <typename T> T as() const {
        return JS_DATA_TO_FUNC_PTR(T, raw());
    }

    void copyFrom(MacroAssembler &masm);

    static IonCode *FromExecutable(uint8_t *buffer) {
        IonCode *code = *(IonCode **)(buffer - sizeof(IonCode *));
        JS_ASSERT(code->raw() == buffer);
        return code;
    }

    static size_t offsetOfCode() {
        return offsetof(IonCode, code_);
    }

    uint8_t *jumpRelocTable() {
        return code_ + jumpRelocTableOffset();
    }

    
    
    
    static IonCode *New(JSContext *cx, uint8_t *code, uint32_t bufferSize, JSC::ExecutablePool *pool);

  public:
    static void readBarrier(IonCode *code);
    static void writeBarrierPre(IonCode *code);
    static void writeBarrierPost(IonCode *code, void *addr);
    static inline ThingRootKind rootKind() { return THING_ROOT_ION_CODE; }
};

class SnapshotWriter;
class SafepointWriter;
class SafepointIndex;
class OsiIndex;
class IonCache;


struct IonScript
{
  private:
    
    HeapPtr<IonCode> method_;

    
    HeapPtr<IonCode> deoptTable_;

    
    jsbytecode *osrPc_;

    
    uint32_t osrEntryOffset_;

    
    
    uint32_t invalidateEpilogueOffset_;

    
    
    
    
    uint32_t invalidateEpilogueDataOffset_;

    
    bool bailoutExpected_;

    
    
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
    uint32_t snapshotsSize_;

    
    uint32_t constantTable_;
    uint32_t constantEntries_;

    
    uint32_t scriptList_;
    uint32_t scriptEntries_;

    
    
    
    
    
    
    
    
    
    uint32_t parallelInvalidatedScriptList_;
    uint32_t parallelInvalidatedScriptEntries_;

    
    size_t refcount_;

    
    types::RecompileInfo recompileInfo_;

  private:
    inline uint8_t *bottomBuffer() {
        return reinterpret_cast<uint8_t *>(this);
    }
    inline const uint8_t *bottomBuffer() const {
        return reinterpret_cast<const uint8_t *>(this);
    }

  public:
    
    uint32_t slowCallCount;

    SnapshotOffset *bailoutTable() {
        return (SnapshotOffset *) &bottomBuffer()[bailoutTable_];
    }
    HeapValue *constants() {
        return (HeapValue *) &bottomBuffer()[constantTable_];
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
    JSScript **scriptList() const {
        return (JSScript **) &bottomBuffer()[scriptList_];
    }
    JSScript **parallelInvalidatedScriptList() {
        return (JSScript **) &bottomBuffer()[parallelInvalidatedScriptList_];
    }

  private:
    void trace(JSTracer *trc);

  public:
    
    IonScript();

    static IonScript *New(JSContext *cx, uint32_t frameLocals, uint32_t frameSize,
                          size_t snapshotsSize, size_t snapshotEntries,
                          size_t constants, size_t safepointIndexEntries, size_t osiIndexEntries,
                          size_t cacheEntries, size_t runtimeSize,
                          size_t safepointsSize, size_t scriptEntries,
                          size_t parallelInvalidatedScriptEntries);
    static void Trace(JSTracer *trc, IonScript *script);
    static void Destroy(FreeOp *fop, IonScript *script);

    static inline size_t offsetOfMethod() {
        return offsetof(IonScript, method_);
    }
    static inline size_t offsetOfOsrEntryOffset() {
        return offsetof(IonScript, osrEntryOffset_);
    }

  public:
    IonCode *method() const {
        return method_;
    }
    void setMethod(IonCode *code) {
        JS_ASSERT(!invalidated());
        method_ = code;
    }
    void setDeoptTable(IonCode *code) {
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
    void setBailoutExpected() {
        bailoutExpected_ = true;
    }
    bool bailoutExpected() const {
        return bailoutExpected_;
    }
    const uint8_t *snapshots() const {
        return reinterpret_cast<const uint8_t *>(this) + snapshots_;
    }
    size_t snapshotsSize() const {
        return snapshotsSize_;
    }
    const uint8_t *safepoints() const {
        return reinterpret_cast<const uint8_t *>(this) + safepointsStart_;
    }
    size_t safepointsSize() const {
        return safepointsSize_;
    }
    RawScript getScript(size_t i) const {
        JS_ASSERT(i < scriptEntries_);
        return scriptList()[i];
    }
    size_t scriptEntries() const {
        return scriptEntries_;
    }
    size_t parallelInvalidatedScriptEntries() const {
        return parallelInvalidatedScriptEntries_;
    }
    RawScript getAndZeroParallelInvalidatedScript(uint32_t i) {
        JS_ASSERT(i < parallelInvalidatedScriptEntries_);
        RawScript script = parallelInvalidatedScriptList()[i];
        parallelInvalidatedScriptList()[i] = NULL;
        return script;
    }
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return mallocSizeOf(this);
    }
    HeapValue &getConstant(size_t index) {
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
    inline IonCache &getCache(uint32_t index) {
        JS_ASSERT(index < cacheEntries_);
        uint32_t offset = cacheIndex()[index];
        JS_ASSERT(offset < runtimeSize_);
        return *(IonCache *) &runtimeData()[offset];
    }
    size_t numCaches() const {
        return cacheEntries_;
    }
    size_t runtimeSize() const {
        return runtimeSize_;
    }
    void toggleBarriers(bool enabled);
    void purgeCaches(JSCompartment *c);
    void copySnapshots(const SnapshotWriter *writer);
    void copyBailoutTable(const SnapshotOffset *table);
    void copyConstants(const HeapValue *vp);
    void copySafepointIndices(const SafepointIndex *firstSafepointIndex, MacroAssembler &masm);
    void copyOsiIndices(const OsiIndex *firstOsiIndex, MacroAssembler &masm);
    void copyRuntimeData(const uint8_t *data);
    void copyCacheEntries(const uint32_t *caches, MacroAssembler &masm);
    void copySafepoints(const SafepointWriter *writer);
    void copyScriptEntries(JSScript **scripts);
    void zeroParallelInvalidatedScripts();

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
};



struct IonBlockCounts
{
  private:
    uint32_t id_;

    
    
    uint32_t offset_;

    
    uint32_t numSuccessors_;
    uint32_t *successors_;

    
    uint64_t hitCount_;

    
    char *code_;

    
    
    uint32_t instructionBytes_;
    uint32_t spillBytes_;

  public:

    bool init(uint32_t id, uint32_t offset, uint32_t numSuccessors) {
        id_ = id;
        offset_ = offset;
        numSuccessors_ = numSuccessors;
        if (numSuccessors) {
            successors_ = (uint32_t *) js_calloc(numSuccessors * sizeof(uint32_t));
            if (!successors_)
                return false;
        }
        return true;
    }

    void destroy() {
        if (successors_)
            js_free(successors_);
        if (code_)
            js_free(code_);
    }

    uint32_t id() const {
        return id_;
    }

    uint32_t offset() const {
        return offset_;
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

    void setInstructionBytes(uint32_t bytes) {
        instructionBytes_ = bytes;
    }

    uint32_t instructionBytes() const {
        return instructionBytes_;
    }

    void setSpillBytes(uint32_t bytes) {
        spillBytes_ = bytes;
    }

    uint32_t spillBytes() const {
        return spillBytes_;
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
        PodZero(this);
    }

    ~IonScriptCounts() {
        for (size_t i = 0; i < numBlocks_; i++)
            blocks_[i].destroy();
        js_free(blocks_);
        if (previous_)
            js_delete(previous_);
    }

    bool init(size_t numBlocks) {
        numBlocks_ = numBlocks;
        blocks_ = (IonBlockCounts *) js_calloc(numBlocks * sizeof(IonBlockCounts));
        return blocks_ != NULL;
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

class IonCompartment;

struct AutoFlushCache {

  private:
    uintptr_t start_;
    uintptr_t stop_;
    const char *name_;
    IonCompartment *myCompartment_;
    bool used_;

  public:
    void update(uintptr_t p, size_t len);
    static void updateTop(uintptr_t p, size_t len);
    ~AutoFlushCache();
    AutoFlushCache(const char * nonce, IonCompartment *comp = NULL);
    void flushAnyway();
};









struct AutoFlushInhibitor {
  private:
    IonCompartment *ic_;
    AutoFlushCache *afc;
  public:
    AutoFlushInhibitor(IonCompartment *ic);
    ~AutoFlushInhibitor();
};
} 

namespace gc {

inline bool
IsMarked(const ion::VMFunction *)
{
    
    
    return true;
}

} 

} 

#endif 

