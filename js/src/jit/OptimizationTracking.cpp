





#include "jit/OptimizationTracking.h"

#include "mozilla/SizePrintfMacros.h"

#include "jsprf.h"

#include "ds/Sort.h"
#include "jit/IonBuilder.h"
#include "jit/JitcodeMap.h"
#include "jit/JitSpewer.h"
#include "js/TrackedOptimizationInfo.h"

using namespace js;
using namespace js::jit;

using mozilla::Maybe;
using mozilla::Some;
using mozilla::Nothing;

using JS::TrackedStrategy;
using JS::TrackedOutcome;
using JS::TrackedTypeSite;
using JS::ForEachTrackedOptimizationAttemptOp;
using JS::ForEachTrackedOptimizationTypeInfoOp;

bool
TrackedOptimizations::trackTypeInfo(OptimizationTypeInfo&& ty)
{
    return types_.append(mozilla::Move(ty));
}

bool
TrackedOptimizations::trackAttempt(TrackedStrategy strategy)
{
    OptimizationAttempt attempt(strategy, TrackedOutcome::GenericFailure);
    currentAttempt_ = attempts_.length();
    return attempts_.append(attempt);
}

void
TrackedOptimizations::amendAttempt(uint32_t index)
{
    currentAttempt_ = index;
}

void
TrackedOptimizations::trackOutcome(TrackedOutcome outcome)
{
    attempts_[currentAttempt_].setOutcome(outcome);
}

void
TrackedOptimizations::trackSuccess()
{
    attempts_[currentAttempt_].setOutcome(TrackedOutcome::GenericSuccess);
}

template <class Vec>
static bool
VectorContentsMatch(const Vec* xs, const Vec* ys)
{
    if (xs->length() != ys->length())
        return false;
    for (auto x = xs->begin(), y = ys->begin(); x != xs->end(); x++, y++) {
        MOZ_ASSERT(y != ys->end());
        if (*x != *y)
            return false;
    }
    return true;
}

bool
TrackedOptimizations::matchTypes(const TempOptimizationTypeInfoVector& other) const
{
    return VectorContentsMatch(&types_, &other);
}

bool
TrackedOptimizations::matchAttempts(const TempOptimizationAttemptsVector& other) const
{
    return VectorContentsMatch(&attempts_, &other);
}

JS_PUBLIC_API(const char*)
JS::TrackedStrategyString(TrackedStrategy strategy)
{
    switch (strategy) {
#define STRATEGY_CASE(name)                       \
      case TrackedStrategy::name:                 \
        return #name;
    TRACKED_STRATEGY_LIST(STRATEGY_CASE)
#undef STRATEGY_CASE

      default:
        MOZ_CRASH("bad strategy");
    }
}

JS_PUBLIC_API(const char*)
JS::TrackedOutcomeString(TrackedOutcome outcome)
{
    switch (outcome) {
#define OUTCOME_CASE(name)                        \
      case TrackedOutcome::name:                  \
        return #name;
      TRACKED_OUTCOME_LIST(OUTCOME_CASE)
#undef OUTCOME_CASE

      default:
        MOZ_CRASH("bad outcome");
    }
}

JS_PUBLIC_API(const char*)
JS::TrackedTypeSiteString(TrackedTypeSite site)
{
    switch (site) {
#define TYPESITE_CASE(name)                       \
      case TrackedTypeSite::name:                 \
        return #name;
      TRACKED_TYPESITE_LIST(TYPESITE_CASE)
#undef TYPESITE_CASE

      default:
        MOZ_CRASH("bad type site");
    }
}

void
SpewTempOptimizationTypeInfoVector(const TempOptimizationTypeInfoVector* types,
                                   const char* indent = nullptr)
{
#ifdef DEBUG
    for (const OptimizationTypeInfo* t = types->begin(); t != types->end(); t++) {
        JitSpewStart(JitSpew_OptimizationTracking, "   %s%s of type %s, type set",
                     indent ? indent : "",
                     TrackedTypeSiteString(t->site()), StringFromMIRType(t->mirType()));
        for (uint32_t i = 0; i < t->types().length(); i++)
            JitSpewCont(JitSpew_OptimizationTracking, " %s", TypeSet::TypeString(t->types()[i]));
        JitSpewFin(JitSpew_OptimizationTracking);
    }
#endif
}

void
SpewTempOptimizationAttemptsVector(const TempOptimizationAttemptsVector* attempts,
                                   const char* indent = nullptr)
{
#ifdef DEBUG
    for (const OptimizationAttempt* a = attempts->begin(); a != attempts->end(); a++) {
        JitSpew(JitSpew_OptimizationTracking, "   %s%s: %s", indent ? indent : "",
                TrackedStrategyString(a->strategy()), TrackedOutcomeString(a->outcome()));
    }
#endif
}

void
TrackedOptimizations::spew() const
{
#ifdef DEBUG
    SpewTempOptimizationTypeInfoVector(&types_);
    SpewTempOptimizationAttemptsVector(&attempts_);
#endif
}

bool
OptimizationTypeInfo::trackTypeSet(TemporaryTypeSet* typeSet)
{
    if (!typeSet)
        return true;
    return typeSet->enumerateTypes(&types_);
}

bool
OptimizationTypeInfo::trackType(TypeSet::Type type)
{
    return types_.append(type);
}

