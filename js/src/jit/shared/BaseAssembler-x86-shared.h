




























#ifndef jit_shared_BaseAssembler_x86_shared_h
#define jit_shared_BaseAssembler_x86_shared_h

#include "mozilla/IntegerPrintfMacros.h"

#include <stdarg.h>

#include "jit/shared/AssemblerBuffer-x86-shared.h"

#include "js/Vector.h"

namespace js {
namespace jit {

inline bool CAN_SIGN_EXTEND_8_32(int32_t value) { return value == (int32_t)(int8_t)value; }
inline bool CAN_SIGN_EXTEND_16_32(int32_t value) { return value == (int32_t)(int16_t)value; }
inline bool CAN_ZERO_EXTEND_8_32(int32_t value) { return value == (int32_t)(uint8_t)value; }
inline bool CAN_ZERO_EXTEND_8H_32(int32_t value) { return value == (value & 0xff00); }
inline bool CAN_ZERO_EXTEND_16_32(int32_t value) { return value == (int32_t)(uint16_t)value; }
inline bool CAN_ZERO_EXTEND_32_64(int32_t value) { return value >= 0; }

namespace X86Registers {
    enum RegisterID {
        eax,
        ecx,
        edx,
        ebx,
        esp,
        ebp,
        esi,
        edi

#ifdef JS_CODEGEN_X64
       ,r8,
        r9,
        r10,
        r11,
        r12,
        r13,
        r14,
        r15
#endif
        ,invalid_reg
    };

    enum XMMRegisterID {
        xmm0,
        xmm1,
        xmm2,
        xmm3,
        xmm4,
        xmm5,
        xmm6,
        xmm7
#ifdef JS_CODEGEN_X64
       ,xmm8,
        xmm9,
        xmm10,
        xmm11,
        xmm12,
        xmm13,
        xmm14,
        xmm15
#endif
       ,invalid_xmm
    };

    static const char* nameFPReg(XMMRegisterID fpreg)
    {
        static const char* const xmmnames[16]
          = { "%xmm0", "%xmm1", "%xmm2", "%xmm3",
              "%xmm4", "%xmm5", "%xmm6", "%xmm7",
              "%xmm8", "%xmm9", "%xmm10", "%xmm11",
              "%xmm12", "%xmm13", "%xmm14", "%xmm15" };
        int off = (XMMRegisterID)fpreg - (XMMRegisterID)xmm0;
        MOZ_ASSERT(off >= 0 && size_t(off) < mozilla::ArrayLength(xmmnames));
        return xmmnames[off];
    }

    static const char* nameIReg(int szB, RegisterID reg)
    {
        static const char* const r64names[16]
          = { "%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi", "%rdi",
              "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15" };
        static const char* const r32names[16]
          = { "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi",
              "%r8d", "%r9d", "%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d" };
        static const char* const r16names[16]
          = { "%ax", "%cx", "%dx", "%bx", "%sp", "%bp", "%si", "%di",
              "%r8w", "%r9w", "%r10w", "%r11w", "%r12w", "%r13w", "%r14w", "%r15w" };
        static const char* const r8names[16]
          = { "%al", "%cl", "%dl", "%bl", "%spl", "%bpl", "%sil", "%dil",
              "%r8b", "%r9b", "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b" };
        int          off = (RegisterID)reg - (RegisterID)eax;
        MOZ_ASSERT(off >= 0 && size_t(off) < mozilla::ArrayLength(r64names));
        const char* const* tab = r64names;
        switch (szB) {
            case 1: tab = r8names; break;
            case 2: tab = r16names; break;
            case 4: tab = r32names; break;
        }
        return tab[off];
    }

    static const char* nameI8Reg_norex(RegisterID reg)
    {
        
        
        static const char* const r8names_norex[16]
          = { "%al", "%cl", "%dl", "%bl", "%ah", "%ch", "%dh", "%bh",
              "%r8b", "%r9b", "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b" };
        int off = (RegisterID)reg - (RegisterID)eax;
        MOZ_ASSERT(off >= 0 && size_t(off) < mozilla::ArrayLength(r8names_norex));
        return r8names_norex[off];
    }

    static const char* nameIReg(RegisterID reg)
    {
#       ifdef JS_CODEGEN_X64
        return nameIReg(8, reg);
#       else
        return nameIReg(4, reg);
#       endif
    }

    inline bool hasSubregL(RegisterID reg)
    {
#       ifdef JS_CODEGEN_X64
        
        return true;
#       else
        
        return reg <= ebx;
#       endif
    }

    inline bool hasSubregH(RegisterID reg)
    {
        
        
        
        
        return reg <= ebx;
    }

    inline RegisterID getSubregH(RegisterID reg) {
        MOZ_ASSERT(hasSubregH(reg));
        return RegisterID(reg + 4);
    }

} 



static inline bool
ByteRegRequiresRex(int reg)
{
    return (reg >= X86Registers::esp);
}

class X86Assembler : public GenericAssembler {
public:
    typedef X86Registers::RegisterID RegisterID;
    typedef X86Registers::XMMRegisterID XMMRegisterID;
    typedef XMMRegisterID FPRegisterID;

    enum Condition {
        ConditionO,
        ConditionNO,
        ConditionB,
        ConditionAE,
        ConditionE,
        ConditionNE,
        ConditionBE,
        ConditionA,
        ConditionS,
        ConditionNS,
        ConditionP,
        ConditionNP,
        ConditionL,
        ConditionGE,
        ConditionLE,
        ConditionG,

        ConditionC  = ConditionB,
        ConditionNC = ConditionAE
    };

    
    enum ConditionCmp {
        ConditionCmp_EQ    = 0x0,
        ConditionCmp_LT    = 0x1,
        ConditionCmp_LE    = 0x2,
        ConditionCmp_UNORD = 0x3,
        ConditionCmp_NEQ   = 0x4,
        ConditionCmp_NLT   = 0x5,
        ConditionCmp_NLE   = 0x6,
        ConditionCmp_ORD   = 0x7,
    };

    static const char* nameCC(Condition cc)
    {
        static const char* const names[16]
          = { "o ", "no", "b ", "ae", "e ", "ne", "be", "a ",
              "s ", "ns", "p ", "np", "l ", "ge", "le", "g " };
        int ix = (int)cc;
        MOZ_ASSERT(ix >= 0 && size_t(ix) < mozilla::ArrayLength(names));
        return names[ix];
    }

    
    enum RoundingMode {
        RoundToNearest = 0x0,
        RoundDown      = 0x1,
        RoundUp        = 0x2,
        RoundToZero    = 0x3
    };

private:
    enum OneByteOpcodeID {
        OP_ADD_EbGb                     = 0x00,
        OP_ADD_EvGv                     = 0x01,
        OP_ADD_GvEv                     = 0x03,
        OP_ADD_EAXIv                    = 0x05,
        OP_OR_EbGb                      = 0x08,
        OP_OR_EvGv                      = 0x09,
        OP_OR_GvEv                      = 0x0B,
        OP_OR_EAXIv                     = 0x0D,
        OP_2BYTE_ESCAPE                 = 0x0F,
        OP_AND_EbGb                     = 0x20,
        OP_AND_EvGv                     = 0x21,
        OP_AND_GvEv                     = 0x23,
        OP_AND_EAXIv                    = 0x25,
        OP_SUB_EbGb                     = 0x28,
        OP_SUB_EvGv                     = 0x29,
        OP_SUB_GvEv                     = 0x2B,
        OP_SUB_EAXIv                    = 0x2D,
        PRE_PREDICT_BRANCH_NOT_TAKEN    = 0x2E,
        OP_XOR_EbGb                     = 0x30,
        OP_XOR_EvGv                     = 0x31,
        OP_XOR_GvEv                     = 0x33,
        OP_XOR_EAXIv                    = 0x35,
        OP_CMP_EvGv                     = 0x39,
        OP_CMP_GvEv                     = 0x3B,
        OP_CMP_EAXIv                    = 0x3D,
#ifdef JS_CODEGEN_X64
        PRE_REX                         = 0x40,
#endif
        OP_PUSH_EAX                     = 0x50,
        OP_POP_EAX                      = 0x58,
#ifdef JS_CODEGEN_X86
        OP_PUSHA                        = 0x60,
        OP_POPA                         = 0x61,
#endif
#ifdef JS_CODEGEN_X64
        OP_MOVSXD_GvEv                  = 0x63,
#endif
        PRE_OPERAND_SIZE                = 0x66,
        PRE_SSE_66                      = 0x66,
        OP_PUSH_Iz                      = 0x68,
        OP_IMUL_GvEvIz                  = 0x69,
        OP_PUSH_Ib                      = 0x6a,
        OP_IMUL_GvEvIb                  = 0x6b,
        OP_JCC_rel8                     = 0x70,
        OP_GROUP1_EbIb                  = 0x80,
        OP_GROUP1_EvIz                  = 0x81,
        OP_GROUP1_EvIb                  = 0x83,
        OP_TEST_EbGb                    = 0x84,
        OP_TEST_EvGv                    = 0x85,
        OP_XCHG_GvEv                    = 0x87,
        OP_MOV_EbGv                     = 0x88,
        OP_MOV_EvGv                     = 0x89,
        OP_MOV_GvEb                     = 0x8A,
        OP_MOV_GvEv                     = 0x8B,
        OP_LEA                          = 0x8D,
        OP_GROUP1A_Ev                   = 0x8F,
        OP_NOP                          = 0x90,
        OP_PUSHFLAGS                    = 0x9C,
        OP_POPFLAGS                     = 0x9D,
        OP_CDQ                          = 0x99,
        OP_MOV_EAXOv                    = 0xA1,
        OP_MOV_OvEAX                    = 0xA3,
        OP_TEST_EAXIb                   = 0xA8,
        OP_TEST_EAXIv                   = 0xA9,
        OP_MOV_EAXIv                    = 0xB8,
        OP_GROUP2_EvIb                  = 0xC1,
        OP_RET_Iz                       = 0xC2,
        PRE_VEX_C4                      = 0xC4,
        PRE_VEX_C5                      = 0xC5,
        OP_RET                          = 0xC3,
        OP_GROUP11_EvIb                 = 0xC6,
        OP_GROUP11_EvIz                 = 0xC7,
        OP_INT3                         = 0xCC,
        OP_GROUP2_Ev1                   = 0xD1,
        OP_GROUP2_EvCL                  = 0xD3,
        OP_FPU6                         = 0xDD,
        OP_FPU6_F32                     = 0xD9,
        OP_CALL_rel32                   = 0xE8,
        OP_JMP_rel32                    = 0xE9,
        OP_JMP_rel8                     = 0xEB,
        PRE_LOCK                        = 0xF0,
        PRE_SSE_F2                      = 0xF2,
        PRE_SSE_F3                      = 0xF3,
        OP_HLT                          = 0xF4,
        OP_GROUP3_EbIb                  = 0xF6,
        OP_GROUP3_Ev                    = 0xF7,
        OP_GROUP3_EvIz                  = 0xF7, 
        OP_GROUP5_Ev                    = 0xFF
    };

    enum ShiftID {
        Shift_vpsrld = 2,
        Shift_vpsrlq = 2,
        Shift_vpsrldq = 3,
        Shift_vpsrad = 4,
        Shift_vpslld = 6,
        Shift_vpsllq = 6
    };

    enum TwoByteOpcodeID {
        OP2_UD2             = 0x0B,
        OP2_MOVSD_VsdWsd    = 0x10,
        OP2_MOVPS_VpsWps    = 0x10,
        OP2_MOVSD_WsdVsd    = 0x11,
        OP2_MOVPS_WpsVps    = 0x11,
        OP2_MOVHLPS_VqUq    = 0x12,
        OP2_MOVSLDUP_VpsWps = 0x12,
        OP2_UNPCKLPS_VsdWsd = 0x14,
        OP2_UNPCKHPS_VsdWsd = 0x15,
        OP2_MOVLHPS_VqUq    = 0x16,
        OP2_MOVSHDUP_VpsWps = 0x16,
        OP2_MOVAPD_VsdWsd   = 0x28,
        OP2_MOVAPS_VsdWsd   = 0x28,
        OP2_MOVAPS_WsdVsd   = 0x29,
        OP2_CVTSI2SD_VsdEd  = 0x2A,
        OP2_CVTTSD2SI_GdWsd = 0x2C,
        OP2_UCOMISD_VsdWsd  = 0x2E,
        OP2_MOVMSKPD_EdVd   = 0x50,
        OP2_ANDPS_VpsWps    = 0x54,
        OP2_ANDNPS_VpsWps   = 0x55,
        OP2_ORPS_VpsWps     = 0x56,
        OP2_XORPS_VpsWps    = 0x57,
        OP2_ADDSD_VsdWsd    = 0x58,
        OP2_ADDPS_VpsWps    = 0x58,
        OP2_MULSD_VsdWsd    = 0x59,
        OP2_MULPS_VpsWps    = 0x59,
        OP2_CVTSS2SD_VsdEd  = 0x5A,
        OP2_CVTSD2SS_VsdEd  = 0x5A,
        OP2_CVTTPS2DQ_VdqWps = 0x5B,
        OP2_CVTDQ2PS_VpsWdq = 0x5B,
        OP2_SUBSD_VsdWsd    = 0x5C,
        OP2_SUBPS_VpsWps    = 0x5C,
        OP2_MINSD_VsdWsd    = 0x5D,
        OP2_MINSS_VssWss    = 0x5D,
        OP2_MINPS_VpsWps    = 0x5D,
        OP2_DIVSD_VsdWsd    = 0x5E,
        OP2_DIVPS_VpsWps    = 0x5E,
        OP2_MAXSD_VsdWsd    = 0x5F,
        OP2_MAXSS_VssWss    = 0x5F,
        OP2_MAXPS_VpsWps    = 0x5F,
        OP2_SQRTSD_VsdWsd   = 0x51,
        OP2_SQRTSS_VssWss   = 0x51,
        OP2_SQRTPS_VpsWps   = 0x51,
        OP2_RSQRTPS_VpsWps  = 0x52,
        OP2_RCPPS_VpsWps    = 0x53,
        OP2_ANDPD_VpdWpd    = 0x54,
        OP2_ORPD_VpdWpd     = 0x56,
        OP2_XORPD_VpdWpd    = 0x57,
        OP2_PCMPGTD_VdqWdq  = 0x66,
        OP2_MOVD_VdEd       = 0x6E,
        OP2_MOVDQ_VsdWsd    = 0x6F,
        OP2_MOVDQ_VdqWdq    = 0x6F,
        OP2_PSHUFD_VdqWdqIb = 0x70,
        OP2_PSLLD_UdqIb     = 0x72,
        OP2_PSRAD_UdqIb     = 0x72,
        OP2_PSRLD_UdqIb     = 0x72,
        OP2_PSRLDQ_Vd       = 0x73,
        OP2_PCMPEQW         = 0x75,
        OP2_PCMPEQD_VdqWdq  = 0x76,
        OP2_MOVD_EdVd       = 0x7E,
        OP2_MOVDQ_WdqVdq    = 0x7F,
        OP2_JCC_rel32       = 0x80,
        OP_SETCC            = 0x90,
        OP_FENCE            = 0xAE,
        OP2_IMUL_GvEv       = 0xAF,
        OP2_CMPXCHG_GvEb    = 0xB0,
        OP2_CMPXCHG_GvEw    = 0xB1,
        OP2_BSR_GvEv        = 0xBD,
        OP2_MOVSX_GvEb      = 0xBE,
        OP2_MOVSX_GvEw      = 0xBF,
        OP2_MOVZX_GvEb      = 0xB6,
        OP2_MOVZX_GvEw      = 0xB7,
        OP2_XADD_EbGb       = 0xC0,
        OP2_XADD_EvGv       = 0xC1,
        OP2_CMPPS_VpsWps    = 0xC2,
        OP2_PEXTRW_GdUdIb   = 0xC5,
        OP2_SHUFPS_VpsWpsIb = 0xC6,
        OP2_PSRLD_VdqWdq    = 0xD2,
        OP2_PANDDQ_VdqWdq   = 0xDB,
        OP2_PANDNDQ_VdqWdq  = 0xDF,
        OP2_PSRAD_VdqWdq    = 0xE2,
        OP2_PORDQ_VdqWdq    = 0xEB,
        OP2_PXORDQ_VdqWdq   = 0xEF,
        OP2_PSLLD_VdqWdq    = 0xF2,
        OP2_PMULUDQ_VdqWdq  = 0xF4,
        OP2_PSUBD_VdqWdq    = 0xFA,
        OP2_PADDD_VdqWdq    = 0xFE
    };

    
    static inline bool IsXMMReversedOperands(TwoByteOpcodeID opcode) {
        switch (opcode) {
          case OP2_MOVSD_WsdVsd: 
          case OP2_MOVAPS_WsdVsd:
          case OP2_MOVDQ_WdqVdq:
          case OP3_PEXTRD_EdVdqIb:
            return true;
          default:
            break;
        }
        return false;
    }

    enum ThreeByteOpcodeID {
        OP3_ROUNDSS_VsdWsd  = 0x0A,
        OP3_ROUNDSD_VsdWsd  = 0x0B,
        OP3_BLENDVPS_VdqWdq = 0x14,
        OP3_PEXTRD_EdVdqIb  = 0x16,
        OP3_BLENDPS_VpsWpsIb = 0x0C,
        OP3_PTEST_VdVd      = 0x17,
        OP3_INSERTPS_VpsUps = 0x21,
        OP3_PINSRD_VdqEdIb  = 0x22,
        OP3_PMULLD_VdqWdq   = 0x40,
        OP3_VBLENDVPS_VdqWdq = 0x4A
    };

    enum ThreeByteEscape {
        ESCAPE_BLENDVPS     = 0x38,
        ESCAPE_PMULLD       = 0x38,
        ESCAPE_PTEST        = 0x38,
        ESCAPE_PINSRD       = 0x3A,
        ESCAPE_PEXTRD       = 0x3A,
        ESCAPE_ROUNDSD      = 0x3A,
        ESCAPE_INSERTPS     = 0x3A,
        ESCAPE_BLENDPS      = 0x3A,
        ESCAPE_VBLENDVPS    = 0x3A
    };

    enum VexOperandType {
        VEX_PS = 0,
        VEX_PD = 1,
        VEX_SS = 2,
        VEX_SD = 3
    };

    OneByteOpcodeID jccRel8(Condition cond)
    {
        return (OneByteOpcodeID)(OP_JCC_rel8 + cond);
    }
    TwoByteOpcodeID jccRel32(Condition cond)
    {
        return (TwoByteOpcodeID)(OP2_JCC_rel32 + cond);
    }

    TwoByteOpcodeID setccOpcode(Condition cond)
    {
        return (TwoByteOpcodeID)(OP_SETCC + cond);
    }

    enum GroupOpcodeID {
        GROUP1_OP_ADD = 0,
        GROUP1_OP_OR  = 1,
        GROUP1_OP_ADC = 2,
        GROUP1_OP_AND = 4,
        GROUP1_OP_SUB = 5,
        GROUP1_OP_XOR = 6,
        GROUP1_OP_CMP = 7,

        GROUP1A_OP_POP = 0,

        GROUP2_OP_SHL = 4,
        GROUP2_OP_SHR = 5,
        GROUP2_OP_SAR = 7,

        GROUP3_OP_TEST = 0,
        GROUP3_OP_NOT  = 2,
        GROUP3_OP_NEG  = 3,
        GROUP3_OP_IMUL = 5,
        GROUP3_OP_DIV  = 6,
        GROUP3_OP_IDIV = 7,

        GROUP5_OP_INC   = 0,
        GROUP5_OP_DEC   = 1,
        GROUP5_OP_CALLN = 2,
        GROUP5_OP_JMPN  = 4,
        GROUP5_OP_PUSH  = 6,

        FPU6_OP_FLD     = 0,
        FPU6_OP_FISTTP  = 1,
        FPU6_OP_FSTP    = 3,

        GROUP11_MOV = 0
    };

    class X86InstructionFormatter;
public:
    X86Assembler()
      : useVEX_(true)
    { }

    void disableVEX() { useVEX_ = false; }

    class JmpSrc {
        friend class X86Assembler;
        friend class X86InstructionFormatter;
    public:
        JmpSrc()
            : m_offset(-1)
        {
        }

        explicit JmpSrc(int32_t offset)
            : m_offset(offset)
        {
        }

        int32_t offset() const {
            return m_offset;
        }

        bool isSet() const {
            return m_offset != -1;
        }

    private:
        int m_offset;
    };

    class JmpDst {
        friend class X86Assembler;
        friend class X86InstructionFormatter;
    public:
        JmpDst()
            : m_offset(-1)
            , m_used(false)
        {
        }

        bool isUsed() const { return m_used; }
        void used() { m_used = true; }
        bool isValid() const { return m_offset != -1; }

        explicit JmpDst(int32_t offset)
            : m_offset(offset)
            , m_used(false)
        {
            MOZ_ASSERT(m_offset == offset);
        }
        int32_t offset() const {
            return m_offset;
        }
    private:
        signed int m_offset : 31;
        bool m_used : 1;
    };

    size_t size() const { return m_formatter.size(); }
    size_t allocSize() const { return m_formatter.allocSize(); }
    unsigned char *buffer() const { return m_formatter.buffer(); }
    bool oom() const { return m_formatter.oom(); }

    void nop()
    {
        spew("nop");
        m_formatter.oneByteOp(OP_NOP);
    }

    void twoByteNop()
    {
        spew("nop (2 byte)");
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp(OP_NOP);
    }

    

    void push_r(RegisterID reg)
    {
        spew("push       %s", nameIReg(reg));
        m_formatter.oneByteOp(OP_PUSH_EAX, reg);
    }

    void pop_r(RegisterID reg)
    {
        spew("pop        %s", nameIReg(reg));
        m_formatter.oneByteOp(OP_POP_EAX, reg);
    }

    void push_i(int32_t imm)
    {
        spew("push       $%s0x%x", PRETTYHEX(imm));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_PUSH_Ib);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp(OP_PUSH_Iz);
            m_formatter.immediate32(imm);
        }
    }

    void push_i32(int32_t imm)
    {
        spew("push       $%s0x%04x", PRETTYHEX(imm));
        m_formatter.oneByteOp(OP_PUSH_Iz);
        m_formatter.immediate32(imm);
    }

    void push_m(int32_t offset, RegisterID base)
    {
        spew("push       " MEM_ob, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_GROUP5_Ev, offset, base, GROUP5_OP_PUSH);
    }

    void pop_m(int32_t offset, RegisterID base)
    {
        spew("pop        " MEM_ob, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_GROUP1A_Ev, offset, base, GROUP1A_OP_POP);
    }

    void push_flags()
    {
        spew("pushf");
        m_formatter.oneByteOp(OP_PUSHFLAGS);
    }

    void pop_flags()
    {
        spew("popf");
        m_formatter.oneByteOp(OP_POPFLAGS);
    }

    

#ifdef JS_CODEGEN_X86
    void adcl_im(int32_t imm, const void* addr)
    {
        FIXME_INSN_PRINTING;
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, addr, GROUP1_OP_ADC);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, addr, GROUP1_OP_ADC);
            m_formatter.immediate32(imm);
        }
    }
