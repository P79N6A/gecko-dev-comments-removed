





#ifndef jit_JitcodeMap_h
#define jit_JitcodeMap_h

#include "ds/SplayTree.h"
#include "jit/CompactBuffer.h"
#include "jit/CompileInfo.h"
#include "jit/shared/CodeGenerator-shared.h"

namespace js {
namespace jit {
















class JitcodeMainTable;

class JitcodeGlobalEntry
{
  public:
    enum Kind {
        INVALID = 0,
        Main,
        Query,
        LIMIT
    };
    JS_STATIC_ASSERT(LIMIT <= 8);

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
            JS_ASSERT(nativeStartAddr);
            JS_ASSERT(nativeEndAddr);
            JS_ASSERT(kind > INVALID && kind < LIMIT);
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

    struct MainEntry : public BaseEntry
    {
        uintptr_t scriptList_;
        JitcodeMainTable *regionTable_;

        static const unsigned LowBits = 3;
        static const uintptr_t LowMask = (uintptr_t(1) << LowBits) - 1;

        enum ScriptListTag {
            Single = 0,
            Multi = 7
        };

        struct SizedScriptList {
            uint32_t size;
            JSScript *scripts[0];
            SizedScriptList(uint32_t sz, JSScript **scr) : size(sz) {
                for (uint32_t i = 0; i < size; i++)
                    scripts[i] = scr[i];
            }
        };

        void init(void *nativeStartAddr, void *nativeEndAddr,
                  JSScript *script, JitcodeMainTable *regionTable)
        {
            JS_ASSERT((uintptr_t(script) & LowMask) == 0);
            JS_ASSERT(script);
            JS_ASSERT(regionTable);
            BaseEntry::init(Main, nativeStartAddr, nativeEndAddr);
            scriptList_ = uintptr_t(script);
            regionTable_ = regionTable;
        }

        void init(void *nativeStartAddr, void *nativeEndAddr,
                  unsigned numScripts, JSScript **scripts, JitcodeMainTable *regionTable)
        {
            JS_ASSERT((uintptr_t(scripts) & LowMask) == 0);
            JS_ASSERT(numScripts >= 1);
            JS_ASSERT(numScripts <= 6);
            JS_ASSERT(scripts);
            JS_ASSERT(regionTable);
            BaseEntry::init(Main, nativeStartAddr, nativeEndAddr);
            scriptList_ = uintptr_t(scripts) | numScripts;
            regionTable_ = regionTable;
        }

        void init(void *nativeStartAddr, void *nativeEndAddr,
                  SizedScriptList *scripts, JitcodeMainTable *regionTable)
        {
            JS_ASSERT((uintptr_t(scripts) & LowMask) == 0);
            JS_ASSERT(scripts->size > 6);
            JS_ASSERT(scripts);
            JS_ASSERT(regionTable);

            BaseEntry::init(Main, nativeStartAddr, nativeEndAddr);
            scriptList_ = uintptr_t(scripts) | uintptr_t(Multi);
            regionTable_ = regionTable;
        }

        ScriptListTag scriptListTag() const {
            return static_cast<ScriptListTag>(scriptList_ & LowMask);
        }
        void *scriptListPointer() const {
            return reinterpret_cast<void *>(scriptList_ & ~LowMask);
        }

        JSScript *singleScript() const {
            JS_ASSERT(scriptListTag() == Single);
            return reinterpret_cast<JSScript *>(scriptListPointer());
        }
        JSScript **rawScriptArray() const {
            JS_ASSERT(scriptListTag() < Multi);
            return reinterpret_cast<JSScript **>(scriptListPointer());
        }
        SizedScriptList *sizedScriptList() const {
            JS_ASSERT(scriptListTag() == Multi);
            return reinterpret_cast<SizedScriptList *>(scriptListPointer());
        }

        unsigned numScripts() const {
            ScriptListTag tag = scriptListTag();
            if (tag == Single)
                return 1;

            if (tag < Multi) {
                JS_ASSERT(int(tag) >= 2);
                return static_cast<unsigned>(tag);
            }

            return sizedScriptList()->size;
        }

        JSScript *getScript(unsigned idx) const {
            JS_ASSERT(idx < numScripts());

            ScriptListTag tag = scriptListTag();

            if (tag == Single)
                return singleScript();

            if (tag < Multi) {
                JS_ASSERT(int(tag) >= 2);
                return rawScriptArray()[idx];
            }

            return sizedScriptList()->scripts[idx];
        }

        void destroy();

        JitcodeMainTable *regionTable() const {
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

        
        
        MainEntry main_;

        
        
        QueryEntry query_;
    };

  public:
    JitcodeGlobalEntry() {
        base_.init();
    }

    JitcodeGlobalEntry(const MainEntry &main) {
        main_ = main;
    }

    JitcodeGlobalEntry(const QueryEntry &query) {
        query_ = query;
    }

