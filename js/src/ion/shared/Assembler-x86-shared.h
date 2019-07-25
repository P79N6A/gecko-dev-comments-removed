








































#ifndef jsion_assembler_x86_shared__
#define jsion_assembler_x86_shared__

#include "assembler/assembler/X86Assembler.h"

namespace js {
namespace ion {

class AssemblerX86Shared
{
  protected:
    struct RelativePatch {
        int32 offset;
        void *target;
        Relocation::Kind kind;

        RelativePatch(int32 offset, void *target, Relocation::Kind kind)
          : offset(offset),
            target(target),
            kind(kind)
        { }
    };

    js::Vector<DeferredData *, 0, SystemAllocPolicy> data_;
    js::Vector<CodeLabel *, 0, SystemAllocPolicy> codeLabels_;
    js::Vector<RelativePatch, 8, SystemAllocPolicy> jumps_;
    CompactBufferWriter relocations_;
    size_t dataBytesNeeded_;

    bool enoughMemory_;

  protected:
    JSC::X86Assembler masm;

    typedef JSC::X86Assembler::JmpSrc JmpSrc;
    typedef JSC::X86Assembler::JmpDst JmpDst;

  public:
    enum Condition {
        Equal = JSC::X86Assembler::ConditionE,
        NotEqual = JSC::X86Assembler::ConditionNE,
        Above = JSC::X86Assembler::ConditionA,
        AboveOrEqual = JSC::X86Assembler::ConditionAE,
        Below = JSC::X86Assembler::ConditionB,
        BelowOrEqual = JSC::X86Assembler::ConditionBE,
        GreaterThan = JSC::X86Assembler::ConditionG,
        GreaterThanOrEqual = JSC::X86Assembler::ConditionGE,
        LessThan = JSC::X86Assembler::ConditionL,
        LessThanOrEqual = JSC::X86Assembler::ConditionLE,
        Overflow = JSC::X86Assembler::ConditionO,
        Signed = JSC::X86Assembler::ConditionS,
        Zero = JSC::X86Assembler::ConditionE,
        NonZero = JSC::X86Assembler::ConditionNE,
        Parity = JSC::X86Assembler::ConditionP,
        NoParity = JSC::X86Assembler::ConditionNP
    };

    AssemblerX86Shared()
      : dataBytesNeeded_(0),
        enoughMemory_(true)
    {
    }

    static Condition InvertCondition(Condition cond);

    static void TraceRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader);

    
    void trace(JSTracer *trc);

    bool oom() const {
        return masm.oom() ||
               !enoughMemory_ ||
               relocations_.oom();
    }

    void executableCopy(void *buffer);
    void processDeferredData(IonCode *code, uint8 *data);
    void processCodeLabels(IonCode *code);
    void copyRelocationTable(uint8 *buffer);

    bool addDeferredData(DeferredData *data, size_t bytes) {
        data->setOffset(dataBytesNeeded_);
        dataBytesNeeded_ += bytes;
        if (dataBytesNeeded_ >= MAX_BUFFER_SIZE)
            return false;
        return data_.append(data);
    }
    
    bool addCodeLabel(CodeLabel *label) {
        return codeLabels_.append(label);
    }

    
    size_t size() const {
        return masm.size();
    }
    
    size_t relocationTableSize() const {
        return relocations_.length();
    }
    
    size_t dataSize() const {
        return dataBytesNeeded_;
    }
    size_t bytesNeeded() const {
        return size() + dataSize() + relocationTableSize();
    }

