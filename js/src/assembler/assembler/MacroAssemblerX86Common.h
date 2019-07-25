
























#ifndef MacroAssemblerX86Common_h
#define MacroAssemblerX86Common_h

#include <wtf/Platform.h>

#if ENABLE_ASSEMBLER

#include "X86Assembler.h"
#include "AbstractMacroAssembler.h"

namespace JSC {

class MacroAssemblerX86Common : public AbstractMacroAssembler<X86Assembler> {
    static const int DoubleConditionBitInvert = 0x10;
    static const int DoubleConditionBitSpecial = 0x20;
    static const int DoubleConditionBits = DoubleConditionBitInvert | DoubleConditionBitSpecial;

protected:
#if WTF_CPU_X86_64
    static const X86Registers::RegisterID scratchRegister = X86Registers::r11;
#endif

public:

    enum Condition {
        Equal = X86Assembler::ConditionE,
        NotEqual = X86Assembler::ConditionNE,
        Above = X86Assembler::ConditionA,
        AboveOrEqual = X86Assembler::ConditionAE,
        Below = X86Assembler::ConditionB,
        BelowOrEqual = X86Assembler::ConditionBE,
        GreaterThan = X86Assembler::ConditionG,
        GreaterThanOrEqual = X86Assembler::ConditionGE,
        LessThan = X86Assembler::ConditionL,
        LessThanOrEqual = X86Assembler::ConditionLE,
        Overflow = X86Assembler::ConditionO,
        Signed = X86Assembler::ConditionS,
        Zero = X86Assembler::ConditionE,
        NonZero = X86Assembler::ConditionNE
    };

    enum DoubleCondition {
        
        DoubleEqual = X86Assembler::ConditionE | DoubleConditionBitSpecial,
        DoubleNotEqual = X86Assembler::ConditionNE,
        DoubleGreaterThan = X86Assembler::ConditionA,
        DoubleGreaterThanOrEqual = X86Assembler::ConditionAE,
        DoubleLessThan = X86Assembler::ConditionA | DoubleConditionBitInvert,
        DoubleLessThanOrEqual = X86Assembler::ConditionAE | DoubleConditionBitInvert,
        
        DoubleEqualOrUnordered = X86Assembler::ConditionE,
        DoubleNotEqualOrUnordered = X86Assembler::ConditionNE | DoubleConditionBitSpecial,
        DoubleGreaterThanOrUnordered = X86Assembler::ConditionB | DoubleConditionBitInvert,
        DoubleGreaterThanOrEqualOrUnordered = X86Assembler::ConditionBE | DoubleConditionBitInvert,
        DoubleLessThanOrUnordered = X86Assembler::ConditionB,
        DoubleLessThanOrEqualOrUnordered = X86Assembler::ConditionBE
    };
    COMPILE_ASSERT(
        !((X86Assembler::ConditionE | X86Assembler::ConditionNE | X86Assembler::ConditionA | X86Assembler::ConditionAE | X86Assembler::ConditionB | X86Assembler::ConditionBE) & DoubleConditionBits),
        DoubleConditionBits_should_not_interfere_with_X86Assembler_Condition_codes);

    static const RegisterID stackPointerRegister = X86Registers::esp;

    static inline bool CanUse8Bit(RegisterID reg) {
        return !!((1 << reg) & ~((1 << X86Registers::esp) |
                                 (1 << X86Registers::edi) |
                                 (1 << X86Registers::esi) |
                                 (1 << X86Registers::ebp)));
    }

    
    
    
    
    
    

    void add32(RegisterID src, RegisterID dest)
    {
        m_assembler.addl_rr(src, dest);
    }

    void add32(Imm32 imm, Address address)
    {
        m_assembler.addl_im(imm.m_value, address.offset, address.base);
    }

    void add32(Imm32 imm, RegisterID dest)
    {
        m_assembler.addl_ir(imm.m_value, dest);
    }
    
    void add32(Address src, RegisterID dest)
    {
        m_assembler.addl_mr(src.offset, src.base, dest);
    }

    void add32(RegisterID src, Address dest)
    {
        m_assembler.addl_rm(src, dest.offset, dest.base);
    }
    
    void and32(RegisterID src, RegisterID dest)
    {
        m_assembler.andl_rr(src, dest);
    }

    void and32(Imm32 imm, RegisterID dest)
    {
        m_assembler.andl_ir(imm.m_value, dest);
    }

