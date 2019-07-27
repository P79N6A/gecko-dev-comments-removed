





#ifndef jit_JitcodeMap_h
#define jit_JitcodeMap_h

#include "ds/SplayTree.h"
#include "jit/CompactBuffer.h"
#include "jit/CompileInfo.h"
#include "jit/shared/CodeGenerator-shared.h"

namespace js {
namespace jit {
















class JitcodeIonTable;
class JitcodeRegionEntry;

class JitcodeGlobalEntry
{
  public:
    enum Kind {
        INVALID = 0,
        Ion,
        Baseline,
        IonCache,
        Dummy,
        Query,
        LIMIT
    };
    JS_STATIC_ASSERT(LIMIT <= 8);

    struct BytecodeLocation {
        JSScript *script;
        jsbytecode *pc;
        BytecodeLocation(JSScript *script, jsbytecode *pc) : script(script), pc(pc) {}
    };
    typedef Vector<BytecodeLocation, 0, SystemAllocPolicy> BytecodeLocationVector;
    typedef Vector<const char *, 0, SystemAllocPolicy> ProfileStringVector;

    struct BaseEntry
    {
        void *nativeStartAddr_;
        void *nativeEndAddr_;
        Kind kind_;

        void init() {
            nativeStartAddr_ = nullptr;
            nativeEndAddr_ = nullptr;
            kind_ = INVALID;
        }

        void init(Kind kind, void *nativeStartAddr, void *nativeEndAddr) {
            MOZ_ASSERT(nativeStartAddr);
            MOZ_ASSERT(nativeEndAddr);
            MOZ_ASSERT(kind > INVALID && kind < LIMIT);
            nativeStartAddr_ = nativeStartAddr;
            nativeEndAddr_ = nativeEndAddr;
            kind_ = kind;
        }

        Kind kind() const {
            return kind_;
        }
        void *nativeStartAddr() const {
            return nativeStartAddr_;
        }
        void *nativeEndAddr() const {
            return nativeEndAddr_;
        }

        bool startsBelowPointer(void *ptr) const {
            return ((uint8_t *)nativeStartAddr()) <= ((uint8_t *) ptr);
        }
        bool endsAbovePointer(void *ptr) const {
            return ((uint8_t *)nativeEndAddr()) > ((uint8_t *) ptr);
        }
        bool containsPointer(void *ptr) const {
            return startsBelowPointer(ptr) && endsAbovePointer(ptr);
        }
    };

    struct IonEntry : public BaseEntry
    {
        
        
        
        
        
        JitcodeIonTable *regionTable_;

        struct ScriptNamePair {
            JSScript *script;
            char *str;
        };

        struct SizedScriptList {
            uint32_t size;
            ScriptNamePair pairs[0];
            SizedScriptList(uint32_t sz, JSScript **scrs, char **strs) : size(sz) {
                for (uint32_t i = 0; i < size; i++) {
                    pairs[i].script = scrs[i];
                    pairs[i].str = strs[i];
                }
            }

            static uint32_t AllocSizeFor(uint32_t nscripts) {
                return sizeof(SizedScriptList) + (nscripts * sizeof(ScriptNamePair));
            }
        };

        SizedScriptList *scriptList_;

        void init(void *nativeStartAddr, void *nativeEndAddr,
                  SizedScriptList *scriptList, JitcodeIonTable *regionTable)
        {
            MOZ_ASSERT(scriptList);
            MOZ_ASSERT(regionTable);
            BaseEntry::init(Ion, nativeStartAddr, nativeEndAddr);
            regionTable_ = regionTable;
            scriptList_ = scriptList;
        }

        SizedScriptList *sizedScriptList() const {
            return scriptList_;
        }

        unsigned numScripts() const {
            return scriptList_->size;
        }

        JSScript *getScript(unsigned idx) const {
            MOZ_ASSERT(idx < numScripts());
            return sizedScriptList()->pairs[idx].script;
        }

        const char *getStr(unsigned idx) const {
            MOZ_ASSERT(idx < numScripts());
            return sizedScriptList()->pairs[idx].str;
        }

