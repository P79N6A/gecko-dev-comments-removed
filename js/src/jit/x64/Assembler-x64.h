





#ifndef jit_x64_Assembler_x64_h
#define jit_x64_Assembler_x64_h

#include "mozilla/ArrayUtils.h"

#include "jit/IonCode.h"
#include "jit/JitCompartment.h"
#include "jit/shared/Assembler-shared.h"

namespace js {
namespace jit {

static MOZ_CONSTEXPR_VAR Register rax = { X86Registers::eax };
static MOZ_CONSTEXPR_VAR Register rbx = { X86Registers::ebx };
static MOZ_CONSTEXPR_VAR Register rcx = { X86Registers::ecx };
static MOZ_CONSTEXPR_VAR Register rdx = { X86Registers::edx };
static MOZ_CONSTEXPR_VAR Register rsi = { X86Registers::esi };
static MOZ_CONSTEXPR_VAR Register rdi = { X86Registers::edi };
static MOZ_CONSTEXPR_VAR Register rbp = { X86Registers::ebp };
static MOZ_CONSTEXPR_VAR Register r8  = { X86Registers::r8  };
static MOZ_CONSTEXPR_VAR Register r9  = { X86Registers::r9  };
static MOZ_CONSTEXPR_VAR Register r10 = { X86Registers::r10 };
static MOZ_CONSTEXPR_VAR Register r11 = { X86Registers::r11 };
static MOZ_CONSTEXPR_VAR Register r12 = { X86Registers::r12 };
static MOZ_CONSTEXPR_VAR Register r13 = { X86Registers::r13 };
static MOZ_CONSTEXPR_VAR Register r14 = { X86Registers::r14 };
static MOZ_CONSTEXPR_VAR Register r15 = { X86Registers::r15 };
static MOZ_CONSTEXPR_VAR Register rsp = { X86Registers::esp };

static MOZ_CONSTEXPR_VAR FloatRegister xmm0 = { X86Registers::xmm0 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm1 = { X86Registers::xmm1 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm2 = { X86Registers::xmm2 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm3 = { X86Registers::xmm3 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm4 = { X86Registers::xmm4 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm5 = { X86Registers::xmm5 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm6 = { X86Registers::xmm6 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm7 = { X86Registers::xmm7 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm8 = { X86Registers::xmm8 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm9 = { X86Registers::xmm9 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm10 = { X86Registers::xmm10 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm11 = { X86Registers::xmm11 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm12 = { X86Registers::xmm12 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm13 = { X86Registers::xmm13 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm14 = { X86Registers::xmm14 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm15 = { X86Registers::xmm15 };


static MOZ_CONSTEXPR_VAR Register eax = rax;
static MOZ_CONSTEXPR_VAR Register ebx = rbx;
static MOZ_CONSTEXPR_VAR Register ecx = rcx;
static MOZ_CONSTEXPR_VAR Register edx = rdx;
static MOZ_CONSTEXPR_VAR Register esi = rsi;
static MOZ_CONSTEXPR_VAR Register edi = rdi;
static MOZ_CONSTEXPR_VAR Register ebp = rbp;
static MOZ_CONSTEXPR_VAR Register esp = rsp;

static MOZ_CONSTEXPR_VAR Register InvalidReg = { X86Registers::invalid_reg };
static MOZ_CONSTEXPR_VAR FloatRegister InvalidFloatReg = { X86Registers::invalid_xmm };

static MOZ_CONSTEXPR_VAR Register StackPointer = rsp;
static MOZ_CONSTEXPR_VAR Register FramePointer = rbp;
static MOZ_CONSTEXPR_VAR Register JSReturnReg = rcx;

static MOZ_CONSTEXPR_VAR Register JSReturnReg_Type = JSReturnReg;
static MOZ_CONSTEXPR_VAR Register JSReturnReg_Data = JSReturnReg;

static MOZ_CONSTEXPR_VAR Register ReturnReg = rax;
static MOZ_CONSTEXPR_VAR Register ScratchReg = r11;
static MOZ_CONSTEXPR_VAR Register HeapReg = r15;
static MOZ_CONSTEXPR_VAR FloatRegister ReturnFloat32Reg = xmm0;
static MOZ_CONSTEXPR_VAR FloatRegister ScratchFloat32Reg = xmm15;
static MOZ_CONSTEXPR_VAR FloatRegister ReturnDoubleReg = xmm0;
static MOZ_CONSTEXPR_VAR FloatRegister ScratchDoubleReg = xmm15;
static MOZ_CONSTEXPR_VAR FloatRegister ReturnSimdReg = xmm0;
static MOZ_CONSTEXPR_VAR FloatRegister ScratchSimdReg = xmm15;


static MOZ_CONSTEXPR_VAR Register ArgumentsRectifierReg = r8;
static MOZ_CONSTEXPR_VAR Register CallTempReg0 = rax;
static MOZ_CONSTEXPR_VAR Register CallTempReg1 = rdi;
static MOZ_CONSTEXPR_VAR Register CallTempReg2 = rbx;
static MOZ_CONSTEXPR_VAR Register CallTempReg3 = rcx;
static MOZ_CONSTEXPR_VAR Register CallTempReg4 = rsi;
static MOZ_CONSTEXPR_VAR Register CallTempReg5 = rdx;


#if defined(_WIN64)
static MOZ_CONSTEXPR_VAR Register IntArgReg0 = rcx;
static MOZ_CONSTEXPR_VAR Register IntArgReg1 = rdx;
static MOZ_CONSTEXPR_VAR Register IntArgReg2 = r8;
static MOZ_CONSTEXPR_VAR Register IntArgReg3 = r9;
static MOZ_CONSTEXPR_VAR uint32_t NumIntArgRegs = 4;
static MOZ_CONSTEXPR_VAR Register IntArgRegs[NumIntArgRegs] = { rcx, rdx, r8, r9 };

static MOZ_CONSTEXPR_VAR Register CallTempNonArgRegs[] = { rax, rdi, rbx, rsi };
static const uint32_t NumCallTempNonArgRegs =
    mozilla::ArrayLength(CallTempNonArgRegs);

static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg0 = xmm0;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg1 = xmm1;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg2 = xmm2;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg3 = xmm3;
static const uint32_t NumFloatArgRegs = 4;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgRegs[NumFloatArgRegs] = { xmm0, xmm1, xmm2, xmm3 };
#else
static MOZ_CONSTEXPR_VAR Register IntArgReg0 = rdi;
static MOZ_CONSTEXPR_VAR Register IntArgReg1 = rsi;
static MOZ_CONSTEXPR_VAR Register IntArgReg2 = rdx;
static MOZ_CONSTEXPR_VAR Register IntArgReg3 = rcx;
static MOZ_CONSTEXPR_VAR Register IntArgReg4 = r8;
static MOZ_CONSTEXPR_VAR Register IntArgReg5 = r9;
static MOZ_CONSTEXPR_VAR uint32_t NumIntArgRegs = 6;
static MOZ_CONSTEXPR_VAR Register IntArgRegs[NumIntArgRegs] = { rdi, rsi, rdx, rcx, r8, r9 };

static MOZ_CONSTEXPR_VAR Register CallTempNonArgRegs[] = { rax, rbx };
static const uint32_t NumCallTempNonArgRegs =
    mozilla::ArrayLength(CallTempNonArgRegs);

static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg0 = xmm0;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg1 = xmm1;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg2 = xmm2;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg3 = xmm3;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg4 = xmm4;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg5 = xmm5;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg6 = xmm6;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgReg7 = xmm7;
static MOZ_CONSTEXPR_VAR uint32_t NumFloatArgRegs = 8;
static MOZ_CONSTEXPR_VAR FloatRegister FloatArgRegs[NumFloatArgRegs] = { xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7 };
#endif



static MOZ_CONSTEXPR_VAR Register ForkJoinGetSliceReg_cx = rdi;
static MOZ_CONSTEXPR_VAR Register ForkJoinGetSliceReg_temp0 = rbx;
static MOZ_CONSTEXPR_VAR Register ForkJoinGetSliceReg_temp1 = rcx;
static MOZ_CONSTEXPR_VAR Register ForkJoinGetSliceReg_output = rsi;


static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegCallee = r10;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE0 = rax;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE1 = rdi;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE2 = rbx;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE3 = rsi;


static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegReturnData = ecx;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegReturnType = ecx;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegD0 = rax;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegD1 = rdi;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegD2 = rbx;

class ABIArgGenerator
{
#if defined(XP_WIN)
    unsigned regIndex_;
#else
    unsigned intRegIndex_;
    unsigned floatRegIndex_;
#endif
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

static MOZ_CONSTEXPR_VAR Register OsrFrameReg = IntArgReg3;

static MOZ_CONSTEXPR_VAR Register PreBarrierReg = rdx;

static const uint32_t ABIStackAlignment = 16;
static const uint32_t CodeAlignment = 8;





static const bool SupportsSimd = true;
static const uint32_t SimdStackAlignment = 16;

static const uint32_t AsmJSStackAlignment = SimdStackAlignment;

static const Scale ScalePointer = TimesEight;

} 
} 

#include "jit/shared/Assembler-x86-shared.h"

namespace js {
namespace jit {


static MOZ_CONSTEXPR_VAR ValueOperand JSReturnOperand = ValueOperand(JSReturnReg);

class Assembler : public AssemblerX86Shared
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static const uint32_t SizeOfExtendedJump = 1 + 1 + 4 + 2 + 8;
    static const uint32_t SizeOfJumpTableEntry = 16;

    uint32_t extendedJumpTable_;

    static JitCode *CodeFromJump(JitCode *code, uint8_t *jump);

  private:
    void writeRelocation(JmpSrc src, Relocation::Kind reloc);
    void addPendingJump(JmpSrc src, ImmPtr target, Relocation::Kind reloc);

  protected:
    size_t addPatchableJump(JmpSrc src, Relocation::Kind reloc);

  public:
    using AssemblerX86Shared::j;
    using AssemblerX86Shared::jmp;
    using AssemblerX86Shared::push;
    using AssemblerX86Shared::pop;

    static uint8_t *PatchableJumpAddress(JitCode *code, size_t index);
    static void PatchJumpEntry(uint8_t *entry, uint8_t *target);

    Assembler()
      : extendedJumpTable_(0)
    {
    }

    static void TraceJumpRelocations(JSTracer *trc, JitCode *code, CompactBufferReader &reader);

    
    
    void finish();

    
    
    void executableCopy(uint8_t *buffer);

    

    void push(const ImmGCPtr ptr) {
        movq(ptr, ScratchReg);
        push(ScratchReg);
    }
    void push(const ImmWord ptr) {
        
        
        if (ptr.value <= INT32_MAX) {
            push(Imm32(ptr.value));
        } else {
            movq(ptr, ScratchReg);
            push(ScratchReg);
        }
    }
    void push(ImmPtr imm) {
        push(ImmWord(uintptr_t(imm.value)));
    }
    void push(FloatRegister src) {
        subq(Imm32(sizeof(double)), StackPointer);
        movsd(src, Address(StackPointer, 0));
    }
    CodeOffsetLabel pushWithPatch(ImmWord word) {
        CodeOffsetLabel label = movWithPatch(word, ScratchReg);
        push(ScratchReg);
        return label;
    }

    void pop(FloatRegister src) {
        movsd(Address(StackPointer, 0), src);
        addq(Imm32(sizeof(double)), StackPointer);
    }

    CodeOffsetLabel movWithPatch(ImmWord word, Register dest) {
        masm.movq_i64r(word.value, dest.code());
        return CodeOffsetLabel(masm.currentOffset());
    }
    CodeOffsetLabel movWithPatch(ImmPtr imm, Register dest) {
        return movWithPatch(ImmWord(uintptr_t(imm.value)), dest);
    }

    
    
    
    void movq(ImmWord word, Register dest) {
        
        
        
        if (word.value <= UINT32_MAX) {
            
            masm.movl_i32r((uint32_t)word.value, dest.code());
        } else if ((intptr_t)word.value >= INT32_MIN && (intptr_t)word.value <= INT32_MAX) {
            
            masm.movq_i32r((int32_t)(intptr_t)word.value, dest.code());
        } else {
            
            masm.movq_i64r(word.value, dest.code());
        }
    }
    void movq(ImmPtr imm, Register dest) {
        movq(ImmWord(uintptr_t(imm.value)), dest);
    }
    void movq(ImmGCPtr ptr, Register dest) {
        masm.movq_i64r(uintptr_t(ptr.value), dest.code());
        writeDataRelocation(ptr);
    }
    void movq(const Operand &src, Register dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.movq_rr(src.reg(), dest.code());
            break;
          case Operand::MEM_REG_DISP:
            masm.movq_mr(src.disp(), src.base(), dest.code());
            break;
          case Operand::MEM_SCALE:
            masm.movq_mr(src.disp(), src.base(), src.index(), src.scale(), dest.code());
            break;
          case Operand::MEM_ADDRESS32:
            masm.movq_mr(src.address(), dest.code());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void movq(Register src, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::REG:
            masm.movq_rr(src.code(), dest.reg());
            break;
          case Operand::MEM_REG_DISP:
            masm.movq_rm(src.code(), dest.disp(), dest.base());
            break;
          case Operand::MEM_SCALE:
            masm.movq_rm(src.code(), dest.disp(), dest.base(), dest.index(), dest.scale());
            break;
          case Operand::MEM_ADDRESS32:
            masm.movq_rm(src.code(), dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void movq(Imm32 imm32, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::REG:
            masm.movl_i32r(imm32.value, dest.reg());
            break;
          case Operand::MEM_REG_DISP:
            masm.movq_i32m(imm32.value, dest.disp(), dest.base());
            break;
          case Operand::MEM_SCALE:
            masm.movq_i32m(imm32.value, dest.disp(), dest.base(), dest.index(), dest.scale());
            break;
          case Operand::MEM_ADDRESS32:
            masm.movq_i32m(imm32.value, dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void movq(Register src, FloatRegister dest) {
        masm.movq_rr(src.code(), dest.code());
    }
    void movq(FloatRegister src, Register dest) {
        masm.movq_rr(src.code(), dest.code());
    }
    void movq(Register src, Register dest) {
        masm.movq_rr(src.code(), dest.code());
    }

    void xchgq(Register src, Register dest) {
        masm.xchgq_rr(src.code(), dest.code());
    }

    void andq(Register src, Register dest) {
        masm.andq_rr(src.code(), dest.code());
    }
    void andq(Imm32 imm, Register dest) {
        masm.andq_ir(imm.value, dest.code());
    }
    void andq(const Operand &src, Register dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.andq_rr(src.reg(), dest.code());
            break;
          case Operand::MEM_REG_DISP:
            masm.andq_mr(src.disp(), src.base(), dest.code());
            break;
          case Operand::MEM_SCALE:
            masm.andq_mr(src.disp(), src.base(), src.index(), src.scale(), dest.code());
            break;
          case Operand::MEM_ADDRESS32:
            masm.andq_mr(src.address(), dest.code());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }

    void addq(Imm32 imm, Register dest) {
        masm.addq_ir(imm.value, dest.code());
    }
    void addq(Imm32 imm, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::REG:
            masm.addq_ir(imm.value, dest.reg());
            break;
          case Operand::MEM_REG_DISP:
            masm.addq_im(imm.value, dest.disp(), dest.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.addq_im(imm.value, dest.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void addq(Register src, Register dest) {
        masm.addq_rr(src.code(), dest.code());
    }
    void addq(const Operand &src, Register dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.addq_rr(src.reg(), dest.code());
            break;
          case Operand::MEM_REG_DISP:
            masm.addq_mr(src.disp(), src.base(), dest.code());
            break;
          case Operand::MEM_ADDRESS32:
            masm.addq_mr(src.address(), dest.code());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }

    void subq(Imm32 imm, Register dest) {
        masm.subq_ir(imm.value, dest.code());
    }
    void subq(Register src, Register dest) {
        masm.subq_rr(src.code(), dest.code());
    }
    void subq(const Operand &src, Register dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.subq_rr(src.reg(), dest.code());
            break;
          case Operand::MEM_REG_DISP:
            masm.subq_mr(src.disp(), src.base(), dest.code());
            break;
          case Operand::MEM_ADDRESS32:
            masm.subq_mr(src.address(), dest.code());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void subq(Register src, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::REG:
            masm.subq_rr(src.code(), dest.reg());
            break;
          case Operand::MEM_REG_DISP:
            masm.subq_rm(src.code(), dest.disp(), dest.base());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void shlq(Imm32 imm, Register dest) {
        masm.shlq_i8r(imm.value, dest.code());
    }
    void shrq(Imm32 imm, Register dest) {
        masm.shrq_i8r(imm.value, dest.code());
    }
    void sarq(Imm32 imm, Register dest) {
        masm.sarq_i8r(imm.value, dest.code());
    }
    void orq(Imm32 imm, Register dest) {
        masm.orq_ir(imm.value, dest.code());
    }
    void orq(Register src, Register dest) {
        masm.orq_rr(src.code(), dest.code());
    }
    void orq(const Operand &src, Register dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.orq_rr(src.reg(), dest.code());
            break;
          case Operand::MEM_REG_DISP:
            masm.orq_mr(src.disp(), src.base(), dest.code());
            break;
          case Operand::MEM_ADDRESS32:
            masm.orq_mr(src.address(), dest.code());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void xorq(Register src, Register dest) {
        masm.xorq_rr(src.code(), dest.code());
    }
    void xorq(Imm32 imm, Register dest) {
        masm.xorq_ir(imm.value, dest.code());
    }

    void mov(ImmWord word, Register dest) {
        
        
        
        
        
        if (word.value == 0)
            xorl(dest, dest);
        else
            movq(word, dest);
    }
    void mov(ImmPtr imm, Register dest) {
        movq(imm, dest);
    }
    void mov(AsmJSImmPtr imm, Register dest) {
        masm.movq_i64r(-1, dest.code());
        append(AsmJSAbsoluteLink(CodeOffsetLabel(masm.currentOffset()), imm.kind()));
    }
    void mov(const Operand &src, Register dest) {
        movq(src, dest);
    }
    void mov(Register src, const Operand &dest) {
        movq(src, dest);
    }
    void mov(Imm32 imm32, const Operand &dest) {
        movq(imm32, dest);
    }
    void mov(Register src, Register dest) {
        movq(src, dest);
    }
    void mov(AbsoluteLabel *label, Register dest) {
        JS_ASSERT(!label->bound());
        
        
        masm.movq_i64r(label->prev(), dest.code());
        label->setPrev(masm.size());
    }
    void xchg(Register src, Register dest) {
        xchgq(src, dest);
    }
    void lea(const Operand &src, Register dest) {
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            masm.leaq_mr(src.disp(), src.base(), dest.code());
            break;
          case Operand::MEM_SCALE:
            masm.leaq_mr(src.disp(), src.base(), src.index(), src.scale(), dest.code());
            break;
          default:
            MOZ_CRASH("unexepcted operand kind");
        }
    }

    CodeOffsetLabel loadRipRelativeInt32(Register dest) {
        return CodeOffsetLabel(masm.movl_ripr(dest.code()).offset());
    }
    CodeOffsetLabel loadRipRelativeInt64(Register dest) {
        return CodeOffsetLabel(masm.movq_ripr(dest.code()).offset());
    }
    CodeOffsetLabel loadRipRelativeDouble(FloatRegister dest) {
        return CodeOffsetLabel(masm.movsd_ripr(dest.code()).offset());
    }
    CodeOffsetLabel storeRipRelativeInt32(Register dest) {
        return CodeOffsetLabel(masm.movl_rrip(dest.code()).offset());
    }
    CodeOffsetLabel storeRipRelativeDouble(FloatRegister dest) {
        return CodeOffsetLabel(masm.movsd_rrip(dest.code()).offset());
    }
    CodeOffsetLabel leaRipRelative(Register dest) {
        return CodeOffsetLabel(masm.leaq_rip(dest.code()).offset());
    }

    void loadAsmJSActivation(Register dest) {
        CodeOffsetLabel label = loadRipRelativeInt64(dest);
        append(AsmJSGlobalAccess(label, AsmJSActivationGlobalDataOffset));
    }

    
    
    
    void cmpq(const Operand &lhs, Register rhs) {
        switch (lhs.kind()) {
          case Operand::REG:
            masm.cmpq_rr(rhs.code(), lhs.reg());
            break;
          case Operand::MEM_REG_DISP:
            masm.cmpq_rm(rhs.code(), lhs.disp(), lhs.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.cmpq_rm(rhs.code(), lhs.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void cmpq(const Operand &lhs, Imm32 rhs) {
        switch (lhs.kind()) {
          case Operand::REG:
            masm.cmpq_ir(rhs.value, lhs.reg());
            break;
          case Operand::MEM_REG_DISP:
            masm.cmpq_im(rhs.value, lhs.disp(), lhs.base());
            break;
          case Operand::MEM_ADDRESS32:
            masm.cmpq_im(rhs.value, lhs.address());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void cmpq(Register lhs, const Operand &rhs) {
        switch (rhs.kind()) {
          case Operand::REG:
            masm.cmpq_rr(rhs.reg(), lhs.code());
            break;
          case Operand::MEM_REG_DISP:
            masm.cmpq_mr(rhs.disp(), rhs.base(), lhs.code());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void cmpq(Register lhs, Register rhs) {
        masm.cmpq_rr(rhs.code(), lhs.code());
    }
    void cmpq(Register lhs, Imm32 rhs) {
        masm.cmpq_ir(rhs.value, lhs.code());
    }

    void testq(Register lhs, Imm32 rhs) {
        masm.testq_i32r(rhs.value, lhs.code());
    }
    void testq(Register lhs, Register rhs) {
        masm.testq_rr(rhs.code(), lhs.code());
    }
    void testq(const Operand &lhs, Imm32 rhs) {
        switch (lhs.kind()) {
          case Operand::REG:
            masm.testq_i32r(rhs.value, lhs.reg());
            break;
          case Operand::MEM_REG_DISP:
            masm.testq_i32m(rhs.value, lhs.disp(), lhs.base());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
            break;
        }
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

    
    
    CodeOffsetLabel toggledCall(JitCode *target, bool enabled) {
        CodeOffsetLabel offset(size());
        JmpSrc src = enabled ? masm.call() : masm.cmp_eax();
        addPendingJump(src, ImmPtr(target->raw()), Relocation::JITCODE);
        JS_ASSERT(size() - offset.offset() == ToggledCallSize(nullptr));
        return offset;
    }

    static size_t ToggledCallSize(uint8_t *code) {
        
        return 5;
    }

    
    using AssemblerX86Shared::call;

    void cvttsd2sq(FloatRegister src, Register dest) {
        masm.cvttsd2sq_rr(src.code(), dest.code());
    }
    void cvttss2sq(FloatRegister src, Register dest) {
        masm.cvttss2sq_rr(src.code(), dest.code());
    }
    void cvtsq2sd(Register src, FloatRegister dest) {
        masm.cvtsq2sd_rr(src.code(), dest.code());
    }
    void cvtsq2ss(Register src, FloatRegister dest) {
        masm.cvtsq2ss_rr(src.code(), dest.code());
    }
};

static inline void
PatchJump(CodeLocationJump jump, CodeLocationLabel label)
{
    if (X86Assembler::canRelinkJump(jump.raw(), label.raw())) {
        X86Assembler::setRel32(jump.raw(), label.raw());
    } else {
        X86Assembler::setRel32(jump.raw(), jump.jumpTableEntry());
        Assembler::PatchJumpEntry(jump.jumpTableEntry(), label.raw());
    }
}
static inline void
PatchBackedge(CodeLocationJump &jump_, CodeLocationLabel label, JitRuntime::BackedgeTarget target)
{
    PatchJump(jump_, label);
}

static inline bool
GetIntArgReg(uint32_t intArg, uint32_t floatArg, Register *out)
{
#if defined(_WIN64)
    uint32_t arg = intArg + floatArg;
#else
    uint32_t arg = intArg;
#endif
    if (arg >= NumIntArgRegs)
        return false;
    *out = IntArgRegs[arg];
    return true;
}






static inline bool
GetTempRegForIntArg(uint32_t usedIntArgs, uint32_t usedFloatArgs, Register *out)
{
    if (GetIntArgReg(usedIntArgs, usedFloatArgs, out))
        return true;
    
    
    
#if defined(_WIN64)
    uint32_t arg = usedIntArgs + usedFloatArgs;
#else
    uint32_t arg = usedIntArgs;
#endif
    arg -= NumIntArgRegs;
    if (arg >= NumCallTempNonArgRegs)
        return false;
    *out = CallTempNonArgRegs[arg];
    return true;
}

static inline bool
GetFloatArgReg(uint32_t intArg, uint32_t floatArg, FloatRegister *out)
{
#if defined(_WIN64)
    uint32_t arg = intArg + floatArg;
#else
    uint32_t arg = floatArg;
#endif
    if (floatArg >= NumFloatArgRegs)
        return false;
    *out = FloatArgRegs[arg];
    return true;
}

} 
} 

#endif 
