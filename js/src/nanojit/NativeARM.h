







































#ifndef __nanojit_NativeArm__
#define __nanojit_NativeArm__


#ifdef PERFM
#include "../vprof/vprof.h"
#define count_instr() _nvprof("arm",1)
#define count_prolog() _nvprof("arm-prolog",1); count_instr();
#define count_imt() _nvprof("arm-imt",1) count_instr()
#else
#define count_instr()
#define count_prolog()
#define count_imt()
#endif

namespace nanojit
{

const int NJ_LOG2_PAGE_SIZE = 12;       








#ifdef NJ_ARM_VFP


#define NJ_VFP_MAX_REGISTERS            8

#else

#define NJ_VFP_MAX_REGISTERS            0
#define NJ_SOFTFLOAT

#endif

#define NJ_MAX_REGISTERS                (11 + NJ_VFP_MAX_REGISTERS)
#define NJ_MAX_STACK_ENTRY              256
#define NJ_MAX_PARAMETERS               16
#define NJ_ALIGN_STACK                  8
#define NJ_STACK_OFFSET                 0

#define NJ_CONSTANT_POOLS
const int NJ_MAX_CPOOL_OFFSET = 4096;
const int NJ_CPOOL_SIZE = 16;

const int LARGEST_UNDERRUN_PROT = 32;  

typedef int NIns;


typedef enum {
    R0  = 0,
    R1  = 1,
    R2  = 2,
    R3  = 3,
    R4  = 4,
    R5  = 5,
    R6  = 6,
    R7  = 7,
    R8  = 8,
    R9  = 9,
    R10 = 10,
    FP  = 11,
    IP  = 12,
    SP  = 13,
    LR  = 14,
    PC  = 15,

    
    D0 = 16,
    D1 = 17,
    D2 = 18,
    D3 = 19,
    D4 = 20,
    D5 = 21,
    D6 = 22,
    D7 = 23,

    FirstFloatReg = 16,
    LastFloatReg = 22,
        
    FirstReg = 0,
#ifdef NJ_ARM_VFP
    LastReg = 23,
#else
    LastReg = 10,
#endif
    Scratch = IP,
    UnknownReg = 31,

    
    FpSingleScratch = 24
} Register;


typedef enum {
    EQ = 0x0, 
    NE = 0x1, 
    CS = 0x2, 
    CC = 0x3, 
    MI = 0x4, 
    PL = 0x5, 
    VS = 0x6, 
    VC = 0x7, 
    HI = 0x8, 
    LS = 0x9, 
    GE = 0xA, 
    LT = 0xB, 
    GT = 0xC, 
    LE = 0xD, 
    AL = 0xE, 
    NV = 0xF  
} ConditionCode;

const char *ccName(ConditionCode cc);

typedef int RegisterMask;
typedef struct _FragInfo {
    RegisterMask    needRestoring;
    NIns*           epilogue;
} FragInfo;



static const RegisterMask SavedFpRegs = 0;
static const RegisterMask SavedRegs = 1<<R4 | 1<<R5 | 1<<R6 | 1<<R7 | 1<<R8 | 1<<R9 | 1<<R10;
static const int NumSavedRegs = 7;

static const RegisterMask FpRegs = 1<<D0 | 1<<D1 | 1<<D2 | 1<<D3 | 1<<D4 | 1<<D5 | 1<<D6; 
static const RegisterMask GpRegs = 0x07FF;
static const RegisterMask AllowableFlagRegs = 1<<R0 | 1<<R1 | 1<<R2 | 1<<R3 | 1<<R4 | 1<<R5 | 1<<R6 | 1<<R7 | 1<<R8 | 1<<R9 | 1<<R10;

#define IsFpReg(_r)     ((rmask(_r) & (FpRegs | (1<<D7))) != 0)
#define IsGpReg(_r)     ((rmask(_r) & (GpRegs | (1<<Scratch))) != 0)
#define FpRegNum(_fpr)  ((_fpr) - FirstFloatReg)

#define firstreg()      R0
#define nextreg(r)      ((Register)((int)(r)+1))
#if 0
static Register nextreg(Register r) {
    if (r == R10)
        return D0;
    return (Register)(r+1);
}
#endif

#define imm2register(c) (Register)(c-1)

verbose_only( extern const char* regNames[]; )


#define nExtractPlatformFlags(x)    0

#define DECLARE_PLATFORM_STATS()

#define DECLARE_PLATFORM_REGALLOC()

#define DECLARE_PLATFORM_ASSEMBLER()                                    \
    const static Register argRegs[4], retRegs[2];                       \
    void LD32_nochk(Register r, int32_t imm);                           \
    void BL(NIns*);                                                     \
    void JMP_far(NIns*);                                                \
    void B_cond_chk(ConditionCode, NIns*, bool);                        \
    void underrunProtect(int bytes);                                    \
    void nativePageReset();                                             \
    void nativePageSetup();                                             \
    void asm_quad_nochk(Register, const int32_t*);                      \
    void asm_add_imm(Register, Register, int32_t);                      \
    int* _nSlot;                                                        \
    int* _nExitSlot;


#define asm_farg(i) NanoAssert(false)



#define swapptrs()  {                                                   \
        NIns* _tins = _nIns; _nIns=_nExitIns; _nExitIns=_tins;          \
        int* _nslot = _nSlot;                                           \
        _nSlot = _nExitSlot;                                            \
        _nExitSlot = _nslot;                                            \
    }