bool
OptimizationTypeInfo::operator ==(const OptimizationTypeInfo& other) const
{
    return site_ == other.site_ && mirType_ == other.mirType_ &&
           VectorContentsMatch(&types_, &other.types_);
}

bool
OptimizationTypeInfo::operator !=(const OptimizationTypeInfo& other) const
{
    return !(*this == other);
}

static inline HashNumber
CombineHash(HashNumber h, HashNumber n)
{
    h += n;
    h += (h << 10);
    h ^= (h >> 6);
    return h;
}

static inline HashNumber
HashType(TypeSet::Type ty)
{
    if (ty.isObjectUnchecked())
        return PointerHasher<TypeSet::ObjectKey*, 3>::hash(ty.objectKey());
    return HashNumber(ty.raw());
}

static HashNumber
HashTypeList(const TempTypeList& types)
{
    HashNumber h = 0;
    for (uint32_t i = 0; i < types.length(); i++)
        h = CombineHash(h, HashType(types[i]));
    return h;
}

HashNumber
OptimizationTypeInfo::hash() const
{
    return ((HashNumber(site_) << 24) + (HashNumber(mirType_) << 16)) ^ HashTypeList(types_);
}

template <class Vec>
static HashNumber
HashVectorContents(const Vec* xs, HashNumber h)
{
    for (auto x = xs->begin(); x != xs->end(); x++)
        h = CombineHash(h, x->hash());
    return h;
}

 HashNumber
UniqueTrackedOptimizations::Key::hash(const Lookup& lookup)
{
    HashNumber h = HashVectorContents(lookup.types, 0);
    h = HashVectorContents(lookup.attempts, h);
    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);
    return h;
}

 bool
UniqueTrackedOptimizations::Key::match(const Key& key, const Lookup& lookup)
{
    return VectorContentsMatch(key.attempts, lookup.attempts) &&
           VectorContentsMatch(key.types, lookup.types);
}

bool
UniqueTrackedOptimizations::add(const TrackedOptimizations* optimizations)
{
    MOZ_ASSERT(!sorted());
    Key key;
    key.types = &optimizations->types_;
    key.attempts = &optimizations->attempts_;
    AttemptsMap::AddPtr p = map_.lookupForAdd(key);
    if (p) {
        p->value().frequency++;
        return true;
    }
    Entry entry;
    entry.index = UINT8_MAX;
    entry.frequency = 1;
    return map_.add(p, key, entry);
}

struct FrequencyComparator
{
    bool operator()(const UniqueTrackedOptimizations::SortEntry& a,
                    const UniqueTrackedOptimizations::SortEntry& b,
                    bool* lessOrEqualp)
    {
        *lessOrEqualp = b.frequency <= a.frequency;
        return true;
    }
};

bool
UniqueTrackedOptimizations::sortByFrequency(JSContext* cx)
{
    MOZ_ASSERT(!sorted());

    JitSpew(JitSpew_OptimizationTracking, "=> Sorting unique optimizations by frequency");

    
    Vector<SortEntry> entries(cx);
    for (AttemptsMap::Range r = map_.all(); !r.empty(); r.popFront()) {
        SortEntry entry;
        entry.types = r.front().key().types;
        entry.attempts = r.front().key().attempts;
        entry.frequency = r.front().value().frequency;
        if (!entries.append(entry))
            return false;
    }

    
    
    if (entries.length() >= UINT8_MAX - 1)
        return false;

    Vector<SortEntry> scratch(cx);
    if (!scratch.resize(entries.length()))
        return false;

    FrequencyComparator comparator;
    MOZ_ALWAYS_TRUE(MergeSort(entries.begin(), entries.length(), scratch.begin(), comparator));

    
    for (size_t i = 0; i < entries.length(); i++) {
        Key key;
        key.types = entries[i].types;
        key.attempts = entries[i].attempts;
        AttemptsMap::Ptr p = map_.lookup(key);
        MOZ_ASSERT(p);
        p->value().index = sorted_.length();

        JitSpew(JitSpew_OptimizationTracking, "   Entry %u has frequency %u",
                sorted_.length(), p->value().frequency);

        if (!sorted_.append(entries[i]))
            return false;
    }

    return true;
}

uint8_t
UniqueTrackedOptimizations::indexOf(const TrackedOptimizations* optimizations) const
{
    MOZ_ASSERT(sorted());
    Key key;
    key.types = &optimizations->types_;
    key.attempts = &optimizations->attempts_;
    AttemptsMap::Ptr p = map_.lookup(key);
    MOZ_ASSERT(p);
    MOZ_ASSERT(p->value().index != UINT8_MAX);
    return p->value().index;
}


class jit::UniqueTrackedTypes
{
  public:
    struct TypeHasher
    {
        typedef TypeSet::Type Lookup;

        static HashNumber hash(const Lookup& ty) { return HashType(ty); }
        static bool match(const TypeSet::Type& ty1, const TypeSet::Type& ty2) { return ty1 == ty2; }
    };

  private:
    
    typedef HashMap<TypeSet::Type, uint8_t, TypeHasher> TypesMap;
    TypesMap map_;

    Vector<TypeSet::Type, 1> list_;

  public:
    explicit UniqueTrackedTypes(JSContext* cx)
      : map_(cx),
        list_(cx)
    { }

