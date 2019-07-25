









































#include "nanojit.h"

#ifdef UNDER_CE
#include <cmnintrin.h>
#endif

#if defined(FEATURE_NANOJIT) && defined(NANOJIT_ARM)

namespace nanojit
{

#ifdef NJ_VERBOSE
const char* regNames[] = {"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","fp","ip","sp","lr","pc",
                          "d0","d1","d2","d3","d4","d5","d6","d7","s0"};
const char* condNames[] = {"eq","ne","cs","cc","mi","pl","vs","vc","hi","ls","ge","lt","gt","le","","nv"};
const char* shiftNames[] = { "lsl", "lsl", "lsr", "lsr", "asr", "asr", "ror", "ror" };
#endif

const Register Assembler::argRegs[] = { R0, R1, R2, R3 };
const Register Assembler::retRegs[] = { R0, R1 };
const Register Assembler::savedRegs[] = { R4, R5, R6, R7, R8, R9, R10 };





#ifdef DEBUG









inline bool
Assembler::isOp2Imm(uint32_t enc)
{
    return ((enc & 0xfff) == enc);
}


inline uint32_t
Assembler::decOp2Imm(uint32_t enc)
{
    NanoAssert(isOp2Imm(enc));

    uint32_t    imm8 = enc & 0xff;
    uint32_t    rot = 32 - ((enc >> 7) & 0x1e);

    return imm8 << (rot & 0x1f);
}
#endif


static inline uint32_t
CountLeadingZeroesSlow(uint32_t data)
{
    
    
    uint32_t    try_shift;

    uint32_t    leading_zeroes = 0;

    
    
    for (try_shift = 16; try_shift != 0; try_shift /= 2) {
        uint32_t    shift = leading_zeroes + try_shift;
        if (((data << shift) >> shift) == data) {
            leading_zeroes = shift;
        }
    }

    return leading_zeroes;
}

inline uint32_t
Assembler::CountLeadingZeroes(uint32_t data)
{
    uint32_t    leading_zeroes;

#if defined(__ARMCC__)
    
    leading_zeroes = __clz(data);
#elif defined(__GNUC__)
    
    if (ARM_ARCH_AT_LEAST(5)) {
        __asm (
#if defined(ANDROID) && (NJ_COMPILER_ARM_ARCH < 7)
        
        
            "   .arch armv7-a\n"
#elif (NJ_COMPILER_ARM_ARCH < 5)
        
        
            "   .arch armv5t\n"
#endif
            "   clz     %0, %1  \n"
            :   "=r"    (leading_zeroes)
            :   "r"     (data)
        );
    } else {
        leading_zeroes = CountLeadingZeroesSlow(data);
    }
#elif defined(UNDER_CE)
    
    leading_zeroes = _CountLeadingZeros(data);
#else
    leading_zeroes = CountLeadingZeroesSlow(data);
#endif

    
    NanoAssert(((0xffffffff >> leading_zeroes) & data) == data);

    return leading_zeroes;
}














inline bool
Assembler::encOp2Imm(uint32_t literal, uint32_t * enc)
{
    
    
    uint32_t    leading_zeroes;

    
    int32_t    rot;
    uint32_t    imm8;

    
    
    
    if (literal < 256)
    {
        *enc = literal;
        return true;
    }

    
    
    leading_zeroes = CountLeadingZeroes(literal);

    
    
    
    
    
    NanoAssert(leading_zeroes < 24);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    rot = 24 - (leading_zeroes & ~1);

    
    
    imm8 = literal >> rot;

    
    
    
    if (literal != (imm8 << rot)) {
        
        
        return false;
    }

    
    
    
    
    *enc = ((-rot << 7) & 0xf00) | imm8;

    
    NanoAssert(decOp2Imm(*enc) == literal);

    return true;
}









void
Assembler::asm_add_imm(Register rd, Register rn, int32_t imm, int stat )
{
    
    uint32_t    op2imm;

    NanoAssert(IsGpReg(rd));
    NanoAssert(IsGpReg(rn));
    NanoAssert((stat & 1) == stat);

    
    
    
    if ((imm == 0) && (stat == 0) && (rd == rn)) {
        return;
    }

    
    
    if (encOp2Imm(imm, &op2imm)) {
        ADDis(rd, rn, op2imm, stat);
    } else if (encOp2Imm(-imm, &op2imm)) {
        
        
        
        NanoAssert(imm != 0);
        SUBis(rd, rn, op2imm, stat);
    } else {
        
        
        
        
        Register    rm = (rn == IP) ? (rd) : (IP);
        NanoAssert(rn != rm);

        ADDs(rd, rn, rm, stat);
        asm_ld_imm(rm, imm);
    }
}









void
Assembler::asm_sub_imm(Register rd, Register rn, int32_t imm, int stat )
{
    
    uint32_t    op2imm;

    NanoAssert(IsGpReg(rd));
    NanoAssert(IsGpReg(rn));
    NanoAssert((stat & 1) == stat);

    
    
    
    if ((imm == 0) && (stat == 0) && (rd == rn)) {
        return;
    }

    
    
    if (encOp2Imm(imm, &op2imm)) {
        SUBis(rd, rn, op2imm, stat);
    } else if (encOp2Imm(-imm, &op2imm)) {
        
        
        
        NanoAssert(imm != 0);
        ADDis(rd, rn, op2imm, stat);
    } else {
        
        
        
        
        Register    rm = (rn == IP) ? (rd) : (IP);
        NanoAssert(rn != rm);

        SUBs(rd, rn, rm, stat);
        asm_ld_imm(rm, imm);
    }
}









void
Assembler::asm_and_imm(Register rd, Register rn, int32_t imm, int stat )
{
    
    uint32_t    op2imm;

    NanoAssert(IsGpReg(rd));
    NanoAssert(IsGpReg(rn));
    NanoAssert((stat & 1) == stat);

    
    
    if (encOp2Imm(imm, &op2imm)) {
        ANDis(rd, rn, op2imm, stat);
    } else if (encOp2Imm(~imm, &op2imm)) {
        
        BICis(rd, rn, op2imm, stat);
    } else {
        
        
        
        
        Register    rm = (rn == IP) ? (rd) : (IP);
        NanoAssert(rn != rm);

        ANDs(rd, rn, rm, stat);
        asm_ld_imm(rm, imm);
    }
}









void
Assembler::asm_orr_imm(Register rd, Register rn, int32_t imm, int stat )
{
    
    uint32_t    op2imm;

    NanoAssert(IsGpReg(rd));
    NanoAssert(IsGpReg(rn));
    NanoAssert((stat & 1) == stat);

    
    
    if (encOp2Imm(imm, &op2imm)) {
        ORRis(rd, rn, op2imm, stat);
    } else {
        
        
        
        
        Register    rm = (rn == IP) ? (rd) : (IP);
        NanoAssert(rn != rm);

        ORRs(rd, rn, rm, stat);
        asm_ld_imm(rm, imm);
    }
}









void
Assembler::asm_eor_imm(Register rd, Register rn, int32_t imm, int stat )
{
    
    uint32_t    op2imm;

    NanoAssert(IsGpReg(rd));
    NanoAssert(IsGpReg(rn));
    NanoAssert((stat & 1) == stat);

    
    
    if (encOp2Imm(imm, &op2imm)) {
        EORis(rd, rn, op2imm, stat);
    } else {
        
        
        
        
        Register    rm = (rn == IP) ? (rd) : (IP);
        NanoAssert(rn != rm);

        EORs(rd, rn, rm, stat);
        asm_ld_imm(rm, imm);
    }
}





void
Assembler::nInit()
{
    nHints[LIR_calli]  = rmask(retRegs[0]);
    nHints[LIR_hcalli] = rmask(retRegs[1]);
    nHints[LIR_paramp] = PREFER_SPECIAL;
}

void Assembler::nBeginAssembly()
{
    max_out_args = 0;
}

NIns*
Assembler::genPrologue()
{
    



    
    
    uint32_t stackNeeded = max_out_args + STACK_GRANULARITY * _activation.stackSlotsNeeded();
    uint32_t savingCount = 2;

    uint32_t savingMask = rmask(FP) | rmask(LR);

    
    uint32_t stackPushed = STACK_GRANULARITY * savingCount;
    uint32_t aligned = alignUp(stackNeeded + stackPushed, NJ_ALIGN_STACK);
    int32_t amt = aligned - stackPushed;

    
    if (amt)
        asm_sub_imm(SP, SP, amt);

    verbose_only( asm_output("## %p:",(void*)_nIns); )
    verbose_only( asm_output("## patch entry"); )
    NIns *patchEntry = _nIns;

    MOV(FP, SP);
    PUSH_mask(savingMask);
    return patchEntry;
}

void
Assembler::nFragExit(LIns* guard)
{
    SideExit *  exit = guard->record()->exit;
    Fragment *  frag = exit->target;

    bool        target_is_known = frag && frag->fragEntry;

    if (target_is_known) {
        
        JMP_far(frag->fragEntry);
    } else {
        
        

        GuardRecord *gr = guard->record();

        if (!_epilogue)
            _epilogue = genEpilogue();

        
        
        
        JMP_far(_epilogue);

        
        
        
        gr->jmp = _nIns;

        
        
        
        
        
        
        
        

        asm_ld_imm(IP, int(gr));
    }

#ifdef NJ_VERBOSE
    if (_config.arm_show_stats) {
        
        
        int fromfrag = int((Fragment*)_thisfrag);
        asm_ld_imm(argRegs[1], fromfrag);
    }
#endif

    
    verbose_only(
       if (_logc->lcbits & LC_FragProfile) {
           asm_inc_m32( &guard->record()->profCount );
       }
    )

    
    MOV(SP, FP);
}

NIns*
Assembler::genEpilogue()
{
    RegisterMask savingMask;

    if (ARM_ARCH_AT_LEAST(5)) {
        
        savingMask = rmask(FP) | rmask(PC);

    } else {
        
        
        savingMask = rmask(FP) | rmask(LR);
        BX(LR);
    }
    POP_mask(savingMask); 

    
    
    
    MOV(R0, IP);

    return _nIns;
}



























void
Assembler::asm_arg(ArgType ty, LIns* arg, ParameterRegisters& params)
{
    
    NanoAssert((params.stkd & 3) == 0);

    if (ty == ARGTYPE_D) {
        
        asm_arg_64(arg, params);
    } else {
        NanoAssert(ty == ARGTYPE_I || ty == ARGTYPE_UI);
        
        if (params.r < R4) {
            asm_regarg(ty, arg, params.r);
            params.r = Register(params.r + 1);
        } else {
            asm_stkarg(arg, params.stkd);
            params.stkd += 4;
        }
    }
}





#ifdef NJ_ARM_EABI_HARD_FLOAT
void
Assembler::asm_arg_64(LIns* arg, ParameterRegisters& params)
{
    NanoAssert(IsFpReg(params.float_r));
    if (params.float_r <= D7) {
        findSpecificRegFor(arg, params.float_r);
        params.float_r = Register(params.float_r + 1);
    } else {
        NanoAssertMsg(0, "Only 8 floating point arguments supported");
    }
}

#else
void
Assembler::asm_arg_64(LIns* arg, ParameterRegisters& params)
{
    
    NanoAssert((params.stkd & 3) == 0);
    
    
    NanoAssert(ARM_VFP || arg->isop(LIR_ii2d));

#ifdef NJ_ARM_EABI
    
    
    
    
    
    if ((params.r == R1) || (params.r == R3)) {
        params.r = Register(params.r + 1);
    }
#endif

    if (params.r < R3) {
        Register    ra = params.r;
        Register    rb = Register(params.r + 1);
        params.r = Register(rb + 1);

#ifdef NJ_ARM_EABI
        
        
        NanoAssert( ((ra == R0) && (rb == R1)) || ((ra == R2) && (rb == R3)) );
#endif

        
        
        
        if (ARM_VFP) {
            Register dm = findRegFor(arg, FpRegs);
            FMRRD(ra, rb, dm);
        } else {
            asm_regarg(ARGTYPE_I, arg->oprnd1(), ra);
            asm_regarg(ARGTYPE_I, arg->oprnd2(), rb);
        }

#ifndef NJ_ARM_EABI
    } else if (params.r == R3) {
        
        
        
        Register    ra = params.r; 
        params.r = R4;

        
        
        NanoAssert(params.stkd == 0);

        if (ARM_VFP) {
            Register dm = findRegFor(arg, FpRegs);
            
            
            

            
            
            STR(IP, SP, 0);
            FMRRD(ra, IP, dm);
        } else {
            
            
            
            asm_regarg(ARGTYPE_I, arg->oprnd1(), ra);
            asm_stkarg(arg->oprnd2(), 0);
        }
        params.stkd += 4;
#endif
    } else {
        
#ifdef NJ_ARM_EABI
        
        if ((params.stkd & 7) != 0) {
            
            
            params.stkd += 4;
        }
#endif
        if (ARM_VFP) {
            asm_stkarg(arg, params.stkd);
        } else {
            asm_stkarg(arg->oprnd1(), params.stkd);
            asm_stkarg(arg->oprnd2(), params.stkd+4);
        }
        params.stkd += 8;
    }
}
#endif 

void
Assembler::asm_regarg(ArgType ty, LIns* p, Register rd)
{
    
    
    

    if (ty == ARGTYPE_I || ty == ARGTYPE_UI)
    {
        
        if (p->isImmI()) {
            asm_ld_imm(rd, p->immI());
        } else {
            if (p->isInReg()) {
                MOV(rd, p->getReg());
            } else {
                
                
                findSpecificRegForUnallocated(p, rd);
            }
        }
    } else {
        NanoAssert(ty == ARGTYPE_D);
        
        NanoAssert(false);
    }
}

void
Assembler::asm_stkarg(LIns* arg, int stkd)
{
    
    NanoAssert(stkd >= 0);
    
    
    if (arg->isI()) {
        Register rt = findRegFor(arg, GpRegs);
        asm_str(rt, SP, stkd);
    } else {
        
        
        
        
        NanoAssert(arg->isD());
        NanoAssert(ARM_VFP);
        Register dt = findRegFor(arg, FpRegs);
#ifdef NJ_ARM_EABI
        
        NanoAssert((stkd % 8) == 0);
#endif
        FSTD(dt, SP, stkd);
    }
}

void
Assembler::asm_call(LIns* ins)
{
    if (ARM_VFP && ins->isop(LIR_calld)) {
        




















#ifdef NJ_ARM_EABI_HARD_FLOAT
        



        prepareResultReg(ins, rmask(D0));
        freeResourcesOf(ins);
#endif
    } else if (!ins->isop(LIR_callv)) {
        prepareResultReg(ins, rmask(retRegs[0]));
        
        
        freeResourcesOf(ins);
    }

    
    

    evictScratchRegsExcept(0);

    const CallInfo* ci = ins->callInfo();
    ArgType argTypes[MAXARGS];
    uint32_t argc = ci->getArgTypes(argTypes);
    bool indirect = ci->isIndirect();

    
    
    NanoAssert(ARM_VFP || ins->isop(LIR_callv) || ins->isop(LIR_calli));

    
    
    
    
    
    if (!ARM_EABI_HARD && ARM_VFP && ins->isExtant()) {
        
        
        if (ci->returnType() == ARGTYPE_D) {
            NanoAssert(ins->isop(LIR_calld));

            if (ins->isInReg()) {
                Register dd = ins->getReg();
                
                FMDRR(dd, R0, R1);
            } else {
                int d = findMemFor(ins);
                
                
                freeResourcesOf(ins);

                
                
                asm_str(R0, FP, d+0);
                asm_str(R1, FP, d+4);
            }
        }
    }

    
    if (!indirect) {
        verbose_only(if (_logc->lcbits & LC_Native)
            outputf("        %p:", _nIns);
        )

        BranchWithLink((NIns*)ci->_address);
    } else {
        
        if (ARM_ARCH_AT_LEAST(5)) {
#ifndef UNDER_CE
            
            underrunProtect(8);
            BLX(IP);
            MOV(IP, LR);
#else
            BLX(LR);
#endif
        } else {
            underrunProtect(12);
            BX(IP);
            MOV(LR, PC);
            MOV(IP, LR);
        }
        asm_regarg(ARGTYPE_I, ins->arg(--argc), LR);
    }

    
    
    ParameterRegisters params = init_params(0, R0, D0);

    
    
    
    
    uint32_t    i = argc;
    while(i--) {
        asm_arg(argTypes[i], ins->arg(i), params);
    }

    if (params.stkd > max_out_args) {
        max_out_args = params.stkd;
    }
}

Register
Assembler::nRegisterAllocFromSet(RegisterMask set)
{
    NanoAssert(set != 0);

    
    
    
    Register r = (Register)(31-CountLeadingZeroes(set));
    _allocator.free &= ~rmask(r);

    NanoAssert(IsGpReg(r) || IsFpReg(r));
    NanoAssert((rmask(r) & set) == rmask(r));

    return r;
}

void
Assembler::nRegisterResetAll(RegAlloc& a)
{
    
    a.clear();
    a.free =
        rmask(R0) | rmask(R1) | rmask(R2) | rmask(R3) | rmask(R4) |
        rmask(R5) | rmask(R6) | rmask(R7) | rmask(R8) | rmask(R9) |
        rmask(R10) | rmask(LR);
    if (ARM_VFP) {
        a.free |=
            rmask(D0) | rmask(D1) | rmask(D2) | rmask(D3) |
            rmask(D4) | rmask(D5) | rmask(D6) | rmask(D7);
    }
}

static inline ConditionCode
get_cc(NIns *ins)
{
    return ConditionCode((*ins >> 28) & 0xF);
}

static inline bool
branch_is_B(NIns* branch)
{
    return (*branch & 0x0E000000) == 0x0A000000;
}

static inline bool
branch_is_LDR_PC(NIns* branch)
{
    return (*branch & 0x0F7FF000) == 0x051FF000;
}


static inline bool
is_ldstr_reg_fp_minus_imm(uint32_t* isLoad, uint32_t* rX,
                          uint32_t* immX, NIns i1)
{
    if ((i1 & 0xFFEF0000) != 0xE50B0000)
        return false;
    *isLoad = (i1 >> 20) & 1;
    *rX     = (i1 >> 12) & 0xF;
    *immX   = i1 & 0xFFF;
    return true;
}


static inline bool
is_ldstmdb_fp(uint32_t* isLoad, uint32_t* regSet, NIns i1)
{
    if ((i1 & 0xFFEF0000) != 0xE90B0000)
        return false;
    *isLoad = (i1 >> 20) & 1;
    *regSet = i1 & 0xFFFF;
    return true;
}


static inline NIns
mk_ldstmdb_fp(uint32_t isLoad, uint32_t regSet)
{
    return 0xE90B0000 | (regSet & 0xFFFF) | ((isLoad & 1) << 20);
}


static inline uint32_t
size_of_regSet(uint32_t regSet)
{
   uint32_t x = regSet;
   x = (x & 0x5555) + ((x >> 1) & 0x5555);
   x = (x & 0x3333) + ((x >> 2) & 0x3333);
   x = (x & 0x0F0F) + ((x >> 4) & 0x0F0F);
   x = (x & 0x00FF) + ((x >> 8) & 0x00FF);
   return x;
}


static bool
do_peep_2_1(NIns* merged, NIns i1, NIns i2)
{
    uint32_t rX, rY, immX, immY, isLoadX, isLoadY, regSet;
    






    if (is_ldstr_reg_fp_minus_imm(&isLoadX, &rX, &immX, i1) &&
        is_ldstr_reg_fp_minus_imm(&isLoadY, &rY, &immY, i2) &&
        immX == 8 && immY == 4 && rX < rY &&
        isLoadX == isLoadY &&
        rX != FP && rY != FP &&
         rX != 15 && rY != 15) {
        *merged = mk_ldstmdb_fp(isLoadX, (1 << rX) | (1<<rY));
        return true;
    }
    









    if (is_ldstr_reg_fp_minus_imm(&isLoadX, &rX, &immX, i1) &&
        is_ldstmdb_fp(&isLoadY, &regSet, i2) &&
        regSet != 0 &&
        (regSet & ((1 << (rX + 1)) - 1)) == 0 &&
        immX == 4 * (1 + size_of_regSet(regSet)) &&
        isLoadX == isLoadY &&
        rX != FP && rX != 15) {
        *merged = mk_ldstmdb_fp(isLoadX, regSet | (1 << rX));
        return true;
    }
    return false;
}



static inline bool
does_next_instruction_exist(NIns* _nIns, NIns* codeStart, NIns* codeEnd,
                            NIns* exitStart, NIns* exitEnd)
{
    return (exitStart <= _nIns && _nIns+1 < exitEnd) ||
           (codeStart <= _nIns && _nIns+1 < codeEnd);
}

void
Assembler::nPatchBranch(NIns* branch, NIns* target)
{
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if (branch_is_B(branch)) {
        
        
        NanoAssert(get_cc(branch) == AL);

        int32_t offset = PC_OFFSET_FROM(target, branch);
        if (isS24(offset>>2)) {
            
            NIns cond = *branch & 0xF0000000;
            *branch = (NIns)( cond | (0xA<<24) | ((offset>>2) & 0xFFFFFF) );
        } else {
            
            
            NanoAssert(*(branch+1) == BKPT_insn);

            
            NIns cond = *branch & 0xF0000000;
            *branch++ = (NIns)( cond | (0x51<<20) | (PC<<16) | (PC<<12) | (4));
            *branch++ = (NIns)target;
        }
    } else {
        
        
        NanoAssert(branch_is_LDR_PC(branch));

        NIns *addr = branch+2;
        int offset = (*branch & 0xFFF) / sizeof(NIns);
        if (*branch & (1<<23)) {
            addr += offset;
        } else {
            addr -= offset;
        }

        
        *addr = (NIns) target;
    }
}

RegisterMask
Assembler::nHint(LIns* ins)
{
    NanoAssert(ins->isop(LIR_paramp));
    RegisterMask prefer = 0;
    if (ins->paramKind() == 0)
        if (ins->paramArg() < 4)
            prefer = rmask(argRegs[ins->paramArg()]);
    return prefer;
}

void
Assembler::asm_qjoin(LIns *ins)
{
    int d = findMemFor(ins);
    NanoAssert(d);
    LIns* lo = ins->oprnd1();
    LIns* hi = ins->oprnd2();

    Register rlo;
    Register rhi;

    findRegFor2(GpRegs, lo, rlo, GpRegs, hi, rhi);

    asm_str(rhi, FP, d+4);
    asm_str(rlo, FP, d);

    freeResourcesOf(ins);
}

void
Assembler::asm_store32(LOpcode op, LIns *value, int dr, LIns *base)
{
    Register ra, rb;
    getBaseReg2(GpRegs, value, ra, GpRegs, base, rb, dr);

    switch (op) {
        case LIR_sti:
            if (isU12(-dr) || isU12(dr)) {
                STR(ra, rb, dr);
            } else {
                STR(ra, IP, 0);
                asm_add_imm(IP, rb, dr);
            }
            return;
        case LIR_sti2c:
            if (isU12(-dr) || isU12(dr)) {
                STRB(ra, rb, dr);
            } else {
                STRB(ra, IP, 0);
                asm_add_imm(IP, rb, dr);
            }
            return;
        case LIR_sti2s:
            
            if (isU8(-dr) || isU8(dr)) {
                STRH(ra, rb, dr);
            } else {
                STRH(ra, IP, 0);
                asm_add_imm(IP, rb, dr);
            }
            return;
        default:
            NanoAssertMsg(0, "asm_store32 should never receive this LIR opcode");
            return;
    }
}

bool
canRematALU(LIns *ins)
{
    
    
    switch (ins->opcode()) {
    case LIR_addi:
    case LIR_subi:
    case LIR_andi:
    case LIR_ori:
    case LIR_xori:
        return ins->oprnd1()->isInReg() && ins->oprnd2()->isImmI();
    default:
        ;
    }
    return false;
}

bool
Assembler::canRemat(LIns* ins)
{
    return ins->isImmI() || ins->isop(LIR_allocp) || canRematALU(ins);
}

void
Assembler::asm_restore(LIns* i, Register r)
{
    
    NanoAssert(r != PC);
    NanoAssert(r != IP);
    NanoAssert(r != SP);

    if (i->isop(LIR_allocp)) {
        int d = findMemFor(i);
        asm_add_imm(r, FP, d);
    } else if (i->isImmI()) {
        asm_ld_imm(r, i->immI());
    } else if (canRematALU(i)) {
        Register rn = i->oprnd1()->getReg();
        int32_t imm = i->oprnd2()->immI();
        switch (i->opcode()) {
        case LIR_addi: asm_add_imm(r, rn, imm,  0); break;
        case LIR_subi: asm_sub_imm(r, rn, imm,  0); break;
        case LIR_andi: asm_and_imm(r, rn, imm,  0); break;
        case LIR_ori:  asm_orr_imm(r, rn, imm,  0); break;
        case LIR_xori: asm_eor_imm(r, rn, imm,  0); break;
        default:       NanoAssert(0);                        break;
        }
    } else {
        
        
        
        int d = findMemFor(i);
        if (ARM_VFP && IsFpReg(r)) {
            if (isU8(d/4) || isU8(-d/4)) {
                FLDD(r, FP, d);
            } else {
                FLDD(r, IP, d%1024);
                asm_add_imm(IP, FP, d-(d%1024));
            }
        } else {
            NIns merged;
            LDR(r, FP, d);
            
            
            if (
                does_next_instruction_exist(_nIns, codeStart, codeEnd,
                                                   exitStart, exitEnd)
                && 
                   do_peep_2_1(&merged, _nIns[0], _nIns[1])) {
                _nIns[1] = merged;
                _nIns++;
                verbose_only( asm_output("merge next into LDMDB"); )
            }
        }
    }
}

void
Assembler::asm_spill(Register rr, int d, bool quad)
{
    (void) quad;
    NanoAssert(d);
    
    NanoAssert(rr != PC);
    NanoAssert(rr != IP);
    NanoAssert(rr != SP);
    if (ARM_VFP && IsFpReg(rr)) {
        if (isU8(d/4) || isU8(-d/4)) {
            FSTD(rr, FP, d);
        } else {
            FSTD(rr, IP, d%1024);
            asm_add_imm(IP, FP, d-(d%1024));
        }
    } else {
        NIns merged;
        
        
        if (asm_str(rr, FP, d)) {
            
            
            if (
                    does_next_instruction_exist(_nIns, codeStart, codeEnd,
                        exitStart, exitEnd)
                    && 
                    do_peep_2_1(&merged, _nIns[0], _nIns[1])) {
                _nIns[1] = merged;
                _nIns++;
                verbose_only( asm_output("merge next into STMDB"); )
            }
        }
    }
}

void
Assembler::asm_load64(LIns* ins)
{
    NanoAssert(ins->isD());

    if (ARM_VFP) {
        Register    dd;
        LIns*       base = ins->oprnd1();
        Register    rn = findRegFor(base, GpRegs);
        int         offset = ins->disp();

        if (ins->isInReg()) {
            dd = prepareResultReg(ins, FpRegs & ~rmask(D0));
        } else {
            
            
            NanoAssert(ins->isInAr());
            int d = arDisp(ins);
            evictIfActive(D0);
            dd = D0;
            
            
            if (isU8(d/4) || isU8(-d/4)) {
                FSTD(dd, FP, d);
            } else {
                FSTD(dd, IP, d%1024);
                asm_add_imm(IP, FP, d-(d%1024));
            }
        }

        switch (ins->opcode()) {
            case LIR_ldd:
                if (isU8(offset/4) || isU8(-offset/4)) {
                    FLDD(dd, rn, offset);
                } else {
                    FLDD(dd, IP, offset%1024);
                    asm_add_imm(IP, rn, offset-(offset%1024));
                }
                break;
            case LIR_ldf2d:
                evictIfActive(D0);
                FCVTDS(dd, S0);
                if (isU8(offset/4) || isU8(-offset/4)) {
                    FLDS(S0, rn, offset);
                } else {
                    FLDS(S0, IP, offset%1024);
                    asm_add_imm(IP, rn, offset-(offset%1024));
                }
                break;
            default:
                NanoAssertMsg(0, "LIR opcode unsupported by asm_load64.");
                break;
        }
    } else {
        NanoAssert(ins->isInAr());
        int         d = arDisp(ins);

        LIns*       base = ins->oprnd1();
        Register    rn = findRegFor(base, GpRegs);
        int         offset = ins->disp();

        switch (ins->opcode()) {
            case LIR_ldd:
                asm_mmq(FP, d, rn, offset);
                break;
            case LIR_ldf2d:
                NanoAssertMsg(0, "LIR_ldf2d is not yet implemented for soft-float.");
                break;
            default:
                NanoAssertMsg(0, "LIR opcode unsupported by asm_load64.");
                break;
        }
    }

    freeResourcesOf(ins);
}

void
Assembler::asm_store64(LOpcode op, LIns* value, int dr, LIns* base)
{
    NanoAssert(value->isD());

    if (ARM_VFP) {
        Register dd = findRegFor(value, FpRegs & ~rmask(D0));
        Register rn = findRegFor(base, GpRegs);

        switch (op) {
            case LIR_std:
                
                
                if (isU8(dr/4) || isU8(-dr/4)) {
                    FSTD(dd, rn, dr);
                } else {
                    FSTD(dd, IP, dr%1024);
                    asm_add_imm(IP, rn, dr-(dr%1024));
                }

                break;
            case LIR_std2f:
                
                
                evictIfActive(D0);
                if (isU8(dr/4) || isU8(-dr/4)) {
                    FSTS(S0, rn, dr);
                } else {
                    FSTS(S0, IP, dr%1024);
                    asm_add_imm(IP, rn, dr-(dr%1024));
                }

                FCVTSD(S0, dd);

                break;
            default:
                NanoAssertMsg(0, "LIR opcode unsupported by asm_store64.");
                break;
        }
    } else {
        int         d = findMemFor(value);
        Register    rn = findRegFor(base, GpRegs);

        switch (op) {
            case LIR_std:
                
                
                
                asm_mmq(rn, dr, FP, d);
                break;
            case LIR_std2f:
                NanoAssertMsg(0, "TODO: Soft-float implementation of LIR_std2f.");
                break;
            default:
                NanoAssertMsg(0, "LIR opcode unsupported by asm_store64.");
                break;
        }
    }
}


void
Assembler::asm_immd_nochk(Register dd, int32_t immDlo, int32_t immDhi)
{
    
    
    
    

    
    
    
    
    

    FLDD(dd, PC, -16);

    *(--_nIns) = (NIns) immDhi;
    *(--_nIns) = (NIns) immDlo;

    B_nochk(_nIns+2);
}

void
Assembler::asm_immd(LIns* ins)
{
    
    
    
    if (ARM_VFP && ins->isInReg()) {
        Register dd = prepareResultReg(ins, FpRegs);
        underrunProtect(4*4);
        asm_immd_nochk(dd, ins->immDlo(), ins->immDhi());
    } else {
        NanoAssert(ins->isInAr());
        int d = arDisp(ins);
        asm_str(IP, FP, d+4);
        asm_ld_imm(IP, ins->immDhi());
        asm_str(IP, FP, d);
        asm_ld_imm(IP, ins->immDlo());
    }

    freeResourcesOf(ins);
}

void
Assembler::asm_nongp_copy(Register r, Register s)
{
    if (ARM_VFP && IsFpReg(r) && IsFpReg(s)) {
        
        FCPYD(r, s);
    } else {
        
        
        NanoAssert(0);
    }
}




void
Assembler::asm_mmq(Register rd, int dd, Register rs, int ds)
{
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    NanoAssert(rs != PC);
    NanoAssert(rd != PC);

    
    
    NanoAssert(rs != IP);
    NanoAssert(rd != IP);

    
    
    
    RegisterMask    free = _allocator.free & AllowableFlagRegs;

    
    
    
    
    

    int32_t dd_adj = 0;
    int32_t ds_adj = 0;

    if ((dd+4) >= 0x1000) {
        dd_adj = ((dd+4) & ~0xfff);
    } else if (dd <= -0x1000) {
        dd_adj = -((-dd) & ~0xfff);
    }
    if ((ds+4) >= 0x1000) {
        ds_adj = ((ds+4) & ~0xfff);
    } else if (ds <= -0x1000) {
        ds_adj = -((-ds) & ~0xfff);
    }

    
    asm_sub_imm(rd, rd, dd_adj);
    asm_sub_imm(rs, rs, ds_adj);

    ds -= ds_adj;
    dd -= dd_adj;

    if (free) {
        
        
        

        
        
        Register    rr = (Register)(31-CountLeadingZeroes(free));

        
        
        NanoAssert((free & rmask(PC)) == 0);
        NanoAssert((free & rmask(LR)) == 0);
        NanoAssert((free & rmask(SP)) == 0);
        NanoAssert((free & rmask(IP)) == 0);
        NanoAssert((free & rmask(FP)) == 0);

        
        STR(IP, rd, dd+4);
        STR(rr, rd, dd);
        LDR(IP, rs, ds+4);
        LDR(rr, rs, ds);
    } else {
        
        STR(IP, rd, dd+4);
        LDR(IP, rs, ds+4);
        STR(IP, rd, dd);
        LDR(IP, rs, ds);
    }

    
    asm_add_imm(rd, rd, dd_adj);
    asm_add_imm(rs, rs, ds_adj);
}



verbose_only(
void Assembler::asm_inc_m32(uint32_t* pCtr)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    underrunProtect(8*4);
    IMM32(0xE8BD0003);       
    IMM32(0xE5801000);       
    IMM32(0xE2811001);       
    IMM32(0xE5901000);       
    IMM32((uint32_t)pCtr);   
    IMM32(0xEA000000);       
    IMM32(0xE59F0000);       
    IMM32(0xE92D0003);       
}
)