        void destroy();

        JitcodeIonTable *regionTable() const {
            return regionTable_;
        }

        int scriptIndex(JSScript *script) const {
            unsigned count = numScripts();
            for (unsigned i = 0; i < count; i++) {
                if (getScript(i) == script)
                    return i;
            }
            return -1;
        }

        bool callStackAtAddr(JSRuntime *rt, void *ptr, BytecodeLocationVector &results,
                             uint32_t *depth) const;

        uint32_t callStackAtAddr(JSRuntime *rt, void *ptr, const char **results,
                                 uint32_t maxResults) const;
    };

    struct BaselineEntry : public BaseEntry
    {
        JSScript *script_;
        const char *str_;

        void init(void *nativeStartAddr, void *nativeEndAddr, JSScript *script, const char *str)
        {
            MOZ_ASSERT(script != nullptr);
            BaseEntry::init(Baseline, nativeStartAddr, nativeEndAddr);
            script_ = script;
            str_ = str;
        }

        JSScript *script() const {
            return script_;
        }

        const char *str() const {
            return str_;
        }

        void destroy() {}

        bool callStackAtAddr(JSRuntime *rt, void *ptr, BytecodeLocationVector &results,
                             uint32_t *depth) const;

        uint32_t callStackAtAddr(JSRuntime *rt, void *ptr, const char **results,
                                 uint32_t maxResults) const;
    };

    struct IonCacheEntry : public BaseEntry
    {
        void *rejoinAddr_;

        void init(void *nativeStartAddr, void *nativeEndAddr, void *rejoinAddr)
        {
            MOZ_ASSERT(rejoinAddr != nullptr);
            BaseEntry::init(IonCache, nativeStartAddr, nativeEndAddr);
            rejoinAddr_ = rejoinAddr;
        }

        void *rejoinAddr() const {
            return rejoinAddr_;
        }

        void destroy() {}

        bool callStackAtAddr(JSRuntime *rt, void *ptr, BytecodeLocationVector &results,
                             uint32_t *depth) const;

        uint32_t callStackAtAddr(JSRuntime *rt, void *ptr, const char **results,
                                 uint32_t maxResults) const;
    };

    
    
    
    struct DummyEntry : public BaseEntry
    {
        void init(void *nativeStartAddr, void *nativeEndAddr) {
            BaseEntry::init(Dummy, nativeStartAddr, nativeEndAddr);
        }

        void destroy() {}

        bool callStackAtAddr(JSRuntime *rt, void *ptr, BytecodeLocationVector &results,
                             uint32_t *depth) const
        {
            return true;
        }

        uint32_t callStackAtAddr(JSRuntime *rt, void *ptr, const char **results,
                                 uint32_t maxResults) const
        {
            return 0;
        }
    };

    
    
    
    struct QueryEntry : public BaseEntry
    {
        void init(void *addr) {
            BaseEntry::init(Query, addr, addr);
        }
        uint8_t *addr() const {
            return reinterpret_cast<uint8_t *>(nativeStartAddr());
        }
        void destroy() {}
    };

  private:
    union {
        
        
        BaseEntry base_;

        
        
        IonEntry ion_;

        
        BaselineEntry baseline_;

        
        IonCacheEntry ionCache_;

        
        DummyEntry dummy_;

        
        
        QueryEntry query_;
    };

  public:
    JitcodeGlobalEntry() {
        base_.init();
    }

    explicit JitcodeGlobalEntry(const IonEntry &ion) {
        ion_ = ion;
    }

    explicit JitcodeGlobalEntry(const BaselineEntry &baseline) {
        baseline_ = baseline;
    }

    explicit JitcodeGlobalEntry(const IonCacheEntry &ionCache) {
        ionCache_ = ionCache;
    }

    explicit JitcodeGlobalEntry(const DummyEntry &dummy) {
        dummy_ = dummy;
    }

    explicit JitcodeGlobalEntry(const QueryEntry &query) {
        query_ = query;
    }

    static JitcodeGlobalEntry MakeQuery(void *ptr) {
        QueryEntry query;
        query.init(ptr);
        return JitcodeGlobalEntry(query);
    }