    bool init() { return map_.init(); }
    bool getIndexOf(TypeSet::Type ty, uint8_t* indexp);

    uint32_t count() const { MOZ_ASSERT(map_.count() == list_.length()); return list_.length(); }
    bool enumerate(TypeSet::TypeList* types) const;
};

bool
UniqueTrackedTypes::getIndexOf(TypeSet::Type ty, uint8_t* indexp)
{
    TypesMap::AddPtr p = map_.lookupForAdd(ty);
    if (p) {
        *indexp = p->value();
        return true;
    }

    
    
    if (count() >= UINT8_MAX)
        return false;

    uint8_t index = (uint8_t) count();
    if (!map_.add(p, ty, index))
        return false;
    if (!list_.append(ty))
        return false;
    *indexp = index;
    return true;
}

bool
UniqueTrackedTypes::enumerate(TypeSet::TypeList* types) const
{
    return types->append(list_.begin(), list_.end());
}

void
IonTrackedOptimizationsRegion::unpackHeader()
{
    CompactBufferReader reader(start_, end_);
    startOffset_ = reader.readUnsigned();
    endOffset_ = reader.readUnsigned();
    rangesStart_ = reader.currentPosition();
    MOZ_ASSERT(startOffset_ < endOffset_);
}

void
IonTrackedOptimizationsRegion::RangeIterator::readNext(uint32_t* startOffset, uint32_t* endOffset,
                                                       uint8_t* index)
{
    MOZ_ASSERT(more());

    CompactBufferReader reader(cur_, end_);

    
    if (cur_ == start_) {
        *startOffset = firstStartOffset_;
        *endOffset = prevEndOffset_ = reader.readUnsigned();
        *index = reader.readByte();
        cur_ = reader.currentPosition();
        MOZ_ASSERT(cur_ <= end_);
        return;
    }

    
    uint32_t startDelta, length;
    ReadDelta(reader, &startDelta, &length, index);
    *startOffset = prevEndOffset_ + startDelta;
    *endOffset = prevEndOffset_ = *startOffset + length;
    cur_ = reader.currentPosition();
    MOZ_ASSERT(cur_ <= end_);
}

Maybe<uint8_t>
JitcodeGlobalEntry::IonEntry::trackedOptimizationIndexAtAddr(void* ptr, uint32_t* entryOffsetOut)
{
    MOZ_ASSERT(hasTrackedOptimizations());
    MOZ_ASSERT(containsPointer(ptr));
    uint32_t ptrOffset = ((uint8_t*) ptr) - ((uint8_t*) nativeStartAddr());
    Maybe<IonTrackedOptimizationsRegion> region = optsRegionTable_->findRegion(ptrOffset);
    if (region.isNothing())
        return Nothing();
    return region->findIndex(ptrOffset, entryOffsetOut);
}

void
IonTrackedOptimizationsAttempts::forEach(ForEachTrackedOptimizationAttemptOp& op)
{
    CompactBufferReader reader(start_, end_);
    const uint8_t* cur = start_;
    while (cur != end_) {
        TrackedStrategy strategy = TrackedStrategy(reader.readUnsigned());
        TrackedOutcome outcome = TrackedOutcome(reader.readUnsigned());
        MOZ_ASSERT(strategy < TrackedStrategy::Count);
        MOZ_ASSERT(outcome < TrackedOutcome::Count);
        op(strategy, outcome);
        cur = reader.currentPosition();
        MOZ_ASSERT(cur <= end_);
    }
}

void
IonTrackedOptimizationsTypeInfo::forEach(ForEachOp& op, const IonTrackedTypeVector* allTypes)
{
    CompactBufferReader reader(start_, end_);
    const uint8_t* cur = start_;
    while (cur != end_) {
        TrackedTypeSite site = JS::TrackedTypeSite(reader.readUnsigned());
        MOZ_ASSERT(site < JS::TrackedTypeSite::Count);
        MIRType mirType = MIRType(reader.readUnsigned());
        uint32_t length = reader.readUnsigned();
        for (uint32_t i = 0; i < length; i++)
            op.readType((*allTypes)[reader.readByte()]);
        op(site, mirType);
        cur = reader.currentPosition();
        MOZ_ASSERT(cur <= end_);
    }
}

Maybe<uint8_t>
IonTrackedOptimizationsRegion::findIndex(uint32_t offset, uint32_t* entryOffsetOut) const
{
    if (offset <= startOffset_ || offset > endOffset_)
        return Nothing();

    
    RangeIterator iter = ranges();
    while (iter.more()) {
        uint32_t startOffset, endOffset;
        uint8_t index;
        iter.readNext(&startOffset, &endOffset, &index);
        if (startOffset < offset && offset <= endOffset) {
            *entryOffsetOut = endOffset;
            return Some(index);
        }
    }
    return Nothing();
}

