





#ifndef jit_shared_MacroAssembler_x86_shared_h
#define jit_shared_MacroAssembler_x86_shared_h

#include "mozilla/Casting.h"
#include "mozilla/DebugOnly.h"

#if defined(JS_CODEGEN_X86)
# include "jit/x86/Assembler-x86.h"
#elif defined(JS_CODEGEN_X64)
# include "jit/x64/Assembler-x64.h"
#endif

namespace js {
namespace jit {

class MacroAssemblerX86Shared : public Assembler
{
  protected:
    
    
    
    
    
    uint32_t framePushed_;

  public:
    using Assembler::call;

    MacroAssemblerX86Shared()
      : framePushed_(0)
    { }

    void compareDouble(DoubleCondition cond, FloatRegister lhs, FloatRegister rhs) {
        if (cond & DoubleConditionBitInvert)
            ucomisd(rhs, lhs);
        else
            ucomisd(lhs, rhs);
    }
    void branchDouble(DoubleCondition cond, FloatRegister lhs, FloatRegister rhs, Label *label)
    {
        compareDouble(cond, lhs, rhs);

        if (cond == DoubleEqual) {
            Label unordered;
            j(Parity, &unordered);
            j(Equal, label);
            bind(&unordered);
            return;
        }
        if (cond == DoubleNotEqualOrUnordered) {
            j(NotEqual, label);
            j(Parity, label);
            return;
        }

        MOZ_ASSERT(!(cond & DoubleConditionBitSpecial));
        j(ConditionFromDoubleCondition(cond), label);
    }

    void compareFloat(DoubleCondition cond, FloatRegister lhs, FloatRegister rhs) {
        if (cond & DoubleConditionBitInvert)
            ucomiss(rhs, lhs);
        else
            ucomiss(lhs, rhs);
    }
    void branchFloat(DoubleCondition cond, FloatRegister lhs, FloatRegister rhs, Label *label)
    {
        compareFloat(cond, lhs, rhs);

        if (cond == DoubleEqual) {
            Label unordered;
            j(Parity, &unordered);
            j(Equal, label);
            bind(&unordered);
            return;
        }
        if (cond == DoubleNotEqualOrUnordered) {
            j(NotEqual, label);
            j(Parity, label);
            return;
        }

        MOZ_ASSERT(!(cond & DoubleConditionBitSpecial));
        j(ConditionFromDoubleCondition(cond), label);
    }

    void branchNegativeZero(FloatRegister reg, Register scratch, Label *label, bool  maybeNonZero = true);
    void branchNegativeZeroFloat32(FloatRegister reg, Register scratch, Label *label);

    void move32(Imm32 imm, Register dest) {
        
        
        
        mov(ImmWord(uint32_t(imm.value)), dest);
    }
    void move32(Imm32 imm, const Operand &dest) {
        movl(imm, dest);
    }
    void move32(Register src, Register dest) {
        movl(src, dest);
    }
    void move32(Register src, const Operand &dest) {
        movl(src, dest);
    }
    void and32(Register src, Register dest) {
        andl(src, dest);
    }
    void and32(const Address &src, Register dest) {
        andl(Operand(src), dest);
    }
    void and32(Imm32 imm, Register dest) {
        andl(imm, dest);
    }
    void and32(Imm32 imm, const Address &dest) {
        andl(imm, Operand(dest));
    }
    void or32(Register src, Register dest) {
        orl(src, dest);
    }
    void or32(Imm32 imm, Register dest) {
        orl(imm, dest);
    }
    void or32(Imm32 imm, const Address &dest) {
        orl(imm, Operand(dest));
    }
    void neg32(Register reg) {
        negl(reg);
    }
    void test32(Register lhs, Register rhs) {
        testl(lhs, rhs);
    }
    void test32(const Address &addr, Imm32 imm) {
        testl(Operand(addr), imm);
    }
    void test32(Register lhs, Imm32 rhs) {
        testl(lhs, rhs);
    }
    void cmp32(Register lhs, Imm32 rhs) {
        cmpl(lhs, rhs);
    }
    void cmp32(Register a, Register b) {
        cmpl(a, b);
    }
    void cmp32(const Operand &lhs, Imm32 rhs) {
        cmpl(lhs, rhs);
    }
    void cmp32(const Operand &lhs, Register rhs) {
        cmpl(lhs, rhs);
    }
    void add32(Register src, Register dest) {
        addl(src, dest);
    }
    void add32(Imm32 imm, Register dest) {
        addl(imm, dest);
    }
    void add32(Imm32 imm, const Address &dest) {
        addl(imm, Operand(dest));
    }
    void sub32(Imm32 imm, Register dest) {
        subl(imm, dest);
    }
    void sub32(Register src, Register dest) {
        subl(src, dest);
    }
    template <typename T>
    void branchAdd32(Condition cond, T src, Register dest, Label *label) {
        add32(src, dest);
        j(cond, label);
    }
    template <typename T>
    void branchSub32(Condition cond, T src, Register dest, Label *label) {
        sub32(src, dest);
        j(cond, label);
    }
    void xor32(Imm32 imm, Register dest) {
        xorl(imm, dest);
    }
    void xor32(Register src, Register dest) {
        xorl(src, dest);
    }
    void not32(Register reg) {
        notl(reg);
    }
    void atomic_inc32(const Operand &addr) {
        lock_incl(addr);
    }
    void atomic_dec32(const Operand &addr) {
        lock_decl(addr);
    }
    void atomic_cmpxchg8(Register newval, const Operand &addr, Register oldval_and_result) {
        
        MOZ_ASSERT(oldval_and_result.code() == X86Registers::eax);
        lock_cmpxchg8(newval, addr);
    }
    void atomic_cmpxchg16(Register newval, const Operand &addr, Register oldval_and_result) {
        
        MOZ_ASSERT(oldval_and_result.code() == X86Registers::eax);
        lock_cmpxchg16(newval, addr);
    }
    void atomic_cmpxchg32(Register newval, const Operand &addr, Register oldval_and_result) {
        
        MOZ_ASSERT(oldval_and_result.code() == X86Registers::eax);
        lock_cmpxchg32(newval, addr);
    }

