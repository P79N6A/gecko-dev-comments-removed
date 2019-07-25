

























#ifndef MacroAssemblerMIPS_h
#define MacroAssemblerMIPS_h

#if ENABLE(ASSEMBLER) && CPU(MIPS)

#include "AbstractMacroAssembler.h"
#include "MIPSAssembler.h"

namespace JSC {

class MacroAssemblerMIPS : public AbstractMacroAssembler<MIPSAssembler> {
public:
    typedef MIPSRegisters::FPRegisterID FPRegisterID;

    MacroAssemblerMIPS()
        : m_fixedWidth(false)
    {
    }

    static const Scale ScalePtr = TimesFour;
    static const unsigned int TotalRegisters = 24; 
						   
						   

    
    static const RegisterID immTempRegister = MIPSRegisters::t0;
    
    static const RegisterID dataTempRegister = MIPSRegisters::t1;
    
    static const RegisterID addrTempRegister = MIPSRegisters::t2;
    
    static const RegisterID cmpTempRegister = MIPSRegisters::t3;
    
    static const RegisterID dataTemp2Register = MIPSRegisters::t4;

    
    static const FPRegisterID fpTempRegister = MIPSRegisters::f16;

    enum Condition {
        Equal,
        NotEqual,
        Above,
        AboveOrEqual,
        Below,
        BelowOrEqual,
        GreaterThan,
        GreaterThanOrEqual,
        LessThan,
        LessThanOrEqual,
        Overflow,
        Signed,
        Zero,
        NonZero
    };

    enum DoubleCondition {
        DoubleEqual,
        DoubleNotEqual,
        DoubleGreaterThan,
        DoubleGreaterThanOrEqual,
        DoubleLessThan,
        DoubleLessThanOrEqual,
        DoubleEqualOrUnordered,
        DoubleNotEqualOrUnordered,
        DoubleGreaterThanOrUnordered,
        DoubleGreaterThanOrEqualOrUnordered,
        DoubleLessThanOrUnordered,
        DoubleLessThanOrEqualOrUnordered
    };

    static const RegisterID stackPointerRegister = MIPSRegisters::sp;
    static const RegisterID returnAddressRegister = MIPSRegisters::ra;

    
    
    
    
    
    

    void lea(Address address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.addiu(dest, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.addiu(dest, addrTempRegister, address.offset);
        }
    }

    void lea(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            




            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.addiu(dest, addrTempRegister, address.offset);
        } else {
            






            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.addiu(dest, addrTempRegister, address.offset);
        }
    }

    void add32(RegisterID src, RegisterID dest)
    {
        m_assembler.addu(dest, dest, src);
    }

    void add32(TrustedImm32 imm, RegisterID dest)
    {
        add32(imm, dest, dest);
    }