#define IMM32(imm)  *(--_nIns) = (NIns)((imm));

#define OP_IMM  (1<<25)
#define OP_STAT (1<<20)

#define COND_AL (0xE<<28)

typedef enum {
    LSL_imm = 0, 
    LSL_reg = 1, 
    LSR_imm = 2, 
    LSR_reg = 3, 
    ASR_imm = 4, 
    ASR_reg = 5, 
    ROR_imm = 6, 
    RRX     = 6, 
    ROR_reg = 7  
} ShiftOperator;

#define LD32_size 8

#define BEGIN_NATIVE_CODE(x)                    \
    { DWORD* _nIns = (uint8_t*)x

#define END_NATIVE_CODE(x)                      \
    (x) = (dictwordp*)_nIns; }


#define BX(_r)  do {                                                    \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x12<<20) | (0xFFF<<8) | (1<<4) | (_r)); \
        asm_output("bx LR"); } while(0)


#define OR(_l,_r)       do {                                            \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0xC<<21) | (_r<<16) | (_l<<12) | (_l) ); \
        asm_output("or %s,%s",gpn(_l),gpn(_r)); } while(0)


#define ORi(_r,_imm)    do {                                            \
        NanoAssert(isU8((_imm)));                                       \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | OP_IMM | (0xC<<21) | (_r<<16) | (_r<<12) | ((_imm)&0xFF) ); \
        asm_output("or %s,%d",gpn(_r), (_imm)); } while(0)


#define AND(_l,_r) do {                                                 \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | ((_r)<<16) | ((_l)<<12) | (_l)); \
        asm_output("and %s,%s",gpn(_l),gpn(_r)); } while(0)


#define ANDi(_r,_imm) do {                                              \
        if (isU8((_imm))) {                                             \
            underrunProtect(4);                                         \
            *(--_nIns) = (NIns)( COND_AL | OP_IMM | ((_r)<<16) | ((_r)<<12) | ((_imm)&0xFF) ); \
            asm_output("and %s,%d",gpn(_r),(_imm));}                   \
        else if ((_imm)<0 && (_imm)>-256) {                             \
            underrunProtect(8);                                         \
            *(--_nIns) = (NIns)( COND_AL | ((_r)<<16) | ((_r)<<12) | (Scratch) ); \
            asm_output("and %s,%s",gpn(_r),gpn(Scratch));              \
            *(--_nIns) = (NIns)( COND_AL | (0x3E<<20) | ((Scratch)<<12) | (((_imm)^0xFFFFFFFF)&0xFF) ); \
            asm_output("mvn %s,%d",gpn(Scratch),(_imm));}              \
        else NanoAssert(0);                                             \
    } while (0)



#define XOR(_l,_r)  do {                                                \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (1<<21) | ((_r)<<16) | ((_l)<<12) | (_l)); \
        asm_output("eor %s,%s",gpn(_l),gpn(_r)); } while(0)


#define XORi(_r,_imm)   do {                                            \
        NanoAssert(isU8((_imm)));                                       \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<21) | ((_r)<<16) | ((_r)<<12) | ((_imm)&0xFF) ); \
        asm_output("eor %s,%d",gpn(_r),(_imm)); } while(0)


#define arm_ADD(_d,_n,_m) do {                                          \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | OP_STAT | (1<<23) | ((_n)<<16) | ((_d)<<12) | (_m)); \
        asm_output("add %s,%s+%s",gpn(_d),gpn(_n),gpn(_m)); } while(0)


#define ADD(_l,_r)   arm_ADD(_l,_l,_r)



#define arm_ADDi(_d,_n,_imm)   asm_add_imm(_d,_n,_imm)
#define ADDi(_r,_imm)  arm_ADDi(_r,_r,_imm)


#define SUB(_l,_r)  do {                                                \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (1<<22) | ((_l)<<16) | ((_l)<<12) | (_r)); \
        asm_output("sub %s,%s",gpn(_l),gpn(_r)); } while(0)


#define SUBi(_r,_imm)  do {                                             \
        if ((_imm)>-256 && (_imm)<256) {                                \
            underrunProtect(4);                                         \
            if ((_imm)>=0)  *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<22) | ((_r)<<16) | ((_r)<<12) | ((_imm)&0xFF) ); \
            else            *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<23) | ((_r)<<16) | ((_r)<<12) | ((-(_imm))&0xFF) ); \
        } else {                                                        \
            if ((_imm)>=0) {                                            \
                if ((_imm)<=510) {                                      \
                    underrunProtect(8);                                 \
                    int rem = (_imm) - 255;                             \
                    NanoAssert(rem<256);                                \
                    *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<22) | ((_r)<<16) | ((_r)<<12) | (rem&0xFF) ); \
                    *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<22) | ((_r)<<16) | ((_r)<<12) | (0xFF) ); \
                } else {                                                \
                    underrunProtect(4+LD32_size);                       \
                    *(--_nIns) = (NIns)( COND_AL | (1<<22) | ((_r)<<16) | ((_r)<<12) | (Scratch)); \
                    LD32_nochk(Scratch, _imm);                          \
                }                                                       \
            } else {                                                    \
                if ((_imm)>=-510) {                                     \
                    underrunProtect(8);                                 \
                    int rem = -(_imm) - 255;                            \
                    *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<23) | ((_r)<<16) | ((_r)<<12) | ((rem)&0xFF) ); \
                    *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<23) | ((_r)<<16) | ((_r)<<12) | (0xFF) ); \
                } else {                                                \
                    underrunProtect(4+LD32_size);                       \
                    *(--_nIns) = (NIns)( COND_AL | (1<<23) | ((_r)<<16) | ((_r)<<12) | (Scratch)); \
                    LD32_nochk(Scratch, -(_imm)); \
                }                                                       \
            }                                                           \
        }                                                               \
        asm_output("sub %s,%d",gpn(_r),(_imm));                        \
    } while (0)


