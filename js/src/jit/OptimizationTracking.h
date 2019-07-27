





#ifndef jit_OptimizationTracking_h
#define jit_OptimizationTracking_h

#include "mozilla/Maybe.h"

#include "jit/CompactBuffer.h"
#include "jit/CompileInfo.h"
#include "jit/JitAllocPolicy.h"
#include "js/TrackedOptimizationInfo.h"
#include "vm/TypeInference.h"

namespace js {

namespace jit {

struct NativeToTrackedOptimizations;

class OptimizationAttempt
{
    JS::TrackedStrategy strategy_;
    JS::TrackedOutcome outcome_;

  public:
    OptimizationAttempt(JS::TrackedStrategy strategy, JS::TrackedOutcome outcome)
      : strategy_(strategy),
        outcome_(outcome)
    { }

    void setOutcome(JS::TrackedOutcome outcome) { outcome_ = outcome; }
    bool succeeded() const { return outcome_ >= JS::TrackedOutcome::GenericSuccess; }
    bool failed() const { return outcome_ < JS::TrackedOutcome::GenericSuccess; }
    JS::TrackedStrategy strategy() const { return strategy_; }
    JS::TrackedOutcome outcome() const { return outcome_; }

    bool operator ==(const OptimizationAttempt &other) const {
        return strategy_ == other.strategy_ && outcome_ == other.outcome_;
    }
    bool operator !=(const OptimizationAttempt &other) const {
        return strategy_ != other.strategy_ || outcome_ != other.outcome_;
    }
    HashNumber hash() const {
        return (HashNumber(strategy_) << 8) + HashNumber(outcome_);
    }

    void writeCompact(CompactBufferWriter &writer) const;
};

typedef Vector<OptimizationAttempt, 4, JitAllocPolicy> TempOptimizationAttemptsVector;

class UniqueTrackedTypes;

class OptimizationTypeInfo
{
    JS::TrackedTypeSite site_;
    MIRType mirType_;
    TypeSet::TypeList types_;

  public:
    OptimizationTypeInfo(OptimizationTypeInfo &&other)
      : site_(other.site_),
        mirType_(other.mirType_),
        types_(mozilla::Move(other.types_))
    { }

    OptimizationTypeInfo(JS::TrackedTypeSite site, MIRType mirType)
      : site_(site),
        mirType_(mirType)
    { }

    bool trackTypeSet(TemporaryTypeSet *typeSet);
    bool trackType(TypeSet::Type type);

    JS::TrackedTypeSite site() const { return site_; }
    MIRType mirType() const { return mirType_; }
    const TypeSet::TypeList &types() const { return types_; }

    bool operator ==(const OptimizationTypeInfo &other) const;
    bool operator !=(const OptimizationTypeInfo &other) const;

    HashNumber hash() const;

    bool writeCompact(CompactBufferWriter &writer, UniqueTrackedTypes &uniqueTypes) const;
};

typedef Vector<OptimizationTypeInfo, 1, JitAllocPolicy> TempOptimizationTypeInfoVector;


class TrackedOptimizations : public TempObject
{
    friend class UniqueTrackedOptimizations;
    TempOptimizationTypeInfoVector types_;
    TempOptimizationAttemptsVector attempts_;
    uint32_t currentAttempt_;

  public:
    explicit TrackedOptimizations(TempAllocator &alloc)
      : types_(alloc),
        attempts_(alloc),
        currentAttempt_(UINT32_MAX)
    { }

    bool trackTypeInfo(OptimizationTypeInfo &&ty);

    bool trackAttempt(JS::TrackedStrategy strategy);
    void amendAttempt(uint32_t index);
    void trackOutcome(JS::TrackedOutcome outcome);
    void trackSuccess();

    bool matchTypes(const TempOptimizationTypeInfoVector &other) const;
    bool matchAttempts(const TempOptimizationAttemptsVector &other) const;

    void spew() const;
};



class UniqueTrackedOptimizations
{
  public:
    struct SortEntry
    {
        const TempOptimizationTypeInfoVector *types;
        const TempOptimizationAttemptsVector *attempts;
        uint32_t frequency;
    };
    typedef Vector<SortEntry, 4> SortedVector;

  private:
    struct Key
    {
        const TempOptimizationTypeInfoVector *types;
        const TempOptimizationAttemptsVector *attempts;

        typedef Key Lookup;
        static HashNumber hash(const Lookup &lookup);
        static bool match(const Key &key, const Lookup &lookup);
        static void rekey(Key &key, const Key &newKey) {
            key = newKey;
        }
    };

    struct Entry
    {
        uint8_t index;
        uint32_t frequency;
    };

    
    
    typedef HashMap<Key, Entry, Key> AttemptsMap;
    AttemptsMap map_;

    
    SortedVector sorted_;

  public:
    explicit UniqueTrackedOptimizations(JSContext *cx)
      : map_(cx),
        sorted_(cx)
    { }