Maybe<IonTrackedOptimizationsRegion>
IonTrackedOptimizationsRegionTable::findRegion(uint32_t offset) const
{
    
    
    
    
    

    static const uint32_t LINEAR_SEARCH_THRESHOLD = 8;
    uint32_t regions = numEntries();
    MOZ_ASSERT(regions > 0);

    
    if (regions <= LINEAR_SEARCH_THRESHOLD) {
        for (uint32_t i = 0; i < regions; i++) {
            IonTrackedOptimizationsRegion region = entry(i);
            if (region.startOffset() < offset && offset <= region.endOffset()) {
                return Some(entry(i));
            }
        }
        return Nothing();
    }

    
    uint32_t i = 0;
    while (regions > 1) {
        uint32_t step = regions / 2;
        uint32_t mid = i + step;
        IonTrackedOptimizationsRegion region = entry(mid);

        if (offset <= region.startOffset()) {
            
            regions = step;
        } else if (offset > region.endOffset()) {
            
            i = mid;
            regions -= step;
        } else {
            
            return Some(entry(i));
        }
    }
    return Nothing();
}

 uint32_t
IonTrackedOptimizationsRegion::ExpectedRunLength(const NativeToTrackedOptimizations* start,
                                                 const NativeToTrackedOptimizations* end)
{
    MOZ_ASSERT(start < end);

    
    uint32_t runLength = 1;
    uint32_t prevEndOffset = start->endOffset.offset();

    for (const NativeToTrackedOptimizations* entry = start + 1; entry != end; entry++) {
        uint32_t startOffset = entry->startOffset.offset();
        uint32_t endOffset = entry->endOffset.offset();
        uint32_t startDelta = startOffset - prevEndOffset;
        uint32_t length = endOffset - startOffset;

        if (!IsDeltaEncodeable(startDelta, length))
            break;

        runLength++;
        if (runLength == MAX_RUN_LENGTH)
            break;

        prevEndOffset = endOffset;
    }

    return runLength;
}

void
OptimizationAttempt::writeCompact(CompactBufferWriter& writer) const
{
    writer.writeUnsigned((uint32_t) strategy_);
    writer.writeUnsigned((uint32_t) outcome_);
}

bool
OptimizationTypeInfo::writeCompact(CompactBufferWriter& writer,
                              UniqueTrackedTypes& uniqueTypes) const
{
    writer.writeUnsigned((uint32_t) site_);
    writer.writeUnsigned((uint32_t) mirType_);
    writer.writeUnsigned(types_.length());
    for (uint32_t i = 0; i < types_.length(); i++) {
        uint8_t index;
        if (!uniqueTypes.getIndexOf(types_[i], &index))
            return false;
        writer.writeByte(index);
    }
    return true;
}

 void
IonTrackedOptimizationsRegion::ReadDelta(CompactBufferReader& reader,
                                         uint32_t* startDelta, uint32_t* length,
                                         uint8_t* index)
{
    
    
    const uint32_t firstByte = reader.readByte();
    const uint32_t secondByte = reader.readByte();
    if ((firstByte & ENC1_MASK) == ENC1_MASK_VAL) {
        uint32_t encVal = firstByte | secondByte << 8;
        *startDelta = encVal >> ENC1_START_DELTA_SHIFT;
        *length = (encVal >> ENC1_LENGTH_SHIFT) & ENC1_LENGTH_MAX;
        *index = (encVal >> ENC1_INDEX_SHIFT) & ENC1_INDEX_MAX;
        MOZ_ASSERT(length != 0);
        return;
    }

    
    
    const uint32_t thirdByte = reader.readByte();
    if ((firstByte & ENC2_MASK) == ENC2_MASK_VAL) {
        uint32_t encVal = firstByte | secondByte << 8 | thirdByte << 16;
        *startDelta = encVal >> ENC2_START_DELTA_SHIFT;
        *length = (encVal >> ENC2_LENGTH_SHIFT) & ENC2_LENGTH_MAX;
        *index = (encVal >> ENC2_INDEX_SHIFT) & ENC2_INDEX_MAX;
        MOZ_ASSERT(length != 0);
        return;
    }

    
    
    const uint32_t fourthByte = reader.readByte();
    if ((firstByte & ENC3_MASK) == ENC3_MASK_VAL) {
        uint32_t encVal = firstByte | secondByte << 8 | thirdByte << 16 | fourthByte << 24;
        *startDelta = encVal >> ENC3_START_DELTA_SHIFT;
        *length = (encVal >> ENC3_LENGTH_SHIFT) & ENC3_LENGTH_MAX;
        *index = (encVal >> ENC3_INDEX_SHIFT) & ENC3_INDEX_MAX;
        MOZ_ASSERT(length != 0);
        return;
    }

    
    
    MOZ_ASSERT((firstByte & ENC4_MASK) == ENC4_MASK_VAL);
    uint64_t fifthByte = reader.readByte();
    uint64_t encVal = firstByte | secondByte << 8 | thirdByte << 16 | fourthByte << 24 |
                      fifthByte << 32;
    *startDelta = encVal >> ENC4_START_DELTA_SHIFT;
    *length = (encVal >> ENC4_LENGTH_SHIFT) & ENC4_LENGTH_MAX;
    *index = (encVal >> ENC4_INDEX_SHIFT) & ENC4_INDEX_MAX;
    MOZ_ASSERT(length != 0);
}

 void
