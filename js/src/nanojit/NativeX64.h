






































#ifndef __nanojit_NativeX64__
#define __nanojit_NativeX64__

#ifndef NANOJIT_64BIT
#error "NANOJIT_64BIT must be defined for X64 backend"
#endif

#ifdef PERFM
#define DOPROF
#include "../vprof/vprof.h"
#define count_instr() _nvprof("x64",1)
#define count_prolog() _nvprof("x64-prolog",1); count_instr();
#define count_imt() _nvprof("x64-imt",1) count_instr()
#else
#define count_instr()
#define count_prolog()
#define count_imt()
#endif

namespace nanojit
{
#define NJ_MAX_STACK_ENTRY              256
#define NJ_ALIGN_STACK                  16
#define NJ_JTBL_SUPPORTED               1

    enum Register {
        RAX = 0, 
        RCX = 1, 
        RDX = 2, 
        RBX = 3, 
        RSP = 4, 
        RBP = 5, 
        RSI = 6, 
        RDI = 7, 
        R8  = 8, 
        R9  = 9, 
        R10 = 10, 
        R11 = 11, 
        R12 = 12, 
        R13 = 13, 
        R14 = 14, 
        R15 = 15, 

        XMM0  = 16, 
        XMM1  = 17, 
        XMM2  = 18, 
        XMM3  = 19, 
        XMM4  = 20, 
        XMM5  = 21, 
        XMM6  = 22, 
        XMM7  = 23, 
        XMM8  = 24, 
        XMM9  = 25, 
        XMM10 = 26, 
        XMM11 = 27, 
        XMM12 = 28, 
        XMM13 = 29, 
        XMM14 = 30, 
        XMM15 = 31, 

        FP = RBP,
        UnknownReg = 32,
        FirstReg = RAX,
        LastReg = XMM15
    };




