#endif

    void addl_rr(RegisterID src, RegisterID dst)
    {
        spew("addl       %s, %s", nameIReg(4,src), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_ADD_GvEv, src, dst);
    }

    void addl_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("addl       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_ADD_GvEv, offset, base, dst);
    }

    void addl_rm(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("addl       %s, " MEM_ob, nameIReg(4,src), ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_ADD_EvGv, offset, base, src);
    }

    void addl_ir(int32_t imm, RegisterID dst)
    {
        spew("addl       $%d, %s", imm, nameIReg(4,dst));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, dst, GROUP1_OP_ADD);
            m_formatter.immediate8s(imm);
        } else {
            if (dst == X86Registers::eax)
                m_formatter.oneByteOp(OP_ADD_EAXIv);
            else
                m_formatter.oneByteOp(OP_GROUP1_EvIz, dst, GROUP1_OP_ADD);
            m_formatter.immediate32(imm);
        }
    }
    void addl_i32r(int32_t imm, RegisterID dst)
    {
        
        spew("addl       $0x%04x, %s", imm, nameIReg(4,dst));
        if (dst == X86Registers::eax)
            m_formatter.oneByteOp(OP_ADD_EAXIv);
        else
            m_formatter.oneByteOp(OP_GROUP1_EvIz, dst, GROUP1_OP_ADD);
        m_formatter.immediate32(imm);
    }

    void addl_im(int32_t imm, int32_t offset, RegisterID base)
    {
        spew("addl       $%d, " MEM_ob, imm, ADDR_ob(offset, base));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, offset, base, GROUP1_OP_ADD);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, offset, base, GROUP1_OP_ADD);
            m_formatter.immediate32(imm);
        }
    }

#ifdef JS_CODEGEN_X64
    void addq_rr(RegisterID src, RegisterID dst)
    {
        spew("addq       %s, %s", nameIReg(8,src), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_ADD_GvEv, src, dst);
    }

    void addq_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("addq       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_ADD_GvEv, offset, base, dst);
    }

    void addq_mr(const void* addr, RegisterID dst)
    {
        spew("addq       %p, %s", addr, nameIReg(8, dst));
        m_formatter.oneByteOp64(OP_ADD_GvEv, addr, dst);
    }

    void addq_ir(int32_t imm, RegisterID dst)
    {
        spew("addq       $%d, %s", imm, nameIReg(8,dst));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, dst, GROUP1_OP_ADD);
            m_formatter.immediate8s(imm);
        } else {
            if (dst == X86Registers::eax)
                m_formatter.oneByteOp64(OP_ADD_EAXIv);
            else
                m_formatter.oneByteOp64(OP_GROUP1_EvIz, dst, GROUP1_OP_ADD);
            m_formatter.immediate32(imm);
        }
    }

    void addq_im(int32_t imm, int32_t offset, RegisterID base)
    {
        spew("addq       $%d, " MEM_ob, imm, ADDR_ob(offset, base));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, offset, base, GROUP1_OP_ADD);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, offset, base, GROUP1_OP_ADD);
            m_formatter.immediate32(imm);
        }
    }

    void addq_im(int32_t imm, const void* addr)
    {
        spew("addq       $%d, %p", imm, addr);
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, addr, GROUP1_OP_ADD);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, addr, GROUP1_OP_ADD);
            m_formatter.immediate32(imm);
        }
    }
#endif
    void addl_im(int32_t imm, const void* addr)
    {
        spew("addl       $%d, %p", imm, addr);
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, addr, GROUP1_OP_ADD);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, addr, GROUP1_OP_ADD);
            m_formatter.immediate32(imm);
        }
    }

    void lock_xaddb_rm(RegisterID srcdest, int32_t offset, RegisterID base)
    {
        spew("lock xaddl %s, " MEM_ob, nameIReg(1, srcdest), ADDR_ob(offset, base));
        m_formatter.oneByteOp(PRE_LOCK);
        m_formatter.twoByteOp(OP2_XADD_EbGb, offset, base, srcdest);
    }

    void lock_xaddb_rm(RegisterID srcdest, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("lock xaddl %s, " MEM_obs, nameIReg(1, srcdest), ADDR_obs(offset, base, index, scale));
        m_formatter.oneByteOp(PRE_LOCK);
        m_formatter.twoByteOp(OP2_XADD_EbGb, offset, base, index, scale, srcdest);
    }

    void lock_xaddl_rm(RegisterID srcdest, int32_t offset, RegisterID base)
    {
        spew("lock xaddl %s, " MEM_ob, nameIReg(4,srcdest), ADDR_ob(offset, base));
        m_formatter.oneByteOp(PRE_LOCK);
        m_formatter.twoByteOp(OP2_XADD_EvGv, offset, base, srcdest);
    }

    void lock_xaddl_rm(RegisterID srcdest, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("lock xaddl %s, " MEM_obs, nameIReg(4, srcdest), ADDR_obs(offset, base, index, scale));
        m_formatter.oneByteOp(PRE_LOCK);
        m_formatter.twoByteOp(OP2_XADD_EvGv, offset, base, index, scale, srcdest);
    }

    void vpaddd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpaddd", VEX_PD, OP2_PADDD_VdqWdq, src1, src0, dst);
    }
    void vpaddd_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpaddd", VEX_PD, OP2_PADDD_VdqWdq, offset, base, src0, dst);
    }
    void vpaddd_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpaddd", VEX_PD, OP2_PADDD_VdqWdq, address, src0, dst);
    }

    void vpsubd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpsubd", VEX_PD, OP2_PSUBD_VdqWdq, src1, src0, dst);
    }
    void vpsubd_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpsubd", VEX_PD, OP2_PSUBD_VdqWdq, offset, base, src0, dst);
    }
    void vpsubd_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpsubd", VEX_PD, OP2_PSUBD_VdqWdq, address, src0, dst);
    }

    void vpmuludq_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpmuludq", VEX_PD, OP2_PMULUDQ_VdqWdq, src1, src0, dst);
    }
    void vpmuludq_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpmuludq", VEX_PD, OP2_PMULUDQ_VdqWdq, offset, base, src0, dst);
    }

    void vpmulld_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        threeByteOpSimd("vpmulld", VEX_PD, OP3_PMULLD_VdqWdq, ESCAPE_PMULLD, src1, src0, dst);
    }
    void vpmulld_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        threeByteOpSimd("vpmulld", VEX_PD, OP3_PMULLD_VdqWdq, ESCAPE_PMULLD, offset, base, src0, dst);
    }
    void vpmulld_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        threeByteOpSimd("vpmulld", VEX_PD, OP3_PMULLD_VdqWdq, ESCAPE_PMULLD, address, src0, dst);
    }

    void vaddps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vaddps", VEX_PS, OP2_ADDPS_VpsWps, src1, src0, dst);
    }
    void vaddps_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vaddps", VEX_PS, OP2_ADDPS_VpsWps, offset, base, src0, dst);
    }
    void vaddps_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vaddps", VEX_PS, OP2_ADDPS_VpsWps, address, src0, dst);
    }

    void vsubps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vsubps", VEX_PS, OP2_SUBPS_VpsWps, src1, src0, dst);
    }
    void vsubps_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vsubps", VEX_PS, OP2_SUBPS_VpsWps, offset, base, src0, dst);
    }
    void vsubps_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vsubps", VEX_PS, OP2_SUBPS_VpsWps, address, src0, dst);
    }

    void vmulps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmulps", VEX_PS, OP2_MULPS_VpsWps, src1, src0, dst);
    }
    void vmulps_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmulps", VEX_PS, OP2_MULPS_VpsWps, offset, base, src0, dst);
    }
    void vmulps_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmulps", VEX_PS, OP2_MULPS_VpsWps, address, src0, dst);
    }

    void vdivps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vdivps", VEX_PS, OP2_DIVPS_VpsWps, src1, src0, dst);
    }
    void vdivps_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vdivps", VEX_PS, OP2_DIVPS_VpsWps, offset, base, src0, dst);
    }
    void vdivps_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vdivps", VEX_PS, OP2_DIVPS_VpsWps, address, src0, dst);
    }

    void vmaxps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmaxps", VEX_PS, OP2_MAXPS_VpsWps, src1, src0, dst);
    }
    void vmaxps_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmaxps", VEX_PS, OP2_MAXPS_VpsWps, offset, base, src0, dst);
    }
    void vmaxps_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmaxps", VEX_PS, OP2_MAXPS_VpsWps, address, src0, dst);
    }

    void vminps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vminps", VEX_PS, OP2_MINPS_VpsWps, src1, src0, dst);
    }
    void vminps_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vminps", VEX_PS, OP2_MINPS_VpsWps, offset, base, src0, dst);
    }
    void vminps_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vminps", VEX_PS, OP2_MINPS_VpsWps, address, src0, dst);
    }

    void andl_rr(RegisterID src, RegisterID dst)
    {
        spew("andl       %s, %s", nameIReg(4,src), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_AND_GvEv, src, dst);
    }

    void andl_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("andl       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_AND_GvEv, offset, base, dst);
    }

    void andl_rm(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("andl       %s, " MEM_ob, nameIReg(4,src), ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_AND_EvGv, offset, base, src);
    }

    void andl_ir(int32_t imm, RegisterID dst)
    {
        spew("andl       $0x%x, %s", imm, nameIReg(4,dst));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, dst, GROUP1_OP_AND);
            m_formatter.immediate8s(imm);
        } else {
            if (dst == X86Registers::eax)
                m_formatter.oneByteOp(OP_AND_EAXIv);
            else
                m_formatter.oneByteOp(OP_GROUP1_EvIz, dst, GROUP1_OP_AND);
            m_formatter.immediate32(imm);
        }
    }

    void andl_im(int32_t imm, int32_t offset, RegisterID base)
    {
        spew("andl       $0x%x, " MEM_ob, imm, ADDR_ob(offset, base));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, offset, base, GROUP1_OP_AND);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, offset, base, GROUP1_OP_AND);
            m_formatter.immediate32(imm);
        }
    }

#ifdef JS_CODEGEN_X64
    void andq_rr(RegisterID src, RegisterID dst)
    {
        spew("andq       %s, %s", nameIReg(8,src), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_AND_GvEv, src, dst);
    }

    void andq_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("andq       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_AND_GvEv, offset, base, dst);
    }

    void andq_mr(int32_t offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        spew("andq       " MEM_obs ", %s", ADDR_obs(offset, base, index, scale), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_AND_GvEv, offset, base, index, scale, dst);
    }

    void andq_mr(const void *addr, RegisterID dst)
    {
        spew("andq       %p, %s", addr, nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_AND_GvEv, addr, dst);
    }

    void orq_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("orq        " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_OR_GvEv, offset, base, dst);
    }

    void orq_mr(const void* addr, RegisterID dst)
    {
        spew("orq        %p, %s", addr, nameIReg(8, dst));
        m_formatter.oneByteOp64(OP_OR_GvEv, addr, dst);
    }

    void andq_ir(int32_t imm, RegisterID dst)
    {
        spew("andq       $0x%" PRIx64 ", %s", int64_t(imm), nameIReg(8,dst));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, dst, GROUP1_OP_AND);
            m_formatter.immediate8s(imm);
        } else {
            if (dst == X86Registers::eax)
                m_formatter.oneByteOp64(OP_AND_EAXIv);
            else
                m_formatter.oneByteOp64(OP_GROUP1_EvIz, dst, GROUP1_OP_AND);
            m_formatter.immediate32(imm);
        }
    }
#else
    void andl_im(int32_t imm, const void* addr)
    {
        spew("andl       $0x%x, %p", imm, addr);
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, addr, GROUP1_OP_AND);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, addr, GROUP1_OP_AND);
            m_formatter.immediate32(imm);
        }
    }
#endif

    void fld_m(int32_t offset, RegisterID base)
    {
        spew("fld        " MEM_ob, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_FPU6, offset, base, FPU6_OP_FLD);
    }
    void fld32_m(int32_t offset, RegisterID base)
    {
        spew("fld        " MEM_ob, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_FPU6_F32, offset, base, FPU6_OP_FLD);
    }
    void fisttp_m(int32_t offset, RegisterID base)
    {
        spew("fisttp     " MEM_ob, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_FPU6, offset, base, FPU6_OP_FISTTP);
    }
    void fstp_m(int32_t offset, RegisterID base)
    {
        spew("fstp       " MEM_ob, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_FPU6, offset, base, FPU6_OP_FSTP);
    }
    void fstp32_m(int32_t offset, RegisterID base)
    {
        spew("fstp32       " MEM_ob, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_FPU6_F32, offset, base, FPU6_OP_FSTP);
    }

    void negl_r(RegisterID dst)
    {
        spew("negl       %s", nameIReg(4,dst));
        m_formatter.oneByteOp(OP_GROUP3_Ev, dst, GROUP3_OP_NEG);
    }

    void negl_m(int32_t offset, RegisterID base)
    {
        FIXME_INSN_PRINTING;
        m_formatter.oneByteOp(OP_GROUP3_Ev, offset, base, GROUP3_OP_NEG);
    }

    void notl_r(RegisterID dst)
    {
        spew("notl       %s", nameIReg(4,dst));
        m_formatter.oneByteOp(OP_GROUP3_Ev, dst, GROUP3_OP_NOT);
    }

    void notl_m(int32_t offset, RegisterID base)
    {
        FIXME_INSN_PRINTING;
        m_formatter.oneByteOp(OP_GROUP3_Ev, offset, base, GROUP3_OP_NOT);
    }

    void orl_rr(RegisterID src, RegisterID dst)
    {
        spew("orl        %s, %s", nameIReg(4,src), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_OR_GvEv, src, dst);
    }

    void orl_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("orl        " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_OR_GvEv, offset, base, dst);
    }

    void orl_rm(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("orl        %s, " MEM_ob, nameIReg(4,src), ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_OR_EvGv, offset, base, src);
    }

    void orl_ir(int32_t imm, RegisterID dst)
    {
        spew("orl        $0x%x, %s", imm, nameIReg(4,dst));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, dst, GROUP1_OP_OR);
            m_formatter.immediate8s(imm);
        } else {
            if (dst == X86Registers::eax)
                m_formatter.oneByteOp(OP_OR_EAXIv);
            else
                m_formatter.oneByteOp(OP_GROUP1_EvIz, dst, GROUP1_OP_OR);
            m_formatter.immediate32(imm);
        }
    }

    void orl_im(int32_t imm, int32_t offset, RegisterID base)
    {
        spew("orl        $0x%x, " MEM_ob, imm, ADDR_ob(offset, base));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, offset, base, GROUP1_OP_OR);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, offset, base, GROUP1_OP_OR);
            m_formatter.immediate32(imm);
        }
    }

#ifdef JS_CODEGEN_X64
    void negq_r(RegisterID dst)
    {
        spew("negq       %s", nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_GROUP3_Ev, dst, GROUP3_OP_NEG);
    }

    void orq_rr(RegisterID src, RegisterID dst)
    {
        spew("orq        %s, %s", nameIReg(8,src), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_OR_GvEv, src, dst);
    }

    void orq_ir(int32_t imm, RegisterID dst)
    {
        spew("orq        $0x%" PRIx64 ", %s", int64_t(imm), nameIReg(8,dst));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, dst, GROUP1_OP_OR);
            m_formatter.immediate8s(imm);
        } else {
            if (dst == X86Registers::eax)
                m_formatter.oneByteOp64(OP_OR_EAXIv);
            else
                m_formatter.oneByteOp64(OP_GROUP1_EvIz, dst, GROUP1_OP_OR);
            m_formatter.immediate32(imm);
        }
    }

    void notq_r(RegisterID dst)
    {
        spew("notq       %s", nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_GROUP3_Ev, dst, GROUP3_OP_NOT);
    }
#else
    void orl_im(int32_t imm, const void* addr)
    {
        FIXME_INSN_PRINTING;
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, addr, GROUP1_OP_OR);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, addr, GROUP1_OP_OR);
            m_formatter.immediate32(imm);
        }
    }
#endif

    void subl_rr(RegisterID src, RegisterID dst)
    {
        spew("subl       %s, %s", nameIReg(4,src), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_SUB_GvEv, src, dst);
    }

    void subl_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("subl       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_SUB_GvEv, offset, base, dst);
    }

    void subl_rm(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("subl       %s, " MEM_ob, nameIReg(4,src), ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_SUB_EvGv, offset, base, src);
    }

    void subl_ir(int32_t imm, RegisterID dst)
    {
        spew("subl       $%d, %s", imm, nameIReg(4, dst));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, dst, GROUP1_OP_SUB);
            m_formatter.immediate8s(imm);
        } else {
            if (dst == X86Registers::eax)
                m_formatter.oneByteOp(OP_SUB_EAXIv);
            else
                m_formatter.oneByteOp(OP_GROUP1_EvIz, dst, GROUP1_OP_SUB);
            m_formatter.immediate32(imm);
        }
    }

    void subl_im(int32_t imm, int32_t offset, RegisterID base)
    {
        spew("subl       $%d, " MEM_ob, imm, ADDR_ob(offset, base));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, offset, base, GROUP1_OP_SUB);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, offset, base, GROUP1_OP_SUB);
            m_formatter.immediate32(imm);
        }
    }

#ifdef JS_CODEGEN_X64
    void subq_rr(RegisterID src, RegisterID dst)
    {
        spew("subq       %s, %s", nameIReg(8,src), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_SUB_GvEv, src, dst);
    }

    void subq_rm(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("subq       %s, " MEM_ob, nameIReg(8,src), ADDR_ob(offset, base));
        m_formatter.oneByteOp64(OP_SUB_EvGv, offset, base, src);
    }

    void subq_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("subq       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_SUB_GvEv, offset, base, dst);
    }

    void subq_mr(const void* addr, RegisterID dst)
    {
        spew("subq       %p, %s", addr, nameIReg(8, dst));
        m_formatter.oneByteOp64(OP_SUB_GvEv, addr, dst);
    }

    void subq_ir(int32_t imm, RegisterID dst)
    {
        spew("subq       $%d, %s", imm, nameIReg(8,dst));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, dst, GROUP1_OP_SUB);
            m_formatter.immediate8s(imm);
        } else {
            if (dst == X86Registers::eax)
                m_formatter.oneByteOp64(OP_SUB_EAXIv);
            else
                m_formatter.oneByteOp64(OP_GROUP1_EvIz, dst, GROUP1_OP_SUB);
            m_formatter.immediate32(imm);
        }
    }
#else
    void subl_im(int32_t imm, const void* addr)
    {
        FIXME_INSN_PRINTING;
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, addr, GROUP1_OP_SUB);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, addr, GROUP1_OP_SUB);
            m_formatter.immediate32(imm);
        }
    }
#endif

    void xorl_rr(RegisterID src, RegisterID dst)
    {
        spew("xorl       %s, %s", nameIReg(4,src), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_XOR_GvEv, src, dst);
    }

    void xorl_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("xorl       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_XOR_GvEv, offset, base, dst);
    }

    void xorl_rm(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("xorl       %s, " MEM_ob, nameIReg(4,src), ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_XOR_EvGv, offset, base, src);
    }

    void xorl_im(int32_t imm, int32_t offset, RegisterID base)
    {
        spew("xorl       $0x%x, " MEM_ob, imm, ADDR_ob(offset, base));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, offset, base, GROUP1_OP_XOR);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, offset, base, GROUP1_OP_XOR);
            m_formatter.immediate32(imm);
        }
    }

    void xorl_ir(int32_t imm, RegisterID dst)
    {
        spew("xorl       $%d, %s", imm, nameIReg(4,dst));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, dst, GROUP1_OP_XOR);
            m_formatter.immediate8s(imm);
        } else {
            if (dst == X86Registers::eax)
                m_formatter.oneByteOp(OP_XOR_EAXIv);
            else
                m_formatter.oneByteOp(OP_GROUP1_EvIz, dst, GROUP1_OP_XOR);
            m_formatter.immediate32(imm);
        }
    }

#ifdef JS_CODEGEN_X64
    void xorq_rr(RegisterID src, RegisterID dst)
    {
        spew("xorq       %s, %s", nameIReg(8,src), nameIReg(8, dst));
        m_formatter.oneByteOp64(OP_XOR_GvEv, src, dst);
    }

    void xorq_ir(int32_t imm, RegisterID dst)
    {
        spew("xorq       $0x%" PRIx64 ", %s", int64_t(imm), nameIReg(8,dst));
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, dst, GROUP1_OP_XOR);
            m_formatter.immediate8s(imm);
        } else {
            if (dst == X86Registers::eax)
                m_formatter.oneByteOp64(OP_XOR_EAXIv);
            else
                m_formatter.oneByteOp64(OP_GROUP1_EvIz, dst, GROUP1_OP_XOR);
            m_formatter.immediate32(imm);
        }
    }
#endif

    void sarl_ir(int32_t imm, RegisterID dst)
    {
        MOZ_ASSERT(imm < 32);
        spew("sarl       $%d, %s", imm, nameIReg(4, dst));
        if (imm == 1)
            m_formatter.oneByteOp(OP_GROUP2_Ev1, dst, GROUP2_OP_SAR);
        else {
            m_formatter.oneByteOp(OP_GROUP2_EvIb, dst, GROUP2_OP_SAR);
            m_formatter.immediate8u(imm);
        }
    }

    void sarl_CLr(RegisterID dst)
    {
        spew("sarl       %%cl, %s", nameIReg(4, dst));
        m_formatter.oneByteOp(OP_GROUP2_EvCL, dst, GROUP2_OP_SAR);
    }

    void shrl_ir(int32_t imm, RegisterID dst)
    {
        MOZ_ASSERT(imm < 32);
        spew("shrl       $%d, %s", imm, nameIReg(4, dst));
        if (imm == 1)
            m_formatter.oneByteOp(OP_GROUP2_Ev1, dst, GROUP2_OP_SHR);
        else {
            m_formatter.oneByteOp(OP_GROUP2_EvIb, dst, GROUP2_OP_SHR);
            m_formatter.immediate8u(imm);
        }
    }

    void shrl_CLr(RegisterID dst)
    {
        spew("shrl       %%cl, %s", nameIReg(4, dst));
        m_formatter.oneByteOp(OP_GROUP2_EvCL, dst, GROUP2_OP_SHR);
    }

    void shll_ir(int32_t imm, RegisterID dst)
    {
        MOZ_ASSERT(imm < 32);
        spew("shll       $%d, %s", imm, nameIReg(4, dst));
        if (imm == 1)
            m_formatter.oneByteOp(OP_GROUP2_Ev1, dst, GROUP2_OP_SHL);
        else {
            m_formatter.oneByteOp(OP_GROUP2_EvIb, dst, GROUP2_OP_SHL);
            m_formatter.immediate8u(imm);
        }
    }

    void shll_CLr(RegisterID dst)
    {
        spew("shll       %%cl, %s", nameIReg(4, dst));
        m_formatter.oneByteOp(OP_GROUP2_EvCL, dst, GROUP2_OP_SHL);
    }

