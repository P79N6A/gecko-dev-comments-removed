





#ifndef ion_x64_Assembler_x64_h
#define ion_x64_Assembler_x64_h

#include "ion/shared/Assembler-shared.h"
#include "ion/CompactBuffer.h"
#include "ion/IonCode.h"
#include "mozilla/Util.h"

namespace js {
namespace ion {

static MOZ_CONSTEXPR_VAR Register rax = { JSC::X86Registers::eax };
static MOZ_CONSTEXPR_VAR Register rbx = { JSC::X86Registers::ebx };
static MOZ_CONSTEXPR_VAR Register rcx = { JSC::X86Registers::ecx };
static MOZ_CONSTEXPR_VAR Register rdx = { JSC::X86Registers::edx };
static MOZ_CONSTEXPR_VAR Register rsi = { JSC::X86Registers::esi };
static MOZ_CONSTEXPR_VAR Register rdi = { JSC::X86Registers::edi };
static MOZ_CONSTEXPR_VAR Register rbp = { JSC::X86Registers::ebp };
static MOZ_CONSTEXPR_VAR Register r8  = { JSC::X86Registers::r8  };
static MOZ_CONSTEXPR_VAR Register r9  = { JSC::X86Registers::r9  };
static MOZ_CONSTEXPR_VAR Register r10 = { JSC::X86Registers::r10 };
static MOZ_CONSTEXPR_VAR Register r11 = { JSC::X86Registers::r11 };
static MOZ_CONSTEXPR_VAR Register r12 = { JSC::X86Registers::r12 };
static MOZ_CONSTEXPR_VAR Register r13 = { JSC::X86Registers::r13 };
static MOZ_CONSTEXPR_VAR Register r14 = { JSC::X86Registers::r14 };
static MOZ_CONSTEXPR_VAR Register r15 = { JSC::X86Registers::r15 };
static MOZ_CONSTEXPR_VAR Register rsp = { JSC::X86Registers::esp };

static MOZ_CONSTEXPR_VAR FloatRegister xmm0 = { JSC::X86Registers::xmm0 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm1 = { JSC::X86Registers::xmm1 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm2 = { JSC::X86Registers::xmm2 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm3 = { JSC::X86Registers::xmm3 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm4 = { JSC::X86Registers::xmm4 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm5 = { JSC::X86Registers::xmm5 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm6 = { JSC::X86Registers::xmm6 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm7 = { JSC::X86Registers::xmm7 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm8 = { JSC::X86Registers::xmm8 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm9 = { JSC::X86Registers::xmm9 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm10 = { JSC::X86Registers::xmm10 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm11 = { JSC::X86Registers::xmm11 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm12 = { JSC::X86Registers::xmm12 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm13 = { JSC::X86Registers::xmm13 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm14 = { JSC::X86Registers::xmm14 };
static MOZ_CONSTEXPR_VAR FloatRegister xmm15 = { JSC::X86Registers::xmm15 };


static const Register eax = rax;
static const Register ebx = rbx;
static const Register ecx = rcx;
static const Register edx = rdx;
static const Register esi = rsi;
static const Register edi = rdi;
static const Register ebp = rbp;
static const Register esp = rsp;

static MOZ_CONSTEXPR_VAR Register InvalidReg = { JSC::X86Registers::invalid_reg };
static MOZ_CONSTEXPR_VAR FloatRegister InvalidFloatReg = { JSC::X86Registers::invalid_xmm };

static const Register StackPointer = rsp;
static const Register FramePointer = rbp;
static MOZ_CONSTEXPR_VAR Register JSReturnReg = rcx;

static MOZ_CONSTEXPR_VAR Register JSReturnReg_Type = JSReturnReg;
static MOZ_CONSTEXPR_VAR Register JSReturnReg_Data = JSReturnReg;

static MOZ_CONSTEXPR_VAR Register ReturnReg = rax;
static MOZ_CONSTEXPR_VAR Register ScratchReg = r11;
static MOZ_CONSTEXPR_VAR Register HeapReg = r15;
static MOZ_CONSTEXPR_VAR FloatRegister ReturnFloatReg = xmm0;
static MOZ_CONSTEXPR_VAR FloatRegister ScratchFloatReg = xmm15;

static MOZ_CONSTEXPR_VAR Register ArgumentsRectifierReg = r8;
static MOZ_CONSTEXPR_VAR Register CallTempReg0 = rax;
static MOZ_CONSTEXPR_VAR Register CallTempReg1 = rdi;
static MOZ_CONSTEXPR_VAR Register CallTempReg2 = rbx;
static MOZ_CONSTEXPR_VAR Register CallTempReg3 = rcx;
static MOZ_CONSTEXPR_VAR Register CallTempReg4 = rsi;
static MOZ_CONSTEXPR_VAR Register CallTempReg5 = rdx;
static MOZ_CONSTEXPR_VAR Register CallTempReg6 = rbp;


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
static const FloatRegister FloatArgRegs[NumFloatArgRegs] = { xmm0, xmm1, xmm2, xmm3 };
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

    
    static const Register NonArgReturnVolatileReg0;
    static const Register NonArgReturnVolatileReg1;
    static const Register NonVolatileReg;
};

static MOZ_CONSTEXPR_VAR Register OsrFrameReg = IntArgReg3;

static MOZ_CONSTEXPR_VAR Register PreBarrierReg = rdx;



static const uint32_t StackAlignment = 16;
static const bool StackKeptAligned = false;
static const uint32_t CodeAlignment = 8;
static const uint32_t NativeFrameSize = sizeof(void*);
static const uint32_t AlignmentAtPrologue = sizeof(void*);
static const uint32_t AlignmentMidPrologue = AlignmentAtPrologue;

static const Scale ScalePointer = TimesEight;

class Operand
{
  public:
    enum Kind {
        REG,
        REG_DISP,
        FPREG,
        SCALE
    };

    Kind kind_ : 3;
    int32_t base_ : 5;
    Scale scale_ : 3;
    int32_t index_ : 5;
    int32_t disp_;

  public:
    explicit Operand(Register reg)
      : kind_(REG),
        base_(reg.code())
    { }
    explicit Operand(FloatRegister reg)
      : kind_(FPREG),
        base_(reg.code())
    { }
    explicit Operand(const Address &address)
      : kind_(REG_DISP),
        base_(address.base.code()),
        disp_(address.offset)
    { }
    explicit Operand(const BaseIndex &address)
      : kind_(SCALE),
        base_(address.base.code()),
        scale_(address.scale),
        index_(address.index.code()),
        disp_(address.offset)
    { }
    Operand(Register base, Register index, Scale scale, int32_t disp = 0)
      : kind_(SCALE),
        base_(base.code()),
        scale_(scale),
        index_(index.code()),
        disp_(disp)
    { }
    Operand(Register reg, int32_t disp)
      : kind_(REG_DISP),
        base_(reg.code()),
        disp_(disp)
    { }

    Address toAddress() const {
        JS_ASSERT(kind() == REG_DISP);
        return Address(Register::FromCode(base()), disp());
    }

    BaseIndex toBaseIndex() const {
        JS_ASSERT(kind() == SCALE);
        return BaseIndex(Register::FromCode(base()), Register::FromCode(index()), scale(), disp());
    }

    Kind kind() const {
        return kind_;
    }
    Register::Code reg() const {
        JS_ASSERT(kind() == REG);
        return (Registers::Code)base_;
    }
    Registers::Code base() const {
        JS_ASSERT(kind() == REG_DISP || kind() == SCALE);
        return (Registers::Code)base_;
    }
    Registers::Code index() const {
        JS_ASSERT(kind() == SCALE);
        return (Registers::Code)index_;
    }
    Scale scale() const {
        JS_ASSERT(kind() == SCALE);
        return scale_;
    }
    FloatRegisters::Code fpu() const {
        JS_ASSERT(kind() == FPREG);
        return (FloatRegisters::Code)base_;
    }
    int32_t disp() const {
        JS_ASSERT(kind() == REG_DISP || kind() == SCALE);
        return disp_;
    }
};

} 
} 

#include "ion/shared/Assembler-x86-shared.h"

namespace js {
namespace ion {


static MOZ_CONSTEXPR_VAR ValueOperand JSReturnOperand = ValueOperand(JSReturnReg);

class Assembler : public AssemblerX86Shared
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static const uint32_t SizeOfExtendedJump = 1 + 1 + 4 + 8;
    static const uint32_t SizeOfJumpTableEntry = 16;

    uint32_t extendedJumpTable_;

    static IonCode *CodeFromJump(IonCode *code, uint8_t *jump);

  private:
    void writeRelocation(JmpSrc src, Relocation::Kind reloc);
    void addPendingJump(JmpSrc src, void *target, Relocation::Kind reloc);

  protected:
    size_t addPatchableJump(JmpSrc src, Relocation::Kind reloc);

  public:
    using AssemblerX86Shared::j;
    using AssemblerX86Shared::jmp;
    using AssemblerX86Shared::push;
    using AssemblerX86Shared::pop;

    static uint8_t *PatchableJumpAddress(IonCode *code, size_t index);
    static void PatchJumpEntry(uint8_t *entry, uint8_t *target);

    Assembler()
      : extendedJumpTable_(0)
    {
    }

    static void TraceJumpRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader);

    
    
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
    void push(const FloatRegister &src) {
        subq(Imm32(sizeof(double)), StackPointer);
        movsd(src, Operand(StackPointer, 0));
    }
    CodeOffsetLabel pushWithPatch(const ImmWord &word) {
        CodeOffsetLabel label = movWithPatch(word, ScratchReg);
        push(ScratchReg);
        return label;
    }

    void pop(const FloatRegister &src) {
        movsd(Operand(StackPointer, 0), src);
        addq(Imm32(sizeof(double)), StackPointer);
    }

    CodeOffsetLabel movWithPatch(const ImmWord &word, const Register &dest) {
        movq(word, dest);
        return masm.currentOffset();
    }

    void movq(ImmWord word, const Register &dest) {
        masm.movq_i64r(word.value, dest.code());
    }
    void movq(ImmGCPtr ptr, const Register &dest) {
        masm.movq_i64r(ptr.value, dest.code());
        writeDataRelocation(ptr);
    }
    void movq(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.movq_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.movq_mr(src.disp(), src.base(), dest.code());
            break;
          case Operand::SCALE:
            masm.movq_mr(src.disp(), src.base(), src.index(), src.scale(), dest.code());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected operand kind");
        }
    }
    void movq(const Register &src, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::REG:
            masm.movq_rr(src.code(), dest.reg());
            break;
          case Operand::REG_DISP:
            masm.movq_rm(src.code(), dest.disp(), dest.base());
            break;
          case Operand::SCALE:
            masm.movq_rm(src.code(), dest.disp(), dest.base(), dest.index(), dest.scale());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected operand kind");
        }
    }
    void movq(Imm32 imm32, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::REG:
            masm.movl_i32r(imm32.value, dest.reg());
            break;
          case Operand::REG_DISP:
            masm.movq_i32m(imm32.value, dest.disp(), dest.base());
            break;
          case Operand::SCALE:
            masm.movq_i32m(imm32.value, dest.disp(), dest.base(), dest.index(), dest.scale());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected operand kind");
        }
    }
    void movqsd(const Register &src, const FloatRegister &dest) {
        masm.movq_rr(src.code(), dest.code());
    }
    void movqsd(const FloatRegister &src, const Register &dest) {
        masm.movq_rr(src.code(), dest.code());
    }
    void movq(const Register &src, const Register &dest) {
        masm.movq_rr(src.code(), dest.code());
    }

    void xchgq(const Register &src, const Register &dest) {
        masm.xchgq_rr(src.code(), dest.code());
    }

    void andq(const Register &src, const Register &dest) {
        masm.andq_rr(src.code(), dest.code());
    }
    void andq(Imm32 imm, const Register &dest) {
        masm.andq_ir(imm.value, dest.code());
    }

    void addq(Imm32 imm, const Register &dest) {
        masm.addq_ir(imm.value, dest.code());
    }
    void addq(Imm32 imm, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::REG:
            masm.addq_ir(imm.value, dest.reg());
            break;
          case Operand::REG_DISP:
            masm.addq_im(imm.value, dest.disp(), dest.base());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected operand kind");
        }
    }
    void addq(const Register &src, const Register &dest) {
        masm.addq_rr(src.code(), dest.code());
    }
    void addq(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.addq_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.addq_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected operand kind");
        }
    }

    void subq(Imm32 imm, const Register &dest) {
        masm.subq_ir(imm.value, dest.code());
    }
    void subq(const Register &src, const Register &dest) {
        masm.subq_rr(src.code(), dest.code());
    }
    void subq(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.subq_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.subq_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected operand kind");
        }
    }
    void shlq(Imm32 imm, const Register &dest) {
        masm.shlq_i8r(imm.value, dest.code());
    }
    void shrq(Imm32 imm, const Register &dest) {
        masm.shrq_i8r(imm.value, dest.code());
    }
    void sarq(Imm32 imm, const Register &dest) {
        masm.sarq_i8r(imm.value, dest.code());
    }
    void orq(Imm32 imm, const Register &dest) {
        masm.orq_ir(imm.value, dest.code());
    }
    void orq(const Register &src, const Register &dest) {
        masm.orq_rr(src.code(), dest.code());
    }
    void orq(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.orq_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.orq_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected operand kind");
        }
    }
    void xorq(const Register &src, const Register &dest) {
        masm.xorq_rr(src.code(), dest.code());
    }
    void xorq(Imm32 imm, const Register &dest) {
        masm.xorq_ir(imm.value, dest.code());
    }

    void mov(ImmWord word, const Register &dest) {
        
        
        
        if (word.value <= UINT32_MAX) {
            uint32_t value32 = static_cast<uint32_t>(word.value);
            Imm32 imm32(static_cast<int32_t>(value32));
            movl(imm32, dest);
        } else {
            movq(word, dest);
        }
    }
    void mov(const Imm32 &imm32, const Register &dest) {
        movl(imm32, dest);
    }
    void mov(const Operand &src, const Register &dest) {
        movq(src, dest);
    }
    void mov(const Register &src, const Operand &dest) {
        movq(src, dest);
    }
    void mov(const Imm32 &imm32, const Operand &dest) {
        movq(imm32, dest);
    }
    void mov(const Register &src, const Register &dest) {
        movq(src, dest);
    }
    void mov(AbsoluteLabel *label, const Register &dest) {
        JS_ASSERT(!label->bound());
        
        
        masm.movq_i64r(label->prev(), dest.code());
        label->setPrev(masm.size());
    }
    void xchg(const Register &src, const Register &dest) {
        xchgq(src, dest);
    }
    void lea(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG_DISP:
            masm.leaq_mr(src.disp(), src.base(), dest.code());
            break;
          case Operand::SCALE:
            masm.leaq_mr(src.disp(), src.base(), src.index(), src.scale(), dest.code());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexepcted operand kind");
        }
    }

    CodeOffsetLabel loadRipRelativeInt32(const Register &dest) {
        return CodeOffsetLabel(masm.movl_ripr(dest.code()).offset());
    }
    CodeOffsetLabel loadRipRelativeInt64(const Register &dest) {
        return CodeOffsetLabel(masm.movq_ripr(dest.code()).offset());
    }
    CodeOffsetLabel loadRipRelativeDouble(const FloatRegister &dest) {
        return CodeOffsetLabel(masm.movsd_ripr(dest.code()).offset());
    }
    CodeOffsetLabel storeRipRelativeInt32(const Register &dest) {
        return CodeOffsetLabel(masm.movl_rrip(dest.code()).offset());
    }
    CodeOffsetLabel storeRipRelativeDouble(const FloatRegister &dest) {
        return CodeOffsetLabel(masm.movsd_rrip(dest.code()).offset());
    }
    CodeOffsetLabel leaRipRelative(const Register &dest) {
        return CodeOffsetLabel(masm.leaq_rip(dest.code()).offset());
    }

    
    
    
    void cmpq(const Operand &lhs, const Register &rhs) {
        switch (lhs.kind()) {
          case Operand::REG:
            masm.cmpq_rr(rhs.code(), lhs.reg());
            break;
          case Operand::REG_DISP:
            masm.cmpq_rm(rhs.code(), lhs.disp(), lhs.base());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected operand kind");
        }
    }
    void cmpq(const Operand &lhs, Imm32 rhs) {
        switch (lhs.kind()) {
          case Operand::REG:
            masm.cmpq_ir(rhs.value, lhs.reg());
            break;
          case Operand::REG_DISP:
            masm.cmpq_im(rhs.value, lhs.disp(), lhs.base());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected operand kind");
        }
    }
    void cmpq(const Register &lhs, const Operand &rhs) {
        switch (rhs.kind()) {
          case Operand::REG:
            masm.cmpq_rr(rhs.reg(), lhs.code());
            break;
          case Operand::REG_DISP:
            masm.cmpq_mr(rhs.disp(), rhs.base(), lhs.code());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected operand kind");
        }
    }
    void cmpq(const Register &lhs, const Register &rhs) {
        masm.cmpq_rr(rhs.code(), lhs.code());
    }
    void cmpq(const Register &lhs, Imm32 rhs) {
        masm.cmpq_ir(rhs.value, lhs.code());
    }

    void testq(const Register &lhs, Imm32 rhs) {
        masm.testq_i32r(rhs.value, lhs.code());
    }
    void testq(const Register &lhs, const Register &rhs) {
        masm.testq_rr(rhs.code(), lhs.code());
    }
    void testq(const Operand &lhs, Imm32 rhs) {
        switch (lhs.kind()) {
          case Operand::REG:
            masm.testq_i32r(rhs.value, lhs.reg());
            break;
          case Operand::REG_DISP:
            masm.testq_i32m(rhs.value, lhs.disp(), lhs.base());
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected operand kind");
            break;
        }
    }

    void jmp(void *target, Relocation::Kind reloc = Relocation::HARDCODED) {
        JmpSrc src = masm.jmp();
        addPendingJump(src, target, reloc);
    }
    void j(Condition cond, void *target,
           Relocation::Kind reloc = Relocation::HARDCODED) {
        JmpSrc src = masm.jCC(static_cast<JSC::X86Assembler::Condition>(cond));
        addPendingJump(src, target, reloc);
    }

    void jmp(IonCode *target) {
        jmp(target->raw(), Relocation::IONCODE);
    }
    void j(Condition cond, IonCode *target) {
        j(cond, target->raw(), Relocation::IONCODE);
    }
    void call(IonCode *target) {
        JmpSrc src = masm.call();
        addPendingJump(src, target->raw(), Relocation::IONCODE);
    }

    
    
    CodeOffsetLabel toggledCall(IonCode *target, bool enabled) {
        CodeOffsetLabel offset(size());
        JmpSrc src = enabled ? masm.call() : masm.cmp_eax();
        addPendingJump(src, target->raw(), Relocation::IONCODE);
        JS_ASSERT(size() - offset.offset() == ToggledCallSize());
        return offset;
    }

    static size_t ToggledCallSize() {
        
        return 5;
    }

    
    using AssemblerX86Shared::call;

    void cvttsd2sq(const FloatRegister &src, const Register &dest) {
        masm.cvttsd2sq_rr(src.code(), dest.code());
    }
    void cvtsq2sd(const Register &src, const FloatRegister &dest) {
        masm.cvtsq2sd_rr(src.code(), dest.code());
    }
};

static inline void
PatchJump(CodeLocationJump jump, CodeLocationLabel label)
{
    if (JSC::X86Assembler::canRelinkJump(jump.raw(), label.raw())) {
        JSC::X86Assembler::setRel32(jump.raw(), label.raw());
    } else {
        JSC::X86Assembler::setRel32(jump.raw(), jump.jumpTableEntry());
        Assembler::PatchJumpEntry(jump.jumpTableEntry(), label.raw());
    }
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
