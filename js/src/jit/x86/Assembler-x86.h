





#ifndef jit_x86_Assembler_x86_h
#define jit_x86_Assembler_x86_h

#include "mozilla/ArrayUtils.h"

#include "jit/CompactBuffer.h"
#include "jit/IonCode.h"
#include "jit/JitCompartment.h"
#include "jit/shared/Assembler-shared.h"
#include "jit/shared/BaseAssembler-x86-shared.h"

namespace js {
namespace jit {

static MOZ_CONSTEXPR_VAR Register eax = { X86Registers::eax };
static MOZ_CONSTEXPR_VAR Register ecx = { X86Registers::ecx };
static MOZ_CONSTEXPR_VAR Register edx = { X86Registers::edx };
static MOZ_CONSTEXPR_VAR Register ebx = { X86Registers::ebx };
static MOZ_CONSTEXPR_VAR Register esp = { X86Registers::esp };
static MOZ_CONSTEXPR_VAR Register ebp = { X86Registers::ebp };
static MOZ_CONSTEXPR_VAR Register esi = { X86Registers::esi };
static MOZ_CONSTEXPR_VAR Register edi = { X86Registers::edi };

static MOZ_CONSTEXPR_VAR FloatRegister xmm0 = { X86Registers::xmm0 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm1 = { X86Registers::xmm1 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm2 = { X86Registers::xmm2 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm3 = { X86Registers::xmm3 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm4 = { X86Registers::xmm4 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm5 = { X86Registers::xmm5 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm6 = { X86Registers::xmm6 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm7 = { X86Registers::xmm7 };

static MOZ_CONSTEXPR_VAR Register InvalidReg = { X86Registers::invalid_reg };
static MOZ_CONSTEXPR_VAR FloatRegister InvalidFloatReg = { X86Registers::invalid_xmm };

static MOZ_CONSTEXPR_VAR Register JSReturnReg_Type = ecx;
static MOZ_CONSTEXPR_VAR Register JSReturnReg_Data = edx;
static MOZ_CONSTEXPR_VAR Register StackPointer = esp;
static MOZ_CONSTEXPR_VAR Register FramePointer = ebp;
static MOZ_CONSTEXPR_VAR Register ReturnReg = eax;
static MOZ_CONSTEXPR_VAR FloatRegister ReturnFloat32Reg = xmm0;
static MOZ_CONSTEXPR_VAR FloatRegister ScratchFloat32Reg = xmm7;
static MOZ_CONSTEXPR_VAR FloatRegister ReturnDoubleReg = xmm0;
static MOZ_CONSTEXPR_VAR FloatRegister ScratchDoubleReg = xmm7;
static MOZ_CONSTEXPR_VAR FloatRegister ReturnSimdReg = xmm0;
static MOZ_CONSTEXPR_VAR FloatRegister ScratchSimdReg = xmm7;


static MOZ_CONSTEXPR_VAR Register ArgumentsRectifierReg = esi;
static MOZ_CONSTEXPR_VAR Register CallTempReg0 = edi;
static MOZ_CONSTEXPR_VAR Register CallTempReg1 = eax;
static MOZ_CONSTEXPR_VAR Register CallTempReg2 = ebx;
static MOZ_CONSTEXPR_VAR Register CallTempReg3 = ecx;
static MOZ_CONSTEXPR_VAR Register CallTempReg4 = esi;
static MOZ_CONSTEXPR_VAR Register CallTempReg5 = edx;



static MOZ_CONSTEXPR_VAR Register ForkJoinGetSliceReg_cx = edi;
static MOZ_CONSTEXPR_VAR Register ForkJoinGetSliceReg_temp0 = ebx;
static MOZ_CONSTEXPR_VAR Register ForkJoinGetSliceReg_temp1 = ecx;
static MOZ_CONSTEXPR_VAR Register ForkJoinGetSliceReg_output = esi;


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
    ABIArg &current() { return current_; }
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
static const uint32_t ABIStackAlignment = 16;
#else
static const uint32_t ABIStackAlignment = 4;
#endif
static const uint32_t CodeAlignment = 8;





static const bool SupportsSimd = true;
static const uint32_t SimdStackAlignment = 16;

static const uint32_t AsmJSStackAlignment = SimdStackAlignment;

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

#include "jit/shared/Assembler-x86-shared.h"

namespace js {
namespace jit {

static inline void
PatchJump(CodeLocationJump jump, CodeLocationLabel label)
{
#ifdef DEBUG
    
    
    
    unsigned char *x = (unsigned char *)jump.raw() - 5;
    MOZ_ASSERT(((*x >= 0x80 && *x <= 0x8F) && *(x - 1) == 0x0F) ||
               (*x == 0xE9));
#endif
    X86Assembler::setRel32(jump.raw(), label.raw());
}
static inline void
PatchBackedge(CodeLocationJump &jump_, CodeLocationLabel label, JitRuntime::BackedgeTarget target)
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
    using AssemblerX86Shared::movsd;
    using AssemblerX86Shared::movss;
    using AssemblerX86Shared::retarget;
    using AssemblerX86Shared::cmpl;
    using AssemblerX86Shared::call;
    using AssemblerX86Shared::push;
    using AssemblerX86Shared::pop;

    static void TraceJumpRelocations(JSTracer *trc, JitCode *code, CompactBufferReader &reader);

    
    
    void executableCopy(uint8_t *buffer);

    

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
        movsd(src, Address(StackPointer, 0));
    }

    CodeOffsetLabel pushWithPatch(ImmWord word) {
        masm.push_i32(int32_t(word.value));
        return CodeOffsetLabel(masm.currentOffset());
    }

    void pop(FloatRegister src) {
        movsd(Address(StackPointer, 0), src);
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
        masm.movl_i32r(uintptr_t(ptr.value), dest.code());
        writeDataRelocation(ptr);
    }
    void movl(ImmGCPtr ptr, const Operand &dest) {
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
        masm.movl_i32r(imm.value, dest.code());
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
        masm.movl_i32r(-1, dest.code());
        append(AsmJSAbsoluteLink(CodeOffsetLabel(masm.currentOffset()), imm.kind()));
    }
    void mov(const Operand &src, Register dest) {
        movl(src, dest);
    }
    void mov(Register src, const Operand &dest) {
        movl(src, dest);
    }
    void mov(Imm32 imm, const Operand &dest) {
        movl(imm, dest);
    }
    void mov(AbsoluteLabel *label, Register dest) {
        MOZ_ASSERT(!label->bound());
        
        
        masm.movl_i32r(label->prev(), dest.code());
        label->setPrev(masm.size());
    }
    void mov(Register src, Register dest) {
        movl(src, dest);
    }
    void xchg(Register src, Register dest) {
        xchgl(src, dest);
    }
    void lea(const Operand &src, Register dest) {
        return leal(src, dest);
    }

    void fld32(const Operand &dest) {
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            masm.fld32_m(dest.disp(), dest.base());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }

    void fstp32(const Operand &src) {
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.fstp32_m(src.disp(), src.base());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }

    void cmpl(const Register src, ImmWord ptr) {
        masm.cmpl_ir(ptr.value, src.code());
    }
    void cmpl(const Register src, ImmPtr imm) {
        cmpl(src, ImmWord(uintptr_t(imm.value)));
    }
    void cmpl(const Register src, ImmGCPtr ptr) {
        masm.cmpl_i32r(uintptr_t(ptr.value), src.code());
        writeDataRelocation(ptr);
    }
    void cmpl(Register lhs, Register rhs) {
        masm.cmpl_rr(rhs.code(), lhs.code());
    }
    void cmpl(const Operand &op, ImmGCPtr imm) {
        switch (op.kind()) {
          case Operand::REG:
            masm.cmpl_i32r(uintptr_t(imm.value), op.reg());
            writeDataRelocation(imm);
            break;
          case Operand::MEM_REG_DISP:
            masm.cmpl_i32m(uintptr_t(imm.value), op.disp(), op.base());
            writeDataRelocation(imm);
            break;
          case Operand::MEM_ADDRESS32:
            masm.cmpl_i32m(uintptr_t(imm.value), op.address());
            writeDataRelocation(imm);
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void cmpl(const Operand &op, ImmMaybeNurseryPtr imm) {
        cmpl(op, noteMaybeNurseryPtr(imm));
    }
    void cmpl(AsmJSAbsoluteAddress lhs, Register rhs) {
        masm.cmpl_rm_disp32(rhs.code(), (void*)-1);
        append(AsmJSAbsoluteLink(CodeOffsetLabel(masm.currentOffset()), lhs.kind()));
    }
    void cmpl(AsmJSAbsoluteAddress lhs, Imm32 rhs) {
        JmpSrc src = masm.cmpl_im_disp32(rhs.value, (void*)-1);
        append(AsmJSAbsoluteLink(CodeOffsetLabel(src.offset()), lhs.kind()));
    }

    void jmp(ImmPtr target, Relocation::Kind reloc = Relocation::HARDCODED) {
        JmpSrc src = masm.jmp();
        addPendingJump(src, target, reloc);
    }
    void j(Condition cond, ImmPtr target,
           Relocation::Kind reloc = Relocation::HARDCODED) {
        JmpSrc src = masm.jCC(static_cast<X86Assembler::Condition>(cond));
        addPendingJump(src, target, reloc);
    }

    void jmp(JitCode *target) {
        jmp(ImmPtr(target->raw()), Relocation::JITCODE);
    }
    void j(Condition cond, JitCode *target) {
        j(cond, ImmPtr(target->raw()), Relocation::JITCODE);
    }
    void call(JitCode *target) {
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

    
    
    CodeOffsetLabel toggledCall(JitCode *target, bool enabled) {
        CodeOffsetLabel offset(size());
        JmpSrc src = enabled ? masm.call() : masm.cmp_eax();
        addPendingJump(src, ImmPtr(target->raw()), Relocation::JITCODE);
        MOZ_ASSERT(size() - offset.offset() == ToggledCallSize(nullptr));
        return offset;
    }

    static size_t ToggledCallSize(uint8_t *code) {
        
        return 5;
    }

    
    
    void retarget(Label *label, ImmPtr target, Relocation::Kind reloc) {
        if (label->used()) {
            bool more;
            X86Assembler::JmpSrc jmp(label->offset());
            do {
                X86Assembler::JmpSrc next;
                more = masm.nextJump(jmp, &next);
                addPendingJump(jmp, target, reloc);
                jmp = next;
            } while (more);
        }
        label->reset();
    }

    
    
    CodeOffsetLabel movlWithPatch(Imm32 imm, Register dest) {
        masm.movl_i32r(imm.value, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }

    
    CodeOffsetLabel movsblWithPatch(Address src, Register dest) {
        masm.movsbl_mr_disp32(src.offset, src.base.code(), dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movzblWithPatch(Address src, Register dest) {
        masm.movzbl_mr_disp32(src.offset, src.base.code(), dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movswlWithPatch(Address src, Register dest) {
        masm.movswl_mr_disp32(src.offset, src.base.code(), dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movzwlWithPatch(Address src, Register dest) {
        masm.movzwl_mr_disp32(src.offset, src.base.code(), dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movlWithPatch(Address src, Register dest) {
        masm.movl_mr_disp32(src.offset, src.base.code(), dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movssWithPatch(Address src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movss_mr_disp32(src.offset, src.base.code(), dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movsdWithPatch(Address src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movsd_mr_disp32(src.offset, src.base.code(), dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movupsWithPatch(Address src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movups_mr_disp32(src.offset, src.base.code(), dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movdquWithPatch(Address src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movdqu_mr_disp32(src.offset, src.base.code(), dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }

    
    CodeOffsetLabel movbWithPatch(Register src, Address dest) {
        masm.movb_rm_disp32(src.code(), dest.offset, dest.base.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movwWithPatch(Register src, Address dest) {
        masm.movw_rm_disp32(src.code(), dest.offset, dest.base.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movlWithPatch(Register src, Address dest) {
        masm.movl_rm_disp32(src.code(), dest.offset, dest.base.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movssWithPatch(FloatRegister src, Address dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movss_rm_disp32(src.code(), dest.offset, dest.base.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movsdWithPatch(FloatRegister src, Address dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movsd_rm_disp32(src.code(), dest.offset, dest.base.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movupsWithPatch(FloatRegister src, Address dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movups_rm_disp32(src.code(), dest.offset, dest.base.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movdquWithPatch(FloatRegister src, Address dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movdqu_rm_disp32(src.code(), dest.offset, dest.base.code());
        return CodeOffsetLabel(masm.currentOffset());
    }

    
    CodeOffsetLabel movlWithPatch(PatchedAbsoluteAddress addr, Register index, Scale scale,
                                  Register dest)
    {
        masm.movl_mr(addr.addr, index.code(), scale, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }

    
    CodeOffsetLabel movsblWithPatch(PatchedAbsoluteAddress src, Register dest) {
        masm.movsbl_mr(src.addr, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movzblWithPatch(PatchedAbsoluteAddress src, Register dest) {
        masm.movzbl_mr(src.addr, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movswlWithPatch(PatchedAbsoluteAddress src, Register dest) {
        masm.movswl_mr(src.addr, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movzwlWithPatch(PatchedAbsoluteAddress src, Register dest) {
        masm.movzwl_mr(src.addr, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movlWithPatch(PatchedAbsoluteAddress src, Register dest) {
        masm.movl_mr(src.addr, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movssWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movss_mr(src.addr, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movsdWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movsd_mr(src.addr, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movdqaWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movdqa_mr(src.addr, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movdquWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movdqu_mr(src.addr, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movapsWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movaps_mr(src.addr, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movupsWithPatch(PatchedAbsoluteAddress src, FloatRegister dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movups_mr(src.addr, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }

    
    CodeOffsetLabel movbWithPatch(Register src, PatchedAbsoluteAddress dest) {
        masm.movb_rm(src.code(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movwWithPatch(Register src, PatchedAbsoluteAddress dest) {
        masm.movw_rm(src.code(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movlWithPatch(Register src, PatchedAbsoluteAddress dest) {
        masm.movl_rm(src.code(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movssWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movss_rm(src.code(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movsdWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movsd_rm(src.code(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movdqaWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movdqa_rm(src.code(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movapsWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movaps_rm(src.code(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movdquWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movdqu_rm(src.code(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movupsWithPatch(FloatRegister src, PatchedAbsoluteAddress dest) {
        MOZ_ASSERT(HasSSE2());
        masm.movups_rm(src.code(), dest.addr);
        return CodeOffsetLabel(masm.currentOffset());
    }

    void loadAsmJSActivation(Register dest) {
        CodeOffsetLabel label = movlWithPatch(PatchedAbsoluteAddress(), dest);
        append(AsmJSGlobalAccess(label, AsmJSActivationGlobalDataOffset));
    }
    void loadAsmJSHeapRegisterFromGlobalData() {
        
    }

    static bool canUseInSingleByteInstruction(Register reg) {
        return !ByteRegRequiresRex(reg.code());
    }
};






static inline bool
GetTempRegForIntArg(uint32_t usedIntArgs, uint32_t usedFloatArgs, Register *out)
{
    if (usedIntArgs >= NumCallTempNonArgRegs)
        return false;
    *out = CallTempNonArgRegs[usedIntArgs];
    return true;
}

} 
} 

#endif 