#ifdef JS_CODEGEN_X64
    void sarq_CLr(RegisterID dst)
    {
        FIXME_INSN_PRINTING;
        m_formatter.oneByteOp64(OP_GROUP2_EvCL, dst, GROUP2_OP_SAR);
    }

    void sarq_ir(int32_t imm, RegisterID dst)
    {
        MOZ_ASSERT(imm < 64);
        spew("sarq       $%d, %s", imm, nameIReg(8, dst));
        if (imm == 1)
            m_formatter.oneByteOp64(OP_GROUP2_Ev1, dst, GROUP2_OP_SAR);
        else {
            m_formatter.oneByteOp64(OP_GROUP2_EvIb, dst, GROUP2_OP_SAR);
            m_formatter.immediate8u(imm);
        }
    }

    void shlq_ir(int32_t imm, RegisterID dst)
    {
        MOZ_ASSERT(imm < 64);
        spew("shlq       $%d, %s", imm, nameIReg(8, dst));
        if (imm == 1)
            m_formatter.oneByteOp64(OP_GROUP2_Ev1, dst, GROUP2_OP_SHL);
        else {
            m_formatter.oneByteOp64(OP_GROUP2_EvIb, dst, GROUP2_OP_SHL);
            m_formatter.immediate8u(imm);
        }
    }

    void shrq_ir(int32_t imm, RegisterID dst)
    {
        MOZ_ASSERT(imm < 64);
        spew("shrq       $%d, %s", imm, nameIReg(8, dst));
        if (imm == 1)
            m_formatter.oneByteOp64(OP_GROUP2_Ev1, dst, GROUP2_OP_SHR);
        else {
            m_formatter.oneByteOp64(OP_GROUP2_EvIb, dst, GROUP2_OP_SHR);
            m_formatter.immediate8u(imm);
        }
    }
#endif

    void bsr_rr(RegisterID src, RegisterID dst)
    {
        spew("bsr        %s, %s", nameIReg(4, src), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_BSR_GvEv, src, dst);
    }

    void imull_rr(RegisterID src, RegisterID dst)
    {
        spew("imull      %s, %s", nameIReg(4,src), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_IMUL_GvEv, src, dst);
    }

    void imull_r(RegisterID multiplier)
    {
        spew("imull      %s", nameIReg(4, multiplier));
        m_formatter.oneByteOp(OP_GROUP3_Ev, multiplier, GROUP3_OP_IMUL);
    }

    void imull_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("imull      " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4,dst));
        m_formatter.twoByteOp(OP2_IMUL_GvEv, offset, base, dst);
    }

    void imull_ir(int32_t value, RegisterID src, RegisterID dst)
    {
        spew("imull      $%d, %s, %s", value, nameIReg(4, src), nameIReg(4, dst));
        if (CAN_SIGN_EXTEND_8_32(value)) {
            m_formatter.oneByteOp(OP_IMUL_GvEvIb, src, dst);
            m_formatter.immediate8s(value);
        } else {
            m_formatter.oneByteOp(OP_IMUL_GvEvIz, src, dst);
            m_formatter.immediate32(value);
        }
    }

    void idivl_r(RegisterID divisor)
    {
        spew("idivl      %s", nameIReg(4, divisor));
        m_formatter.oneByteOp(OP_GROUP3_Ev, divisor, GROUP3_OP_IDIV);
    }

    void divl_r(RegisterID divisor)
    {
        spew("div        %s", nameIReg(4, divisor));
        m_formatter.oneByteOp(OP_GROUP3_Ev, divisor, GROUP3_OP_DIV);
    }

    void prefix_lock()
    {
        spew("lock");
        m_formatter.oneByteOp(PRE_LOCK);
    }

    void prefix_16_for_32()
    {
        m_formatter.prefix(PRE_OPERAND_SIZE);
    }

    void incl_m32(int32_t offset, RegisterID base)
    {
        spew("incl       " MEM_ob, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_GROUP5_Ev, offset, base, GROUP5_OP_INC);
    }

    void decl_m32(int32_t offset, RegisterID base)
    {
        spew("decl       " MEM_ob, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_GROUP5_Ev, offset, base, GROUP5_OP_DEC);
    }

    
    
    
    

    void cmpxchg8(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("cmpxchg8   %s, " MEM_ob, nameIReg(src), ADDR_ob(offset, base));
        m_formatter.twoByteOp(OP2_CMPXCHG_GvEb, offset, base, src);
    }
    void cmpxchg8(RegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("cmpxchg8   %s, " MEM_obs, nameIReg(src), ADDR_obs(offset, base, index, scale));
        m_formatter.twoByteOp(OP2_CMPXCHG_GvEb, offset, base, index, scale, src);
    }
    void cmpxchg16(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("cmpxchg16  %s, " MEM_ob, nameIReg(src), ADDR_ob(offset, base));
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.twoByteOp(OP2_CMPXCHG_GvEw, offset, base, src);
    }
    void cmpxchg16(RegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("cmpxchg16  %s, " MEM_obs, nameIReg(src), ADDR_obs(offset, base, index, scale));
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.twoByteOp(OP2_CMPXCHG_GvEw, offset, base, index, scale, src);
    }
    void cmpxchg32(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("cmpxchg32  %s, " MEM_ob, nameIReg(src), ADDR_ob(offset, base));
        m_formatter.twoByteOp(OP2_CMPXCHG_GvEw, offset, base, src);
    }
    void cmpxchg32(RegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("cmpxchg32  %s, " MEM_obs, nameIReg(src), ADDR_obs(offset, base, index, scale));
        m_formatter.twoByteOp(OP2_CMPXCHG_GvEw, offset, base, index, scale, src);
    }


    

    void cmpl_rr(RegisterID rhs, RegisterID lhs)
    {
        spew("cmpl       %s, %s", nameIReg(4, rhs), nameIReg(4, lhs));
        m_formatter.oneByteOp(OP_CMP_GvEv, rhs, lhs);
    }

    void cmpl_rm(RegisterID rhs, int32_t offset, RegisterID base)
    {
        spew("cmpl       %s, " MEM_ob, nameIReg(4, rhs), ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_CMP_EvGv, offset, base, rhs);
    }

    void cmpl_mr(int32_t offset, RegisterID base, RegisterID lhs)
    {
        spew("cmpl       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4, lhs));
        m_formatter.oneByteOp(OP_CMP_GvEv, offset, base, lhs);
    }

    void cmpl_mr(const void *address, RegisterID lhs)
    {
        spew("cmpl       %p, %s", address, nameIReg(4, lhs));
        m_formatter.oneByteOp(OP_CMP_GvEv, address, lhs);
    }

    void cmpl_ir(int rhs, RegisterID lhs)
    {
        if (rhs == 0) {
            testl_rr(lhs, lhs);
            return;
        }

        spew("cmpl       $0x%x, %s", rhs, nameIReg(4, lhs));
        if (CAN_SIGN_EXTEND_8_32(rhs)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, lhs, GROUP1_OP_CMP);
            m_formatter.immediate8s(rhs);
        } else {
            if (lhs == X86Registers::eax)
                m_formatter.oneByteOp(OP_CMP_EAXIv);
            else
                m_formatter.oneByteOp(OP_GROUP1_EvIz, lhs, GROUP1_OP_CMP);
            m_formatter.immediate32(rhs);
        }
    }

    void cmpl_i32r(int rhs, RegisterID lhs)
    {
        spew("cmpl       $0x%04x, %s", rhs, nameIReg(4, lhs));
        if (lhs == X86Registers::eax)
            m_formatter.oneByteOp(OP_CMP_EAXIv);
        else
            m_formatter.oneByteOp(OP_GROUP1_EvIz, lhs, GROUP1_OP_CMP);
        m_formatter.immediate32(rhs);
    }

    void cmpl_im(int rhs, int32_t offset, RegisterID base)
    {
        spew("cmpl       $0x%x, " MEM_ob, rhs, ADDR_ob(offset, base));
        if (CAN_SIGN_EXTEND_8_32(rhs)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, offset, base, GROUP1_OP_CMP);
            m_formatter.immediate8s(rhs);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, offset, base, GROUP1_OP_CMP);
            m_formatter.immediate32(rhs);
        }
    }

    void cmpb_im(int rhs, int32_t offset, RegisterID base)
    {
        spew("cmpb       $0x%x, " MEM_ob, rhs, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_GROUP1_EbIb, offset, base, GROUP1_OP_CMP);
        m_formatter.immediate8(rhs);
    }

    void cmpb_im(int rhs, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("cmpb       $0x%x, " MEM_obs, rhs, ADDR_obs(offset, base, index, scale));
        m_formatter.oneByteOp(OP_GROUP1_EbIb, offset, base, index, scale, GROUP1_OP_CMP);
        m_formatter.immediate8(rhs);
    }

    void cmpl_im(int rhs, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("cmpl       $0x%x, " MEM_o32b, rhs, ADDR_o32b(offset, base));
        if (CAN_SIGN_EXTEND_8_32(rhs)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, offset, base, index, scale, GROUP1_OP_CMP);
            m_formatter.immediate8s(rhs);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, offset, base, index, scale, GROUP1_OP_CMP);
            m_formatter.immediate32(rhs);
        }
    }

    MOZ_WARN_UNUSED_RESULT JmpSrc
    cmpl_im_disp32(int rhs, int32_t offset, RegisterID base)
    {
        spew("cmpl       $0x%x, " MEM_o32b, rhs, ADDR_o32b(offset, base));
        JmpSrc r;
        if (CAN_SIGN_EXTEND_8_32(rhs)) {
            m_formatter.oneByteOp_disp32(OP_GROUP1_EvIb, offset, base, GROUP1_OP_CMP);
            r = JmpSrc(m_formatter.size());
            m_formatter.immediate8s(rhs);
        } else {
            m_formatter.oneByteOp_disp32(OP_GROUP1_EvIz, offset, base, GROUP1_OP_CMP);
            r = JmpSrc(m_formatter.size());
            m_formatter.immediate32(rhs);
        }
        return r;
    }

    MOZ_WARN_UNUSED_RESULT JmpSrc
    cmpl_im_disp32(int rhs, const void *addr)
    {
        spew("cmpl       $0x%x, %p", rhs, addr);
        JmpSrc r;
        if (CAN_SIGN_EXTEND_8_32(rhs)) {
            m_formatter.oneByteOp_disp32(OP_GROUP1_EvIb, addr, GROUP1_OP_CMP);
            r = JmpSrc(m_formatter.size());
            m_formatter.immediate8s(rhs);
        } else {
            m_formatter.oneByteOp_disp32(OP_GROUP1_EvIz, addr, GROUP1_OP_CMP);
            r = JmpSrc(m_formatter.size());
            m_formatter.immediate32(rhs);
        }
        return r;
    }

    void cmpl_i32m(int rhs, int32_t offset, RegisterID base)
    {
        spew("cmpl       $0x%04x, " MEM_ob, rhs, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_GROUP1_EvIz, offset, base, GROUP1_OP_CMP);
        m_formatter.immediate32(rhs);
    }

    void cmpl_i32m(int rhs, const void *addr)
    {
        spew("cmpl       $0x%04x, %p", rhs, addr);
        m_formatter.oneByteOp(OP_GROUP1_EvIz, addr, GROUP1_OP_CMP);
        m_formatter.immediate32(rhs);
    }

#ifdef JS_CODEGEN_X64
    void cmpq_rr(RegisterID rhs, RegisterID lhs)
    {
        spew("cmpq       %s, %s", nameIReg(8, rhs), nameIReg(8, lhs));
        m_formatter.oneByteOp64(OP_CMP_GvEv, rhs, lhs);
    }

    void cmpq_rm(RegisterID rhs, int32_t offset, RegisterID base)
    {
        spew("cmpq       %s, " MEM_ob, nameIReg(8, rhs), ADDR_ob(offset, base));
        m_formatter.oneByteOp64(OP_CMP_EvGv, offset, base, rhs);
    }

    void cmpq_mr(int32_t offset, RegisterID base, RegisterID lhs)
    {
        spew("cmpq       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(8, lhs));
        m_formatter.oneByteOp64(OP_CMP_GvEv, offset, base, lhs);
    }

    void cmpq_ir(int rhs, RegisterID lhs)
    {
        if (rhs == 0) {
            testq_rr(lhs, lhs);
            return;
        }

        spew("cmpq       $0x%" PRIx64 ", %s", int64_t(rhs), nameIReg(8, lhs));
        if (CAN_SIGN_EXTEND_8_32(rhs)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, lhs, GROUP1_OP_CMP);
            m_formatter.immediate8s(rhs);
        } else {
            if (lhs == X86Registers::eax)
                m_formatter.oneByteOp64(OP_CMP_EAXIv);
            else
                m_formatter.oneByteOp64(OP_GROUP1_EvIz, lhs, GROUP1_OP_CMP);
            m_formatter.immediate32(rhs);
        }
    }

    void cmpq_im(int rhs, int32_t offset, RegisterID base)
    {
        spew("cmpq       $0x%" PRIx64 ", " MEM_ob, int64_t(rhs), ADDR_ob(offset, base));
        if (CAN_SIGN_EXTEND_8_32(rhs)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, offset, base, GROUP1_OP_CMP);
            m_formatter.immediate8s(rhs);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, offset, base, GROUP1_OP_CMP);
            m_formatter.immediate32(rhs);
        }
    }

    void cmpq_im(int rhs, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        FIXME_INSN_PRINTING;
        if (CAN_SIGN_EXTEND_8_32(rhs)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, offset, base, index, scale, GROUP1_OP_CMP);
            m_formatter.immediate8s(rhs);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, offset, base, index, scale, GROUP1_OP_CMP);
            m_formatter.immediate32(rhs);
        }
    }
    void cmpq_im(int rhs, const void* addr)
    {
        spew("cmpq       $0x%" PRIx64 ", %p", int64_t(rhs), addr);
        if (CAN_SIGN_EXTEND_8_32(rhs)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, addr, GROUP1_OP_CMP);
            m_formatter.immediate8s(rhs);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, addr, GROUP1_OP_CMP);
            m_formatter.immediate32(rhs);
        }
    }
    void cmpq_rm(RegisterID rhs, const void* addr)
    {
        spew("cmpq       %s, %p", nameIReg(8, rhs), addr);
        m_formatter.oneByteOp64(OP_CMP_EvGv, addr, rhs);
    }
#endif
    void cmpl_rm(RegisterID rhs, const void* addr)
    {
        spew("cmpl       %s, %p", nameIReg(4, rhs), addr);
        m_formatter.oneByteOp(OP_CMP_EvGv, addr, rhs);
    }

    void cmpl_rm_disp32(RegisterID rhs, const void* addr)
    {
        spew("cmpl       %s, %p", nameIReg(4, rhs), addr);
        m_formatter.oneByteOp_disp32(OP_CMP_EvGv, addr, rhs);
    }

    void cmpl_im(int rhs, const void* addr)
    {
        spew("cmpl       $0x%x, %p", rhs, addr);
        if (CAN_SIGN_EXTEND_8_32(rhs)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, addr, GROUP1_OP_CMP);
            m_formatter.immediate8s(rhs);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, addr, GROUP1_OP_CMP);
            m_formatter.immediate32(rhs);
        }
    }

    void cmpw_rr(RegisterID rhs, RegisterID lhs)
    {
        spew("cmpw       %s, %s", nameIReg(2, rhs), nameIReg(2, lhs));
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp(OP_CMP_GvEv, rhs, lhs);
    }

    void cmpw_rm(RegisterID rhs, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        FIXME_INSN_PRINTING;
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp(OP_CMP_EvGv, offset, base, index, scale, rhs);
    }

    void cmpw_im(int32_t imm, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        FIXME_INSN_PRINTING;
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.prefix(PRE_OPERAND_SIZE);
            m_formatter.oneByteOp(OP_GROUP1_EvIb, offset, base, index, scale, GROUP1_OP_CMP);
            m_formatter.immediate8s(imm);
        } else {
            m_formatter.prefix(PRE_OPERAND_SIZE);
            m_formatter.oneByteOp(OP_GROUP1_EvIz, offset, base, index, scale, GROUP1_OP_CMP);
            m_formatter.immediate16(imm);
        }
    }

    void testl_rr(RegisterID rhs, RegisterID lhs)
    {
        spew("testl      %s, %s", nameIReg(4, rhs), nameIReg(4, lhs));
        m_formatter.oneByteOp(OP_TEST_EvGv, lhs, rhs);
    }

    void testb_rr(RegisterID rhs, RegisterID lhs)
    {
        spew("testb      %s, %s", nameIReg(1, rhs), nameIReg(1, lhs));
        m_formatter.oneByteOp(OP_TEST_EbGb, lhs, rhs);
    }

    void testl_ir(int rhs, RegisterID lhs)
    {
        
        
        if (CAN_ZERO_EXTEND_8_32(rhs) && X86Registers::hasSubregL(lhs)) {
            testb_ir(rhs, lhs);
            return;
        }
        
        
        if (CAN_ZERO_EXTEND_8H_32(rhs) && X86Registers::hasSubregH(lhs)) {
            testb_ir_norex(rhs >> 8, X86Registers::getSubregH(lhs));
            return;
        }
        spew("testl      $0x%x, %s", rhs, nameIReg(4, lhs));
        if (lhs == X86Registers::eax)
            m_formatter.oneByteOp(OP_TEST_EAXIv);
        else
            m_formatter.oneByteOp(OP_GROUP3_EvIz, lhs, GROUP3_OP_TEST);
        m_formatter.immediate32(rhs);
    }

    void testl_i32m(int rhs, int32_t offset, RegisterID base)
    {
        spew("testl      $0x%x, " MEM_ob, rhs, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_GROUP3_EvIz, offset, base, GROUP3_OP_TEST);
        m_formatter.immediate32(rhs);
    }

    void testl_i32m(int rhs, const void *addr)
    {
        spew("testl      $0x%x, %p", rhs, addr);
        m_formatter.oneByteOp(OP_GROUP3_EvIz, addr, GROUP3_OP_TEST);
        m_formatter.immediate32(rhs);
    }

    void testb_im(int rhs, int32_t offset, RegisterID base)
    {
        FIXME_INSN_PRINTING;
        m_formatter.oneByteOp(OP_GROUP3_EbIb, offset, base, GROUP3_OP_TEST);
        m_formatter.immediate8(rhs);
    }

    void testb_im(int rhs, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        FIXME_INSN_PRINTING;
        m_formatter.oneByteOp(OP_GROUP3_EbIb, offset, base, index, scale, GROUP3_OP_TEST);
        m_formatter.immediate8(rhs);
    }

    void testl_i32m(int rhs, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        FIXME_INSN_PRINTING;
        m_formatter.oneByteOp(OP_GROUP3_EvIz, offset, base, index, scale, GROUP3_OP_TEST);
        m_formatter.immediate32(rhs);
    }

#ifdef JS_CODEGEN_X64
    void testq_rr(RegisterID rhs, RegisterID lhs)
    {
        spew("testq      %s, %s", nameIReg(8, rhs), nameIReg(8, lhs));
        m_formatter.oneByteOp64(OP_TEST_EvGv, lhs, rhs);
    }

    void testq_ir(int rhs, RegisterID lhs)
    {
        
        
        if (CAN_ZERO_EXTEND_32_64(rhs)) {
            testl_ir(rhs, lhs);
            return;
        }
        spew("testq      $0x%" PRIx64 ", %s", int64_t(rhs), nameIReg(8, lhs));
        if (lhs == X86Registers::eax)
            m_formatter.oneByteOp64(OP_TEST_EAXIv);
        else
            m_formatter.oneByteOp64(OP_GROUP3_EvIz, lhs, GROUP3_OP_TEST);
        m_formatter.immediate32(rhs);
    }

    void testq_i32m(int rhs, int32_t offset, RegisterID base)
    {
        spew("testq      $0x%" PRIx64 ", " MEM_ob, int64_t(rhs), ADDR_ob(offset, base));
        m_formatter.oneByteOp64(OP_GROUP3_EvIz, offset, base, GROUP3_OP_TEST);
        m_formatter.immediate32(rhs);
    }

    void testq_i32m(int rhs, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        FIXME_INSN_PRINTING;
        m_formatter.oneByteOp64(OP_GROUP3_EvIz, offset, base, index, scale, GROUP3_OP_TEST);
        m_formatter.immediate32(rhs);
    }
#endif

    void testw_rr(RegisterID rhs, RegisterID lhs)
    {
        FIXME_INSN_PRINTING;
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp(OP_TEST_EvGv, lhs, rhs);
    }

    void testb_ir(int rhs, RegisterID lhs)
    {
        spew("testb      $0x%x, %s", rhs, nameIReg(1, lhs));
        if (lhs == X86Registers::eax)
            m_formatter.oneByteOp8(OP_TEST_EAXIb);
        else
            m_formatter.oneByteOp8(OP_GROUP3_EbIb, lhs, GROUP3_OP_TEST);
        m_formatter.immediate8(rhs);
    }

    
    
    void testb_ir_norex(int rhs, RegisterID lhs)
    {
        spew("testb      $0x%x, %s", rhs, nameI8Reg_norex(lhs));
        m_formatter.oneByteOp8_norex(OP_GROUP3_EbIb, lhs, GROUP3_OP_TEST);
        m_formatter.immediate8(rhs);
    }

    void setCC_r(Condition cond, RegisterID lhs)
    {
        spew("set%s      %s", nameCC(cond), nameIReg(1, lhs));
        m_formatter.twoByteOp8(setccOpcode(cond), lhs, (GroupOpcodeID)0);
    }

    void sete_r(RegisterID dst)
    {
        setCC_r(ConditionE, dst);
    }

    void setz_r(RegisterID dst)
    {
        sete_r(dst);
    }

    void setne_r(RegisterID dst)
    {
        setCC_r(ConditionNE, dst);
    }

    void setnz_r(RegisterID dst)
    {
        setne_r(dst);
    }

    

    void cdq()
    {
        spew("cdq        ");
        m_formatter.oneByteOp(OP_CDQ);
    }

    void xchgl_rr(RegisterID src, RegisterID dst)
    {
        spew("xchgl      %s, %s", nameIReg(4,src), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_XCHG_GvEv, src, dst);
    }

#ifdef JS_CODEGEN_X64
    void xchgq_rr(RegisterID src, RegisterID dst)
    {
        spew("xchgq      %s, %s", nameIReg(8,src), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_XCHG_GvEv, src, dst);
    }
