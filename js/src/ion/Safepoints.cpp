








































#include "Safepoints.h"
#include "IonSpewer.h"

using namespace js;
using namespace ion;

bool
SafepointWriter::init(uint32 localSlotCount)
{
    frameSlots_ = BitSet::New(localSlotCount);
    if (!frameSlots_)
        return false;

    return true;
}

uint32
SafepointWriter::startEntry()
{
    IonSpew(IonSpew_Safepoints, "Encoding safepoint (position %d):", stream_.length());
    return uint32(stream_.length());
}

void
SafepointWriter::writeOsiCallPointOffset(uint32 osiCallPointOffset)
{
    stream_.writeUnsigned(osiCallPointOffset);
}

static void
WriteRegisterMask(CompactBufferWriter &stream, uint32 bits)
{
    if (sizeof(PackedRegisterMask) == 8)
        stream.writeByte(bits);
    else
        stream.writeUnsigned(bits);
}

void
SafepointWriter::writeGcRegs(GeneralRegisterSet gc, GeneralRegisterSet spilled)
{
    WriteRegisterMask(stream_, spilled.bits());
    if (!spilled.empty())
        WriteRegisterMask(stream_, gc.bits());

    
    JS_ASSERT((gc.bits() & ~spilled.bits()) == 0);

    if (IonSpewEnabled(IonSpew_Safepoints)) {
        for (GeneralRegisterIterator iter(spilled); iter.more(); iter++) {
            const char *type = gc.has(*iter) ? "gc" : "any";
            IonSpew(IonSpew_Safepoints, "    %s reg: %s", type, (*iter).name());
        }
    }
}

static void
MapSlotsToBitset(BitSet *set, CompactBufferWriter &stream, uint32 nslots, uint32 *slots)
{
    set->clear();

    for (uint32 i = 0; i < nslots; i++) {
        
        
        
        set->insert(slots[i] - 1);
    }

    size_t count = set->rawLength();
    uint32 *words = set->raw();
    for (size_t i = 0; i < count; i++)
        stream.writeUnsigned(words[i]);
}

void
SafepointWriter::writeGcSlots(uint32 nslots, uint32 *slots)
{
#ifdef DEBUG
    for (uint32 i = 0; i < nslots; i++)
        IonSpew(IonSpew_Safepoints, "    gc slot: %d", slots[i]);
#endif

    MapSlotsToBitset(frameSlots_, stream_, nslots, slots);
}

void
SafepointWriter::writeValueSlots(uint32 nslots, uint32 *slots)
{
#ifdef DEBUG
    for (uint32 i = 0; i < nslots; i++)
        IonSpew(IonSpew_Safepoints, "    gc value: %d", slots[i]);
#endif

    MapSlotsToBitset(frameSlots_, stream_, nslots, slots);
}

#ifdef DEBUG
static void
DumpNunboxPart(const LAllocation &a)
{
    if (a.isStackSlot()) {
        fprintf(IonSpewFile, "stack %d", a.toStackSlot()->slot());
    } else if (a.isArgument()) {
        fprintf(IonSpewFile, "arg %d", a.toArgument()->index());
    } else {
        fprintf(IonSpewFile, "reg %s", a.toGeneralReg()->reg().name());
    }
}
#endif 


















enum NunboxPartKind {
    Part_Reg,
    Part_Stack,
    Part_Arg
};

static const uint32 PART_KIND_BITS = 3;
static const uint32 PART_KIND_MASK = (1 << PART_KIND_BITS) - 1;
static const uint32 PART_INFO_BITS = 5;
static const uint32 PART_INFO_MASK = (1 << PART_INFO_BITS) - 1;

static const uint32 MAX_INFO_VALUE = 1; 
static const uint32 TYPE_KIND_SHIFT = 16 - PART_KIND_BITS;
static const uint32 PAYLOAD_KIND_SHIFT = TYPE_KIND_SHIFT - PART_KIND_BITS;
static const uint32 TYPE_INFO_SHIFT = PAYLOAD_KIND_SHIFT - PART_INFO_BITS;
static const uint32 PAYLOAD_INFO_SHIFT = TYPE_INFO_SHIFT - PART_INFO_BITS;

JS_STATIC_ASSERT(PAYLOAD_INFO_SHIFT == 0);

static inline NunboxPartKind
AllocationToPartKind(const LAllocation &a)
{
    if (a.isRegister())
        return Part_Reg;
    if (a.isStackSlot())
        return Part_Stack;
    JS_ASSERT(a.isArgument());
    return Part_Arg;
}

static inline bool
CanEncodeInfoInHeader(const LAllocation &a, uint32 *out)
{
    if (a.isGeneralReg()) {
        *out = a.toGeneralReg()->reg().code();
        return true;
    }

    if (a.isStackSlot())
        *out = a.toStackSlot()->slot();
    else
        *out = a.toArgument()->index();

    return *out < MAX_INFO_VALUE;
}