    static JitcodeGlobalEntry MakeQuery(void *ptr) {
        QueryEntry query;
        query.init(ptr);
        return JitcodeGlobalEntry(query);
    }

    void destroy() {
        switch (kind()) {
          case Main:
            mainEntry().destroy();
            break;
          case Query:
            queryEntry().destroy();
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("Invalid JitcodeGlobalEntry kind.");
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

    bool isMain() const {
        return kind() == Main;
    }
    bool isQuery() const {
        return kind() == Query;
    }

    MainEntry &mainEntry() {
        JS_ASSERT(isMain());
        return main_;
    }
    QueryEntry &queryEntry() {
        JS_ASSERT(isQuery());
        return query_;
    }

    const MainEntry &mainEntry() const {
        JS_ASSERT(isMain());
        return main_;
    }
    const QueryEntry &queryEntry() const {
        JS_ASSERT(isQuery());
        return query_;
    }

    
    
    uint32_t lookupInlineCallDepth(void *ptr);

    
    static int compare(const JitcodeGlobalEntry &ent1, const JitcodeGlobalEntry &ent2);
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
    JitcodeGlobalTable() : treeAlloc_(LIFO_CHUNK_SIZE), tree_(&treeAlloc_), entries_() {}
    ~JitcodeGlobalTable() {}

    bool lookup(void *ptr, JitcodeGlobalEntry *result);
    void lookupInfallible(void *ptr, JitcodeGlobalEntry *result);

    bool addEntry(const JitcodeGlobalEntry::MainEntry &entry) {
        return addEntry(JitcodeGlobalEntry(entry));
    }

  private:
    bool addEntry(const JitcodeGlobalEntry &entry);
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
        JS_ASSERT(data_ < end_);
        unpack();
        JS_ASSERT(scriptPcStack_ < end_);
        JS_ASSERT(deltaRun_ <= end_);
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
            JS_ASSERT((idx_ == count_) == (cur_ == end_));
            JS_ASSERT((idx_ < count_) == (cur_ < end_));
            return cur_ < end_;
        }

        void readNext(uint32_t *scriptIdxOut, uint32_t *pcOffsetOut)
        {
            JS_ASSERT(scriptIdxOut);
            JS_ASSERT(pcOffsetOut);
            JS_ASSERT(hasMore());

            CompactBufferReader reader(cur_, end_);
            ReadScriptPc(reader, scriptIdxOut, pcOffsetOut);

            cur_ = reader.currentPosition();
            JS_ASSERT(cur_ <= end_);

            idx_++;
            JS_ASSERT_IF(idx_ == count_, cur_ == end_);
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
            JS_ASSERT(cur_ <= end_);
            return cur_ < end_;
        }

        void readNext(uint32_t *nativeDeltaOut, int32_t *pcDeltaOut)
        {
            JS_ASSERT(nativeDeltaOut != nullptr);
            JS_ASSERT(pcDeltaOut != nullptr);

            JS_ASSERT(hasMore());

            CompactBufferReader reader(cur_, end_);
            ReadDelta(reader, nativeDeltaOut, pcDeltaOut);

            cur_ = reader.currentPosition();
            JS_ASSERT(cur_ <= end_);
        }

        void reset() {
            cur_ = start_;
        }
    };
    DeltaIterator deltaIterator() const {
        return DeltaIterator(deltaRun_, end_);
    }
};

class JitcodeMainTable
{
  private:
    
    uint32_t numRegions_;
    uint32_t regionOffsets_[0];

    const uint8_t *payloadEnd() const {
        return reinterpret_cast<const uint8_t *>(this);
    }

  public:
    JitcodeMainTable(uint32_t numRegions)
      : numRegions_(numRegions)
    {
        for (uint32_t i = 0; i < numRegions; i++)
            regionOffsets_[i] = 0;
    }

    uint32_t numRegions() const {
        return numRegions_;
    }

    uint32_t regionOffset(uint32_t regionIndex) const {
        JS_ASSERT(regionIndex < numRegions());
        return regionOffsets_[regionIndex];
    }

    JitcodeRegionEntry regionEntry(uint32_t regionIndex) const {
        const uint8_t *regionStart = payloadEnd() - regionOffset(regionIndex);
        const uint8_t *regionEnd = payloadEnd();
        if (regionIndex < numRegions_ - 1)
            regionEnd -= regionOffset(regionIndex + 1);
        return JitcodeRegionEntry(regionStart, regionEnd);
    }

    const uint8_t *payloadStart() const {
        
        return payloadEnd() - regionOffset(0);
    }

    static bool WriteMainTable(CompactBufferWriter &writer,
                               JSScript **scriptList, uint32_t scriptListSize,
                               const CodeGeneratorShared::NativeToBytecode *start,
                               const CodeGeneratorShared::NativeToBytecode *end,
                               uint32_t *tableOffsetOut, uint32_t *numRegionsOut);
};


} 
} 

#endif 