    void destroy() {
        switch (kind()) {
          case Ion:
            ionEntry().destroy();
            break;
          case Baseline:
            baselineEntry().destroy();
            break;
          case IonCache:
            ionCacheEntry().destroy();
            break;
          case Dummy:
            dummyEntry().destroy();
            break;
          case Query:
            queryEntry().destroy();
            break;
          default:
            MOZ_CRASH("Invalid JitcodeGlobalEntry kind.");
        }
    }

    void *nativeStartAddr() const {
        return base_.nativeStartAddr();
    }
    void *nativeEndAddr() const {
        return base_.nativeEndAddr();
    }

    bool startsBelowPointer(void *ptr) const {
        return base_.startsBelowPointer(ptr);
    }
    bool endsAbovePointer(void *ptr) const {
        return base_.endsAbovePointer(ptr);
    }
    bool containsPointer(void *ptr) const {
        return base_.containsPointer(ptr);
    }

    bool overlapsWith(const JitcodeGlobalEntry &entry) const {
        
        if (containsPointer(entry.nativeStartAddr()) || containsPointer(entry.nativeEndAddr()))
            return true;

        
        if (startsBelowPointer(entry.nativeEndAddr()) && endsAbovePointer(entry.nativeStartAddr()))
            return true;

        return false;
    }

    Kind kind() const {
        return base_.kind();
    }

    bool isIon() const {
        return kind() == Ion;
    }
    bool isBaseline() const {
        return kind() == Baseline;
    }
    bool isIonCache() const {
        return kind() == IonCache;
    }
    bool isDummy() const {
        return kind() == Dummy;
    }
    bool isQuery() const {
        return kind() == Query;
    }

    IonEntry &ionEntry() {
        MOZ_ASSERT(isIon());
        return ion_;
    }
    BaselineEntry &baselineEntry() {
        MOZ_ASSERT(isBaseline());
        return baseline_;
    }
    IonCacheEntry &ionCacheEntry() {
        MOZ_ASSERT(isIonCache());
        return ionCache_;
    }
    DummyEntry &dummyEntry() {
        MOZ_ASSERT(isDummy());
        return dummy_;
    }
    QueryEntry &queryEntry() {
        MOZ_ASSERT(isQuery());
        return query_;
    }

    const IonEntry &ionEntry() const {
        MOZ_ASSERT(isIon());
        return ion_;
    }
    const BaselineEntry &baselineEntry() const {
        MOZ_ASSERT(isBaseline());
        return baseline_;
    }
    const IonCacheEntry &ionCacheEntry() const {
        MOZ_ASSERT(isIonCache());
        return ionCache_;
    }
    const DummyEntry &dummyEntry() const {
        MOZ_ASSERT(isDummy());
        return dummy_;
    }
    const QueryEntry &queryEntry() const {
        MOZ_ASSERT(isQuery());
        return query_;
    }

    
    
    
    
    
    bool callStackAtAddr(JSRuntime *rt, void *ptr, BytecodeLocationVector &results,
                         uint32_t *depth) const
    {
        switch (kind()) {
          case Ion:
            return ionEntry().callStackAtAddr(rt, ptr, results, depth);
          case Baseline:
            return baselineEntry().callStackAtAddr(rt, ptr, results, depth);
          case IonCache:
            return ionCacheEntry().callStackAtAddr(rt, ptr, results, depth);
          case Dummy:
            return dummyEntry().callStackAtAddr(rt, ptr, results, depth);
          default:
            MOZ_CRASH("Invalid JitcodeGlobalEntry kind.");
        }
        return false;
    }

    uint32_t callStackAtAddr(JSRuntime *rt, void *ptr, const char **results,
                             uint32_t maxResults) const
    {
        switch (kind()) {
          case Ion:
            return ionEntry().callStackAtAddr(rt, ptr, results, maxResults);
          case Baseline:
            return baselineEntry().callStackAtAddr(rt, ptr, results, maxResults);
          case IonCache:
            return ionCacheEntry().callStackAtAddr(rt, ptr, results, maxResults);
          case Dummy:
            return dummyEntry().callStackAtAddr(rt, ptr, results, maxResults);
          default:
            MOZ_CRASH("Invalid JitcodeGlobalEntry kind.");
        }
        return false;
    }

    
    
