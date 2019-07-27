





#include "jit/arm64/Assembler-arm64.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"

#include "jscompartment.h"
#include "jsutil.h"

#include "gc/Marking.h"

#include "jit/arm64/MacroAssembler-arm64.h"
#include "jit/ExecutableAllocator.h"
#include "jit/JitCompartment.h"

using namespace js;
using namespace js::jit;

using mozilla::CountLeadingZeroes32;
using mozilla::DebugOnly;




ABIArg
ABIArgGenerator::next(MIRType type)
{
    switch (type) {
      case MIRType_Int32:
      case MIRType_Pointer:
        if (intRegIndex_ == NumIntArgRegs) {
            current_ = ABIArg(stackOffset_);
            stackOffset_ += sizeof(uintptr_t);
            break;
        }
        current_ = ABIArg(Register::FromCode(intRegIndex_));
        intRegIndex_++;
        break;

      case MIRType_Float32:
      case MIRType_Double:
        if (floatRegIndex_ == NumFloatArgRegs) {
            current_ = ABIArg(stackOffset_);
            stackOffset_ += sizeof(double);
            break;
        }
        current_ = ABIArg(FloatRegister(floatRegIndex_,
                                        type == MIRType_Double ? FloatRegisters::Double
                                                               : FloatRegisters::Single));
        floatRegIndex_++;
        break;

      default:
        MOZ_CRASH("Unexpected argument type");
    }
    return current_;
}

const Register ABIArgGenerator::NonArgReturnReg0 = r8;
const Register ABIArgGenerator::NonArgReturnReg1 = r9;
const Register ABIArgGenerator::NonVolatileReg = r1;
const Register ABIArgGenerator::NonArg_VolatileReg = r13;
const Register ABIArgGenerator::NonReturn_VolatileReg0 = r2;
const Register ABIArgGenerator::NonReturn_VolatileReg1 = r3;