IonTrackedOptimizationsRegion::WriteDelta(CompactBufferWriter& writer,
                                          uint32_t startDelta, uint32_t length,
                                          uint8_t index)
{
    
    
    if (startDelta <= ENC1_START_DELTA_MAX &&
        length <= ENC1_LENGTH_MAX &&
        index <= ENC1_INDEX_MAX)
    {
        uint16_t val = ENC1_MASK_VAL |
                       (startDelta << ENC1_START_DELTA_SHIFT) |
                       (length << ENC1_LENGTH_SHIFT) |
                       (index << ENC1_INDEX_SHIFT);
        writer.writeByte(val & 0xff);
        writer.writeByte((val >> 8) & 0xff);
        return;
    }

    
    
    if (startDelta <= ENC2_START_DELTA_MAX &&
        length <= ENC2_LENGTH_MAX &&
        index <= ENC2_INDEX_MAX)
    {
        uint32_t val = ENC2_MASK_VAL |
                       (startDelta << ENC2_START_DELTA_SHIFT) |
                       (length << ENC2_LENGTH_SHIFT) |
                       (index << ENC2_INDEX_SHIFT);
        writer.writeByte(val & 0xff);
        writer.writeByte((val >> 8) & 0xff);
        writer.writeByte((val >> 16) & 0xff);
        return;
    }

    
    
    if (startDelta <= ENC3_START_DELTA_MAX &&
        length <= ENC3_LENGTH_MAX)
    {
        
        
        MOZ_ASSERT(ENC3_INDEX_MAX == UINT8_MAX);
        uint32_t val = ENC3_MASK_VAL |
                       (startDelta << ENC3_START_DELTA_SHIFT) |
                       (length << ENC3_LENGTH_SHIFT) |
                       (index << ENC3_INDEX_SHIFT);
        writer.writeByte(val & 0xff);
        writer.writeByte((val >> 8) & 0xff);
        writer.writeByte((val >> 16) & 0xff);
        writer.writeByte((val >> 24) & 0xff);
        return;
    }

    
    
    if (startDelta <= ENC4_START_DELTA_MAX &&
        length <= ENC4_LENGTH_MAX)
    {
        
        
        MOZ_ASSERT(ENC4_INDEX_MAX == UINT8_MAX);
        uint64_t val = ENC4_MASK_VAL |
                       (((uint64_t) startDelta) << ENC4_START_DELTA_SHIFT) |
                       (((uint64_t) length) << ENC4_LENGTH_SHIFT) |
                       (((uint64_t) index) << ENC4_INDEX_SHIFT);
        writer.writeByte(val & 0xff);
        writer.writeByte((val >> 8) & 0xff);
        writer.writeByte((val >> 16) & 0xff);
        writer.writeByte((val >> 24) & 0xff);
        writer.writeByte((val >> 32) & 0xff);
        return;
    }

    MOZ_CRASH("startDelta,length,index triple too large to encode.");
}

 bool
IonTrackedOptimizationsRegion::WriteRun(CompactBufferWriter& writer,
                                        const NativeToTrackedOptimizations* start,
                                        const NativeToTrackedOptimizations* end,
                                        const UniqueTrackedOptimizations& unique)
{
    
    JitSpew(JitSpew_OptimizationTracking, "     Header: [%u, %u]",
            start->startOffset.offset(), (end - 1)->endOffset.offset());
    writer.writeUnsigned(start->startOffset.offset());
    writer.writeUnsigned((end - 1)->endOffset.offset());

    
    JitSpew(JitSpew_OptimizationTracking,
            "     [%6u, %6u]                        vector %3u, offset %4u",
            start->startOffset.offset(), start->endOffset.offset(),
            unique.indexOf(start->optimizations), writer.length());
    uint32_t prevEndOffset = start->endOffset.offset();
    writer.writeUnsigned(prevEndOffset);
    writer.writeByte(unique.indexOf(start->optimizations));

    
    for (const NativeToTrackedOptimizations* entry = start + 1; entry != end; entry++) {
        uint32_t startOffset = entry->startOffset.offset();
        uint32_t endOffset = entry->endOffset.offset();

        uint32_t startDelta = startOffset - prevEndOffset;
        uint32_t length = endOffset - startOffset;
        uint8_t index = unique.indexOf(entry->optimizations);

        JitSpew(JitSpew_OptimizationTracking,
                "     [%6u, %6u] delta [+%5u, +%5u] vector %3u, offset %4u",
                startOffset, endOffset, startDelta, length, index, writer.length());

        WriteDelta(writer, startDelta, length, index);

        prevEndOffset = endOffset;
    }

    if (writer.oom())
        return false;

    return true;
}

static bool
WriteOffsetsTable(CompactBufferWriter& writer, const Vector<uint32_t, 16>& offsets,
                  uint32_t* tableOffsetp)
{
    
    uint32_t padding = sizeof(uint32_t) - (writer.length() % sizeof(uint32_t));
    if (padding == sizeof(uint32_t))
        padding = 0;
    JitSpew(JitSpew_OptimizationTracking, "   Padding %u byte%s",
            padding, padding == 1 ? "" : "s");
    for (uint32_t i = 0; i < padding; i++)
        writer.writeByte(0);

    
    uint32_t tableOffset = writer.length();

    
    writer.writeNativeEndianUint32_t(padding);
    writer.writeNativeEndianUint32_t(offsets.length());

    
    for (size_t i = 0; i < offsets.length(); i++) {
        JitSpew(JitSpew_OptimizationTracking, "   Entry %u reverse offset %u",
                i, tableOffset - padding - offsets[i]);
        writer.writeNativeEndianUint32_t(tableOffset - padding - offsets[i]);
    }

    if (writer.oom())
        return false;

    *tableOffsetp = tableOffset;
    return true;
}