void
Assembler::nativePageReset()
{
    _nSlot = 0;
    _nExitSlot = 0;
}

void
Assembler::nativePageSetup()
{
    NanoAssert(!_inExit);
    if (!_nIns)
        codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes), NJ_MAX_CPOOL_OFFSET);

    
    
    if (!_nSlot)
        _nSlot = codeStart;
}


void
Assembler::underrunProtect(int bytes)
{
    NanoAssertMsg(bytes<=LARGEST_UNDERRUN_PROT, "constant LARGEST_UNDERRUN_PROT is too small");
    NanoAssert(_nSlot != 0 && int(_nIns)-int(_nSlot) <= 4096);
    uintptr_t top = uintptr_t(_nSlot);
    uintptr_t pc = uintptr_t(_nIns);
    if (pc - bytes < top)
    {
        verbose_only(verbose_outputf("        %p:", _nIns);)
        NIns* target = _nIns;
        
        codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes), NJ_MAX_CPOOL_OFFSET);

        _nSlot = codeStart;

        
        
        
        B_nochk(target);
    }
}

void
Assembler::JMP_far(NIns* addr)
{
    
    
    underrunProtect(8);

    intptr_t offs = PC_OFFSET_FROM(addr,_nIns-2);

    if (isS24(offs>>2)) {
        
        
        BKPT_nochk();

        asm_output("bkpt");

        
        *(--_nIns) = (NIns)( COND_AL | (0xA<<24) | ((offs>>2) & 0xFFFFFF) );

        asm_output("b %p", (void*)addr);
    } else {
        
        *(--_nIns) = (NIns)((addr));
        
        
        *(--_nIns) = (NIns)( COND_AL | (0x51<<20) | (PC<<16) | (PC<<12) | (4));

        asm_output("ldr pc, =%p", (void*)addr);
    }
}