#endif

    void movl_rr(RegisterID src, RegisterID dst)
    {
        spew("movl       %s, %s", nameIReg(4,src), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_MOV_GvEv, src, dst);
    }

    void movw_rm(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("movw       %s, " MEM_ob, nameIReg(2,src), ADDR_ob(offset, base));
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp(OP_MOV_EvGv, offset, base, src);
    }

    void movw_rm_disp32(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("movw       %s, " MEM_o32b, nameIReg(2,src), ADDR_o32b(offset, base));
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp_disp32(OP_MOV_EvGv, offset, base, src);
    }

    void movw_rm(RegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("movw       %s, " MEM_obs, nameIReg(2, src), ADDR_obs(offset, base, index, scale));
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp(OP_MOV_EvGv, offset, base, index, scale, src);
    }

    void movw_rm(RegisterID src, const void* addr)
    {
        spew("movw       %s, %p", nameIReg(2, src), addr);
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp_disp32(OP_MOV_EvGv, addr, src);
    }

    void movl_rm(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("movl       %s, " MEM_ob, nameIReg(4,src), ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_MOV_EvGv, offset, base, src);
    }

    void movl_rm_disp32(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("movl       %s, " MEM_o32b, nameIReg(4,src), ADDR_o32b(offset, base));
        m_formatter.oneByteOp_disp32(OP_MOV_EvGv, offset, base, src);
    }

    void movl_rm(RegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("movl       %s, " MEM_obs, nameIReg(4, src), ADDR_obs(offset, base, index, scale));
        m_formatter.oneByteOp(OP_MOV_EvGv, offset, base, index, scale, src);
    }

    void movl_mEAX(const void* addr)
    {
#ifdef JS_CODEGEN_X64
        if (isAddressImmediate(addr)) {
            movl_mr(addr, X86Registers::eax);
            return;
        }
#endif

#ifdef JS_CODEGEN_X64
        spew("movabs     %p, %%eax", addr);
#else
        spew("movl       %p, %%eax", addr);
#endif
        m_formatter.oneByteOp(OP_MOV_EAXOv);
#ifdef JS_CODEGEN_X64
        m_formatter.immediate64(reinterpret_cast<int64_t>(addr));
#else
        m_formatter.immediate32(reinterpret_cast<int>(addr));
#endif
    }

    void movl_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movl       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4, dst));
        m_formatter.oneByteOp(OP_MOV_GvEv, offset, base, dst);
    }

    void movl_mr_disp32(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movl       " MEM_o32b ", %s", ADDR_o32b(offset, base), nameIReg(4,dst));
        m_formatter.oneByteOp_disp32(OP_MOV_GvEv, offset, base, dst);
    }

    void movl_mr(const void* base, RegisterID index, int scale, RegisterID dst)
    {
        int32_t disp = addressImmediate(base);

        spew("movl       " MEM_os ", %s", ADDR_os(disp, index, scale), nameIReg(4, dst));
        m_formatter.oneByteOp_disp32(OP_MOV_GvEv, disp, index, scale, dst);
    }

    void movl_mr(int32_t offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        spew("movl       " MEM_obs ", %s", ADDR_obs(offset, base, index, scale), nameIReg(4, dst));
        m_formatter.oneByteOp(OP_MOV_GvEv, offset, base, index, scale, dst);
    }

    void movl_mr(const void* addr, RegisterID dst)
    {
        if (dst == X86Registers::eax
#ifdef JS_CODEGEN_X64
            && !isAddressImmediate(addr)
#endif
            )
        {
            movl_mEAX(addr);
            return;
        }

        spew("movl       %p, %s", addr, nameIReg(4, dst));
        m_formatter.oneByteOp(OP_MOV_GvEv, addr, dst);
    }

    void movl_i32r(int32_t imm, RegisterID dst)
    {
        spew("movl       $0x%x, %s", imm, nameIReg(4, dst));
        m_formatter.oneByteOp(OP_MOV_EAXIv, dst);
        m_formatter.immediate32(imm);
    }

    void movb_ir(int32_t imm, RegisterID reg)
    {
        spew("movb       $0x%x, %s", imm, nameIReg(1, reg));
        m_formatter.oneByteOp(OP_MOV_EbGv, reg);
        m_formatter.immediate8(imm);
    }

    void movb_im(int32_t imm, int32_t offset, RegisterID base)
    {
        spew("movb       $0x%x, " MEM_ob, imm, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_GROUP11_EvIb, offset, base, GROUP11_MOV);
        m_formatter.immediate8(imm);
    }

    void movb_im(int32_t imm, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("movb       $0x%x, " MEM_obs, imm, ADDR_obs(offset, base, index, scale));
        m_formatter.oneByteOp(OP_GROUP11_EvIb, offset, base, index, scale, GROUP11_MOV);
        m_formatter.immediate8(imm);
    }

    void movb_im(int32_t imm, const void* addr)
    {
        spew("movb       $%d, %p", imm, addr);
        m_formatter.oneByteOp_disp32(OP_GROUP11_EvIb, addr, GROUP11_MOV);
        m_formatter.immediate8(imm);
    }

    void movw_im(int32_t imm, int32_t offset, RegisterID base)
    {
        spew("movw       $0x%x, " MEM_ob, imm, ADDR_ob(offset, base));
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp(OP_GROUP11_EvIz, offset, base, GROUP11_MOV);
        m_formatter.immediate16(imm);
    }

    void movw_im(int32_t imm, const void* addr)
    {
        spew("movw       $%d, %p", imm, addr);
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp_disp32(OP_GROUP11_EvIz, addr, GROUP11_MOV);
        m_formatter.immediate16(imm);
    }

    void movl_i32m(int32_t imm, int32_t offset, RegisterID base)
    {
        spew("movl       $0x%x, " MEM_ob, imm, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_GROUP11_EvIz, offset, base, GROUP11_MOV);
        m_formatter.immediate32(imm);
    }

    void movw_im(int32_t imm, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("movw       $0x%x, " MEM_obs, imm, ADDR_obs(offset, base, index, scale));
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp(OP_GROUP11_EvIz, offset, base, index, scale, GROUP11_MOV);
        m_formatter.immediate16(imm);
    }

    void movl_i32m(int32_t imm, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("movl       $0x%x, " MEM_obs, imm, ADDR_obs(offset, base, index, scale));
        m_formatter.oneByteOp(OP_GROUP11_EvIz, offset, base, index, scale, GROUP11_MOV);
        m_formatter.immediate32(imm);
    }

    void movl_EAXm(const void* addr)
    {
#ifdef JS_CODEGEN_X64
        if (isAddressImmediate(addr)) {
            movl_rm(X86Registers::eax, addr);
            return;
        }
#endif

        spew("movl       %%eax, %p", addr);
        m_formatter.oneByteOp(OP_MOV_OvEAX);
#ifdef JS_CODEGEN_X64
        m_formatter.immediate64(reinterpret_cast<int64_t>(addr));
#else
        m_formatter.immediate32(reinterpret_cast<int>(addr));
#endif
    }

#ifdef JS_CODEGEN_X64
    void movq_rr(RegisterID src, RegisterID dst)
    {
        spew("movq       %s, %s", nameIReg(8,src), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_MOV_GvEv, src, dst);
    }

    void movq_rm(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("movq       %s, " MEM_ob, nameIReg(8,src), ADDR_ob(offset, base));
        m_formatter.oneByteOp64(OP_MOV_EvGv, offset, base, src);
    }

    void movq_rm_disp32(RegisterID src, int32_t offset, RegisterID base)
    {
        FIXME_INSN_PRINTING;
        m_formatter.oneByteOp64_disp32(OP_MOV_EvGv, offset, base, src);
    }

    void movq_rm(RegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("movq       %s, " MEM_obs, nameIReg(8,src), ADDR_obs(offset, base, index, scale));
        m_formatter.oneByteOp64(OP_MOV_EvGv, offset, base, index, scale, src);
    }

    void movq_rm(RegisterID src, const void* addr)
    {
        if (src == X86Registers::eax && !isAddressImmediate(addr)) {
            movq_EAXm(addr);
            return;
        }

        spew("movq       %s, %p", nameIReg(8, src), addr);
        m_formatter.oneByteOp64(OP_MOV_EvGv, addr, src);
    }

    void movq_mEAX(const void* addr)
    {
        if (isAddressImmediate(addr)) {
            movq_mr(addr, X86Registers::eax);
            return;
        }

        spew("movq       %p, %%rax", addr);
        m_formatter.oneByteOp64(OP_MOV_EAXOv);
        m_formatter.immediate64(reinterpret_cast<int64_t>(addr));
    }

    void movq_EAXm(const void* addr)
    {
        if (isAddressImmediate(addr)) {
            movq_rm(X86Registers::eax, addr);
            return;
        }

        spew("movq       %%rax, %p", addr);
        m_formatter.oneByteOp64(OP_MOV_OvEAX);
        m_formatter.immediate64(reinterpret_cast<int64_t>(addr));
    }

    void movq_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movq       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_MOV_GvEv, offset, base, dst);
    }

    void movq_mr_disp32(int32_t offset, RegisterID base, RegisterID dst)
    {
        FIXME_INSN_PRINTING;
        m_formatter.oneByteOp64_disp32(OP_MOV_GvEv, offset, base, dst);
    }

    void movq_mr(int32_t offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        spew("movq       " MEM_obs ", %s", ADDR_obs(offset, base, index, scale), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_MOV_GvEv, offset, base, index, scale, dst);
    }

    void movq_mr(const void* addr, RegisterID dst)
    {
        if (dst == X86Registers::eax && !isAddressImmediate(addr)) {
            movq_mEAX(addr);
            return;
        }

        spew("movq       %p, %s", addr, nameIReg(8, dst));
        m_formatter.oneByteOp64(OP_MOV_GvEv, addr, dst);
    }

    void leaq_mr(int32_t offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        spew("leaq       " MEM_obs ", %s", ADDR_obs(offset, base, index, scale), nameIReg(8,dst)),
        m_formatter.oneByteOp64(OP_LEA, offset, base, index, scale, dst);
    }

    void movq_i32m(int32_t imm, int32_t offset, RegisterID base)
    {
        spew("movq       $%d, " MEM_ob, imm, ADDR_ob(offset, base));
        m_formatter.oneByteOp64(OP_GROUP11_EvIz, offset, base, GROUP11_MOV);
        m_formatter.immediate32(imm);
    }

    void movq_i32m(int32_t imm, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("movq       $%d, " MEM_obs, imm, ADDR_obs(offset, base, index, scale));
        m_formatter.oneByteOp64(OP_GROUP11_EvIz, offset, base, index, scale, GROUP11_MOV);
        m_formatter.immediate32(imm);
    }
    void movq_i32m(int32_t imm, const void* addr)
    {
        spew("movq       $%d, %p", imm, addr);
        m_formatter.oneByteOp64(OP_GROUP11_EvIz, addr, GROUP11_MOV);
        m_formatter.immediate32(imm);
    }

    
    
    
    
    
    
    void movq_i32r(int32_t imm, RegisterID dst) {
        spew("movq       $%d, %s", imm, nameIReg(dst));
        m_formatter.oneByteOp64(OP_GROUP11_EvIz, dst, GROUP11_MOV);
        m_formatter.immediate32(imm);
    }

    void movq_i64r(int64_t imm, RegisterID dst)
    {
        spew("movabsq    $0x%" PRIx64 ", %s", imm, nameIReg(8, dst));
        m_formatter.oneByteOp64(OP_MOV_EAXIv, dst);
        m_formatter.immediate64(imm);
    }

    void movsxd_rr(RegisterID src, RegisterID dst)
    {
        spew("movsxd     %s, %s", nameIReg(4, src), nameIReg(8, dst));
        m_formatter.oneByteOp64(OP_MOVSXD_GvEv, src, dst);
    }

    JmpSrc movl_ripr(RegisterID dst)
    {
        m_formatter.oneByteRipOp(OP_MOV_GvEv, 0, (RegisterID)dst);
        JmpSrc label(m_formatter.size());
        spew("movl       .Lfrom%d(%%rip), %s", label.offset(), nameIReg(4, dst));
        return label;
    }

    JmpSrc movl_rrip(RegisterID src)
    {
        m_formatter.oneByteRipOp(OP_MOV_EvGv, 0, (RegisterID)src);
        JmpSrc label(m_formatter.size());
        spew("movl       %s, .Lfrom%d(%%rip)", nameIReg(4, src), label.offset());
        return label;
    }

    JmpSrc movq_ripr(RegisterID dst)
    {
        m_formatter.oneByteRipOp64(OP_MOV_GvEv, 0, dst);
        JmpSrc label(m_formatter.size());
        spew("movq       .Lfrom%d(%%rip), %s", label.offset(), nameIReg(dst));
        return label;
    }
#endif
    void movl_rm(RegisterID src, const void* addr)
    {
        if (src == X86Registers::eax
#ifdef JS_CODEGEN_X64
            && !isAddressImmediate(addr)
#endif
            ) {
            movl_EAXm(addr);
            return;
        }

        spew("movl       %s, %p", nameIReg(4, src), addr);
        m_formatter.oneByteOp(OP_MOV_EvGv, addr, src);
    }

    void movl_i32m(int32_t imm, const void* addr)
    {
        spew("movl       $%d, %p", imm, addr);
        m_formatter.oneByteOp(OP_GROUP11_EvIz, addr, GROUP11_MOV);
        m_formatter.immediate32(imm);
    }

    void movb_rm(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("movb       %s, " MEM_ob, nameIReg(1, src), ADDR_ob(offset, base));
        m_formatter.oneByteOp8(OP_MOV_EbGv, offset, base, src);
    }

    void movb_rm_disp32(RegisterID src, int32_t offset, RegisterID base)
    {
        spew("movb       %s, " MEM_o32b, nameIReg(1, src), ADDR_o32b(offset, base));
        m_formatter.oneByteOp8_disp32(OP_MOV_EbGv, offset, base, src);
    }

    void movb_rm(RegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        spew("movb       %s, " MEM_obs, nameIReg(1, src), ADDR_obs(offset, base, index, scale));
        m_formatter.oneByteOp8(OP_MOV_EbGv, offset, base, index, scale, src);
    }

    void movb_rm(RegisterID src, const void* addr)
    {
        spew("movb       %s, %p", nameIReg(1, src), addr);
        m_formatter.oneByteOp8(OP_MOV_EbGv, addr, src);
    }

    void movb_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movb       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(1, dst));
        m_formatter.oneByteOp(OP_MOV_GvEb, offset, base, dst);
    }

    void movb_mr(int32_t offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        spew("movb       " MEM_obs ", %s", ADDR_obs(offset, base, index, scale), nameIReg(1, dst));
        m_formatter.oneByteOp(OP_MOV_GvEb, offset, base, index, scale, dst);
    }

    void movzbl_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movzbl     " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVZX_GvEb, offset, base, dst);
    }

    void movzbl_mr_disp32(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movzbl     " MEM_o32b ", %s", ADDR_o32b(offset, base), nameIReg(4, dst));
        m_formatter.twoByteOp_disp32(OP2_MOVZX_GvEb, offset, base, dst);
    }

    void movzbl_mr(int32_t offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        spew("movzbl     " MEM_obs ", %s", ADDR_obs(offset, base, index, scale), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVZX_GvEb, offset, base, index, scale, dst);
    }

    void movzbl_mr(const void* addr, RegisterID dst)
    {
        spew("movzbl     %p, %s", addr, nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVZX_GvEb, addr, dst);
    }

    void movsbl_rr(RegisterID src, RegisterID dst)
    {
        spew("movsbl     %s, %s", nameIReg(1,src), nameIReg(4,dst));
        m_formatter.twoByteOp8_movx(OP2_MOVSX_GvEb, src, dst);
    }

    void movsbl_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movsbl     " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVSX_GvEb, offset, base, dst);
    }

    void movsbl_mr_disp32(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movsbl     " MEM_o32b ", %s", ADDR_o32b(offset, base), nameIReg(4, dst));
        m_formatter.twoByteOp_disp32(OP2_MOVSX_GvEb, offset, base, dst);
    }

    void movsbl_mr(int32_t offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        spew("movsbl     " MEM_obs ", %s", ADDR_obs(offset, base, index, scale), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVSX_GvEb, offset, base, index, scale, dst);
    }

    void movsbl_mr(const void* addr, RegisterID dst)
    {
        spew("movsbl     %p, %s", addr, nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVSX_GvEb, addr, dst);
    }

    void movzwl_rr(RegisterID src, RegisterID dst)
    {
        spew("movzwl     %s, %s", nameIReg(2, src), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVZX_GvEw, src, dst);
    }

    void movzwl_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movzwl     " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVZX_GvEw, offset, base, dst);
    }

    void movzwl_mr_disp32(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movzwl     " MEM_o32b ", %s", ADDR_o32b(offset, base), nameIReg(4, dst));
        m_formatter.twoByteOp_disp32(OP2_MOVZX_GvEw, offset, base, dst);
    }

    void movzwl_mr(int32_t offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        spew("movzwl     " MEM_obs ", %s", ADDR_obs(offset, base, index, scale), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVZX_GvEw, offset, base, index, scale, dst);
    }

    void movzwl_mr(const void* addr, RegisterID dst)
    {
        spew("movzwl     %p, %s", addr, nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVZX_GvEw, addr, dst);
    }

    void movswl_rr(RegisterID src, RegisterID dst)
    {
        spew("movswl     %s, %s", nameIReg(2, src), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVSX_GvEw, src, dst);
    }

    void movswl_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movswl     " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVSX_GvEw, offset, base, dst);
    }

    void movswl_mr_disp32(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("movswl     " MEM_o32b ", %s", ADDR_o32b(offset, base), nameIReg(4, dst));
        m_formatter.twoByteOp_disp32(OP2_MOVSX_GvEw, offset, base, dst);
    }

    void movswl_mr(int32_t offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        spew("movswl     " MEM_obs ", %s", ADDR_obs(offset, base, index, scale), nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVSX_GvEw, offset, base, index, scale, dst);
    }

    void movswl_mr(const void* addr, RegisterID dst)
    {
        spew("movswl     %p, %s", addr, nameIReg(4, dst));
        m_formatter.twoByteOp(OP2_MOVSX_GvEw, addr, dst);
    }

    void movzbl_rr(RegisterID src, RegisterID dst)
    {
        spew("movzbl     %s, %s", nameIReg(1,src), nameIReg(4,dst));
        m_formatter.twoByteOp8_movx(OP2_MOVZX_GvEb, src, dst);
    }

    void leal_mr(int32_t offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        spew("leal       " MEM_obs ", %s", ADDR_obs(offset, base, index, scale), nameIReg(4, dst));
        m_formatter.oneByteOp(OP_LEA, offset, base, index, scale, dst);
    }

    void leal_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("leal       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(4,dst));
        m_formatter.oneByteOp(OP_LEA, offset, base, dst);
    }
#ifdef JS_CODEGEN_X64
    void leaq_mr(int32_t offset, RegisterID base, RegisterID dst)
    {
        spew("leaq       " MEM_ob ", %s", ADDR_ob(offset, base), nameIReg(8,dst));
        m_formatter.oneByteOp64(OP_LEA, offset, base, dst);
    }

    JmpSrc leaq_rip(RegisterID dst)
    {
        m_formatter.oneByteRipOp64(OP_LEA, 0, dst);
        JmpSrc label(m_formatter.size());
        spew("leaq       .Lfrom%d(%%rip), %s", label.offset(), nameIReg(dst));
        return label;
    }
#endif

    

    JmpSrc call()
    {
        m_formatter.oneByteOp(OP_CALL_rel32);
        JmpSrc r = m_formatter.immediateRel32();
        spew("call       .Lfrom%d", r.m_offset);
        return r;
    }

    JmpSrc call(RegisterID dst)
    {
        m_formatter.oneByteOp(OP_GROUP5_Ev, dst, GROUP5_OP_CALLN);
        JmpSrc r = JmpSrc(m_formatter.size());
        spew("call       *%s", nameIReg(dst));
        return r;
    }

    void call_m(int32_t offset, RegisterID base)
    {
        spew("call       *" MEM_ob, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_GROUP5_Ev, offset, base, GROUP5_OP_CALLN);
    }

    
    
    
    JmpSrc cmp_eax()
    {
        m_formatter.oneByteOp(OP_CMP_EAXIv);
        JmpSrc r = m_formatter.immediateRel32();
        spew("cmpl       %%eax, .Lfrom%d", r.m_offset);
        return r;
    }

    void jmp_i(JmpDst dst)
    {
        int32_t diff = dst.offset() - m_formatter.size();
        spew("jmp        .Llabel%d", dst.offset());

        
        
        
        if (CAN_SIGN_EXTEND_8_32(diff - 2)) {
            m_formatter.oneByteOp(OP_JMP_rel8);
            m_formatter.immediate8s(diff - 2);
        } else {
            m_formatter.oneByteOp(OP_JMP_rel32);
            m_formatter.immediate32(diff - 5);
        }
    }
    MOZ_WARN_UNUSED_RESULT JmpSrc
    jmp()
    {
        m_formatter.oneByteOp(OP_JMP_rel32);
        JmpSrc r = m_formatter.immediateRel32();
        spew("jmp        .Lfrom%d", r.m_offset);
        return r;
    }

    
    
    
    JmpSrc jmp_r(RegisterID dst)
    {
        spew("jmp        *%s", nameIReg(dst));
        m_formatter.oneByteOp(OP_GROUP5_Ev, dst, GROUP5_OP_JMPN);
        return JmpSrc(m_formatter.size());
    }

    void jmp_m(int32_t offset, RegisterID base)
    {
        spew("jmp        *" MEM_ob, ADDR_ob(offset, base));
        m_formatter.oneByteOp(OP_GROUP5_Ev, offset, base, GROUP5_OP_JMPN);
    }

    void jmp_m(int32_t offset, RegisterID base, RegisterID index, int scale) {
        spew("jmp        *" MEM_obs, ADDR_obs(offset, base, index, scale));
        m_formatter.oneByteOp(OP_GROUP5_Ev, offset, base, index, scale, GROUP5_OP_JMPN);
    }

#ifdef JS_CODEGEN_X64
    void jmp_rip(int ripOffset) {
        
        spew("jmp        *%d(%%rip)", ripOffset);
        m_formatter.oneByteRipOp(OP_GROUP5_Ev, ripOffset, GROUP5_OP_JMPN);
    }

    void immediate64(int64_t imm)
    {
        spew(".quad      %lld", (long long)imm);
        m_formatter.immediate64(imm);
    }
