





#ifndef jit_x86_Assembler_x86_h
#define jit_x86_Assembler_x86_h

#include "mozilla/ArrayUtils.h"

#include "jit/CompactBuffer.h"
#include "jit/IonCode.h"
#include "jit/JitCompartment.h"
#include "jit/shared/Assembler-shared.h"
#include "jit/x86-shared/Constants-x86-shared.h"

namespace js {
namespace jit {

static MOZ_CONSTEXPR_VAR Register eax = { X86Encoding::rax };
static MOZ_CONSTEXPR_VAR Register ecx = { X86Encoding::rcx };
static MOZ_CONSTEXPR_VAR Register edx = { X86Encoding::rdx };
static MOZ_CONSTEXPR_VAR Register ebx = { X86Encoding::rbx };
static MOZ_CONSTEXPR_VAR Register esp = { X86Encoding::rsp };
static MOZ_CONSTEXPR_VAR Register ebp = { X86Encoding::rbp };
static MOZ_CONSTEXPR_VAR Register esi = { X86Encoding::rsi };
static MOZ_CONSTEXPR_VAR Register edi = { X86Encoding::rdi };

static MOZ_CONSTEXPR_VAR FloatRegister xmm0 = FloatRegister(X86Encoding::xmm0, FloatRegisters::Double);
static MOZ_CONSTEXPR_VAR FloatRegister xmm1 = FloatRegister(X86Encoding::xmm1, FloatRegisters::Double);
static MOZ_CONSTEXPR_VAR FloatRegister xmm2 = FloatRegister(X86Encoding::xmm2, FloatRegisters::Double);
static MOZ_CONSTEXPR_VAR FloatRegister xmm3 = FloatRegister(X86Encoding::xmm3, FloatRegisters::Double);
static MOZ_CONSTEXPR_VAR FloatRegister xmm4 = FloatRegister(X86Encoding::xmm4, FloatRegisters::Double);
static MOZ_CONSTEXPR_VAR FloatRegister xmm5 = FloatRegister(X86Encoding::xmm5, FloatRegisters::Double);
static MOZ_CONSTEXPR_VAR FloatRegister xmm6 = FloatRegister(X86Encoding::xmm6, FloatRegisters::Double);
static MOZ_CONSTEXPR_VAR FloatRegister xmm7 = FloatRegister(X86Encoding::xmm7, FloatRegisters::Double);

static MOZ_CONSTEXPR_VAR Register InvalidReg = { X86Encoding::invalid_reg };
static MOZ_CONSTEXPR_VAR FloatRegister InvalidFloatReg = FloatRegister();

static MOZ_CONSTEXPR_VAR Register JSReturnReg_Type = ecx;
static MOZ_CONSTEXPR_VAR Register JSReturnReg_Data = edx;
static MOZ_CONSTEXPR_VAR Register StackPointer = esp;
static MOZ_CONSTEXPR_VAR Register FramePointer = ebp;
static MOZ_CONSTEXPR_VAR Register ReturnReg = eax;
static MOZ_CONSTEXPR_VAR FloatRegister ReturnFloat32Reg = FloatRegister(X86Encoding::xmm0, FloatRegisters::Single);
static MOZ_CONSTEXPR_VAR FloatRegister ReturnDoubleReg = FloatRegister(X86Encoding::xmm0, FloatRegisters::Double);
static MOZ_CONSTEXPR_VAR FloatRegister ReturnInt32x4Reg = FloatRegister(X86Encoding::xmm0, FloatRegisters::Int32x4);
static MOZ_CONSTEXPR_VAR FloatRegister ReturnFloat32x4Reg = FloatRegister(X86Encoding::xmm0, FloatRegisters::Float32x4);
static MOZ_CONSTEXPR_VAR FloatRegister ScratchFloat32Reg = FloatRegister(X86Encoding::xmm7, FloatRegisters::Single);
static MOZ_CONSTEXPR_VAR FloatRegister ScratchDoubleReg = FloatRegister(X86Encoding::xmm7, FloatRegisters::Double);
static MOZ_CONSTEXPR_VAR FloatRegister ScratchSimdReg = xmm7;


static MOZ_CONSTEXPR_VAR Register ArgumentsRectifierReg = esi;
static MOZ_CONSTEXPR_VAR Register CallTempReg0 = edi;
static MOZ_CONSTEXPR_VAR Register CallTempReg1 = eax;
static MOZ_CONSTEXPR_VAR Register CallTempReg2 = ebx;
static MOZ_CONSTEXPR_VAR Register CallTempReg3 = ecx;
static MOZ_CONSTEXPR_VAR Register CallTempReg4 = esi;
static MOZ_CONSTEXPR_VAR Register CallTempReg5 = edx;


static MOZ_CONSTEXPR_VAR Register CallTempNonArgRegs[] = { edi, eax, ebx, ecx, esi, edx };
static const uint32_t NumCallTempNonArgRegs =
    mozilla::ArrayLength(CallTempNonArgRegs);

class ABIArgGenerator
{
    uint32_t stackOffset_;
    ABIArg current_;

