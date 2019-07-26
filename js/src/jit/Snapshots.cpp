





#include "jit/Snapshots.h"

#include "jsscript.h"

#include "jit/CompileInfo.h"
#include "jit/IonSpewer.h"
#ifdef TRACK_SNAPSHOTS
# include "jit/LIR.h"
# include "jit/MIR.h"
#endif

using namespace js;
using namespace js::jit;































































































const RValueAllocation::Layout &
RValueAllocation::layoutFromMode(Mode mode)
{
    switch (mode) {
      case CONSTANT: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_INDEX,
            PAYLOAD_NONE,
            "constant"
        };
        return layout;
      }

      case CST_UNDEFINED: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_NONE,
            PAYLOAD_NONE,
            "undefined"
        };
        return layout;
      }

      case CST_NULL: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_NONE,
            PAYLOAD_NONE,
            "null"
        };
        return layout;
      }

      case DOUBLE_REG: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_FPU,
            PAYLOAD_NONE,
            "double"
        };
        return layout;
      }
      case FLOAT32_REG: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_FPU,
            PAYLOAD_NONE,
            "float32"
        };
        return layout;
      }
      case FLOAT32_STACK: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_STACK_OFFSET,
            PAYLOAD_NONE,
            "float32"
        };
        return layout;
      }
#if defined(JS_NUNBOX32)
      case UNTYPED_REG_REG: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_GPR,
            PAYLOAD_GPR,
            "value"
        };
        return layout;
      }
      case UNTYPED_REG_STACK: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_GPR,
            PAYLOAD_STACK_OFFSET,
            "value"
        };
        return layout;
      }
      case UNTYPED_STACK_REG: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_STACK_OFFSET,
            PAYLOAD_GPR
        };
        return layout;
      }
      case UNTYPED_STACK_STACK: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_STACK_OFFSET,
            PAYLOAD_STACK_OFFSET,
            "value"
        };
        return layout;
      }
#elif defined(JS_PUNBOX64)
      case UNTYPED_REG: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_GPR,
            PAYLOAD_NONE,
            "value"
        };
        return layout;
      }
      case UNTYPED_STACK: {
        static const RValueAllocation::Layout layout = {
            PAYLOAD_STACK_OFFSET,
            PAYLOAD_NONE,
            "value"
        };
        return layout;
      }
#endif
      default: {
        static const RValueAllocation::Layout regLayout = {
            PAYLOAD_PACKED_TAG,
            PAYLOAD_GPR,
            "typed value"
        };

        static const RValueAllocation::Layout stackLayout = {
            PAYLOAD_PACKED_TAG,
            PAYLOAD_STACK_OFFSET,
            "typed value"
        };

        if (mode >= TYPED_REG_MIN && mode <= TYPED_REG_MAX)
            return regLayout;
        if (mode >= TYPED_STACK_MIN && mode <= TYPED_STACK_MAX)
            return stackLayout;
      }
    }

    MOZ_ASSUME_UNREACHABLE("Wrong mode type?");
}














static const size_t ALLOCATION_TABLE_ALIGNMENT = 2; 

void
RValueAllocation::readPayload(CompactBufferReader &reader, PayloadType type,
                              uint8_t *mode, Payload *p)
{
    switch (type) {
      case PAYLOAD_NONE:
        break;
      case PAYLOAD_INDEX:
        p->index = reader.readUnsigned();
        break;
      case PAYLOAD_STACK_OFFSET:
        p->stackOffset = reader.readSigned();
        break;
      case PAYLOAD_GPR:
        p->gpr = Register::FromCode(reader.readByte());
        break;
      case PAYLOAD_FPU:
        p->fpu = FloatRegister::FromCode(reader.readByte());
        break;
      case PAYLOAD_PACKED_TAG:
        p->type = JSValueType(*mode & 0x07);
        *mode = *mode & ~0x07;
        break;
    }
}

RValueAllocation
RValueAllocation::read(CompactBufferReader &reader)
{
    uint8_t mode = reader.readByte();
    const Layout &layout = layoutFromMode(Mode(mode));
    Payload arg1, arg2;

    readPayload(reader, layout.type1, &mode, &arg1);
    readPayload(reader, layout.type2, &mode, &arg2);
    return RValueAllocation(Mode(mode), arg1, arg2);
}