    bool init() { return map_.init(); }
    bool add(const TrackedOptimizations *optimizations);

    bool sortByFrequency(JSContext *cx);
    bool sorted() const { return !sorted_.empty(); }
    uint32_t count() const { MOZ_ASSERT(sorted()); return sorted_.length(); }
    const SortedVector &sortedVector() const { MOZ_ASSERT(sorted()); return sorted_; }
    uint8_t indexOf(const TrackedOptimizations *optimizations) const;
};

























































































class IonTrackedOptimizationsRegion
{
    const uint8_t *start_;
    const uint8_t *end_;

    
    uint32_t startOffset_;
    uint32_t endOffset_;
    const uint8_t *rangesStart_;

    void unpackHeader();

  public:
    IonTrackedOptimizationsRegion(const uint8_t *start, const uint8_t *end)
      : start_(start), end_(end),
        startOffset_(0), endOffset_(0), rangesStart_(nullptr)
    {
        MOZ_ASSERT(start < end);
        unpackHeader();
    }

    
    
    
    
    uint32_t startOffset() const { return startOffset_; }
    uint32_t endOffset() const { return endOffset_; }

    class RangeIterator
    {
        const uint8_t *cur_;
        const uint8_t *start_;
        const uint8_t *end_;

        uint32_t firstStartOffset_;
        uint32_t prevEndOffset_;

      public:
        RangeIterator(const uint8_t *start, const uint8_t *end, uint32_t startOffset)
          : cur_(start), start_(start), end_(end),
            firstStartOffset_(startOffset), prevEndOffset_(0)
        { }

        bool more() const { return cur_ < end_; }
        void readNext(uint32_t *startOffset, uint32_t *endOffset, uint8_t *index);
    };

    RangeIterator ranges() const { return RangeIterator(rangesStart_, end_, startOffset_); }

    
    
    mozilla::Maybe<uint8_t> findIndex(uint32_t offset) const;

    
    
    
    
    
    
    

    static const uint32_t ENC1_MASK = 0x1;
    static const uint32_t ENC1_MASK_VAL = 0x0;

    static const uint32_t ENC1_START_DELTA_MAX = 0x7f;
    static const uint32_t ENC1_START_DELTA_SHIFT = 9;

    static const uint32_t ENC1_LENGTH_MAX = 0x3f;
    static const uint32_t ENC1_LENGTH_SHIFT = 3;

    static const uint32_t ENC1_INDEX_MAX = 0x3;
    static const uint32_t ENC1_INDEX_SHIFT = 1;

    
    
    

    static const uint32_t ENC2_MASK = 0x3;
    static const uint32_t ENC2_MASK_VAL = 0x1;

    static const uint32_t ENC2_START_DELTA_MAX = 0xfff;
    static const uint32_t ENC2_START_DELTA_SHIFT = 12;

    static const uint32_t ENC2_LENGTH_MAX = 0x3f;
    static const uint32_t ENC2_LENGTH_SHIFT = 6;

    static const uint32_t ENC2_INDEX_MAX = 0xf;
    static const uint32_t ENC2_INDEX_SHIFT = 2;

    
    
    

    static const uint32_t ENC3_MASK = 0x7;
    static const uint32_t ENC3_MASK_VAL = 0x3;

    static const uint32_t ENC3_START_DELTA_MAX = 0x7ff;
    static const uint32_t ENC3_START_DELTA_SHIFT = 21;

    static const uint32_t ENC3_LENGTH_MAX = 0x3ff;
    static const uint32_t ENC3_LENGTH_SHIFT = 11;

    static const uint32_t ENC3_INDEX_MAX = 0xff;
    static const uint32_t ENC3_INDEX_SHIFT = 3;

    
    
    

    static const uint32_t ENC4_MASK = 0x7;
    static const uint32_t ENC4_MASK_VAL = 0x7;

    static const uint32_t ENC4_START_DELTA_MAX = 0x7fff;
    static const uint32_t ENC4_START_DELTA_SHIFT = 25;

    static const uint32_t ENC4_LENGTH_MAX = 0x3fff;
    static const uint32_t ENC4_LENGTH_SHIFT = 11;

    static const uint32_t ENC4_INDEX_MAX = 0xff;
    static const uint32_t ENC4_INDEX_SHIFT = 3;

    static bool IsDeltaEncodeable(uint32_t startDelta, uint32_t length) {
        MOZ_ASSERT(length != 0);
        return startDelta <= ENC4_START_DELTA_MAX && length <= ENC4_LENGTH_MAX;
    }

    static const uint32_t MAX_RUN_LENGTH = 100;

    static uint32_t ExpectedRunLength(const NativeToTrackedOptimizations *start,
                                      const NativeToTrackedOptimizations *end);