  public:
    ABIArgGenerator();
    ABIArg next(MIRType argType);
    ABIArg& current() { return current_; }
    uint32_t stackBytesConsumedSoFar() const { return stackOffset_; }

    
    static const Register NonArgReturnReg0;
    static const Register NonArgReturnReg1;
    static const Register NonVolatileReg;
    static const Register NonArg_VolatileReg;
    static const Register NonReturn_VolatileReg0;
};

static MOZ_CONSTEXPR_VAR Register OsrFrameReg = edx;
static MOZ_CONSTEXPR_VAR Register PreBarrierReg = edx;


static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegCallee = ecx;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE0 = edi;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE1 = eax;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE2 = ebx;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE3 = edx;


static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegReturnData = edx;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegReturnType = ecx;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegD0 = edi;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegD1 = eax;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegD2 = esi;



#if defined(__GNUC__)
static MOZ_CONSTEXPR_VAR uint32_t ABIStackAlignment = 16;
#else
static MOZ_CONSTEXPR_VAR uint32_t ABIStackAlignment = 4;
#endif
static MOZ_CONSTEXPR_VAR uint32_t CodeAlignment = 16;
static MOZ_CONSTEXPR_VAR uint32_t JitStackAlignment = 16;

static MOZ_CONSTEXPR_VAR uint32_t JitStackValueAlignment = JitStackAlignment / sizeof(Value);
static_assert(JitStackAlignment % sizeof(Value) == 0 && JitStackValueAlignment >= 1,
  "Stack alignment should be a non-zero multiple of sizeof(Value)");





static MOZ_CONSTEXPR_VAR bool SupportsSimd = true;
static MOZ_CONSTEXPR_VAR uint32_t SimdMemoryAlignment = 16;

static_assert(CodeAlignment % SimdMemoryAlignment == 0,
  "Code alignment should be larger than any of the alignments which are used for "
  "the constant sections of the code buffer.  Thus it should be larger than the "
  "alignment for SIMD constants.");

static_assert(JitStackAlignment % SimdMemoryAlignment == 0,
  "Stack alignment should be larger than any of the alignments which are used for "
  "spilled values.  Thus it should be larger than the alignment for SIMD accesses.");

static const uint32_t AsmJSStackAlignment = SimdMemoryAlignment;

struct ImmTag : public Imm32
{
    ImmTag(JSValueTag mask)
      : Imm32(int32_t(mask))
    { }
};

struct ImmType : public ImmTag
{
    ImmType(JSValueType type)
      : ImmTag(JSVAL_TYPE_TO_TAG(type))
    { }
};

static const Scale ScalePointer = TimesFour;

} 
} 

#include "jit/x86-shared/Assembler-x86-shared.h"

