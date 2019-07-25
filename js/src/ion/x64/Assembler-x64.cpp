








































#include "Assembler-x64.h"
#include "jsgcmark.h"

using namespace js;
using namespace js::ion;

void
Assembler::writeRelocation(JmpSrc src)
{
    if (!relocations_.length()) {
        
        
        
        
        relocations_.writeFixedUint32(0);
    }
    relocations_.writeUnsigned(src.offset());
    relocations_.writeUnsigned(jumps_.length());
}

void
Assembler::addPendingJump(JmpSrc src, void *target, Relocation::Kind reloc)
{
    
    
    if (reloc == Relocation::CODE)
        writeRelocation(src);
    enoughMemory_ &= jumps_.append(RelativePatch(src.offset(), target, reloc));
}

void
Assembler::flush()
{
    if (!jumps_.length() || oom())
        return;

    
    masm.align(16);
    extendedJumpTable_ = masm.size();

    
    
    JS_ASSERT(relocations_.length() >= sizeof(uint32));
    *(uint32 *)relocations_.buffer() = extendedJumpTable_;

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
Assembler::executableCopy(uint8 *buffer)
{
    AssemblerX86Shared::executableCopy(buffer);

    for (size_t i = 0; i < jumps_.length(); i++) {
        RelativePatch &rp = jumps_[i];
        uint8 *src = buffer + rp.offset;
        if (JSC::X86Assembler::canRelinkJump(src, rp.target)) {
            JSC::X86Assembler::setRel32(src, rp.target);
        } else {
            
            
            JS_ASSERT(extendedJumpTable_);
            JS_ASSERT((extendedJumpTable_ + i * SizeOfJumpTableEntry) < size() - SizeOfJumpTableEntry);

            
            uint8 *entry = buffer + extendedJumpTable_ + i * SizeOfJumpTableEntry;
            JSC::X86Assembler::setRel32(src, entry);

            
            
            JSC::X86Assembler::repatchPointer(entry + SizeOfExtendedJump, rp.target);
        }
    }
}

class RelocationIterator
{
    CompactBufferReader reader_;
    uint32 tableStart_;
    uint32 offset_;
    uint32 extOffset_;

  public:
    RelocationIterator(CompactBufferReader &reader)
      : reader_(reader)
    {
        tableStart_ = reader_.readFixedUint32();
    }

    bool read() {
        if (!reader_.more())
            return false;
        offset_ = reader_.readUnsigned();
        extOffset_ = reader_.readUnsigned();
        return true;
    }

    uint32 offset() const {
        return offset_;
    }
    uint32 extendedOffset() const {
        return extOffset_;
    }
};

IonCode *
Assembler::CodeFromJump(IonCode *code, uint8 *jump)
{
    uint8 *target = (uint8 *)JSC::X86Assembler::getRel32Target(jump);
    if (target >= code->raw() && target < code->raw() + code->instructionsSize()) {
        
        
        JS_ASSERT(target + SizeOfJumpTableEntry < code->raw() + code->instructionsSize());

        target = (uint8 *)JSC::X86Assembler::getPointer(target + SizeOfExtendedJump);
    }

    return IonCode::FromExecutable(target);
}

void
Assembler::TraceRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader)
{
    RelocationIterator iter(reader);
    while (iter.read()) {
        IonCode *code = CodeFromJump(code, code->raw() + iter.offset());
        MarkIonCode(trc, code, "rel32");
    }
}