void
RValueAllocation::writePayload(CompactBufferWriter &writer, PayloadType type,
                               Payload p)
{
    switch (type) {
      case PAYLOAD_NONE:
        break;
      case PAYLOAD_INDEX:
        writer.writeUnsigned(p.index);
        break;
      case PAYLOAD_STACK_OFFSET:
        writer.writeSigned(p.stackOffset);
        break;
      case PAYLOAD_GPR:
        static_assert(Registers::Total <= 0x100,
                      "Not enough bytes to encode all registers.");
        writer.writeByte(p.gpr.code());
        break;
      case PAYLOAD_FPU:
        static_assert(FloatRegisters::Total <= 0x100,
                      "Not enough bytes to encode all float registers.");
        writer.writeByte(p.fpu.code());
        break;
      case PAYLOAD_PACKED_TAG: {
        
        
        MOZ_ASSERT(writer.length());
        uint8_t *mode = writer.buffer() + (writer.length() - 1);
        MOZ_ASSERT((*mode & 0x07) == 0 && (p.type & ~0x07) == 0);
        *mode = *mode | p.type;
        break;
      }
    }
}

void
RValueAllocation::writePadding(CompactBufferWriter &writer)
{
    
    while (writer.length() % ALLOCATION_TABLE_ALIGNMENT)
        writer.writeByte(0x7f);
}

void
RValueAllocation::write(CompactBufferWriter &writer) const
{
    const Layout &layout = layoutFromMode(mode());
    MOZ_ASSERT(layout.type2 != PAYLOAD_PACKED_TAG);
    MOZ_ASSERT(writer.length() % ALLOCATION_TABLE_ALIGNMENT == 0);

    writer.writeByte(mode_);
    writePayload(writer, layout.type1, arg1_);
    writePayload(writer, layout.type2, arg2_);
    writePadding(writer);
}

HashNumber
RValueAllocation::hash() const {
    CompactBufferWriter writer;
    write(writer);

    
    
    
    MOZ_ASSERT(!writer.oom());
    MOZ_ASSERT(writer.length() <= 12);

    HashNumber res = 0;
    for (size_t i = 0; i < writer.length(); i++) {
        res = ((res << 8) | (res >> (sizeof(res) - 1)));
        res ^= writer.buffer()[i];
    }
    return res;
}

static const char *
ValTypeToString(JSValueType type)
{
    switch (type) {
      case JSVAL_TYPE_INT32:
        return "int32_t";
      case JSVAL_TYPE_DOUBLE:
        return "double";
      case JSVAL_TYPE_STRING:
        return "string";
      case JSVAL_TYPE_BOOLEAN:
        return "boolean";
      case JSVAL_TYPE_OBJECT:
        return "object";
      case JSVAL_TYPE_MAGIC:
        return "magic";
      default:
        MOZ_ASSUME_UNREACHABLE("no payload");
    }
}

void
RValueAllocation::dumpPayload(FILE *fp, PayloadType type, Payload p)
{
    switch (type) {
      case PAYLOAD_NONE:
        break;
      case PAYLOAD_INDEX:
        fprintf(fp, "index %u", p.index);
        break;
      case PAYLOAD_STACK_OFFSET:
        fprintf(fp, "stack %d", p.stackOffset);
        break;
      case PAYLOAD_GPR:
        fprintf(fp, "reg %s", p.gpr.name());
        break;
      case PAYLOAD_FPU:
        fprintf(fp, "reg %s", p.fpu.name());
        break;
      case PAYLOAD_PACKED_TAG:
        fprintf(fp, ValTypeToString(p.type));
        break;
    }
}

void
RValueAllocation::dump(FILE *fp) const
{
    const Layout &layout = layoutFromMode(mode());
    fprintf(fp, "%s", layout.name);

    if (layout.type1 != PAYLOAD_NONE)
        fprintf(fp, " (");
    dumpPayload(fp, layout.type1, arg1_);
    if (layout.type2 != PAYLOAD_NONE)
        fprintf(fp, ", ");
    dumpPayload(fp, layout.type2, arg2_);
    if (layout.type1 != PAYLOAD_NONE)
        fprintf(fp, ")");
}

