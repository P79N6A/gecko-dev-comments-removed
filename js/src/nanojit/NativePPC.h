






































#ifndef __nanojit_NativePPC__
#define __nanojit_NativePPC__

#ifdef PERFM
#define DOPROF
#include "../vprof/vprof.h"
#define count_instr() _nvprof("ppc",1)
#define count_prolog() _nvprof("ppc-prolog",1); count_instr();
#define count_imt() _nvprof("ppc-imt",1) count_instr()
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

    enum ConditionRegister {
        CR0 = 0,
        CR1 = 1,
        CR2 = 2,
        CR3 = 3,
        CR4 = 4,
        CR5 = 5,
        CR6 = 6,
        CR7 = 7,
    };

    enum ConditionBit {
        COND_lt = 0, 
        COND_gt = 1,
        COND_eq = 2,
        COND_so = 3, 
        COND_un = 3,
    };

    
    enum ConditionOption {
        BO_true = 12, 
        BO_false = 4, 
    };

    enum Register {
        
        R0  = 0, 
        SP  = 1, 
        R2  = 2, 
        R3  = 3, 
        R4  = 4, 
        R5  = 5, 
        R6  = 6, 
        R7  = 7, 
        R8  = 8, 
        R9  = 9, 
        R10 = 10, 
        R11 = 11, 
        R12 = 12, 
        R13 = 13, 
        R14 = 14, 
        R15 = 15,
        R16 = 16,
        R17 = 17,
        R18 = 18,
        R19 = 19,
        R20 = 20,
        R21 = 21,
        R22 = 22,
        R23 = 23,
        R24 = 24,
        R25 = 25,
        R26 = 26,
        R27 = 27,
        R28 = 28,
        R29 = 29,
        R30 = 30,
        R31 = 31, 
        FP  = R31,

        
        F0  = 32, 
        F1  = 33, 
        F2  = 34, 
        F3  = 35, 
        F4  = 36, 
        F5  = 37, 
        F6  = 38, 
        F7  = 39, 
        F8  = 40, 
        F9  = 41, 
        F10 = 42, 
        F11 = 43, 
        F12 = 44, 
        F13 = 45, 
        F14 = 46, 
        F15 = 47,
        F16 = 48,
        F17 = 49,
        F18 = 50,
        F19 = 51,
        F20 = 52,
        F21 = 53,
        F22 = 54,
        F23 = 55,
        F24 = 56,
        F25 = 57,
        F26 = 58,
        F27 = 59,
        F28 = 60,
        F29 = 61,
        F30 = 62,
        F31 = 63,

        
        Rxer = 1,
        Rlr  = 8,
        Rctr = 9,

        UnknownReg = 127,
        FirstReg = R0,
        LastReg = F31
    };

    enum PpcOpcode {
        
        PPC_add     = 0x7C000214, 
        PPC_addo    = 0x7C000614, 
        PPC_addi    = 0x38000000, 
        PPC_addis   = 0x3C000000, 
        PPC_and     = 0x7C000038, 
        PPC_andc    = 0x7C000078, 
        PPC_andi    = 0x70000000, 
        PPC_andis   = 0x74000000, 
        PPC_b       = 0x48000000, 
        PPC_bc      = 0x40000000, 
        PPC_bcctr   = 0x4C000420, 
        PPC_cmp     = 0x7C000000, 
        PPC_cmpi    = 0x2C000000, 
        PPC_cmpl    = 0x7C000040, 
        PPC_cmpli   = 0x28000000, 
        PPC_cror    = 0x4C000382, 
        PPC_extsw   = 0x7C0007B4, 
        PPC_fadd    = 0xFC00002A, 
        PPC_fcfid   = 0xFC00069C, 
        PPC_fcmpu   = 0xFC000000, 
        PPC_fdiv    = 0xFC000024, 
        PPC_fmr     = 0xFC000090, 
        PPC_fmul    = 0xFC000032, 
        PPC_fneg    = 0xFC000050, 
        PPC_fsub    = 0xFC000028, 
        PPC_lbz     = 0x88000000, 
        PPC_ld      = 0xE8000000, 
        PPC_ldx     = 0x7C00002A, 
        PPC_lfd     = 0xC8000000, 
        PPC_lfdx    = 0x7C0004AE, 
        PPC_lwz     = 0x80000000, 
        PPC_lwzx    = 0x7C00002E, 
        PPC_mfcr    = 0x7C000026, 
        PPC_mfspr   = 0x7C0002A6, 
        PPC_mtspr   = 0x7C0003A6, 
        PPC_mulli   = 0x1C000000, 
        PPC_mullw   = 0x7C0001D6, 
        PPC_neg     = 0x7C0000D0, 
        PPC_nor     = 0x7C0000F8, 
        PPC_or      = 0x7C000378, 
        PPC_ori     = 0x60000000, 
        PPC_oris    = 0x64000000, 
        PPC_rlwinm  = 0x54000000, 
        PPC_rldicl  = 0x78000000, 
        PPC_rldicr  = 0x78000004, 
        PPC_rldimi  = 0x7800000C, 
        PPC_sld     = 0x7C000036, 
        PPC_slw     = 0x7C000030, 
        PPC_srad    = 0x7C000634, 
        PPC_sradi   = 0x7C000674, 
        PPC_sraw    = 0x7C000630, 
        PPC_srawi   = 0x7C000670, 
        PPC_srd     = 0x7C000436, 
        PPC_srw     = 0x7C000430, 
        PPC_std     = 0xF8000000, 
        PPC_stdu    = 0xF8000001, 
        PPC_stdux   = 0x7C00016A, 
        PPC_stdx    = 0x7C00012A, 
        PPC_stfd    = 0xD8000000, 
        PPC_stfdx   = 0x7C0005AE, 
        PPC_stw     = 0x90000000, 
        PPC_stwu    = 0x94000000, 
        PPC_stwux   = 0x7C00016E, 
        PPC_stwx    = 0x7C00012E, 
        PPC_subf    = 0x7C000050, 
        PPC_xor     = 0x7C000278, 
        PPC_xori    = 0x68000000, 
        PPC_xoris   = 0x6C000000, 

        
        PPC_mr = PPC_or,
        PPC_not = PPC_nor,
        PPC_nop = PPC_ori,
    };

    typedef uint64_t RegisterMask;

    static const RegisterMask GpRegs = 0xffffffff;
    static const RegisterMask FpRegs = 0xffffffff00000000LL;
    