    static void ReadDelta(CompactBufferReader &reader, uint32_t *startDelta, uint32_t *length,
                          uint8_t *index);
    static void WriteDelta(CompactBufferWriter &writer, uint32_t startDelta, uint32_t length,
                           uint8_t index);
    static bool WriteRun(CompactBufferWriter &writer,
                         const NativeToTrackedOptimizations *start,
                         const NativeToTrackedOptimizations *end,
                         const UniqueTrackedOptimizations &unique);
};

class IonTrackedOptimizationsAttempts
{
    const uint8_t *start_;
    const uint8_t *end_;

  public:
    IonTrackedOptimizationsAttempts(const uint8_t *start, const uint8_t *end)
      : start_(start), end_(end)
    {
        
        MOZ_ASSERT(start < end);
    }

    void forEach(JS::ForEachTrackedOptimizationAttemptOp &op);
};

struct IonTrackedTypeWithAddendum
{
    TypeSet::Type type;

    enum HasAddendum {
        HasNothing,
        HasAllocationSite,
        HasConstructor
    };
    HasAddendum hasAddendum;

    
    
    
    union {
        struct {
            JSScript *script;
            uint32_t offset;
        };
        JSFunction *constructor;
    };

    explicit IonTrackedTypeWithAddendum(TypeSet::Type type)
      : type(type),
        hasAddendum(HasNothing)
    { }

    IonTrackedTypeWithAddendum(TypeSet::Type type, JSScript *script, uint32_t offset)
      : type(type),
        hasAddendum(HasAllocationSite),
        script(script),
        offset(offset)
    { }

    IonTrackedTypeWithAddendum(TypeSet::Type type, JSFunction *constructor)
      : type(type),
        hasAddendum(HasConstructor),
        constructor(constructor)
    { }

    bool hasAllocationSite() const { return hasAddendum == HasAllocationSite; }
    bool hasConstructor() const { return hasAddendum == HasConstructor; }
};

typedef Vector<IonTrackedTypeWithAddendum, 1, SystemAllocPolicy> IonTrackedTypeVector;

class IonTrackedOptimizationsTypeInfo
{
    const uint8_t *start_;
    const uint8_t *end_;

  public:
    IonTrackedOptimizationsTypeInfo(const uint8_t *start, const uint8_t *end)
      : start_(start), end_(end)
    {
        
    }

    bool empty() const { return start_ == end_; }

    
    
    
    
    
    
    struct ForEachOp
    {
        virtual void readType(const IonTrackedTypeWithAddendum &tracked) = 0;
        virtual void operator()(JS::TrackedTypeSite site, MIRType mirType) = 0;
    };

    class ForEachOpAdapter : public ForEachOp
    {
        JS::ForEachTrackedOptimizationTypeInfoOp &op_;

      public:
        explicit ForEachOpAdapter(JS::ForEachTrackedOptimizationTypeInfoOp &op)
          : op_(op)
        { }

        void readType(const IonTrackedTypeWithAddendum &tracked) MOZ_OVERRIDE;
        void operator()(JS::TrackedTypeSite site, MIRType mirType) MOZ_OVERRIDE;
    };

    void forEach(ForEachOp &op, const IonTrackedTypeVector *allTypes);
};

template <class Entry>
class IonTrackedOptimizationsOffsetsTable
{
    uint32_t padding_;
    uint32_t numEntries_;
    uint32_t entryOffsets_[1];

  protected:
    const uint8_t *payloadEnd() const {
        return (uint8_t *)(this) - padding_;
    }

  public:
    uint32_t numEntries() const { return numEntries_; }
    uint32_t entryOffset(uint32_t index) const {
        MOZ_ASSERT(index < numEntries());
        return entryOffsets_[index];
    }

    Entry entry(uint32_t index) const {
        const uint8_t *start = payloadEnd() - entryOffset(index);
        const uint8_t *end = payloadEnd();
        if (index < numEntries() - 1)
            end -= entryOffset(index + 1);
        return Entry(start, end);
    }
};

class IonTrackedOptimizationsRegionTable
  : public IonTrackedOptimizationsOffsetsTable<IonTrackedOptimizationsRegion>
{
  public:
    mozilla::Maybe<IonTrackedOptimizationsRegion> findRegion(uint32_t offset) const;

    const uint8_t *payloadStart() const { return payloadEnd() - entryOffset(0); }
};

typedef IonTrackedOptimizationsOffsetsTable<IonTrackedOptimizationsAttempts>
    IonTrackedOptimizationsAttemptsTable;

typedef IonTrackedOptimizationsOffsetsTable<IonTrackedOptimizationsTypeInfo>
    IonTrackedOptimizationsTypesTable;

bool
WriteIonTrackedOptimizationsTable(JSContext *cx, CompactBufferWriter &writer,
                                  const NativeToTrackedOptimizations *start,
                                  const NativeToTrackedOptimizations *end,
                                  const UniqueTrackedOptimizations &unique,
                                  uint32_t *numRegions, uint32_t *regionTableOffsetp,
                                  uint32_t *typesTableOffsetp, uint32_t *attemptsTableOffsetp,
                                  IonTrackedTypeVector *allTypes);

} 
} 

#endif 