    void and32(RegisterID src, Address dest)
    {
        m_assembler.andl_rm(src, dest.offset, dest.base);
    }

    void and32(Address src, RegisterID dest)
    {
        m_assembler.andl_mr(src.offset, src.base, dest);
    }

    void and32(Imm32 imm, Address address)
    {
        m_assembler.andl_im(imm.m_value, address.offset, address.base);
    }

    void lshift32(Imm32 imm, RegisterID dest)
    {
        m_assembler.shll_i8r(imm.m_value, dest);
    }
    
    void lshift32(RegisterID shift_amount, RegisterID dest)
    {
        
        
        if (shift_amount != X86Registers::ecx) {
            swap(shift_amount, X86Registers::ecx);

            
            if (dest == shift_amount)
                m_assembler.shll_CLr(X86Registers::ecx);
            
            else if (dest == X86Registers::ecx)
                m_assembler.shll_CLr(shift_amount);
            
            else
                m_assembler.shll_CLr(dest);
        
            swap(shift_amount, X86Registers::ecx);
        } else
            m_assembler.shll_CLr(dest);
    }
    
    void mul32(RegisterID src, RegisterID dest)
    {
        m_assembler.imull_rr(src, dest);
    }

    void mul32(Address src, RegisterID dest)
    {
        m_assembler.imull_mr(src.offset, src.base, dest);
    }
    
    void mul32(Imm32 imm, RegisterID src, RegisterID dest)
    {
        m_assembler.imull_i32r(src, imm.m_value, dest);
    }

    void neg32(RegisterID srcDest)
    {
        m_assembler.negl_r(srcDest);
    }

    void neg32(Address srcDest)
    {
        m_assembler.negl_m(srcDest.offset, srcDest.base);
    }

    void not32(RegisterID srcDest)
    {
        m_assembler.notl_r(srcDest);
    }

    void not32(Address srcDest)
    {
        m_assembler.notl_m(srcDest.offset, srcDest.base);
    }
    
    void or32(RegisterID src, RegisterID dest)
    {
        m_assembler.orl_rr(src, dest);
    }

    void or32(Imm32 imm, RegisterID dest)
    {
        m_assembler.orl_ir(imm.m_value, dest);
    }

    void or32(RegisterID src, Address dest)
    {
        m_assembler.orl_rm(src, dest.offset, dest.base);
    }

    void or32(Address src, RegisterID dest)
    {
        m_assembler.orl_mr(src.offset, src.base, dest);
    }

    void or32(Imm32 imm, Address address)
    {
        m_assembler.orl_im(imm.m_value, address.offset, address.base);
    }

    void rshift32(RegisterID shift_amount, RegisterID dest)
    {
        
        
        if (shift_amount != X86Registers::ecx) {
            swap(shift_amount, X86Registers::ecx);

            
            if (dest == shift_amount)
                m_assembler.sarl_CLr(X86Registers::ecx);
            
            else if (dest == X86Registers::ecx)
                m_assembler.sarl_CLr(shift_amount);
            
            else
                m_assembler.sarl_CLr(dest);
        
            swap(shift_amount, X86Registers::ecx);
        } else
            m_assembler.sarl_CLr(dest);
    }

    void rshift32(Imm32 imm, RegisterID dest)
    {
        m_assembler.sarl_i8r(imm.m_value, dest);
    }
    
    void urshift32(RegisterID shift_amount, RegisterID dest)
    {
        
        
        if (shift_amount != X86Registers::ecx) {
            swap(shift_amount, X86Registers::ecx);
            
            
            if (dest == shift_amount)
                m_assembler.shrl_CLr(X86Registers::ecx);
            
            else if (dest == X86Registers::ecx)
                m_assembler.shrl_CLr(shift_amount);
            
            else
                m_assembler.shrl_CLr(dest);
            
            swap(shift_amount, X86Registers::ecx);
        } else
            m_assembler.shrl_CLr(dest);
    }
    
    void urshift32(Imm32 imm, RegisterID dest)
    {
        m_assembler.shrl_i8r(imm.m_value, dest);
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.subl_rr(src, dest);
    }
    
    void sub32(Imm32 imm, RegisterID dest)
    {
        m_assembler.subl_ir(imm.m_value, dest);
    }
    
    void sub32(Imm32 imm, Address address)
    {
        m_assembler.subl_im(imm.m_value, address.offset, address.base);
    }