    void add32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (!imm.m_isPointer && imm.m_value >= -32768 && imm.m_value <= 32767
            && !m_fixedWidth) {
            


            m_assembler.addiu(dest, src, imm.m_value);
        } else {
            



            move(imm, immTempRegister);
            m_assembler.addu(dest, src, immTempRegister);
        }
    }

    void add32(TrustedImm32 imm, Address address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            





            m_assembler.lw(dataTempRegister, address.base, address.offset);
            if (!imm.m_isPointer
                && imm.m_value >= -32768 && imm.m_value <= 32767
                && !m_fixedWidth)
                m_assembler.addiu(dataTempRegister, dataTempRegister,
                                  imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.addu(dataTempRegister, dataTempRegister,
                                 immTempRegister);
            }
            m_assembler.sw(dataTempRegister, address.base, address.offset);
        } else {
            







            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lw(dataTempRegister, addrTempRegister, address.offset);

            if (imm.m_value >= -32768 && imm.m_value <= 32767 && !m_fixedWidth)
                m_assembler.addiu(dataTempRegister, dataTempRegister,
                                  imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.addu(dataTempRegister, dataTempRegister,
                                 immTempRegister);
            }
            m_assembler.sw(dataTempRegister, addrTempRegister, address.offset);
        }
    }

    void add32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        add32(dataTempRegister, dest);
    }

    void add32(RegisterID src, Address dest)
    {
        if (dest.offset >= -32768 && dest.offset <= 32767 && !m_fixedWidth) {
            




            m_assembler.lw(dataTempRegister, dest.base, dest.offset);
            m_assembler.addu(dataTempRegister, dataTempRegister, src);
            m_assembler.sw(dataTempRegister, dest.base, dest.offset);
        } else {
            






            m_assembler.lui(addrTempRegister, (dest.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, dest.base);
            m_assembler.lw(dataTempRegister, addrTempRegister, dest.offset);
            m_assembler.addu(dataTempRegister, dataTempRegister, src);
            m_assembler.sw(dataTempRegister, addrTempRegister, dest.offset);
        }
    }

    void add32(TrustedImm32 imm, AbsoluteAddress address)
    {
        






        move(ImmPtr(address.m_ptr), addrTempRegister);
        m_assembler.lw(dataTempRegister, addrTempRegister, 0);
        if (!imm.m_isPointer && imm.m_value >= -32768 && imm.m_value <= 32767
            && !m_fixedWidth)
            m_assembler.addiu(dataTempRegister, dataTempRegister, imm.m_value);
        else {
            move(imm, immTempRegister);
            m_assembler.addu(dataTempRegister, dataTempRegister, immTempRegister);
        }
        m_assembler.sw(dataTempRegister, addrTempRegister, 0);
    }

    
    void setShiftedCanonicalNaN(RegisterID src, RegisterID dest)
    {
        






        move(TrustedImm32(0x7FF80000 << 1), dataTempRegister);
        move(TrustedImm32(0x7FF7FFFF << 1), immTempRegister);
        xor32(src, immTempRegister);
        move(src, dest);
        m_assembler.movz(dest, dataTempRegister, immTempRegister);
    }

    void and32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        m_assembler.andInsn(dest, dest, dataTempRegister);
    }

    void and32(RegisterID src, RegisterID dest)
    {
        m_assembler.andInsn(dest, dest, src);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
            move(MIPSRegisters::zero, dest);
        else if (!imm.m_isPointer && imm.m_value > 0 && imm.m_value < 65535
                 && !m_fixedWidth)
            m_assembler.andi(dest, dest, imm.m_value);
        else {
            



            move(imm, immTempRegister);
            m_assembler.andInsn(dest, dest, immTempRegister);
        }
    }

    void lshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.sll(dest, dest, imm.m_value);
    }

    void lshift32(RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.sllv(dest, dest, shiftAmount);
    }

    void mul32(RegisterID src, RegisterID dest)
    {
        m_assembler.mul(dest, dest, src);
    }

    void mul32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
            move(MIPSRegisters::zero, dest);
        else if (!imm.m_isPointer && imm.m_value == 1 && !m_fixedWidth)
            move(src, dest);
        else {
            



            move(imm, dataTempRegister);
            m_assembler.mul(dest, src, dataTempRegister);
        }
    }

    void neg32(RegisterID srcDest)
    {
        m_assembler.subu(srcDest, MIPSRegisters::zero, srcDest);
    }

    void not32(RegisterID srcDest)
    {
        m_assembler.nor(srcDest, srcDest, MIPSRegisters::zero);
    }

    void or32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        m_assembler.orInsn(dest, dest, dataTempRegister);
    }

    void or32(RegisterID src, RegisterID dest)
    {
        m_assembler.orInsn(dest, dest, src);
    }

    void or32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
            return;

        if (!imm.m_isPointer && imm.m_value > 0 && imm.m_value < 65535
            && !m_fixedWidth) {
            m_assembler.ori(dest, dest, imm.m_value);
            return;
        }

        



        move(imm, dataTempRegister);
        m_assembler.orInsn(dest, dest, dataTempRegister);
    }

    void rshift32(RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.srav(dest, dest, shiftAmount);
    }

    void rshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.sra(dest, dest, imm.m_value);
    }

    void urshift32(RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.srlv(dest, dest, shiftAmount);
    }

    void urshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.srl(dest, dest, imm.m_value);
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.subu(dest, dest, src);
    }

    void sub32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_isPointer && imm.m_value >= -32767 && imm.m_value <= 32768
            && !m_fixedWidth) {
            


            m_assembler.addiu(dest, dest, -imm.m_value);
        } else {
            



            move(imm, immTempRegister);
            m_assembler.subu(dest, dest, immTempRegister);
        }
    }

    void sub32(TrustedImm32 imm, Address address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            





            m_assembler.lw(dataTempRegister, address.base, address.offset);
            if (!imm.m_isPointer
                && imm.m_value >= -32767 && imm.m_value <= 32768
                && !m_fixedWidth)
                m_assembler.addiu(dataTempRegister, dataTempRegister,
                                  -imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.subu(dataTempRegister, dataTempRegister,
                                 immTempRegister);
            }
            m_assembler.sw(dataTempRegister, address.base, address.offset);
        } else {
            







            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lw(dataTempRegister, addrTempRegister, address.offset);

            if (!imm.m_isPointer
                && imm.m_value >= -32767 && imm.m_value <= 32768
                && !m_fixedWidth)
                m_assembler.addiu(dataTempRegister, dataTempRegister,
                                  -imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.subu(dataTempRegister, dataTempRegister,
                                 immTempRegister);
            }
            m_assembler.sw(dataTempRegister, addrTempRegister, address.offset);
        }
    }

    void sub32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        sub32(dataTempRegister, dest);
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address)
    {
        






        move(ImmPtr(address.m_ptr), addrTempRegister);
        m_assembler.lw(dataTempRegister, addrTempRegister, 0);

        if (!imm.m_isPointer && imm.m_value >= -32767 && imm.m_value <= 32768
            && !m_fixedWidth) {
            m_assembler.addiu(dataTempRegister, dataTempRegister,
                              -imm.m_value);
        } else {
            move(imm, immTempRegister);
            m_assembler.subu(dataTempRegister, dataTempRegister, immTempRegister);
        }
        m_assembler.sw(dataTempRegister, addrTempRegister, 0);
    }

    void xor32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        m_assembler.xorInsn(dest, dest, dataTempRegister);
    }

    void xor32(RegisterID src, RegisterID dest)
    {
        m_assembler.xorInsn(dest, dest, src);
    }

    void xor32(TrustedImm32 imm, RegisterID dest)
    {
        



        move(imm, immTempRegister);
        m_assembler.xorInsn(dest, dest, immTempRegister);
    }

    void load16SignExtend(ImplicitAddress address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.lh(dest, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lh(dest, addrTempRegister, address.offset);
        }
    }

    void load16ZeroExtend(ImplicitAddress address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.lhu(dest, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lhu(dest, addrTempRegister, address.offset);
        }
    }

    void load16SignExtend(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            




            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lh(dest, addrTempRegister, address.offset);
        } else {
            






            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.lh(dest, addrTempRegister, address.offset);
        }
    }

    void load16ZeroExtend(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            




            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lhu(dest, addrTempRegister, address.offset);
        } else {
            






            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.lhu(dest, addrTempRegister, address.offset);
        }
    }

    void load8SignExtend(ImplicitAddress address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.lb(dest, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lb(dest, addrTempRegister, address.offset);
        }
    }

    void load8ZeroExtend(ImplicitAddress address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.lbu(dest, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lbu(dest, addrTempRegister, address.offset);
        }
    }

    void load8SignExtend(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            




            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lb(dest, addrTempRegister, address.offset);
        } else {
            






            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.lb(dest, addrTempRegister, address.offset);
        }
    }

    void load8ZeroExtend(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            




            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lbu(dest, addrTempRegister, address.offset);
        } else {
            






            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.lbu(dest, addrTempRegister, address.offset);
        }
    }

    void negDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.negd(dest, src);
    }

    void absDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.absd(dest, src);
    }

    void sqrtDouble(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.sqrtd(dst, src);
    }

    
    
    
    
    
    

    
    void load8(ImplicitAddress address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.lbu(dest, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lbu(dest, addrTempRegister, address.offset);
        }
    }

    void load32(ImplicitAddress address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.lw(dest, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lw(dest, addrTempRegister, address.offset);
        }
    }

    void load32(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            




            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lw(dest, addrTempRegister, address.offset);
        } else {
            






            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.lw(dest, addrTempRegister, address.offset);
        }
    }

    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32764
            && !m_fixedWidth) {
            









            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
#if CPU(BIG_ENDIAN)
            m_assembler.lwl(dest, addrTempRegister, address.offset);
            m_assembler.lwr(dest, addrTempRegister, address.offset + 3);
#else
            m_assembler.lwl(dest, addrTempRegister, address.offset + 3);
            m_assembler.lwr(dest, addrTempRegister, address.offset);

#endif
        } else {
            












            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, address.offset >> 16);
            m_assembler.ori(immTempRegister, immTempRegister, address.offset);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
#if CPU(BIG_ENDIAN)
            m_assembler.lwl(dest, addrTempRegister, 0);
            m_assembler.lwr(dest, addrTempRegister, 3);
#else
            m_assembler.lwl(dest, addrTempRegister, 3);
            m_assembler.lwr(dest, addrTempRegister, 0);