void
Assembler::BranchWithLink(NIns* addr)
{
    
    
    
    
    underrunProtect(8+LD32_size);

    
    
    intptr_t offs = PC_OFFSET_FROM(addr,_nIns-1);

    
    
    if (isS24(offs>>2)) {
        
        
        intptr_t offs2 = (offs>>2) & 0xffffff;

        if (((intptr_t)addr & 1) == 0) {
            

            
            *(--_nIns) = (NIns)( (COND_AL) | (0xB<<24) | (offs2) );
            asm_output("bl %p", (void*)addr);
            return;
        } else if (ARM_ARCH_AT_LEAST(5)) {
            
            
            uint32_t    H = (offs & 0x2) << 23;

            
            *(--_nIns) = (NIns)( (0xF << 28) | (0x5<<25) | (H) | (offs2) );
            asm_output("blx %p", (void*)addr);
            return;
        }
        

    }
    if (ARM_ARCH_AT_LEAST(5)) {
        
        
        BLX(IP, false);
    } else {
        BX(IP);
        MOV(LR, PC);
    }
    
    asm_ld_imm(IP, (int32_t)addr, false);
}



inline void
Assembler::BLX(Register addr, bool chk )
{
    
    
    
    NanoAssert(ARM_ARCH_AT_LEAST(5));

    NanoAssert(IsGpReg(addr));
#ifdef UNDER_CE
    
    
    NanoAssert(addr != LR);
#endif

    if (chk) {
        underrunProtect(4);
    }

    
    *(--_nIns) = (NIns)( (COND_AL) | (0x12<<20) | (0xFFF<<8) | (0x3<<4) | (addr) );
    asm_output("blx %s", gpn(addr));
}