static JSFunction*
MaybeConstructorFromType(TypeSet::Type ty)
{
    if (ty.isUnknown() || ty.isAnyObject() || !ty.isGroup())
        return nullptr;
    ObjectGroup* obj = ty.group();
    TypeNewScript* newScript = obj->newScript();
    if (!newScript && obj->maybeUnboxedLayout())
        newScript = obj->unboxedLayout().newScript();
    return newScript ? newScript->function() : nullptr;
}

static void
SpewConstructor(TypeSet::Type ty, JSFunction* constructor)
{
#ifdef DEBUG
    if (!constructor->isInterpreted()) {
        JitSpew(JitSpew_OptimizationTracking, "   Unique type %s has native constructor",
                TypeSet::TypeString(ty));
        return;
    }

    char buf[512];
    if (constructor->displayAtom())
        PutEscapedString(buf, 512, constructor->displayAtom(), 0);
    else
        JS_snprintf(buf, mozilla::ArrayLength(buf), "??");

    const char* filename;
    size_t lineno;
    if (constructor->hasScript()) {
        filename = constructor->nonLazyScript()->filename();
        lineno = constructor->nonLazyScript()->lineno();
    } else {
        filename = constructor->lazyScript()->filename();
        lineno = constructor->lazyScript()->lineno();
    }

    JitSpew(JitSpew_OptimizationTracking, "   Unique type %s has constructor %s (%s:%" PRIuSIZE ")",
            TypeSet::TypeString(ty), buf, filename, lineno);
#endif
}

static void
SpewAllocationSite(TypeSet::Type ty, JSScript* script, uint32_t offset)
{
#ifdef DEBUG
    JitSpew(JitSpew_OptimizationTracking, "   Unique type %s has alloc site %s:%u",
            TypeSet::TypeString(ty), script->filename(),
            PCToLineNumber(script, script->offsetToPC(offset)));
#endif
}

bool
jit::WriteIonTrackedOptimizationsTable(JSContext* cx, CompactBufferWriter& writer,
                                       const NativeToTrackedOptimizations* start,
                                       const NativeToTrackedOptimizations* end,
                                       const UniqueTrackedOptimizations& unique,
                                       uint32_t* numRegions,
                                       uint32_t* regionTableOffsetp,
                                       uint32_t* typesTableOffsetp,
                                       uint32_t* optimizationTableOffsetp,
                                       IonTrackedTypeVector* allTypes)
{
    MOZ_ASSERT(unique.sorted());

#ifdef DEBUG
    
    
    if (JitSpewEnabled(JitSpew_OptimizationTracking)) {
        JitSpewStart(JitSpew_OptimizationTracking, "=> Training data: ");
        for (const NativeToTrackedOptimizations* entry = start; entry != end; entry++) {
            JitSpewCont(JitSpew_OptimizationTracking, "%u,%u,%u ",
                        entry->startOffset.offset(), entry->endOffset.offset(),
                        unique.indexOf(entry->optimizations));
        }
        JitSpewFin(JitSpew_OptimizationTracking);
    }
#endif

    Vector<uint32_t, 16> offsets(cx);
    const NativeToTrackedOptimizations* entry = start;

    
    JitSpew(JitSpew_Profiling, "=> Writing regions");
    while (entry != end) {
        uint32_t runLength = IonTrackedOptimizationsRegion::ExpectedRunLength(entry, end);
        JitSpew(JitSpew_OptimizationTracking, "   Run at entry %u, length %u, offset %u",
                entry - start, runLength, writer.length());

        if (!offsets.append(writer.length()))
            return false;

        if (!IonTrackedOptimizationsRegion::WriteRun(writer, entry, entry + runLength, unique))
            return false;

        entry += runLength;
    }

    
    if (!WriteOffsetsTable(writer, offsets, regionTableOffsetp))
        return false;

    *numRegions = offsets.length();

    
    
    offsets.clear();

    const UniqueTrackedOptimizations::SortedVector& vec = unique.sortedVector();
    JitSpew(JitSpew_OptimizationTracking, "=> Writing unique optimizations table with %u entr%s",
            vec.length(), vec.length() == 1 ? "y" : "ies");

    
    UniqueTrackedTypes uniqueTypes(cx);
    if (!uniqueTypes.init())
        return false;

    for (const UniqueTrackedOptimizations::SortEntry* p = vec.begin(); p != vec.end(); p++) {
        const TempOptimizationTypeInfoVector* v = p->types;
        JitSpew(JitSpew_OptimizationTracking, "   Type info entry %u of length %u, offset %u",
                p - vec.begin(), v->length(), writer.length());
        SpewTempOptimizationTypeInfoVector(v, "  ");

        if (!offsets.append(writer.length()))
            return false;

        for (const OptimizationTypeInfo* t = v->begin(); t != v->end(); t++) {
            if (!t->writeCompact(writer, uniqueTypes))
                return false;
        }
    }

    
    
    
    
    
    TypeSet::TypeList uniqueTypeList;
    if (!uniqueTypes.enumerate(&uniqueTypeList))
        return false;
    for (uint32_t i = 0; i < uniqueTypeList.length(); i++) {
        TypeSet::Type ty = uniqueTypeList[i];
        if (JSFunction* constructor = MaybeConstructorFromType(ty)) {
            if (!allTypes->append(IonTrackedTypeWithAddendum(ty, constructor)))
                return false;
            SpewConstructor(ty, constructor);
        } else {
            JSScript* script;
            uint32_t offset;
            if (!ty.isUnknown() && !ty.isAnyObject() && ty.isGroup() &&
                ObjectGroup::findAllocationSite(cx, ty.group(), &script, &offset))
            {
                if (!allTypes->append(IonTrackedTypeWithAddendum(ty, script, offset)))
                    return false;
                SpewAllocationSite(ty, script, offset);
            } else {
                if (!allTypes->append(IonTrackedTypeWithAddendum(ty)))
                    return false;
            }
        }
    }

    if (!WriteOffsetsTable(writer, offsets, typesTableOffsetp))
        return false;
    offsets.clear();

    
    for (const UniqueTrackedOptimizations::SortEntry* p = vec.begin(); p != vec.end(); p++) {
        const TempOptimizationAttemptsVector* v = p->attempts;
        JitSpew(JitSpew_OptimizationTracking, "   Attempts entry %u of length %u, offset %u",
                p - vec.begin(), v->length(), writer.length());
        SpewTempOptimizationAttemptsVector(v, "  ");

        if (!offsets.append(writer.length()))
            return false;

        for (const OptimizationAttempt* a = v->begin(); a != v->end(); a++)
            a->writeCompact(writer);
    }

    return WriteOffsetsTable(writer, offsets, optimizationTableOffsetp);
}