    template <typename T>
    void atomicFetchAdd8SignExtend(Register src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(output == eax);
        if (src != output)
            movl(src, output);
        lock_xaddb(output, Operand(mem));
        movsbl(output, output);
    }

    template <typename T>
    void atomicFetchAdd8ZeroExtend(Register src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(output == eax);
        MOZ_ASSERT(temp == InvalidReg);
        if (src != output)
            movl(src, output);
        lock_xaddb(output, Operand(mem));
        movzbl(output, output);
    }

    template <typename T>
    void atomicFetchAdd8SignExtend(Imm32 src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(output == eax);
        MOZ_ASSERT(temp == InvalidReg);
        movb(src, output);
        lock_xaddb(output, Operand(mem));
        movsbl(output, output);
    }

    template <typename T>
    void atomicFetchAdd8ZeroExtend(Imm32 src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(output == eax);
        MOZ_ASSERT(temp == InvalidReg);
        movb(src, output);
        lock_xaddb(output, Operand(mem));
        movzbl(output, output);
    }

    template <typename T>
    void atomicFetchAdd16SignExtend(Register src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(temp == InvalidReg);
        if (src != output)
            movl(src, output);
        lock_xaddw(output, Operand(mem));
        movswl(output, output);
    }

    template <typename T>
    void atomicFetchAdd16ZeroExtend(Register src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(temp == InvalidReg);
        if (src != output)
            movl(src, output);
        lock_xaddw(output, Operand(mem));
        movzwl(output, output);
    }

    template <typename T>
    void atomicFetchAdd16SignExtend(Imm32 src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(temp == InvalidReg);
        movl(src, output);
        lock_xaddw(output, Operand(mem));
        movswl(output, output);
    }

    template <typename T>
    void atomicFetchAdd16ZeroExtend(Imm32 src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(temp == InvalidReg);
        movl(src, output);
        lock_xaddw(output, Operand(mem));
        movzwl(output, output);
    }

    template <typename T>
    void atomicFetchAdd32(Register src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(temp == InvalidReg);
        if (src != output)
            movl(src, output);
        lock_xaddl(output, Operand(mem));
    }

    template <typename T>
    void atomicFetchAdd32(Imm32 src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(temp == InvalidReg);
        movl(src, output);
        lock_xaddl(output, Operand(mem));
    }

    template <typename T>
    void atomicFetchSub8SignExtend(Register src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(output == eax);
        MOZ_ASSERT(temp == InvalidReg);
        if (src != output)
            movl(src, output);
        negl(output);
        lock_xaddb(output, Operand(mem));
        movsbl(output, output);
    }

    template <typename T>
    void atomicFetchSub8ZeroExtend(Register src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(output == eax);
        MOZ_ASSERT(temp == InvalidReg);
        if (src != output)
            movl(src, output);
        negl(output);
        lock_xaddb(output, Operand(mem));
        movzbl(output, output);
    }