    uint32_t lookupInlineCallDepth(void *ptr);

    
    static int compare(const JitcodeGlobalEntry &ent1, const JitcodeGlobalEntry &ent2);

    
    static char *createScriptString(JSContext *cx, JSScript *script, size_t *length=nullptr);
};




class JitcodeGlobalTable
{
  public:
    typedef SplayTree<JitcodeGlobalEntry, JitcodeGlobalEntry> EntryTree;

    typedef Vector<JitcodeGlobalEntry, 0, SystemAllocPolicy> EntryVector;

  private:
    static const size_t LIFO_CHUNK_SIZE = 16 * 1024;
    LifoAlloc treeAlloc_;
    EntryTree tree_;
    EntryVector entries_;

  public:
    JitcodeGlobalTable() : treeAlloc_(LIFO_CHUNK_SIZE), tree_(&treeAlloc_), entries_() {
        
        
        tree_.disableCheckCoherency();
    }
    ~JitcodeGlobalTable() {}

    bool empty() const {
        return tree_.empty();
    }

    bool lookup(void *ptr, JitcodeGlobalEntry *result, JSRuntime *rt);
    void lookupInfallible(void *ptr, JitcodeGlobalEntry *result, JSRuntime *rt);

    bool addEntry(const JitcodeGlobalEntry::IonEntry &entry, JSRuntime *rt) {
        return addEntry(JitcodeGlobalEntry(entry), rt);
    }
    bool addEntry(const JitcodeGlobalEntry::BaselineEntry &entry, JSRuntime *rt) {
        return addEntry(JitcodeGlobalEntry(entry), rt);
    }
    bool addEntry(const JitcodeGlobalEntry::IonCacheEntry &entry, JSRuntime *rt) {
        return addEntry(JitcodeGlobalEntry(entry), rt);
    }
    bool addEntry(const JitcodeGlobalEntry::DummyEntry &entry, JSRuntime *rt) {
        return addEntry(JitcodeGlobalEntry(entry), rt);
    }

    void removeEntry(void *startAddr, JSRuntime *rt);

  private:
    bool addEntry(const JitcodeGlobalEntry &entry, JSRuntime *rt);
};








































































class JitcodeRegionEntry
{
  private:
    static const unsigned MAX_RUN_LENGTH = 100;

  public:
    static void WriteHead(CompactBufferWriter &writer,
                          uint32_t nativeOffset, uint8_t scriptDepth);
    static void ReadHead(CompactBufferReader &reader,
                         uint32_t *nativeOffset, uint8_t *scriptDepth);

    static void WriteScriptPc(CompactBufferWriter &writer, uint32_t scriptIdx, uint32_t pcOffset);
    static void ReadScriptPc(CompactBufferReader &reader, uint32_t *scriptIdx, uint32_t *pcOffset);

    static void WriteDelta(CompactBufferWriter &writer, uint32_t nativeDelta, int32_t pcDelta);
    static void ReadDelta(CompactBufferReader &reader, uint32_t *nativeDelta, int32_t *pcDelta);

    
    
    
    static uint32_t ExpectedRunLength(const CodeGeneratorShared::NativeToBytecode *entry,
                                      const CodeGeneratorShared::NativeToBytecode *end);

    
    static bool WriteRun(CompactBufferWriter &writer,
                         JSScript **scriptList, uint32_t scriptListSize,
                         uint32_t runLength, const CodeGeneratorShared::NativeToBytecode *entry);

    
    
    
    
    
    
    static const uint32_t ENC1_MASK = 0x1;
    static const uint32_t ENC1_MASK_VAL = 0x0;

    static const uint32_t ENC1_NATIVE_DELTA_MAX = 0xf;
    static const unsigned ENC1_NATIVE_DELTA_SHIFT = 4;