BytecodeSite*
IonBuilder::maybeTrackedOptimizationSite(jsbytecode* pc)
{
    
    
    
    
    
    
    
    
    
    
    MOZ_ASSERT(isOptimizationTrackingEnabled());
    for (size_t i = trackedOptimizationSites_.length(); i != 0; i--) {
        BytecodeSite* site = trackedOptimizationSites_[i - 1];
        if (site->pc() == pc) {
            MOZ_ASSERT(site->tree() == info().inlineScriptTree());
            return site;
        }
    }
    return nullptr;
}

void
IonBuilder::startTrackingOptimizations()
{
    if (isOptimizationTrackingEnabled()) {
        BytecodeSite* site = maybeTrackedOptimizationSite(current->trackedSite()->pc());

        if (!site) {
            site = current->trackedSite();
            site->setOptimizations(new(alloc()) TrackedOptimizations(alloc()));
            
            if (!trackedOptimizationSites_.append(site))
                site = nullptr;
        } else {
            
            
            
            site->optimizations()->clear();
        }

        if (site)
            current->updateTrackedSite(site);
    }
}

void
IonBuilder::trackTypeInfoUnchecked(TrackedTypeSite kind, MIRType mirType,
                                   TemporaryTypeSet* typeSet)
{
    BytecodeSite* site = current->trackedSite();
    
    OptimizationTypeInfo typeInfo(alloc(), kind, mirType);
    if (!typeInfo.trackTypeSet(typeSet)) {
        site->setOptimizations(nullptr);
        return;
    }
    if (!site->optimizations()->trackTypeInfo(mozilla::Move(typeInfo)))
        site->setOptimizations(nullptr);
}

void
IonBuilder::trackTypeInfoUnchecked(TrackedTypeSite kind, JSObject* obj)
{
    BytecodeSite* site = current->trackedSite();
    
    OptimizationTypeInfo typeInfo(alloc(), kind, MIRType_Object);
    if (!typeInfo.trackType(TypeSet::ObjectType(obj)))
        return;
    if (!site->optimizations()->trackTypeInfo(mozilla::Move(typeInfo)))
        site->setOptimizations(nullptr);
}

void
IonBuilder::trackTypeInfoUnchecked(CallInfo& callInfo)
{
    MDefinition* thisArg = callInfo.thisArg();
    trackTypeInfoUnchecked(TrackedTypeSite::Call_This, thisArg->type(), thisArg->resultTypeSet());

    for (uint32_t i = 0; i < callInfo.argc(); i++) {
        MDefinition* arg = callInfo.getArg(i);
        trackTypeInfoUnchecked(TrackedTypeSite::Call_Arg, arg->type(), arg->resultTypeSet());
    }

    TemporaryTypeSet* returnTypes = getInlineReturnTypeSet();
    trackTypeInfoUnchecked(TrackedTypeSite::Call_Return, returnTypes->getKnownMIRType(),
                           returnTypes);
}

void
IonBuilder::trackOptimizationAttemptUnchecked(TrackedStrategy strategy)
{
    BytecodeSite* site = current->trackedSite();
    
    if (!site->optimizations()->trackAttempt(strategy))
        site->setOptimizations(nullptr);
}

void
IonBuilder::amendOptimizationAttemptUnchecked(uint32_t index)
{
    const BytecodeSite* site = current->trackedSite();
    site->optimizations()->amendAttempt(index);
}

void
IonBuilder::trackOptimizationOutcomeUnchecked(TrackedOutcome outcome)
{
    const BytecodeSite* site = current->trackedSite();
    site->optimizations()->trackOutcome(outcome);
}

void
IonBuilder::trackOptimizationSuccessUnchecked()
{
    const BytecodeSite* site = current->trackedSite();
    site->optimizations()->trackSuccess();
}

