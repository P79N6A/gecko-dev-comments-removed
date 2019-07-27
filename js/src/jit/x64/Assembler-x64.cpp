





#include "jit/x64/Assembler-x64.h"

#include "gc/Marking.h"

using namespace js;
using namespace js::jit;

ABIArgGenerator::ABIArgGenerator()
  :
#if defined(XP_WIN)
    regIndex_(0),
    stackOffset_(ShadowStackSpace),
#else
    intRegIndex_(0),
    floatRegIndex_(0),
    stackOffset_(0),
#endif
    current_()
{}

ABIArg
ABIArgGenerator::next(MIRType type)
{
#if defined(XP_WIN)
    JS_STATIC_ASSERT(NumIntArgRegs == NumFloatArgRegs);
    if (regIndex_ == NumIntArgRegs) {
        if (IsSimdType(type)) {
            
            
            
            stackOffset_ = AlignBytes(stackOffset_, SimdStackAlignment);
            current_ = ABIArg(stackOffset_);
            stackOffset_ += Simd128DataSize;
        } else {
            current_ = ABIArg(stackOffset_);
            stackOffset_ += sizeof(uint64_t);
        }
        return current_;
    }
    switch (type) {
      case MIRType_Int32:
      case MIRType_Pointer:
        current_ = ABIArg(IntArgRegs[regIndex_++]);
        break;
      case MIRType_Float32:
      case MIRType_Double:
        current_ = ABIArg(FloatArgRegs[regIndex_++]);
        break;
      case MIRType_Int32x4:
      case MIRType_Float32x4:
        
        
        
        current_ = ABIArg(FloatArgRegs[regIndex_++]);
        break;
      default:
        MOZ_CRASH("Unexpected argument type");
    }
    return current_;
#else
    switch (type) {
      case MIRType_Int32:
      case MIRType_Pointer:
        if (intRegIndex_ == NumIntArgRegs) {
            current_ = ABIArg(stackOffset_);
            stackOffset_ += sizeof(uint64_t);
            break;
        }
        current_ = ABIArg(IntArgRegs[intRegIndex_++]);
        break;
      case MIRType_Double:
      case MIRType_Float32:
        if (floatRegIndex_ == NumFloatArgRegs) {
            current_ = ABIArg(stackOffset_);
            stackOffset_ += sizeof(uint64_t);
            break;
        }
        current_ = ABIArg(FloatArgRegs[floatRegIndex_++]);
        break;
      case MIRType_Int32x4:
      case MIRType_Float32x4:
        if (floatRegIndex_ == NumFloatArgRegs) {
            stackOffset_ = AlignBytes(stackOffset_, SimdStackAlignment);
            current_ = ABIArg(stackOffset_);
            stackOffset_ += Simd128DataSize;
            break;
        }
        current_ = ABIArg(FloatArgRegs[floatRegIndex_++]);
        break;
      default:
        MOZ_CRASH("Unexpected argument type");
    }
    return current_;
#endif
}


const Register ABIArgGenerator::NonArgReturnReg0 = r10;
const Register ABIArgGenerator::NonArgReturnReg1 = r12;
const Register ABIArgGenerator::NonVolatileReg = r13;
const Register ABIArgGenerator::NonArg_VolatileReg = rax;
const Register ABIArgGenerator::NonReturn_VolatileReg0 = rcx;

void
Assembler::writeRelocation(JmpSrc src, Relocation::Kind reloc)
{
    if (!jumpRelocations_.length()) {
        
        
        
        
        jumpRelocations_.writeFixedUint32_t(0);
    }
    if (reloc == Relocation::JITCODE) {
        jumpRelocations_.writeUnsigned(src.offset());
        jumpRelocations_.writeUnsigned(jumps_.length());
    }
}

void
Assembler::addPendingJump(JmpSrc src, ImmPtr target, Relocation::Kind reloc)
{
    JS_ASSERT(target.value != nullptr);

    
    
    if (reloc == Relocation::JITCODE)
        writeRelocation(src, reloc);
    enoughMemory_ &= jumps_.append(RelativePatch(src.offset(), target.value, reloc));
}

size_t
Assembler::addPatchableJump(JmpSrc src, Relocation::Kind reloc)
{
    
    
    writeRelocation(src, reloc);

    size_t index = jumps_.length();
    enoughMemory_ &= jumps_.append(RelativePatch(src.offset(), nullptr, reloc));
    return index;
}