#endif

    void jCC_i(Condition cond, JmpDst dst)
    {
        int32_t diff = dst.offset() - m_formatter.size();
        spew("j%s        .Llabel%d", nameCC(cond), dst.offset());

        
        
        
        if (CAN_SIGN_EXTEND_8_32(diff - 2)) {
            m_formatter.oneByteOp(jccRel8(cond));
            m_formatter.immediate8s(diff - 2);
        } else {
            m_formatter.twoByteOp(jccRel32(cond));
            m_formatter.immediate32(diff - 6);
        }
    }

    MOZ_WARN_UNUSED_RESULT JmpSrc
    jCC(Condition cond)
    {
        m_formatter.twoByteOp(jccRel32(cond));
        JmpSrc r = m_formatter.immediateRel32();
        spew("j%s        .Lfrom%d", nameCC(cond), r.m_offset);
        return r;
    }

    

    void vpcmpeqw_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpcmpeqw", VEX_PD, OP2_PCMPEQW, src1, src0, dst);
    }

    void vpcmpeqd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpcmpeqd", VEX_PD, OP2_PCMPEQD_VdqWdq, src1, src0, dst);
    }
    void vpcmpeqd_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpcmpeqd", VEX_PD, OP2_PCMPEQD_VdqWdq, offset, base, src0, dst);
    }
    void vpcmpeqd_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpcmpeqd", VEX_PD, OP2_PCMPEQD_VdqWdq, address, src0, dst);
    }

    void vpcmpgtd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpcmpgtd", VEX_PD, OP2_PCMPGTD_VdqWdq, src1, src0, dst);
    }
    void vpcmpgtd_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpcmpgtd", VEX_PD, OP2_PCMPGTD_VdqWdq, offset, base, src0, dst);
    }
    void vpcmpgtd_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpcmpgtd", VEX_PD, OP2_PCMPGTD_VdqWdq, address, src0, dst);
    }

    void vcmpps_rr(uint8_t order, XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpImmSimd("vcmpps", VEX_PS, OP2_CMPPS_VpsWps, order, src1, src0, dst);
    }
    void vcmpps_mr(uint8_t order, int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpImmSimd("vcmpps", VEX_PS, OP2_CMPPS_VpsWps, order, offset, base, src0, dst);
    }
    void vcmpps_mr(uint8_t order, const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpImmSimd("vcmpps", VEX_PS, OP2_CMPPS_VpsWps, order, address, src0, dst);
    }

    void vrcpps_rr(XMMRegisterID src, XMMRegisterID dst) {
        twoByteOpSimd("vrcpps", VEX_PS, OP2_RCPPS_VpsWps, src, X86Registers::invalid_xmm, dst);
    }
    void vrcpps_mr(int32_t offset, RegisterID base, XMMRegisterID dst) {
        twoByteOpSimd("vrcpps", VEX_PS, OP2_RCPPS_VpsWps, offset, base, X86Registers::invalid_xmm, dst);
    }
    void vrcpps_mr(const void* address, XMMRegisterID dst) {
        twoByteOpSimd("vrcpps", VEX_PS, OP2_RCPPS_VpsWps, address, X86Registers::invalid_xmm, dst);
    }

    void vrsqrtps_rr(XMMRegisterID src, XMMRegisterID dst) {
        twoByteOpSimd("vrsqrtps", VEX_PS, OP2_RSQRTPS_VpsWps, src, X86Registers::invalid_xmm, dst);
    }
    void vrsqrtps_mr(int32_t offset, RegisterID base, XMMRegisterID dst) {
        twoByteOpSimd("vrsqrtps", VEX_PS, OP2_RSQRTPS_VpsWps, offset, base, X86Registers::invalid_xmm, dst);
    }
    void vrsqrtps_mr(const void* address, XMMRegisterID dst) {
        twoByteOpSimd("vrsqrtps", VEX_PS, OP2_RSQRTPS_VpsWps, address, X86Registers::invalid_xmm, dst);
    }

    void vsqrtps_rr(XMMRegisterID src, XMMRegisterID dst) {
        twoByteOpSimd("vsqrtps", VEX_PS, OP2_SQRTPS_VpsWps, src, X86Registers::invalid_xmm, dst);
    }
    void vsqrtps_mr(int32_t offset, RegisterID base, XMMRegisterID dst) {
        twoByteOpSimd("vsqrtps", VEX_PS, OP2_SQRTPS_VpsWps, offset, base, X86Registers::invalid_xmm, dst);
    }
    void vsqrtps_mr(const void* address, XMMRegisterID dst) {
        twoByteOpSimd("vsqrtps", VEX_PS, OP2_SQRTPS_VpsWps, address, X86Registers::invalid_xmm, dst);
    }

    void vaddsd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vaddsd", VEX_SD, OP2_ADDSD_VsdWsd, src1, src0, dst);
    }

    void vaddss_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vaddss", VEX_SS, OP2_ADDSD_VsdWsd, src1, src0, dst);
    }

    void vaddsd_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vaddsd", VEX_SD, OP2_ADDSD_VsdWsd, offset, base, src0, dst);
    }

    void vaddss_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vaddss", VEX_SS, OP2_ADDSD_VsdWsd, offset, base, src0, dst);
    }

    void vaddsd_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vaddsd", VEX_SD, OP2_ADDSD_VsdWsd, address, src0, dst);
    }
    void vaddss_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vaddss", VEX_SS, OP2_ADDSD_VsdWsd, address, src0, dst);
    }

    void vcvtss2sd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vcvtss2sd", VEX_SS, OP2_CVTSS2SD_VsdEd, src1, src0, dst);
    }

    void vcvtsd2ss_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vcvtsd2ss", VEX_SD, OP2_CVTSD2SS_VsdEd, src1, src0, dst);
    }

    void vcvtsi2ss_rr(RegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpInt32Simd("vcvtsi2ss", VEX_SS, OP2_CVTSI2SD_VsdEd, src1, src0, dst);
    }

    void vcvtsi2sd_rr(RegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpInt32Simd("vcvtsi2sd", VEX_SD, OP2_CVTSI2SD_VsdEd, src1, src0, dst);
    }

    void vcvttps2dq_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        twoByteOpSimd("vcvttps2dq", VEX_SS, OP2_CVTTPS2DQ_VdqWps, src, X86Registers::invalid_xmm, dst);
    }

    void vcvtdq2ps_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        twoByteOpSimd("vcvtdq2ps", VEX_PS, OP2_CVTDQ2PS_VpsWdq, src, X86Registers::invalid_xmm, dst);
    }

#ifdef JS_CODEGEN_X64
    void vcvtsq2sd_rr(RegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpInt64Simd("vcvtsi2sd", VEX_SD, OP2_CVTSI2SD_VsdEd, src1, src0, dst);
    }
    void vcvtsq2ss_rr(RegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpInt64Simd("vcvtsi2ss", VEX_SS, OP2_CVTSI2SD_VsdEd, src1, src0, dst);
    }
#endif

    void vcvtsi2sd_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vcvtsi2sd", VEX_SD, OP2_CVTSI2SD_VsdEd, offset, base, src0, dst);
    }

    void vcvtsi2sd_mr(int32_t offset, RegisterID base, RegisterID index, int scale, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vcvtsi2sd", VEX_SD, OP2_CVTSI2SD_VsdEd, offset, base, index, scale, src0, dst);
    }

    void vcvtsi2ss_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vcvtsi2ss", VEX_SS, OP2_CVTSI2SD_VsdEd, offset, base, src0, dst);
    }

    void vcvtsi2ss_mr(int32_t offset, RegisterID base, RegisterID index, int scale, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vcvtsi2ss", VEX_SS, OP2_CVTSI2SD_VsdEd, offset, base, index, scale, src0, dst);
    }

#ifdef JS_CODEGEN_X86
    void vcvtsi2sd_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vcvtsi2sd", VEX_SD, OP2_CVTSI2SD_VsdEd, address, src0, dst);
    }
#endif

    void vcvttsd2si_rr(XMMRegisterID src, RegisterID dst)
    {
        twoByteOpSimdInt32("vcvttsd2si", VEX_SD, OP2_CVTTSD2SI_GdWsd, src, dst);
    }

    void vcvttss2si_rr(XMMRegisterID src, RegisterID dst)
    {
        twoByteOpSimdInt32("vcvttss2si", VEX_SS, OP2_CVTTSD2SI_GdWsd, src, dst);
    }

#ifdef JS_CODEGEN_X64
    void vcvttsd2sq_rr(XMMRegisterID src, RegisterID dst)
    {
        twoByteOpSimdInt64("vcvttsd2si", VEX_SD, OP2_CVTTSD2SI_GdWsd, src, dst);
    }

    void vcvttss2sq_rr(XMMRegisterID src, RegisterID dst)
    {
        twoByteOpSimdInt64("vcvttss2si", VEX_SS, OP2_CVTTSD2SI_GdWsd, src, dst);
    }
#endif

    void vunpcklps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vunpcklps", VEX_PS, OP2_UNPCKLPS_VsdWsd, src1, src0, dst);
    }

    void vunpckhps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vunpckhps", VEX_PS, OP2_UNPCKHPS_VsdWsd, src1, src0, dst);
    }

    void vpand_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpand", VEX_PD, OP2_PANDDQ_VdqWdq, src1, src0, dst);
    }
    void vpand_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpand", VEX_PD, OP2_PANDDQ_VdqWdq, offset, base, src0, dst);
    }
    void vpand_mr(const void *address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpand", VEX_PD, OP2_PANDDQ_VdqWdq, address, src0, dst);
    }
    void vpor_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpor", VEX_PD, OP2_PORDQ_VdqWdq, src1, src0, dst);
    }
    void vpor_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpor", VEX_PD, OP2_PORDQ_VdqWdq, offset, base, src0, dst);
    }
    void vpor_mr(const void *address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpor", VEX_PD, OP2_PORDQ_VdqWdq, address, src0, dst);
    }
    void vpxor_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpxor", VEX_PD, OP2_PXORDQ_VdqWdq, src1, src0, dst);
    }
    void vpxor_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpxor", VEX_PD, OP2_PXORDQ_VdqWdq, offset, base, src0, dst);
    }
    void vpxor_mr(const void *address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpxor", VEX_PD, OP2_PXORDQ_VdqWdq, address, src0, dst);
    }
    void vpandn_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpandn", VEX_PD, OP2_PANDNDQ_VdqWdq, src1, src0, dst);
    }
    void vpandn_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpandn", VEX_PD, OP2_PANDNDQ_VdqWdq, offset, base, src0, dst);
    }
    void vpandn_mr(const void *address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpandn", VEX_PD, OP2_PANDNDQ_VdqWdq, address, src0, dst);
    }

    void vpshufd_irr(uint32_t mask, XMMRegisterID src, XMMRegisterID dst)
    {
        twoByteOpImmSimd("vpshufd", VEX_PD, OP2_PSHUFD_VdqWdqIb, mask, src, X86Registers::invalid_xmm, dst);
    }
    void vpshufd_imr(uint32_t mask, int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpImmSimd("vpshufd", VEX_PD, OP2_PSHUFD_VdqWdqIb, mask, offset, base, X86Registers::invalid_xmm, dst);
    }
    void vpshufd_imr(uint32_t mask, const void* address, XMMRegisterID dst)
    {
        twoByteOpImmSimd("vpshufd", VEX_PD, OP2_PSHUFD_VdqWdqIb, mask, address, X86Registers::invalid_xmm, dst);
    }

    void vshufps_irr(uint32_t mask, XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpImmSimd("vshufps", VEX_PS, OP2_SHUFPS_VpsWpsIb, mask, src1, src0, dst);
    }
    void vshufps_imr(uint32_t mask, int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpImmSimd("vshufps", VEX_PS, OP2_SHUFPS_VpsWpsIb, mask, offset, base, src0, dst);
    }
    void vshufps_imr(uint32_t mask, const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpImmSimd("vshufps", VEX_PS, OP2_SHUFPS_VpsWpsIb, mask, address, src0, dst);
    }

    void vmovhlps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovhlps", VEX_PS, OP2_MOVHLPS_VqUq, src1, src0, dst);
    }

    void vmovlhps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovlhps", VEX_PS, OP2_MOVLHPS_VqUq, src1, src0, dst);
    }

    void vpsrldq_ir(uint32_t count, XMMRegisterID src, XMMRegisterID dst)
    {
        MOZ_ASSERT(count < 16);
        shiftOpImmSimd("vpsrldq", OP2_PSRLDQ_Vd, Shift_vpsrldq, count, src, dst);
    }

    void vpsllq_ir(uint32_t count, XMMRegisterID src, XMMRegisterID dst)
    {
        MOZ_ASSERT(count < 64);
        shiftOpImmSimd("vpsllq", OP2_PSRLDQ_Vd, Shift_vpsllq, count, src, dst);
    }

    void vpsrlq_ir(uint32_t count, XMMRegisterID src, XMMRegisterID dst)
    {
        MOZ_ASSERT(count < 64);
        shiftOpImmSimd("vpsrlq", OP2_PSRLDQ_Vd, Shift_vpsrlq, count, src, dst);
    }

    void vpslld_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpslld", VEX_PD, OP2_PSLLD_VdqWdq, src1, src0, dst);
    }

    void vpslld_ir(uint32_t count, XMMRegisterID src, XMMRegisterID dst)
    {
        MOZ_ASSERT(count < 32);
        shiftOpImmSimd("vpslld", OP2_PSLLD_UdqIb, Shift_vpslld, count, src, dst);
    }

    void vpsrad_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpsrad", VEX_PD, OP2_PSRAD_VdqWdq, src1, src0, dst);
    }

    void vpsrad_ir(int32_t count, XMMRegisterID src, XMMRegisterID dst)
    {
        MOZ_ASSERT(count < 32);
        shiftOpImmSimd("vpsrad", OP2_PSRAD_UdqIb, Shift_vpsrad, count, src, dst);
    }

    void vpsrld_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vpsrld", VEX_PD, OP2_PSRLD_VdqWdq, src1, src0, dst);
    }

    void vpsrld_ir(uint32_t count, XMMRegisterID src, XMMRegisterID dst)
    {
        MOZ_ASSERT(count < 32);
        shiftOpImmSimd("vpsrld", OP2_PSRLD_UdqIb, Shift_vpsrld, count, src, dst);
    }

    void vmovmskpd_rr(XMMRegisterID src, RegisterID dst)
    {
        twoByteOpSimdInt32("vmovmskpd", VEX_PD, OP2_MOVMSKPD_EdVd, src, dst);
    }

    void vmovmskps_rr(XMMRegisterID src, RegisterID dst)
    {
        twoByteOpSimdInt32("vmovmskps", VEX_PS, OP2_MOVMSKPD_EdVd, src, dst);
    }

    void vptest_rr(XMMRegisterID rhs, XMMRegisterID lhs) {
        threeByteOpSimd("vptest", VEX_PD, OP3_PTEST_VdVd, ESCAPE_PTEST, rhs, X86Registers::invalid_xmm, lhs);
    }

    void vmovd_rr(XMMRegisterID src, RegisterID dst)
    {
        twoByteOpSimdInt32("vmovd", VEX_PD, OP2_MOVD_EdVd, (XMMRegisterID)dst, (RegisterID)src);
    }

    void vmovd_rr(RegisterID src, XMMRegisterID dst)
    {
        twoByteOpInt32Simd("vmovd", VEX_PD, OP2_MOVD_VdEd, src, X86Registers::invalid_xmm, dst);
    }

#ifdef JS_CODEGEN_X64
    void vmovq_rr(XMMRegisterID src, RegisterID dst)
    {
        twoByteOpSimdInt64("vmovq", VEX_PD, OP2_MOVD_EdVd, (XMMRegisterID)dst, (RegisterID)src);
    }

    void vmovq_rr(RegisterID src, XMMRegisterID dst)
    {
        twoByteOpInt64Simd("vmovq", VEX_PD, OP2_MOVD_VdEd, src, X86Registers::invalid_xmm, dst);
    }
#endif

    void vmovsd_rm(XMMRegisterID src, int32_t offset, RegisterID base)
    {
        twoByteOpSimd("vmovsd", VEX_SD, OP2_MOVSD_WsdVsd, offset, base, X86Registers::invalid_xmm, src);
    }

    void vmovsd_rm_disp32(XMMRegisterID src, int32_t offset, RegisterID base)
    {
        twoByteOpSimd_disp32("vmovsd", VEX_SD, OP2_MOVSD_WsdVsd, offset, base, X86Registers::invalid_xmm, src);
    }

    void vmovss_rm(XMMRegisterID src, int32_t offset, RegisterID base)
    {
        twoByteOpSimd("vmovss", VEX_SS, OP2_MOVSD_WsdVsd, offset, base, X86Registers::invalid_xmm, src);
    }

    void vmovss_rm_disp32(XMMRegisterID src, int32_t offset, RegisterID base)
    {
        twoByteOpSimd_disp32("vmovss", VEX_SS, OP2_MOVSD_WsdVsd, offset, base, X86Registers::invalid_xmm, src);
    }

    void vmovss_mr(int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovss", VEX_SS, OP2_MOVSD_VsdWsd, offset, base, X86Registers::invalid_xmm, dst);
    }

    void vmovss_mr_disp32(int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpSimd_disp32("vmovss", VEX_SS, OP2_MOVSD_VsdWsd, offset, base, X86Registers::invalid_xmm, dst);
    }

    void vmovsd_rm(XMMRegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        twoByteOpSimd("vmovsd", VEX_SD, OP2_MOVSD_WsdVsd, offset, base, index, scale, X86Registers::invalid_xmm, src);
    }

    void vmovss_rm(XMMRegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        twoByteOpSimd("vmovss", VEX_SS, OP2_MOVSD_WsdVsd, offset, base, index, scale, X86Registers::invalid_xmm, src);
    }

    void vmovss_mr(int32_t offset, RegisterID base, RegisterID index, int scale, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovss", VEX_SS, OP2_MOVSD_VsdWsd, offset, base, index, scale, X86Registers::invalid_xmm, dst);
    }

    void vmovsd_mr(int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovsd", VEX_SD, OP2_MOVSD_VsdWsd, offset, base, X86Registers::invalid_xmm, dst);
    }

    void vmovsd_mr_disp32(int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpSimd_disp32("vmovsd", VEX_SD, OP2_MOVSD_VsdWsd, offset, base, X86Registers::invalid_xmm, dst);
    }

    void vmovsd_mr(int32_t offset, RegisterID base, RegisterID index, int scale, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovsd", VEX_SD, OP2_MOVSD_VsdWsd, offset, base, index, scale, X86Registers::invalid_xmm, dst);
    }

    
    
    
    void vmovsd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovsd", VEX_SD, OP2_MOVSD_VsdWsd, src1, src0, dst);
    }

    
    
    void vmovss_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovss", VEX_SS, OP2_MOVSD_VsdWsd, src1, src0, dst);
    }

    void vmovsd_mr(const void* address, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovsd", VEX_SD, OP2_MOVSD_VsdWsd, address, X86Registers::invalid_xmm, dst);
    }

    void vmovss_mr(const void* address, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovss", VEX_SS, OP2_MOVSD_VsdWsd, address, X86Registers::invalid_xmm, dst);
    }

    void vmovups_mr(const void* address, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovups", VEX_PS, OP2_MOVPS_VpsWps, address, X86Registers::invalid_xmm, dst);
    }

    void vmovdqu_mr(const void* address, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovdqu", VEX_SS, OP2_MOVDQ_VdqWdq, address, X86Registers::invalid_xmm, dst);
    }

    void vmovsd_rm(XMMRegisterID src, const void* address)
    {
        twoByteOpSimd("vmovsd", VEX_SD, OP2_MOVSD_WsdVsd, address, X86Registers::invalid_xmm, src);
    }

    void vmovss_rm(XMMRegisterID src, const void* address)
    {
        twoByteOpSimd("vmovss", VEX_SS, OP2_MOVSD_WsdVsd, address, X86Registers::invalid_xmm, src);
    }

    void vmovdqa_rm(XMMRegisterID src, const void* address)
    {
        twoByteOpSimd("vmovdqa", VEX_PD, OP2_MOVDQ_WdqVdq, address, X86Registers::invalid_xmm, src);
    }

    void vmovaps_rm(XMMRegisterID src, const void* address)
    {
        twoByteOpSimd("vmovaps", VEX_PS, OP2_MOVAPS_WsdVsd, address, X86Registers::invalid_xmm, src);
    }

    void vmovdqu_rm(XMMRegisterID src, const void* address)
    {
        twoByteOpSimd("vmovdqu", VEX_SS, OP2_MOVDQ_WdqVdq, address, X86Registers::invalid_xmm, src);
    }

    void vmovups_rm(XMMRegisterID src, const void* address)
    {
        twoByteOpSimd("vmovups", VEX_PS, OP2_MOVPS_WpsVps, address, X86Registers::invalid_xmm, src);
    }
#ifdef JS_CODEGEN_X64
    JmpSrc vmovsd_ripr(XMMRegisterID dst)
    {
        return twoByteRipOpSimd("vmovsd", VEX_SD, OP2_MOVSD_VsdWsd, 0, X86Registers::invalid_xmm, dst);
    }
    JmpSrc vmovss_ripr(XMMRegisterID dst)
    {
        return twoByteRipOpSimd("vmovss", VEX_SS, OP2_MOVSD_VsdWsd, 0, X86Registers::invalid_xmm, dst);
    }
    JmpSrc vmovsd_rrip(XMMRegisterID src)
    {
        return twoByteRipOpSimd("vmovsd", VEX_SD, OP2_MOVSD_WsdVsd, 0, X86Registers::invalid_xmm, src);
    }
    JmpSrc vmovss_rrip(XMMRegisterID src)
    {
        return twoByteRipOpSimd("vmovss", VEX_SS, OP2_MOVSD_WsdVsd, 0, X86Registers::invalid_xmm, src);
    }
    JmpSrc vmovdqa_rrip(XMMRegisterID src)
    {
        return twoByteRipOpSimd("vmovdqa", VEX_PD, OP2_MOVDQ_WdqVdq, 0, X86Registers::invalid_xmm, src);
    }
    JmpSrc vmovaps_rrip(XMMRegisterID src)
    {
        return twoByteRipOpSimd("vmovdqa", VEX_PS, OP2_MOVAPS_WsdVsd, 0, X86Registers::invalid_xmm, src);
    }
#endif

    void vmovaps_rr(XMMRegisterID src, XMMRegisterID dst)
    {
#ifdef JS_CODEGEN_X64
        
        
        
        
        if (src >= X86Registers::xmm8 && dst < X86Registers::xmm8) {
            twoByteOpSimd("vmovaps", VEX_PS, OP2_MOVAPS_WsdVsd, dst, X86Registers::invalid_xmm, src);
            return;
        }
#endif
        twoByteOpSimd("vmovaps", VEX_PS, OP2_MOVAPS_VsdWsd, src, X86Registers::invalid_xmm, dst);
    }
    void vmovaps_rm(XMMRegisterID src, int32_t offset, RegisterID base)
    {
        twoByteOpSimd("vmovaps", VEX_PS, OP2_MOVAPS_WsdVsd, offset, base, X86Registers::invalid_xmm, src);
    }
    void vmovaps_rm(XMMRegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        twoByteOpSimd("vmovaps", VEX_PS, OP2_MOVAPS_WsdVsd, offset, base, index, scale, X86Registers::invalid_xmm, src);
    }
    void vmovaps_mr(int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovaps", VEX_PS, OP2_MOVAPS_VsdWsd, offset, base, X86Registers::invalid_xmm, dst);
    }
    void vmovaps_mr(int32_t offset, RegisterID base, RegisterID index, int scale, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovaps", VEX_PS, OP2_MOVAPS_VsdWsd, offset, base, index, scale, X86Registers::invalid_xmm, dst);
    }

    void vmovups_rm(XMMRegisterID src, int32_t offset, RegisterID base)
    {
        twoByteOpSimd("vmovups", VEX_PS, OP2_MOVPS_WpsVps, offset, base, X86Registers::invalid_xmm, src);
    }
    void vmovups_rm_disp32(XMMRegisterID src, int32_t offset, RegisterID base)
    {
        twoByteOpSimd_disp32("vmovups", VEX_PS, OP2_MOVPS_WpsVps, offset, base, X86Registers::invalid_xmm, src);
    }
    void vmovups_rm(XMMRegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        twoByteOpSimd("vmovups", VEX_PS, OP2_MOVPS_WpsVps, offset, base, index, scale, X86Registers::invalid_xmm, src);
    }
    void vmovups_mr(int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovups", VEX_PS, OP2_MOVPS_VpsWps, offset, base, X86Registers::invalid_xmm, dst);
    }
    void vmovups_mr_disp32(int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpSimd_disp32("vmovups", VEX_PS, OP2_MOVPS_VpsWps, offset, base, X86Registers::invalid_xmm, dst);
    }
    void vmovups_mr(int32_t offset, RegisterID base, RegisterID index, int scale, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovups", VEX_PS, OP2_MOVPS_VpsWps, offset, base, index, scale, X86Registers::invalid_xmm, dst);
    }

    void vmovapd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
#ifdef JS_CODEGEN_X64
        
        
        
        
        if (src >= X86Registers::xmm8 && dst < X86Registers::xmm8) {
            twoByteOpSimd("vmovapd", VEX_PD, OP2_MOVAPS_WsdVsd, dst, X86Registers::invalid_xmm, src);
            return;
        }
