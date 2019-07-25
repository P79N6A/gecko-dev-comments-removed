





























#ifndef MacroAssemblerARMv7_h
#define MacroAssemblerARMv7_h

#include "assembler/wtf/Platform.h"

#if ENABLE(ASSEMBLER)

#include "ARMv7Assembler.h"
#include "AbstractMacroAssembler.h"

namespace JSC {

class MacroAssemblerARMv7 : public AbstractMacroAssembler<ARMv7Assembler> {
    
    
    
    static const ARMRegisters::RegisterID dataTempRegister = ARMRegisters::ip;
    static const RegisterID addressTempRegister = ARMRegisters::r3;
    static const FPRegisterID fpTempRegister = ARMRegisters::d7;
    static const unsigned int TotalRegisters = 16;

    struct ArmAddress {
        enum AddressType {
            HasOffset,
            HasIndex,
        } type;
        RegisterID base;
        union {
            int32_t offset;
            struct {
                RegisterID index;
                Scale scale;
            };
        } u;
        
        explicit ArmAddress(RegisterID base, int32_t offset = 0)
            : type(HasOffset)
            , base(base)
        {
            u.offset = offset;
        }
        
        explicit ArmAddress(RegisterID base, RegisterID index, Scale scale = TimesOne)
            : type(HasIndex)
            , base(base)
        {
            u.index = index;
            u.scale = scale;
        }
    };
    
public:

    static const Scale ScalePtr = TimesFour;

    enum Condition {
        Equal = ARMv7Assembler::ConditionEQ,
        NotEqual = ARMv7Assembler::ConditionNE,
        Above = ARMv7Assembler::ConditionHI,
        AboveOrEqual = ARMv7Assembler::ConditionHS,
        Below = ARMv7Assembler::ConditionLO,
        BelowOrEqual = ARMv7Assembler::ConditionLS,
        GreaterThan = ARMv7Assembler::ConditionGT,
        GreaterThanOrEqual = ARMv7Assembler::ConditionGE,
        LessThan = ARMv7Assembler::ConditionLT,
        LessThanOrEqual = ARMv7Assembler::ConditionLE,
        Overflow = ARMv7Assembler::ConditionVS,
        Signed = ARMv7Assembler::ConditionMI,
        Zero = ARMv7Assembler::ConditionEQ,
        NonZero = ARMv7Assembler::ConditionNE
    };
    enum DoubleCondition {
        
        DoubleEqual = ARMv7Assembler::ConditionEQ,
        DoubleNotEqual = ARMv7Assembler::ConditionVC, 
        DoubleGreaterThan = ARMv7Assembler::ConditionGT,
        DoubleGreaterThanOrEqual = ARMv7Assembler::ConditionGE,
        DoubleLessThan = ARMv7Assembler::ConditionLO,
        DoubleLessThanOrEqual = ARMv7Assembler::ConditionLS,
        
        DoubleEqualOrUnordered = ARMv7Assembler::ConditionVS, 
        DoubleNotEqualOrUnordered = ARMv7Assembler::ConditionNE,
        DoubleGreaterThanOrUnordered = ARMv7Assembler::ConditionHI,
        DoubleGreaterThanOrEqualOrUnordered = ARMv7Assembler::ConditionHS,
        DoubleLessThanOrUnordered = ARMv7Assembler::ConditionLT,
        DoubleLessThanOrEqualOrUnordered = ARMv7Assembler::ConditionLE,
    };

    static const RegisterID stackPointerRegister = ARMRegisters::sp;
    static const RegisterID linkRegister = ARMRegisters::lr;

    
    
    
    
    
    

    void add32(RegisterID src, RegisterID dest)
    {
        m_assembler.add(dest, dest, src);
    }

    void add32(Imm32 imm, RegisterID dest)
    {
        add32(imm, dest, dest);
    }

    void add32(Imm32 imm, RegisterID src, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.add(dest, src, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.add(dest, src, dataTempRegister);
        }
    }

    void add32(Imm32 imm, Address address)
    {
        load32(address, dataTempRegister);

        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.add(dataTempRegister, dataTempRegister, armImm);
        else {
            
            
            move(imm, addressTempRegister);
            m_assembler.add(dataTempRegister, dataTempRegister, addressTempRegister);
        }

        store32(dataTempRegister, address);
    }