    enum X64Opcode
#if defined(_MSC_VER) && _MSC_VER >= 1400
#pragma warning(disable:4480) // nonstandard extension used: specifying underlying type for enum
          : uint64_t
#endif
    {
        
        
        X64_addqrr  = 0xC003480000000003LL, 
        X64_addqri  = 0xC081480000000003LL, 
        X64_addqr8  = 0x00C0834800000004LL, 
        X64_andqri  = 0xE081480000000003LL, 
        X64_andqr8  = 0x00E0834800000004LL, 
        X64_orqri   = 0xC881480000000003LL, 
        X64_orqr8   = 0x00C8834800000004LL, 
        X64_xorqri  = 0xF081480000000003LL, 
        X64_xorqr8  = 0x00F0834800000004LL, 
        X64_addlri  = 0xC081400000000003LL, 
        X64_addlr8  = 0x00C0834000000004LL, 
        X64_andlri  = 0xE081400000000003LL, 
        X64_andlr8  = 0x00E0834000000004LL, 
        X64_orlri   = 0xC881400000000003LL, 
        X64_orlr8   = 0x00C8834000000004LL, 
        X64_sublri  = 0xE881400000000003LL, 
        X64_sublr8  = 0x00E8834000000004LL, 
        X64_xorlri  = 0xF081400000000003LL, 
        X64_xorlr8  = 0x00F0834000000004LL, 
        X64_addrr   = 0xC003400000000003LL, 
        X64_andqrr  = 0xC023480000000003LL, 
        X64_andrr   = 0xC023400000000003LL, 
        X64_call    = 0x00000000E8000005LL, 
        X64_callrax = 0xD0FF000000000002LL, 
        X64_cmovqno = 0xC0410F4800000004LL, 
        X64_cmovqnae= 0xC0420F4800000004LL, 
        X64_cmovqnb = 0xC0430F4800000004LL, 
        X64_cmovqne = 0xC0450F4800000004LL, 
        X64_cmovqna = 0xC0460F4800000004LL, 
        X64_cmovqnbe= 0xC0470F4800000004LL, 
        X64_cmovqnge= 0xC04C0F4800000004LL, 
        X64_cmovqnl = 0xC04D0F4800000004LL, 
        X64_cmovqng = 0xC04E0F4800000004LL, 
        X64_cmovqnle= 0xC04F0F4800000004LL, 
        X64_cmovno  = 0xC0410F4000000004LL, 
        X64_cmovnae = 0xC0420F4000000004LL, 
        X64_cmovnb  = 0xC0430F4000000004LL, 
        X64_cmovne  = 0xC0450F4000000004LL, 
        X64_cmovna  = 0xC0460F4000000004LL, 
        X64_cmovnbe = 0xC0470F4000000004LL, 
        X64_cmovnge = 0xC04C0F4000000004LL, 
        X64_cmovnl  = 0xC04D0F4000000004LL, 
        X64_cmovng  = 0xC04E0F4000000004LL, 
        X64_cmovnle = 0xC04F0F4000000004LL, 
        X64_cmplr   = 0xC03B400000000003LL, 
        X64_cmpqr   = 0xC03B480000000003LL, 
        X64_cmplri  = 0xF881400000000003LL, 
        X64_cmpqri  = 0xF881480000000003LL, 
        X64_cmplr8  = 0x00F8834000000004LL, 
        X64_cmpqr8  = 0x00F8834800000004LL, 
        X64_cvtsi2sd= 0xC02A0F40F2000005LL, 
        X64_cvtsq2sd= 0xC02A0F48F2000005LL, 
        X64_divsd   = 0xC05E0F40F2000005LL, 
        X64_mulsd   = 0xC0590F40F2000005LL, 
        X64_addsd   = 0xC0580F40F2000005LL, 
        X64_idiv    = 0xF8F7400000000003LL, 
        X64_imul    = 0xC0AF0F4000000004LL, 
        X64_imuli   = 0xC069400000000003LL, 
        X64_imul8   = 0x00C06B4000000004LL, 
        X64_jmp     = 0x00000000E9000005LL, 
        X64_jmp8    = 0x00EB000000000002LL, 
        X64_jo      = 0x00000000800F0006LL, 
        X64_jb      = 0x00000000820F0006LL, 
        X64_jae     = 0x00000000830F0006LL, 
        X64_ja      = 0x00000000870F0006LL, 
        X64_jbe     = 0x00000000860F0006LL, 
        X64_je      = 0x00000000840F0006LL, 
        X64_jl      = 0x000000008C0F0006LL, 
        X64_jge     = 0x000000008D0F0006LL, 
        X64_jg      = 0x000000008F0F0006LL, 
        X64_jle     = 0x000000008E0F0006LL, 
        X64_jp      = 0x000000008A0F0006LL, 
        X64_jneg    = 0x0000000001000000LL, 
        X64_jo8     = 0x0070000000000002LL, 
        X64_jb8     = 0x0072000000000002LL, 
        X64_jae8    = 0x0073000000000002LL, 
        X64_ja8     = 0x0077000000000002LL, 
        X64_jbe8    = 0x0076000000000002LL, 
        X64_je8     = 0x0074000000000002LL, 
        X64_jne8    = 0x0075000000000002LL, 
        X64_jl8     = 0x007C000000000002LL, 
        X64_jge8    = 0x007D000000000002LL, 
        X64_jg8     = 0x007F000000000002LL, 
        X64_jle8    = 0x007E000000000002LL, 
        X64_jp8     = 0x007A000000000002LL, 
        X64_jnp8    = 0x007B000000000002LL, 
        X64_jneg8   = 0x0001000000000000LL, 
        X64_leaqrm  = 0x00000000808D4807LL, 
        X64_learm   = 0x00000000808D4007LL, 
        X64_learip  = 0x00000000058D4807LL, 
        X64_movlr   = 0xC08B400000000003LL, 
        X64_movlmr  = 0x0000000080894007LL, 
        X64_movlrm  = 0x00000000808B4007LL, 
        X64_movqmr  = 0x0000000080894807LL, 
        X64_movqspr = 0x0024448948000005LL, 
        X64_movqr   = 0xC08B480000000003LL, 
        X64_movqi   = 0xB848000000000002LL, 
        X64_movi    = 0xB840000000000002LL, 
        X64_movqi32 = 0xC0C7480000000003LL, 
        X64_movapsr = 0xC0280F4000000004LL, 
        X64_movqrx  = 0xC07E0F4866000005LL, 
        X64_movqxr  = 0xC06E0F4866000005LL, 
        X64_movqrm  = 0x00000000808B4807LL, 
        X64_movsdrr = 0xC0100F40F2000005LL, 
        X64_movsdrm = 0x80100F40F2000005LL, 
        X64_movsdmr = 0x80110F40F2000005LL, 
        X64_movsxdr = 0xC063480000000003LL, 
        X64_movzx8  = 0xC0B60F4000000004LL, 
        X64_movzx8m = 0x80B60F4000000004LL, 
        X64_movzx16m= 0x80B70F4000000004LL, 
        X64_neg     = 0xD8F7400000000003LL, 
        X64_nop1    = 0x9000000000000001LL, 
        X64_nop2    = 0x9066000000000002LL, 
        X64_nop3    = 0x001F0F0000000003LL, 
        X64_nop4    = 0x00401F0F00000004LL, 
        X64_nop5    = 0x0000441F0F000005LL, 
        X64_nop6    = 0x0000441F0F660006LL, 
        X64_nop7    = 0x00000000801F0F07LL, 
        X64_not     = 0xD0F7400000000003LL, 
        X64_orlrr   = 0xC00B400000000003LL, 
        X64_orqrr   = 0xC00B480000000003LL, 
        X64_popr    = 0x5840000000000002LL, 
        X64_pushr   = 0x5040000000000002LL, 
        X64_pxor    = 0xC0EF0F4066000005LL, 
        X64_ret     = 0xC300000000000001LL, 
        X64_sete    = 0xC0940F4000000004LL, 
        X64_seto    = 0xC0900F4000000004LL, 
        X64_setc    = 0xC0920F4000000004LL, 
        X64_setl    = 0xC09C0F4000000004LL, 
        X64_setle   = 0xC09E0F4000000004LL, 
        X64_setg    = 0xC09F0F4000000004LL, 
        X64_setge   = 0xC09D0F4000000004LL, 
        X64_seta    = 0xC0970F4000000004LL, 
        X64_setae   = 0xC0930F4000000004LL, 
        X64_setb    = 0xC0920F4000000004LL, 
        X64_setbe   = 0xC0960F4000000004LL, 
        X64_subsd   = 0xC05C0F40F2000005LL, 
        X64_shl     = 0xE0D3400000000003LL, 
        X64_shlq    = 0xE0D3480000000003LL, 
        X64_shr     = 0xE8D3400000000003LL, 
        X64_shrq    = 0xE8D3480000000003LL, 
        X64_sar     = 0xF8D3400000000003LL, 
        X64_sarq    = 0xF8D3480000000003LL, 
        X64_shli    = 0x00E0C14000000004LL, 
        X64_shlqi   = 0x00E0C14800000004LL, 
        X64_sari    = 0x00F8C14000000004LL, 
        X64_sarqi   = 0x00F8C14800000004LL, 
        X64_shri    = 0x00E8C14000000004LL, 
        X64_shrqi   = 0x00E8C14800000004LL, 
        X64_subqrr  = 0xC02B480000000003LL, 
        X64_subrr   = 0xC02B400000000003LL, 
        X64_subqri  = 0xE881480000000003LL, 
        X64_subqr8  = 0x00E8834800000004LL, 
        X64_ucomisd = 0xC02E0F4066000005LL, 
        X64_xorqrr  = 0xC033480000000003LL, 
        X64_xorrr   = 0xC033400000000003LL, 
        X64_xorpd   = 0xC0570F4066000005LL, 
        X64_xorps   = 0xC0570F4000000004LL, 
        X64_xorpsm  = 0x05570F4000000004LL, 
        X64_xorpsa  = 0x2504570F40000005LL, 
        X64_jmpx    = 0xC524ff4000000004LL, 
        X64_jmpxb   = 0xC024ff4000000004LL, 

        X86_and8r   = 0xC022000000000002LL, 
        X86_sete    = 0xC0940F0000000003LL, 
        X86_setnp   = 0xC09B0F0000000003LL  
    };