void
Assembler::asm_ldr_chk(Register d, Register b, int32_t off, bool chk)
{
    if (ARM_VFP && IsFpReg(d)) {
        FLDD_chk(d,b,off,chk);
        return;
    }

    NanoAssert(IsGpReg(d));
    NanoAssert(IsGpReg(b));

    
    
    
    NanoAssert((b != PC) || (!chk));

    if (isU12(off)) {
        
        if (chk) underrunProtect(4);
        *(--_nIns) = (NIns)( COND_AL | (0x59<<20) | (b<<16) | (d<<12) | off );
    } else if (isU12(-off)) {
        
        if (chk) underrunProtect(4);
        *(--_nIns) = (NIns)( COND_AL | (0x51<<20) | (b<<16) | (d<<12) | -off );
    } else {
        
        

        
        

        NanoAssert(b != PC);
        NanoAssert(b != IP);

        if (chk) underrunProtect(4+LD32_size);

        *(--_nIns) = (NIns)( COND_AL | (0x79<<20) | (b<<16) | (d<<12) | IP );
        asm_ld_imm(IP, off, false);
    }

    asm_output("ldr %s, [%s, #%d]",gpn(d),gpn(b),(off));
}





















int32_t
Assembler::asm_str(Register rt, Register rr, int32_t offset)
{
    
    
    
    
    NanoAssert(rr != PC);
    NanoAssert(rt != PC);
    if (offset >= 0) {
        
        if (isU12(offset)) {
            STR(rt, rr, offset);
            return 1;
        }

        if (rt != IP) {
            STR(rt, IP, offset & 0xfff);
            asm_add_imm(IP, rr, offset & ~0xfff);
        } else {
            int32_t adj = offset & ~0xfff;
            asm_sub_imm(rr, rr, adj);
            STR(rt, rr, offset-adj);
            asm_add_imm(rr, rr, adj);
        }
    } else {
        
        if (isU12(-offset)) {
            STR(rt, rr, offset);
            return 1;
        }

        if (rt != IP) {
            STR(rt, IP, -((-offset) & 0xfff));
            asm_sub_imm(IP, rr, (-offset) & ~0xfff);
        } else {
            int32_t adj = ((-offset) & ~0xfff);
            asm_add_imm(rr, rr, adj);
            STR(rt, rr, offset+adj);
            asm_sub_imm(rr, rr, adj);
        }
    }

    return 0;
}