#endif
        twoByteOpSimd("vmovapd", VEX_PD, OP2_MOVAPD_VsdWsd, src, X86Registers::invalid_xmm, dst);
    }

#ifdef JS_CODEGEN_X64
    JmpSrc vmovaps_ripr(XMMRegisterID dst)
    {
        return twoByteRipOpSimd("vmovaps", VEX_PS, OP2_MOVAPS_VsdWsd, 0, X86Registers::invalid_xmm, dst);
    }

    JmpSrc vmovdqa_ripr(XMMRegisterID dst)
    {
        return twoByteRipOpSimd("vmovdqa", VEX_PD, OP2_MOVDQ_VdqWdq, 0, X86Registers::invalid_xmm, dst);
    }
#else
    void vmovaps_mr(const void* address, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovaps", VEX_PS, OP2_MOVAPS_VsdWsd, address, X86Registers::invalid_xmm, dst);
    }

    void vmovdqa_mr(const void* address, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovdqa", VEX_PD, OP2_MOVDQ_VdqWdq, address, X86Registers::invalid_xmm, dst);
    }
#endif 

    void vmovdqu_rm(XMMRegisterID src, int32_t offset, RegisterID base)
    {
        twoByteOpSimd("vmovdqu", VEX_SS, OP2_MOVDQ_WdqVdq, offset, base, X86Registers::invalid_xmm, src);
    }

    void vmovdqu_rm_disp32(XMMRegisterID src, int32_t offset, RegisterID base)
    {
        twoByteOpSimd_disp32("vmovdqu", VEX_SS, OP2_MOVDQ_WdqVdq, offset, base, X86Registers::invalid_xmm, src);
    }

    void vmovdqu_rm(XMMRegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        twoByteOpSimd("vmovdqu", VEX_SS, OP2_MOVDQ_WdqVdq, offset, base, index, scale, X86Registers::invalid_xmm, src);
    }

    void vmovdqu_mr(int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovdqu", VEX_SS, OP2_MOVDQ_VdqWdq, offset, base, X86Registers::invalid_xmm, dst);
    }

    void vmovdqu_mr_disp32(int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpSimd_disp32("vmovdqu", VEX_SS, OP2_MOVDQ_VdqWdq, offset, base, X86Registers::invalid_xmm, dst);
    }

    void vmovdqu_mr(int32_t offset, RegisterID base, RegisterID index, int scale, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovdqu", VEX_SS, OP2_MOVDQ_VdqWdq, offset, base, index, scale, X86Registers::invalid_xmm, dst);
    }

    void vmovdqa_rr(XMMRegisterID src, XMMRegisterID dst)
    {
#ifdef JS_CODEGEN_X64
        
        
        
        
        if (src >= X86Registers::xmm8 && dst < X86Registers::xmm8) {
            twoByteOpSimd("vmovdqa", VEX_PD, OP2_MOVDQ_WdqVdq, dst, X86Registers::invalid_xmm, src);
            return;
        }
#endif
        twoByteOpSimd("vmovdqa", VEX_PD, OP2_MOVDQ_VdqWdq, src, X86Registers::invalid_xmm, dst);
    }

    void vmovdqa_rm(XMMRegisterID src, int32_t offset, RegisterID base)
    {
        twoByteOpSimd("vmovdqa", VEX_PD, OP2_MOVDQ_WdqVdq, offset, base, X86Registers::invalid_xmm, src);
    }

    void vmovdqa_rm(XMMRegisterID src, int32_t offset, RegisterID base, RegisterID index, int scale)
    {
        twoByteOpSimd("vmovdqa", VEX_PD, OP2_MOVDQ_WdqVdq, offset, base, index, scale, X86Registers::invalid_xmm, src);
    }

    void vmovdqa_mr(int32_t offset, RegisterID base, XMMRegisterID dst)
    {

        twoByteOpSimd("vmovdqa", VEX_PD, OP2_MOVDQ_VdqWdq, offset, base, X86Registers::invalid_xmm, dst);
    }

    void vmovdqa_mr(int32_t offset, RegisterID base, RegisterID index, int scale, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovdqa", VEX_PD, OP2_MOVDQ_VdqWdq, offset, base, index, scale, X86Registers::invalid_xmm, dst);
    }

    void vmulsd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmulsd", VEX_SD, OP2_MULSD_VsdWsd, src1, src0, dst);
    }

    void vmulss_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmulss", VEX_SS, OP2_MULSD_VsdWsd, src1, src0, dst);
    }

    void vmulsd_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmulsd", VEX_SD, OP2_MULSD_VsdWsd, offset, base, src0, dst);
    }

    void vmulss_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmulss", VEX_SS, OP2_MULSD_VsdWsd, offset, base, src0, dst);
    }

    void vpextrw_irr(uint32_t whichWord, XMMRegisterID src, RegisterID dst)
    {
        MOZ_ASSERT(whichWord < 8);
        twoByteOpImmSimdInt32("vpextrw", VEX_PD, OP2_PEXTRW_GdUdIb, whichWord, src, dst);
    }

    void vsubsd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vsubsd", VEX_SD, OP2_SUBSD_VsdWsd, src1, src0, dst);
    }

    void vsubss_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vsubss", VEX_SS, OP2_SUBSD_VsdWsd, src1, src0, dst);
    }

    void vsubsd_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vsubsd", VEX_SD, OP2_SUBSD_VsdWsd, offset, base, src0, dst);
    }

    void vsubss_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vsubss", VEX_SS, OP2_SUBSD_VsdWsd, offset, base, src0, dst);
    }

    void vucomiss_rr(XMMRegisterID rhs, XMMRegisterID lhs)
    {
        twoByteOpSimdFlags("vucomiss", VEX_PS, OP2_UCOMISD_VsdWsd, rhs, lhs);
    }

    void vucomisd_rr(XMMRegisterID rhs, XMMRegisterID lhs)
    {
        twoByteOpSimdFlags("vucomisd", VEX_PD, OP2_UCOMISD_VsdWsd, rhs, lhs);
    }

    void vucomisd_mr(int32_t offset, RegisterID base, XMMRegisterID lhs)
    {
        twoByteOpSimdFlags("vucomisd", VEX_PD, OP2_UCOMISD_VsdWsd, offset, base, lhs);
    }

    void vdivsd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vdivsd", VEX_SD, OP2_DIVSD_VsdWsd, src1, src0, dst);
    }

    void vdivss_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vdivss", VEX_SS, OP2_DIVSD_VsdWsd, src1, src0, dst);
    }

    void vdivsd_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vdivsd", VEX_SD, OP2_DIVSD_VsdWsd, offset, base, src0, dst);
    }

    void vdivss_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vdivss", VEX_SS, OP2_DIVSD_VsdWsd, offset, base, src0, dst);
    }

    void vxorpd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vxorpd", VEX_PD, OP2_XORPD_VpdWpd, src1, src0, dst);
    }

    void vorpd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vorpd", VEX_PD, OP2_ORPD_VpdWpd, src1, src0, dst);
    }

    void vandpd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vandpd", VEX_PD, OP2_ANDPD_VpdWpd, src1, src0, dst);
    }

    void vandps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vandps", VEX_PS, OP2_ANDPS_VpsWps, src1, src0, dst);
    }

    void vandps_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vandps", VEX_PS, OP2_ANDPS_VpsWps, offset, base, src0, dst);
    }

    void vandps_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vandps", VEX_PS, OP2_ANDPS_VpsWps, address, src0, dst);
    }

    void vandnps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vandnps", VEX_PS, OP2_ANDNPS_VpsWps, src1, src0, dst);
    }

    void vandnps_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vandnps", VEX_PS, OP2_ANDNPS_VpsWps, offset, base, src0, dst);
    }

    void vandnps_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vandnps", VEX_PS, OP2_ANDNPS_VpsWps, address, src0, dst);
    }

    void vorps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vorps", VEX_PS, OP2_ORPS_VpsWps, src1, src0, dst);
    }

    void vorps_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vorps", VEX_PS, OP2_ORPS_VpsWps, offset, base, src0, dst);
    }

    void vorps_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vorps", VEX_PS, OP2_ORPS_VpsWps, address, src0, dst);
    }

    void vxorps_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vxorps", VEX_PS, OP2_XORPS_VpsWps, src1, src0, dst);
    }

    void vxorps_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vxorps", VEX_PS, OP2_XORPS_VpsWps, offset, base, src0, dst);
    }

    void vxorps_mr(const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vxorps", VEX_PS, OP2_XORPS_VpsWps, address, src0, dst);
    }

    void vsqrtsd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vsqrtsd", VEX_SD, OP2_SQRTSD_VsdWsd, src1, src0, dst);
    }

    void vsqrtss_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vsqrtss", VEX_SS, OP2_SQRTSS_VssWss, src1, src0, dst);
    }

    void vroundsd_irr(RoundingMode mode, XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        threeByteOpImmSimd("vroundsd", VEX_PD, OP3_ROUNDSD_VsdWsd, ESCAPE_ROUNDSD, mode, src1, src0, dst);
    }

    void vroundss_irr(RoundingMode mode, XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        threeByteOpImmSimd("vroundss", VEX_PD, OP3_ROUNDSS_VsdWsd, ESCAPE_ROUNDSD, mode, src1, src0, dst);
    }

    void vinsertps_irr(uint32_t mask, XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        threeByteOpImmSimd("vinsertps", VEX_PD, OP3_INSERTPS_VpsUps, ESCAPE_INSERTPS, mask, src1, src0, dst);
    }

    void vpinsrd_irr(unsigned lane, RegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        MOZ_ASSERT(lane < 4);
        threeByteOpImmInt32Simd("vpinsrd", VEX_PD, OP3_PINSRD_VdqEdIb, ESCAPE_PINSRD, lane, src1, src0, dst);
    }

    void vpinsrd_imr(unsigned lane, int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        MOZ_ASSERT(lane < 4);
        threeByteOpImmInt32Simd("vpinsrd", VEX_PD, OP3_PINSRD_VdqEdIb, ESCAPE_PINSRD, lane, offset, base, src0, dst);
    }

    void vpextrd_irr(unsigned lane, XMMRegisterID src, RegisterID dst)
    {
        MOZ_ASSERT(lane < 4);
        threeByteOpImmSimdInt32("vpextrd", VEX_PD, OP3_PEXTRD_EdVdqIb, ESCAPE_PEXTRD, lane, (XMMRegisterID)dst, (RegisterID)src);
    }

    void vpextrd_irm(unsigned lane, XMMRegisterID src, int32_t offset, RegisterID base)
    {
        MOZ_ASSERT(lane < 4);
        spew("pextrd     $0x%x, %s, " MEM_ob, lane, nameFPReg(src), ADDR_ob(offset, base));
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.threeByteOp(OP3_PEXTRD_EdVdqIb, ESCAPE_PEXTRD, offset, base, (RegisterID)src);
        m_formatter.immediate8u(lane);
    }

    void vblendps_irr(unsigned imm, XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        MOZ_ASSERT(imm < 16);
        
        threeByteOpImmSimd("vblendps", VEX_PD, OP3_BLENDPS_VpsWpsIb, ESCAPE_BLENDPS, imm, src1, src0, dst);
    }

    void vblendps_imr(unsigned imm, int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        MOZ_ASSERT(imm < 16);
        
threeByteOpImmSimd("vblendps", VEX_PD, OP3_BLENDPS_VpsWpsIb, ESCAPE_BLENDPS, imm, offset, base, src0, dst);
    }

    void vblendvps_rr(XMMRegisterID mask, XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst) {
        vblendvOpSimd(mask, src1, src0, dst);
    }
    void vblendvps_mr(XMMRegisterID mask, int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst) {
        vblendvOpSimd(mask, offset, base, src0, dst);
    }

    void vmovsldup_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovsldup", VEX_SS, OP2_MOVSLDUP_VpsWps, src, X86Registers::invalid_xmm, dst);
    }
    void vmovsldup_mr(int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovsldup", VEX_SS, OP2_MOVSLDUP_VpsWps, offset, base, X86Registers::invalid_xmm, dst);
    }

    void vmovshdup_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovshdup", VEX_SS, OP2_MOVSHDUP_VpsWps, src, X86Registers::invalid_xmm, dst);
    }
    void vmovshdup_mr(int32_t offset, RegisterID base, XMMRegisterID dst)
    {
        twoByteOpSimd("vmovshdup", VEX_SS, OP2_MOVSHDUP_VpsWps, offset, base, X86Registers::invalid_xmm, dst);
    }

    void vminsd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vminsd", VEX_SD, OP2_MINSD_VsdWsd, src1, src0, dst);
    }
    void vminsd_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vminsd", VEX_SD, OP2_MINSD_VsdWsd, offset, base, src0, dst);
    }

    void vminss_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vminss", VEX_SS, OP2_MINSS_VssWss, src1, src0, dst);
    }

    void vmaxsd_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmaxsd", VEX_SD, OP2_MAXSD_VsdWsd, src1, src0, dst);
    }
    void vmaxsd_mr(int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmaxsd", VEX_SD, OP2_MAXSD_VsdWsd, offset, base, src0, dst);
    }

    void vmaxss_rr(XMMRegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        twoByteOpSimd("vmaxss", VEX_SS, OP2_MAXSS_VssWss, src1, src0, dst);
    }

    

    void int3()
    {
        spew("int3");
        m_formatter.oneByteOp(OP_INT3);
    }

    void ud2()
    {
        spew("ud2");
        m_formatter.twoByteOp(OP2_UD2);
    }

    void ret()
    {
        spew("ret");
        m_formatter.oneByteOp(OP_RET);
    }

    void ret_i(int32_t imm)
    {
        spew("ret        $%d", imm);
        m_formatter.oneByteOp(OP_RET_Iz);
        m_formatter.immediate16u(imm);
    }

    void predictNotTaken()
    {
        FIXME_INSN_PRINTING;
        m_formatter.prefix(PRE_PREDICT_BRANCH_NOT_TAKEN);
    }

#ifdef JS_CODEGEN_X86
    void pusha()
    {
        spew("pusha");
        m_formatter.oneByteOp(OP_PUSHA);
    }

    void popa()
    {
        spew("popa");
        m_formatter.oneByteOp(OP_POPA);
    }
#endif

    void mfence() {
        spew("mfence");
        m_formatter.twoByteOp(OP_FENCE, (RegisterID)0, 6);
    }

    

    JmpDst label()
    {
        JmpDst r = JmpDst(m_formatter.size());
        spew(".set .Llabel%d, .", r.m_offset);
        return r;
    }

    size_t currentOffset() const {
        return m_formatter.size();
    }

    static JmpDst labelFor(JmpSrc jump, intptr_t offset = 0)
    {
        return JmpDst(jump.m_offset + offset);
    }

    JmpDst align(int alignment)
    {
        spew(".balign %d, 0x%x   # hlt", alignment, OP_HLT);
        while (!m_formatter.isAligned(alignment))
            m_formatter.oneByteOp(OP_HLT);

        return label();
    }

    void jumpTablePointer(uintptr_t ptr)
    {
        spew("#jumpTablePointer %llu", (unsigned long long)ptr);
        m_formatter.jumpTablePointer(ptr);
    }

    void doubleConstant(double d)
    {
        spew(".double %.20f", d);
        m_formatter.doubleConstant(d);
    }
    void floatConstant(float f)
    {
        spew(".float %.20f", f);
        m_formatter.floatConstant(f);
    }

    void int32x4Constant(const int32_t s[4])
    {
        spew(".int %d,%d,%d,%d", s[0], s[1], s[2], s[3]);
        MOZ_ASSERT(m_formatter.isAligned(16));
        m_formatter.int32x4Constant(s);
    }
    void float32x4Constant(const float f[4])
    {
        spew(".float %f,%f,%f,%f", f[0], f[1], f[2], f[3]);
        MOZ_ASSERT(m_formatter.isAligned(16));
        m_formatter.float32x4Constant(f);
    }

    void int64Constant(int64_t i)
    {
        spew(".quad %lld", (long long)i);
        m_formatter.int64Constant(i);
    }

    
    
    
    
    
    
    
    

    
    
    
    bool nextJump(const JmpSrc& from, JmpSrc* next)
    {
        
        
        if (oom())
            return false;

        char* code = reinterpret_cast<char*>(m_formatter.data());
        int32_t offset = getInt32(code + from.m_offset);
        if (offset == -1)
            return false;
        *next = JmpSrc(offset);
        return true;
    }
    void setNextJump(const JmpSrc& from, const JmpSrc &to)
    {
        
        
        if (oom())
            return;

        char* code = reinterpret_cast<char*>(m_formatter.data());
        setInt32(code + from.m_offset, to.m_offset);
    }

    void linkJump(JmpSrc from, JmpDst to)
    {
        MOZ_ASSERT(from.m_offset != -1);
        MOZ_ASSERT(to.m_offset != -1);

        
        
        if (oom())
            return;

        spew("##link     ((%d)) jumps to ((%d))", from.m_offset, to.m_offset);
        char* code = reinterpret_cast<char*>(m_formatter.data());
        setRel32(code + from.m_offset, code + to.m_offset);
    }

    static void linkJump(void* code, JmpSrc from, void* to)
    {
        MOZ_ASSERT(from.m_offset != -1);

        staticSpew("##link     ((%d)) jumps to ((%p))", from.m_offset, to);
        setRel32(reinterpret_cast<char*>(code) + from.m_offset, to);
    }

    static void linkCall(void* code, JmpSrc from, void* to)
    {
        MOZ_ASSERT(from.m_offset != -1);

        staticSpew("##linkCall");
        setRel32(reinterpret_cast<char*>(code) + from.m_offset, to);
    }

    static void linkPointer(void* code, JmpDst where, void* value)
    {
        MOZ_ASSERT(where.m_offset != -1);

        staticSpew("##linkPointer");
        setPointer(reinterpret_cast<char*>(code) + where.m_offset, value);
    }

    static void relinkJump(void* from, void* to)
    {
        staticSpew("##relinkJump ((from=%p)) ((to=%p))", from, to);
        setRel32(from, to);
    }

    static bool canRelinkJump(void* from, void* to)
    {
        intptr_t offset = reinterpret_cast<intptr_t>(to) - reinterpret_cast<intptr_t>(from);
        return (offset == static_cast<int32_t>(offset));
    }

    static void relinkCall(void* from, void* to)
    {
        staticSpew("##relinkCall ((from=%p)) ((to=%p))", from, to);
        setRel32(from, to);
    }

    static void repatchInt32(void* where, int32_t value)
    {
        staticSpew("##relinkInt32 ((where=%p)) ((value=%d))", where, value);
        setInt32(where, value);
    }

    static void repatchPointer(void* where, const void* value)
    {
        staticSpew("##repatchPtr ((where=%p)) ((value=%p))", where, value);
        setPointer(where, value);
    }

    static void repatchLoadPtrToLEA(void* where)
    {
        staticSpew("##repatchLoadPtrToLEA ((where=%p))", where);

#ifdef JS_CODEGEN_X64
        
        
        where = reinterpret_cast<char*>(where) + 1;
#endif
        *reinterpret_cast<unsigned char*>(where) = static_cast<unsigned char>(OP_LEA);
    }

    static void repatchLEAToLoadPtr(void* where)
    {
        staticSpew("##repatchLEAToLoadPtr ((where=%p))", where);
#ifdef JS_CODEGEN_X64
        
        
        where = reinterpret_cast<char*>(where) + 1;
#endif
        *reinterpret_cast<unsigned char*>(where) = static_cast<unsigned char>(OP_MOV_GvEv);
    }

    static unsigned getCallReturnOffset(JmpSrc call)
    {
        MOZ_ASSERT(call.m_offset >= 0);
        return call.m_offset;
    }

    static void* getRelocatedAddress(void* code, JmpSrc jump)
    {
        MOZ_ASSERT(jump.m_offset != -1);

        return reinterpret_cast<void*>(reinterpret_cast<ptrdiff_t>(code) + jump.m_offset);
    }

    static void* getRelocatedAddress(void* code, JmpDst destination)
    {
        MOZ_ASSERT(destination.m_offset != -1);

        return reinterpret_cast<void*>(reinterpret_cast<ptrdiff_t>(code) + destination.m_offset);
    }

    static int getDifferenceBetweenLabels(JmpDst src, JmpDst dst)
    {
        return dst.m_offset - src.m_offset;
    }

    static int getDifferenceBetweenLabels(JmpDst src, JmpSrc dst)
    {
        return dst.m_offset - src.m_offset;
    }

    static int getDifferenceBetweenLabels(JmpSrc src, JmpDst dst)
    {
        return dst.m_offset - src.m_offset;
    }

    void* executableAllocAndCopy(js::jit::ExecutableAllocator* allocator,
                                 js::jit::ExecutablePool **poolp, js::jit::CodeKind kind)
    {
        return m_formatter.executableAllocAndCopy(allocator, poolp, kind);
    }

    void executableCopy(void* buffer)
    {
        memcpy(buffer, m_formatter.buffer(), size());
    }

    static void setRel32(void* from, void* to)
    {
        intptr_t offset = reinterpret_cast<intptr_t>(to) - reinterpret_cast<intptr_t>(from);
        MOZ_ASSERT(offset == static_cast<int32_t>(offset),
                   "offset is too great for a 32-bit relocation");
        if (offset != static_cast<int32_t>(offset))
            MOZ_CRASH("offset is too great for a 32-bit relocation");

        staticSpew("##setRel32 ((from=%p)) ((to=%p))", from, to);
        setInt32(from, offset);
    }

    static void *getRel32Target(void* where)
    {
        int32_t rel = getInt32(where);
        return (char *)where + rel;
    }

    static void *getPointer(void* where)
    {
        return reinterpret_cast<void **>(where)[-1];
    }

    static void **getPointerRef(void* where)
    {
        return &reinterpret_cast<void **>(where)[-1];
    }

    static void setPointer(void* where, const void* value)
    {
        staticSpew("##setPtr     ((where=%p)) ((value=%p))", where, value);
        reinterpret_cast<const void**>(where)[-1] = value;
    }

    
    
    
    static bool isAddressImmediate(const void *address) {
        intptr_t value = reinterpret_cast<intptr_t>(address);
        int32_t immediate = static_cast<int32_t>(value);
        return value == immediate;
    }

    
    
    
    static int32_t addressImmediate(const void *address) {
#ifdef JS_CODEGEN_X64
        
        MOZ_ASSERT(isAddressImmediate(address));
#endif
        return static_cast<int32_t>(reinterpret_cast<intptr_t>(address));
    }

    static void setInt32(void* where, int32_t value)
    {
        reinterpret_cast<int32_t*>(where)[-1] = value;
    }