void
SafepointWriter::writeNunboxParts(uint32 nentries, SafepointNunboxEntry *entries)
{
#ifdef DEBUG
    if (IonSpewEnabled(IonSpew_Safepoints)) {
        for (uint32 i = 0; i < nentries; i++) {
            IonSpewHeader(IonSpew_Safepoints);
            fprintf(IonSpewFile, "    nunbox (type in ");
            DumpNunboxPart(entries[i].type);
            fprintf(IonSpewFile, ", payload in ");
            DumpNunboxPart(entries[i].payload);
            fprintf(IonSpewFile, ")\n");
        }
    }
#endif

    stream_.writeUnsigned(nentries);

    for (size_t i = 0; i < nentries; i++) {
        SafepointNunboxEntry &entry = entries[i];

        uint16 header = 0;

        header |= (AllocationToPartKind(entry.type) << TYPE_KIND_SHIFT);
        header |= (AllocationToPartKind(entry.payload) << PAYLOAD_KIND_SHIFT);

        uint32 typeVal;
        bool typeExtra = !CanEncodeInfoInHeader(entry.type, &typeVal);
        if (!typeExtra)
            header |= (typeVal << TYPE_INFO_SHIFT);
        else
            header |= (MAX_INFO_VALUE << TYPE_INFO_SHIFT);

        uint32 payloadVal;
        bool payloadExtra = !CanEncodeInfoInHeader(entry.payload, &payloadVal);
        if (!payloadExtra)
            header |= (payloadVal << PAYLOAD_INFO_SHIFT);
        else
            header |= (MAX_INFO_VALUE << PAYLOAD_INFO_SHIFT);

        stream_.writeFixedUint16(header);
        if (typeExtra)
            stream_.writeUnsigned(typeVal);
        if (payloadExtra)
            stream_.writeUnsigned(payloadVal);
    }
}

void
SafepointWriter::endEntry()
{
    IonSpew(IonSpew_Safepoints, "    -- entry ended at %d", uint32(stream_.length()));
}

SafepointReader::SafepointReader(IonScript *script, const SafepointIndex *si)
  : stream_(script->safepoints() + si->safepointOffset(),
            script->safepoints() + script->safepointsSize()),
    localSlotCount_(script->frameLocals())
{
    osiCallPointOffset_ = stream_.readUnsigned();

    
    allSpills_ = GeneralRegisterSet(stream_.readUnsigned());
    if (allSpills_.empty())
        gcSpills_ = allSpills_;
    else
        gcSpills_ = GeneralRegisterSet(stream_.readUnsigned());

    advanceFromGcRegs();
}

uint32
SafepointReader::osiReturnPointOffset() const
{
    return osiCallPointOffset_ + Assembler::patchWrite_NearCallSize();
}

CodeLocationLabel
SafepointReader::InvalidationPatchPoint(IonScript *script, const SafepointIndex *si)
{
    SafepointReader reader(script, si);

    return CodeLocationLabel(script->method(), reader.osiCallPointOffset());
}

void
SafepointReader::advanceFromGcRegs()
{
    currentSlotChunkNumber_ = 0;
    currentSlotChunk_ = stream_.readUnsigned();
}

bool
SafepointReader::getSlotFromBitmap(uint32 *slot)
{
    while (currentSlotChunk_ == 0) {
        currentSlotChunkNumber_++;

        
        if (currentSlotChunkNumber_ == BitSet::RawLengthForBits(localSlotCount_))
            return false;

        
        currentSlotChunk_ = stream_.readUnsigned();
    }

    
    
    uint32 bit;
    JS_FLOOR_LOG2(bit, currentSlotChunk_);
    currentSlotChunk_ &= ~(1 << bit);

    
    
    *slot = (currentSlotChunkNumber_ * sizeof(uint32) * 8) + bit + 1;
    return true;
}

bool
SafepointReader::getGcSlot(uint32 *slot)
{
    if (getSlotFromBitmap(slot))
        return true;
    advanceFromGcSlots();
    return false;
}

void
SafepointReader::advanceFromGcSlots()
{
    
    currentSlotChunkNumber_ = 0;
#ifdef JS_NUNBOX32
    currentSlotChunk_ = stream_.readUnsigned();
#endif
}

bool
SafepointReader::getValueSlot(uint32 *slot)
{
#ifdef JS_NUNBOX32
    if (getSlotFromBitmap(slot))
        return true;
    advanceFromValueSlots();
    return false;
#else
    return false;
#endif
}

void
SafepointReader::advanceFromValueSlots()
{
#ifdef JS_NUNBOX32
    nunboxSlotsRemaining_ = stream_.readUnsigned();
#else
    nunboxSlotsRemaining_ = 0;
#endif
}

static inline LAllocation
PartFromStream(CompactBufferReader &stream, NunboxPartKind kind, uint32 info)
{
    if (kind == Part_Reg)
        return LGeneralReg(Register::FromCode(info));

    if (info == MAX_INFO_VALUE)
        info = stream.readUnsigned();

    if (kind == Part_Stack)
        return LStackSlot(info);

    JS_ASSERT(kind == Part_Arg);
    return LArgument(info);
}

bool
SafepointReader::getNunboxSlot(LAllocation *type, LAllocation *payload)
{
    if (!nunboxSlotsRemaining_--)
        return false;

    uint16_t header = stream_.readFixedUint16();
    NunboxPartKind typeKind = (NunboxPartKind)((header >> TYPE_KIND_SHIFT) & PART_KIND_MASK);
    NunboxPartKind payloadKind = (NunboxPartKind)((header >> PAYLOAD_KIND_SHIFT) & PART_KIND_MASK);
    uint32 typeInfo = (header >> TYPE_INFO_SHIFT) & PART_INFO_MASK;
    uint32 payloadInfo = (header >> PAYLOAD_INFO_SHIFT) & PART_INFO_MASK;

    *type = PartFromStream(stream_, typeKind, typeInfo);
    *payload = PartFromStream(stream_, payloadKind, payloadInfo);
    return true;
}