#define MUL(_l,_r)  do {                                                \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (_l)<<16 | (_l)<<8 | 0x90 | (_r) ); \
        asm_output("mul %s,%s",gpn(_l),gpn(_r)); } while(0)




#define NEG(_r) do {                                                    \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL |  (0x27<<20) | ((_r)<<16) | ((_r)<<12) ); \
        asm_output("neg %s",gpn(_r)); } while(0)



#define NOT(_r) do {                                                    \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL |  (0x1F<<20) | ((_r)<<12) |  (_r) ); \
        asm_output("mvn %s",gpn(_r)); } while(0)



#define SHR(_r,_s) do {                                                 \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x1B<<20) | ((_r)<<12) | ((_s)<<8) | (LSR_reg<<4) | (_r) ); \
        asm_output("shr %s,%s",gpn(_r),gpn(_s)); } while(0)



#define SHRi(_r,_imm) do {                                              \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x1B<<20) | ((_r)<<12) | ((_imm)<<7) | (LSR_imm<<4) | (_r) ); \
        asm_output("shr %s,%d",gpn(_r),_imm); } while(0)



#define SAR(_r,_s) do {                                                 \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x1B<<20) | ((_r)<<12) | ((_s)<<8) | (ASR_reg<<4) | (_r) ); \
        asm_output("asr %s,%s",gpn(_r),gpn(_s)); } while(0)




#define SARi(_r,_imm) do {                                              \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x1B<<20) | ((_r)<<12) | ((_imm)<<7) | (ASR_imm<<4) | (_r) ); \
        asm_output("asr %s,%d",gpn(_r),_imm); } while(0)



#define SHL(_r,_s) do {                                                 \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x1B<<20) | ((_r)<<12) | ((_s)<<8) | (LSL_reg<<4) | (_r) ); \
        asm_output("lsl %s,%s",gpn(_r),gpn(_s)); } while(0)



#define SHLi(_r,_imm) do {                                              \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x1B<<20) | ((_r)<<12) | ((_imm)<<7) | (LSL_imm<<4) | (_r) ); \
        asm_output("lsl %s,%d",gpn(_r),(_imm)); } while(0)
                    

#define TEST(_d,_s) do {                                                \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x11<<20) | ((_d)<<16) | (_s) ); \
        asm_output("test %s,%s",gpn(_d),gpn(_s)); } while(0)

#define TSTi(_d,_imm) do {                                              \
        underrunProtect(4);                                             \
        NanoAssert(((_imm) & 0xff) == (_imm));                          \
        *(--_nIns) = (NIns)( COND_AL | OP_IMM | (0x11<<20) | ((_d) << 16) | (0xF<<12) | ((_imm) & 0xff) ); \
        asm_output("tst %s,#0x%x", gpn(_d), _imm);                     \
    } while (0);


#define CMP(_l,_r)  do {                                                \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x015<<20) | ((_l)<<16) | (_r) ); \
        asm_output("cmp %s,%s",gpn(_l),gpn(_r)); } while(0)


#define CMPi(_r,_imm)  do {                                             \
        if (_imm<0) {                                                   \
            if ((_imm)>-256) {                                          \
                underrunProtect(4);                                     \
                *(--_nIns) = (NIns)( COND_AL | (0x37<<20) | ((_r)<<16) | (-(_imm)) ); \
            } else {                                                      \
                underrunProtect(4+LD32_size);                           \
                *(--_nIns) = (NIns)( COND_AL | (0x17<<20) | ((_r)<<16) | (Scratch) ); \
                LD32_nochk(Scratch, (_imm));                            \
            }                                                           \
        } else {                                                        \
            if ((_imm)<256) {                                           \
                underrunProtect(4);                                     \
                *(--_nIns) = (NIns)( COND_AL | (0x035<<20) | ((_r)<<16) | ((_imm)&0xFF) ); \
            } else {                                                    \
                underrunProtect(4+LD32_size);                           \
                *(--_nIns) = (NIns)( COND_AL | (0x015<<20) | ((_r)<<16) | (Scratch) ); \
                LD32_nochk(Scratch, (_imm));                            \
            }                                                           \
        }                                                               \
        asm_output("cmp %s,0x%x",gpn(_r),(_imm));                      \
    } while(0)


#define MR(_d,_s)  do {                                                 \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0xD<<21) | ((_d)<<12) | (_s) ); \
        asm_output("mov %s,%s",gpn(_d),gpn(_s)); } while (0)


#define MR_cond(_d,_s,_cond,_nm)  do {                                  \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( ((_cond)<<28) | (0xD<<21) | ((_d)<<12) | (_s) ); \
        asm_output(_nm " %s,%s",gpn(_d),gpn(_s)); } while (0)