void
IonBuilder::trackInlineSuccessUnchecked(InliningStatus status)
{
    if (status == InliningStatus_Inlined)
        trackOptimizationOutcome(TrackedOutcome::Inlined);
}

JS_PUBLIC_API(void)
JS::ForEachTrackedOptimizationAttempt(JSRuntime* rt, void* addr, uint8_t index,
                                      ForEachTrackedOptimizationAttemptOp& op,
                                      JSScript** scriptOut, jsbytecode** pcOut)
{
    JitcodeGlobalTable* table = rt->jitRuntime()->getJitcodeGlobalTable();
    JitcodeGlobalEntry entry;
    table->lookupInfallible(addr, &entry, rt);
    entry.youngestFrameLocationAtAddr(rt, addr, scriptOut, pcOut);
    entry.trackedOptimizationAttempts(index).forEach(op);
}

static void
InterpretedFunctionFilenameAndLineNumber(JSFunction* fun, const char** filename, unsigned* lineno)
{
    ScriptSource* source;
    if (fun->hasScript()) {
        source = fun->nonLazyScript()->maybeForwardedScriptSource();
        *lineno = fun->nonLazyScript()->lineno();
    } else {
        source = fun->lazyScript()->maybeForwardedScriptSource();
        *lineno = fun->lazyScript()->lineno();
    }
    *filename = source->filename();
}

static JSFunction*
FunctionFromTrackedType(const IonTrackedTypeWithAddendum& tracked)
{
    if (tracked.hasConstructor())
        return tracked.constructor;

    TypeSet::Type ty = tracked.type;

    if (ty.isSingleton()) {
        JSObject* obj = ty.singleton();
        return obj->is<JSFunction>() ? &obj->as<JSFunction>() : nullptr;
    }

    return ty.group()->maybeInterpretedFunction();
}

void
IonTrackedOptimizationsTypeInfo::ForEachOpAdapter::readType(const IonTrackedTypeWithAddendum& tracked)
{
    TypeSet::Type ty = tracked.type;

    if (ty.isPrimitive() || ty.isUnknown() || ty.isAnyObject()) {
        op_.readType("primitive", TypeSet::NonObjectTypeString(ty), nullptr, 0);
        return;
    }

    char buf[512];
    const uint32_t bufsize = mozilla::ArrayLength(buf);

    if (JSFunction* fun = FunctionFromTrackedType(tracked)) {
        
        
        char* name = nullptr;
        if (fun->displayAtom()) {
            PutEscapedString(buf, bufsize, fun->displayAtom(), 0);
            name = buf;
        }

        if (fun->isNative()) {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            char locationBuf[20];
            if (!name) {
                uintptr_t addr = JS_FUNC_TO_DATA_PTR(uintptr_t, fun->native());
                JS_snprintf(locationBuf, mozilla::ArrayLength(locationBuf), "%llx", addr);
            }
            op_.readType("native", name, name ? nullptr : locationBuf, UINT32_MAX);
            return;
        }

        const char* filename;
        unsigned lineno;
        InterpretedFunctionFilenameAndLineNumber(fun, &filename, &lineno);
        op_.readType(tracked.constructor ? "constructor" : "function",
                     name, filename, lineno);
        return;
    }

    const char* className = ty.objectKey()->clasp()->name;
    JS_snprintf(buf, bufsize, "[object %s]", className);

    if (tracked.hasAllocationSite()) {
        JSScript* script = tracked.script;
        op_.readType("alloc site", buf,
                     script->maybeForwardedScriptSource()->filename(),
                     PCToLineNumber(script, script->offsetToPC(tracked.offset)));
        return;
    }

    if (ty.isGroup()) {
        op_.readType("prototype", buf, nullptr, UINT32_MAX);
        return;
    }

    op_.readType("singleton", buf, nullptr, UINT32_MAX);
}

void
IonTrackedOptimizationsTypeInfo::ForEachOpAdapter::operator()(JS::TrackedTypeSite site,
                                                              MIRType mirType)
{
    op_(site, StringFromMIRType(mirType));
}

JS_PUBLIC_API(void)
JS::ForEachTrackedOptimizationTypeInfo(JSRuntime* rt, void* addr, uint8_t index,
                                       ForEachTrackedOptimizationTypeInfoOp& op)
{
    JitcodeGlobalTable* table = rt->jitRuntime()->getJitcodeGlobalTable();
    JitcodeGlobalEntry entry;
    table->lookupInfallible(addr, &entry, rt);
    IonTrackedOptimizationsTypeInfo::ForEachOpAdapter adapter(op);
    entry.trackedOptimizationTypeInfo(index).forEach(adapter, entry.allTrackedTypes());
}

JS_PUBLIC_API(Maybe<uint8_t>)
JS::TrackedOptimizationIndexAtAddr(JSRuntime* rt, void* addr, void** entryAddr)
{
    JitcodeGlobalTable* table = rt->jitRuntime()->getJitcodeGlobalTable();
    JitcodeGlobalEntry entry;
    table->lookupInfallible(addr, &entry, rt);
    if (!entry.hasTrackedOptimizations())
        return Nothing();
    uint32_t entryOffset = 0;
    Maybe<uint8_t> index = entry.trackedOptimizationIndexAtAddr(addr, &entryOffset);
    if (index.isSome())
        *entryAddr = (void*)(((uint8_t*) entry.nativeStartAddr()) + entryOffset);
    return index;
}