namespace js {
namespace jit {

void
Assembler::finish()
{
    armbuffer_.flushPool();

    
    ExtendedJumpTable_ = emitExtendedJumpTable();
    Assembler::FinalizeCode();

    
    
    if (tmpJumpRelocations_.length())
        jumpRelocations_.writeFixedUint32_t(toFinalOffset(ExtendedJumpTable_));

    for (unsigned int i = 0; i < tmpJumpRelocations_.length(); i++) {
        JumpRelocation& reloc = tmpJumpRelocations_[i];

        
        jumpRelocations_.writeUnsigned(toFinalOffset(reloc.jump));
        jumpRelocations_.writeUnsigned(reloc.extendedTableIndex);
    }

    for (unsigned int i = 0; i < tmpDataRelocations_.length(); i++)
        dataRelocations_.writeUnsigned(toFinalOffset(tmpDataRelocations_[i]));

    for (unsigned int i = 0; i < tmpPreBarriers_.length(); i++)
        preBarriers_.writeUnsigned(toFinalOffset(tmpPreBarriers_[i]));
}

BufferOffset
Assembler::emitExtendedJumpTable()
{
    if (!pendingJumps_.length() || oom())
        return BufferOffset();

    armbuffer_.flushPool();
    armbuffer_.align(SizeOfJumpTableEntry);

    BufferOffset tableOffset = armbuffer_.nextOffset();

    for (size_t i = 0; i < pendingJumps_.length(); i++) {
        
        
        
        
        
        DebugOnly<size_t> preOffset = size_t(armbuffer_.nextOffset().getOffset());

        ldr(vixl::ip0, ptrdiff_t(8 / vixl::kInstructionSize));
        br(vixl::ip0);

        DebugOnly<size_t> prePointer = size_t(armbuffer_.nextOffset().getOffset());
        MOZ_ASSERT(prePointer - preOffset == OffsetOfJumpTableEntryPointer);

        brk(0x0);
        brk(0x0);

        DebugOnly<size_t> postOffset = size_t(armbuffer_.nextOffset().getOffset());

        MOZ_ASSERT(postOffset - preOffset == SizeOfJumpTableEntry);
    }

    return tableOffset;
}

void
Assembler::executableCopy(uint8_t* buffer)
{
    
    armbuffer_.executableCopy(buffer);

    
    
    for (size_t i = 0; i < pendingJumps_.length(); i++) {
        RelativePatch& rp = pendingJumps_[i];

        if (!rp.target) {
            
            
            
            continue;
        }

        Instruction* target = (Instruction*)rp.target;
        Instruction* branch = (Instruction*)(buffer + toFinalOffset(rp.offset));
        JumpTableEntry* extendedJumpTable =
            reinterpret_cast<JumpTableEntry*>(buffer + toFinalOffset(ExtendedJumpTable_));
        if (branch->BranchType() != vixl::UnknownBranchType) {
            if (branch->IsTargetReachable(target)) {
                branch->SetImmPCOffsetTarget(target);
            } else {
                JumpTableEntry* entry = &extendedJumpTable[i];
                branch->SetImmPCOffsetTarget(entry->getLdr());
                entry->data = target;
            }
        } else {
            
            
        }
    }
}

BufferOffset
Assembler::immPool(ARMRegister dest, uint8_t* value, vixl::LoadLiteralOp op, ARMBuffer::PoolEntry* pe)
{
    uint32_t inst = op | Rt(dest);
    const size_t numInst = 1;
    const unsigned sizeOfPoolEntryInBytes = 4;
    const unsigned numPoolEntries = sizeof(value) / sizeOfPoolEntryInBytes;
    return armbuffer_.allocEntry(numInst, numPoolEntries, (uint8_t*)&inst, value, pe);
}

BufferOffset
Assembler::immPool64(ARMRegister dest, uint64_t value, ARMBuffer::PoolEntry* pe)
{
    return immPool(dest, (uint8_t*)&value, vixl::LDR_x_lit, pe);
}

BufferOffset
Assembler::immPool64Branch(RepatchLabel* label, ARMBuffer::PoolEntry* pe, Condition c)
{
    MOZ_CRASH("immPool64Branch");
}

BufferOffset
Assembler::fImmPool(ARMFPRegister dest, uint8_t* value, vixl::LoadLiteralOp op)
{
    uint32_t inst = op | Rt(dest);
    const size_t numInst = 1;
    const unsigned sizeOfPoolEntryInBits = 32;
    const unsigned numPoolEntries = dest.size() / sizeOfPoolEntryInBits;
    return armbuffer_.allocEntry(numInst, numPoolEntries, (uint8_t*)&inst, value);
}

BufferOffset
Assembler::fImmPool64(ARMFPRegister dest, double value)
{
    return fImmPool(dest, (uint8_t*)&value, vixl::LDR_d_lit);
}
BufferOffset
Assembler::fImmPool32(ARMFPRegister dest, float value)
{
    return fImmPool(dest, (uint8_t*)&value, vixl::LDR_s_lit);
}

void
Assembler::bind(Label* label, BufferOffset targetOffset)
{
    
    if (!label->used()) {
        label->bind(targetOffset.getOffset());
        return;
    }

    
    
    uint32_t branchOffset = label->offset();

    while ((int32_t)branchOffset != LabelBase::INVALID_OFFSET) {
        Instruction* link = getInstructionAt(BufferOffset(branchOffset));

        
        
        uint32_t nextLinkOffset = uint32_t(link->ImmPCRawOffset());
        if (nextLinkOffset != uint32_t(LabelBase::INVALID_OFFSET))
            nextLinkOffset += branchOffset;
        
        
        
        
        
        ptrdiff_t relativeByteOffset = targetOffset.getOffset() - branchOffset;
        Instruction* target = (Instruction*)(((uint8_t*)link) + relativeByteOffset);

        
        link->SetImmPCOffsetTarget(target);
        branchOffset = nextLinkOffset;
    }

    
    label->bind(targetOffset.getOffset());
}

void
Assembler::bind(RepatchLabel* label)
{
    
    if (!label->used()) {
        label->bind(nextOffset().getOffset());
        return;
    }
    int branchOffset = label->offset();
    Instruction* inst = getInstructionAt(BufferOffset(branchOffset));
    inst->SetImmPCOffsetTarget(inst + nextOffset().getOffset() - branchOffset);
}

void
Assembler::trace(JSTracer* trc)
{
    for (size_t i = 0; i < pendingJumps_.length(); i++) {
        RelativePatch& rp = pendingJumps_[i];
        if (rp.kind == Relocation::JITCODE) {
            JitCode* code = JitCode::FromExecutable((uint8_t*)rp.target);
            TraceManuallyBarrieredEdge(trc, &code, "masmrel32");
            MOZ_ASSERT(code == JitCode::FromExecutable((uint8_t*)rp.target));
        }
    }

    
#if 0
    if (tmpDataRelocations_.length())
        ::TraceDataRelocations(trc, &armbuffer_, &tmpDataRelocations_);
#endif
}

void
Assembler::addJumpRelocation(BufferOffset src, Relocation::Kind reloc)
{
    
    MOZ_ASSERT(reloc == Relocation::JITCODE);

    
    tmpJumpRelocations_.append(JumpRelocation(src, pendingJumps_.length()));
}

void
Assembler::addPendingJump(BufferOffset src, ImmPtr target, Relocation::Kind reloc)
{
    MOZ_ASSERT(target.value != nullptr);

    if (reloc == Relocation::JITCODE)
        addJumpRelocation(src, reloc);

    
    
    
    enoughMemory_ &= pendingJumps_.append(RelativePatch(src, target.value, reloc));
}

size_t
Assembler::addPatchableJump(BufferOffset src, Relocation::Kind reloc)
{
    MOZ_CRASH("TODO: This is currently unused (and untested)");
    if (reloc == Relocation::JITCODE)
        addJumpRelocation(src, reloc);

    size_t extendedTableIndex = pendingJumps_.length();
    enoughMemory_ &= pendingJumps_.append(RelativePatch(src, nullptr, reloc));
    return extendedTableIndex;
}

void
PatchJump(CodeLocationJump& jump_, CodeLocationLabel label, ReprotectCode reprotect)
{
    MOZ_CRASH("PatchJump");
}

void
Assembler::PatchDataWithValueCheck(CodeLocationLabel label, PatchedImmPtr newValue,
                                   PatchedImmPtr expected)
{
    Instruction* i = (Instruction*)label.raw();
    void** pValue = i->LiteralAddress<void**>();
    MOZ_ASSERT(*pValue == expected.value);
    *pValue = newValue.value;
}

void
Assembler::PatchDataWithValueCheck(CodeLocationLabel label, ImmPtr newValue, ImmPtr expected)
{
    PatchDataWithValueCheck(label, PatchedImmPtr(newValue.value), PatchedImmPtr(expected.value));
}

void
Assembler::ToggleToJmp(CodeLocationLabel inst_)
{
    Instruction* i = (Instruction*)inst_.raw();
    MOZ_ASSERT(i->IsAddSubImmediate());

    
    int imm19 = (int)i->Bits(23, 5);
    MOZ_ASSERT(vixl::is_int19(imm19));

    b(i, imm19, Always);
}

void
Assembler::ToggleToCmp(CodeLocationLabel inst_)
{
    Instruction* i = (Instruction*)inst_.raw();
    MOZ_ASSERT(i->IsCondB());

    int imm19 = i->ImmCondBranch();
    
    
    MOZ_ASSERT(vixl::is_int18(imm19));

    
    
    
    
    
    
    

    
    Emit(i, vixl::ThirtyTwoBits | vixl::AddSubImmediateFixed | vixl::SUB | Flags(vixl::SetFlags) |
            Rd(vixl::xzr) | (imm19 << vixl::Rn_offset));
}

void
Assembler::ToggleCall(CodeLocationLabel inst_, bool enabled)
{
    Instruction* first = (Instruction*)inst_.raw();
    Instruction* load;
    Instruction* call;

    if (first->InstructionBits() == 0x9100039f) {
        load = (Instruction*)NextInstruction(first);
        call = NextInstruction(load);
    } else {
        load = first;
        call = NextInstruction(first);
    }

    if (call->IsBLR() == enabled)
        return;

    if (call->IsBLR()) {
        
        
        
        
        
        
        int32_t offset = load->ImmLLiteral();
        adr(load, xzr, int32_t(offset));
        nop(call);
    } else {
        
        
        
        
        

        int32_t offset = (int)load->ImmPCRawOffset();
        MOZ_ASSERT(vixl::is_int19(offset));
        ldr(load, ScratchReg2_64, int32_t(offset));
        blr(call, ScratchReg2_64);
    }
}

class RelocationIterator
{
    CompactBufferReader reader_;
    uint32_t tableStart_;
    uint32_t offset_;
    uint32_t extOffset_;