#define MREQ(dr,sr) MR_cond(dr, sr, EQ, "moveq")
#define MRNE(dr,sr) MR_cond(dr, sr, NE, "movne")
#define MRL(dr,sr)  MR_cond(dr, sr, LT, "movlt")
#define MRLE(dr,sr) MR_cond(dr, sr, LE, "movle")
#define MRG(dr,sr)  MR_cond(dr, sr, GT, "movgt")
#define MRGE(dr,sr) MR_cond(dr, sr, GE, "movge")
#define MRB(dr,sr)  MR_cond(dr, sr, CC, "movcc")
#define MRBE(dr,sr) MR_cond(dr, sr, LS, "movls")
#define MRA(dr,sr)  MR_cond(dr, sr, HI, "movcs")
#define MRAE(dr,sr) MR_cond(dr, sr, CS, "movhi")
#define MRNO(dr,sr) MR_cond(dr, sr, VC, "movvc") // overflow clear
#define MRNC(dr,sr) MR_cond(dr, sr, CC, "movcc") // carry clear

#define LDR_chk(_d,_b,_off,_chk) do {                                   \
        if (IsFpReg(_d)) {                                              \
            FLDD_chk(_d,_b,_off,_chk);                                  \
        } else if ((_off) > -4096 && (_off) < 4096) {                   \
            if (_chk) underrunProtect(4);                               \
            *(--_nIns) = (NIns)( COND_AL | (((_off) < 0 ? 0x51 : 0x59)<<20) | ((_b)<<16) | ((_d)<<12) | (((_off) < 0 ? -(_off) : (_off))&0xFFF) ); \
        } else {                                                        \
            if (_chk) underrunProtect(4+LD32_size);                     \
            NanoAssert((_b) != IP);                                     \
            *(--_nIns) = (NIns)( COND_AL | (0x79<<20) | ((_b)<<16) | ((_d)<<12) | Scratch ); \
            LD32_nochk(Scratch, _off);                                  \
        }                                                               \
        asm_output("ldr %s, [%s, #%d]",gpn(_d),gpn(_b),(_off));        \
    } while(0)

#define LDR(_d,_b,_off)        LDR_chk(_d,_b,_off,1)
#define LDR_nochk(_d,_b,_off)  LDR_chk(_d,_b,_off,0)


#define LD(reg,offset,base)    LDR_chk(reg,base,offset,1)
#define ST(base,offset,reg)    STR(reg,base,offset)

#define LDi(_d,_imm) do {                                               \
        if ((_imm) == 0) {                                              \
            XOR(_d,_d);                                                 \
        } else if (isS8((_imm)) || isU8((_imm))) {                      \
            underrunProtect(4);                                         \
            if ((_imm)<0)   *(--_nIns) = (NIns)( COND_AL | (0x3E<<20) | ((_d)<<12) | (((_imm)^0xFFFFFFFF)&0xFF) ); \
            else            *(--_nIns) = (NIns)( COND_AL | (0x3B<<20) | ((_d)<<12) | ((_imm)&0xFF) ); \
            asm_output("ld  %s,0x%x",gpn((_d)),(_imm));                \
        } else {                                                        \
            underrunProtect(LD32_size);                                 \
            LD32_nochk(_d, (_imm));                                     \
            asm_output("ld  %s,0x%x",gpn((_d)),(_imm));                \
        }                                                               \
    } while(0)






#define LDRB(_d,_off,_b) do {                                           \
        NanoAssert((_off)>=0&&(_off)<=31);                              \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x5D<<20) | ((_b)<<16) | ((_d)<<12) | ((_off)&0xfff)  ); \
        asm_output("ldrb %s,%d(%s)", gpn(_d),(_off),gpn(_b));          \
    } while(0)


#define LDRH(_d,_off,_b) do {                  \
        NanoAssert((_off)>=0&&(_off)<=31);      \
        underrunProtect(4);                     \
        *(--_nIns) = (NIns)( COND_AL | (0x1D<<20) | ((_b)<<16) | ((_d)<<12) | ((0xB)<<4) | (((_off)&0xf0)<<4) | ((_off)&0xf) ); \
        asm_output("ldrsh %s,%d(%s)", gpn(_d),(_off),gpn(_b));         \
    } while(0)

#define STR(_d,_n,_off) do {                                            \
        NanoAssert(!IsFpReg(_d) && isS12(_off));                        \
        underrunProtect(4);                                             \
        if ((_off)<0)   *(--_nIns) = (NIns)( COND_AL | (0x50<<20) | ((_n)<<16) | ((_d)<<12) | ((-(_off))&0xFFF) ); \
        else            *(--_nIns) = (NIns)( COND_AL | (0x58<<20) | ((_n)<<16) | ((_d)<<12) | ((_off)&0xFFF) ); \
        asm_output("str %s, [%s, #%d]", gpn(_d), gpn(_n), (_off)); \
    } while(0)


#define STR_preindex(_d,_n,_off) do {                                   \
        NanoAssert(!IsFpReg(_d) && isS12(_off));                        \
        underrunProtect(4);                                             \
        if ((_off)<0)   *(--_nIns) = (NIns)( COND_AL | (0x52<<20) | ((_n)<<16) | ((_d)<<12) | ((-(_off))&0xFFF) ); \
        else            *(--_nIns) = (NIns)( COND_AL | (0x5A<<20) | ((_n)<<16) | ((_d)<<12) | ((_off)&0xFFF) ); \
        asm_output("str %s, [%s, #%d]", gpn(_d), gpn(_n), (_off));      \
    } while(0)