    template <typename T>
    void atomicFetchSub8SignExtend(Imm32 src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(output == eax);
        MOZ_ASSERT(temp == InvalidReg);
        movb(Imm32(-src.value), output);
        lock_xaddb(output, Operand(mem));
        movsbl(output, output);
    }

    template <typename T>
    void atomicFetchSub8ZeroExtend(Imm32 src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(output == eax);
        MOZ_ASSERT(temp == InvalidReg);
        movb(Imm32(-src.value), output);
        lock_xaddb(output, Operand(mem));
        movzbl(output, output);
    }

    template <typename T>
    void atomicFetchSub16SignExtend(Register src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(temp == InvalidReg);
        if (src != output)
            movl(src, output);
        negl(output);
        lock_xaddw(output, Operand(mem));
        movswl(output, output);
    }

    template <typename T>
    void atomicFetchSub16ZeroExtend(Register src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(temp == InvalidReg);
        if (src != output)
            movl(src, output);
        negl(output);
        lock_xaddw(output, Operand(mem));
        movzwl(output, output);
    }

    template <typename T>
    void atomicFetchSub16SignExtend(Imm32 src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(temp == InvalidReg);
        movl(Imm32(-src.value), output);
        lock_xaddw(output, Operand(mem));
        movswl(output, output);
    }

    template <typename T>
    void atomicFetchSub16ZeroExtend(Imm32 src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(temp == InvalidReg);
        movl(Imm32(-src.value), output);
        lock_xaddw(output, Operand(mem));
        movzwl(output, output);
    }

    template <typename T>
    void atomicFetchSub32(Register src, const T &mem, Register temp, Register output) {
        MOZ_ASSERT(temp == InvalidReg);
        if (src != output)
            movl(src, output);
        negl(output);
        lock_xaddl(output, Operand(mem));
    }

    template <typename T>
    void atomicFetchSub32(Imm32 src, const T &mem, Register temp, Register output) {
        movl(Imm32(-src.value), output);
        lock_xaddl(output, Operand(mem));
    }

    
#define ATOMIC_BITOP_BODY(LOAD, OP, LOCK_CMPXCHG)        \
        MOZ_ASSERT(output == eax); \
        LOAD(Operand(mem), eax);  \
        Label again;              \
        bind(&again);             \
        movl(eax, temp);          \
        OP(src, temp);            \
        LOCK_CMPXCHG(temp, Operand(mem)); \
        j(NonZero, &again);

    template <typename S, typename T>
    void atomicFetchAnd8SignExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movb, andl, lock_cmpxchg8)
        movsbl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchAnd8ZeroExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movb, andl, lock_cmpxchg8)
        movzbl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchAnd16SignExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movw, andl, lock_cmpxchg16)
        movswl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchAnd16ZeroExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movw, andl, lock_cmpxchg16)
        movzwl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchAnd32(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movl, andl, lock_cmpxchg32)
    }

    template <typename S, typename T>
    void atomicFetchOr8SignExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movb, orl, lock_cmpxchg8)
        movsbl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchOr8ZeroExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movb, orl, lock_cmpxchg8)
        movzbl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchOr16SignExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movw, orl, lock_cmpxchg16)
        movswl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchOr16ZeroExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movw, orl, lock_cmpxchg16)
        movzwl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchOr32(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movl, orl, lock_cmpxchg32)
    }

    template <typename S, typename T>
    void atomicFetchXor8SignExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movb, xorl, lock_cmpxchg8)
        movsbl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchXor8ZeroExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movb, xorl, lock_cmpxchg8)
        movzbl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchXor16SignExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movw, xorl, lock_cmpxchg16)
        movswl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchXor16ZeroExtend(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movw, xorl, lock_cmpxchg16)
        movzwl(eax, eax);
    }
    template <typename S, typename T>
    void atomicFetchXor32(const S &src, const T &mem, Register temp, Register output) {
        ATOMIC_BITOP_BODY(movl, xorl, lock_cmpxchg32)
    }