#endif
        }
    }

    void load32(void* address, RegisterID dest)
    {
        



        move(ImmPtr(address), addrTempRegister);
        m_assembler.lw(dest, addrTempRegister, 0);
    }

    DataLabel32 load32WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        m_fixedWidth = true;
        





        DataLabel32 dataLabel(this);
        move(Imm32(address.offset), addrTempRegister);
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.lw(dest, addrTempRegister, 0);
        m_fixedWidth = false;
        return dataLabel;
    }

    void load64WithPatch(Address address, RegisterID tag, RegisterID payload, int tag_offset, int payload_offset)
    {
        m_fixedWidth = true;
        






        Label label(this);
        move(Imm32(address.offset), addrTempRegister);
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.lw(tag, addrTempRegister, tag_offset);
        m_assembler.lw(payload, addrTempRegister, payload_offset);
        m_fixedWidth = false;
    }

    void store64WithPatch(Address address, RegisterID tag, RegisterID payload, int tag_offset, int payload_offset)
    {
        m_fixedWidth = true;
        






        Label label(this);
        move(Imm32(address.offset), addrTempRegister);
        m_fixedWidth = false;
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.sw(tag, addrTempRegister, tag_offset);
        m_assembler.sw(payload, addrTempRegister, payload_offset);
    }

    void store64WithPatch(Address address, Imm32 tag, RegisterID payload, int tag_offset, int payload_offset)
    {
        m_fixedWidth = true;
        







        Label label(this);
        move(Imm32(address.offset), addrTempRegister);
        m_fixedWidth = false;
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        move(tag, immTempRegister);
        m_assembler.sw(immTempRegister, addrTempRegister, tag_offset);
        m_assembler.sw(payload, addrTempRegister, payload_offset);
    }

    void store64WithPatch(Address address, Imm32 tag, Imm32 payload, int tag_offset, int payload_offset)
    {
        m_fixedWidth = true;
        








        Label label(this);
        move(Imm32(address.offset), addrTempRegister);
        m_fixedWidth = false;
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        move(tag, immTempRegister);
        m_assembler.sw(immTempRegister, addrTempRegister, tag_offset);
        move(payload, immTempRegister);
        m_assembler.sw(immTempRegister, addrTempRegister, payload_offset);
    }

    Label loadPtrWithPatchToLEA(Address address, RegisterID dest)
    {
        m_fixedWidth = true;
        





        Label label(this);
        move(Imm32(address.offset), addrTempRegister);
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.lw(dest, addrTempRegister, 0);
        m_fixedWidth = false;
        return label;
    }

    
    void load16(ImplicitAddress address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.lhu(dest, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lhu(dest, addrTempRegister, address.offset);
        }
    }

    
    void load16(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            




            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lhu(dest, addrTempRegister, address.offset);
        } else {
            






            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.lhu(dest, addrTempRegister, address.offset);
        }
    }

    DataLabel32 store32WithAddressOffsetPatch(RegisterID src, Address address)
    {
        m_fixedWidth = true;
        





        DataLabel32 dataLabel(this);
        move(Imm32(address.offset), addrTempRegister);
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.sw(src, addrTempRegister, 0);
        m_fixedWidth = false;
        return dataLabel;
    }

    void store8(RegisterID src, ImplicitAddress address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.sb(src, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sb(src, addrTempRegister, address.offset);
        }
    }

    void store8(RegisterID src, BaseIndex address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            




            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sb(src, addrTempRegister, address.offset);
        } else {
            






            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.sb(src, addrTempRegister, address.offset);
        }
    }

    void store8(TrustedImm32 imm, BaseIndex address)
    {
        if (!imm.m_isPointer && !imm.m_value)
            store8(MIPSRegisters::zero, address);
        else {
            move(imm, immTempRegister);
            store8(immTempRegister, address);
        }
    }

    void store8(TrustedImm32 imm, ImplicitAddress address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            if (!imm.m_isPointer && !imm.m_value)
                m_assembler.sb(MIPSRegisters::zero, address.base,
                               address.offset);
            else {
                move(imm, immTempRegister);
                m_assembler.sb(immTempRegister, address.base, address.offset);
            }
        } else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
                m_assembler.sb(MIPSRegisters::zero, addrTempRegister,
                               address.offset);
            else {
                move(imm, immTempRegister);
                m_assembler.sb(immTempRegister, addrTempRegister,
                               address.offset);
            }
        }
    }

    void store8(RegisterID src, void* address)
    {
        



        move(ImmPtr(address), addrTempRegister);
        m_assembler.sb(src, addrTempRegister, 0);
    }

    void store8(TrustedImm32 imm, void* address)
    {
        




        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth) {
            move(ImmPtr(address), addrTempRegister);
            m_assembler.sb(MIPSRegisters::zero, addrTempRegister, 0);
        } else {
            move(imm, immTempRegister);
            move(ImmPtr(address), addrTempRegister);
            m_assembler.sb(immTempRegister, addrTempRegister, 0);
        }
    }

    void store16(RegisterID src, ImplicitAddress address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.sh(src, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sh(src, addrTempRegister, address.offset);
        }
    }

    void store16(RegisterID src, BaseIndex address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            




            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sh(src, addrTempRegister, address.offset);
        } else {
            






            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.sh(src, addrTempRegister, address.offset);
        }
    }

    void store16(TrustedImm32 imm, BaseIndex address)
    {
        if (!imm.m_isPointer && !imm.m_value)
            store16(MIPSRegisters::zero, address);
        else {
            move(imm, immTempRegister);
            store16(immTempRegister, address);
        }
    }

    void store16(TrustedImm32 imm, ImplicitAddress address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            if (!imm.m_isPointer && !imm.m_value)
                m_assembler.sh(MIPSRegisters::zero, address.base,
                               address.offset);
            else {
                move(imm, immTempRegister);
                m_assembler.sh(immTempRegister, address.base, address.offset);
            }
        } else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
                m_assembler.sh(MIPSRegisters::zero, addrTempRegister,
                               address.offset);
            else {
                move(imm, immTempRegister);
                m_assembler.sh(immTempRegister, addrTempRegister,
                               address.offset);
            }
        }
    }

    void store16(RegisterID src, void* address)
    {
        



        move(ImmPtr(address), addrTempRegister);
        m_assembler.sh(src, addrTempRegister, 0);
    }

    void store16(TrustedImm32 imm, void* address)
    {
        




        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth) {
            move(ImmPtr(address), addrTempRegister);
            m_assembler.sh(MIPSRegisters::zero, addrTempRegister, 0);
        } else {
            move(imm, immTempRegister);
            move(ImmPtr(address), addrTempRegister);
            m_assembler.sh(immTempRegister, addrTempRegister, 0);
        }
    }

    void store32(RegisterID src, ImplicitAddress address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.sw(src, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sw(src, addrTempRegister, address.offset);
        }
    }

    void store32(RegisterID src, BaseIndex address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            




            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sw(src, addrTempRegister, address.offset);
        } else {
            






            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.sw(src, addrTempRegister, address.offset);
        }
    }

    void store32(TrustedImm32 imm, BaseIndex address)
    {
        if (!imm.m_isPointer && !imm.m_value)
            store32(MIPSRegisters::zero, address);
        else {
            move(imm, immTempRegister);
            store32(immTempRegister, address);
        }
    }

    void store32(TrustedImm32 imm, ImplicitAddress address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            if (!imm.m_isPointer && !imm.m_value)
                m_assembler.sw(MIPSRegisters::zero, address.base,
                               address.offset);
            else {
                move(imm, immTempRegister);
                m_assembler.sw(immTempRegister, address.base, address.offset);
            }
        } else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
                m_assembler.sw(MIPSRegisters::zero, addrTempRegister,
                               address.offset);
            else {
                move(imm, immTempRegister);
                m_assembler.sw(immTempRegister, addrTempRegister,
                               address.offset);
            }
        }
    }

    void store32(RegisterID src, void* address)
    {
        



        move(ImmPtr(address), addrTempRegister);
        m_assembler.sw(src, addrTempRegister, 0);
    }

    void store32(TrustedImm32 imm, void* address)
    {
        




        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth) {
            move(ImmPtr(address), addrTempRegister);
            m_assembler.sw(MIPSRegisters::zero, addrTempRegister, 0);
        } else {
            move(imm, immTempRegister);
            move(ImmPtr(address), addrTempRegister);
            m_assembler.sw(immTempRegister, addrTempRegister, 0);
        }
    }

    

    bool supportsFloatingPoint() const
    {
#if WTF_MIPS_DOUBLE_FLOAT
        return true;
#else
        return false;
#endif
    }

    bool supportsFloatingPointTruncate() const
    {
#if WTF_MIPS_DOUBLE_FLOAT && WTF_MIPS_ISA_AT_LEAST(2)
        return true;
#else
        return false;
#endif
    }

    bool supportsFloatingPointSqrt() const
    {
#if WTF_MIPS_DOUBLE_FLOAT && WTF_MIPS_ISA_AT_LEAST(2)
        return true;
#else
        return false;
#endif
    }

    
    
    
    
    
    
    

    void pop(RegisterID dest)
    {
        m_assembler.lw(dest, MIPSRegisters::sp, 0);
        m_assembler.addiu(MIPSRegisters::sp, MIPSRegisters::sp, 4);
    }

    void push(RegisterID src)
    {
        m_assembler.addiu(MIPSRegisters::sp, MIPSRegisters::sp, -4);
        m_assembler.sw(src, MIPSRegisters::sp, 0);
    }

    void push(Address address)
    {
        load32(address, dataTempRegister);
        push(dataTempRegister);
    }

    void push(TrustedImm32 imm)
    {
        move(imm, immTempRegister);
        push(immTempRegister);
    }

    
    
    

    void move(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
            move(MIPSRegisters::zero, dest);
        else if (imm.m_isPointer || m_fixedWidth) {
            m_assembler.lui(dest, imm.m_value >> 16);
            m_assembler.ori(dest, dest, imm.m_value);
        } else
            m_assembler.li(dest, imm.m_value);
    }

    void move(RegisterID src, RegisterID dest)
    {
        if (src != dest || m_fixedWidth)
            m_assembler.move(dest, src);
    }

    void move(TrustedImmPtr imm, RegisterID dest)
    {
        move(Imm32(imm), dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        move(reg1, immTempRegister);
        move(reg2, reg1);
        move(immTempRegister, reg2);
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        if (src != dest || m_fixedWidth)
            move(src, dest);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        if (src != dest || m_fixedWidth)
            move(src, dest);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    Jump branch8(Condition cond, Address left, TrustedImm32 right)
    {
        
        ASSERT(!(right.m_value & 0xFFFFFF00));
        load8(left, dataTempRegister);
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branch32(Condition cond, RegisterID left, RegisterID right)
    {
        if (cond == Equal || cond == Zero)
            return branchEqual(left, right);
        if (cond == NotEqual || cond == NonZero)
            return branchNotEqual(left, right);
        if (cond == Above) {
            m_assembler.sltu(cmpTempRegister, right, left);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == AboveOrEqual) {
            m_assembler.sltu(cmpTempRegister, left, right);
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Below) {
            m_assembler.sltu(cmpTempRegister, left, right);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == BelowOrEqual) {
            m_assembler.sltu(cmpTempRegister, right, left);
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == GreaterThan) {
            m_assembler.slt(cmpTempRegister, right, left);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == GreaterThanOrEqual) {
            m_assembler.slt(cmpTempRegister, left, right);
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == LessThan) {
            m_assembler.slt(cmpTempRegister, left, right);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == LessThanOrEqual) {
            m_assembler.slt(cmpTempRegister, right, left);
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Overflow) {
            















            m_assembler.xorInsn(cmpTempRegister, left, right);
            m_assembler.bgez(cmpTempRegister, 11);
            m_assembler.nop();
            m_assembler.subu(cmpTempRegister, left, right);
            m_assembler.xorInsn(cmpTempRegister, cmpTempRegister, left);
            m_assembler.bgez(cmpTempRegister, 7);
            m_assembler.nop();
            return jump();
        }
        if (cond == Signed) {
            m_assembler.subu(cmpTempRegister, left, right);
            
            m_assembler.slt(cmpTempRegister, cmpTempRegister,
                            MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        ASSERT(0);

        return Jump();
    }

    Jump branch32(Condition cond, RegisterID left, TrustedImm32 right)
    {
        move(right, immTempRegister);
        return branch32(cond, left, immTempRegister);
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

    Jump branch32(Condition cond, Address left, TrustedImm32 right)
    {
        load32(left, dataTempRegister);
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branch32(Condition cond, BaseIndex left, TrustedImm32 right)
    {
        load32(left, dataTempRegister);
        
        
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branch32WithUnalignedHalfWords(Condition cond, BaseIndex left, TrustedImm32 right)
    {
        load32WithUnalignedHalfWords(left, dataTempRegister);
        
        
        
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branch32(Condition cond, AbsoluteAddress left, RegisterID right)
    {
        load32(left.m_ptr, dataTempRegister);
        return branch32(cond, dataTempRegister, right);
    }

    Jump branch32(Condition cond, AbsoluteAddress left, TrustedImm32 right)
    {
        load32(left.m_ptr, dataTempRegister);
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branch16(Condition cond, BaseIndex left, RegisterID right)
    {
        load16(left, dataTempRegister);
        return branch32(cond, dataTempRegister, right);
    }

    Jump branch16(Condition cond, BaseIndex left, TrustedImm32 right)
    {
        ASSERT(!(right.m_value & 0xFFFF0000));
        load16(left, dataTempRegister);
        
        
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branchTest32(Condition cond, RegisterID reg, RegisterID mask)
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        m_assembler.andInsn(cmpTempRegister, reg, mask);
        if (cond == Zero)
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
    }

    Jump branchTest32(Condition cond, RegisterID reg, Imm32 mask = Imm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        if (mask.m_value == -1 && !m_fixedWidth) {
            if (cond == Zero)
                return branchEqual(reg, MIPSRegisters::zero);
            return branchNotEqual(reg, MIPSRegisters::zero);
        }
        move(mask, immTempRegister);
        return branchTest32(cond, reg, immTempRegister);
    }

    Jump branchTest32(Condition cond, Address address, Imm32 mask = Imm32(-1))
    {
        load32(address, dataTempRegister);
        return branchTest32(cond, dataTempRegister, mask);
    }

    Jump branchTest32(Condition cond, BaseIndex address, Imm32 mask = Imm32(-1))
    {
        load32(address, dataTempRegister);
        return branchTest32(cond, dataTempRegister, mask);
    }

    Jump branchTest8(Condition cond, Address address, Imm32 mask = Imm32(-1))
    {
        load8(address, dataTempRegister);
        return branchTest32(cond, dataTempRegister, mask);
    }

    Jump jump()
    {
        return branchEqual(MIPSRegisters::zero, MIPSRegisters::zero);
    }

    void jump(RegisterID target)
    {
        m_assembler.jr(target);
        m_assembler.nop();
    }

    void jump(Address address)
    {
        m_fixedWidth = true;
        load32(address, MIPSRegisters::t9);
        m_assembler.jr(MIPSRegisters::t9);
        m_assembler.nop();
        m_fixedWidth = false;
    }

    void jump(BaseIndex address)
    {
        load32(address, MIPSRegisters::t9);
        m_assembler.jr(MIPSRegisters::t9);
        m_assembler.nop();
    }

    
    
    
    
    
    
    
    
    

    Jump branchAdd32(Condition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            















            move(dest, dataTempRegister);
            m_assembler.xorInsn(cmpTempRegister, dataTempRegister, src);
            m_assembler.bltz(cmpTempRegister, 10);
            m_assembler.addu(dest, dataTempRegister, src);
            m_assembler.xorInsn(cmpTempRegister, dest, dataTempRegister);
            m_assembler.bgez(cmpTempRegister, 7);
            m_assembler.nop();
            return jump();
        }
        if (cond == Signed) {
            add32(src, dest);
            
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero) {
            add32(src, dest);
            return branchEqual(dest, MIPSRegisters::zero);
        }
        if (cond == NonZero) {
            add32(src, dest);
            return branchNotEqual(dest, MIPSRegisters::zero);
        }
        ASSERT(0);
        return Jump();
    }

    Jump branchAdd32(Condition cond, Imm32 imm, RegisterID dest)
    {
        move(imm, immTempRegister);
        return branchAdd32(cond, immTempRegister, dest);
    }

    Jump branchAdd32(Condition cond, Address src, RegisterID dest)
    {
        load32(src, immTempRegister);
        return branchAdd32(cond, immTempRegister, dest);
    }

    Jump branchMul32(Condition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            














            m_assembler.mult(src, dest);
            m_assembler.mfhi(dataTempRegister);
            m_assembler.mflo(dest);
            m_assembler.sra(addrTempRegister, dest, 31);
            m_assembler.beq(dataTempRegister, addrTempRegister, 7);
            m_assembler.nop();
            return jump();
        }
        if (cond == Signed) {
            mul32(src, dest);
            
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero) {
            mul32(src, dest);
            return branchEqual(dest, MIPSRegisters::zero);
        }
        if (cond == NonZero) {
            mul32(src, dest);
            return branchNotEqual(dest, MIPSRegisters::zero);
        }
        ASSERT(0);
        return Jump();
    }

    Jump branchMul32(Condition cond, Imm32 imm, RegisterID src, RegisterID dest)
    {
        move(imm, immTempRegister);
        move(src, dest);
        return branchMul32(cond, immTempRegister, dest);
    }

    Jump branchSub32(Condition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            















            move(dest, dataTempRegister);
            m_assembler.xorInsn(cmpTempRegister, dataTempRegister, src);
            m_assembler.bgez(cmpTempRegister, 10);
            m_assembler.subu(dest, dataTempRegister, src);
            m_assembler.xorInsn(cmpTempRegister, dest, dataTempRegister);
            m_assembler.bgez(cmpTempRegister, 7);
            m_assembler.nop();
            return jump();
        }
        if (cond == Signed) {
            sub32(src, dest);
            
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero) {
            sub32(src, dest);
            return branchEqual(dest, MIPSRegisters::zero);
        }
        if (cond == NonZero) {
            sub32(src, dest);
            return branchNotEqual(dest, MIPSRegisters::zero);
        }
        ASSERT(0);
        return Jump();
    }

    Jump branchSub32(Condition cond, Imm32 imm, RegisterID dest)
    {
        move(imm, immTempRegister);
        return branchSub32(cond, immTempRegister, dest);
    }

    Jump branchSub32(Condition cond, Address src, RegisterID dest)
    {
        load32(src, immTempRegister);
        return branchSub32(cond, immTempRegister, dest);
    }

    Jump branchSub32(Condition cond, Imm32 imm, Address dest)
    {
	
        load32(dest, dataTemp2Register);

        
        sub32(imm, dest);

        return branchSub32(cond, imm, dataTemp2Register);
    }

    Jump branchOr32(Condition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Signed) {
            or32(src, dest);
            
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero) {
            or32(src, dest);
            return branchEqual(dest, MIPSRegisters::zero);
        }
        if (cond == NonZero) {
            or32(src, dest);
            return branchNotEqual(dest, MIPSRegisters::zero);
        }
        ASSERT(0);
        return Jump();
    }

    

    void breakpoint()
    {
        m_assembler.bkpt();
    }

    Call nearCall()
    {
        
        m_assembler.nop();
        m_assembler.nop();
        m_assembler.jal();
        m_assembler.nop();
        return Call(m_assembler.newJmpSrc(), Call::LinkableNear);
    }

    Call call()
    {
        m_assembler.lui(MIPSRegisters::t9, 0);
        m_assembler.ori(MIPSRegisters::t9, MIPSRegisters::t9, 0);
        m_assembler.jalr(MIPSRegisters::t9);
        m_assembler.nop();
        return Call(m_assembler.newJmpSrc(), Call::Linkable);
    }

    Call call(RegisterID target)
    {
        m_assembler.jalr(target);
        m_assembler.nop();
        return Call(m_assembler.newJmpSrc(), Call::None);
    }

    Call call(Address address)
    {
        m_fixedWidth = true;
        load32(address, MIPSRegisters::t9);
        m_assembler.jalr(MIPSRegisters::t9);
        m_assembler.nop();
        m_fixedWidth = false;
        return Call(m_assembler.newJmpSrc(), Call::None);
    }

    void ret()
    {
        m_assembler.jr(MIPSRegisters::ra);
        m_assembler.nop();
    }

    void set8(Condition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        set32(cond, left, right, dest);
    }

    void set8(Condition cond, RegisterID left, Imm32 right, RegisterID dest)
    {
        move(right, immTempRegister);
        set32(cond, left, immTempRegister, dest);
    }

    void set32(Condition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        if (cond == Equal || cond == Zero) {
            m_assembler.xorInsn(dest, left, right);
            m_assembler.sltiu(dest, dest, 1);
        } else if (cond == NotEqual || cond == NonZero) {
            m_assembler.xorInsn(dest, left, right);
            m_assembler.sltu(dest, MIPSRegisters::zero, dest);
        } else if (cond == Above)
            m_assembler.sltu(dest, right, left);
        else if (cond == AboveOrEqual) {
            m_assembler.sltu(dest, left, right);
            m_assembler.xori(dest, dest, 1);
        } else if (cond == Below)
            m_assembler.sltu(dest, left, right);
        else if (cond == BelowOrEqual) {
            m_assembler.sltu(dest, right, left);
            m_assembler.xori(dest, dest, 1);
        } else if (cond == GreaterThan)
            m_assembler.slt(dest, right, left);
        else if (cond == GreaterThanOrEqual) {
            m_assembler.slt(dest, left, right);
            m_assembler.xori(dest, dest, 1);
        } else if (cond == LessThan)
            m_assembler.slt(dest, left, right);
        else if (cond == LessThanOrEqual) {
            m_assembler.slt(dest, right, left);
            m_assembler.xori(dest, dest, 1);
        } else if (cond == Overflow) {
            








            m_assembler.xorInsn(cmpTempRegister, left, right);
            m_assembler.bgez(cmpTempRegister, 4);
            m_assembler.move(dest, MIPSRegisters::zero);
            m_assembler.subu(cmpTempRegister, left, right);
            m_assembler.xorInsn(cmpTempRegister, cmpTempRegister, left);
            m_assembler.slt(dest, cmpTempRegister, MIPSRegisters::zero);
        } else if (cond == Signed) {
            m_assembler.subu(dest, left, right);
            
            m_assembler.slt(dest, dest, MIPSRegisters::zero);
        }
    }

    void set32(Condition cond, Address left, Imm32 right, RegisterID dest)
    {
        load32(left, dataTempRegister);
        move(right, immTempRegister);
        set32(cond, dataTempRegister, immTempRegister, dest);
    }

    void set32(Condition cond, RegisterID left, Imm32 right, RegisterID dest)
    {
        move(right, immTempRegister);
        set32(cond, left, immTempRegister, dest);
    }

    void set32(Condition cond, Address left, RegisterID right, RegisterID dest)
    {
        load32(left, immTempRegister);
        set32(cond, immTempRegister, right, dest);
    }

    void set32(Condition cond, RegisterID left, Address right, RegisterID dest)
    {
        load32(right, immTempRegister);
        set32(cond, left, immTempRegister, dest);
    }

    void setTest8(Condition cond, Address address, Imm32 mask, RegisterID dest)
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        load8(address, dataTempRegister);
        if (mask.m_value == -1 && !m_fixedWidth) {
            if (cond == Zero)
                m_assembler.sltiu(dest, dataTempRegister, 1);
            else
                m_assembler.sltu(dest, MIPSRegisters::zero, dataTempRegister);
        } else {
            move(mask, immTempRegister);
            m_assembler.andInsn(cmpTempRegister, dataTempRegister,
                                immTempRegister);
            if (cond == Zero)
                m_assembler.sltiu(dest, cmpTempRegister, 1);
            else
                m_assembler.sltu(dest, MIPSRegisters::zero, cmpTempRegister);
        }
    }

    void setTest32(Condition cond, Address address, Imm32 mask, RegisterID dest)
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        load32(address, dataTempRegister);
        if (mask.m_value == -1 && !m_fixedWidth) {
            if (cond == Zero)
                m_assembler.sltiu(dest, dataTempRegister, 1);
            else
                m_assembler.sltu(dest, MIPSRegisters::zero, dataTempRegister);
        } else {
            move(mask, immTempRegister);
            m_assembler.andInsn(cmpTempRegister, dataTempRegister,
                                immTempRegister);
            if (cond == Zero)
                m_assembler.sltiu(dest, cmpTempRegister, 1);
            else
                m_assembler.sltu(dest, MIPSRegisters::zero, cmpTempRegister);
        }
    }

    DataLabel32 moveWithPatch(TrustedImm32 imm, RegisterID dest)
    {
        m_fixedWidth = true;
        DataLabel32 label(this);
        move(imm, dest);
        m_fixedWidth = false;
        return label;
    }

    DataLabelPtr moveWithPatch(TrustedImmPtr initialValue, RegisterID dest)
    {
        m_fixedWidth = true;
        DataLabelPtr label(this);
        move(initialValue, dest);
        m_fixedWidth = false;
        return label;
    }

    Jump branch32WithPatch(Condition cond, RegisterID left, TrustedImm32 right, DataLabel32& dataLabel)
    {
        m_fixedWidth = true;
        dataLabel = moveWithPatch(right, immTempRegister);
        Jump temp = branch32(cond, left, immTempRegister);
        m_fixedWidth = false;
        return temp;
    }

    Jump branch32FixedLength(Condition cond, RegisterID left, TrustedImm32 right)
    {
        m_fixedWidth = true;
        move(right, immTempRegister);
        Jump temp = branch32(cond, left, immTempRegister);
        m_fixedWidth = false;
        return temp;
    }

    Jump branch32WithPatch(Condition cond, Address left, TrustedImm32 right, DataLabel32& dataLabel)
    {
        m_fixedWidth = true;
        load32(left, dataTempRegister);
        dataLabel = moveWithPatch(right, immTempRegister);
        Jump temp = branch32(cond, dataTempRegister, immTempRegister);
        m_fixedWidth = false;
        return temp;
    }

    Jump branchPtrWithPatch(Condition cond, RegisterID left, DataLabelPtr& dataLabel, ImmPtr initialRightValue = ImmPtr(0))
    {
        m_fixedWidth = true;
        dataLabel = moveWithPatch(initialRightValue, immTempRegister);
        Jump temp = branch32(cond, left, immTempRegister);
        m_fixedWidth = false;
        return temp;
    }

    Jump branchPtrWithPatch(Condition cond, Address left, DataLabelPtr& dataLabel, ImmPtr initialRightValue = ImmPtr(0))
    {
        m_fixedWidth = true;
        load32(left, dataTempRegister);
        dataLabel = moveWithPatch(initialRightValue, immTempRegister);
        Jump temp = branch32(cond, dataTempRegister, immTempRegister);
        m_fixedWidth = false;
        return temp;
    }

    DataLabelPtr storePtrWithPatch(TrustedImmPtr initialValue, ImplicitAddress address)
    {
        m_fixedWidth = true;
        DataLabelPtr dataLabel = moveWithPatch(initialValue, dataTempRegister);
        store32(dataTempRegister, address);
        m_fixedWidth = false;
        return dataLabel;
    }

    DataLabelPtr storePtrWithPatch(ImplicitAddress address)
    {
        return storePtrWithPatch(ImmPtr(0), address);
    }

    Call tailRecursiveCall()
    {
        
        m_fixedWidth = true;
        move(Imm32(0), MIPSRegisters::t9);
        m_assembler.jr(MIPSRegisters::t9);
        m_assembler.nop();
        m_fixedWidth = false;
        return Call(m_assembler.newJmpSrc(), Call::Linkable);
    }

    Call makeTailRecursiveCall(Jump oldJump)
    {
        oldJump.link(this);
        return tailRecursiveCall();
    }

    void moveDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.movd(dest, src);
    }

    void loadFloat(ImplicitAddress address, FPRegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            m_assembler.lwc1(dest, address.base, address.offset);
            m_assembler.cvtds(dest, dest);
        } else {
            





            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lwc1(dest, addrTempRegister, address.offset);
            m_assembler.cvtds(dest, dest);
        }
    }

    void loadFloat(BaseIndex address, FPRegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            





            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lwc1(dest, addrTempRegister, address.offset);
            m_assembler.cvtds(dest, dest);
        } else {
            







            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.lwc1(dest, addrTempRegister, address.offset);
            m_assembler.cvtds(dest, dest);
        }
    }

    DataLabelPtr loadFloat(const void* address, FPRegisterID dest)
    {
        DataLabelPtr label = moveWithPatch(ImmPtr(address), addrTempRegister);
        



        m_assembler.lwc1(dest, addrTempRegister, 0);
        m_assembler.cvtds(dest, dest);
        return label;
    }

    void loadDouble(ImplicitAddress address, FPRegisterID dest)
    {
#if WTF_MIPS_ISA(1)
        





        move(Imm32(address.offset), addrTempRegister);
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.lwc1(dest, addrTempRegister, 0);
        m_assembler.lwc1(FPRegisterID(dest + 1), addrTempRegister, 4);
#else
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            m_assembler.ldc1(dest, address.base, address.offset);
        } else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.ldc1(dest, addrTempRegister, address.offset);
        }