bool
RValueAllocation::equalPayloads(PayloadType type, Payload lhs, Payload rhs)
{
    switch (type) {
      case PAYLOAD_NONE:
        return true;
      case PAYLOAD_INDEX:
        return lhs.index == rhs.index;
      case PAYLOAD_STACK_OFFSET:
        return lhs.stackOffset == rhs.stackOffset;
      case PAYLOAD_GPR:
        return lhs.gpr == rhs.gpr;
      case PAYLOAD_FPU:
        return lhs.fpu == rhs.fpu;
      case PAYLOAD_PACKED_TAG:
        return lhs.type == rhs.type;
    }

    return false;
}

SnapshotReader::SnapshotReader(const uint8_t *snapshots, uint32_t offset,
                               uint32_t RVATableSize, uint32_t listSize)
  : reader_(snapshots + offset, snapshots + listSize),
    allocReader_(snapshots + listSize, snapshots + listSize + RVATableSize),
    allocTable_(snapshots + listSize),
    allocCount_(0),
    frameCount_(0),
    allocRead_(0)
{
    if (!snapshots)
        return;
    IonSpew(IonSpew_Snapshots, "Creating snapshot reader");
    readSnapshotHeader();
    nextFrame();
}

static const uint32_t BAILOUT_KIND_SHIFT = 0;
static const uint32_t BAILOUT_KIND_MASK = (1 << BAILOUT_KIND_BITS) - 1;
static const uint32_t BAILOUT_RESUME_SHIFT = BAILOUT_KIND_SHIFT + BAILOUT_KIND_BITS;
static const uint32_t BAILOUT_FRAMECOUNT_SHIFT = BAILOUT_KIND_BITS + BAILOUT_RESUME_BITS;
static const uint32_t BAILOUT_FRAMECOUNT_BITS = (8 * sizeof(uint32_t)) - BAILOUT_FRAMECOUNT_SHIFT;

void
SnapshotReader::readSnapshotHeader()
{
    uint32_t bits = reader_.readUnsigned();
    frameCount_ = bits >> BAILOUT_FRAMECOUNT_SHIFT;
    JS_ASSERT(frameCount_ > 0);
    bailoutKind_ = BailoutKind((bits >> BAILOUT_KIND_SHIFT) & BAILOUT_KIND_MASK);
    resumeAfter_ = !!(bits & (1 << BAILOUT_RESUME_SHIFT));
    framesRead_ = 0;

    IonSpew(IonSpew_Snapshots, "Read snapshot header with frameCount %u, bailout kind %u (ra: %d)",
            frameCount_, bailoutKind_, resumeAfter_);
}

void
SnapshotReader::readFrameHeader()
{
    JS_ASSERT(moreFrames());
    JS_ASSERT(allocRead_ == allocCount_);

    pcOffset_ = reader_.readUnsigned();
    allocCount_ = reader_.readUnsigned();
#ifdef TRACK_SNAPSHOTS
    pcOpcode_  = reader_.readUnsigned();
    mirOpcode_ = reader_.readUnsigned();
    mirId_     = reader_.readUnsigned();
    lirOpcode_ = reader_.readUnsigned();
    lirId_     = reader_.readUnsigned();
#endif
    IonSpew(IonSpew_Snapshots, "Read pc offset %u, nslots %u", pcOffset_, allocCount_);

    framesRead_++;
    allocRead_ = 0;
}

#ifdef TRACK_SNAPSHOTS
void
SnapshotReader::spewBailingFrom() const
{
    if (IonSpewEnabled(IonSpew_Bailouts)) {
        IonSpewHeader(IonSpew_Bailouts);
        fprintf(IonSpewFile, " bailing from bytecode: %s, MIR: ", js_CodeName[pcOpcode_]);
        MDefinition::PrintOpcodeName(IonSpewFile, MDefinition::Opcode(mirOpcode_));
        fprintf(IonSpewFile, " [%u], LIR: ", mirId_);
        LInstruction::printName(IonSpewFile, LInstruction::Opcode(lirOpcode_));
        fprintf(IonSpewFile, " [%u]", lirId_);
        fprintf(IonSpewFile, "\n");
    }
}
#endif

RValueAllocation
SnapshotReader::readAllocation()
{
    JS_ASSERT(allocRead_ < allocCount_);
    IonSpew(IonSpew_Snapshots, "Reading slot %u", allocRead_);
    allocRead_++;

    uint32_t offset = reader_.readUnsigned() * ALLOCATION_TABLE_ALIGNMENT;
    allocReader_.seek(allocTable_, offset);
    return RValueAllocation::read(allocReader_);
}