    void sub32(Address src, RegisterID dest)
    {
        m_assembler.subl_mr(src.offset, src.base, dest);
    }

    void sub32(RegisterID src, Address dest)
    {
        m_assembler.subl_rm(src, dest.offset, dest.base);
    }


    void xor32(RegisterID src, RegisterID dest)
    {
        m_assembler.xorl_rr(src, dest);
    }

    void xor32(Imm32 imm, Address dest)
    {
        m_assembler.xorl_im(imm.m_value, dest.offset, dest.base);
    }

    void xor32(Imm32 imm, RegisterID dest)
    {
        m_assembler.xorl_ir(imm.m_value, dest);
    }

    void xor32(RegisterID src, Address dest)
    {
        m_assembler.xorl_rm(src, dest.offset, dest.base);
    }

    void xor32(Address src, RegisterID dest)
    {
        m_assembler.xorl_mr(src.offset, src.base, dest);
    }
    
    void sqrtDouble(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.sqrtsd_rr(src, dst);
    }

    
    
    
    
    
    

    void load32(ImplicitAddress address, RegisterID dest)
    {
        m_assembler.movl_mr(address.offset, address.base, dest);
    }

    void load32(BaseIndex address, RegisterID dest)
    {
        m_assembler.movl_mr(address.offset, address.base, address.index, address.scale, dest);
    }

    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
    {
        load32(address, dest);
    }

    DataLabel32 load32WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        m_assembler.movl_mr_disp32(address.offset, address.base, dest);
        return DataLabel32(this);
    }

    void load16(BaseIndex address, RegisterID dest)
    {
        m_assembler.movzwl_mr(address.offset, address.base, address.index, address.scale, dest);
    }
    
    void load16(Address address, RegisterID dest)
    {
        m_assembler.movzwl_mr(address.offset, address.base, dest);
    }

    DataLabel32 store32WithAddressOffsetPatch(RegisterID src, Address address)
    {
        m_assembler.movl_rm_disp32(src, address.offset, address.base);
        return DataLabel32(this);
    }

    void store32(RegisterID src, ImplicitAddress address)
    {
        m_assembler.movl_rm(src, address.offset, address.base);
    }

    void store32(Imm32 imm, BaseIndex address)
    {
        m_assembler.movl_i32m(imm.m_value, address.offset, address.base, address.index, address.scale);
    }

    void store32(RegisterID src, BaseIndex address)
    {
        m_assembler.movl_rm(src, address.offset, address.base, address.index, address.scale);
    }

    void store32(Imm32 imm, ImplicitAddress address)
    {
        m_assembler.movl_i32m(imm.m_value, address.offset, address.base);
    }


    
    
    

    void moveDouble(FPRegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.movsd_rr(src, dest);
    }

    void loadDouble(ImplicitAddress address, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.movsd_mr(address.offset, address.base, dest);
    }

    void storeDouble(FPRegisterID src, ImplicitAddress address)
    {
        ASSERT(isSSE2Present());
        m_assembler.movsd_rm(src, address.offset, address.base);
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.addsd_rr(src, dest);
    }