#define STR_postindex(_d,_n,_off) do {                                  \
        NanoAssert(!IsFpReg(_d) && isS12(_off));                        \
        underrunProtect(4);                                             \
        if ((_off)<0)   *(--_nIns) = (NIns)( COND_AL | (0x40<<20) | ((_n)<<16) | ((_d)<<12) | ((-(_off))&0xFFF) ); \
        else            *(--_nIns) = (NIns)( COND_AL | (0x48<<20) | ((_n)<<16) | ((_d)<<12) | ((_off)&0xFFF) ); \
        asm_output("str %s, [%s], %d", gpn(_d), gpn(_n), (_off));      \
    } while(0)


#define LEA(_r,_d,_b) do {                                              \
        NanoAssert((_d)<=1020);                                         \
        NanoAssert(((_d)&3)==0);                                        \
        if (_b!=SP) NanoAssert(0);                                      \
        if ((_d)<256) {                                                 \
            underrunProtect(4);                                         \
            *(--_nIns) = (NIns)( COND_AL | (0x28<<20) | ((_b)<<16) | ((_r)<<12) | ((_d)&0xFF) ); \
        } else {                                                        \
            underrunProtect(8);                                         \
            *(--_nIns) = (NIns)( COND_AL | (0x4<<21) | ((_b)<<16) | ((_r)<<12) | (2<<7)| (_r) ); \
            *(--_nIns) = (NIns)( COND_AL | (0x3B<<20) | ((_r)<<12) | (((_d)>>2)&0xFF) ); \
        }                                                               \
        asm_output("lea %s, %d(SP)", gpn(_r), _d);                     \
    } while(0)







#define BKPT_insn ((NIns)( (0xE<<24) | (0x12<<20) | (0x7<<4) ))
#define BKPT_nochk() do { \
        *(--_nIns) = BKPT_insn; } while (0)


#define NOP_nochk() do { \
        *(--_nIns) = (NIns)( COND_AL | (0xD<<21) | ((R0)<<12) | (R0) ); \
        asm_output("nop"); } while(0)


#define PUSHr(_r)  do {                                                 \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x92<<20) | (SP<<16) | (1<<(_r)) ); \
        asm_output("push %s",gpn(_r)); } while (0)


#define PUSH_mask(_mask)  do {                                          \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x92<<20) | (SP<<16) | (_mask) ); \
        asm_output("push %x", (_mask));} while (0)



#define PUSHm(_off,_b)  do {                                            \
        NanoAssert( (int)(_off)>0 );                                    \
        underrunProtect(8);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x92<<20) | (SP<<16) | (1<<(Scratch)) ); \
        *(--_nIns) = (NIns)( COND_AL | (0x59<<20) | ((_b)<<16) | ((Scratch)<<12) | ((_off)&0xFFF) ); \
        asm_output("push %d(%s)",(_off),gpn(_b)); } while (0)

#define POPr(_r) do {                                                   \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x8B<<20) | (SP<<16) | (1<<(_r)) ); \
        asm_output("pop %s",gpn(_r));} while (0)

#define POP_mask(_mask) do {                                            \
        underrunProtect(4);                                             \
        *(--_nIns) = (NIns)( COND_AL | (0x8B<<20) | (SP<<16) | (_mask) ); \
        asm_output("pop %x", (_mask));} while (0)



#define PC_OFFSET_FROM(target,frompc) ((intptr_t)(target) - ((intptr_t)(frompc) + 8))
#define isS12(offs) ((-(1<<12)) <= (offs) && (offs) < (1<<12))

#define B_cond(_c,_t)                           \
    B_cond_chk(_c,_t,1)


#define JMP(_t)                                 \
    B_cond_chk(AL,_t,1)

#define JMP_nochk(_t)                           \
    B_cond_chk(AL,_t,0)

#define JA(t)   do {B_cond(HI,t); asm_output("ja 0x%08x",(unsigned int)t); } while(0)
#define JNA(t)  do {B_cond(LS,t); asm_output("jna 0x%08x",(unsigned int)t); } while(0)
#define JB(t)   do {B_cond(CC,t); asm_output("jb 0x%08x",(unsigned int)t); } while(0)
#define JNB(t)  do {B_cond(CS,t); asm_output("jnb 0x%08x",(unsigned int)t); } while(0)
#define JE(t)   do {B_cond(EQ,t); asm_output("je 0x%08x",(unsigned int)t); } while(0)
#define JNE(t)  do {B_cond(NE,t); asm_output("jne 0x%08x",(unsigned int)t); } while(0)                     
#define JBE(t)  do {B_cond(LS,t); asm_output("jbe 0x%08x",(unsigned int)t); } while(0)
#define JNBE(t) do {B_cond(HI,t); asm_output("jnbe 0x%08x",(unsigned int)t); } while(0)
#define JAE(t)  do {B_cond(CS,t); asm_output("jae 0x%08x",(unsigned int)t); } while(0)
#define JNAE(t) do {B_cond(CC,t); asm_output("jnae 0x%08x",(unsigned int)t); } while(0)
#define JL(t)   do {B_cond(LT,t); asm_output("jl 0x%08x",(unsigned int)t); } while(0)  
#define JNL(t)  do {B_cond(GE,t); asm_output("jnl 0x%08x",(unsigned int)t); } while(0)
#define JLE(t)  do {B_cond(LE,t); asm_output("jle 0x%08x",(unsigned int)t); } while(0)
#define JNLE(t) do {B_cond(GT,t); asm_output("jnle 0x%08x",(unsigned int)t); } while(0)
#define JGE(t)  do {B_cond(GE,t); asm_output("jge 0x%08x",(unsigned int)t); } while(0)
#define JNGE(t) do {B_cond(LT,t); asm_output("jnge 0x%08x",(unsigned int)t); } while(0)
#define JG(t)   do {B_cond(GT,t); asm_output("jg 0x%08x",(unsigned int)t); } while(0)  
#define JNG(t)  do {B_cond(LE,t); asm_output("jng 0x%08x",(unsigned int)t); } while(0)
#define JC(t)   do {B_cond(CS,t); asm_output("bcs 0x%08x",(unsigned int)t); } while(0)
#define JNC(t)  do {B_cond(CC,t); asm_output("bcc 0x%08x",(unsigned int)t); } while(0)
#define JO(t)   do {B_cond(VS,t); asm_output("bvs 0x%08x",(unsigned int)t); } while(0)
#define JNO(t)  do {B_cond(VC,t); asm_output("bvc 0x%08x",(unsigned int)t); } while(0)