private:
    
    

    bool useLegacySSEEncoding(XMMRegisterID src0, XMMRegisterID dst)
    {
        
        if (!useVEX_) {
            MOZ_ASSERT(src0 == X86Registers::invalid_xmm || src0 == dst,
                       "Legacy SSE (pre-AVX) encoding requires the output register to be "
                       "the same as the src0 input register");
            return true;
        }

        
        
        
        return src0 == dst;
    }

    bool useLegacySSEEncodingForVblendv(XMMRegisterID mask, XMMRegisterID src0, XMMRegisterID dst)
    {
        
        

        if (!useVEX_) {
            MOZ_ASSERT(src0 == dst,
                       "Legacy SSE (pre-AVX) encoding requires the output register to be "
                       "the same as the src0 input register");
            MOZ_ASSERT(mask == X86Registers::xmm0,
                       "Legacy SSE (pre-AVX) encoding for blendv requires the mask to be "
                       "in xmm0");
            return true;
        }

        return src0 == dst && mask == X86Registers::xmm0;
    }

    bool useLegacySSEEncodingForOtherOutput()
    {
        return !useVEX_;
    }

    const char *legacySSEOpName(const char *name)
    {
        MOZ_ASSERT(name[0] == 'v');
        return name + 1;
    }

 #ifdef JS_CODEGEN_X64
    JmpSrc twoByteRipOpSimd(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                            int ripOffset, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteRipOp(opcode, ripOffset, dst);
            JmpSrc label(m_formatter.size());
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, .Lfrom%d%+d(%%rip)", legacySSEOpName(name), nameFPReg(dst), label.offset(), ripOffset);
            else
                spew("%-11s.Lfrom%d%+d(%%rip), %s", legacySSEOpName(name), label.offset(), ripOffset, nameFPReg(dst));
            return label;
        }

        m_formatter.twoByteRipOpVex(ty, opcode, ripOffset, src0, dst);
        JmpSrc label(m_formatter.size());
        if (src0 == X86Registers::invalid_xmm) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, .Lfrom%d%+d(%%rip)", name, nameFPReg(dst), label.offset(), ripOffset);
            else
                spew("%-11s.Lfrom%d%+d(%%rip), %s", name, label.offset(), ripOffset, nameFPReg(dst));
        } else {
            spew("%-11s.Lfrom%d%+d(%%rip), %s, %s", name, label.offset(), ripOffset, nameFPReg(src0), nameFPReg(dst));
        }
        return label;
    }
#endif

    void twoByteOpSimd(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                       XMMRegisterID rm, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, %s", legacySSEOpName(name), nameFPReg(dst), nameFPReg(rm));
            else
                spew("%-11s%s, %s", legacySSEOpName(name), nameFPReg(rm), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, (RegisterID)rm, dst);
            return;
        }

        if (src0 == X86Registers::invalid_xmm) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, %s", name, nameFPReg(dst), nameFPReg(rm));
            else
                spew("%-11s%s, %s", name, nameFPReg(rm), nameFPReg(dst));
        } else {
            spew("%-11s%s, %s, %s", name, nameFPReg(rm), nameFPReg(src0), nameFPReg(dst));
        }
        m_formatter.twoByteOpVex(ty, opcode, (RegisterID)rm, src0, dst);
    }

    void twoByteOpImmSimd(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                          uint32_t imm, XMMRegisterID rm, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            spew("%-11s$0x%x, %s, %s", legacySSEOpName(name), imm, nameFPReg(rm), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, (RegisterID)rm, dst);
            m_formatter.immediate8u(imm);
            return;
        }

        if (src0 == X86Registers::invalid_xmm)
            spew("%-11s$0x%x, %s, %s", name, imm, nameFPReg(rm), nameFPReg(dst));
        else
            spew("%-11s$0x%x, %s, %s, %s", name, imm, nameFPReg(rm), nameFPReg(src0), nameFPReg(dst));
        m_formatter.twoByteOpVex(ty, opcode, (RegisterID)rm, src0, dst);
        m_formatter.immediate8u(imm);
    }

    void twoByteOpSimd(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                       int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            if (IsXMMReversedOperands(opcode)) {
                spew("%-11s%s, " MEM_ob, legacySSEOpName(name),
                     nameFPReg(dst), ADDR_ob(offset, base));
            } else {
                spew("%-11s" MEM_ob ", %s", legacySSEOpName(name),
                     ADDR_ob(offset, base), nameFPReg(dst));
            }
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, offset, base, dst);
            return;
        }

        if (src0 == X86Registers::invalid_xmm) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, " MEM_ob, name, nameFPReg(dst), ADDR_ob(offset, base));
            else
                spew("%-11s" MEM_ob ", %s", name, ADDR_ob(offset, base), nameFPReg(dst));
        } else {
            spew("%-11s" MEM_ob ", %s, %s", name,
                 ADDR_ob(offset, base), nameFPReg(src0), nameFPReg(dst));
        }
        m_formatter.twoByteOpVex(ty, opcode, offset, base, src0, dst);
    }

    void twoByteOpSimd_disp32(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                              int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, " MEM_o32b, legacySSEOpName(name), nameFPReg(dst), ADDR_o32b(offset, base));
            else
                spew("%-11s" MEM_o32b ", %s", legacySSEOpName(name), ADDR_o32b(offset, base), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp_disp32(opcode, offset, base, dst);
            return;
        }

        if (src0 == X86Registers::invalid_xmm) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, " MEM_o32b, name, nameFPReg(dst), ADDR_o32b(offset, base));
            else
                spew("%-11s" MEM_o32b ", %s", name, ADDR_o32b(offset, base), nameFPReg(dst));
        } else {
            spew("%-11s" MEM_o32b ", %s, %s", name,
                 ADDR_o32b(offset, base), nameFPReg(src0), nameFPReg(dst));
        }
        m_formatter.twoByteOpVex_disp32(ty, opcode, offset, base, src0, dst);
    }

    void twoByteOpImmSimd(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                          uint32_t imm, int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            spew("%-11s$0x%x, " MEM_ob ", %s", legacySSEOpName(name), imm,
                 ADDR_ob(offset, base), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, offset, base, dst);
            m_formatter.immediate8u(imm);
            return;
        }

        spew("%-11s$0x%x, " MEM_ob ", %s, %s", name, imm, ADDR_ob(offset, base),
             nameFPReg(src0), nameFPReg(dst));
        m_formatter.twoByteOpVex(ty, opcode, offset, base, src0, dst);
        m_formatter.immediate8u(imm);
    }

    void twoByteOpSimd(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                       int32_t offset, RegisterID base, RegisterID index, int scale,
                       XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            if (IsXMMReversedOperands(opcode)) {
                spew("%-11s%s, " MEM_obs, legacySSEOpName(name),
                     nameFPReg(dst), ADDR_obs(offset, base, index, scale));
            } else {
                spew("%-11s" MEM_obs ", %s", legacySSEOpName(name),
                     ADDR_obs(offset, base, index, scale), nameFPReg(dst));
            }
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, offset, base, index, scale, dst);
            return;
        }

        if (src0 == X86Registers::invalid_xmm) {
            if (IsXMMReversedOperands(opcode)) {
                spew("%-11s%s, " MEM_obs, name, nameFPReg(dst),
                     ADDR_obs(offset, base, index, scale));
            } else {
                spew("%-11s" MEM_obs ", %s", name, ADDR_obs(offset, base, index, scale),
                     nameFPReg(dst));
            }
        } else {
            spew("%-11s" MEM_obs ", %s, %s", name, ADDR_obs(offset, base, index, scale),
                 nameFPReg(src0), nameFPReg(dst));
        }
        m_formatter.twoByteOpVex(ty, opcode, offset, base, index, scale, src0, dst);
    }

    void twoByteOpSimd(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                       const void* address, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, %p", legacySSEOpName(name), nameFPReg(dst), address);
            else
                spew("%-11s%p, %s", legacySSEOpName(name), address, nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, address, dst);
            return;
        }

        if (src0 == X86Registers::invalid_xmm) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, %p", name, nameFPReg(dst), address);
            else
                spew("%-11s%p, %s", name, address, nameFPReg(dst));
        } else {
            spew("%-11s%p, %s, %s", name, address, nameFPReg(src0), nameFPReg(dst));
        }
        m_formatter.twoByteOpVex(ty, opcode, address, src0, dst);
    }

    void twoByteOpImmSimd(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                          uint32_t imm, const void *address, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            spew("%-11s$0x%x, %p, %s", legacySSEOpName(name), imm, address, nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, address, dst);
            m_formatter.immediate8u(imm);
            return;
        }

        spew("%-11s$0x%x, %p, %s, %s", name, imm, address, nameFPReg(src0), nameFPReg(dst));
        m_formatter.twoByteOpVex(ty, opcode, address, src0, dst);
        m_formatter.immediate8u(imm);
    }

    void twoByteOpInt32Simd(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                            RegisterID rm, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, %s", legacySSEOpName(name), nameFPReg(dst), nameIReg(4, rm));
            else
                spew("%-11s%s, %s", legacySSEOpName(name), nameIReg(4, rm), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, rm, dst);
            return;
        }

        if (src0 == X86Registers::invalid_xmm) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, %s", name, nameFPReg(dst), nameIReg(4, rm));
            else
                spew("%-11s%s, %s", name, nameIReg(4, rm), nameFPReg(dst));
        } else {
            spew("%-11s%s, %s, %s", name, nameIReg(4, rm), nameFPReg(src0), nameFPReg(dst));
        }
        m_formatter.twoByteOpVex(ty, opcode, rm, src0, dst);
    }

#ifdef JS_CODEGEN_X64
    void twoByteOpInt64Simd(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                            RegisterID rm, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, %s", legacySSEOpName(name), nameFPReg(dst), nameIReg(rm));
            else
                spew("%-11s%s, %s", legacySSEOpName(name), nameIReg(rm), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp64(opcode, rm, dst);
            return;
        }

        if (src0 == X86Registers::invalid_xmm) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, %s", name, nameFPReg(dst), nameIReg(rm));
            else
                spew("%-11s%s, %s", name, nameIReg(rm), nameFPReg(dst));
        } else {
            spew("%-11s%s, %s, %s", name, nameIReg(rm), nameFPReg(src0), nameFPReg(dst));
        }
        m_formatter.twoByteOpVex64(ty, opcode, rm, src0, dst);
    }
#endif

    void twoByteOpSimdInt32(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                            XMMRegisterID rm, RegisterID dst)
    {
        if (useLegacySSEEncodingForOtherOutput()) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, %s", legacySSEOpName(name), nameIReg(4, dst), nameFPReg(rm));
            else if (opcode == OP2_MOVD_EdVd)
                spew("%-11s%s, %s", legacySSEOpName(name), nameFPReg((XMMRegisterID)dst), nameIReg(4, (RegisterID)rm));
            else
                spew("%-11s%s, %s", legacySSEOpName(name), nameFPReg(rm), nameIReg(4, dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, (RegisterID)rm, dst);
            return;
        }

        if (IsXMMReversedOperands(opcode))
            spew("%-11s%s, %s", name, nameIReg(4, dst), nameFPReg(rm));
        else if (opcode == OP2_MOVD_EdVd)
            spew("%-11s%s, %s", name, nameFPReg((XMMRegisterID)dst), nameIReg(4, (RegisterID)rm));
        else
            spew("%-11s%s, %s", name, nameFPReg(rm), nameIReg(4, dst));
        m_formatter.twoByteOpVex(ty, opcode, (RegisterID)rm, X86Registers::invalid_xmm, dst);
    }

    void twoByteOpImmSimdInt32(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                               uint32_t imm, XMMRegisterID rm, RegisterID dst)
    {
        if (useLegacySSEEncodingForOtherOutput()) {
            spew("%-11s$0x%x, %s, %s", legacySSEOpName(name), imm, nameFPReg(rm), nameIReg(4, dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, (RegisterID)rm, dst);
            m_formatter.immediate8u(imm);
            return;
        }

        spew("%-11s$0x%x, %s, %s", name, imm, nameFPReg(rm), nameIReg(4, dst));
        m_formatter.twoByteOpVex(ty, opcode, (RegisterID)rm, X86Registers::invalid_xmm, dst);
        m_formatter.immediate8u(imm);
    }

#ifdef JS_CODEGEN_X64
    void twoByteOpSimdInt64(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                            XMMRegisterID rm, RegisterID dst)
    {
        if (useLegacySSEEncodingForOtherOutput()) {
            if (IsXMMReversedOperands(opcode))
                spew("%-11s%s, %s", legacySSEOpName(name), nameIReg(dst), nameFPReg(rm));
            else if (opcode == OP2_MOVD_EdVd)
                spew("%-11s%s, %s", legacySSEOpName(name), nameFPReg((XMMRegisterID)dst), nameIReg((RegisterID)rm));
            else
                spew("%-11s%s, %s", legacySSEOpName(name), nameFPReg(rm), nameIReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp64(opcode, (RegisterID)rm, dst);
            return;
        }

        if (IsXMMReversedOperands(opcode))
            spew("%-11s%s, %s", name, nameIReg(dst), nameFPReg(rm));
        else if (opcode == OP2_MOVD_EdVd)
            spew("%-11s%s, %s", name, nameFPReg((XMMRegisterID)dst), nameIReg((RegisterID)rm));
        else
            spew("%-11s%s, %s", name, nameFPReg(rm), nameIReg(dst));
        m_formatter.twoByteOpVex64(ty, opcode, (RegisterID)rm, X86Registers::invalid_xmm, (XMMRegisterID)dst);
    }
#endif

    void twoByteOpSimdFlags(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                            XMMRegisterID rm, XMMRegisterID reg)
    {
        if (useLegacySSEEncodingForOtherOutput()) {
            spew("%-11s%s, %s", legacySSEOpName(name), nameFPReg(rm), nameFPReg(reg));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, (RegisterID)rm, reg);
            return;
        }

        spew("%-11s%s, %s", name, nameFPReg(rm), nameFPReg(reg));
        m_formatter.twoByteOpVex(ty, opcode, (RegisterID)rm, X86Registers::invalid_xmm, (XMMRegisterID)reg);
    }

    void twoByteOpSimdFlags(const char *name, VexOperandType ty, TwoByteOpcodeID opcode,
                            int32_t offset, RegisterID base, XMMRegisterID reg)
    {
        if (useLegacySSEEncodingForOtherOutput()) {
            spew("%-11s" MEM_ob ", %s", legacySSEOpName(name),
                 ADDR_ob(offset, base), nameFPReg(reg));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.twoByteOp(opcode, offset, base, reg);
            return;
        }

        spew("%-11s" MEM_ob ", %s", name,
             ADDR_ob(offset, base), nameFPReg(reg));
        m_formatter.twoByteOpVex(ty, opcode, offset, base, X86Registers::invalid_xmm, (XMMRegisterID)reg);
    }

    void threeByteOpSimd(const char *name, VexOperandType ty, ThreeByteOpcodeID opcode,
                         ThreeByteEscape escape,
                         XMMRegisterID rm, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            spew("%-11s%s, %s", legacySSEOpName(name), nameFPReg(rm), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.threeByteOp(opcode, escape, (RegisterID)rm, dst);
            return;
        }

        spew("%-11s%s, %s, %s", name, nameFPReg(rm), nameFPReg(src0), nameFPReg(dst));
        m_formatter.threeByteOpVex(ty, opcode, escape, (RegisterID)rm, src0, dst);
    }

    void threeByteOpImmSimd(const char *name, VexOperandType ty, ThreeByteOpcodeID opcode,
                            ThreeByteEscape escape,
                            uint32_t imm, XMMRegisterID rm, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            spew("%-11s$0x%x, %s, %s", legacySSEOpName(name), imm, nameFPReg(rm), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.threeByteOp(opcode, escape, (RegisterID)rm, dst);
            m_formatter.immediate8u(imm);
            return;
        }

        spew("%-11s$0x%x, %s, %s, %s", name, imm, nameFPReg(rm), nameFPReg(src0), nameFPReg(dst));
        m_formatter.threeByteOpVex(ty, opcode, escape, (RegisterID)rm, src0, dst);
        m_formatter.immediate8u(imm);
    }

    void threeByteOpSimd(const char *name, VexOperandType ty, ThreeByteOpcodeID opcode,
                         ThreeByteEscape escape,
                         int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            spew("%-11s" MEM_ob ", %s", legacySSEOpName(name),
                 ADDR_ob(offset, base), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.threeByteOp(opcode, escape, offset, base, dst);
            return;
        }

        spew("%-11s" MEM_ob ", %s, %s", name,
             ADDR_ob(offset, base), nameFPReg(src0), nameFPReg(dst));
        m_formatter.threeByteOpVex(ty, opcode, escape, offset, base, src0, dst);
    }

    void threeByteOpImmSimd(const char *name, VexOperandType ty, ThreeByteOpcodeID opcode,
                            ThreeByteEscape escape,
                            uint32_t imm, int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            spew("%-11s$0x%x, " MEM_ob ", %s", legacySSEOpName(name), imm,
                 ADDR_ob(offset, base), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.threeByteOp(opcode, escape, offset, base, dst);
            m_formatter.immediate8u(imm);
            return;
        }

        spew("%-11s$0x%x, " MEM_ob ", %s, %s", name, imm, ADDR_ob(offset, base),
             nameFPReg(src0), nameFPReg(dst));
        m_formatter.threeByteOpVex(ty, opcode, escape, offset, base, src0, dst);
        m_formatter.immediate8u(imm);
    }

    void threeByteOpSimd(const char *name, VexOperandType ty, ThreeByteOpcodeID opcode,
                         ThreeByteEscape escape,
                         const void *address, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            spew("%-11s%p, %s", legacySSEOpName(name), address, nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.threeByteOp(opcode, escape, address, dst);
            return;
        }

        spew("%-11s%p, %s, %s", name, address, nameFPReg(src0), nameFPReg(dst));
        m_formatter.threeByteOpVex(ty, opcode, escape, address, src0, dst);
    }

    void threeByteOpImmInt32Simd(const char *name, VexOperandType ty, ThreeByteOpcodeID opcode,
                                 ThreeByteEscape escape, uint32_t imm,
                                 RegisterID src1, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            spew("%-11s$0x%x, %s, %s", legacySSEOpName(name), imm, nameIReg(4, src1), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.threeByteOp(opcode, escape, src1, dst);
            m_formatter.immediate8u(imm);
            return;
        }

        spew("%-11s$0x%x, %s, %s, %s", name, imm, nameIReg(4, src1), nameFPReg(src0), nameFPReg(dst));
        m_formatter.threeByteOpVex(ty, opcode, escape, src1, src0, dst);
        m_formatter.immediate8u(imm);
    }

    void threeByteOpImmInt32Simd(const char *name, VexOperandType ty, ThreeByteOpcodeID opcode,
                                 ThreeByteEscape escape, uint32_t imm,
                                 int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src0, dst)) {
            spew("%-11s$0x%x, " MEM_ob ", %s", legacySSEOpName(name), imm, ADDR_ob(offset, base), nameFPReg(dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.threeByteOp(opcode, escape, offset, base, dst);
            m_formatter.immediate8u(imm);
            return;
        }

        spew("%-11s$0x%x, " MEM_ob ", %s, %s", name, imm, ADDR_ob(offset, base), nameFPReg(src0), nameFPReg(dst));
        m_formatter.threeByteOpVex(ty, opcode, escape, offset, base, src0, dst);
        m_formatter.immediate8u(imm);
    }

    void threeByteOpImmSimdInt32(const char *name, VexOperandType ty, ThreeByteOpcodeID opcode,
                                 ThreeByteEscape escape, uint32_t imm,
                                 XMMRegisterID src, RegisterID dst)
    {
        if (useLegacySSEEncodingForOtherOutput()) {
            spew("%-11s$0x%x, %s, %s", legacySSEOpName(name), imm, nameFPReg(src), nameIReg(4, dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.threeByteOp(opcode, escape, (RegisterID)src, dst);
            m_formatter.immediate8u(imm);
            return;
        }

        if (opcode == OP3_PEXTRD_EdVdqIb)
            spew("%-11s$0x%x, %s, %s", name, imm, nameFPReg((XMMRegisterID)dst), nameIReg(4, (RegisterID)src));
        else
            spew("%-11s$0x%x, %s, %s", name, imm, nameFPReg(src), nameIReg(4, dst));
        m_formatter.threeByteOpVex(ty, opcode, escape, (RegisterID)src, X86Registers::invalid_xmm, dst);
        m_formatter.immediate8u(imm);
    }

    void threeByteOpImmSimdInt32(const char *name, VexOperandType ty, ThreeByteOpcodeID opcode,
                                 ThreeByteEscape escape, uint32_t imm,
                                 int32_t offset, RegisterID base, RegisterID dst)
    {
        if (useLegacySSEEncodingForOtherOutput()) {
            spew("%-11s$0x%x, " MEM_ob ", %s", legacySSEOpName(name), imm, ADDR_ob(offset, base), nameIReg(4, dst));
            m_formatter.legacySSEPrefix(ty);
            m_formatter.threeByteOp(opcode, escape, offset, base, dst);
            m_formatter.immediate8u(imm);
            return;
        }

        spew("%-11s$0x%x, " MEM_ob ", %s", name, imm, ADDR_ob(offset, base), nameIReg(4, dst));
        m_formatter.threeByteOpVex(ty, opcode, escape, offset, base, X86Registers::invalid_xmm, dst);
        m_formatter.immediate8u(imm);
    }

    
    
    void vblendvOpSimd(XMMRegisterID mask, XMMRegisterID rm, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncodingForVblendv(mask, src0, dst)) {
            spew("blendvps   %s, %s", nameFPReg(rm), nameFPReg(dst));
            
            m_formatter.legacySSEPrefix(VEX_PD);
            m_formatter.threeByteOp(OP3_BLENDVPS_VdqWdq, ESCAPE_BLENDVPS, (RegisterID)rm, dst);
            return;
        }

        spew("vblendvps  %s, %s, %s, %s",
             nameFPReg(mask), nameFPReg(rm), nameFPReg(src0), nameFPReg(dst));
        
        m_formatter.vblendvOpVex(VEX_PD, OP3_VBLENDVPS_VdqWdq, ESCAPE_VBLENDVPS,
                                 mask, (RegisterID)rm, src0, dst);
    }

    void vblendvOpSimd(XMMRegisterID mask, int32_t offset, RegisterID base, XMMRegisterID src0, XMMRegisterID dst)
    {
        if (useLegacySSEEncodingForVblendv(mask, src0, dst)) {
            spew("blendvps   " MEM_ob ", %s", ADDR_ob(offset, base), nameFPReg(dst));
            
            m_formatter.legacySSEPrefix(VEX_PD);
            m_formatter.threeByteOp(OP3_BLENDVPS_VdqWdq, ESCAPE_BLENDVPS, offset, base, dst);
            return;
        }

        spew("vblendvps  %s, " MEM_ob ", %s, %s",
             nameFPReg(mask), ADDR_ob(offset, base), nameFPReg(src0), nameFPReg(dst));
        
        m_formatter.vblendvOpVex(VEX_PD, OP3_VBLENDVPS_VdqWdq, ESCAPE_VBLENDVPS,
                                 mask, offset, base, src0, dst);
    }

    void shiftOpImmSimd(const char *name, TwoByteOpcodeID opcode, ShiftID shiftKind,
                        uint32_t imm, XMMRegisterID src, XMMRegisterID dst)
    {
        if (useLegacySSEEncoding(src, dst)) {
            spew("%-11s$%d, %s", legacySSEOpName(name), imm, nameFPReg(dst));
            m_formatter.legacySSEPrefix(VEX_PD);
            m_formatter.twoByteOp(opcode, (RegisterID)dst, (int)shiftKind);
            m_formatter.immediate8u(imm);
            return;
        }

        spew("%-11s$%d, %s, %s", name, imm, nameFPReg(src), nameFPReg(dst));
        m_formatter.twoByteOpVex(VEX_PD, opcode, (RegisterID)dst, src, (int)shiftKind);
        m_formatter.immediate8u(imm);
    }

    static int32_t getInt32(void* where)
    {
        return reinterpret_cast<int32_t*>(where)[-1];
    }

    class X86InstructionFormatter {

        static const int maxInstructionSize = 16;

    public:
        
        
        

        void prefix(OneByteOpcodeID pre)
        {
            m_buffer.putByte(pre);
        }

        void legacySSEPrefix(VexOperandType ty)
        {
            switch (ty) {
              case VEX_PS: break;
              case VEX_PD: prefix(PRE_SSE_66); break;
              case VEX_SS: prefix(PRE_SSE_F3); break;
              case VEX_SD: prefix(PRE_SSE_F2); break;
            }
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        void oneByteOp(OneByteOpcodeID opcode)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            m_buffer.putByteUnchecked(opcode);
        }

        void oneByteOp(OneByteOpcodeID opcode, RegisterID reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(0, 0, reg);
            m_buffer.putByteUnchecked(opcode + (reg & 7));
        }

        void oneByteOp(OneByteOpcodeID opcode, RegisterID rm, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, rm);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(rm, reg);
        }

        void oneByteOp(OneByteOpcodeID opcode, int32_t offset, RegisterID base, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(offset, base, reg);
        }

        void oneByteOp_disp32(OneByteOpcodeID opcode, int32_t offset, RegisterID base, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp32(offset, base, reg);
        }

        void oneByteOp(OneByteOpcodeID opcode, int32_t offset, RegisterID base, RegisterID index, int scale, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, index, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(offset, base, index, scale, reg);
        }

        void oneByteOp_disp32(OneByteOpcodeID opcode, int32_t offset, RegisterID index, int scale, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, index, 0);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp32(offset, index, scale, reg);
        }

        void oneByteOp(OneByteOpcodeID opcode, const void* address, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, 0);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp32(address, reg);
        }

        void oneByteOp_disp32(OneByteOpcodeID opcode, const void* address, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, 0);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp32(address, reg);
        }