    void add32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        add32(dataTempRegister, dest);
    }

    void add32(Imm32 imm, AbsoluteAddress address)
    {
        load32(address.m_ptr, dataTempRegister);

        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.add(dataTempRegister, dataTempRegister, armImm);
        else {
            
            
            move(imm, addressTempRegister);
            m_assembler.add(dataTempRegister, dataTempRegister, addressTempRegister);
        }

        store32(dataTempRegister, address.m_ptr);
    }

    void and32(RegisterID src, RegisterID dest)
    {
        m_assembler.ARM_and(dest, dest, src);
    }

    void and32(Imm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.ARM_and(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.ARM_and(dest, dest, dataTempRegister);
        }
    }

    void lshift32(RegisterID shift_amount, RegisterID dest)
    {
        
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(0x1f);
        ASSERT(armImm.isValid());
        m_assembler.ARM_and(dataTempRegister, shift_amount, armImm);

        m_assembler.lsl(dest, dest, dataTempRegister);
    }

    void lshift32(Imm32 imm, RegisterID dest)
    {
        m_assembler.lsl(dest, dest, imm.m_value & 0x1f);
    }

    void mul32(RegisterID src, RegisterID dest)
    {
        m_assembler.smull(dest, dataTempRegister, dest, src);
    }

    void mul32(Imm32 imm, RegisterID src, RegisterID dest)
    {
        move(imm, dataTempRegister);
        m_assembler.smull(dest, dataTempRegister, src, dataTempRegister);
    }

    void not32(RegisterID srcDest)
    {
        m_assembler.mvn(srcDest, srcDest);
    }

    void or32(RegisterID src, RegisterID dest)
    {
        m_assembler.orr(dest, dest, src);
    }

    void or32(Imm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.orr(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.orr(dest, dest, dataTempRegister);
        }
    }

    void rshift32(RegisterID shift_amount, RegisterID dest)
    {
        
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(0x1f);
        ASSERT(armImm.isValid());
        m_assembler.ARM_and(dataTempRegister, shift_amount, armImm);

        m_assembler.asr(dest, dest, dataTempRegister);
    }

    void rshift32(Imm32 imm, RegisterID dest)
    {
        m_assembler.asr(dest, dest, imm.m_value & 0x1f);
    }
    
    void urshift32(RegisterID shift_amount, RegisterID dest)
    {
        
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(0x1f);
        ASSERT(armImm.isValid());
        m_assembler.ARM_and(dataTempRegister, shift_amount, armImm);
        
        m_assembler.lsr(dest, dest, dataTempRegister);
    }
    
    void urshift32(Imm32 imm, RegisterID dest)
    {
        m_assembler.lsr(dest, dest, imm.m_value & 0x1f);
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.sub(dest, dest, src);
    }

    void sub32(Imm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.sub(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.sub(dest, dest, dataTempRegister);
        }
    }

    void sub32(Imm32 imm, Address address)
    {
        load32(address, dataTempRegister);

        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.sub(dataTempRegister, dataTempRegister, armImm);
        else {
            
            
            move(imm, addressTempRegister);
            m_assembler.sub(dataTempRegister, dataTempRegister, addressTempRegister);
        }

        store32(dataTempRegister, address);
    }

    void sub32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        sub32(dataTempRegister, dest);
    }

    void sub32(Imm32 imm, AbsoluteAddress address)
    {
        load32(address.m_ptr, dataTempRegister);

        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.sub(dataTempRegister, dataTempRegister, armImm);
        else {
            
            
            move(imm, addressTempRegister);
            m_assembler.sub(dataTempRegister, dataTempRegister, addressTempRegister);
        }

        store32(dataTempRegister, address.m_ptr);
    }

    void xor32(RegisterID src, RegisterID dest)
    {
        m_assembler.eor(dest, dest, src);
    }

    void xor32(Imm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.eor(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.eor(dest, dest, dataTempRegister);
        }
    }
    

    
    
    
    
    
    