#define JP(t)   do {NanoAssert(0); B_cond(NE,t); asm_output("jp 0x%08x",t); } while(0) 


#define JNP(t)  do {NanoAssert(0); B_cond(EQ,t); asm_output("jnp 0x%08x",t); } while(0)




#define SET(_r,_cond,_opp) do {                                         \
    underrunProtect(8);                                                 \
    *(--_nIns) = (NIns)( (_opp<<28) | (1<<21) | ((_r)<<16) | ((_r)<<12) | (_r) ); \
    *(--_nIns) = (NIns)( (_cond<<28) | (0x3A<<20) | ((_r)<<12) | (1) ); \
    asm_output("mov%s %s, #1", ccName(_cond), gpn(r), gpn(r));          \
    asm_output("eor%s %s, %s", ccName(_opp), gpn(r), gpn(r));           \
    } while (0)


#define SETE(r)     SET(r,EQ,NE)
#define SETL(r)     SET(r,LT,GE)
#define SETLE(r)    SET(r,LE,GT)
#define SETG(r)     SET(r,GT,LE)
#define SETGE(r)    SET(r,GE,LT)
#define SETB(r)     SET(r,CC,CS)
#define SETBE(r)    SET(r,LS,HI)
#define SETAE(r)    SET(r,CS,CC)
#define SETA(r)     SET(r,HI,LS)
#define SETO(r)     SET(r,VS,LS)
#define SETC(r)     SET(r,CS,LS)



#define MOVZX8(r,r2)


#define MOVSX(_d,_off,_b) do {                                          \
        if ((_off)>=0) {                                                \
            if ((_off)<256) {                                           \
                underrunProtect(4);                                     \
                *(--_nIns) = (NIns)( COND_AL | (0x1D<<20) | ((_b)<<16) | ((_d)<<12) |  ((((_off)>>4)&0xF)<<8) | (0xF<<4) | ((_off)&0xF)  ); \
            } else if ((_off)<=510) {                                   \
                underrunProtect(8);                                     \
                int rem = (_off) - 255;                                 \
                NanoAssert(rem<256);                                    \
                *(--_nIns) = (NIns)( COND_AL | (0x1D<<20) | ((_d)<<16) | ((_d)<<12) |  ((((rem)>>4)&0xF)<<8) | (0xF<<4) | ((rem)&0xF)  ); \
                *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<23) | ((_b)<<16) | ((_d)<<12) | (0xFF) ); \
            } else {                                                    \
                underrunProtect(16);                                    \
                int rem = (_off) & 3;                                   \
                *(--_nIns) = (NIns)( COND_AL | (0x19<<20) | ((_b)<<16) | ((_d)<<12) | (0xF<<4) | (_d) ); \
                asm_output("ldrsh %s,[%s, #%d]",gpn(_d), gpn(_b), (_off)); \
                *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<23) | ((_d)<<16) | ((_d)<<12) | rem ); \
                *(--_nIns) = (NIns)( COND_AL | (0x1A<<20) | ((_d)<<12) | (2<<7)| (_d) ); \
                *(--_nIns) = (NIns)( COND_AL | (0x3B<<20) | ((_d)<<12) | (((_off)>>2)&0xFF) ); \
                asm_output("mov %s,%d",gpn(_d),(_off));                \
            }                                                           \
        } else {                                                        \
            if ((_off)>-256) {                                          \
                underrunProtect(4);                                     \
                *(--_nIns) = (NIns)( COND_AL | (0x15<<20) | ((_b)<<16) | ((_d)<<12) |  ((((-(_off))>>4)&0xF)<<8) | (0xF<<4) | ((-(_off))&0xF)  ); \
                asm_output("ldrsh %s,[%s, #%d]",gpn(_d), gpn(_b), (_off)); \
            } else if ((_off)>=-510){                                   \
                underrunProtect(8);                                     \
                int rem = -(_off) - 255;                                \
                NanoAssert(rem<256);                                    \
                *(--_nIns) = (NIns)( COND_AL | (0x15<<20) | ((_d)<<16) | ((_d)<<12) |  ((((rem)>>4)&0xF)<<8) | (0xF<<4) | ((rem)&0xF)  ); \
                *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<22) | ((_b)<<16) | ((_d)<<12) | (0xFF) ); \
            } else NanoAssert(0);                                        \
        }                                                               \
    } while(0)

