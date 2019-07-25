








































#ifndef jsion_coderef_h__
#define jsion_coderef_h__

#include "jscell.h"
#include "IonTypes.h"

namespace JSC {
    class ExecutablePool;
}

struct JSScript;

namespace js {
namespace ion {




static const uint32 MAX_BUFFER_SIZE = (1 << 30) - 1;

class MacroAssembler;

class IonCode : public gc::Cell
{
  protected:
    uint8 *code_;
    JSC::ExecutablePool *pool_;
    uint32 bufferSize_;             
    uint32 insnSize_;               
    uint32 dataSize_;               
    uint32 jumpRelocTableBytes_;    
    uint32 dataRelocTableBytes_;    
    JSBool invalidated_;            
                                    

    IonCode()
      : code_(NULL),
        pool_(NULL)
    { }
    IonCode(uint8 *code, uint32 bufferSize, JSC::ExecutablePool *pool)
      : code_(code),
        pool_(pool),
        bufferSize_(bufferSize),
        insnSize_(0),
        dataSize_(0),
        jumpRelocTableBytes_(0),
        dataRelocTableBytes_(0),
        invalidated_(false)
    { }

    uint32 dataOffset() const {
        return insnSize_;
    }
    uint32 jumpRelocTableOffset() const {
        return dataOffset() + dataSize_;
    }
    uint32 dataRelocTableOffset() const {
        return jumpRelocTableOffset() + jumpRelocTableBytes_;
    }

  public:
    uint8 *raw() const {
        return code_;
    }
    size_t instructionsSize() const {
        return insnSize_;
    }
    size_t bufferSize() const {
        return bufferSize_;
    }
    void trace(JSTracer *trc);
    void finalize(JSContext *cx, bool background);
    void setInvalidated() {
        invalidated_ = true;
    }

    
    
    
    bool invalidated() const {
        return !!invalidated_;
    }

    template <typename T> T as() const {
        return JS_DATA_TO_FUNC_PTR(T, raw());
    }

    void copyFrom(MacroAssembler &masm);

    static IonCode *FromExecutable(uint8 *buffer) {
        IonCode *code = *(IonCode **)(buffer - sizeof(IonCode *));
        JS_ASSERT(code->raw() == buffer);
        return code;
    }

    static size_t OffsetOfCode() {
        return offsetof(IonCode, code_);
    }

    uint8 *jumpRelocTable() {
        return code_ + jumpRelocTableOffset();
    }

    
    
    
    static IonCode *New(JSContext *cx, uint8 *code, uint32 bufferSize, JSC::ExecutablePool *pool);

  public:
    static void readBarrier(IonCode *code);
    static void writeBarrierPre(IonCode *code);
    static void writeBarrierPost(IonCode *code, void *addr);
};

class SnapshotWriter;
class SafepointWriter;
class SafepointIndex;
class OsiIndex;
class IonCache;


struct IonScript
{
    
    HeapPtr<IonCode> method_;

    
    HeapPtr<IonCode> deoptTable_;

    
    jsbytecode *osrPc_;

    
    uint32 osrEntryOffset_;

    
    
    uint32 invalidateEpilogueOffset_;

    
    
    
    
    uint32 invalidateEpilogueDataOffset_;

    
    
    bool forbidOsr_;

    
    uint32 snapshots_;
    uint32 snapshotsSize_;

    
    uint32 bailoutTable_;
    uint32 bailoutEntries_;

    
    uint32 constantTable_;
    uint32 constantEntries_;

    
    uint32 safepointIndexOffset_;
    uint32 safepointIndexEntries_;

    
    uint32 osiIndexOffset_;
    uint32 osiIndexEntries_;

    
    
    
    uint32 frameLocals_;
    uint32 frameSize_;

    
    uint32 safepointsStart_;
    uint32 safepointsSize_;

    
    uint32 cacheList_;
    uint32 cacheEntries_;

    
    size_t refcount_;

    SnapshotOffset *bailoutTable() {
        return (SnapshotOffset *)(reinterpret_cast<uint8 *>(this) + bailoutTable_);
    }
    HeapValue *constants() {
        return (HeapValue *)(reinterpret_cast<uint8 *>(this) + constantTable_);
    }
    const SafepointIndex *safepointIndices() const {
        return const_cast<IonScript *>(this)->safepointIndices();
    }
    SafepointIndex *safepointIndices() {
        return (SafepointIndex *)(reinterpret_cast<uint8 *>(this) + safepointIndexOffset_);
    }
    const OsiIndex *osiIndices() const {
        return const_cast<IonScript *>(this)->osiIndices();
    }
    OsiIndex *osiIndices() {
        return (OsiIndex *)(reinterpret_cast<uint8 *>(this) + osiIndexOffset_);
    }
    IonCache *cacheList() {
        return (IonCache *)(reinterpret_cast<uint8 *>(this) + cacheList_);
    }