#endif
    }

    void loadDouble(BaseIndex address, FPRegisterID dest)
    {
#if WTF_MIPS_ISA(1)
        if (address.offset >= -32768 && address.offset <= 32763
            && !m_fixedWidth) {
            





            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lwc1(dest, addrTempRegister, address.offset);
            m_assembler.lwc1(FPRegisterID(dest + 1), addrTempRegister, address.offset + 4);
        } else {
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                li      immTemp, address.offset
                addu    addrTemp, addrTemp, immTemp
                lwc1    dest, 0(addrTemp)
                lwc1    dest+1, 4(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.li(immTempRegister, address.offset);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.lwc1(dest, addrTempRegister, 0);
            m_assembler.lwc1(FPRegisterID(dest + 1), addrTempRegister, 4);
#else
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            




            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.ldc1(dest, addrTempRegister, address.offset);
        } else {
            






            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.ldc1(dest, addrTempRegister, address.offset);
        }
#endif
    }

    DataLabelPtr loadDouble(const void* address, FPRegisterID dest)
    {
        DataLabelPtr label = moveWithPatch(ImmPtr(address), addrTempRegister);
#if WTF_MIPS_ISA(1)
        



        m_assembler.lwc1(dest, addrTempRegister, 0);
        m_assembler.lwc1(FPRegisterID(dest + 1), addrTempRegister, 4);
#else
        


        m_assembler.ldc1(dest, addrTempRegister, 0);
#endif
        return label;
    }

    void storeFloat(FPRegisterID src, BaseIndex address)
    {
        lea(address, addrTempRegister);
        m_assembler.swc1(src, addrTempRegister, 0);
    }

    void storeFloat(FPRegisterID src, ImplicitAddress address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.swc1(src, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.swc1(src, addrTempRegister, address.offset);
        }
    }

    void storeFloat(ImmDouble imm, Address address)
    {
        union {
            float f;
            uint32_t u32;
        } u;
        u.f = imm.u.d;
        store32(Imm32(u.u32), address);
    }

    void storeFloat(ImmDouble imm, BaseIndex address)
    {
        union {
            float f;
            uint32_t u32;
        } u;
        u.f = imm.u.d;
        store32(Imm32(u.u32), address);
    }

    void storeDouble(FPRegisterID src, BaseIndex address)
    {
        lea(address, addrTempRegister);
#if WTF_MIPS_ISA(1)
        m_assembler.swc1(src, addrTempRegister, 0);
        m_assembler.swc1(FPRegisterID(src + 1), addrTempRegister, 4);
#else
        m_assembler.sdc1(src, addrTempRegister, 0);
#endif
    }

    void storeDouble(FPRegisterID src, ImplicitAddress address)
    {
#if WTF_MIPS_ISA(1)
        





        move(Imm32(address.offset), addrTempRegister);
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.swc1(src, addrTempRegister, 0);
        m_assembler.swc1(FPRegisterID(src + 1), addrTempRegister, 4);
#else
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.sdc1(src, address.base, address.offset);
        else {
            




            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sdc1(src, addrTempRegister, address.offset);
        }
#endif
    }

    void storeDouble(ImmDouble imm, Address address)
    {
#if CPU(BIG_ENDIAN)
        store32(Imm32(imm.u.s.msb), address);
        store32(Imm32(imm.u.s.lsb), Address(address.base, address.offset + 4));
#else
        store32(Imm32(imm.u.s.lsb), address);
        store32(Imm32(imm.u.s.msb), Address(address.base, address.offset + 4));
#endif
    }

    void storeDouble(ImmDouble imm, BaseIndex address)
    {
#if CPU(BIG_ENDIAN)
        store32(Imm32(imm.u.s.msb), address);
        store32(Imm32(imm.u.s.lsb),
                BaseIndex(address.base, address.index, address.scale, address.offset + 4));
#else
        store32(Imm32(imm.u.s.lsb), address);
        store32(Imm32(imm.u.s.msb),
                BaseIndex(address.base, address.index, address.scale, address.offset + 4));
#endif
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.addd(dest, dest, src);
    }

    void addDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        m_assembler.addd(dest, dest, fpTempRegister);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.subd(dest, dest, src);
    }

    void subDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        m_assembler.subd(dest, dest, fpTempRegister);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.muld(dest, dest, src);
    }

    void mulDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        m_assembler.muld(dest, dest, fpTempRegister);
    }

    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.divd(dest, dest, src);
    }

    void convertUInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        












        m_assembler.mtc1(src, fpTempRegister);
        m_assembler.bgez(src, 5);
        m_assembler.cvtdw(dest, fpTempRegister);

        m_assembler.mtc1(MIPSRegisters::zero, fpTempRegister);
        m_assembler.lui(dataTempRegister, 0x41F0);
