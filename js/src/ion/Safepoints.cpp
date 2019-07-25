








































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
SafepointWriter::writeOsiReturnPointOffset(uint32 osiReturnPointOffset)
{
    stream_.writeUnsigned(osiReturnPointOffset);
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
SafepointWriter::writeGcRegs(GeneralRegisterSet actual, GeneralRegisterSet spilled)
{
    WriteRegisterMask(stream_, actual.bits());
    if (!actual.empty())
        WriteRegisterMask(stream_, spilled.bits());

#ifdef DEBUG
    for (AnyRegisterIterator iter(actual, FloatRegisterSet()); iter.more(); iter++)
        IonSpew(IonSpew_Safepoints, "    gc reg: %s", (*iter).name());
#endif
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
            fprintf(IonSpewFile, ")");
        }
    }
#endif

    if (nentries) {
        
        return;
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
}

CodeLocationLabel
SafepointReader::InvalidationPatchPoint(IonScript *script, const SafepointIndex *si)
{
    SafepointReader reader(script, si);

    
    
    
    
    uint32 osiPointOffset = reader.getOsiReturnPointOffset() - Assembler::patchWrite_NearCallSize();
    return CodeLocationLabel(script->method(), osiPointOffset);
}

uint32
SafepointReader::getOsiReturnPointOffset()
{
    return stream_.readUnsigned();
}

void
SafepointReader::getGcRegs(GeneralRegisterSet *actual, GeneralRegisterSet *spilled)
{
    *actual = GeneralRegisterSet(stream_.readUnsigned());
    if (actual->empty())
        *spilled = *actual;
    else
        *spilled = GeneralRegisterSet(stream_.readUnsigned());

    advanceFromGcRegs();
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
}