void
Assembler::asm_ld_imm(Register d, int32_t imm, bool chk )
{
    uint32_t    op2imm;

    NanoAssert(IsGpReg(d));

    
    
    

    if (encOp2Imm(imm, &op2imm)) {
        
        MOVis(d, op2imm, 0);
        return;
    }

    if (encOp2Imm(~imm, &op2imm)) {
        
        MVNis(d, op2imm, 0);
        return;
    }

    
    
    
    
    
    
    
    
    if (ARM_ARCH_AT_LEAST(7) && (d != PC)) {
        
        uint32_t    high_h = (uint32_t)imm >> 16;
        uint32_t    low_h = imm & 0xffff;

        if (high_h != 0) {
            
            MOVTi_chk(d, high_h, chk);
        }
        
        
        
        MOVWi_chk(d, low_h, chk);

        return;
    }

    
    

    
    
    
    
    
    

    
    
    if (chk) {
        underrunProtect(LD32_size);
    }

    int offset = PC_OFFSET_FROM(_nSlot, _nIns-1);
    
    while (offset <= -4096) {
        ++_nSlot;
        offset += sizeof(_nSlot);
    }
    NanoAssert((isU12(-offset) || isU12(offset)) && (offset <= -8));

    
    *(_nSlot++) = imm;
    asm_output("## imm= 0x%x", imm);

    
    LDR_nochk(d,PC,offset);
    NanoAssert(uintptr_t(_nIns) + 8 + offset == uintptr_t(_nSlot-1));
    NanoAssert(*((int32_t*)_nSlot-1) == imm);
}


