    static const uint32_t ENC1_PC_DELTA_MASK = 0x0e;
    static const int32_t ENC1_PC_DELTA_MAX = 0x7;
    static const unsigned ENC1_PC_DELTA_SHIFT = 1;

    
    
    
    
    static const uint32_t ENC2_MASK = 0x3;
    static const uint32_t ENC2_MASK_VAL = 0x1;

    static const uint32_t ENC2_NATIVE_DELTA_MAX = 0xff;
    static const unsigned ENC2_NATIVE_DELTA_SHIFT = 8;

    static const uint32_t ENC2_PC_DELTA_MASK = 0x00fc;
    static const int32_t ENC2_PC_DELTA_MAX = 0x3f;
    static const unsigned ENC2_PC_DELTA_SHIFT = 2;

    
    
    
    
    static const uint32_t ENC3_MASK = 0x7;
    static const uint32_t ENC3_MASK_VAL = 0x3;

    static const uint32_t ENC3_NATIVE_DELTA_MAX = 0x7ff;
    static const unsigned ENC3_NATIVE_DELTA_SHIFT = 13;

    static const uint32_t ENC3_PC_DELTA_MASK = 0x001ff8;
    static const int32_t ENC3_PC_DELTA_MAX = 0x1ff;
    static const int32_t ENC3_PC_DELTA_MIN = -ENC3_PC_DELTA_MAX - 1;
    static const unsigned ENC3_PC_DELTA_SHIFT = 3;

    
    
    
    static const uint32_t ENC4_MASK = 0x7;
    static const uint32_t ENC4_MASK_VAL = 0x7;

    static const uint32_t ENC4_NATIVE_DELTA_MAX = 0xffff;
    static const unsigned ENC4_NATIVE_DELTA_SHIFT = 16;

    static const uint32_t ENC4_PC_DELTA_MASK = 0x0000fff8;
    static const int32_t ENC4_PC_DELTA_MAX = 0xfff;
    static const int32_t ENC4_PC_DELTA_MIN = -ENC4_PC_DELTA_MAX - 1;
    static const unsigned ENC4_PC_DELTA_SHIFT = 3;

    static bool IsDeltaEncodeable(uint32_t nativeDelta, int32_t pcDelta) {
        return (nativeDelta <= ENC4_NATIVE_DELTA_MAX) &&
               (pcDelta >= ENC4_PC_DELTA_MIN) && (pcDelta <= ENC4_PC_DELTA_MAX);
    }

  private:
    const uint8_t *data_;
    const uint8_t *end_;

    
    uint32_t nativeOffset_;
    uint8_t scriptDepth_;
    const uint8_t *scriptPcStack_;
    const uint8_t *deltaRun_;

    void unpack();

  public:
    JitcodeRegionEntry(const uint8_t *data, const uint8_t *end)
      : data_(data), end_(end),
        nativeOffset_(0), scriptDepth_(0),
        scriptPcStack_(nullptr), deltaRun_(nullptr)
    {
        MOZ_ASSERT(data_ < end_);
        unpack();
        MOZ_ASSERT(scriptPcStack_ < end_);
        MOZ_ASSERT(deltaRun_ <= end_);
    }

    uint32_t nativeOffset() const {
        return nativeOffset_;
    }
    uint32_t scriptDepth() const {
        return scriptDepth_;
    }

    class ScriptPcIterator
    {
      private:
        uint32_t count_;
        const uint8_t *start_;
        const uint8_t *end_;

        uint32_t idx_;
        const uint8_t *cur_;

      public:
        ScriptPcIterator(uint32_t count, const uint8_t *start, const uint8_t *end)
          : count_(count), start_(start), end_(end), idx_(0), cur_(start_)
        {}

        bool hasMore() const
        {
            MOZ_ASSERT((idx_ == count_) == (cur_ == end_));
            MOZ_ASSERT((idx_ < count_) == (cur_ < end_));
            return cur_ < end_;
        }