#define STMIA(_b, _mask) do {                                           \
        underrunProtect(4);                                             \
        NanoAssert(((_mask)&rmask(_b))==0 && isU8(_mask));              \
        *(--_nIns) = (NIns)(COND_AL | (0x8A<<20) | ((_b)<<16) | (_mask)&0xFF); \
        asm_output("stmia %s!,{0x%x}", gpn(_b), _mask); \
    } while (0)

#define LDMIA(_b, _mask) do {                                           \
        underrunProtect(4);                                             \
        NanoAssert(((_mask)&rmask(_b))==0 && isU8(_mask));              \
        *(--_nIns) = (NIns)(COND_AL | (0x8B<<20) | ((_b)<<16) | (_mask)&0xFF); \
        asm_output("ldmia %s!,{0x%x}", gpn(_b), (_mask)); \
    } while (0)

#define MRS(_d) do {                            \
        underrunProtect(4);                     \
        *(--_nIns) = (NIns)(COND_AL | (0x10<<20) | (0xF<<16) | ((_d)<<12)); \
        asm_output("msr %s", gpn(_d));                                 \
    } while (0)





#define FMDRR(_Dm,_Rd,_Rn) do {                                         \
        underrunProtect(4);                                             \
        NanoAssert(IsFpReg(_Dm) && IsGpReg(_Rd) && IsGpReg(_Rn));       \
        *(--_nIns) = (NIns)( COND_AL | (0xC4<<20) | ((_Rn)<<16) | ((_Rd)<<12) | (0xB1<<4) | (FpRegNum(_Dm)) ); \
        asm_output("fmdrr %s,%s,%s", gpn(_Dm), gpn(_Rd), gpn(_Rn));    \
    } while (0)

#define FMRRD(_Rd,_Rn,_Dm) do {                                         \
        underrunProtect(4);                                             \
        NanoAssert(IsGpReg(_Rd) && IsGpReg(_Rn) && IsFpReg(_Dm));       \
        *(--_nIns) = (NIns)( COND_AL | (0xC5<<20) | ((_Rn)<<16) | ((_Rd)<<12) | (0xB1<<4) | (FpRegNum(_Dm)) ); \
        asm_output("fmrrd %s,%s,%s", gpn(_Rd), gpn(_Rn), gpn(_Dm));    \
    } while (0)

#define FMRDH(_Rd,_Dn) do {                                             \
        underrunProtect(4);                                             \
        NanoAssert(IsGpReg(_Rd) && IsFpReg(_Dm));                       \
        *(--_nIns) = (NIns)( COND_AL | (0xE3<<20) | (FpRegNum(_Dn)<<16) | ((_Rd)<<12) | (0xB<<8) | (1<<4) ); \
        asm_output("fmrdh %s,%s", gpn(_Rd), gpn(_Dn));                  \
    } while (0)

#define FSTD(_Dd,_Rn,_offs) do {                                        \
        underrunProtect(4);                                             \
        NanoAssert((((_offs) & 3) == 0) && isS8((_offs) >> 2));         \
        NanoAssert(IsFpReg(_Dd) && !IsFpReg(_Rn));                      \
        int negflag = 1<<23;                                            \
        intptr_t offs = (_offs);                                        \
        if (_offs < 0) {                                                \
            negflag = 0<<23;                                            \
            offs = -(offs);                                             \
        }                                                               \
        *(--_nIns) = (NIns)( COND_AL | (0xD0<<20) | ((_Rn)<<16) | (FpRegNum(_Dd)<<12) | (0xB<<8) | negflag | ((offs>>2)&0xff) ); \
        asm_output("fstd %s,%s(%d)", gpn(_Dd), gpn(_Rn), _offs);    \
    } while (0)

#define FLDD_chk(_Dd,_Rn,_offs,_chk) do {                               \
        if(_chk) underrunProtect(4);                                    \
        NanoAssert((((_offs) & 3) == 0) && isS8((_offs) >> 2));         \
        NanoAssert(IsFpReg(_Dd) && !IsFpReg(_Rn));                      \
        int negflag = 1<<23;                                            \
        intptr_t offs = (_offs);                                        \
        if (_offs < 0) {                                                \
            negflag = 0<<23;                                            \
            offs = -(offs);                                             \
        }                                                               \
        *(--_nIns) = (NIns)( COND_AL | (0xD1<<20) | ((_Rn)<<16) | (FpRegNum(_Dd)<<12) | (0xB<<8) | negflag | ((offs>>2)&0xff) ); \
        asm_output("fldd %s,%s(%d)", gpn(_Dd), gpn(_Rn), _offs);       \
    } while (0)
#define FLDD(_Dd,_Rn,_offs) FLDD_chk(_Dd,_Rn,_offs,1)

#define FSITOD(_Dd,_Sm) do {                                            \
        underrunProtect(4);                                             \
        NanoAssert(IsFpReg(_Dd) && ((_Sm) == FpSingleScratch));         \
        *(--_nIns) = (NIns)( COND_AL | (0xEB8<<16) | (FpRegNum(_Dd)<<12) | (0x2F<<6) | (0<<5) | (0x7) ); \
        asm_output("fsitod %s,%s", gpn(_Dd), gpn(_Sm));                \
    } while (0)