#if WTF_MIPS_ISA_REV(2) && WTF_MIPS_FP64
        m_assembler.mthc1(dataTempRegister, fpTempRegister);
#else
        m_assembler.mtc1(dataTempRegister, FPRegisterID(fpTempRegister + 1));
#endif
        m_assembler.addd(dest, dest, fpTempRegister);
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.mtc1(src, fpTempRegister);
        m_assembler.cvtdw(dest, fpTempRegister);
    }

    void convertInt32ToDouble(Address src, FPRegisterID dest)
    {
        load32(src, dataTempRegister);
        m_assembler.mtc1(dataTempRegister, fpTempRegister);
        m_assembler.cvtdw(dest, fpTempRegister);
    }

    void convertInt32ToDouble(AbsoluteAddress src, FPRegisterID dest)
    {
        load32(src.m_ptr, dataTempRegister);
        m_assembler.mtc1(dataTempRegister, fpTempRegister);
        m_assembler.cvtdw(dest, fpTempRegister);
    }

    void insertRelaxationWords()
    {
        
        m_assembler.beq(MIPSRegisters::zero, MIPSRegisters::zero, 3); 
        m_assembler.nop();
        m_assembler.nop();
        m_assembler.nop();
    }

    Jump branchTrue()
    {
        m_assembler.appendJump();
        m_assembler.bc1t();
        m_assembler.nop();
        insertRelaxationWords();
        return Jump(m_assembler.newJmpSrc());
    }

    Jump branchFalse()
    {
        m_assembler.appendJump();
        m_assembler.bc1f();
        m_assembler.nop();
        insertRelaxationWords();
        return Jump(m_assembler.newJmpSrc());
    }

    Jump branchEqual(RegisterID rs, RegisterID rt)
    {
        m_assembler.appendJump();
        m_assembler.beq(rs, rt, 0);
        m_assembler.nop();
        insertRelaxationWords();
        return Jump(m_assembler.newJmpSrc());
    }

    Jump branchNotEqual(RegisterID rs, RegisterID rt)
    {
        m_assembler.appendJump();
        m_assembler.bne(rs, rt, 0);
        m_assembler.nop();
        insertRelaxationWords();
        return Jump(m_assembler.newJmpSrc());
    }

    void fastStoreDouble(FPRegisterID src, RegisterID lo, RegisterID hi)
    {
#if WTF_MIPS_ISA_REV(2) && WTF_MIPS_FP64
        m_assembler.mfc1(lo, src);
        m_assembler.mfhc1(hi, src);
#else
        m_assembler.mfc1(lo, src);
        m_assembler.mfc1(hi, FPRegisterID(src + 1));
#endif
    }

    void fastLoadDouble(RegisterID lo, RegisterID hi, FPRegisterID fpReg)
    {
#if WTF_MIPS_ISA_REV(2) && WTF_MIPS_FP64
        m_assembler.mtc1(lo, fpReg);
        m_assembler.mthc1(hi, fpReg);
#else
        m_assembler.mtc1(lo, fpReg);
        m_assembler.mtc1(hi, FPRegisterID(fpReg + 1));
#endif
    }

    void convertDoubleToFloat(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.cvtsd(dest, src);
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        if (cond == DoubleEqual) {
            m_assembler.ceqd(left, right);
            return branchTrue();
        }
        if (cond == DoubleNotEqual) {
            m_assembler.cueqd(left, right);
            return branchFalse(); 
        }
        if (cond == DoubleGreaterThan) {
            m_assembler.cngtd(left, right);
            return branchFalse(); 
        }
        if (cond == DoubleGreaterThanOrEqual) {
            m_assembler.cnged(left, right);
            return branchFalse(); 
        }
        if (cond == DoubleLessThan) {
            m_assembler.cltd(left, right);
            return branchTrue();
        }
        if (cond == DoubleLessThanOrEqual) {
            m_assembler.cled(left, right);
            return branchTrue();
        }
        if (cond == DoubleEqualOrUnordered) {
            m_assembler.cueqd(left, right);
            return branchTrue();
        }
        if (cond == DoubleNotEqualOrUnordered) {
            m_assembler.ceqd(left, right);
            return branchFalse(); 
        }
        if (cond == DoubleGreaterThanOrUnordered) {
            m_assembler.coled(left, right);
            return branchFalse(); 
        }
        if (cond == DoubleGreaterThanOrEqualOrUnordered) {
            m_assembler.coltd(left, right);
            return branchFalse(); 
        }
        if (cond == DoubleLessThanOrUnordered) {
            m_assembler.cultd(left, right);
            return branchTrue();
        }
        if (cond == DoubleLessThanOrEqualOrUnordered) {
            m_assembler.culed(left, right);
            return branchTrue();
        }
        ASSERT(0);

        return Jump();
    }

    
    
    
    
    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.truncwd(fpTempRegister, src);
        m_assembler.mfc1(dest, fpTempRegister);
        return branch32(Equal, dest, Imm32(0x7fffffff));
    }

    
    
    
    
    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID fpTemp)
    {
        m_assembler.cvtwd(fpTempRegister, src);
        m_assembler.mfc1(dest, fpTempRegister);

        
        failureCases.append(branch32(Equal, dest, MIPSRegisters::zero));

        
        convertInt32ToDouble(dest, fpTemp);
        failureCases.append(branchDouble(DoubleNotEqualOrUnordered, fpTemp, src));
    }

    void zeroDouble(FPRegisterID dest)
    {
#if WTF_MIPS_ISA_REV(2) && WTF_MIPS_FP64
        m_assembler.mtc1(MIPSRegisters::zero, dest);
        m_assembler.mthc1(MIPSRegisters::zero, dest);
#else
        m_assembler.mtc1(MIPSRegisters::zero, dest);
        m_assembler.mtc1(MIPSRegisters::zero, FPRegisterID(dest + 1));
#endif
    }

private:
    
    
    bool m_fixedWidth;

    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkCall(void* code, Call call, FunctionPtr function)
    {
        MIPSAssembler::linkCall(code, call.m_jmp, function.value());
    }

    static void repatchCall(CodeLocationCall call, CodeLocationLabel destination)
    {
        MIPSAssembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

    static void repatchCall(CodeLocationCall call, FunctionPtr destination)
    {
        MIPSAssembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

};

}

#endif 

#endif