    typedef uint32_t RegisterMask;

    static const RegisterMask GpRegs = 0xffff;
    static const RegisterMask FpRegs = 0xffff0000;
#ifdef _MSC_VER
    static const RegisterMask SavedRegs = 1<<RBX | 1<<RSI | 1<<RDI | 1<<R12 | 1<<R13 | 1<<R14 | 1<<R15;
    static const int NumSavedRegs = 7; 
    static const int NumArgRegs = 4;
#else
    static const RegisterMask SavedRegs = 1<<RBX | 1<<R12 | 1<<R13 | 1<<R14 | 1<<R15;
    static const int NumSavedRegs = 5; 
    static const int NumArgRegs = 6;
#endif

    static inline bool isValidDisplacement(LOpcode, int32_t) {
        return true;
    }
    static inline bool IsFpReg(Register r) {
        return ((1<<r) & FpRegs) != 0;
    }
    static inline bool IsGpReg(Register r) {
        return ((1<<r) & GpRegs) != 0;
    }

    verbose_only( extern const char* regNames[]; )
    verbose_only( extern const char* gpRegNames32[]; )
    verbose_only( extern const char* gpRegNames8[]; )
    verbose_only( extern const char* gpRegNames8hi[]; )

    #define DECLARE_PLATFORM_STATS()
    #define DECLARE_PLATFORM_REGALLOC()