#ifdef JS_CODEGEN_X64
        void oneByteRipOp(OneByteOpcodeID opcode, int ripOffset, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, 0);
            m_buffer.putByteUnchecked(opcode);
            putModRm(ModRmMemoryNoDisp, noBase, reg);
            m_buffer.putIntUnchecked(ripOffset);
        }

        void oneByteRipOp64(OneByteOpcodeID opcode, int ripOffset, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, 0, 0);
            m_buffer.putByteUnchecked(opcode);
            putModRm(ModRmMemoryNoDisp, noBase, reg);
            m_buffer.putIntUnchecked(ripOffset);
        }

        void twoByteRipOp(TwoByteOpcodeID opcode, int ripOffset, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, 0);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            putModRm(ModRmMemoryNoDisp, noBase, reg);
            m_buffer.putIntUnchecked(ripOffset);
        }

        void twoByteRipOpVex(VexOperandType ty, TwoByteOpcodeID opcode, int ripOffset,
                             XMMRegisterID src0, XMMRegisterID reg)
        {
            int r = (reg >> 3), x = 0, b = 0;
            int m = 1; 
            int w = 0, v = src0, l = 0;
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            putModRm(ModRmMemoryNoDisp, noBase, reg);
            m_buffer.putIntUnchecked(ripOffset);
        }
#endif

        void twoByteOp(TwoByteOpcodeID opcode)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
        }

        void twoByteOp(TwoByteOpcodeID opcode, RegisterID rm, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, rm);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(rm, reg);
        }

        void twoByteOpVex(VexOperandType ty, TwoByteOpcodeID opcode,
                          RegisterID rm, XMMRegisterID src0, int reg)
        {
            int r = (reg >> 3), x = 0, b = (rm >> 3);
            int m = 1; 
            int w = 0, v = src0, l = 0;
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            registerModRM(rm, reg);
        }

        void twoByteOp(TwoByteOpcodeID opcode, int32_t offset, RegisterID base, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, base);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(offset, base, reg);
        }

        void twoByteOpVex(VexOperandType ty, TwoByteOpcodeID opcode,
                          int32_t offset, RegisterID base, XMMRegisterID src0, int reg)
        {
            int r = (reg >> 3), x = 0, b = (base >> 3);
            int m = 1; 
            int w = 0, v = src0, l = 0;
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            memoryModRM(offset, base, reg);
        }

        void twoByteOp_disp32(TwoByteOpcodeID opcode, int32_t offset, RegisterID base, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, base);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp32(offset, base, reg);
        }

        void twoByteOpVex_disp32(VexOperandType ty, TwoByteOpcodeID opcode,
                                 int32_t offset, RegisterID base, XMMRegisterID src0, int reg)
        {
            int r = (reg >> 3), x = 0, b = (base >> 3);
            int m = 1; 
            int w = 0, v = src0, l = 0;
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            memoryModRM_disp32(offset, base, reg);
        }

        void twoByteOp(TwoByteOpcodeID opcode, int32_t offset, RegisterID base, RegisterID index, int scale, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, index, base);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(offset, base, index, scale, reg);
        }

        void twoByteOpVex(VexOperandType ty, TwoByteOpcodeID opcode,
                          int32_t offset, RegisterID base, RegisterID index, int scale,
                          XMMRegisterID src0, int reg)
        {
            int r = (reg >> 3), x = (index >> 3), b = (base >> 3);
            int m = 1; 
            int w = 0, v = src0, l = 0;
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            memoryModRM(offset, base, index, scale, reg);
        }

        void twoByteOp(TwoByteOpcodeID opcode, const void* address, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, 0);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(address, reg);
        }

        void twoByteOpVex(VexOperandType ty, TwoByteOpcodeID opcode,
                          const void* address, XMMRegisterID src0, int reg)
        {
            int r = (reg >> 3), x = 0, b = 0;
            int m = 1; 
            int w = 0, v = src0, l = 0;
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            memoryModRM(address, reg);
        }

        void threeByteOp(ThreeByteOpcodeID opcode, ThreeByteEscape escape, RegisterID rm, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, rm);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(escape);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(rm, reg);
        }

        void threeByteOpVex(VexOperandType ty, ThreeByteOpcodeID opcode, ThreeByteEscape escape,
                            RegisterID rm, XMMRegisterID src0, int reg)
        {
            int r = (reg >> 3), x = 0, b = (rm >> 3);
            int m = 0, w = 0, v = src0, l = 0;
            switch (escape) {
              case 0x38: m = 2; break; 
              case 0x3A: m = 3; break; 
              default: MOZ_CRASH("unexpected escape");
            }
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            registerModRM(rm, reg);
        }

        void threeByteOp(ThreeByteOpcodeID opcode, ThreeByteEscape escape, int32_t offset, RegisterID base, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, base);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(escape);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(offset, base, reg);
        }

        void threeByteOpVex(VexOperandType ty, ThreeByteOpcodeID opcode, ThreeByteEscape escape,
                            int32_t offset, RegisterID base, XMMRegisterID src0, int reg)
        {
            int r = (reg >> 3), x = 0, b = (base >> 3);
            int m = 0, w = 0, v = src0, l = 0;
            switch (escape) {
              case 0x38: m = 2; break; 
              case 0x3A: m = 3; break; 
              default: MOZ_CRASH("unexpected escape");
            }
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            memoryModRM(offset, base, reg);
        }

        void threeByteOp(ThreeByteOpcodeID opcode, ThreeByteEscape escape, const void* address, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, 0);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(escape);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(address, reg);
        }

        void threeByteOpVex(VexOperandType ty, ThreeByteOpcodeID opcode, ThreeByteEscape escape,
                            const void *address, XMMRegisterID src0, int reg)
        {
            int r = (reg >> 3), x = 0, b = 0;
            int m = 0, w = 0, v = src0, l = 0;
            switch (escape) {
              case 0x38: m = 2; break; 
              case 0x3A: m = 3; break; 
              default: MOZ_CRASH("unexpected escape");
            }
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            memoryModRM(address, reg);
        }

        void vblendvOpVex(VexOperandType ty, ThreeByteOpcodeID opcode, ThreeByteEscape escape,
                          XMMRegisterID mask, RegisterID rm, XMMRegisterID src0, int reg)
        {
            int r = (reg >> 3), x = 0, b = (rm >> 3);
            int m = 0, w = 0, v = src0, l = 0;
            switch (escape) {
              case 0x38: m = 2; break; 
              case 0x3A: m = 3; break; 
              default: MOZ_CRASH("unexpected escape");
            }
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            registerModRM(rm, reg);
            immediate8u(mask << 4);
        }

        void vblendvOpVex(VexOperandType ty, ThreeByteOpcodeID opcode, ThreeByteEscape escape,
                          XMMRegisterID mask, int32_t offset, RegisterID base, XMMRegisterID src0, int reg)
        {
            int r = (reg >> 3), x = 0, b = (base >> 3);
            int m = 0, w = 0, v = src0, l = 0;
            switch (escape) {
              case 0x38: m = 2; break; 
              case 0x3A: m = 3; break; 
              default: MOZ_CRASH("unexpected escape");
            }
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            memoryModRM(offset, base, reg);
            immediate8u(mask << 4);
        }

#ifdef JS_CODEGEN_X64
        
        
        
        
        

        void oneByteOp64(OneByteOpcodeID opcode)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(0, 0, 0);
            m_buffer.putByteUnchecked(opcode);
        }

        void oneByteOp64(OneByteOpcodeID opcode, RegisterID reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(0, 0, reg);
            m_buffer.putByteUnchecked(opcode + (reg & 7));
        }

        void oneByteOp64(OneByteOpcodeID opcode, RegisterID rm, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, 0, rm);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(rm, reg);
        }

        void oneByteOp64(OneByteOpcodeID opcode, int32_t offset, RegisterID base, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(offset, base, reg);
        }

        void oneByteOp64_disp32(OneByteOpcodeID opcode, int32_t offset, RegisterID base, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp32(offset, base, reg);
        }

        void oneByteOp64(OneByteOpcodeID opcode, int32_t offset, RegisterID base, RegisterID index, int scale, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, index, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(offset, base, index, scale, reg);
        }

        void oneByteOp64(OneByteOpcodeID opcode, const void* address, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, 0, 0);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(address, reg);
        }

        void twoByteOp64(TwoByteOpcodeID opcode, RegisterID rm, int reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, 0, rm);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(rm, reg);
        }

        void twoByteOpVex64(VexOperandType ty, TwoByteOpcodeID opcode,
                            RegisterID rm, XMMRegisterID src0, XMMRegisterID reg)
        {
            int r = (reg >> 3), x = 0, b = (rm >> 3);
            int m = 1; 
            int w = 1, v = src0, l = 0;
            threeOpVex(ty, r, x, b, m, w, v, l, opcode);
            registerModRM(rm, reg);
        }
#endif

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        void oneByteOp8(OneByteOpcodeID opcode)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            m_buffer.putByteUnchecked(opcode);
        }

        void oneByteOp8(OneByteOpcodeID opcode, RegisterID rm, GroupOpcodeID groupOp)
        {
#ifdef JS_CODEGEN_X86
            MOZ_ASSERT(!ByteRegRequiresRex(rm));
#endif
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(ByteRegRequiresRex(rm), 0, 0, rm);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(rm, groupOp);
        }

        
        void oneByteOp8_norex(OneByteOpcodeID opcode, RegisterID rm, GroupOpcodeID groupOp)
        {
            MOZ_ASSERT(!regRequiresRex(rm));
            m_buffer.ensureSpace(maxInstructionSize);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(rm, groupOp);
        }

        void oneByteOp8(OneByteOpcodeID opcode, int32_t offset, RegisterID base, int reg)
        {
#ifdef JS_CODEGEN_X86
            MOZ_ASSERT(!ByteRegRequiresRex(reg));
#endif
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(ByteRegRequiresRex(reg), reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(offset, base, reg);
        }

        void oneByteOp8_disp32(OneByteOpcodeID opcode, int32_t offset, RegisterID base, int reg)
        {
#ifdef JS_CODEGEN_X86
            MOZ_ASSERT(!ByteRegRequiresRex(reg));
#endif
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(ByteRegRequiresRex(reg), reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp32(offset, base, reg);
        }

        void oneByteOp8(OneByteOpcodeID opcode, int32_t offset, RegisterID base, RegisterID index, int scale, int reg)
        {
#ifdef JS_CODEGEN_X86
            MOZ_ASSERT(!ByteRegRequiresRex(reg));
#endif
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(ByteRegRequiresRex(reg), reg, index, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(offset, base, index, scale, reg);
        }

        void oneByteOp8(OneByteOpcodeID opcode, const void* address, int reg)
        {
#ifdef JS_CODEGEN_X86
            MOZ_ASSERT(!ByteRegRequiresRex(reg));
#endif
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(ByteRegRequiresRex(reg), reg, 0, 0);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp32(address, reg);
        }

        void twoByteOp8(TwoByteOpcodeID opcode, RegisterID rm, RegisterID reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(ByteRegRequiresRex(reg)|ByteRegRequiresRex(rm), reg, 0, rm);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(rm, reg);
        }

        
        
        
        
        void twoByteOp8_movx(TwoByteOpcodeID opcode, RegisterID rm, RegisterID reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(regRequiresRex(reg)|ByteRegRequiresRex(rm), reg, 0, rm);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(rm, reg);
        }

        void twoByteOp8(TwoByteOpcodeID opcode, RegisterID rm, GroupOpcodeID groupOp)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(ByteRegRequiresRex(rm), 0, 0, rm);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(rm, groupOp);
        }

        
        
        
        
        

        
        void immediate8s(int32_t imm)
        {
            MOZ_ASSERT(CAN_SIGN_EXTEND_8_32(imm));
            m_buffer.putByteUnchecked(imm);
        }

        
        void immediate8u(uint32_t imm)
        {
            MOZ_ASSERT(CAN_ZERO_EXTEND_8_32(imm));
            m_buffer.putByteUnchecked(int32_t(imm));
        }

        
        
        void immediate8(int32_t imm)
        {
            m_buffer.putByteUnchecked(imm);
        }

        
        void immediate16s(int32_t imm)
        {
            MOZ_ASSERT(CAN_SIGN_EXTEND_16_32(imm));
            m_buffer.putShortUnchecked(imm);
        }

        
        void immediate16u(int32_t imm)
        {
            MOZ_ASSERT(CAN_ZERO_EXTEND_16_32(imm));
            m_buffer.putShortUnchecked(imm);
        }

        
        
        void immediate16(int32_t imm)
        {
            m_buffer.putShortUnchecked(imm);
        }

        void immediate32(int32_t imm)
        {
            m_buffer.putIntUnchecked(imm);
        }

        void immediate64(int64_t imm)
        {
            m_buffer.putInt64Unchecked(imm);
        }

        JmpSrc immediateRel32()
        {
            m_buffer.putIntUnchecked(0);
            return JmpSrc(m_buffer.size());
        }

        

        void jumpTablePointer(uintptr_t ptr)
        {
            m_buffer.ensureSpace(sizeof(uintptr_t));
#ifdef JS_CODEGEN_X64
            m_buffer.putInt64Unchecked(ptr);
#else
            m_buffer.putIntUnchecked(ptr);
#endif
        }

        void doubleConstant(double d)
        {
            m_buffer.ensureSpace(sizeof(double));
            union {
                uint64_t u64;
                double d;
            } u;
            u.d = d;
            m_buffer.putInt64Unchecked(u.u64);
        }

        void floatConstant(float f)
        {
            m_buffer.ensureSpace(sizeof(float));
            union {
                uint32_t u32;
                float f;
            } u;
            u.f = f;
            m_buffer.putIntUnchecked(u.u32);
        }

        void int32x4Constant(const int32_t s[4])
        {
            for (size_t i = 0; i < 4; ++i)
                int32Constant(s[i]);
        }

        void float32x4Constant(const float s[4])
        {
            for (size_t i = 0; i < 4; ++i)
                floatConstant(s[i]);
        }

        void int64Constant(int64_t i)
        {
            m_buffer.ensureSpace(sizeof(int64_t));
            m_buffer.putInt64Unchecked(i);
        }

        void int32Constant(int32_t i)
        {
            m_buffer.ensureSpace(sizeof(int32_t));
            m_buffer.putIntUnchecked(i);
        }

        

        size_t size() const { return m_buffer.size(); }
        size_t allocSize() const { return m_buffer.allocSize(); }
        unsigned char *buffer() const { return m_buffer.buffer(); }
        bool oom() const { return m_buffer.oom(); }
        bool isAligned(int alignment) const { return m_buffer.isAligned(alignment); }
        void* data() const { return m_buffer.data(); }
        void* executableAllocAndCopy(js::jit::ExecutableAllocator* allocator,
                                     js::jit::ExecutablePool** poolp, js::jit::CodeKind kind) {
            return m_buffer.executableAllocAndCopy(allocator, poolp, kind);
        }

    private:

        

        static const RegisterID noBase = X86Registers::ebp;
        static const RegisterID hasSib = X86Registers::esp;
        static const RegisterID noIndex = X86Registers::esp;
#ifdef JS_CODEGEN_X64
        static const RegisterID noBase2 = X86Registers::r13;
        static const RegisterID hasSib2 = X86Registers::r12;

        
        inline bool regRequiresRex(int reg)
        {
            return (reg >= X86Registers::r8);
        }

        
        inline void emitRex(bool w, int r, int x, int b)
        {
            m_buffer.putByteUnchecked(PRE_REX | ((int)w << 3) | ((r>>3)<<2) | ((x>>3)<<1) | (b>>3));
        }

        
        inline void emitRexW(int r, int x, int b)
        {
            emitRex(true, r, x, b);
        }

        
        
        
        
        
        
        
        inline void emitRexIf(bool condition, int r, int x, int b)
        {
            if (condition || regRequiresRex(r) || regRequiresRex(x) || regRequiresRex(b))
                emitRex(false, r, x, b);
        }

        
        
        inline void emitRexIfNeeded(int r, int x, int b)
        {
            emitRexIf(regRequiresRex(r) || regRequiresRex(x) || regRequiresRex(b), r, x, b);
        }
#else
        
        inline bool regRequiresRex(int) { return false; }
        inline void emitRexIf(bool, int, int, int) {}
        inline void emitRexIfNeeded(int, int, int) {}
#endif

        enum ModRmMode {
            ModRmMemoryNoDisp,
            ModRmMemoryDisp8,
            ModRmMemoryDisp32,
            ModRmRegister
        };

        void putModRm(ModRmMode mode, RegisterID rm, int reg)
        {
            m_buffer.putByteUnchecked((mode << 6) | ((reg & 7) << 3) | (rm & 7));
        }

        void putModRmSib(ModRmMode mode, RegisterID base, RegisterID index, int scale, int reg)
        {
            MOZ_ASSERT(mode != ModRmRegister);

            putModRm(mode, hasSib, reg);
            m_buffer.putByteUnchecked((scale << 6) | ((index & 7) << 3) | (base & 7));
        }

        void registerModRM(RegisterID rm, int reg)
        {
            putModRm(ModRmRegister, rm, reg);
        }

        void memoryModRM(int32_t offset, RegisterID base, int reg)
        {
            
            
#ifdef JS_CODEGEN_X64
            if ((base == hasSib) || (base == hasSib2))
#else
            if (base == hasSib)
#endif
            {
                if (!offset) 
                    putModRmSib(ModRmMemoryNoDisp, base, noIndex, 0, reg);
                else if (CAN_SIGN_EXTEND_8_32(offset)) {
                    putModRmSib(ModRmMemoryDisp8, base, noIndex, 0, reg);
                    m_buffer.putByteUnchecked(offset);
                } else {
                    putModRmSib(ModRmMemoryDisp32, base, noIndex, 0, reg);
                    m_buffer.putIntUnchecked(offset);
                }
            } else {
#ifdef JS_CODEGEN_X64
                if (!offset && (base != noBase) && (base != noBase2))
#else
                if (!offset && (base != noBase))
#endif
                    putModRm(ModRmMemoryNoDisp, base, reg);
                else if (CAN_SIGN_EXTEND_8_32(offset)) {
                    putModRm(ModRmMemoryDisp8, base, reg);
                    m_buffer.putByteUnchecked(offset);
                } else {
                    putModRm(ModRmMemoryDisp32, base, reg);
                    m_buffer.putIntUnchecked(offset);
                }
            }
        }

        void memoryModRM_disp32(int32_t offset, RegisterID base, int reg)
        {
            
            
#ifdef JS_CODEGEN_X64
            if ((base == hasSib) || (base == hasSib2))
#else
            if (base == hasSib)
#endif
            {
                putModRmSib(ModRmMemoryDisp32, base, noIndex, 0, reg);
                m_buffer.putIntUnchecked(offset);
            } else {
                putModRm(ModRmMemoryDisp32, base, reg);
                m_buffer.putIntUnchecked(offset);
            }
        }

        void memoryModRM(int32_t offset, RegisterID base, RegisterID index, int scale, int reg)
        {
            MOZ_ASSERT(index != noIndex);

#ifdef JS_CODEGEN_X64
            if (!offset && (base != noBase) && (base != noBase2))
#else
            if (!offset && (base != noBase))
#endif
                putModRmSib(ModRmMemoryNoDisp, base, index, scale, reg);
            else if (CAN_SIGN_EXTEND_8_32(offset)) {
                putModRmSib(ModRmMemoryDisp8, base, index, scale, reg);
                m_buffer.putByteUnchecked(offset);
            } else {
                putModRmSib(ModRmMemoryDisp32, base, index, scale, reg);
                m_buffer.putIntUnchecked(offset);
            }
        }

        void memoryModRM_disp32(int32_t offset, RegisterID index, int scale, int reg)
        {
            MOZ_ASSERT(index != noIndex);

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            putModRmSib(ModRmMemoryNoDisp, noBase, index, scale, reg);
            m_buffer.putIntUnchecked(offset);
        }

        void memoryModRM_disp32(const void* address, int reg)
        {
            int32_t disp = addressImmediate(address);

#ifdef JS_CODEGEN_X64
            
            putModRmSib(ModRmMemoryNoDisp, noBase, noIndex, 0, reg);
#else
            
            putModRm(ModRmMemoryNoDisp, noBase, reg);
#endif
            m_buffer.putIntUnchecked(disp);
        }

        void memoryModRM(const void* address, int reg)
        {
            memoryModRM_disp32(address, reg);
        }

        void threeOpVex(VexOperandType p, int r, int x, int b, int m, int w, int v, int l,
                        int opcode)
        {
            m_buffer.ensureSpace(maxInstructionSize);

            if (v == X86Registers::invalid_xmm)
                v = XMMRegisterID(0);

            if (x == 0 && b == 0 && m == 1 && w == 0) {
                
                m_buffer.putByteUnchecked(PRE_VEX_C5);
                m_buffer.putByteUnchecked(((r << 7) | (v << 3) | (l << 2) | p) ^ 0xf8);
            } else {
                
                m_buffer.putByteUnchecked(PRE_VEX_C4);
                m_buffer.putByteUnchecked(((r << 7) | (x << 6) | (b << 5) | m) ^ 0xe0);
                m_buffer.putByteUnchecked(((w << 7) | (v << 3) | (l << 2) | p) ^ 0x78);
            }

            m_buffer.putByteUnchecked(opcode);
        }

        AssemblerBuffer m_buffer;
    } m_formatter;

    bool useVEX_;
};

} 
} 

#endif 