#ifdef NANOJIT_64BIT
    
    static const RegisterMask SavedRegs = 0x7fffc000; 
    static const int NumSavedRegs = 17; 
#else
    static const RegisterMask SavedRegs = 0x7fffe000; 
    static const int NumSavedRegs = 18; 
#endif

    static inline bool isValidDisplacement(LOpcode, int32_t) {
        return true;
    }
    static inline bool IsFpReg(Register r) {
        return r >= F0;
    }

    verbose_only( extern const char* regNames[]; )

    #define DECLARE_PLATFORM_STATS()
    #define DECLARE_PLATFORM_REGALLOC()

#ifdef NANOJIT_64BIT
    #define DECL_PPC64()\
        void asm_qbinop(LIns*);
#else
    #define DECL_PPC64()
#endif

    #define DECLARE_PLATFORM_ASSEMBLER()                                    \
        const static Register argRegs[8], retRegs[2];                       \
        void underrunProtect(int bytes);                                    \
        void nativePageReset();                                             \
        void nativePageSetup();                                             \
        void br(NIns *addr, int link);                                      \
        void br_far(NIns *addr, int link);                                  \
        void asm_regarg(ArgSize, LIns*, Register);                          \
        void asm_li(Register r, int32_t imm);                               \
        void asm_li32(Register r, int32_t imm);                             \
        void asm_li64(Register r, uint64_t imm);                            \
        void asm_cmp(LOpcode op, LIns *a, LIns *b, ConditionRegister);      \
        NIns* asm_branch_far(bool onfalse, LIns *cond, NIns * const targ);  \
        NIns* asm_branch_near(bool onfalse, LIns *cond, NIns * const targ); \
        int  max_param_size; /* bytes */                                    \
        DECL_PPC64()

    #define swapptrs()  do {                                                \
            NIns* _tins = _nIns; _nIns=_nExitIns; _nExitIns=_tins;          \
        } while (0) /* no semi */

    const int LARGEST_UNDERRUN_PROT = 9*4;  

    typedef uint32_t NIns;

    
    const size_t LARGEST_BRANCH_PATCH = 4 * sizeof(NIns);

    #define EMIT1(ins, fmt, ...) do {\
        underrunProtect(4);\
        *(--_nIns) = (NIns) (ins);\
        asm_output(fmt, ##__VA_ARGS__);\
        } while (0) /* no semi */

    #define GPR(r) (r)
    #define FPR(r) ((r)&31)

    #define Bx(li,aa,lk) EMIT1(PPC_b | ((li)&0xffffff)<<2 | (aa)<<1 | (lk),\
        "b%s%s %p", (lk)?"l":"", (aa)?"a":"", _nIns+(li))

    #define B(li)   Bx(li,0,0)
    #define BA(li)  Bx(li,1,0)
    #define BL(li)  Bx(li,0,1)
    #define BLA(li) Bx(li,1,1)

    #define BCx(op,bo,bit,cr,bd,aa,lk) EMIT1(PPC_bc | (bo)<<21 | (4*(cr)+COND_##bit)<<16 |\
        ((bd)&0x3fff)<<2 | (aa)<<1 | (lk),\
        "%s%s%s cr%d,%p", #op, (lk)?"l":"", (aa)?"a":"", (cr), _nIns+(bd))

    #define BLT(cr,bd) BCx(blt, BO_true,  lt, cr, bd, 0, 0)
    #define BGT(cr,bd) BCx(bgt, BO_true,  gt, cr, bd, 0, 0)
    #define BEQ(cr,bd) BCx(beq, BO_true,  eq, cr, bd, 0, 0)
    #define BGE(cr,bd) BCx(bge, BO_false, lt, cr, bd, 0, 0)
    #define BLE(cr,bd) BCx(ble, BO_false, gt, cr, bd, 0, 0)
    #define BNE(cr,bd) BCx(bne, BO_false, eq, cr, bd, 0, 0)
    #define BNG(cr,bd) BCx(bng, BO_false, gt, cr, bd, 0, 0)
    #define BNL(cr,bd) BCx(bnl, BO_false, lt, cr, bd, 0, 0)

    #define BCCTRx(op, bo, bit, cr, lk) EMIT1(PPC_bcctr | (bo)<<21 | (4*(cr)+COND_##bit)<<16 | (lk)&1,\
        "%sctr%s cr%d", #op, (lk)?"l":"", (cr))

    #define BLTCTR(cr) BCCTRx(blt, BO_true,  lt, cr, 0)
    #define BGTCTR(cr) BCCTRx(bgt, BO_true,  gt, cr, 0)
    #define BEQCTR(cr) BCCTRx(beq, BO_true,  eq, cr, 0)
    #define BGECTR(cr) BCCTRx(bge, BO_false, lt, cr, 0)
    #define BLECTR(cr) BCCTRx(ble, BO_false, gt, cr, 0)
    #define BNECTR(cr) BCCTRx(bne, BO_false, eq, cr, 0)
    #define BNGCTR(cr) BCCTRx(bng, BO_false, gt, cr, 0)
    #define BNLCTR(cr) BCCTRx(bnl, BO_false, lt, cr, 0)

    #define Simple(asm,op) EMIT1(op, "%s", #asm)

    #define BCTR(link) EMIT1(0x4E800420 | (link), "bctr%s", (link) ? "l" : "")
    #define BCTRL() BCTR(1)

    #define BLR()   EMIT1(0x4E800020, "blr")
    #define NOP()   EMIT1(PPC_nop, "nop") /* ori 0,0,0 */

    #define ALU2(op, rd, ra, rb, rc) EMIT1(PPC_##op | GPR(rd)<<21 | GPR(ra)<<16 | GPR(rb)<<11 | (rc),\
        "%s%s %s,%s,%s", #op, (rc)?".":"", gpn(rd), gpn(ra), gpn(rb))
    #define BITALU2(op, ra, rs, rb, rc) EMIT1(PPC_##op | GPR(rs)<<21 | GPR(ra)<<16 | GPR(rb)<<11 | (rc),\
        "%s%s %s,%s,%s", #op, (rc)?".":"", gpn(ra), gpn(rs), gpn(rb))
    #define FPUAB(op, d, a, b, rc) EMIT1(PPC_##op | FPR(d)<<21 | FPR(a)<<16 | FPR(b)<<11 | (rc),\
        "%s%s %s,%s,%s", #op, (rc)?".":"", gpn(d), gpn(a), gpn(b))
    #define FPUAC(op, d, a, c, rc) EMIT1(PPC_##op | FPR(d)<<21 | FPR(a)<<16 | FPR(c)<<6 | (rc),\
        "%s%s %s,%s,%s", #op, (rc)?".":"", gpn(d), gpn(a), gpn(c))

    #define ADD(rd,ra,rb)   ALU2(add,  rd, ra, rb, 0)
    #define ADD_(rd,ra,rb)  ALU2(add,  rd, ra, rb, 1)
    #define ADDO(rd,ra,rb)  ALU2(addo, rd, ra, rb, 0)
    #define ADDO_(rd,ra,rb) ALU2(addo, rd, ra, rb, 1)
    #define SUBF(rd,ra,rb)  ALU2(subf, rd, ra, rb, 0)
    #define SUBF_(rd,ra,rb) ALU2(subf, rd, ra, rb, 1)

    #define AND(rd,rs,rb)   BITALU2(and,  rd, rs, rb, 0)
    #define AND_(rd,rs,rb)  BITALU2(and,  rd, rs, rb, 1)
    #define OR(rd,rs,rb)    BITALU2(or,   rd, rs, rb, 0)
    #define OR_(rd,rs,rb)   BITALU2(or,   rd, rs, rb, 1)
    #define NOR(rd,rs,rb)   BITALU2(nor,  rd, rs, rb, 0)
    #define NOR_(rd,rs,rb)  BITALU2(nor,  rd, rs, rb, 1)
    #define SLW(rd,rs,rb)   BITALU2(slw,  rd, rs, rb, 0)
    #define SLW_(rd,rs,rb)  BITALU2(slw,  rd, rs, rb, 1)
    #define SRW(rd,rs,rb)   BITALU2(srw,  rd, rs, rb, 0)
    #define SRW_(rd,rs,rb)  BITALU2(srw,  rd, rs, rb, 1)
    #define SRAW(rd,rs,rb)  BITALU2(sraw, rd, rs, rb, 0)
    #define SRAW_(rd,rs,rb) BITALU2(sraw, rd, rs, rb, 1)
    #define XOR(rd,rs,rb)   BITALU2(xor,  rd, rs, rb, 0)
    #define XOR_(rd,rs,rb)  BITALU2(xor,  rd, rs, rb, 1)

    #define SLD(rd,rs,rb)   BITALU2(sld,  rd, rs, rb, 0)
    #define SRD(rd,rs,rb)   BITALU2(srd,  rd, rs, rb, 0)
    #define SRAD(rd,rs,rb)  BITALU2(srad, rd, rs, rb, 0)

    #define FADD(rd,ra,rb)  FPUAB(fadd, rd, ra, rb, 0)
    #define FADD_(rd,ra,rb) FPUAB(fadd, rd, ra, rb, 1)
    #define FDIV(rd,ra,rb)  FPUAB(fdiv, rd, ra, rb, 0)
    #define FDIV_(rd,ra,rb) FPUAB(fdiv, rd, ra, rb, 1)
    #define FMUL(rd,ra,rb)  FPUAC(fmul, rd, ra, rb, 0)
    #define FMUL_(rd,ra,rb) FPUAC(fmul, rd, ra, rb, 1)
    #define FSUB(rd,ra,rb)  FPUAB(fsub, rd, ra, rb, 0)
    #define FSUB_(rd,ra,rb) FPUAB(fsub, rd, ra, rb, 1)

    #define MULLI(rd,ra,simm) EMIT1(PPC_mulli | GPR(rd)<<21 | GPR(ra)<<16 | uint16_t(simm),\
        "mulli %s,%s,%d", gpn(rd), gpn(ra), int16_t(simm))
    #define MULLW(rd,ra,rb) EMIT1(PPC_mullw | GPR(rd)<<21 | GPR(ra)<<16 | GPR(rb)<<11,\
        "mullw %s,%s,%s", gpn(rd), gpn(ra), gpn(rb))

    
    #define ALU1(op, ra, rs, rc) EMIT1(PPC_##op | GPR(rs)<<21 | GPR(ra)<<16 | GPR(rs)<<11 | (rc),\
        "%s%s %s,%s", #op, (rc)?".":"", gpn(ra), gpn(rs))

    #define MR(rd, rs)    ALU1(mr,    rd, rs, 0)   // or   rd,rs,rs
    #define MR_(rd, rs)   ALU1(mr,    rd, rs, 1)   // or.  rd,rs,rs
    #define NOT(rd, rs)   ALU1(not,   rd, rs, 0)   // nor  rd,rs,rs
    #define NOT_(rd, rs)  ALU1(not,   rd, rs, 0)   // nor. rd,rs,rs

    #define EXTSW(rd, rs) EMIT1(PPC_extsw | GPR(rs)<<21 | GPR(rd)<<16,\
        "extsw %s,%s", gpn(rd), gpn(rs))

    #define NEG(rd, rs)  EMIT1(PPC_neg | GPR(rd)<<21 | GPR(rs)<<16, "neg %s,%s", gpn(rd), gpn(rs))
    #define FNEG(rd,rs)  EMIT1(PPC_fneg | FPR(rd)<<21 | FPR(rs)<<11, "fneg %s,%s", gpn(rd), gpn(rs))
    #define FMR(rd,rb)   EMIT1(PPC_fmr  | FPR(rd)<<21 | FPR(rb)<<11, "fmr %s,%s", gpn(rd), gpn(rb))
    #define FCFID(rd,rs) EMIT1(PPC_fcfid | FPR(rd)<<21 | FPR(rs)<<11, "fcfid %s,%s", gpn(rd), gpn(rs))

    #define JMP(addr) br(addr, 0)

    #define SPR(spr) ((R##spr)>>5|(R##spr&31)<<5)
    #define MTSPR(spr,rs) EMIT1(PPC_mtspr | GPR(rs)<<21 | SPR(spr)<<11,\
        "mt%s %s", #spr, gpn(rs))
    #define MFSPR(rd,spr) EMIT1(PPC_mfspr | GPR(rd)<<21 | SPR(spr)<<11,\
        "mf%s %s", #spr, gpn(rd))

    #define MTXER(r) MTSPR(xer, r)
    #define MTLR(r)  MTSPR(lr,  r)
    #define MTCTR(r) MTSPR(ctr, r)

    #define MFXER(r) MFSPR(r, xer)
    #define MFLR(r)  MFSPR(r, lr)
    #define MFCTR(r) MFSPR(r, ctr)

    #define MEMd(op, r, d, a) do {\
        NanoAssert(isS16(d) && (d&3)==0);\
        EMIT1(PPC_##op | GPR(r)<<21 | GPR(a)<<16 | uint16_t(d), "%s %s,%d(%s)", #op, gpn(r), int16_t(d), gpn(a));\
        } while(0) /* no addr */

    #define FMEMd(op, r, d, b) do {\
        NanoAssert(isS16(d));\
        EMIT1(PPC_##op | FPR(r)<<21 | GPR(b)<<16 | uint16_t(d), "%s %s,%d(%s)", #op, gpn(r), int16_t(d), gpn(b));\
        } while(0) /* no addr */

    #define MEMx(op, r, a, b) EMIT1(PPC_##op | GPR(r)<<21 | GPR(a)<<16 | GPR(b)<<11,\
        "%s %s,%s,%s", #op, gpn(r), gpn(a), gpn(b))
    #define FMEMx(op, r, a, b) EMIT1(PPC_##op | FPR(r)<<21 | GPR(a)<<16 | GPR(b)<<11,\
        "%s %s,%s,%s", #op, gpn(r), gpn(a), gpn(b))

    #define MEMux(op, rs, ra, rb) EMIT1(PPC_##op | GPR(rs)<<21 | GPR(ra)<<16 | GPR(rb)<<11,\
                "%s %s,%s,%s", #op, gpn(rs), gpn(ra), gpn(rb))

    #define LBZ(r,  d, b) MEMd(lbz,  r, d, b)
    #define LWZ(r,  d, b) MEMd(lwz,  r, d, b)
    #define LD(r,   d, b) MEMd(ld,   r, d, b)
    #define LWZX(r, a, b) MEMx(lwzx, r, a, b)
    #define LDX(r,  a, b) MEMx(ldx,  r, a, b)

    #define STW(r,  d, b)     MEMd(stw,    r, d, b)
    #define STWU(r, d, b)     MEMd(stwu,   r, d, b)
    #define STWX(s, a, b)     MEMx(stwx,   s, a, b)
    #define STWUX(s, a, b)    MEMux(stwux, s, a, b)

    #define STD(r,  d, b)     MEMd(std,    r, d, b)
    #define STDU(r, d, b)     MEMd(stdu,   r, d, b)
    #define STDX(s, a, b)     MEMx(stdx,   s, a, b)
    #define STDUX(s, a, b)    MEMux(stdux, s, a, b)

