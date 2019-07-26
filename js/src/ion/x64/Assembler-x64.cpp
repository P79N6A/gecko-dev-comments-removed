






#include "Assembler-x64.h"
#include "gc/Marking.h"
#include "ion/LIR.h"

#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;

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
        current_ = ABIArg(stackOffset_);
        stackOffset_ += sizeof(uint64_t);
        return current_;
    }
    switch (type) {
      case MIRType_Int32:
      case MIRType_Pointer:
        current_ = ABIArg(IntArgRegs[regIndex_++]);
        break;
      case MIRType_Double:
        current_ = ABIArg(FloatArgRegs[regIndex_++]);
        break;
      default:
        JS_NOT_REACHED("Unexpected argument type");
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
        if (floatRegIndex_ == NumFloatArgRegs) {
            current_ = ABIArg(stackOffset_);
            stackOffset_ += sizeof(uint64_t);
            break;
        }
        current_ = ABIArg(FloatArgRegs[floatRegIndex_++]);
        break;
      default:
        JS_NOT_REACHED("Unexpected argument type");
    }
    return current_;
#endif
}

const Register ABIArgGenerator::NonArgReturnVolatileReg1 = r10;
const Register ABIArgGenerator::NonArgReturnVolatileReg2 = r11;
const Register ABIArgGenerator::NonVolatileReg = r12;

void
Assembler::writeRelocation(JmpSrc src, Relocation::Kind reloc)
{
    if (!jumpRelocations_.length()) {
        
        
        
        
        jumpRelocations_.writeFixedUint32_t(0);
    }
    if (reloc == Relocation::IONCODE) {
        jumpRelocations_.writeUnsigned(src.offset());
        jumpRelocations_.writeUnsigned(jumps_.length());
    }
}

void
Assembler::addPendingJump(JmpSrc src, void *target, Relocation::Kind reloc)
{
    JS_ASSERT(target);

    
    
    if (reloc == Relocation::IONCODE)
        writeRelocation(src, reloc);
    enoughMemory_ &= jumps_.append(RelativePatch(src.offset(), target, reloc));
}

size_t
Assembler::addPatchableJump(JmpSrc src, Relocation::Kind reloc)
{
    
    
    writeRelocation(src, reloc);

    size_t index = jumps_.length();
    enoughMemory_ &= jumps_.append(RelativePatch(src.offset(), NULL, reloc));
    return index;
}


uint8_t *
Assembler::PatchableJumpAddress(IonCode *code, size_t index)
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

    
    masm.align(16);
    extendedJumpTable_ = masm.size();

    
    
    
    JS_ASSERT_IF(jumpRelocations_.length(), jumpRelocations_.length() >= sizeof(uint32_t));
    if (jumpRelocations_.length())
        *(uint32_t *)jumpRelocations_.buffer() = extendedJumpTable_;

    
    for (size_t i = 0; i < jumps_.length(); i++) {
#ifdef DEBUG
        size_t oldSize = masm.size();
#endif
        masm.jmp_rip(0);
        masm.immediate64(0);
        masm.align(SizeOfJumpTableEntry);
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
        if (JSC::X86Assembler::canRelinkJump(src, rp.target)) {
            JSC::X86Assembler::setRel32(src, rp.target);
        } else {
            
            
            JS_ASSERT(extendedJumpTable_);
            JS_ASSERT((extendedJumpTable_ + i * SizeOfJumpTableEntry) <= size() - SizeOfJumpTableEntry);

            
            uint8_t *entry = buffer + extendedJumpTable_ + i * SizeOfJumpTableEntry;
            JSC::X86Assembler::setRel32(src, entry);

            
            
            JSC::X86Assembler::repatchPointer(entry + SizeOfExtendedJump, rp.target);
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
    RelocationIterator(CompactBufferReader &reader)
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

IonCode *
Assembler::CodeFromJump(IonCode *code, uint8_t *jump)
{
    uint8_t *target = (uint8_t *)JSC::X86Assembler::getRel32Target(jump);
    if (target >= code->raw() && target < code->raw() + code->instructionsSize()) {
        
        
        JS_ASSERT(target + SizeOfJumpTableEntry <= code->raw() + code->instructionsSize());

        target = (uint8_t *)JSC::X86Assembler::getPointer(target + SizeOfExtendedJump);
    }

    return IonCode::FromExecutable(target);
}

void
Assembler::TraceJumpRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader)
{
    RelocationIterator iter(reader);
    while (iter.read()) {
        IonCode *child = CodeFromJump(code, code->raw() + iter.offset());
        MarkIonCodeUnbarriered(trc, &child, "rel32");
        JS_ASSERT(child == CodeFromJump(code, code->raw() + iter.offset()));
    }
}