private:
    void load32(ArmAddress address, RegisterID dest)
    {
        if (address.type == ArmAddress::HasIndex)
            m_assembler.ldr(dest, address.base, address.u.index, address.u.scale);
        else if (address.u.offset >= 0) {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12(address.u.offset);
            ASSERT(armImm.isValid());
            m_assembler.ldr(dest, address.base, armImm);
        } else {
            ASSERT(address.u.offset >= -255);
            m_assembler.ldr(dest, address.base, address.u.offset, true, false);
        }
    }

    void load16(ArmAddress address, RegisterID dest)
    {
        if (address.type == ArmAddress::HasIndex)
            m_assembler.ldrh(dest, address.base, address.u.index, address.u.scale);
        else if (address.u.offset >= 0) {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12(address.u.offset);
            ASSERT(armImm.isValid());
            m_assembler.ldrh(dest, address.base, armImm);
        } else {
            ASSERT(address.u.offset >= -255);
            m_assembler.ldrh(dest, address.base, address.u.offset, true, false);
        }
    }

    void load8(ArmAddress address, RegisterID dest)
    {
        if (address.type == ArmAddress::HasIndex)
            m_assembler.ldrb(dest, address.base, address.u.index, address.u.scale);
        else if (address.u.offset >= 0) {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12(address.u.offset);
            ASSERT(armImm.isValid());
            m_assembler.ldrb(dest, address.base, armImm);
        } else {
            ASSERT(address.u.offset >= -255);
            m_assembler.ldrb(dest, address.base, address.u.offset, true, false);
        }
    }

    void store32(RegisterID src, ArmAddress address)
    {
        if (address.type == ArmAddress::HasIndex)
            m_assembler.str(src, address.base, address.u.index, address.u.scale);
        else if (address.u.offset >= 0) {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12(address.u.offset);
            ASSERT(armImm.isValid());
            m_assembler.str(src, address.base, armImm);
        } else {
            ASSERT(address.u.offset >= -255);
            m_assembler.str(src, address.base, address.u.offset, true, false);
        }
    }