    void addDouble(Address src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.addsd_mr(src.offset, src.base, dest);
    }

    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.divsd_rr(src, dest);
    }

    void divDouble(Address src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.divsd_mr(src.offset, src.base, dest);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.subsd_rr(src, dest);
    }

    void subDouble(Address src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.subsd_mr(src.offset, src.base, dest);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.mulsd_rr(src, dest);
    }

    void mulDouble(Address src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.mulsd_mr(src.offset, src.base, dest);
    }

    void xorDouble(FPRegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.xorpd_rr(src, dest);
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvtsi2sd_rr(src, dest);
    }

    void convertInt32ToDouble(Address src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvtsi2sd_mr(src.offset, src.base, dest);
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        ASSERT(isSSE2Present());

        if (cond & DoubleConditionBitInvert)
            m_assembler.ucomisd_rr(left, right);
        else
            m_assembler.ucomisd_rr(right, left);

        if (cond == DoubleEqual) {
            Jump isUnordered(m_assembler.jp());
            Jump result = Jump(m_assembler.je());
            isUnordered.link(this);
            return result;
        } else if (cond == DoubleNotEqualOrUnordered) {
            Jump isUnordered(m_assembler.jp());
            Jump isEqual(m_assembler.je());
            isUnordered.link(this);
            Jump result = jump();
            isEqual.link(this);
            return result;
        }

        ASSERT(!(cond & DoubleConditionBitSpecial));
        return Jump(m_assembler.jCC(static_cast<X86Assembler::Condition>(cond & ~DoubleConditionBits)));
    }

    
    
    
    
    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvttsd2si_rr(src, dest);
        return branch32(Equal, dest, Imm32(0x80000000));
    }

    
    
    
    
    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID fpTemp)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvttsd2si_rr(src, dest);

        
        failureCases.append(branchTest32(Zero, dest));

        
        convertInt32ToDouble(dest, fpTemp);
        m_assembler.ucomisd_rr(fpTemp, src);
        failureCases.append(m_assembler.jp());
        failureCases.append(m_assembler.jne());
    }

    void zeroDouble(FPRegisterID srcDest)
    {
        ASSERT(isSSE2Present());
        m_assembler.xorpd_rr(srcDest, srcDest);
    }


    
    
    
    
    
    
    
    
    void pop(RegisterID dest)
    {
        m_assembler.pop_r(dest);
    }

    void push(RegisterID src)
    {
        m_assembler.push_r(src);
    }

    void push(Address address)
    {
        m_assembler.push_m(address.offset, address.base);
    }

    void push(Imm32 imm)
    {
        m_assembler.push_i32(imm.m_value);
    }


    
    
    

    void move(Imm32 imm, RegisterID dest)
    {
        
        
        if (!imm.m_value)
            m_assembler.xorl_rr(dest, dest);
        else
            m_assembler.movl_i32r(imm.m_value, dest);
    }