        void readNext(uint32_t *scriptIdxOut, uint32_t *pcOffsetOut)
        {
            MOZ_ASSERT(scriptIdxOut);
            MOZ_ASSERT(pcOffsetOut);
            MOZ_ASSERT(hasMore());

            CompactBufferReader reader(cur_, end_);
            ReadScriptPc(reader, scriptIdxOut, pcOffsetOut);

            cur_ = reader.currentPosition();
            MOZ_ASSERT(cur_ <= end_);

            idx_++;
            MOZ_ASSERT_IF(idx_ == count_, cur_ == end_);
        }

        void reset() {
            idx_ = 0;
            cur_ = start_;
        }
    };

    ScriptPcIterator scriptPcIterator() const {
        
        return ScriptPcIterator(scriptDepth_, scriptPcStack_,  deltaRun_);
    }

    class DeltaIterator {
      private:
        const uint8_t *start_;
        const uint8_t *end_;
        const uint8_t *cur_;

      public:
        DeltaIterator(const uint8_t *start, const uint8_t *end)
          : start_(start), end_(end), cur_(start)
        {}

        bool hasMore() const
        {
            MOZ_ASSERT(cur_ <= end_);
            return cur_ < end_;
        }

        void readNext(uint32_t *nativeDeltaOut, int32_t *pcDeltaOut)
        {
            MOZ_ASSERT(nativeDeltaOut != nullptr);
            MOZ_ASSERT(pcDeltaOut != nullptr);

            MOZ_ASSERT(hasMore());

            CompactBufferReader reader(cur_, end_);
            ReadDelta(reader, nativeDeltaOut, pcDeltaOut);

            cur_ = reader.currentPosition();
            MOZ_ASSERT(cur_ <= end_);
        }

        void reset() {
            cur_ = start_;
        }
    };
    DeltaIterator deltaIterator() const {
        return DeltaIterator(deltaRun_, end_);
    }

    uint32_t findPcOffset(uint32_t queryNativeOffset, uint32_t startPcOffset) const;
};

class JitcodeIonTable
{
  private:
    
    uint32_t numRegions_;
    uint32_t regionOffsets_[0];

    const uint8_t *payloadEnd() const {
        return reinterpret_cast<const uint8_t *>(this);
    }

  public:
    explicit JitcodeIonTable(uint32_t numRegions)
      : numRegions_(numRegions)
    {
        for (uint32_t i = 0; i < numRegions; i++)
            regionOffsets_[i] = 0;
    }

    bool makeIonEntry(JSContext *cx, JitCode *code, uint32_t numScripts,
                      JSScript **scripts, JitcodeGlobalEntry::IonEntry &out);

    uint32_t numRegions() const {
        return numRegions_;
    }

    uint32_t regionOffset(uint32_t regionIndex) const {
        MOZ_ASSERT(regionIndex < numRegions());
        return regionOffsets_[regionIndex];
    }

    JitcodeRegionEntry regionEntry(uint32_t regionIndex) const {
        const uint8_t *regionStart = payloadEnd() - regionOffset(regionIndex);
        const uint8_t *regionEnd = payloadEnd();
        if (regionIndex < numRegions_ - 1)
            regionEnd -= regionOffset(regionIndex + 1);
        return JitcodeRegionEntry(regionStart, regionEnd);
    }

    bool regionContainsOffset(uint32_t regionIndex, uint32_t nativeOffset) {
        MOZ_ASSERT(regionIndex < numRegions());

        JitcodeRegionEntry ent = regionEntry(regionIndex);
        if (nativeOffset < ent.nativeOffset())
            return false;

        if (regionIndex == numRegions_ - 1)
            return true;

        return nativeOffset < regionEntry(regionIndex + 1).nativeOffset();
    }

    uint32_t findRegionEntry(uint32_t offset) const;

    const uint8_t *payloadStart() const {
        
        return payloadEnd() - regionOffset(0);
    }

    static bool WriteIonTable(CompactBufferWriter &writer,
                              JSScript **scriptList, uint32_t scriptListSize,
                              const CodeGeneratorShared::NativeToBytecode *start,
                              const CodeGeneratorShared::NativeToBytecode *end,
                              uint32_t *tableOffsetOut, uint32_t *numRegionsOut);
};


} 
} 

#endif 