public:
    void load32(ImplicitAddress address, RegisterID dest)
    {
        load32(setupArmAddress(address), dest);
    }

    void load32(BaseIndex address, RegisterID dest)
    {
        load32(setupArmAddress(address), dest);
    }

    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
    {
        load32(setupArmAddress(address), dest);
    }

    void load32(void* address, RegisterID dest)
    {
        move(ImmPtr(address), addressTempRegister);
        m_assembler.ldr(dest, addressTempRegister, ARMThumbImmediate::makeUInt16(0));
    }

    void load8(ImplicitAddress address, RegisterID dest)
    {
        load8(setupArmAddress(address), dest);
    }

    DataLabel32 load32WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        DataLabel32 label = moveWithPatch(Imm32(address.offset), dataTempRegister);
        load32(ArmAddress(address.base, dataTempRegister), dest);
        return label;
    }

    Label loadPtrWithPatchToLEA(Address address, RegisterID dest)
    {
        Label label(this);
        moveFixedWidthEncoding(Imm32(address.offset), dataTempRegister);
        load32(ArmAddress(address.base, dataTempRegister), dest);
        return label;
    }

    void load16(BaseIndex address, RegisterID dest)
    {
        m_assembler.ldrh(dest, makeBaseIndexBase(address), address.index, address.scale);
    }
    
    void load16(ImplicitAddress address, RegisterID dest)
    {
        m_assembler.ldrh(dest, address.base, address.offset);
    }

    DataLabel32 store32WithAddressOffsetPatch(RegisterID src, Address address)
    {
        DataLabel32 label = moveWithPatch(Imm32(address.offset), dataTempRegister);
        store32(src, ArmAddress(address.base, dataTempRegister));
        return label;
    }

    void store32(RegisterID src, ImplicitAddress address)
    {
        store32(src, setupArmAddress(address));
    }

    void store32(RegisterID src, BaseIndex address)
    {
        store32(src, setupArmAddress(address));
    }

    void store32(Imm32 imm, ImplicitAddress address)
    {
        move(imm, dataTempRegister);
        store32(dataTempRegister, setupArmAddress(address));
    }

    void store32(RegisterID src, void* address)
    {
        move(ImmPtr(address), addressTempRegister);
        m_assembler.str(src, addressTempRegister, ARMThumbImmediate::makeUInt16(0));
    }

    void store32(Imm32 imm, void* address)
    {
        move(imm, dataTempRegister);
        store32(dataTempRegister, address);
    }


    

    bool supportsFloatingPoint() const { return true; }
    
    
    
    
    
    
    
    
    
    
    
    bool supportsFloatingPointTruncate() const { return false; }

    bool supportsFloatingPointSqrt() const
    {
        return false;
    }

    void loadDouble(ImplicitAddress address, FPRegisterID dest)
    {
        RegisterID base = address.base;
        int32_t offset = address.offset;

        
        if ((offset & 3) || (offset > (255 * 4)) || (offset < -(255 * 4))) {
            add32(Imm32(offset), base, addressTempRegister);
            base = addressTempRegister;
            offset = 0;
        }
        
        m_assembler.vldr(dest, base, offset);
    }

    void storeDouble(FPRegisterID src, ImplicitAddress address)
    {
        RegisterID base = address.base;
        int32_t offset = address.offset;

        
        if ((offset & 3) || (offset > (255 * 4)) || (offset < -(255 * 4))) {
            add32(Imm32(offset), base, addressTempRegister);
            base = addressTempRegister;
            offset = 0;
        }
        
        m_assembler.vstr(src, base, offset);
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vadd_F64(dest, dest, src);
    }

    void addDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        addDouble(fpTempRegister, dest);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vsub_F64(dest, dest, src);
    }

    void subDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        subDouble(fpTempRegister, dest);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vmul_F64(dest, dest, src);
    }

    void mulDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        mulDouble(fpTempRegister, dest);
    }

    void sqrtDouble(FPRegisterID, FPRegisterID)
    {
        ASSERT_NOT_REACHED();
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.vmov(fpTempRegister, src);
        m_assembler.vcvt_F64_S32(dest, fpTempRegister);
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        m_assembler.vcmp_F64(left, right);
        m_assembler.vmrs_APSR_nzcv_FPSCR();

        if (cond == DoubleNotEqual) {
            
            Jump unordered = makeBranch(ARMv7Assembler::ConditionVS);
            Jump result = makeBranch(ARMv7Assembler::ConditionNE);
            unordered.link(this);
            return result;
        }
        if (cond == DoubleEqualOrUnordered) {
            Jump unordered = makeBranch(ARMv7Assembler::ConditionVS);
            Jump notEqual = makeBranch(ARMv7Assembler::ConditionNE);
            unordered.link(this);
            
            Jump result = makeJump();
            notEqual.link(this);
            return result;
        }
        return makeBranch(cond);
    }

    Jump branchTruncateDoubleToInt32(FPRegisterID, RegisterID)
    {
        ASSERT_NOT_REACHED();
        return jump();
    }


    
    
    
    
    
    
    
    
    void pop(RegisterID dest)
    {
        
        m_assembler.ldr(dest, ARMRegisters::sp, sizeof(void*), false, true);
    }

    void push(RegisterID src)
    {
        
        m_assembler.str(src, ARMRegisters::sp, -sizeof(void*), true, true);
    }

    void push(Address address)
    {
        load32(address, dataTempRegister);
        push(dataTempRegister);
    }

    void push(Imm32 imm)
    {
        move(imm, dataTempRegister);
        push(dataTempRegister);
    }

    
    
    

    void move(Imm32 imm, RegisterID dest)
    {
        uint32_t value = imm.m_value;

        if (imm.m_isPointer)
            moveFixedWidthEncoding(imm, dest);
        else {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(value);

            if (armImm.isValid())
                m_assembler.mov(dest, armImm);
            else if ((armImm = ARMThumbImmediate::makeEncodedImm(~value)).isValid())
                m_assembler.mvn(dest, armImm);
            else {
                m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(value));
                if (value & 0xffff0000)
                    m_assembler.movt(dest, ARMThumbImmediate::makeUInt16(value >> 16));
            }
        }
    }

    void move(RegisterID src, RegisterID dest)
    {
        m_assembler.mov(dest, src);
    }

    void move(ImmPtr imm, RegisterID dest)
    {
        move(Imm32(imm), dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        move(reg1, dataTempRegister);
        move(reg2, reg1);
        move(dataTempRegister, reg2);
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        if (src != dest)
            move(src, dest);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        if (src != dest)
            move(src, dest);
    }


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
private:

    
    void compare32(RegisterID left, Imm32 right)
    {
        int32_t imm = right.m_value;
        if (!imm)
            m_assembler.tst(left, left);
        else {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm);
            if (armImm.isValid())
                m_assembler.cmp(left, armImm);
            if ((armImm = ARMThumbImmediate::makeEncodedImm(-imm)).isValid())
                m_assembler.cmn(left, armImm);
            else {
                move(Imm32(imm), dataTempRegister);
                m_assembler.cmp(left, dataTempRegister);
            }
        }
    }

    void test32(RegisterID reg, Imm32 mask)
    {
        int32_t imm = mask.m_value;

        if (imm == -1)
            m_assembler.tst(reg, reg);
        else {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm);
            if (armImm.isValid())
                m_assembler.tst(reg, armImm);
            else {
                move(mask, dataTempRegister);
                m_assembler.tst(reg, dataTempRegister);
            }
        }
    }

