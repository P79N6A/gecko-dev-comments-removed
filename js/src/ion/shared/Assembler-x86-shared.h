








































#ifndef jsion_assembler_x86_shared__
#define jsion_assembler_x86_shared__

#include "assembler/assembler/X86Assembler.h"

namespace js {
namespace ion {

class AssemblerX86Shared
{
  protected:
    JSC::X86Assembler masm;

    typedef JSC::X86Assembler::JmpSrc JmpSrc;
    typedef JSC::X86Assembler::JmpDst JmpDst;

  public:
    bool oom() const {
        return masm.oom();
    }

  public:
    void movl(const Imm32 &imm32, const Register &dest) {
        masm.movl_i32r(imm32.i32, dest.code());
    }
    void jmp(Label *label) {
        if (label->bound()) {
            
            masm.linkJump(masm.jmp(), JmpDst(label->offset()));
        } else {
            
            JmpSrc j = masm.jmp();
            JmpSrc prev = JmpSrc(label->use(j.offset()));
            masm.setNextJump(j, prev);
        }
    }
    void bind(Label *label) {
        JSC::MacroAssembler::Label jsclabel;
        if (label->used()) {
            bool more;
            JSC::X86Assembler::JmpSrc jmp(label->offset());
            do {
                JSC::X86Assembler::JmpSrc next;
                more = masm.nextJump(jmp, &next);
                masm.linkJump(jmp, masm.label());
                jmp = next;
            } while (more);
        }
        label->bind(masm.label().offset());
    }

    void pop(const Register &reg) {
        masm.pop_r(reg.code());
    }
    void ret() {
        masm.ret();
    }
};

} 
} 

#endif 