  public:
    explicit RelocationIterator(CompactBufferReader& reader)
      : reader_(reader)
    {
        
        tableStart_ = reader_.readFixedUint32_t();
    }

    bool read() {
        if (!reader_.more())
            return false;
        offset_ = reader_.readUnsigned();
        extOffset_ = reader_.readUnsigned();
        return true;
    }

    uint32_t offset() const {
        return offset_;
    }
    uint32_t extendedOffset() const {
        return extOffset_;
    }
};

static JitCode*
CodeFromJump(JitCode* code, uint8_t* jump)
{
    Instruction* branch = (Instruction*)jump;
    uint8_t* target;
    
    if (branch->BranchType() == vixl::UnknownBranchType)
        target = (uint8_t*)branch->Literal64();
    else
        target = (uint8_t*)branch->ImmPCOffsetTarget();

    
    if (target >= code->raw() && target < code->raw() + code->instructionsSize()) {
        MOZ_ASSERT(target + Assembler::SizeOfJumpTableEntry <= code->raw() + code->instructionsSize());

        uint8_t** patchablePtr = (uint8_t**)(target + Assembler::OffsetOfJumpTableEntryPointer);
        target = *patchablePtr;
    }

    return JitCode::FromExecutable(target);
}

void
Assembler::TraceJumpRelocations(JSTracer* trc, JitCode* code, CompactBufferReader& reader)
{
    RelocationIterator iter(reader);
    while (iter.read()) {
        JitCode* child = CodeFromJump(code, code->raw() + iter.offset());
        TraceManuallyBarrieredEdge(trc, &child, "rel32");
        MOZ_ASSERT(child == CodeFromJump(code, code->raw() + iter.offset()));
    }
}

static void
TraceDataRelocations(JSTracer* trc, uint8_t* buffer, CompactBufferReader& reader)
{
    while (reader.more()) {
        size_t offset = reader.readUnsigned();
        Instruction* load = (Instruction*)&buffer[offset];

        
        
        MOZ_ASSERT(load->Mask(vixl::LoadLiteralMask) == vixl::LDR_x_lit);

        uintptr_t* literalAddr = load->LiteralAddress<uintptr_t*>();
        uintptr_t literal = *literalAddr;

        
        
        if (literal >> JSVAL_TAG_SHIFT) {
            jsval_layout layout;
            layout.asBits = literal;
            Value v = IMPL_TO_JSVAL(layout);
            TraceManuallyBarrieredEdge(trc, &v, "ion-masm-value");
            *literalAddr = JSVAL_TO_IMPL(v).asBits;

            
            continue;
        }

        
        TraceManuallyBarrieredGenericPointerEdge(trc, reinterpret_cast<gc::Cell**>(literalAddr),
                                                 "ion-masm-ptr");

        
    }
}

void
Assembler::TraceDataRelocations(JSTracer* trc, JitCode* code, CompactBufferReader& reader)
{
    ::TraceDataRelocations(trc, code->raw(), reader);
}

void
Assembler::FixupNurseryObjects(JSContext* cx, JitCode* code, CompactBufferReader& reader,
                               const ObjectVector& nurseryObjects)
{

    MOZ_ASSERT(!nurseryObjects.empty());

    uint8_t* buffer = code->raw();
    bool hasNurseryPointers = false;

    while (reader.more()) {
        size_t offset = reader.readUnsigned();
        Instruction* ins = (Instruction*)&buffer[offset];

        uintptr_t* literalAddr = ins->LiteralAddress<uintptr_t*>();
        uintptr_t literal = *literalAddr;

        if (literal >> JSVAL_TAG_SHIFT)
            continue; 

        if (!(literal & 0x1))
            continue;

        uint32_t index = literal >> 1;
        JSObject* obj = nurseryObjects[index];
        *literalAddr = uintptr_t(obj);

        
        MOZ_ASSERT_IF(hasNurseryPointers, IsInsideNursery(obj));

        if (!hasNurseryPointers && IsInsideNursery(obj))
            hasNurseryPointers = true;
    }

    if (hasNurseryPointers)
        cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(code);
}

int32_t
Assembler::ExtractCodeLabelOffset(uint8_t* code)
{
    return *(int32_t*)code;
}

void
Assembler::PatchInstructionImmediate(uint8_t* code, PatchedImmPtr imm)
{
    MOZ_CRASH("PatchInstructionImmediate()");
}

void
Assembler::UpdateBoundsCheck(uint32_t heapSize, Instruction* inst)
{
    int32_t mask = ~(heapSize - 1);
    unsigned n, imm_s, imm_r;
    if (!IsImmLogical(mask, 32, &n, &imm_s, &imm_r))
        MOZ_CRASH("Could not encode immediate!?");

    inst->SetImmR(imm_r);
    inst->SetImmS(imm_s);
    inst->SetBitN(n);
}

void
Assembler::retarget(Label* label, Label* target)
{
    if (label->used()) {
        if (target->bound()) {
            bind(label, BufferOffset(target));
        } else if (target->used()) {
            
            
            BufferOffset labelBranchOffset(label);
            BufferOffset next;

            
            while (nextLink(labelBranchOffset, &next))
                labelBranchOffset = next;

            
            
            Instruction* branch = getInstructionAt(labelBranchOffset);
            target->use(label->offset());
            branch->SetImmPCOffsetTarget(branch - labelBranchOffset.getOffset());
        } else {
            
            
            DebugOnly<uint32_t> prev = target->use(label->offset());
            MOZ_ASSERT((int32_t)prev == Label::INVALID_OFFSET);
        }
    }
    label->reset();
}

} 
} 