public:
    Jump branch32(Condition cond, RegisterID left, RegisterID right)
    {
        m_assembler.cmp(left, right);
        return Jump(makeBranch(cond));
    }

    Jump branch32(Condition cond, RegisterID left, Imm32 right)
    {
        compare32(left, right);
        return Jump(makeBranch(cond));
    }

    Jump branch32(Condition cond, RegisterID left, Address right)
    {
        load32(right, dataTempRegister);
        return branch32(cond, left, dataTempRegister);
    }

    Jump branch32(Condition cond, Address left, RegisterID right)
    {
        load32(left, dataTempRegister);
        return branch32(cond, dataTempRegister, right);
    }

    Jump branch32(Condition cond, Address left, Imm32 right)
    {
        
        load32(left, addressTempRegister);
        return branch32(cond, addressTempRegister, right);
    }

    Jump branch32(Condition cond, BaseIndex left, Imm32 right)
    {
        
        load32(left, addressTempRegister);
        return branch32(cond, addressTempRegister, right);
    }

    Jump branch32WithUnalignedHalfWords(Condition cond, BaseIndex left, Imm32 right)
    {
        
        load32WithUnalignedHalfWords(left, addressTempRegister);
        return branch32(cond, addressTempRegister, right);
    }

    Jump branch32(Condition cond, AbsoluteAddress left, RegisterID right)
    {
        load32(left.m_ptr, dataTempRegister);
        return branch32(cond, dataTempRegister, right);
    }

    Jump branch32(Condition cond, AbsoluteAddress left, Imm32 right)
    {
        
        load32(left.m_ptr, addressTempRegister);
        return branch32(cond, addressTempRegister, right);
    }

    Jump branch16(Condition cond, BaseIndex left, RegisterID right)
    {
        load16(left, dataTempRegister);
        m_assembler.lsl(addressTempRegister, right, 16);
        m_assembler.lsl(dataTempRegister, dataTempRegister, 16);
        return branch32(cond, dataTempRegister, addressTempRegister);
    }

    Jump branch16(Condition cond, BaseIndex left, Imm32 right)
    {
        
        load16(left, addressTempRegister);
        m_assembler.lsl(addressTempRegister, addressTempRegister, 16);
        return branch32(cond, addressTempRegister, Imm32(right.m_value << 16));
    }

    Jump branch8(Condition cond, RegisterID left, Imm32 right)
    {
        compare32(left, right);
        return Jump(makeBranch(cond));
    }

    Jump branch8(Condition cond, Address left, Imm32 right)
    {
        
        load8(left, addressTempRegister);
        return branch8(cond, addressTempRegister, right);
    }

    Jump branchTest32(Condition cond, RegisterID reg, RegisterID mask)
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        m_assembler.tst(reg, mask);
        return Jump(makeBranch(cond));
    }

    Jump branchTest32(Condition cond, RegisterID reg, Imm32 mask = Imm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        test32(reg, mask);
        return Jump(makeBranch(cond));
    }

    Jump branchTest32(Condition cond, Address address, Imm32 mask = Imm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        
        load32(address, addressTempRegister);
        return branchTest32(cond, addressTempRegister, mask);
    }

    Jump branchTest32(Condition cond, BaseIndex address, Imm32 mask = Imm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        
        load32(address, addressTempRegister);
        return branchTest32(cond, addressTempRegister, mask);
    }

    Jump branchTest8(Condition cond, RegisterID reg, Imm32 mask = Imm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        test32(reg, mask);
        return Jump(makeBranch(cond));
    }

    Jump branchTest8(Condition cond, Address address, Imm32 mask = Imm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        
        load8(address, addressTempRegister);
        return branchTest8(cond, addressTempRegister, mask);
    }

    Jump jump()
    {
        return Jump(makeJump());
    }

    void jump(RegisterID target)
    {
        m_assembler.bx(target);
    }

    
    void jump(Address address)
    {
        load32(address, dataTempRegister);
        m_assembler.bx(dataTempRegister);
    }


    
    
    
    
    
    
    
    
    
    
    Jump branchAdd32(Condition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        m_assembler.add_S(dest, dest, src);
        return Jump(makeBranch(cond));
    }

    Jump branchAdd32(Condition cond, Imm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.add_S(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.add_S(dest, dest, dataTempRegister);
        }
        return Jump(makeBranch(cond));
    }

    Jump branchMul32(Condition cond, RegisterID src, RegisterID dest)
    {
        ASSERT(cond == Overflow);
        m_assembler.smull(dest, dataTempRegister, dest, src);
        m_assembler.asr(addressTempRegister, dest, 31);
        return branch32(NotEqual, addressTempRegister, dataTempRegister);
    }

    Jump branchMul32(Condition cond, Imm32 imm, RegisterID src, RegisterID dest)
    {
        ASSERT(cond == Overflow);
        move(imm, dataTempRegister);
        m_assembler.smull(dest, dataTempRegister, src, dataTempRegister);
        m_assembler.asr(addressTempRegister, dest, 31);
        return branch32(NotEqual, addressTempRegister, dataTempRegister);
    }

    Jump branchSub32(Condition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        m_assembler.sub_S(dest, dest, src);
        return Jump(makeBranch(cond));
    }

    Jump branchSub32(Condition cond, Imm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.sub_S(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.sub_S(dest, dest, dataTempRegister);
        }
        return Jump(makeBranch(cond));
    }
    

    

    void breakpoint()
    {
        m_assembler.bkpt();
    }

    Call nearCall()
    {
        moveFixedWidthEncoding(Imm32(0), dataTempRegister);
        return Call(m_assembler.blx(dataTempRegister), Call::LinkableNear);
    }

    Call call()
    {
        moveFixedWidthEncoding(Imm32(0), dataTempRegister);
        return Call(m_assembler.blx(dataTempRegister), Call::Linkable);
    }

    Call call(RegisterID target)
    {
        return Call(m_assembler.blx(target), Call::None);
    }

    Call call(Address address)
    {
        load32(address, dataTempRegister);
        return Call(m_assembler.blx(dataTempRegister), Call::None);
    }

    void ret()
    {
        m_assembler.bx(linkRegister);
    }

    void set32(Condition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmp(left, right);
        m_assembler.it(armV7Condition(cond), false);
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(1));
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(0));
    }

    void set32(Condition cond, RegisterID left, Imm32 right, RegisterID dest)
    {
        compare32(left, right);
        m_assembler.it(armV7Condition(cond), false);
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(1));
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(0));
    }

    
    
    
    
    void setTest32(Condition cond, Address address, Imm32 mask, RegisterID dest)
    {
        load32(address, dataTempRegister);
        test32(dataTempRegister, mask);
        m_assembler.it(armV7Condition(cond), false);
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(1));
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(0));
    }

    void setTest8(Condition cond, Address address, Imm32 mask, RegisterID dest)
    {
        load8(address, dataTempRegister);
        test32(dataTempRegister, mask);
        m_assembler.it(armV7Condition(cond), false);
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(1));
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(0));
    }

    DataLabel32 moveWithPatch(Imm32 imm, RegisterID dst)
    {
        moveFixedWidthEncoding(imm, dst);
        return DataLabel32(this);
    }

    DataLabelPtr moveWithPatch(ImmPtr imm, RegisterID dst)
    {
        moveFixedWidthEncoding(Imm32(imm), dst);
        return DataLabelPtr(this);
    }

    Jump branchPtrWithPatch(Condition cond, RegisterID left, DataLabelPtr& dataLabel, ImmPtr initialRightValue = ImmPtr(0))
    {
        dataLabel = moveWithPatch(initialRightValue, dataTempRegister);
        return branch32(cond, left, dataTempRegister);
    }

    Jump branchPtrWithPatch(Condition cond, Address left, DataLabelPtr& dataLabel, ImmPtr initialRightValue = ImmPtr(0))
    {
        load32(left, addressTempRegister);
        dataLabel = moveWithPatch(initialRightValue, dataTempRegister);
        return branch32(cond, addressTempRegister, dataTempRegister);
    }

    DataLabelPtr storePtrWithPatch(ImmPtr initialValue, ImplicitAddress address)
    {
        DataLabelPtr label = moveWithPatch(initialValue, dataTempRegister);
        store32(dataTempRegister, address);
        return label;
    }
    DataLabelPtr storePtrWithPatch(ImplicitAddress address) { return storePtrWithPatch(ImmPtr(0), address); }


    Call tailRecursiveCall()
    {
        
        moveFixedWidthEncoding(Imm32(0), dataTempRegister);
        return Call(m_assembler.bx(dataTempRegister), Call::Linkable);
    }

    Call makeTailRecursiveCall(Jump oldJump)
    {
        oldJump.link(this);
        return tailRecursiveCall();
    }