#ifdef NANOJIT_64BIT
    #define LP(r, d, b)       LD(r, d, b)
    #define STP(r, d, b)      STD(r, d, b)
    #define STPU(r, d, b)     STDU(r, d, b)
    #define STPX(s, a, b)     STDX(s, a, b)
    #define STPUX(s, a, b)    STDUX(s, a, b)
#else
    #define LP(r, d, b)       LWZ(r, d, b)
    #define STP(r, d, b)      STW(r, d, b)
    #define STPU(r, d, b)     STWU(r, d, b)
    #define STPX(s, a, b)     STWX(s, a, b)
    #define STPUX(s, a, b)    STWUX(s, a, b)
#endif

    #define LFD(r,  d, b) FMEMd(lfd,  r, d, b)
    #define LFDX(r, a, b) FMEMx(lfdx, r, a, b)
    #define STFD(r, d, b) FMEMd(stfd, r, d, b)
    #define STFDX(s, a, b) FMEMx(stfdx, s, a, b)

    #define ALUI(op,rd,ra,d) EMIT1(PPC_##op | GPR(rd)<<21 | GPR(ra)<<16 | uint16_t(d),\
                "%s %s,%s,%d (0x%x)", #op, gpn(rd), gpn(ra), int16_t(d), int16_t(d))

    #define ADDI(rd,ra,d)  ALUI(addi,  rd, ra, d)
    #define ADDIS(rd,ra,d) ALUI(addis, rd, ra, d)

    
    #define BITALUI(op,rd,ra,d) EMIT1(PPC_##op | GPR(ra)<<21 | GPR(rd)<<16 | uint16_t(d),\
                "%s %s,%s,%u (0x%x)", #op, gpn(rd), gpn(ra), uint16_t(d), uint16_t(d))

    #define ANDI(rd,ra,d)  BITALUI(andi,  rd, ra, d)
    #define ORI(rd,ra,d)   BITALUI(ori,   rd, ra, d)
    #define ORIS(rd,ra,d)  BITALUI(oris,  rd, ra, d)
    #define XORI(rd,ra,d)  BITALUI(xori,  rd, ra, d)
    #define XORIS(rd,ra,d) BITALUI(xoris, rd, ra, d)

    #define SUBI(rd,ra,d) EMIT1(PPC_addi | GPR(rd)<<21 | GPR(ra)<<16 | uint16_t(-(d)),\
        "subi %s,%s,%d", gpn(rd), gpn(ra), (d))

    #define LI(rd,v) EMIT1(PPC_addi | GPR(rd)<<21 | uint16_t(v),\
        "li %s,%d (0x%x)", gpn(rd), int16_t(v), int16_t(v)) /* addi rd,0,v */

    #define LIS(rd,v) EMIT1(PPC_addis | GPR(rd)<<21 | uint16_t(v),\
        "lis %s,%d (0x%x)", gpn(rd), int16_t(v), int16_t(v)<<16) /* addis, rd,0,v */

    #define MTCR(rs)
    #define MFCR(rd) EMIT1(PPC_mfcr | GPR(rd)<<21, "mfcr %s", gpn(rd))

    #define CMPx(op, crfd, ra, rb, l) EMIT1(PPC_##op | (crfd)<<23 | (l)<<21 | GPR(ra)<<16 | GPR(rb)<<11,\
        "%s%c cr%d,%s,%s", #op, (l)?'d':'w', (crfd), gpn(ra), gpn(rb))

    #define CMPW(cr, ra, rb)   CMPx(cmp,    cr, ra, rb, 0)
    #define CMPLW(cr, ra, rb)  CMPx(cmpl,   cr, ra, rb, 0)
    #define CMPD(cr, ra, rb)   CMPx(cmp,    cr, ra, rb, 1)
    #define CMPLD(cr, ra, rb)  CMPx(cmpl,   cr, ra, rb, 1)

    #define CMPxI(cr, ra, simm, l) EMIT1(PPC_cmpi | (cr)<<23 | (l)<<21 | GPR(ra)<<16 | uint16_t(simm),\
        "cmp%ci cr%d,%s,%d (0x%x)", (l)?'d':'w', (cr), gpn(ra), int16_t(simm), int16_t(simm))

    #define CMPWI(cr, ra, simm) CMPxI(cr, ra, simm, 0)
    #define CMPDI(cr, ra, simm) CMPxI(cr, ra, simm, 1)

    #define CMPLxI(cr, ra, uimm, l) EMIT1(PPC_cmpli | (cr)<<23 | (l)<<21 | GPR(ra)<<16 | uint16_t(uimm),\
        "cmp%ci cr%d,%s,%d (0x%x)", (l)?'d':'w', (cr), gpn(ra), uint16_t(uimm), uint16_t(uimm))

    #define CMPLWI(cr, ra, uimm) CMPLxI(cr, ra, uimm, 0)
    #define CMPLDI(cr, ra, uimm) CMPLxI(cr, ra, uimm, 1)

    #define FCMPx(op, crfd, ra, rb) EMIT1(PPC_##op | (crfd)<<23 | FPR(ra)<<16 | FPR(rb)<<11,\
        "%s cr%d,%s,%s", #op, (crfd), gpn(ra), gpn(rb))

    #define FCMPU(cr, ra, rb) FCMPx(fcmpu, cr, ra, rb)

    #define CROR(cr,d,a,b) EMIT1(PPC_cror | (4*(cr)+COND_##d)<<21 | (4*(cr)+COND_##a)<<16 | (4*(cr)+COND_##b)<<11,\
        "cror %d,%d,%d", 4*(cr)+COND_##d, 4*(cr)+COND_##a, 4*(cr)+COND_##b)

    #define RLWINM(rd,rs,sh,mb,me) EMIT1(PPC_rlwinm | GPR(rs)<<21 | GPR(rd)<<16 | (sh)<<11 | (mb)<<6 | (me)<<1,\
        "rlwinm %s,%s,%d,%d,%d", gpn(rd), gpn(rs), (sh), (mb), (me))

    #define LO5(sh) ((sh) & 31)
    #define BIT6(sh) (((sh) >> 5) & 1)
    #define SPLITMB(mb) (LO5(mb)<<1 | BIT6(mb))

    #define RLDICL(rd,rs,sh,mb) \
        EMIT1(PPC_rldicl | GPR(rs)<<21 | GPR(rd)<<16 | LO5(sh)<<11 | SPLITMB(mb)<<5 | BIT6(sh)<<1,\
        "rldicl %s,%s,%d,%d", gpn(rd), gpn(rs), (sh), (mb))

    
    #define CLRLDI(rd,rs,n) \
        EMIT1(PPC_rldicl | GPR(rs)<<21 | GPR(rd)<<16 | SPLITMB(n)<<5,\
        "clrldi %s,%s,%d", gpn(rd), gpn(rs), (n))

    #define RLDIMI(rd,rs,sh,mb) \
        EMIT1(PPC_rldimi | GPR(rs)<<21 | GPR(rd)<<16 | LO5(sh)<<11 | SPLITMB(mb)<<5 | BIT6(sh)<<1,\
        "rldimi %s,%s,%d,%d", gpn(rd), gpn(rs), (sh), (mb))

    
    #define INSRDI(rd,rs,n,b) \
        EMIT1(PPC_rldimi | GPR(rs)<<21 | GPR(rd)<<16 | LO5(64-((b)+(n)))<<11 | SPLITMB(b)<<5 | BIT6(64-((b)+(n)))<<1,\
        "insrdi %s,%s,%d,%d", gpn(rd), gpn(rs), (n), (b))

    #define EXTRWI(rd,rs,n,b) EMIT1(PPC_rlwinm | GPR(rs)<<21 | GPR(rd)<<16 | ((n)+(b))<<11 | (32-(n))<<6 | 31<<1,\
        "extrwi %s,%s,%d,%d", gpn(rd), gpn(rs), (n), (b))

    
    #define SLDI(rd,rs,n) EMIT1(PPC_rldicr | GPR(rs)<<21 | GPR(rd)<<16 | LO5(n)<<11 | SPLITMB(63-(n))<<5 | BIT6(n)<<1,\
        "sldi %s,%s,%d", gpn(rd), gpn(rs), (n))

    #define SLWI(rd,rs,n) EMIT1(PPC_rlwinm | GPR(rs)<<21 | GPR(rd)<<16 | (n)<<11 | 0<<6 | (31-(n))<<1,\
        "slwi %s,%s,%d", gpn(rd), gpn(rs), (n))
    #define SRWI(rd,rs,n) EMIT1(PPC_rlwinm | GPR(rs)<<21 | GPR(rd)<<16 | (32-(n))<<11 | (n)<<6 | 31<<1,\
        "slwi %s,%s,%d", gpn(rd), gpn(rs), (n))
    #define SRAWI(rd,rs,n) EMIT1(PPC_srawi | GPR(rs)<<21 | GPR(rd)<<16 | (n)<<11,\
        "srawi %s,%s,%d", gpn(rd), gpn(rs), (n))

} 

#endif 