namespace js {
namespace jit {

static inline void
PatchJump(CodeLocationJump jump, CodeLocationLabel label)
{
#ifdef DEBUG
    
    
    
    unsigned char* x = (unsigned char*)jump.raw() - 5;
    MOZ_ASSERT(((*x >= 0x80 && *x <= 0x8F) && *(x - 1) == 0x0F) ||
               (*x == 0xE9));
#endif
    X86Encoding::SetRel32(jump.raw(), label.raw());
}
static inline void
PatchBackedge(CodeLocationJump& jump_, CodeLocationLabel label, JitRuntime::BackedgeTarget target)
{
    PatchJump(jump_, label);
}


static const ValueOperand JSReturnOperand = ValueOperand(JSReturnReg_Type, JSReturnReg_Data);

class Assembler : public AssemblerX86Shared
{
    void writeRelocation(JmpSrc src) {
        jumpRelocations_.writeUnsigned(src.offset());
    }
    void addPendingJump(JmpSrc src, ImmPtr target, Relocation::Kind kind) {
        enoughMemory_ &= jumps_.append(RelativePatch(src.offset(), target.value, kind));
        if (kind == Relocation::JITCODE)
            writeRelocation(src);
    }

  public:
    using AssemblerX86Shared::movl;
    using AssemblerX86Shared::j;
    using AssemblerX86Shared::jmp;
    using AssemblerX86Shared::vmovsd;
    using AssemblerX86Shared::vmovss;
    using AssemblerX86Shared::retarget;
    using AssemblerX86Shared::cmpl;
    using AssemblerX86Shared::call;
    using AssemblerX86Shared::push;
    using AssemblerX86Shared::pop;

    static void TraceJumpRelocations(JSTracer* trc, JitCode* code, CompactBufferReader& reader);

    
    
    void executableCopy(uint8_t* buffer);

    

    void push(ImmGCPtr ptr) {
        masm.push_i32(int32_t(ptr.value));
        writeDataRelocation(ptr);
    }
    void push(ImmMaybeNurseryPtr ptr) {
        push(noteMaybeNurseryPtr(ptr));
    }
    void push(const ImmWord imm) {
        push(Imm32(imm.value));
    }
    void push(const ImmPtr imm) {
        push(ImmWord(uintptr_t(imm.value)));
    }
    void push(FloatRegister src) {
        subl(Imm32(sizeof(double)), StackPointer);
        vmovsd(src, Address(StackPointer, 0));
    }

    CodeOffsetLabel pushWithPatch(ImmWord word) {
        masm.push_i32(int32_t(word.value));
        return CodeOffsetLabel(masm.currentOffset());
    }

    void pop(FloatRegister src) {
        vmovsd(Address(StackPointer, 0), src);
        addl(Imm32(sizeof(double)), StackPointer);
    }