void
Assembler::B_cond_chk(ConditionCode _c, NIns* _t, bool _chk)
{
    int32_t offs = PC_OFFSET_FROM(_t,_nIns-1);
    

    
    if (_chk && isS24(offs>>2) && (_t != 0)) {
        underrunProtect(4);
        
        
        offs = PC_OFFSET_FROM(_t,_nIns-1);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if (isS24(offs>>2) && (_t != 0)) {
        
        *(--_nIns) = (NIns)( ((_c)<<28) | (0xA<<24) | (((offs)>>2) & 0xFFFFFF) );
        asm_output("b%s %p", _c == AL ? "" : condNames[_c], (void*)(_t));
    } else if (_c == AL) {
        if(_chk) underrunProtect(8);
        *(--_nIns) = (NIns)(_t);
        *(--_nIns) = (NIns)( COND_AL | (0x51<<20) | (PC<<16) | (PC<<12) | 0x4 );
        asm_output("b%s %p", _c == AL ? "" : condNames[_c], (void*)(_t));
    } else if (PC_OFFSET_FROM(_nSlot, _nIns-1) > -0x1000) {
        if(_chk) underrunProtect(8);
        *(_nSlot++) = (NIns)(_t);
        offs = PC_OFFSET_FROM(_nSlot-1,_nIns-1);
        NanoAssert(offs < 0);
        *(--_nIns) = (NIns)( ((_c)<<28) | (0x51<<20) | (PC<<16) | (PC<<12) | ((-offs) & 0xFFF) );
        asm_output("ldr%s %s, [%s, #-%d]", condNames[_c], gpn(PC), gpn(PC), -offs);
        NanoAssert(uintptr_t(_nIns)+8+offs == uintptr_t(_nSlot-1));
    } else {
        if(_chk) underrunProtect(12);
        
        *(--_nIns) = (NIns)(_t);
        
        
        
        *(--_nIns) = (NIns)( COND_AL | (0xA<<24) | 0x0 );
        
        *(--_nIns) = (NIns)( ((_c)<<28) | (0x51<<20) | (PC<<16) | (PC<<12) | 0x0 );
        asm_output("b%s %p", _c == AL ? "" : condNames[_c], (void*)(_t));
    }
}





void
Assembler::asm_i2d(LIns* ins)
{
    Register dd = prepareResultReg(ins, FpRegs & ~rmask(D0));
    Register rt = findRegFor(ins->oprnd1(), GpRegs);

    evictIfActive(D0);
    FSITOD(dd, S0);
    FMSR(S0, rt);

    freeResourcesOf(ins);
}

void
Assembler::asm_ui2d(LIns* ins)
{
    Register dd = prepareResultReg(ins, FpRegs & ~rmask(D0));
    Register rt = findRegFor(ins->oprnd1(), GpRegs);

    evictIfActive(D0);
    FUITOD(dd, S0);
    FMSR(S0, rt);

    freeResourcesOf(ins);
}

void Assembler::asm_d2i(LIns* ins)
{
    evictIfActive(D0);
    if (ins->isInReg()) {
        Register rt = ins->getReg();
        FMRS(rt, S0);
    } else {
        
        
        int32_t d = arDisp(ins);
        
        
        if (isU8(d/4) || isU8(-d/4)) {
            FSTS(S0, FP, d);
        } else {
            FSTS(S0, IP, d%1024);
            asm_add_imm(IP, FP, d-(d%1024));
        }
    }

    Register dm = findRegFor(ins->oprnd1(), FpRegs & ~rmask(D0));

    FTOSID(S0, dm);

    freeResourcesOf(ins);
}

void
Assembler::asm_fneg(LIns* ins)
{
    LIns* lhs = ins->oprnd1();

    Register dd = prepareResultReg(ins, FpRegs);
    
    Register dm = lhs->isInReg() ? lhs->getReg() : dd;

    FNEGD(dd, dm);

    freeResourcesOf(ins);
    if (dd == dm) {
        NanoAssert(!lhs->isInReg());
        findSpecificRegForUnallocated(lhs, dd);
    }
}

void
Assembler::asm_fop(LIns* ins)
{
    LIns*   lhs = ins->oprnd1();
    LIns*   rhs = ins->oprnd2();

    Register    dd = prepareResultReg(ins, FpRegs);
    
    Register    dn = lhs->isInReg() ? lhs->getReg() : dd;
    Register    dm = rhs->isInReg() ? rhs->getReg() : dd;
    if ((dn == dm) && (lhs != rhs)) {
        
        
        dm = findRegFor(rhs, FpRegs & ~rmask(dd));
        NanoAssert(rhs->isInReg());
    }

    

    switch(ins->opcode()) {
        case LIR_addd:      FADDD(dd,dn,dm);        break;
        case LIR_subd:      FSUBD(dd,dn,dm);        break;
        case LIR_muld:      FMULD(dd,dn,dm);        break;
        case LIR_divd:      FDIVD(dd,dn,dm);        break;
        default:            NanoAssert(0);          break;
    }

    freeResourcesOf(ins);

    
    if (dn == dd) {
        NanoAssert(!lhs->isInReg());
        findSpecificRegForUnallocated(lhs, dd);
    } else if (dm == dd) {
        NanoAssert(!rhs->isInReg());
        findSpecificRegForUnallocated(rhs, dd);
    } else {
        NanoAssert(lhs->isInReg());
        NanoAssert(rhs->isInReg());
    }
}

void
Assembler::asm_cmpd(LIns* ins)
{
    LIns* lhs = ins->oprnd1();
    LIns* rhs = ins->oprnd2();
    LOpcode op = ins->opcode();

    NanoAssert(ARM_VFP);
    NanoAssert(isCmpDOpcode(op));
    NanoAssert(lhs->isD() && rhs->isD());

    Register ra, rb;
    findRegFor2(FpRegs, lhs, ra, FpRegs, rhs, rb);

    int e_bit = (op != LIR_eqd);

    
    
    
    FMSTAT();
    FCMPD(ra, rb, e_bit);
}




Branches
Assembler::asm_branch(bool branchOnFalse, LIns* cond, NIns* targ)
{
    LOpcode condop = cond->opcode();
    NanoAssert(cond->isCmp());
    NanoAssert(ARM_VFP || !isCmpDOpcode(condop));

    
    
    ConditionCode cc = AL;

    
    bool    fp_cond;

    
    switch (condop)
    {
        
        
        
        case LIR_eqd:   cc = EQ;    fp_cond = true;     break;
        case LIR_ltd:   cc = LO;    fp_cond = true;     break;
        case LIR_led:   cc = LS;    fp_cond = true;     break;
        case LIR_ged:   cc = GE;    fp_cond = true;     break;
        case LIR_gtd:   cc = GT;    fp_cond = true;     break;

        
        case LIR_eqi:   cc = EQ;    fp_cond = false;    break;
        case LIR_lti:   cc = LT;    fp_cond = false;    break;
        case LIR_lei:   cc = LE;    fp_cond = false;    break;
        case LIR_gti:   cc = GT;    fp_cond = false;    break;
        case LIR_gei:   cc = GE;    fp_cond = false;    break;
        case LIR_ltui:  cc = LO;    fp_cond = false;    break;
        case LIR_leui:  cc = LS;    fp_cond = false;    break;
        case LIR_gtui:  cc = HI;    fp_cond = false;    break;
        case LIR_geui:  cc = HS;    fp_cond = false;    break;

        
        default:        cc = AL;    fp_cond = false;    break;
    }

    
    if (branchOnFalse)
        cc = OppositeCond(cc);

    
    NanoAssert((cc != AL) && (cc != NV));

    
    NanoAssert(ARM_VFP || !fp_cond);

    
    B_cond(cc, targ);

    
    
    NIns *at = _nIns;

    asm_cmp(cond);

    return Branches(at);
}

NIns* Assembler::asm_branch_ov(LOpcode op, NIns* target)
{
    
    
    
    ConditionCode cc = ( (op == LIR_mulxovi) || (op == LIR_muljovi) ? NE : VS );

    
    B_cond(cc, target);
    return _nIns;
}

void
Assembler::asm_cmp(LIns *cond)
{
    LIns* lhs = cond->oprnd1();
    LIns* rhs = cond->oprnd2();

    
    
    
    if (lhs->isD()) {
        NanoAssert(rhs->isD());
        asm_cmpd(cond);
        return;
    }

    NanoAssert(lhs->isI() && rhs->isI());

    
    if (rhs->isImmI()) {
        int c = rhs->immI();
        Register r = findRegFor(lhs, GpRegs);
        asm_cmpi(r, c);
    } else {
        Register ra, rb;
        findRegFor2(GpRegs, lhs, ra, GpRegs, rhs, rb);
        CMP(ra, rb);
    }
}

void
Assembler::asm_cmpi(Register r, int32_t imm)
{
    if (imm < 0) {
        if (imm > -256) {
            ALUi(AL, cmn, 1, 0, r, -imm);
        } else {
            underrunProtect(4 + LD32_size);
            CMP(r, IP);
            asm_ld_imm(IP, imm);
        }
    } else {
        if (imm < 256) {
            ALUi(AL, cmp, 1, 0, r, imm);
        } else {
            underrunProtect(4 + LD32_size);
            CMP(r, IP);
            asm_ld_imm(IP, imm);
        }
    }
}

void
Assembler::asm_condd(LIns* ins)
{
    Register rd = prepareResultReg(ins, GpRegs);

    
    
    

    switch (ins->opcode()) {
        case LIR_eqd:   SETEQ(rd);      break;
        case LIR_ltd:   SETLO(rd);      break; 
        case LIR_led:   SETLS(rd);      break; 
        case LIR_ged:   SETGE(rd);      break;
        case LIR_gtd:   SETGT(rd);      break;
        default:        NanoAssert(0);  break;
    }

    freeResourcesOf(ins);

    asm_cmpd(ins);
}

void
Assembler::asm_cond(LIns* ins)
{
    Register rd = prepareResultReg(ins, GpRegs);
    LOpcode op = ins->opcode();

    switch(op)
    {
        case LIR_eqi:   SETEQ(rd);      break;
        case LIR_lti:   SETLT(rd);      break;
        case LIR_lei:   SETLE(rd);      break;
        case LIR_gti:   SETGT(rd);      break;
        case LIR_gei:   SETGE(rd);      break;
        case LIR_ltui:  SETLO(rd);      break;
        case LIR_leui:  SETLS(rd);      break;
        case LIR_gtui:  SETHI(rd);      break;
        case LIR_geui:  SETHS(rd);      break;
        default:        NanoAssert(0);  break;
    }

    freeResourcesOf(ins);

    asm_cmp(ins);
}

void
Assembler::asm_arith(LIns* ins)
{
    LOpcode     op = ins->opcode();
    LIns*       lhs = ins->oprnd1();
    LIns*       rhs = ins->oprnd2();

    
    
    
    Register    rd = prepareResultReg(ins, GpRegs);

    
    Register    rn = lhs->isInReg() ? lhs->getReg() : rd;

    
    
    
    
    
    
    if (rhs->isImmI() && (op != LIR_muli) && (op != LIR_mulxovi) && (op != LIR_muljovi))
    {
        int32_t immI = rhs->immI();

        switch (op)
        {
            case LIR_addi:       asm_add_imm(rd, rn, immI);     break;
            case LIR_addjovi:
            case LIR_addxovi:    asm_add_imm(rd, rn, immI, 1);  break;
            case LIR_subi:       asm_sub_imm(rd, rn, immI);     break;
            case LIR_subjovi:
            case LIR_subxovi:    asm_sub_imm(rd, rn, immI, 1);  break;
            case LIR_andi:       asm_and_imm(rd, rn, immI);     break;
            case LIR_ori:        asm_orr_imm(rd, rn, immI);     break;
            case LIR_xori:       asm_eor_imm(rd, rn, immI);     break;
            case LIR_lshi:       LSLi(rd, rn, immI);            break;
            case LIR_rshi:       ASRi(rd, rn, immI);            break;
            case LIR_rshui:      LSRi(rd, rn, immI);            break;

            default:
                NanoAssertMsg(0, "Unsupported");
                break;
        }

        freeResourcesOf(ins);
        if (rd == rn) {
            
            NanoAssert(!lhs->isInReg());
            findSpecificRegForUnallocated(lhs, rd);
        }
        return;
    }

    
    

    Register    rm = rhs->isInReg() ? rhs->getReg() : rd;
    if ((rm == rn) && (lhs != rhs)) {
        
        
        
        rn = findRegFor(lhs, GpRegs & ~rmask(rd));
        NanoAssert(lhs->isInReg());
    }

    switch (op)
    {
        case LIR_addi:       ADDs(rd, rn, rm, 0);    break;
        case LIR_addjovi:
        case LIR_addxovi:    ADDs(rd, rn, rm, 1);    break;
        case LIR_subi:       SUBs(rd, rn, rm, 0);    break;
        case LIR_subjovi:
        case LIR_subxovi:    SUBs(rd, rn, rm, 1);    break;
        case LIR_andi:       ANDs(rd, rn, rm, 0);    break;
        case LIR_ori:        ORRs(rd, rn, rm, 0);    break;
        case LIR_xori:       EORs(rd, rn, rm, 0);    break;

        case LIR_muli:
            if (!ARM_ARCH_AT_LEAST(6) && (rd == rn)) {
                
                
                NanoAssert(!lhs->isInReg());
                rn = findRegFor(lhs, GpRegs & ~rmask(rd) & ~rmask(rm));
                if (lhs == rhs) {
                    rm = rn;
                }
            }
            MUL(rd, rn, rm);
            break;
        case LIR_muljovi:
        case LIR_mulxovi:
            if (!ARM_ARCH_AT_LEAST(6) && (rd == rn)) {
                
                
                NanoAssert(!lhs->isInReg());
                rn = findRegFor(lhs, GpRegs & ~rmask(rd) & ~rmask(rm));
                if (lhs == rhs) {
                    rm = rn;
                }
            }
            
            
            
            
            
            
            ALUr_shi(AL, cmp, 1, SBZ, IP, rd, ASR_imm, 31);
            SMULL(rd, IP, rn, rm);
            break;

        
        
        
        case LIR_lshi:
            LSL(rd, rn, IP);
            ANDi(IP, rm, 0x1f);
            break;
        case LIR_rshi:
            ASR(rd, rn, IP);
            ANDi(IP, rm, 0x1f);
            break;
        case LIR_rshui:
            LSR(rd, rn, IP);
            ANDi(IP, rm, 0x1f);
            break;
        default:
            NanoAssertMsg(0, "Unsupported");
            break;
    }

    freeResourcesOf(ins);
    
    if (rn == rd) {
        NanoAssert(!lhs->isInReg());
        findSpecificRegForUnallocated(lhs, rd);
    } else if (rm == rd) {
        NanoAssert(!rhs->isInReg());
        findSpecificRegForUnallocated(rhs, rd);
    } else {
        NanoAssert(lhs->isInReg());
        NanoAssert(rhs->isInReg());
    }
}

void
Assembler::asm_neg_not(LIns* ins)
{
    LIns* lhs = ins->oprnd1();
    Register rr = prepareResultReg(ins, GpRegs);

    
    Register ra = lhs->isInReg() ? lhs->getReg() : rr;

    if (ins->isop(LIR_noti)) {
        MVN(rr, ra);
    } else {
        NanoAssert(ins->isop(LIR_negi));
        RSBS(rr, ra);
    }

    freeResourcesOf(ins);
    if (!lhs->isInReg()) {
        NanoAssert(ra == rr);
        
        findSpecificRegForUnallocated(lhs, ra);
    }
}

void
Assembler::asm_load32(LIns* ins)
{
    LOpcode op = ins->opcode();
    LIns*   base = ins->oprnd1();
    int     d = ins->disp();

    Register rt = prepareResultReg(ins, GpRegs);
    
    Register rn = base->isInReg() ? base->getReg() : rt;

    
    
    

    switch (op) {
        case LIR_lduc2ui:
            if (isU12(-d) || isU12(d)) {
                LDRB(rt, rn, d);
            } else {
                LDRB(rt, IP, d%4096);
                asm_add_imm(IP, rn, d-(d%4096));
            }
            break;
        case LIR_ldus2ui:
            
            
            if (isU8(-d) || isU8(d)) {
                LDRH(rt, rn, d);
            } else {
                LDRH(rt, IP, d%256);
                asm_add_imm(IP, rn, d-(d%256));
            }
            break;
        case LIR_ldi:
            
            if (isU12(-d) || isU12(d)) {
                LDR(rt, rn, d);
            } else {
                LDR(rt, IP, d%4096);
                asm_add_imm(IP, rn, d-(d%4096));
            }
            break;
        case LIR_ldc2i:
            
            
            if (isU8(-d) || isU8(d)) {
                LDRSB(rt, rn, d);
            } else {
                LDRSB(rn, IP, d%256);
                asm_add_imm(IP, rn, d-(d%256));
            }
            break;
        case LIR_lds2i:
            
            if (isU8(-d) || isU8(d)) {
                LDRSH(rt, rn, d);
            } else {
                LDRSH(rt, IP, d%256);
                asm_add_imm(IP, rn, d-(d%256));
            }
            break;
        default:
            NanoAssertMsg(0, "asm_load32 should never receive this LIR opcode");
            break;
    }

    freeResourcesOf(ins);

    if (rn == rt) {
        NanoAssert(!base->isInReg());
        findSpecificRegForUnallocated(base, rn);
    }
}

void
Assembler::asm_cmov(LIns* ins)
{
    LIns*           condval = ins->oprnd1();
    LIns*           iftrue  = ins->oprnd2();
    LIns*           iffalse = ins->oprnd3();
    RegisterMask    allow = ins->isD() ? FpRegs : GpRegs;
    ConditionCode   cc;

    NanoAssert(condval->isCmp());
    NanoAssert((ins->isop(LIR_cmovi) && iftrue->isI() && iffalse->isI()) ||
               (ins->isop(LIR_cmovd) && iftrue->isD() && iffalse->isD()));

    Register rd = prepareResultReg(ins, allow);

    
    Register rt = iftrue->isInReg() ? iftrue->getReg() : rd;
    Register rf = iffalse->isInReg() ? iffalse->getReg() : rd;
    
    
    if ((rt == rf) && (iftrue != iffalse)) {
        
        
        rf = findRegFor(iffalse, allow & ~rmask(rd));
        NanoAssert(iffalse->isInReg());
    }

    switch(condval->opcode()) {
        default:        NanoAssert(0);
        
        case LIR_eqi:   cc = EQ;        break;
        case LIR_lti:   cc = LT;        break;
        case LIR_lei:   cc = LE;        break;
        case LIR_gti:   cc = GT;        break;
        case LIR_gei:   cc = GE;        break;
        case LIR_ltui:  cc = LO;        break;
        case LIR_leui:  cc = LS;        break;
        case LIR_gtui:  cc = HI;        break;
        case LIR_geui:  cc = HS;        break;
        
        case LIR_eqd:   cc = EQ;        break;
        case LIR_ltd:   cc = LO;        break;
        case LIR_led:   cc = LS;        break;
        case LIR_ged:   cc = GE;        break;
        case LIR_gtd:   cc = GT;        break;
    }

    
    
    
    
    
    
    if (ins->isI()) {
        if (rd != rf) {
            MOV_cond(OppositeCond(cc), rd, rf);
        }
        if (rd != rt) {
            MOV_cond(cc, rd, rt);
        }
    } else if (ins->isD()) {
        
        
        NanoAssert(ARM_VFP);
        if (rd != rf) {
            FCPYD_cond(OppositeCond(cc), rd, rf);
        }
        if (rd != rt) {
            FCPYD_cond(cc, rd, rt);
        }
    } else {
        NanoAssert(0);
    }

    freeResourcesOf(ins);

    
    
    if (rt == rd) {
        NanoAssert(!iftrue->isInReg());
        findSpecificRegForUnallocated(iftrue, rd);
    } else if (rf == rd) {
        NanoAssert(!iffalse->isInReg());
        findSpecificRegForUnallocated(iffalse, rd);
    } else {
        NanoAssert(iffalse->isInReg());
        NanoAssert(iftrue->isInReg());
    }

    asm_cmp(condval);
}

void
Assembler::asm_qhi(LIns* ins)
{
    Register rd = prepareResultReg(ins, GpRegs);
    LIns *lhs = ins->oprnd1();
    int d = findMemFor(lhs);

    LDR(rd, FP, d+4);

    freeResourcesOf(ins);
}

void
Assembler::asm_qlo(LIns* ins)
{
    Register rd = prepareResultReg(ins, GpRegs);
    LIns *lhs = ins->oprnd1();
    int d = findMemFor(lhs);

    LDR(rd, FP, d);

    freeResourcesOf(ins);
}

void
Assembler::asm_param(LIns* ins)
{
    uint32_t a = ins->paramArg();
    uint32_t kind = ins->paramKind();
    if (kind == 0) {
        
        
        if (a < 4) {
            
            prepareResultReg(ins, rmask(argRegs[a]));
        } else {
            
            Register r = prepareResultReg(ins, GpRegs);
            int d = (a - 4) * sizeof(intptr_t) + 8;
            LDR(r, FP, d);
        }
    } else {
        
        NanoAssert(a < (sizeof(savedRegs)/sizeof(savedRegs[0])));
        prepareResultReg(ins, rmask(savedRegs[a]));
    }
    freeResourcesOf(ins);
}

void
Assembler::asm_immi(LIns* ins)
{
    Register rd = prepareResultReg(ins, GpRegs);
    asm_ld_imm(rd, ins->immI());
    freeResourcesOf(ins);
}

void
Assembler::asm_ret(LIns *ins)
{
    genEpilogue();

    
    
    
    
    
    
    if (!(ARM_EABI_HARD && ins->isop(LIR_retd))) {
        MOV(IP, R0);
    }

    
    MOV(SP,FP);

    releaseRegisters();
    assignSavedRegs();
    LIns *value = ins->oprnd1();
    if (ins->isop(LIR_reti)) {
        findSpecificRegFor(value, R0);
    }
    else {
        NanoAssert(ins->isop(LIR_retd));
        if (ARM_VFP) {
#ifdef NJ_ARM_EABI_HARD_FLOAT
            findSpecificRegFor(value, D0);
#else
            Register reg = findRegFor(value, FpRegs);
            FMRRD(R0, R1, reg);
#endif
        } else {
            NanoAssert(value->isop(LIR_ii2d));
            findSpecificRegFor(value->oprnd1(), R0); 
            findSpecificRegFor(value->oprnd2(), R1); 
        }
    }
}

void
Assembler::asm_jtbl(LIns* ins, NIns** table)
{
    Register indexreg = findRegFor(ins->oprnd1(), GpRegs);
    Register tmp = registerAllocTmp(GpRegs & ~rmask(indexreg));
    LDR_scaled(PC, tmp, indexreg, 2);      
    asm_ld_imm(tmp, (int32_t)table);       
}

void Assembler::swapCodeChunks() {
    if (!_nExitIns)
        codeAlloc(exitStart, exitEnd, _nExitIns verbose_only(, exitBytes), NJ_MAX_CPOOL_OFFSET);
    if (!_nExitSlot)
        _nExitSlot = exitStart;
    SWAP(NIns*, _nIns, _nExitIns);
    SWAP(NIns*, _nSlot, _nExitSlot);        
    SWAP(NIns*, codeStart, exitStart);
    SWAP(NIns*, codeEnd, exitEnd);
    verbose_only( SWAP(size_t, codeBytes, exitBytes); )
}

void Assembler::asm_insert_random_nop() {
    NanoAssert(0); 
}

}
#endif 