  public:
    void align(int alignment) {
        masm.align(alignment);
    }
    void movl(const Imm32 &imm32, const Register &dest) {
        masm.movl_i32r(imm32.value, dest.code());
    }
    void movl(const Register &src, const Register &dest) {
        masm.movl_rr(src.code(), dest.code());
    }
    void movl(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.movl_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.movl_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void movl(const Register &src, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::REG:
            masm.movl_rr(src.code(), dest.reg());
            break;
          case Operand::REG_DISP:
            masm.movl_rm(src.code(), dest.disp(), dest.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }

    void movsd(const FloatRegister &src, const FloatRegister &dest) {
        masm.movsd_rr(src.code(), dest.code());
    }
    void movsd(const Operand &src, const FloatRegister &dest) {
        switch (src.kind()) {
          case Operand::FPREG:
            masm.movsd_rr(src.fpu(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.movsd_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void movsd(const FloatRegister &src, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::FPREG:
            masm.movsd_rr(src.code(), dest.fpu());
            break;
          case Operand::REG_DISP:
            masm.movsd_rm(src.code(), dest.disp(), dest.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }

    void load16(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG_DISP:
            masm.movzwl_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }

    void j(Condition cond, Label *label) {
        if (label->bound()) {
            
            masm.linkJump(masm.jCC(static_cast<JSC::X86Assembler::Condition>(cond)), JmpDst(label->offset()));
        } else {
            
            JmpSrc j = masm.jCC(static_cast<JSC::X86Assembler::Condition>(cond));
            JmpSrc prev = JmpSrc(label->use(j.offset()));
            masm.setNextJump(j, prev);
        }
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
    void jmp(const Operand &op){
        switch (op.kind()) {
          case Operand::SCALE:
            masm.jmp_m(op.disp(), op.base(), op.index(), op.scale());
            break;
          case Operand::REG:
            masm.jmp_r(op.reg());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
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

    
    void retarget(Label *label, Label *target) {
        JSC::MacroAssembler::Label jsclabel;
        if (label->used()) {
            bool more;
            JSC::X86Assembler::JmpSrc jmp(label->offset());
            do {
                JSC::X86Assembler::JmpSrc next;
                more = masm.nextJump(jmp, &next);

                if (target->bound()) {
                    
                    masm.linkJump(jmp, JmpDst(target->offset()));
                } else {
                    
                    JmpSrc prev = JmpSrc(target->use(jmp.offset()));
                    masm.setNextJump(jmp, prev);
                }

                jmp = next;
            } while (more);
        }
        label->reset();
    }

    static void Bind(IonCode *code, AbsoluteLabel *label, const void *address) {
        uint8 *raw = code->raw();
        if (label->used()) {
            intptr_t src = label->offset();
            do {
                intptr_t next = reinterpret_cast<intptr_t>(JSC::X86Assembler::getPointer(raw + src));
                JSC::X86Assembler::setPointer(raw + src, address);
                src = next;
            } while (src != AbsoluteLabel::INVALID_OFFSET);
        }
        JS_ASSERT(((uint8 *)address - raw) >= 0 && ((uint8 *)address - raw) < INT_MAX);
        label->bind();
    }

    void ret() {
        masm.ret();
    }
    void call(Label *label) {
        if (label->bound()) {
            masm.linkJump(masm.call(), JmpDst(label->offset()));
        } else {
            JmpSrc j = masm.call();
            JmpSrc prev = JmpSrc(label->use(j.offset()));
            masm.setNextJump(j, prev);
        }
    }
    void call(const Register &reg) {
        masm.call(reg.code());
    }
    void call(const Operand &op) {
        switch (op.kind()) {
          case Operand::REG:
            masm.call(op.reg());
            break;
          case Operand::REG_DISP:
            masm.call_m(op.disp(), op.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }

    void breakpoint() {
        masm.int3();
    }

    static bool HasSSE41() {
        return JSC::MacroAssembler::getSSEState() >= JSC::MacroAssembler::HasSSE4_1;
    }

    
    
    
    void cmpl(const Register &lhs, const Register &rhs) {
        masm.cmpl_rr(rhs.code(), lhs.code());
    }
    void cmpl(const Register &lhs, const Operand &rhs) {
        switch (rhs.kind()) {
          case Operand::REG:
            masm.cmpl_rr(rhs.reg(), lhs.code());
            break;
          case Operand::REG_DISP:
            masm.cmpl_mr(rhs.disp(), rhs.base(), lhs.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void cmpl(const Register &src, Imm32 imm) {
        masm.cmpl_ir(imm.value, src.code());
    }
    void cmpl(const Operand &op, Imm32 imm) {
        switch (op.kind()) {
          case Operand::REG:
            masm.cmpl_ir(imm.value, op.reg());
            break;
          case Operand::REG_DISP:
            masm.cmpl_im(imm.value, op.disp(), op.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void setCC(Condition cond, const Register &r) {
        masm.setCC_r(static_cast<JSC::X86Assembler::Condition>(cond), r.code());
    }
    void testl(const Register &lhs, const Register &rhs) {
        masm.testl_rr(rhs.code(), lhs.code());
    }
    void testl(Imm32 lhs, const Register &rhs) {
        masm.testl_i32r(lhs.value, rhs.code());
    }
    void testl(Imm32 lhs, const Operand &rhs) {
        switch (rhs.kind()) {
          case Operand::REG:
            masm.testl_i32r(lhs.value, rhs.reg());
            break;
          case Operand::REG_DISP:
            masm.testl_i32m(lhs.value, rhs.disp(), rhs.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
            break;
        }
    }

    void addl(Imm32 imm, const Register &dest) {
        masm.addl_ir(imm.value, dest.code());
    }
    void addl(Imm32 imm, const Operand &op) {
        switch (op.kind()) {
          case Operand::REG:
            masm.addl_ir(imm.value, op.reg());
            break;
          case Operand::REG_DISP:
            masm.addl_im(imm.value, op.disp(), op.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void subl(Imm32 imm, const Register &dest) {
        masm.subl_ir(imm.value, dest.code());
    }
    void subl(Imm32 imm, const Operand &op) {
        switch (op.kind()) {
          case Operand::REG:
            masm.subl_ir(imm.value, op.reg());
            break;
          case Operand::REG_DISP:
            masm.subl_im(imm.value, op.disp(), op.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void addl(const Register &src, const Register &dest) {
        masm.addl_rr(src.code(), dest.code());
    }
    void subl(const Register &src, const Register &dest) {
        masm.subl_rr(src.code(), dest.code());
    }
    void subl(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.subl_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.subl_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void orl(Imm32 imm, const Register &reg) {
        masm.orl_ir(imm.value, reg.code());
    }
    void orl(Imm32 imm, const Operand &op) {
        switch (op.kind()) {
          case Operand::REG:
            masm.orl_ir(imm.value, op.reg());
            break;
          case Operand::REG_DISP:
            masm.orl_im(imm.value, op.disp(), op.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void xorl(const Register &src, const Register &dest) {
        masm.xorl_rr(src.code(), dest.code());
    }
    void xorl(Imm32 imm, const Register &reg) {
        masm.xorl_ir(imm.value, reg.code());
    }
    void xorl(Imm32 imm, const Operand &op) {
        switch (op.kind()) {
          case Operand::REG:
            masm.xorl_ir(imm.value, op.reg());
            break;
          case Operand::REG_DISP:
            masm.xorl_im(imm.value, op.disp(), op.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void andl(Imm32 imm, const Register &dest) {
        masm.andl_ir(imm.value, dest.code());
    }
    void andl(Imm32 imm, const Operand &op) {
        switch (op.kind()) {
          case Operand::REG:
            masm.andl_ir(imm.value, op.reg());
            break;
          case Operand::REG_DISP:
            masm.andl_im(imm.value, op.disp(), op.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void addl(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.addl_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.addl_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void orl(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.orl_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.orl_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void xorl(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.xorl_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.xorl_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void andl(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.andl_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.andl_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void imull(Imm32 imm, const Register &dest) {
        masm.imull_i32r(dest.code(), imm.value, dest.code());
    }
    void imull(const Register &src, const Register &dest) {
        masm.imull_rr(src.code(), dest.code());
    }
    void imull(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.imull_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.imull_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void negl(const Operand &src) {
        switch (src.kind()) {
          case Operand::REG:
            masm.negl_r(src.reg());
            break;
          case Operand::REG_DISP:
            masm.negl_m(src.disp(), src.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void notl(const Operand &src) {
        switch (src.kind()) {
          case Operand::REG:
            masm.notl_r(src.reg());
            break;
          case Operand::REG_DISP:
            masm.notl_m(src.disp(), src.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }

    void shrl(const Imm32 imm, const Register &dest) {
        masm.shrl_i8r(imm.value, dest.code());
    }
    void shll(const Imm32 imm, const Register &dest) {
        masm.shll_i8r(imm.value, dest.code());
    }
    void sarl(const Imm32 imm, const Register &dest) {
        masm.sarl_i8r(imm.value, dest.code());
    }
    void shrl_cl(const Register &dest) {
        masm.shrl_CLr(dest.code());
    }
    void shll_cl(const Register &dest) {
        masm.shll_CLr(dest.code());
    }
    void sarl_cl(const Register &dest) {
        masm.sarl_CLr(dest.code());
    }

    void push(const Imm32 imm) {
        masm.push_i32(imm.value);
    }
    void push(const Operand &src) {
        switch (src.kind()) {
          case Operand::REG:
            masm.push_r(src.reg());
            break;
          case Operand::REG_DISP:
            masm.push_m(src.disp(), src.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void push(const Register &src) {
        masm.push_r(src.code());
    }
    void pop(const Operand &src) {
        switch (src.kind()) {
          case Operand::REG:
            masm.pop_r(src.reg());
            break;
          case Operand::REG_DISP:
            masm.pop_m(src.disp(), src.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void pop(const Register &src) {
        masm.pop_r(src.code());
    }

    
    void movzxbl(const Register &src, const Register &dest) {
        masm.movzbl_rr(src.code(), dest.code());
    }

    void cdq() {
        masm.cdq();
    }
    void idiv(Register dest) {
        masm.idivl_r(dest.code());
    }

    void unpcklps(const FloatRegister &src, const FloatRegister &dest) {
        masm.unpcklps_rr(src.code(), dest.code());
    }
    void pinsrd(const Register &src, const FloatRegister &dest) {
        masm.pinsrd_rr(src.code(), dest.code());
    }
    void pinsrd(const Operand &src, const FloatRegister &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.pinsrd_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.pinsrd_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void psrlq(Imm32 shift, const FloatRegister &dest) {
        masm.psrldq_rr(dest.code(), shift.value);
    }

    void cvtsi2sd(const Operand &src, const FloatRegister &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.cvtsi2sd_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.cvtsi2sd_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void cvttsd2si(const FloatRegister &src, const Register &dest) {
        masm.cvttsd2si_rr(src.code(), dest.code());
    }
    void cvtsi2sd(const Register &src, const FloatRegister &dest) {
        masm.cvtsi2sd_rr(src.code(), dest.code());
    }
    void movmskpd(const FloatRegister &src, const Register &dest) {
        masm.movmskpd_rr(src.code(), dest.code());
    }
    void ptest(const FloatRegister &lhs, const FloatRegister &rhs) {
        JS_ASSERT(HasSSE41());
        masm.ptest_rr(rhs.code(), lhs.code());
    }
    void ucomisd(const FloatRegister &lhs, const FloatRegister &rhs) {
        masm.ucomisd_rr(rhs.code(), lhs.code());
    }
    void movd(const Register &src, const FloatRegister &dest) {
        masm.movd_rr(src.code(), dest.code());
    }
    void movd(const FloatRegister &src, const Register &dest) {
        masm.movd_rr(src.code(), dest.code());
    }
    void addsd(const FloatRegister &src, const FloatRegister &dest) {
        masm.addsd_rr(src.code(), dest.code());
    }
    void subsd(const FloatRegister &src, const FloatRegister &dest) {
        masm.subsd_rr(src.code(), dest.code());
    }
    void mulsd(const FloatRegister &src, const FloatRegister &dest) {
        masm.mulsd_rr(src.code(), dest.code());
    }
    void divsd(const FloatRegister &src, const FloatRegister &dest) {
        masm.divsd_rr(src.code(), dest.code());
    }
    void xorpd(const FloatRegister &src, const FloatRegister &dest) {
        masm.xorpd_rr(src.code(), dest.code());
    }
};

} 
} 

#endif 