uint8_t *
Assembler::PatchableJumpAddress(JitCode *code, size_t index)
{
    
    
    uint32_t jumpOffset = * (uint32_t *) code->jumpRelocTable();
    jumpOffset += index * SizeOfJumpTableEntry;

    JS_ASSERT(jumpOffset + SizeOfExtendedJump <= code->instructionsSize());
    return code->raw() + jumpOffset;
}


void
Assembler::PatchJumpEntry(uint8_t *entry, uint8_t *target)
{
    uint8_t **index = (uint8_t **) (entry + SizeOfExtendedJump - sizeof(void*));
    *index = target;
}

void
Assembler::finish()
{
    if (!jumps_.length() || oom())
        return;

    
    masm.align(SizeOfJumpTableEntry);
    extendedJumpTable_ = masm.size();

    
    
    
    JS_ASSERT_IF(jumpRelocations_.length(), jumpRelocations_.length() >= sizeof(uint32_t));
    if (jumpRelocations_.length())
        *(uint32_t *)jumpRelocations_.buffer() = extendedJumpTable_;

    
    for (size_t i = 0; i < jumps_.length(); i++) {
#ifdef DEBUG
        size_t oldSize = masm.size();
#endif
        masm.jmp_rip(2);
        JS_ASSERT(masm.size() - oldSize == 6);
        
        
        masm.ud2();
        JS_ASSERT(masm.size() - oldSize == 8);
        masm.immediate64(0);
        JS_ASSERT(masm.size() - oldSize == SizeOfExtendedJump);
        JS_ASSERT(masm.size() - oldSize == SizeOfJumpTableEntry);
    }
}

void
Assembler::executableCopy(uint8_t *buffer)
{
    AssemblerX86Shared::executableCopy(buffer);

    for (size_t i = 0; i < jumps_.length(); i++) {
        RelativePatch &rp = jumps_[i];
        uint8_t *src = buffer + rp.offset;
        if (!rp.target) {
            
            
            
            continue;
        }
        if (X86Assembler::canRelinkJump(src, rp.target)) {
            X86Assembler::setRel32(src, rp.target);
        } else {
            
            
            JS_ASSERT(extendedJumpTable_);
            JS_ASSERT((extendedJumpTable_ + i * SizeOfJumpTableEntry) <= size() - SizeOfJumpTableEntry);

            
            uint8_t *entry = buffer + extendedJumpTable_ + i * SizeOfJumpTableEntry;
            X86Assembler::setRel32(src, entry);

            
            
            X86Assembler::repatchPointer(entry + SizeOfExtendedJump, rp.target);
        }
    }
}

class RelocationIterator
{
    CompactBufferReader reader_;
    uint32_t tableStart_;
    uint32_t offset_;
    uint32_t extOffset_;

  public:
    explicit RelocationIterator(CompactBufferReader &reader)
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

JitCode *
Assembler::CodeFromJump(JitCode *code, uint8_t *jump)
{
    uint8_t *target = (uint8_t *)X86Assembler::getRel32Target(jump);
    if (target >= code->raw() && target < code->raw() + code->instructionsSize()) {
        
        
        JS_ASSERT(target + SizeOfJumpTableEntry <= code->raw() + code->instructionsSize());

        target = (uint8_t *)X86Assembler::getPointer(target + SizeOfExtendedJump);
    }

    return JitCode::FromExecutable(target);
}

void
Assembler::TraceJumpRelocations(JSTracer *trc, JitCode *code, CompactBufferReader &reader)
{
    RelocationIterator iter(reader);
    while (iter.read()) {
        JitCode *child = CodeFromJump(code, code->raw() + iter.offset());
        MarkJitCodeUnbarriered(trc, &child, "rel32");
        JS_ASSERT(child == CodeFromJump(code, code->raw() + iter.offset()));
    }
}

FloatRegisterSet
FloatRegister::ReduceSetForPush(const FloatRegisterSet &s)
{
    return s;
}
uint32_t
FloatRegister::GetSizeInBytes(const FloatRegisterSet &s)
{
    uint32_t ret = s.size() * sizeof(double);
    return ret;
}
uint32_t
FloatRegister::GetPushSizeInBytes(const FloatRegisterSet &s)
{
    return s.size() * sizeof(double);
}
uint32_t
FloatRegister::getRegisterDumpOffsetInBytes()
{
    return code() * sizeof(double);
}