#define FUITOD(_Dd,_Sm) do {                                            \
        underrunProtect(4);                                             \
        NanoAssert(IsFpReg(_Dd) && ((_Sm) == FpSingleScratch));         \
        *(--_nIns) = (NIns)( COND_AL | (0xEB8<<16) | (FpRegNum(_Dd)<<12) | (0x2D<<6) | (0<<5) | (0x7) ); \
        asm_output("fuitod %s,%s", gpn(_Dd), gpn(_Sm));                \
    } while (0)

#define FMSR(_Sn,_Rd) do {                                              \
        underrunProtect(4);                                             \
        NanoAssert(((_Sn) == FpSingleScratch) && IsGpReg(_Rd));         \
        *(--_nIns) = (NIns)( COND_AL | (0xE0<<20) | (0x7<<16) | ((_Rd)<<12) | (0xA<<8) | (0<<7) | (0x1<<4) ); \
        asm_output("fmsr %s,%s", gpn(_Sn), gpn(_Rd));                  \
    } while (0)

#define FNEGD(_Dd,_Dm) do {                                             \
        underrunProtect(4);                                             \
        NanoAssert(IsFpReg(_Dd) && IsFpReg(_Dm));                       \
        *(--_nIns) = (NIns)( COND_AL | (0xEB1<<16) | (FpRegNum(_Dd)<<12) | (0xB4<<4) | (FpRegNum(_Dm)) ); \
        asm_output("fnegd %s,%s", gpn(_Dd), gpn(_Dm));                 \
    } while (0)

#define FADDD(_Dd,_Dn,_Dm) do {                                         \
        underrunProtect(4);                                             \
        NanoAssert(IsFpReg(_Dd) && IsFpReg(_Dn) && IsFpReg(_Dm));       \
        *(--_nIns) = (NIns)( COND_AL | (0xE3<<20) | (FpRegNum(_Dn)<<16) | (FpRegNum(_Dd)<<12) | (0xB0<<4) | (FpRegNum(_Dm)) ); \
        asm_output("faddd %s,%s,%s", gpn(_Dd), gpn(_Dn), gpn(_Dm));    \
    } while (0)

#define FSUBD(_Dd,_Dn,_Dm) do {                                         \
        underrunProtect(4);                                             \
        NanoAssert(IsFpReg(_Dd) && IsFpReg(_Dn) && IsFpReg(_Dm));       \
        *(--_nIns) = (NIns)( COND_AL | (0xE3<<20) | (FpRegNum(_Dn)<<16) | (FpRegNum(_Dd)<<12) | (0xB4<<4) | (FpRegNum(_Dm)) ); \
        asm_output("fsubd %s,%s,%s", gpn(_Dd), gpn(_Dn), gpn(_Dm));    \
    } while (0)

#define FMULD(_Dd,_Dn,_Dm) do {                                         \
        underrunProtect(4);                                             \
        NanoAssert(IsFpReg(_Dd) && IsFpReg(_Dn) && IsFpReg(_Dm));       \
        *(--_nIns) = (NIns)( COND_AL | (0xE2<<20) | (FpRegNum(_Dn)<<16) | (FpRegNum(_Dd)<<12) | (0xB0<<4) | (FpRegNum(_Dm)) ); \
        asm_output("fmuld %s,%s,%s", gpn(_Dd), gpn(_Dn), gpn(_Dm));    \
    } while (0)

#define FDIVD(_Dd,_Dn,_Dm) do {                                         \
        underrunProtect(4);                                             \
        NanoAssert(IsFpReg(_Dd) && IsFpReg(_Dn) && IsFpReg(_Dm));       \
        *(--_nIns) = (NIns)( COND_AL | (0xE8<<20) | (FpRegNum(_Dn)<<16) | (FpRegNum(_Dd)<<12) | (0xB0<<4) | (FpRegNum(_Dm)) ); \
        asm_output("fmuld %s,%s,%s", gpn(_Dd), gpn(_Dn), gpn(_Dm));    \
    } while (0)

#define FMSTAT() do {                               \
        underrunProtect(4);                         \
        *(--_nIns) = (NIns)( COND_AL | 0x0EF1FA10); \
        asm_output("fmstat");                       \
    } while (0)

#define FCMPD(_Dd,_Dm) do {                                             \
        underrunProtect(4);                                             \
        NanoAssert(IsFpReg(_Dd) && IsFpReg(_Dm));                       \
        *(--_nIns) = (NIns)( COND_AL | (0xEB4<<16) | (FpRegNum(_Dd)<<12) | (0xB4<<4) | (FpRegNum(_Dm)) ); \
        asm_output("fcmpd %s,%s", gpn(_Dd), gpn(_Dm));                 \
    } while (0)

#define FCPYD(_Dd,_Dm) do {                                             \
        underrunProtect(4);                                             \
        NanoAssert(IsFpReg(_Dd) && IsFpReg(_Dm));                       \
        *(--_nIns) = (NIns)( COND_AL | (0xEB0<<16) | (FpRegNum(_Dd)<<12) | (0xB4<<4) | (FpRegNum(_Dm)) ); \
        asm_output("fcpyd %s,%s", gpn(_Dd), gpn(_Dm));                 \
    } while (0)
}
#endif 