    #define DECLARE_PLATFORM_ASSEMBLER()                                    \
        const static Register argRegs[NumArgRegs], retRegs[1];              \
        void underrunProtect(ptrdiff_t bytes);                              \
        void nativePageReset();                                             \
        void nativePageSetup();                                             \
        void asm_qbinop(LIns*);                                             \
        void MR(Register, Register);\
        void JMP(NIns*);\
        void JMPl(NIns*);\
        void emit(uint64_t op);\
        void emit8(uint64_t op, int64_t val);\
        void emit_target8(size_t underrun, uint64_t op, NIns* target);\
        void emit_target32(size_t underrun, uint64_t op, NIns* target);\
        void emitrr(uint64_t op, Register r, Register b);\
        void emitrxb(uint64_t op, Register r, Register x, Register b);\
        void emitxb(uint64_t op, Register x, Register b) { emitrxb(op, (Register)0, x, b); }\
        void emitrr8(uint64_t op, Register r, Register b);\
        void emitr(uint64_t op, Register b) { emitrr(op, (Register)0, b); }\
        void emitr8(uint64_t op, Register b) { emitrr8(op, (Register)0, b); }\
        void emitprr(uint64_t op, Register r, Register b);\
        void emitrm(uint64_t op, Register r, int32_t d, Register b);\
        void emitrm_wide(uint64_t op, Register r, int32_t d, Register b);\
        uint64_t emit_disp32(uint64_t op, int32_t d);\
        void emitprm(uint64_t op, Register r, int32_t d, Register b);\
        void emitrr_imm(uint64_t op, Register r, Register b, int32_t imm);\
        void emitr_imm64(uint64_t op, Register r, uint64_t imm);\
        void emitrxb_imm(uint64_t op, Register r, Register x, Register b, int32_t imm);\
        void emitr_imm(uint64_t op, Register r, int32_t imm) { emitrr_imm(op, (Register)0, r, imm); }\
        void emitr_imm8(uint64_t op, Register b, int32_t imm8);\
        void emitxm_abs(uint64_t op, Register r, int32_t addr32);\
        void emitxm_rel(uint64_t op, Register r, NIns* addr64);\
        void asm_quad(Register r, uint64_t v);\
        void asm_regarg(ArgSize, LIns*, Register);\
        void asm_stkarg(ArgSize, LIns*, int);\
        void asm_shift(LIns*);\
        void asm_shift_imm(LIns*);\
        void asm_arith_imm(LIns*);\
        void regalloc_unary(LIns *ins, RegisterMask allow, Register &rr, Register &ra);\
        void regalloc_binary(LIns *ins, RegisterMask allow, Register &rr, Register &ra, Register &rb);\
        void regalloc_load(LIns *ins, Register &rr, int32_t &d, Register &rb);\
        void dis(NIns *p, int bytes);\
        void asm_cmp(LIns*);\
        void asm_cmp_imm(LIns*);\
        void fcmp(LIns*, LIns*);\
        NIns* asm_fbranch(bool, LIns*, NIns*);\
        void asm_div_mod(LIns *i);\
        int max_stk_used;\
        void PUSHR(Register r);\
        void POPR(Register r);\
        void NOT(Register r);\
        void NEG(Register r);\
        void IDIV(Register r);\
        void SHR(Register r);\
        void SAR(Register r);\
        void SHL(Register r);\
        void SHRQ(Register r);\
        void SARQ(Register r);\
        void SHLQ(Register r);\
        void SHRI(Register r, int i);\
        void SARI(Register r, int i);\
        void SHLI(Register r, int i);\
        void SHRQI(Register r, int i);\
        void SARQI(Register r, int i);\
        void SHLQI(Register r, int i);\
        void SETE(Register r);\
        void SETL(Register r);\
        void SETLE(Register r);\
        void SETG(Register r);\
        void SETGE(Register r);\
        void SETB(Register r);\
        void SETBE(Register r);\
        void SETA(Register r);\
        void SETAE(Register r);\
        void SETO(Register r);\
        void ADDRR(Register l, Register r);\
        void SUBRR(Register l, Register r);\
        void ANDRR(Register l, Register r);\
        void ORLRR(Register l, Register r);\
        void XORRR(Register l, Register r);\
        void IMUL(Register l, Register r);\
        void CMPLR(Register l, Register r);\
        void MOVLR(Register l, Register r);\
        void ADDQRR(Register l, Register r);\
        void SUBQRR(Register l, Register r);\
        void ANDQRR(Register l, Register r);\
        void ORQRR(Register l, Register r);\
        void XORQRR(Register l, Register r);\
        void CMPQR(Register l, Register r);\
        void MOVQR(Register l, Register r);\
        void MOVAPSR(Register l, Register r);\
        void CMOVNO(Register l, Register r);\
        void CMOVNE(Register l, Register r);\
        void CMOVNL(Register l, Register r);\
        void CMOVNLE(Register l, Register r);\
        void CMOVNG(Register l, Register r);\
        void CMOVNGE(Register l, Register r);\
        void CMOVNB(Register l, Register r);\
        void CMOVNBE(Register l, Register r);\
        void CMOVNA(Register l, Register r);\
        void CMOVNAE(Register l, Register r);\
        void CMOVQNO(Register l, Register r);\
        void CMOVQNE(Register l, Register r);\
        void CMOVQNL(Register l, Register r);\
        void CMOVQNLE(Register l, Register r);\
        void CMOVQNG(Register l, Register r);\
        void CMOVQNGE(Register l, Register r);\
        void CMOVQNB(Register l, Register r);\
        void CMOVQNBE(Register l, Register r);\
        void CMOVQNA(Register l, Register r);\
        void CMOVQNAE(Register l, Register r);\
        void MOVSXDR(Register l, Register r);\
        void MOVZX8(Register l, Register r);\
        void XORPS(Register r);\
        void DIVSD(Register l, Register r);\
        void MULSD(Register l, Register r);\
        void ADDSD(Register l, Register r);\
        void SUBSD(Register l, Register r);\
        void CVTSQ2SD(Register l, Register r);\
        void CVTSI2SD(Register l, Register r);\
        void UCOMISD(Register l, Register r);\
        void MOVQRX(Register l, Register r);\
        void MOVQXR(Register l, Register r);\
        void MOVI(Register r, int32_t i32);\
        void ADDLRI(Register r, int32_t i32);\
        void SUBLRI(Register r, int32_t i32);\
        void ANDLRI(Register r, int32_t i32);\
        void ORLRI(Register r, int32_t i32);\
        void XORLRI(Register r, int32_t i32);\
        void CMPLRI(Register r, int32_t i32);\
        void ADDQRI(Register r, int32_t i32);\
        void SUBQRI(Register r, int32_t i32);\
        void ANDQRI(Register r, int32_t i32);\
        void ORQRI(Register r, int32_t i32);\
        void XORQRI(Register r, int32_t i32);\
        void CMPQRI(Register r, int32_t i32);\
        void MOVQI32(Register r, int32_t i32);\
        void ADDLR8(Register r, int32_t i8);\
        void SUBLR8(Register r, int32_t i8);\
        void ANDLR8(Register r, int32_t i8);\
        void ORLR8(Register r, int32_t i8);\
        void XORLR8(Register r, int32_t i8);\
        void CMPLR8(Register r, int32_t i8);\
        void ADDQR8(Register r, int32_t i8);\
        void SUBQR8(Register r, int32_t i8);\
        void ANDQR8(Register r, int32_t i8);\
        void ORQR8(Register r, int32_t i8);\
        void XORQR8(Register r, int32_t i8);\
        void CMPQR8(Register r, int32_t i8);\
        void IMULI(Register l, Register r, int32_t i32);\
        void MOVQI(Register r, uint64_t u64);\
        void LEARIP(Register r, int32_t d);\
        void LEAQRM(Register r1, int d, Register r2);\
        void MOVLRM(Register r1, int d, Register r2);\
        void MOVQRM(Register r1, int d, Register r2);\
        void MOVLMR(Register r1, int d, Register r2);\
        void MOVQMR(Register r1, int d, Register r2);\
        void MOVZX8M(Register r1, int d, Register r2);\
        void MOVZX16M(Register r1, int d, Register r2);\
        void MOVSDRM(Register r1, int d, Register r2);\
        void MOVSDMR(Register r1, int d, Register r2);\
        void JMP8(size_t n, NIns* t);\
        void JMP32(size_t n, NIns* t);\
        void JO(size_t n, NIns* t);\
        void JE(size_t n, NIns* t);\
        void JL(size_t n, NIns* t);\
        void JLE(size_t n, NIns* t);\
        void JG(size_t n, NIns* t);\
        void JGE(size_t n, NIns* t);\
        void JB(size_t n, NIns* t);\
        void JBE(size_t n, NIns* t);\
        void JA(size_t n, NIns* t);\
        void JAE(size_t n, NIns* t);\
        void JP(size_t n, NIns* t);\
        void JNO(size_t n, NIns* t);\
        void JNE(size_t n, NIns* t);\
        void JNL(size_t n, NIns* t);\
        void JNLE(size_t n, NIns* t);\
        void JNG(size_t n, NIns* t);\
        void JNGE(size_t n, NIns* t);\
        void JNB(size_t n, NIns* t);\
        void JNBE(size_t n, NIns* t);\
        void JNA(size_t n, NIns* t);\
        void JNAE(size_t n, NIns* t);\
        void JO8(size_t n, NIns* t);\
        void JE8(size_t n, NIns* t);\
        void JL8(size_t n, NIns* t);\
        void JLE8(size_t n, NIns* t);\
        void JG8(size_t n, NIns* t);\
        void JGE8(size_t n, NIns* t);\
        void JB8(size_t n, NIns* t);\
        void JBE8(size_t n, NIns* t);\
        void JA8(size_t n, NIns* t);\
        void JAE8(size_t n, NIns* t);\
        void JP8(size_t n, NIns* t);\
        void JNO8(size_t n, NIns* t);\
        void JNE8(size_t n, NIns* t);\
        void JNL8(size_t n, NIns* t);\
        void JNLE8(size_t n, NIns* t);\
        void JNG8(size_t n, NIns* t);\
        void JNGE8(size_t n, NIns* t);\
        void JNB8(size_t n, NIns* t);\
        void JNBE8(size_t n, NIns* t);\
        void JNA8(size_t n, NIns* t);\
        void JNAE8(size_t n, NIns* t);\
        void CALL(size_t n, NIns* t);\
        void CALLRAX();\
        void RET();\
        void MOVQSPR(int d, Register r);\
        void XORPSA(Register r, int32_t i32);\
        void XORPSM(Register r, NIns* a64);\
        void X86_AND8R(Register r);\
        void X86_SETNP(Register r);\
        void X86_SETE(Register r);\

    #define swapptrs()  { NIns* _tins = _nIns; _nIns=_nExitIns; _nExitIns=_tins; }

    const int LARGEST_UNDERRUN_PROT = 32;  

    typedef uint8_t NIns;

    
    const size_t LARGEST_BRANCH_PATCH = 16 * sizeof(NIns);

    inline Register nextreg(Register r) {
        return Register(r+1);
    }

} 

#endif