#if WTF_CPU_X86_64
    void move(RegisterID src, RegisterID dest)
    {
        
        
        if (src != dest)
            m_assembler.movq_rr(src, dest);
    }

    void move(ImmPtr imm, RegisterID dest)
    {
        m_assembler.movq_i64r(imm.asIntptr(), dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        
        if (reg1 != reg2) {
            m_assembler.movq_rr(reg1, scratchRegister);
            m_assembler.movq_rr(reg2, reg1);
            m_assembler.movq_rr(scratchRegister, reg2);
        }
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.movsxd_rr(src, dest);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.movl_rr(src, dest);
    }
#else
    void move(RegisterID src, RegisterID dest)
    {
        if (src != dest)
            m_assembler.movl_rr(src, dest);
    }

    void move(ImmPtr imm, RegisterID dest)
    {
        m_assembler.movl_i32r(imm.asIntptr(), dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        if (reg1 != reg2)
            m_assembler.xchgl_rr(reg1, reg2);
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        move(src, dest);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        move(src, dest);
    }
#endif


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

public:
    Jump branch8(Condition cond, Address left, Imm32 right)
    {
        m_assembler.cmpb_im(right.m_value, left.offset, left.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32(Condition cond, RegisterID left, RegisterID right)
    {
        m_assembler.cmpl_rr(right, left);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32(Condition cond, RegisterID left, Imm32 right)
    {
        if (((cond == Equal) || (cond == NotEqual)) && !right.m_value)
            m_assembler.testl_rr(left, left);
        else
            m_assembler.cmpl_ir(right.m_value, left);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    
    
    Jump branch32_force32(Condition cond, RegisterID left, Imm32 right)
    {
        m_assembler.cmpl_ir_force32(right.m_value, left);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    
    Jump branch32WithPatch(Condition cond, RegisterID left, Imm32 right, Label &clabel)
    {
        
        m_assembler.cmpl_ir(right.m_value, left);
        clabel = label();
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32(Condition cond, RegisterID left, Address right)
    {
        m_assembler.cmpl_mr(right.offset, right.base, left);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branch32(Condition cond, Address left, RegisterID right)
    {
        m_assembler.cmpl_rm(right, left.offset, left.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32(Condition cond, Address left, Imm32 right)
    {
        m_assembler.cmpl_im(right.m_value, left.offset, left.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32(Condition cond, BaseIndex left, Imm32 right)
    {
        m_assembler.cmpl_im(right.m_value, left.offset, left.base, left.index, left.scale);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32WithUnalignedHalfWords(Condition cond, BaseIndex left, Imm32 right)
    {
        return branch32(cond, left, right);
    }

    Jump branch16(Condition cond, BaseIndex left, RegisterID right)
    {
        m_assembler.cmpw_rm(right, left.offset, left.base, left.index, left.scale);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch16(Condition cond, BaseIndex left, Imm32 right)
    {
        ASSERT(!(right.m_value & 0xFFFF0000));

        m_assembler.cmpw_im(right.m_value, left.offset, left.base, left.index, left.scale);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchTest32(Condition cond, RegisterID reg, RegisterID mask)
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        m_assembler.testl_rr(reg, mask);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchTest32(Condition cond, RegisterID reg, Imm32 mask = Imm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        
        if (mask.m_value == -1)
            m_assembler.testl_rr(reg, reg);
        else if (CanUse8Bit(reg) && (mask.m_value & ~0x7f) == 0)
            m_assembler.testb_i8r(mask.m_value, reg);
        else
            m_assembler.testl_i32r(mask.m_value, reg);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchTest32(Condition cond, Address address, Imm32 mask = Imm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        if (mask.m_value == -1)
            m_assembler.cmpl_im(0, address.offset, address.base);
        else
            m_assembler.testl_i32m(mask.m_value, address.offset, address.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchTest32(Condition cond, BaseIndex address, Imm32 mask = Imm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        if (mask.m_value == -1)
            m_assembler.cmpl_im(0, address.offset, address.base, address.index, address.scale);
        else
            m_assembler.testl_i32m(mask.m_value, address.offset, address.base, address.index, address.scale);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchTest8(Condition cond, Address address, Imm32 mask = Imm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        if (mask.m_value == -1)
            m_assembler.cmpb_im(0, address.offset, address.base);
        else
            m_assembler.testb_im(mask.m_value, address.offset, address.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchTest8(Condition cond, BaseIndex address, Imm32 mask = Imm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        if (mask.m_value == -1)
            m_assembler.cmpb_im(0, address.offset, address.base, address.index, address.scale);
        else
            m_assembler.testb_im(mask.m_value, address.offset, address.base, address.index, address.scale);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump jump()
    {
        return Jump(m_assembler.jmp());
    }

    void jump(RegisterID target)
    {
        m_assembler.jmp_r(target);
    }

    
    void jump(Address address)
    {
        m_assembler.jmp_m(address.offset, address.base);
    }

    
    
    
    
    
    
    
    
    
    
    
    Jump branchAdd32(Condition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        add32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchAdd32(Condition cond, Imm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        add32(imm, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchAdd32(Condition cond, Imm32 src, Address dest)
    {
        ASSERT((cond == Overflow) || (cond == Zero) || (cond == NonZero));
        add32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchAdd32(Condition cond, RegisterID src, Address dest)
    {
        ASSERT((cond == Overflow) || (cond == Zero) || (cond == NonZero));
        add32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchAdd32(Condition cond, Address src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Zero) || (cond == NonZero));
        add32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchMul32(Condition cond, RegisterID src, RegisterID dest)
    {
        ASSERT(cond == Overflow);
        mul32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchMul32(Condition cond, Address src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Zero) || (cond == NonZero));
        mul32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchMul32(Condition cond, Imm32 imm, RegisterID src, RegisterID dest)
    {
        ASSERT(cond == Overflow);
        mul32(imm, src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchSub32(Condition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        sub32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchSub32(Condition cond, Imm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        sub32(imm, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchSub32(Condition cond, Imm32 imm, Address dest)
    {
        ASSERT((cond == Overflow) || (cond == Zero) || (cond == NonZero));
        sub32(imm, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchSub32(Condition cond, RegisterID src, Address dest)
    {
        ASSERT((cond == Overflow) || (cond == Zero) || (cond == NonZero));
        sub32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchSub32(Condition cond, Address src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Zero) || (cond == NonZero));
        sub32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchNeg32(Condition cond, RegisterID srcDest)
    {
        ASSERT((cond == Overflow) || (cond == Zero) || (cond == NonZero));
        neg32(srcDest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchOr32(Condition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Signed) || (cond == Zero) || (cond == NonZero));
        or32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }


    

    void breakpoint()
    {
        m_assembler.int3();
    }

    Call nearCall()
    {
        return Call(m_assembler.call(), Call::LinkableNear);
    }

    Call call(RegisterID target)
    {
        return Call(m_assembler.call(target), Call::None);
    }

    void call(Address address)
    {
        m_assembler.call_m(address.offset, address.base);
    }

    void ret()
    {
        m_assembler.ret();
    }

    void set8(Condition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmpl_rr(right, left);
        m_assembler.setCC_r(x86Condition(cond), dest);
    }

    void set8(Condition cond, Address left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmpl_mr(left.offset, left.base, right);
        m_assembler.setCC_r(x86Condition(cond), dest);
    }

    void set8(Condition cond, RegisterID left, Imm32 right, RegisterID dest)
    {
        if (((cond == Equal) || (cond == NotEqual)) && !right.m_value)
            m_assembler.testl_rr(left, left);
        else
            m_assembler.cmpl_ir(right.m_value, left);
        m_assembler.setCC_r(x86Condition(cond), dest);
    }

    void set32(Condition cond, Address left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmpl_rm(right, left.offset, left.base);
        m_assembler.setCC_r(x86Condition(cond), dest);
        m_assembler.movzbl_rr(dest, dest);
    }

    void set32(Condition cond, RegisterID left, Address right, RegisterID dest)
    {
        m_assembler.cmpl_mr(right.offset, right.base, left);
        m_assembler.setCC_r(x86Condition(cond), dest);
        m_assembler.movzbl_rr(dest, dest);
    }

    void set32(Condition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmpl_rr(right, left);
        m_assembler.setCC_r(x86Condition(cond), dest);
        m_assembler.movzbl_rr(dest, dest);
    }

    void set32(Condition cond, Address left, Imm32 right, RegisterID dest)
    {
        m_assembler.cmpl_im(right.m_value, left.offset, left.base);
        m_assembler.setCC_r(x86Condition(cond), dest);
        m_assembler.movzbl_rr(dest, dest);
    }

    void set32(Condition cond, RegisterID left, Imm32 right, RegisterID dest)
    {
        if (((cond == Equal) || (cond == NotEqual)) && !right.m_value)
            m_assembler.testl_rr(left, left);
        else
            m_assembler.cmpl_ir(right.m_value, left);
        m_assembler.setCC_r(x86Condition(cond), dest);
        m_assembler.movzbl_rr(dest, dest);
    }

    
    
    
    

    void setTest8(Condition cond, Address address, Imm32 mask, RegisterID dest)
    {
        if (mask.m_value == -1)
            m_assembler.cmpb_im(0, address.offset, address.base);
        else
            m_assembler.testb_im(mask.m_value, address.offset, address.base);
        m_assembler.setCC_r(x86Condition(cond), dest);
        m_assembler.movzbl_rr(dest, dest);
    }

    void setTest32(Condition cond, Address address, Imm32 mask, RegisterID dest)
    {
        if (mask.m_value == -1)
            m_assembler.cmpl_im(0, address.offset, address.base);
        else
            m_assembler.testl_i32m(mask.m_value, address.offset, address.base);
        m_assembler.setCC_r(x86Condition(cond), dest);
        m_assembler.movzbl_rr(dest, dest);
    }

protected:
    X86Assembler::Condition x86Condition(Condition cond)
    {
        return static_cast<X86Assembler::Condition>(cond);
    }

private:
    
    
    friend class MacroAssemblerX86;

#if WTF_CPU_X86
#if WTF_PLATFORM_MAC

    
    static bool isSSE2Present()
    {
        return true;
    }

#else 

    enum SSE2CheckState {
        NotCheckedSSE2,
        HasSSE2,
        NoSSE2
    };

    static bool isSSE2Present()
    {
        if (s_sse2CheckState == NotCheckedSSE2) {
            
            
            int flags = 0;
#if WTF_COMPILER_MSVC
            _asm {
                mov eax, 1 
                cpuid;
                mov flags, edx;
            }
#elif WTF_COMPILER_GCC
            asm (
                 "movl $0x1, %%eax;"
                 "pushl %%ebx;"
                 "cpuid;"
                 "popl %%ebx;"
                 "movl %%edx, %0;"
                 : "=g" (flags)
                 :
                 : "%eax", "%ecx", "%edx"
                 );
#endif
            static const int SSE2FeatureBit = 1 << 26;
            s_sse2CheckState = (flags & SSE2FeatureBit) ? HasSSE2 : NoSSE2;
        }
        
        ASSERT(s_sse2CheckState != NotCheckedSSE2);

        return s_sse2CheckState == HasSSE2;
    }
    
    static SSE2CheckState s_sse2CheckState;

#endif 
#elif !defined(NDEBUG) 

    
    
    static bool isSSE2Present()
    {
        return true;
    }

#endif
};

} 

#endif 

#endif 