    CodeOffsetLabel movWithPatch(ImmWord word, Register dest) {
        movl(Imm32(word.value), dest);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movWithPatch(ImmPtr imm, Register dest) {
        return movWithPatch(ImmWord(uintptr_t(imm.value)), dest);
    }

    void movl(ImmGCPtr ptr, Register dest) {
        masm.movl_i32r(uintptr_t(ptr.value), dest.encoding());
        writeDataRelocation(ptr);
    }
    void movl(ImmGCPtr ptr, const Operand& dest) {
        switch (dest.kind()) {
          case Operand::REG:
            masm.movl_i32r(uintptr_t(ptr.value), dest.reg());
            writeDataRelocation(ptr);
            break;
          case Operand::MEM_REG_DISP:
            masm.movl_i32m(uintptr_t(ptr.value), dest.disp(), dest.base());
            writeDataRelocation(ptr);
            break;
          case Operand::MEM_SCALE:
            masm.movl_i32m(uintptr_t(ptr.value), dest.disp(), dest.base(), dest.index(), dest.scale());
            writeDataRelocation(ptr);
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void movl(ImmWord imm, Register dest) {
        masm.movl_i32r(imm.value, dest.encoding());
    }
    void movl(ImmPtr imm, Register dest) {
        movl(ImmWord(uintptr_t(imm.value)), dest);
    }
    void mov(ImmWord imm, Register dest) {
        
        
        
        if (imm.value == 0)
            xorl(dest, dest);
        else
            movl(imm, dest);
    }
    void mov(ImmPtr imm, Register dest) {
        mov(ImmWord(uintptr_t(imm.value)), dest);
    }
    void mov(AsmJSImmPtr imm, Register dest) {
        masm.movl_i32r(-1, dest.encoding());
        append(AsmJSAbsoluteLink(CodeOffsetLabel(masm.currentOffset()), imm.kind()));
    }
    void mov(const Operand& src, Register dest) {
        movl(src, dest);
    }
    void mov(Register src, const Operand& dest) {
        movl(src, dest);
    }
    void mov(Imm32 imm, const Operand& dest) {
        movl(imm, dest);
    }
    void mov(AbsoluteLabel* label, Register dest) {
        MOZ_ASSERT(!label->bound());
        
        
        masm.movl_i32r(label->prev(), dest.encoding());
        label->setPrev(masm.size());
    }
    void mov(Register src, Register dest) {
        movl(src, dest);
    }
    void xchg(Register src, Register dest) {
        xchgl(src, dest);
    }
    void lea(const Operand& src, Register dest) {
        return leal(src, dest);
    }

    void fld32(const Operand& dest) {
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            masm.fld32_m(dest.disp(), dest.base());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }

    void fstp32(const Operand& src) {
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.fstp32_m(src.disp(), src.base());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }

    void cmpl(ImmWord rhs, Register lhs) {
        masm.cmpl_ir(rhs.value, lhs.encoding());
    }
    void cmpl(ImmPtr rhs, Register lhs) {
        cmpl(ImmWord(uintptr_t(rhs.value)), lhs);
    }
    void cmpl(ImmGCPtr rhs, Register lhs) {
        masm.cmpl_i32r(uintptr_t(rhs.value), lhs.encoding());
        writeDataRelocation(rhs);
    }
    void cmpl(Register rhs, Register lhs) {
        masm.cmpl_rr(rhs.encoding(), lhs.encoding());
    }
    void cmpl(ImmGCPtr rhs, const Operand& lhs) {
        switch (lhs.kind()) {
          case Operand::REG:
            masm.cmpl_i32r(uintptr_t(rhs.value), lhs.reg());
            writeDataRelocation(rhs);
            break;
          case Operand::MEM_REG_DISP:
            masm.cmpl_i32m(uintptr_t(rhs.value), lhs.disp(), lhs.base());
            writeDataRelocation(rhs);
            break;
          case Operand::MEM_ADDRESS32:
            masm.cmpl_i32m(uintptr_t(rhs.value), lhs.address());
            writeDataRelocation(rhs);
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void cmpl(ImmMaybeNurseryPtr rhs, const Operand& lhs) {
        cmpl(noteMaybeNurseryPtr(rhs), lhs);
    }
    void cmpl(Register rhs, AsmJSAbsoluteAddress lhs) {
        masm.cmpl_rm_disp32(rhs.encoding(), (void*)-1);
        append(AsmJSAbsoluteLink(CodeOffsetLabel(masm.currentOffset()), lhs.kind()));
    }
    void cmpl(Imm32 rhs, AsmJSAbsoluteAddress lhs) {
        JmpSrc src = masm.cmpl_im_disp32(rhs.value, (void*)-1);
        append(AsmJSAbsoluteLink(CodeOffsetLabel(src.offset()), lhs.kind()));
    }

    void jmp(ImmPtr target, Relocation::Kind reloc = Relocation::HARDCODED) {
        JmpSrc src = masm.jmp();
        addPendingJump(src, target, reloc);
    }
    void j(Condition cond, ImmPtr target,
           Relocation::Kind reloc = Relocation::HARDCODED) {
        JmpSrc src = masm.jCC(static_cast<X86Encoding::Condition>(cond));
        addPendingJump(src, target, reloc);
    }

    void jmp(JitCode* target) {
        jmp(ImmPtr(target->raw()), Relocation::JITCODE);
    }
    void j(Condition cond, JitCode* target) {
        j(cond, ImmPtr(target->raw()), Relocation::JITCODE);
    }
    void call(JitCode* target) {
        JmpSrc src = masm.call();
        addPendingJump(src, ImmPtr(target->raw()), Relocation::JITCODE);
    }
    void call(ImmWord target) {
        call(ImmPtr((void*)target.value));
    }
    void call(ImmPtr target) {
        JmpSrc src = masm.call();
        addPendingJump(src, target, Relocation::HARDCODED);
    }

    
    
    CodeOffsetLabel toggledCall(JitCode* target, bool enabled) {
        CodeOffsetLabel offset(size());
        JmpSrc src = enabled ? masm.call() : masm.cmp_eax();
        addPendingJump(src, ImmPtr(target->raw()), Relocation::JITCODE);
        MOZ_ASSERT(size() - offset.offset() == ToggledCallSize(nullptr));
        return offset;
    }

    static size_t ToggledCallSize(uint8_t* code) {
        
        return 5;
    }

    
    
    void retarget(Label* label, ImmPtr target, Relocation::Kind reloc) {
        if (label->used()) {
            bool more;
            X86Encoding::JmpSrc jmp(label->offset());
            do {
                X86Encoding::JmpSrc next;
                more = masm.nextJump(jmp, &next);
                addPendingJump(jmp, target, reloc);
                jmp = next;
            } while (more);
        }
        label->reset();
    }

    
    
    CodeOffsetLabel movlWithPatch(Imm32 imm, Register dest) {
        masm.movl_i32r(imm.value, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }

    
    CodeOffsetLabel movsblWithPatch(const Operand& src, Register dest) {
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.movsbl_mr_disp32(src.disp(), src.base(), dest.encoding());
            break;
          case Operand::MEM_ADDRESS32:
            masm.movsbl_mr(src.address(), dest.encoding());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movzblWithPatch(const Operand& src, Register dest) {
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.movzbl_mr_disp32(src.disp(), src.base(), dest.encoding());
            break;
          case Operand::MEM_ADDRESS32:
            masm.movzbl_mr(src.address(), dest.encoding());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movswlWithPatch(const Operand& src, Register dest) {
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.movswl_mr_disp32(src.disp(), src.base(), dest.encoding());
            break;
          case Operand::MEM_ADDRESS32:
            masm.movswl_mr(src.address(), dest.encoding());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movzwlWithPatch(const Operand& src, Register dest) {
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.movzwl_mr_disp32(src.disp(), src.base(), dest.encoding());
            break;
          case Operand::MEM_ADDRESS32:
            masm.movzwl_mr(src.address(), dest.encoding());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movlWithPatch(const Operand& src, Register dest) {
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.movl_mr_disp32(src.disp(), src.base(), dest.encoding());
            break;
          case Operand::MEM_ADDRESS32:
            masm.movl_mr(src.address(), dest.encoding());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovssWithPatch(const Operand& src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovss_mr_disp32(src.disp(), src.base(), dest.encoding());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovss_mr(src.address(), dest.encoding());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovdWithPatch(const Operand& src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovd_mr_disp32(src.disp(), src.base(), dest.encoding());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovd_mr(src.address(), dest.encoding());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovqWithPatch(const Operand& src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovq_mr_disp32(src.disp(), src.base(), dest.encoding());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovq_mr(src.address(), dest.encoding());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovsdWithPatch(const Operand& src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovsd_mr_disp32(src.disp(), src.base(), dest.encoding());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovsd_mr(src.address(), dest.encoding());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovupsWithPatch(const Operand& src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovups_mr_disp32(src.disp(), src.base(), dest.encoding());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovups_mr(src.address(), dest.encoding());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovdquWithPatch(const Operand& src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovdqu_mr_disp32(src.disp(), src.base(), dest.encoding());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovdqu_mr(src.address(), dest.encoding());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }

    
    CodeOffsetLabel movbWithPatch(Register src, const Operand& dest) {
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            masm.movb_rm_disp32(src.encoding(), dest.disp(), dest.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.movb_rm(src.encoding(), dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movwWithPatch(Register src, const Operand& dest) {
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            masm.movw_rm_disp32(src.encoding(), dest.disp(), dest.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.movw_rm(src.encoding(), dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movlWithPatch(Register src, const Operand& dest) {
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            masm.movl_rm_disp32(src.encoding(), dest.disp(), dest.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.movl_rm(src.encoding(), dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovdWithPatch(FloatRegister src, const Operand& dest) {
        MOZ_ASSERT(HasSSE2());
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovd_rm_disp32(src.encoding(), dest.disp(), dest.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovd_rm(src.encoding(), dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovqWithPatch(FloatRegister src, const Operand& dest) {
        MOZ_ASSERT(HasSSE2());
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovq_rm_disp32(src.encoding(), dest.disp(), dest.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovq_rm(src.encoding(), dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovssWithPatch(FloatRegister src, const Operand& dest) {
        MOZ_ASSERT(HasSSE2());
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovss_rm_disp32(src.encoding(), dest.disp(), dest.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovss_rm(src.encoding(), dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovsdWithPatch(FloatRegister src, const Operand& dest) {
        MOZ_ASSERT(HasSSE2());
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovsd_rm_disp32(src.encoding(), dest.disp(), dest.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovsd_rm(src.encoding(), dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovupsWithPatch(FloatRegister src, const Operand& dest) {
        MOZ_ASSERT(HasSSE2());
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovups_rm_disp32(src.encoding(), dest.disp(), dest.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovups_rm(src.encoding(), dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovdquWithPatch(FloatRegister src, const Operand& dest) {
        MOZ_ASSERT(HasSSE2());
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            masm.vmovdqu_rm_disp32(src.encoding(), dest.disp(), dest.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.vmovdqu_rm(src.encoding(), dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
        return CodeOffsetLabel(masm.currentOffset());
    }

    
    CodeOffsetLabel movlWithPatch(PatchedAbsoluteAddress addr, Register index, Scale scale,
                                  Register dest)
    {
        masm.movl_mr(addr.addr, index.encoding(), scale, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }

    
    CodeOffsetLabel movsblWithPatch(PatchedAbsoluteAddress src, Register dest) {
        masm.movsbl_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movzblWithPatch(PatchedAbsoluteAddress src, Register dest) {
        masm.movzbl_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movswlWithPatch(PatchedAbsoluteAddress src, Register dest) {
        masm.movswl_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movzwlWithPatch(PatchedAbsoluteAddress src, Register dest) {
        masm.movzwl_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movlWithPatch(PatchedAbsoluteAddress src, Register dest) {
        masm.movl_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovssWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovss_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovdWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovd_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovqWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovq_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovsdWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovsd_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovdqaWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovdqa_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovdquWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovdqu_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovapsWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovaps_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovupsWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovups_mr(src.addr, dest.encoding());
        return CodeOffsetLabel(masm.currentOffset());
    }

    
    CodeOffsetLabel movbWithPatch(Register src, PatchedAbsoluteAddress dest) {
        masm.movb_rm(src.encoding(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movwWithPatch(Register src, PatchedAbsoluteAddress dest) {
        masm.movw_rm(src.encoding(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movlWithPatch(Register src, PatchedAbsoluteAddress dest) {
        masm.movl_rm(src.encoding(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovssWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovss_rm(src.encoding(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovdWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovd_rm(src.encoding(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovqWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovq_rm(src.encoding(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovsdWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovsd_rm(src.encoding(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovdqaWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovdqa_rm(src.encoding(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovapsWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovaps_rm(src.encoding(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovdquWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovdqu_rm(src.encoding(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel vmovupsWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.vmovups_rm(src.encoding(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }

    void loadAsmJSActivation(Register dest) {
        CodeOffsetLabel label = movlWithPatch(PatchedAbsoluteAddress(), dest);
        append(AsmJSGlobalAccess(label, AsmJSActivationGlobalDataOffset));
    }
    void loadAsmJSHeapRegisterFromGlobalData() {
        
    }

    static bool canUseInSingleByteInstruction(Register reg) {
        return X86Encoding::HasSubregL(reg.encoding());
    }
};






static inline bool
GetTempRegForIntArg(uint32_t usedIntArgs, uint32_t usedFloatArgs, Register* out)
{
    if (usedIntArgs >= NumCallTempNonArgRegs)
        return false;
    *out = CallTempNonArgRegs[usedIntArgs];
    return true;
}

} 
} 

#endif 