bool
SnapshotWriter::init()
{
    
    
    
    return allocMap_.init(32);
}

SnapshotOffset
SnapshotWriter::startSnapshot(uint32_t frameCount, BailoutKind kind, bool resumeAfter)
{
    nframes_ = frameCount;
    framesWritten_ = 0;

    lastStart_ = writer_.length();

    IonSpew(IonSpew_Snapshots, "starting snapshot with frameCount %u, bailout kind %u",
            frameCount, kind);
    JS_ASSERT(frameCount > 0);
    JS_ASSERT(frameCount < (1 << BAILOUT_FRAMECOUNT_BITS));
    JS_ASSERT(uint32_t(kind) < (1 << BAILOUT_KIND_BITS));

    uint32_t bits = (uint32_t(kind) << BAILOUT_KIND_SHIFT) |
                  (frameCount << BAILOUT_FRAMECOUNT_SHIFT);
    if (resumeAfter)
        bits |= (1 << BAILOUT_RESUME_SHIFT);

    writer_.writeUnsigned(bits);
    return lastStart_;
}

void
SnapshotWriter::startFrame(JSFunction *fun, JSScript *script, jsbytecode *pc, uint32_t exprStack)
{
    
    
    
    JS_ASSERT(CountArgSlots(script, fun) < SNAPSHOT_MAX_NARGS + 4);

    uint32_t implicit = StartArgSlot(script);
    uint32_t formalArgs = CountArgSlots(script, fun);

    nallocs_ = formalArgs + script->nfixed() + exprStack;
    allocWritten_ = 0;

    IonSpew(IonSpew_Snapshots, "Starting frame; implicit %u, formals %u, fixed %u, exprs %u",
            implicit, formalArgs - implicit, script->nfixed(), exprStack);

    uint32_t pcoff = script->pcToOffset(pc);
    IonSpew(IonSpew_Snapshots, "Writing pc offset %u, nslots %u", pcoff, nallocs_);
    writer_.writeUnsigned(pcoff);
    writer_.writeUnsigned(nallocs_);
}

#ifdef TRACK_SNAPSHOTS
void
SnapshotWriter::trackFrame(uint32_t pcOpcode, uint32_t mirOpcode, uint32_t mirId,
                                            uint32_t lirOpcode, uint32_t lirId)
{
    writer_.writeUnsigned(pcOpcode);
    writer_.writeUnsigned(mirOpcode);
    writer_.writeUnsigned(mirId);
    writer_.writeUnsigned(lirOpcode);
    writer_.writeUnsigned(lirId);
}
#endif

bool
SnapshotWriter::add(const RValueAllocation &alloc)
{
    MOZ_ASSERT(allocMap_.initialized());

    uint32_t offset;
    RValueAllocMap::AddPtr p = allocMap_.lookupForAdd(alloc);
    if (!p) {
        offset = allocWriter_.length();
        alloc.write(allocWriter_);
        if (!allocMap_.add(p, alloc, offset))
            return false;
    } else {
        offset = p->value();
    }

    if (IonSpewEnabled(IonSpew_Snapshots)) {
        IonSpewHeader(IonSpew_Snapshots);
        fprintf(IonSpewFile, "    slot %u (%d): ", allocWritten_, offset);
        alloc.dump(IonSpewFile);
        fprintf(IonSpewFile, "\n");
    }

    allocWritten_++;
    JS_ASSERT(allocWritten_ <= nallocs_);
    writer_.writeUnsigned(offset / ALLOCATION_TABLE_ALIGNMENT);
    return true;
}

void
SnapshotWriter::endFrame()
{
    
    JS_ASSERT(nallocs_ == allocWritten_);
    nallocs_ = allocWritten_ = 0;
    framesWritten_++;
}

void
SnapshotWriter::endSnapshot()
{
    JS_ASSERT(nframes_ == framesWritten_);

    
#ifdef DEBUG
    writer_.writeSigned(-1);
#endif
    
    IonSpew(IonSpew_Snapshots, "ending snapshot total size: %u bytes (start %u)",
            uint32_t(writer_.length() - lastStart_), lastStart_);
}