#undef ATOMIC_BITOP_BODY

    void storeLoadFence() {
        
        if (HasSSE2())
            masm.mfence();
        else
            lock_addl(Imm32(0), Operand(Address(esp, 0)));
    }

    void branch16(Condition cond, Register lhs, Register rhs, Label *label) {
        cmpw(lhs, rhs);
        j(cond, label);
    }
    void branch32(Condition cond, const Operand &lhs, Register rhs, Label *label) {
        cmpl(lhs, rhs);
        j(cond, label);
    }
    void branch32(Condition cond, const Operand &lhs, Imm32 rhs, Label *label) {
        cmpl(lhs, rhs);
        j(cond, label);
    }
    void branch32(Condition cond, const Address &lhs, Register rhs, Label *label) {
        cmpl(Operand(lhs), rhs);
        j(cond, label);
    }
    void branch32(Condition cond, const Address &lhs, Imm32 imm, Label *label) {
        cmpl(Operand(lhs), imm);
        j(cond, label);
    }
    void branch32(Condition cond, const BaseIndex &lhs, Register rhs, Label *label) {
        cmpl(Operand(lhs), rhs);
        j(cond, label);
    }
    void branch32(Condition cond, const BaseIndex &lhs, Imm32 imm, Label *label) {
        cmpl(Operand(lhs), imm);
        j(cond, label);
    }
    void branch32(Condition cond, Register lhs, Imm32 imm, Label *label) {
        cmpl(lhs, imm);
        j(cond, label);
    }
    void branch32(Condition cond, Register lhs, Register rhs, Label *label) {
        cmpl(lhs, rhs);
        j(cond, label);
    }
    void branchTest16(Condition cond, Register lhs, Register rhs, Label *label) {
        testw(lhs, rhs);
        j(cond, label);
    }
    void branchTest32(Condition cond, Register lhs, Register rhs, Label *label) {
        MOZ_ASSERT(cond == Zero || cond == NonZero || cond == Signed || cond == NotSigned);
        testl(lhs, rhs);
        j(cond, label);
    }
    void branchTest32(Condition cond, Register lhs, Imm32 imm, Label *label) {
        MOZ_ASSERT(cond == Zero || cond == NonZero || cond == Signed || cond == NotSigned);
        testl(lhs, imm);
        j(cond, label);
    }
    void branchTest32(Condition cond, const Address &address, Imm32 imm, Label *label) {
        MOZ_ASSERT(cond == Zero || cond == NonZero || cond == Signed || cond == NotSigned);
        testl(Operand(address), imm);
        j(cond, label);
    }

    
    template <typename T>
    void Push(const T &t) {
        push(t);
        framePushed_ += sizeof(intptr_t);
    }
    void Push(FloatRegister t) {
        push(t);
        framePushed_ += sizeof(double);
    }
    CodeOffsetLabel PushWithPatch(ImmWord word) {
        framePushed_ += sizeof(word.value);
        return pushWithPatch(word);
    }
    CodeOffsetLabel PushWithPatch(ImmPtr imm) {
        return PushWithPatch(ImmWord(uintptr_t(imm.value)));
    }

    template <typename T>
    void Pop(const T &t) {
        pop(t);
        framePushed_ -= sizeof(intptr_t);
    }
    void Pop(FloatRegister t) {
        pop(t);
        framePushed_ -= sizeof(double);
    }
    void implicitPop(uint32_t args) {
        MOZ_ASSERT(args % sizeof(intptr_t) == 0);
        framePushed_ -= args;
    }
    uint32_t framePushed() const {
        return framePushed_;
    }
    void setFramePushed(uint32_t framePushed) {
        framePushed_ = framePushed;
    }

    void jump(Label *label) {
        jmp(label);
    }
    void jump(RepatchLabel *label) {
        jmp(label);
    }
    void jump(Register reg) {
        jmp(Operand(reg));
    }
    void jump(const Address &addr) {
        jmp(Operand(addr));
    }

    void convertInt32ToDouble(Register src, FloatRegister dest) {
        
        
        
        
        
        
        zeroDouble(dest);
        cvtsi2sd(src, dest);
    }
    void convertInt32ToDouble(const Address &src, FloatRegister dest) {
        convertInt32ToDouble(Operand(src), dest);
    }
    void convertInt32ToDouble(const Operand &src, FloatRegister dest) {
        
        zeroDouble(dest);
        cvtsi2sd(Operand(src), dest);
    }
    void convertInt32ToFloat32(Register src, FloatRegister dest) {
        
        zeroFloat32(dest);
        cvtsi2ss(src, dest);
    }
    void convertInt32ToFloat32(const Address &src, FloatRegister dest) {
        convertInt32ToFloat32(Operand(src), dest);
    }
    void convertInt32ToFloat32(const Operand &src, FloatRegister dest) {
        
        zeroFloat32(dest);
        cvtsi2ss(src, dest);
    }
    Condition testDoubleTruthy(bool truthy, FloatRegister reg) {
        zeroDouble(ScratchDoubleReg);
        ucomisd(ScratchDoubleReg, reg);
        return truthy ? NonZero : Zero;
    }
    void branchTestDoubleTruthy(bool truthy, FloatRegister reg, Label *label) {
        Condition cond = testDoubleTruthy(truthy, reg);
        j(cond, label);
    }
    void load8ZeroExtend(const Address &src, Register dest) {
        movzbl(Operand(src), dest);
    }
    void load8ZeroExtend(const BaseIndex &src, Register dest) {
        movzbl(Operand(src), dest);
    }
    void load8SignExtend(const Address &src, Register dest) {
        movsbl(Operand(src), dest);
    }
    void load8SignExtend(const BaseIndex &src, Register dest) {
        movsbl(Operand(src), dest);
    }
    template <typename S, typename T>
    void store8(const S &src, const T &dest) {
        movb(src, Operand(dest));
    }
    template <typename T>
    void compareExchange8ZeroExtend(const T &mem, Register oldval, Register newval, Register output) {
        MOZ_ASSERT(output == eax);
        MOZ_ASSERT(newval == ebx || newval == ecx || newval == edx);
        if (oldval != output)
            movl(oldval, output);
        lock_cmpxchg8(newval, Operand(mem));
        movzbl(output, output);
    }
    template <typename T>
    void compareExchange8SignExtend(const T &mem, Register oldval, Register newval, Register output) {
        MOZ_ASSERT(output == eax);
        MOZ_ASSERT(newval == ebx || newval == ecx || newval == edx);
        if (oldval != output)
            movl(oldval, output);
        lock_cmpxchg8(newval, Operand(mem));
        movsbl(output, output);
    }
    void load16ZeroExtend(const Address &src, Register dest) {
        movzwl(Operand(src), dest);
    }
    void load16ZeroExtend(const BaseIndex &src, Register dest) {
        movzwl(Operand(src), dest);
    }
    template <typename S, typename T>
    void store16(const S &src, const T &dest) {
        movw(src, Operand(dest));
    }
    template <typename T>
    void compareExchange16ZeroExtend(const T &mem, Register oldval, Register newval, Register output) {
        MOZ_ASSERT(output == eax);
        if (oldval != output)
            movl(oldval, output);
        lock_cmpxchg16(newval, Operand(mem));
        movzwl(output, output);
    }
    template <typename T>
    void compareExchange16SignExtend(const T &mem, Register oldval, Register newval, Register output) {
        MOZ_ASSERT(output == eax);
        if (oldval != output)
            movl(oldval, output);
        lock_cmpxchg16(newval, Operand(mem));
        movswl(output, output);
    }
    void load16SignExtend(const Address &src, Register dest) {
        movswl(Operand(src), dest);
    }
    void load16SignExtend(const BaseIndex &src, Register dest) {
        movswl(Operand(src), dest);
    }
    void load32(const Address &address, Register dest) {
        movl(Operand(address), dest);
    }
    void load32(const BaseIndex &src, Register dest) {
        movl(Operand(src), dest);
    }
    void load32(const Operand &src, Register dest) {
        movl(src, dest);
    }
    template <typename S, typename T>
    void store32(const S &src, const T &dest) {
        movl(src, Operand(dest));
    }
    template <typename T>
    void compareExchange32(const T &mem, Register oldval, Register newval, Register output) {
        MOZ_ASSERT(output == eax);
        if (oldval != output)
            movl(oldval, output);
        lock_cmpxchg32(newval, Operand(mem));
    }
    template <typename S, typename T>
    void store32_NoSecondScratch(const S &src, const T &dest) {
        store32(src, dest);
    }
    void loadDouble(const Address &src, FloatRegister dest) {
        movsd(src, dest);
    }
    void loadDouble(const BaseIndex &src, FloatRegister dest) {
        movsd(src, dest);
    }
    void loadDouble(const Operand &src, FloatRegister dest) {
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            loadDouble(src.toAddress(), dest);
            break;
          case Operand::MEM_SCALE:
            loadDouble(src.toBaseIndex(), dest);
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void storeDouble(FloatRegister src, const Address &dest) {
        movsd(src, dest);
    }
    void storeDouble(FloatRegister src, const BaseIndex &dest) {
        movsd(src, dest);
    }
    void storeDouble(FloatRegister src, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            storeDouble(src, dest.toAddress());
            break;
          case Operand::MEM_SCALE:
            storeDouble(src, dest.toBaseIndex());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void moveDouble(FloatRegister src, FloatRegister dest) {
        
        movapd(src, dest);
    }
    void zeroDouble(FloatRegister reg) {
        xorpd(reg, reg);
    }
    void zeroFloat32(FloatRegister reg) {
        xorps(reg, reg);
    }
    void negateDouble(FloatRegister reg) {
        
        pcmpeqw(ScratchDoubleReg, ScratchDoubleReg);
        psllq(Imm32(63), ScratchDoubleReg);

        
        xorpd(ScratchDoubleReg, reg); 
    }
    void negateFloat(FloatRegister reg) {
        pcmpeqw(ScratchFloat32Reg, ScratchFloat32Reg);
        psllq(Imm32(31), ScratchFloat32Reg);

        
        xorps(ScratchFloat32Reg, reg); 
    }
    void addDouble(FloatRegister src, FloatRegister dest) {
        addsd(src, dest);
    }
    void subDouble(FloatRegister src, FloatRegister dest) {
        subsd(src, dest);
    }
    void mulDouble(FloatRegister src, FloatRegister dest) {
        mulsd(src, dest);
    }
    void divDouble(FloatRegister src, FloatRegister dest) {
        divsd(src, dest);
    }
    void convertFloat32ToDouble(FloatRegister src, FloatRegister dest) {
        cvtss2sd(src, dest);
    }
    void convertDoubleToFloat32(FloatRegister src, FloatRegister dest) {
        cvtsd2ss(src, dest);
    }

    void convertFloat32x4ToInt32x4(FloatRegister src, FloatRegister dest) {
        
        
        
        
        
        cvttps2dq(src, dest);
    }
    void convertInt32x4ToFloat32x4(FloatRegister src, FloatRegister dest) {
        cvtdq2ps(src, dest);
    }

    void bitwiseAndX4(const Operand &src, FloatRegister dest) {
        
        
        andps(src, dest);
    }
    void bitwiseAndNotX4(const Operand &src, FloatRegister dest) {
        andnps(src, dest);
    }
    void bitwiseOrX4(const Operand &src, FloatRegister dest) {
        orps(src, dest);
    }
    void bitwiseXorX4(const Operand &src, FloatRegister dest) {
        xorps(src, dest);
    }

    void loadAlignedInt32x4(const Address &src, FloatRegister dest) {
        movdqa(Operand(src), dest);
    }
    void loadAlignedInt32x4(const Operand &src, FloatRegister dest) {
        movdqa(src, dest);
    }
    void storeAlignedInt32x4(FloatRegister src, const Address &dest) {
        movdqa(src, Operand(dest));
    }
    void moveAlignedInt32x4(FloatRegister src, FloatRegister dest) {
        movdqa(src, dest);
    }
    void loadUnalignedInt32x4(const Address &src, FloatRegister dest) {
        movdqu(Operand(src), dest);
    }
    void storeUnalignedInt32x4(FloatRegister src, const Address &dest) {
        movdqu(src, Operand(dest));
    }
    void packedEqualInt32x4(const Operand &src, FloatRegister dest) {
        pcmpeqd(src, dest);
    }
    void packedGreaterThanInt32x4(const Operand &src, FloatRegister dest) {
        pcmpgtd(src, dest);
    }
    void packedAddInt32(const Operand &src, FloatRegister dest) {
        paddd(src, dest);
    }
    void packedSubInt32(const Operand &src, FloatRegister dest) {
        psubd(src, dest);
    }
    void packedReciprocalFloat32x4(const Operand &src, FloatRegister dest) {
        
        
        
        rcpps(src, dest);
    }
    void packedReciprocalSqrtFloat32x4(const Operand &src, FloatRegister dest) {
        
        rsqrtps(src, dest);
    }
    void packedSqrtFloat32x4(const Operand &src, FloatRegister dest) {
        sqrtps(src, dest);
    }

    void packedLeftShiftByScalar(FloatRegister src, FloatRegister dest) {
        pslld(src, dest);
    }
    void packedLeftShiftByScalar(Imm32 count, FloatRegister dest) {
        pslld(count, dest);
    }
    void packedRightShiftByScalar(FloatRegister src, FloatRegister dest) {
        psrad(src, dest);
    }
    void packedRightShiftByScalar(Imm32 count, FloatRegister dest) {
        psrad(count, dest);
    }
    void packedUnsignedRightShiftByScalar(FloatRegister src, FloatRegister dest) {
        psrld(src, dest);
    }
    void packedUnsignedRightShiftByScalar(Imm32 count, FloatRegister dest) {
        psrld(count, dest);
    }

    void loadAlignedFloat32x4(const Address &src, FloatRegister dest) {
        movaps(Operand(src), dest);
    }
    void loadAlignedFloat32x4(const Operand &src, FloatRegister dest) {
        movaps(src, dest);
    }
    void storeAlignedFloat32x4(FloatRegister src, const Address &dest) {
        movaps(src, Operand(dest));
    }
    void moveAlignedFloat32x4(FloatRegister src, FloatRegister dest) {
        movaps(src, dest);
    }
    void loadUnalignedFloat32x4(const Address &src, FloatRegister dest) {
        movups(Operand(src), dest);
    }
    void storeUnalignedFloat32x4(FloatRegister src, const Address &dest) {
        movups(src, Operand(dest));
    }
    void packedAddFloat32(const Operand &src, FloatRegister dest) {
        addps(src, dest);
    }
    void packedSubFloat32(const Operand &src, FloatRegister dest) {
        subps(src, dest);
    }
    void packedMulFloat32(const Operand &src, FloatRegister dest) {
        mulps(src, dest);
    }
    void packedDivFloat32(const Operand &src, FloatRegister dest) {
        divps(src, dest);
    }

    static uint32_t ComputeShuffleMask(uint32_t x = LaneX, uint32_t y = LaneY,
                                       uint32_t z = LaneZ, uint32_t w = LaneW)
    {
        MOZ_ASSERT(x < 4 && y < 4 && z < 4 && w < 4);
        uint32_t r = (w << 6) | (z << 4) | (y << 2) | (x << 0);
        MOZ_ASSERT(r < 256);
        return r;
    }

    void shuffleInt32(uint32_t mask, FloatRegister src, FloatRegister dest) {
        pshufd(mask, src, dest);
    }
    void moveLowInt32(FloatRegister src, Register dest) {
        movd(src, dest);
    }

    void moveHighPairToLowPairFloat32(FloatRegister src, FloatRegister dest) {
        movhlps(src, dest);
    }
    void shuffleFloat32(uint32_t mask, FloatRegister src, FloatRegister dest) {
        
        
        
        
        
        if (src != dest)
            moveAlignedFloat32x4(src, dest);
        shufps(mask, dest, dest);
    }
    void shuffleMix(uint32_t mask, const Operand &src, FloatRegister dest) {
        
        
        shufps(mask, src, dest);
    }

    void moveFloatAsDouble(Register src, FloatRegister dest) {
        movd(src, dest);
        cvtss2sd(dest, dest);
    }
    void loadFloatAsDouble(const Address &src, FloatRegister dest) {
        movss(src, dest);
        cvtss2sd(dest, dest);
    }
    void loadFloatAsDouble(const BaseIndex &src, FloatRegister dest) {
        movss(src, dest);
        cvtss2sd(dest, dest);
    }
    void loadFloatAsDouble(const Operand &src, FloatRegister dest) {
        loadFloat32(src, dest);
        cvtss2sd(dest, dest);
    }
    void loadFloat32(const Address &src, FloatRegister dest) {
        movss(src, dest);
    }
    void loadFloat32(const BaseIndex &src, FloatRegister dest) {
        movss(src, dest);
    }
    void loadFloat32(const Operand &src, FloatRegister dest) {
        switch (src.kind()) {
          case Operand::MEM_REG_DISP:
            loadFloat32(src.toAddress(), dest);
            break;
          case Operand::MEM_SCALE:
            loadFloat32(src.toBaseIndex(), dest);
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void storeFloat32(FloatRegister src, const Address &dest) {
        movss(src, dest);
    }
    void storeFloat32(FloatRegister src, const BaseIndex &dest) {
        movss(src, dest);
    }
    void storeFloat32(FloatRegister src, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::MEM_REG_DISP:
            storeFloat32(src, dest.toAddress());
            break;
          case Operand::MEM_SCALE:
            storeFloat32(src, dest.toBaseIndex());
            break;
          default:
            MOZ_CRASH("unexpected operand kind");
        }
    }
    void moveFloat32(FloatRegister src, FloatRegister dest) {
        
        movaps(src, dest);
    }

    
    
    
    void convertDoubleToInt32(FloatRegister src, Register dest, Label *fail,
                              bool negativeZeroCheck = true)
    {
        
        if (negativeZeroCheck)
            branchNegativeZero(src, dest, fail);

        cvttsd2si(src, dest);
        cvtsi2sd(dest, ScratchDoubleReg);
        ucomisd(src, ScratchDoubleReg);
        j(Assembler::Parity, fail);
        j(Assembler::NotEqual, fail);

    }

    
    
    
    void convertFloat32ToInt32(FloatRegister src, Register dest, Label *fail,
                               bool negativeZeroCheck = true)
    {
        
        if (negativeZeroCheck)
            branchNegativeZeroFloat32(src, dest, fail);

        cvttss2si(src, dest);
        convertInt32ToFloat32(dest, ScratchFloat32Reg);
        ucomiss(src, ScratchFloat32Reg);
        j(Assembler::Parity, fail);
        j(Assembler::NotEqual, fail);
    }

    void clampIntToUint8(Register reg) {
        Label inRange;
        branchTest32(Assembler::Zero, reg, Imm32(0xffffff00), &inRange);
        {
            sarl(Imm32(31), reg);
            notl(reg);
            andl(Imm32(255), reg);
        }
        bind(&inRange);
    }

    bool maybeInlineDouble(double d, FloatRegister dest) {
        uint64_t u = mozilla::BitwiseCast<uint64_t>(d);

        
        if (u == 0) {
            xorpd(dest, dest);
            return true;
        }

        
        
        
        
        
        
        

        return false;
    }

    bool maybeInlineFloat(float f, FloatRegister dest) {
        uint32_t u = mozilla::BitwiseCast<uint32_t>(f);

        
        if (u == 0) {
            xorps(dest, dest);
            return true;
        }
        return false;
    }

    bool maybeInlineInt32x4(const SimdConstant &v, const FloatRegister &dest) {
        static const SimdConstant zero = SimdConstant::CreateX4(0, 0, 0, 0);
        static const SimdConstant minusOne = SimdConstant::CreateX4(-1, -1, -1, -1);
        if (v == zero) {
            pxor(dest, dest);
            return true;
        }
        if (v == minusOne) {
            pcmpeqw(dest, dest);
            return true;
        }
        return false;
    }
    bool maybeInlineFloat32x4(const SimdConstant &v, const FloatRegister &dest) {
        static const SimdConstant zero = SimdConstant::CreateX4(0.f, 0.f, 0.f, 0.f);
        if (v == zero) {
            
            
            xorps(dest, dest);
            return true;
        }
        return false;
    }

    void convertBoolToInt32(Register source, Register dest) {
        
        
        movzbl(source, dest);
    }

    void emitSet(Assembler::Condition cond, Register dest,
                 Assembler::NaNCond ifNaN = Assembler::NaN_HandledByCond) {
        if (GeneralRegisterSet(Registers::SingleByteRegs).has(dest)) {
            
            
            setCC(cond, dest);
            movzbl(dest, dest);

            if (ifNaN != Assembler::NaN_HandledByCond) {
                Label noNaN;
                j(Assembler::NoParity, &noNaN);
                mov(ImmWord(ifNaN == Assembler::NaN_IsTrue), dest);
                bind(&noNaN);
            }
        } else {
            Label end;
            Label ifFalse;

            if (ifNaN == Assembler::NaN_IsFalse)
                j(Assembler::Parity, &ifFalse);
            
            
            
            
            movl(Imm32(1), dest);
            j(cond, &end);
            if (ifNaN == Assembler::NaN_IsTrue)
                j(Assembler::Parity, &end);
            bind(&ifFalse);
            mov(ImmWord(0), dest);

            bind(&end);
        }
    }

    template <typename T1, typename T2>
    void cmp32Set(Assembler::Condition cond, T1 lhs, T2 rhs, Register dest)
    {
        cmp32(lhs, rhs);
        emitSet(cond, dest);
    }

    
    CodeOffsetLabel toggledJump(Label *label) {
        CodeOffsetLabel offset(size());
        jump(label);
        return offset;
    }

    template <typename T>
    void computeEffectiveAddress(const T &address, Register dest) {
        lea(Operand(address), dest);
    }

    
    
    bool buildFakeExitFrame(Register scratch, uint32_t *offset);
    void callWithExitFrame(Label *target);
    void callWithExitFrame(JitCode *target);

    void call(const CallSiteDesc &desc, Label *label) {
        call(label);
        append(desc, currentOffset(), framePushed_);
    }
    void call(const CallSiteDesc &desc, Register reg) {
        call(reg);
        append(desc, currentOffset(), framePushed_);
    }
    void callIon(Register callee) {
        call(callee);
    }
    void callIonFromAsmJS(Register callee) {
        call(callee);
    }
    void call(AsmJSImmPtr target) {
        mov(target, eax);
        call(eax);
    }

    void checkStackAlignment() {
        
    }

    CodeOffsetLabel labelForPatch() {
        return CodeOffsetLabel(size());
    }

    void abiret() {
        ret();
    }

  protected:
    bool buildOOLFakeExitFrame(void *fakeReturnAddr);
};

} 
} 

#endif 