  private:
    void trace(JSTracer *trc);

  public:
    
    IonScript();

    static IonScript *New(JSContext *cx, uint32 frameLocals, uint32 frameSize,
                          size_t snapshotsSize, size_t snapshotEntries,
                          size_t constants, size_t safepointIndexEntries, size_t osiIndexEntries,
                          size_t cacheEntries, size_t safepointsSize);
    static void Trace(JSTracer *trc, IonScript *script);
    static void Destroy(JSContext *cx, IonScript *script);

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
    void setOsrEntryOffset(uint32 offset) {
        JS_ASSERT(!osrEntryOffset_);
        osrEntryOffset_ = offset;
    }
    uint32 osrEntryOffset() const {
        return osrEntryOffset_;
    }
    bool containsCodeAddress(uint8 *addr) const {
        return method()->raw() <= addr && addr <= method()->raw() + method()->instructionsSize();
    }
    bool containsReturnAddress(uint8 *addr) const {
        
        
        return method()->raw() <= addr && addr <= method()->raw() + method()->instructionsSize();
    }
    void setInvalidationEpilogueOffset(uint32 offset) {
        JS_ASSERT(!invalidateEpilogueOffset_);
        invalidateEpilogueOffset_ = offset;
    }
    uint32 invalidateEpilogueOffset() const {
        JS_ASSERT(invalidateEpilogueOffset_);
        return invalidateEpilogueOffset_;
    }
    void setInvalidationEpilogueDataOffset(uint32 offset) {
        JS_ASSERT(!invalidateEpilogueDataOffset_);
        invalidateEpilogueDataOffset_ = offset;
    }
    uint32 invalidateEpilogueDataOffset() const {
        JS_ASSERT(invalidateEpilogueDataOffset_);
        return invalidateEpilogueDataOffset_;
    }
    void forbidOsr() {
        forbidOsr_ = true;
    }
    bool isOsrForbidden() const {
        return forbidOsr_;
    }
    const uint8 *snapshots() const {
        return reinterpret_cast<const uint8 *>(this) + snapshots_;
    }
    size_t snapshotsSize() const {
        return snapshotsSize_;
    }
    const uint8 *safepoints() const {
        return reinterpret_cast<const uint8 *>(this) + safepointsStart_;
    }
    size_t safepointsSize() const {
        return safepointsSize_;
    }
    size_t size() const {
        return safepointsStart_ + safepointsSize_;
    }
    HeapValue &getConstant(size_t index) {
        JS_ASSERT(index < numConstants());
        return constants()[index];
    }
    size_t numConstants() const {
        return constantEntries_;
    }
    uint32 frameLocals() const {
        return frameLocals_;
    }
    uint32 frameSize() const {
        return frameSize_;
    }
    SnapshotOffset bailoutToSnapshot(uint32 bailoutId) {
        JS_ASSERT(bailoutId < bailoutEntries_);
        return bailoutTable()[bailoutId];
    }
    const SafepointIndex *getSafepointIndex(uint32 disp) const;
    const SafepointIndex *getSafepointIndex(uint8 *retAddr) const {
        JS_ASSERT(containsCodeAddress(retAddr));
        return getSafepointIndex(retAddr - method()->raw());
    }
    const OsiIndex *getOsiIndex(uint32 disp) const;
    const OsiIndex *getOsiIndex(uint8 *retAddr) const;
    inline IonCache &getCache(size_t index);
    size_t numCaches() const {
        return cacheEntries_;
    }
    void copySnapshots(const SnapshotWriter *writer);
    void copyBailoutTable(const SnapshotOffset *table);
    void copyConstants(const HeapValue *vp);
    void copySafepointIndices(const SafepointIndex *firstSafepointIndex, MacroAssembler &masm);
    void copyOsiIndices(const OsiIndex *firstOsiIndex, MacroAssembler &masm);
    void copyCacheEntries(const IonCache *caches, MacroAssembler &masm);
    void copySafepoints(const SafepointWriter *writer);

    bool invalidated() const {
        return refcount_ != 0;
    }
    size_t refcount() const {
        return refcount_;
    }
    void incref() {
        refcount_++;
    }
    void decref(JSContext *cx) {
        JS_ASSERT(refcount_);
        refcount_--;
        if (!refcount_)
            Destroy(cx, this);
    }
};

struct VMFunction;

} 

namespace gc {

inline bool
IsMarked(JSContext *, const ion::VMFunction *)
{
    
    
    return true;
}

} 

} 

#endif 