protected:
    ARMv7Assembler::JmpSrc makeJump()
    {
        moveFixedWidthEncoding(Imm32(0), dataTempRegister);
        return m_assembler.bx(dataTempRegister);
    }

    ARMv7Assembler::JmpSrc makeBranch(ARMv7Assembler::Condition cond)
    {
        m_assembler.it(cond, true, true);
        moveFixedWidthEncoding(Imm32(0), dataTempRegister);
        return m_assembler.bx(dataTempRegister);
    }
    ARMv7Assembler::JmpSrc makeBranch(Condition cond) { return makeBranch(armV7Condition(cond)); }
    ARMv7Assembler::JmpSrc makeBranch(DoubleCondition cond) { return makeBranch(armV7Condition(cond)); }

    ArmAddress setupArmAddress(BaseIndex address)
    {
        if (address.offset) {
            ARMThumbImmediate imm = ARMThumbImmediate::makeUInt12OrEncodedImm(address.offset);
            if (imm.isValid())
                m_assembler.add(addressTempRegister, address.base, imm);
            else {
                move(Imm32(address.offset), addressTempRegister);
                m_assembler.add(addressTempRegister, addressTempRegister, address.base);
            }

            return ArmAddress(addressTempRegister, address.index, address.scale);
        } else
            return ArmAddress(address.base, address.index, address.scale);
    }

    ArmAddress setupArmAddress(Address address)
    {
        if ((address.offset >= -0xff) && (address.offset <= 0xfff))
            return ArmAddress(address.base, address.offset);

        move(Imm32(address.offset), addressTempRegister);
        return ArmAddress(address.base, addressTempRegister);
    }

    ArmAddress setupArmAddress(ImplicitAddress address)
    {
        if ((address.offset >= -0xff) && (address.offset <= 0xfff))
            return ArmAddress(address.base, address.offset);

        move(Imm32(address.offset), addressTempRegister);
        return ArmAddress(address.base, addressTempRegister);
    }

    RegisterID makeBaseIndexBase(BaseIndex address)
    {
        if (!address.offset)
            return address.base;

        ARMThumbImmediate imm = ARMThumbImmediate::makeUInt12OrEncodedImm(address.offset);
        if (imm.isValid())
            m_assembler.add(addressTempRegister, address.base, imm);
        else {
            move(Imm32(address.offset), addressTempRegister);
            m_assembler.add(addressTempRegister, addressTempRegister, address.base);
        }

        return addressTempRegister;
    }

    void moveFixedWidthEncoding(Imm32 imm, RegisterID dst)
    {
        uint32_t value = imm.m_value;
        m_assembler.movT3(dst, ARMThumbImmediate::makeUInt16(value & 0xffff));
        m_assembler.movt(dst, ARMThumbImmediate::makeUInt16(value >> 16));
    }

    ARMv7Assembler::Condition armV7Condition(Condition cond)
    {
        return static_cast<ARMv7Assembler::Condition>(cond);
    }

    ARMv7Assembler::Condition armV7Condition(DoubleCondition cond)
    {
        return static_cast<ARMv7Assembler::Condition>(cond);
    }

private:
    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkCall(void* code, Call call, FunctionPtr function)
    {
        ARMv7Assembler::linkCall(code, call.m_jmp, function.value());
    }

    static void repatchCall(CodeLocationCall call, CodeLocationLabel destination)
    {
        ARMv7Assembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

    static void repatchCall(CodeLocationCall call, FunctionPtr destination)
    {
        ARMv7Assembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }
};

} 

#endif 

#endif 
