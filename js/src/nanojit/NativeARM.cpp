







































#include "nanojit.h"

#ifdef UNDER_CE
#include <cmnintrin.h>
extern "C" bool blx_lr_broken();
#endif

#if defined(FEATURE_NANOJIT) && defined(NANOJIT_ARM)

namespace nanojit
{

#ifdef NJ_VERBOSE
const char* regNames[] = {"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","fp","ip","sp","lr","pc",
                          "d0","d1","d2","d3","d4","d5","d6","d7","s14"};
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


inline uint32_t
Assembler::CountLeadingZeroes(uint32_t data)
{
    uint32_t    leading_zeroes;

    
    
    
    
    
    
    
    NanoAssert(ARM_ARCH >= 5);

#if defined(__ARMCC__)
    
    leading_zeroes = __clz(data);





#elif defined(__GNUC__) && !(defined(ANDROID) && __ARM_ARCH__ <= 5) 
    
    __asm (
        "   clz     %0, %1  \n"
        :   "=r"    (leading_zeroes)
        :   "r"     (data)
    );
#elif defined(WINCE)
    
    leading_zeroes = _CountLeadingZeros(data);
#else
    
    
    uint32_t    try_shift;

    leading_zeroes = 0;

    
    
    for (try_shift = 16; try_shift != 0; try_shift /= 2) {
        uint32_t    shift = leading_zeroes + try_shift;
        if (((data << shift) >> shift) == data) {
            leading_zeroes = shift;
        }
    }
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
Assembler::nInit(AvmCore*)
{
#ifdef UNDER_CE
    blx_lr_bug = blx_lr_broken();
#else
    blx_lr_bug = 0;
#endif
}

void Assembler::nBeginAssembly()
{
    max_out_args = 0;
}

NIns*
Assembler::genPrologue()
{
    



    
    
    uint32_t stackNeeded = max_out_args + STACK_GRANULARITY * _activation.tos;
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
Assembler::nFragExit(LInsp guard)
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
    if (config.show_stats) {
        
        
        int fromfrag = int((Fragment*)_thisfrag);
        asm_ld_imm(argRegs[1], fromfrag);
    }
#endif

    
    MOV(SP, FP);
}

NIns*
Assembler::genEpilogue()
{
    
    
    NanoAssert(ARM_ARCH >= 5);

    RegisterMask savingMask = rmask(FP) | rmask(PC);

    POP_mask(savingMask); 

    
    
    
    MOV(R0, IP);

    return _nIns;
}
























void 
Assembler::asm_arg(ArgSize sz, LInsp arg, Register& r, int& stkd)
{
    
    NanoAssert((stkd & 3) == 0);

    if (sz == ARGSIZE_F) {
        
        asm_arg_64(arg, r, stkd);
    } else if (sz & ARGSIZE_MASK_INT) {
        
        if (r < R4) {
            asm_regarg(sz, arg, r);
            r = nextreg(r);
        } else {
            asm_stkarg(arg, stkd);
            stkd += 4;
        }
    } else {
        NanoAssert(sz == ARGSIZE_Q);
        
        NanoAssert(false);
    }
}




void
Assembler::asm_arg_64(LInsp arg, Register& r, int& stkd)
{
    
    NanoAssert((stkd & 3) == 0);
    
    
    NanoAssert(ARM_VFP || arg->isop(LIR_qjoin));

    Register    fp_reg = UnknownReg;

    if (ARM_VFP) {
        fp_reg = findRegFor(arg, FpRegs);
        NanoAssert(isKnownReg(fp_reg));
    }

#ifdef NJ_ARM_EABI
    
    
    
    
    
    if ((r == R1) || (r == R3)) {
        r = nextreg(r);
    }
#endif

    if (r < R3) {
        Register    ra = r;
        Register    rb = nextreg(r);
        r = nextreg(rb);

#ifdef NJ_ARM_EABI
        
        
        NanoAssert( ((ra == R0) && (rb == R1)) || ((ra == R2) && (rb == R3)) );
#endif

        
        
        
        if (ARM_VFP) {
            FMRRD(ra, rb, fp_reg);
        } else {
            asm_regarg(ARGSIZE_LO, arg->oprnd1(), ra);
            asm_regarg(ARGSIZE_LO, arg->oprnd2(), rb);
        }

#ifndef NJ_ARM_EABI
    } else if (r == R3) {
        
        
        
        Register    ra = r;
        r = nextreg(r);

        
        
        NanoAssert(r == R4);

        
        
        NanoAssert(stkd == 0);

        if (ARM_VFP) {
            
            
            

            
            
            STR(IP, SP, 0);
            stkd += 4;
            FMRRD(ra, IP, fp_reg);
        } else {
            
            
            
            asm_regarg(ARGSIZE_LO, arg->oprnd1(), ra);
            asm_stkarg(arg->oprnd2(), 0);
            stkd += 4;
        }
#endif
    } else {
        
#ifdef NJ_ARM_EABI
        
        if ((stkd & 7) != 0) {
            
            
            stkd += 4;
        }
#endif
        asm_stkarg(arg, stkd);
        stkd += 8;
    }
}

void 
Assembler::asm_regarg(ArgSize sz, LInsp p, Register r)
{
    NanoAssert(isKnownReg(r));
    if (sz & ARGSIZE_MASK_INT)
    {
        
        if (p->isconst()) {
            asm_ld_imm(r, p->imm32());
        } else {
            if (p->isUsed()) {
                if (!p->hasKnownReg()) {
                    
                    int d = findMemFor(p);
                    if (p->isop(LIR_alloc)) {
                        asm_add_imm(r, FP, d, 0);
                    } else {
                        LDR(r, FP, d);
                    }
                } else {
                    
                    MOV(r, p->getReg());
                }
            }
            else {
                
                
                findSpecificRegFor(p, r);
            }
        }
    }
    else if (sz == ARGSIZE_Q) {
        
        NanoAssert(false);
    }
    else
    {
        NanoAssert(sz == ARGSIZE_F);
        
        
        NanoAssert(false);
    }
}

void
Assembler::asm_stkarg(LInsp arg, int stkd)
{
    bool isQuad = arg->isQuad();

    Register rr;
    if (arg->isUsed() && (rr = arg->getReg(), isKnownReg(rr))) {
        
        
        if (!ARM_VFP || !isQuad) {
            NanoAssert(IsGpReg(rr));

            STR(rr, SP, stkd);
        } else {
            
            
            
            
            
            
            NanoAssert(ARM_VFP);
            NanoAssert(IsFpReg(rr));

#ifdef NJ_ARM_EABI
            
            NanoAssert((stkd & 7) == 0);
#endif

            FSTD(rr, SP, stkd);
        }
    } else {
        
        
        int d = findMemFor(arg);
        if (!isQuad) {
            STR(IP, SP, stkd);
            if (arg->isop(LIR_alloc)) {
                asm_add_imm(IP, FP, d);
            } else {
                LDR(IP, FP, d);
            }
        } else {
#ifdef NJ_ARM_EABI
            
            NanoAssert((stkd & 7) == 0);
#endif

            STR(IP, SP, stkd+4);
            LDR(IP, FP, d+4);
            STR(IP, SP, stkd);
            LDR(IP, FP, d);
        }
    }
}

void
Assembler::asm_call(LInsp ins)
{
    const CallInfo* call = ins->callInfo();
    ArgSize sizes[MAXARGS];
    uint32_t argc = call->get_sizes(sizes);
    bool indirect = call->isIndirect();

    
    
    NanoAssert(ARM_VFP || ins->isop(LIR_icall));

    
    
    
    
    if (ARM_VFP && ins->isUsed()) {
        
        ArgSize rsize = (ArgSize)(call->_argtypes & ARGSIZE_MASK_ANY);

        
        
        if (rsize == ARGSIZE_F) {
            Register rr = ins->getReg();

            NanoAssert(ins->opcode() == LIR_fcall);

            if (!isKnownReg(rr)) {
                int d = disp(ins);
                NanoAssert(d != 0);
                freeRsrcOf(ins, false);

                
                
                STR(R0, FP, d+0);
                STR(R1, FP, d+4);
            } else {
                NanoAssert(IsFpReg(rr));

                
                prepResultReg(ins, rmask(rr));
                FMDRR(rr, R0, R1);
            }
        }
    }

    
    if (!indirect) {
        verbose_only(if (_logc->lcbits & LC_Assembly)
            outputf("        %p:", _nIns);
        )

        
        
        
        
        
        BranchWithLink((NIns*)call->_address);
    } else {
        
        
        
        
        
        if (blx_lr_bug) {
            
            underrunProtect(8);
            BLX(IP);
            MOV(IP,LR);
        } else {
            BLX(LR);
        }
        asm_regarg(ARGSIZE_LO, ins->arg(--argc), LR);
    }

    
    Register    r = R0;
    int         stkd = 0;

    
    
    
    
    uint32_t    i = argc;
    while(i--) {
        asm_arg(sizes[i], ins->arg(i), r, stkd);
    }

    if (stkd > max_out_args) {
        max_out_args = stkd;
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
    if (ARM_VFP)
        a.free |= FpRegs;

    debug_only(a.managed = a.free);
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
Assembler::hint(LIns* i, RegisterMask allow )
{
    uint32_t op = i->opcode();
    int prefer = ~0;
    if (op==LIR_icall)
        prefer = rmask(R0);
    else if (op == LIR_callh)
        prefer = rmask(R1);
    else if (op == LIR_param) {
        if (i->paramArg() < 4)
            prefer = rmask(argRegs[i->paramArg()]);
    }
    if (_allocator.free & allow & prefer)
        allow &= prefer;
    return allow;
}

void
Assembler::asm_qjoin(LIns *ins)
{
    int d = findMemFor(ins);
    NanoAssert(d);
    LIns* lo = ins->oprnd1();
    LIns* hi = ins->oprnd2();

    Register r = findRegFor(hi, GpRegs);
    STR(r, FP, d+4);

    
    r = findRegFor(lo, GpRegs);
    STR(r, FP, d);
    freeRsrcOf(ins, false); 
}

void
Assembler::asm_store32(LIns *value, int dr, LIns *base)
{
    Register ra, rb;
    if (base->isop(LIR_alloc)) {
        rb = FP;
        dr += findMemFor(base);
        ra = findRegFor(value, GpRegs);
    } else {
        findRegFor2b(GpRegs, value, ra, base, rb);
    }

    if (!isS12(dr)) {
        STR(ra, IP, 0);
        asm_add_imm(IP, rb, dr);
    } else {
        STR(ra, rb, dr);
    }
}

void
Assembler::asm_restore(LInsp i, Reservation *, Register r)
{
    if (i->isop(LIR_alloc)) {
        asm_add_imm(r, FP, disp(i));
    } else if (i->isconst()) {
        if (!i->getArIndex()) {
            i->markAsClear();
        }
        asm_ld_imm(r, i->imm32());
    }
    else {
        
        
        
        int d = findMemFor(i);
        if (ARM_VFP && IsFpReg(r)) {
            if (isS8(d >> 2)) {
                FLDD(r, FP, d);
            } else {
                FLDD(r, IP, 0);
                asm_add_imm(IP, FP, d);
            }
        } else {
            LDR(r, FP, d);
        }
    }
    verbose_only(
        asm_output("        restore %s",_thisfrag->lirbuf->names->formatRef(i));
        )
}

void
Assembler::asm_spill(Register rr, int d, bool pop, bool quad)
{
    (void) pop;
    (void) quad;
    if (d) {
        if (ARM_VFP && IsFpReg(rr)) {
            if (isS8(d >> 2)) {
                FSTD(rr, FP, d);
            } else {
                FSTD(rr, IP, 0);
                asm_add_imm(IP, FP, d);
            }
        } else {
            STR(rr, FP, d);
        }
    }
}

void
Assembler::asm_load64(LInsp ins)
{
    

    NanoAssert(ins->isQuad());

    LIns* base = ins->oprnd1();
    int offset = ins->disp();

    Register rr = ins->getReg();
    int d = disp(ins);

    Register rb = findRegFor(base, GpRegs);
    NanoAssert(IsGpReg(rb));
    freeRsrcOf(ins, false);

    

    if (ARM_VFP && isKnownReg(rr)) {
        
        NanoAssert(IsFpReg(rr));

        if (!isS8(offset >> 2) || (offset&3) != 0) {
            FLDD(rr,IP,0);
            asm_add_imm(IP, rb, offset);
        } else {
            FLDD(rr,rb,offset);
        }
    } else {
        
        
        
        NanoAssert(!isKnownReg(rr));
        NanoAssert(d != 0);

        
        NanoAssert((d & 0x7) == 0);

        
        asm_mmq(FP, d, rb, offset);
    }

    
}

void
Assembler::asm_store64(LInsp value, int dr, LInsp base)
{
    

    if (ARM_VFP) {
        Register rb = findRegFor(base, GpRegs);

        if (value->isconstq()) {
            underrunProtect(LD32_size*2 + 8);

            
            STR(IP, rb, dr);
            asm_ld_imm(IP, value->imm64_0(), false);
            STR(IP, rb, dr+4);
            asm_ld_imm(IP, value->imm64_1(), false);

            return;
        }

        Register rv = findRegFor(value, FpRegs);

        NanoAssert(isKnownReg(rb));
        NanoAssert(isKnownReg(rv));

        Register baseReg = rb;
        intptr_t baseOffset = dr;

        if (!isS8(dr)) {
            baseReg = IP;
            baseOffset = 0;
        }

        FSTD(rv, baseReg, baseOffset);

        if (!isS8(dr)) {
            asm_add_imm(IP, rb, dr);
        }

        
        
        if (value->isconstq()) {
            underrunProtect(4*4);
            asm_quad_nochk(rv, value->imm64_0(), value->imm64_1());
        }
    } else {
        int da = findMemFor(value);
        Register rb = findRegFor(base, GpRegs);
        
        asm_mmq(rb, dr, FP, da);
    }

    
}



void
Assembler::asm_quad_nochk(Register rr, int32_t imm64_0, int32_t imm64_1)
{
    
    
    
    

    
    
    
    
    

    FLDD(rr, PC, -16);

    *(--_nIns) = (NIns) imm64_1;
    *(--_nIns) = (NIns) imm64_0;

    B_nochk(_nIns+2);
}

void
Assembler::asm_quad(LInsp ins)
{
    

    int d = disp(ins);
    Register rr = ins->getReg();

    freeRsrcOf(ins, false);

    if (ARM_VFP && isKnownReg(rr))
    {
        asm_spill(rr, d, false, true);

        underrunProtect(4*4);
        asm_quad_nochk(rr, ins->imm64_0(), ins->imm64_1());
    } else {
        NanoAssert(d);
        
        
        

        STR(IP, FP, d+4);
        asm_ld_imm(IP, ins->imm64_1());
        STR(IP, FP, d);
        asm_ld_imm(IP, ins->imm64_0());
    }

    
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

Register
Assembler::asm_binop_rhs_reg(LInsp)
{
    return UnknownReg;
}




void
Assembler::asm_mmq(Register rd, int dd, Register rs, int ds)
{
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    NanoAssert(rs != PC);
    NanoAssert(rd != PC);

    
    
    
    RegisterMask    free = _allocator.free & AllowableFlagRegs;

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
}



verbose_only(
void Assembler::asm_inc_m32(uint32_t* )
{
    
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
    if (!_nIns)
        codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
    if (!_nExitIns)
        codeAlloc(exitStart, exitEnd, _nExitIns verbose_only(, exitBytes));

    
    
    if (!_nSlot)
        _nSlot = codeStart;
    if (!_nExitSlot)
        _nExitSlot = exitStart;
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
        if (_inExit)
            codeAlloc(exitStart, exitEnd, _nIns verbose_only(, exitBytes));
        else
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));

        _nSlot = _inExit ? exitStart : codeStart;

        
        
        
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
    
    
    
    
    underrunProtect(4+LD32_size);

    
    
    intptr_t offs = PC_OFFSET_FROM(addr,_nIns-1);

    
    
    if (isS24(offs>>2)) {
        
        
        intptr_t offs2 = (offs>>2) & 0xffffff;

        if (((intptr_t)addr & 1) == 0) {
            

            
            *(--_nIns) = (NIns)( (COND_AL) | (0xB<<24) | (offs2) );
            asm_output("bl %p", (void*)addr);
        } else {
            

            
            
            
            NanoAssert(ARM_ARCH >= 5);

            
            uint32_t    H = (offs & 0x2) << 23;

            
            *(--_nIns) = (NIns)( (0xF << 28) | (0x5<<25) | (H) | (offs2) );
            asm_output("blx %p", (void*)addr);
        }
    } else {
        
        
        BLX(IP, false);

        
        asm_ld_imm(IP, (int32_t)addr, false);
    }
}



inline void
Assembler::BLX(Register addr, bool chk )
{
    
    
    
    NanoAssert(ARM_ARCH >= 5);

    NanoAssert(IsGpReg(addr));
    
    
    if (blx_lr_bug) { NanoAssert(addr != LR); }

    if (chk) {
        underrunProtect(4);
    }

    
    *(--_nIns) = (NIns)( (COND_AL) | (0x12<<20) | (0xFFF<<8) | (0x3<<4) | (addr) );
    asm_output("blx ip");
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

    
    
    
    
    if (ARM_THUMB2 && (d != PC)) {
        
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
    NanoAssert(isS12(offset) && (offset <= -8));

    
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
Assembler::asm_i2f(LInsp ins)
{
    Register rr = prepResultReg(ins, FpRegs);
    Register srcr = findRegFor(ins->oprnd1(), GpRegs);

    
    NanoAssert(isKnownReg(srcr));

    FSITOD(rr, FpSingleScratch);
    FMSR(FpSingleScratch, srcr);
}

void
Assembler::asm_u2f(LInsp ins)
{
    Register rr = prepResultReg(ins, FpRegs);
    Register sr = findRegFor(ins->oprnd1(), GpRegs);

    
    NanoAssert(isKnownReg(sr));

    FUITOD(rr, FpSingleScratch);
    FMSR(FpSingleScratch, sr);
}

void
Assembler::asm_fneg(LInsp ins)
{
    LInsp lhs = ins->oprnd1();
    Register rr = prepResultReg(ins, FpRegs);

    Register sr = ( lhs->isUnusedOrHasUnknownReg()
                  ? findRegFor(lhs, FpRegs)
                  : lhs->getReg() );

    FNEGD(rr, sr);
}

void
Assembler::asm_fop(LInsp ins)
{
    LInsp lhs = ins->oprnd1();
    LInsp rhs = ins->oprnd2();
    LOpcode op = ins->opcode();

    NanoAssert(op >= LIR_fadd && op <= LIR_fdiv);

    

    Register rr = prepResultReg(ins, FpRegs);

    Register ra = findRegFor(lhs, FpRegs);
    Register rb = (rhs == lhs) ? ra : findRegFor(rhs, FpRegs & ~rmask(ra));

    

    switch (op)
    {
        case LIR_fadd:      FADDD(rr,ra,rb);    break;
        case LIR_fsub:      FSUBD(rr,ra,rb);    break;
        case LIR_fmul:      FMULD(rr,ra,rb);    break;
        case LIR_fdiv:      FDIVD(rr,ra,rb);    break;
        default:            NanoAssert(0);      break;
    }
}

void
Assembler::asm_fcmp(LInsp ins)
{
    LInsp lhs = ins->oprnd1();
    LInsp rhs = ins->oprnd2();
    LOpcode op = ins->opcode();

    NanoAssert(op >= LIR_feq && op <= LIR_fge);

    Register ra, rb;
    findRegFor2b(FpRegs, lhs, ra, rhs, rb);

    int e_bit = (op != LIR_feq);

    
    FMSTAT();
    FCMPD(ra, rb, e_bit);
}

Register
Assembler::asm_prep_fcall(Reservation*, LInsp)
{
    




















    return UnknownReg;
}




NIns*
Assembler::asm_branch(bool branchOnFalse, LInsp cond, NIns* targ)
{
    LOpcode condop = cond->opcode();
    NanoAssert(cond->isCond());
    NanoAssert(ARM_VFP || ((condop < LIR_feq) || (condop > LIR_fge)));

    
    
    ConditionCode cc = AL;

    
    bool    fp_cond;

    
    
    
    if ((condop == LIR_ov) && (cond->oprnd1()->isop(LIR_mul))) {
        condop = LIR_eq;
        branchOnFalse = !branchOnFalse;
    }

    
    switch (condop)
    {
        
        
        
        case LIR_feq:   cc = EQ;    fp_cond = true;     break;
        case LIR_flt:   cc = LO;    fp_cond = true;     break;
        case LIR_fle:   cc = LS;    fp_cond = true;     break;
        case LIR_fge:   cc = GE;    fp_cond = true;     break;
        case LIR_fgt:   cc = GT;    fp_cond = true;     break;

        
        case LIR_eq:    cc = EQ;    fp_cond = false;    break;
        case LIR_ov:    cc = VS;    fp_cond = false;    break;
        case LIR_lt:    cc = LT;    fp_cond = false;    break;
        case LIR_le:    cc = LE;    fp_cond = false;    break;
        case LIR_gt:    cc = GT;    fp_cond = false;    break;
        case LIR_ge:    cc = GE;    fp_cond = false;    break;
        case LIR_ult:   cc = LO;    fp_cond = false;    break;
        case LIR_ule:   cc = LS;    fp_cond = false;    break;
        case LIR_ugt:   cc = HI;    fp_cond = false;    break;
        case LIR_uge:   cc = HS;    fp_cond = false;    break;

        
        default:        cc = AL;    fp_cond = false;    break;
    }

    
    if (branchOnFalse)
        cc = OppositeCond(cc);

    
    NanoAssert((cc != AL) && (cc != NV));

    
    NanoAssert(ARM_VFP || !fp_cond);

    
    B_cond(cc, targ);

    
    
    NIns *at = _nIns;

    if (ARM_VFP && fp_cond)
        asm_fcmp(cond);
    else
        asm_cmp(cond);

    return at;
}

void
Assembler::asm_cmp(LIns *cond)
{
    LOpcode condop = cond->opcode();

    
    if ((condop == LIR_ov))
        return;

    LInsp lhs = cond->oprnd1();
    LInsp rhs = cond->oprnd2();

    
    NanoAssert(!lhs->isQuad() && !rhs->isQuad());

    
    if (rhs->isconst()) {
        int c = rhs->imm32();
        if (c == 0 && cond->isop(LIR_eq)) {
            Register r = findRegFor(lhs, GpRegs);
            TST(r,r);
            
        } else if (!rhs->isQuad()) {
            Register r = getBaseReg(condop, lhs, c, GpRegs);
            asm_cmpi(r, c);
        } else {
            NanoAssert(0);
        }
    } else {
        Register ra, rb;
        findRegFor2b(GpRegs, lhs, ra, rhs, rb);
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
Assembler::asm_fcond(LInsp ins)
{
    
    Register r = prepResultReg(ins, AllowableFlagRegs);

    switch (ins->opcode()) {
        case LIR_feq: SETEQ(r); break;
        case LIR_flt: SETLO(r); break; 
        case LIR_fle: SETLS(r); break; 
        case LIR_fge: SETGE(r); break;
        case LIR_fgt: SETGT(r); break;
        default: NanoAssert(0); break;
    }

    asm_fcmp(ins);
}

void
Assembler::asm_cond(LInsp ins)
{
    Register r = prepResultReg(ins, AllowableFlagRegs);
    LOpcode op = ins->opcode();
    
    switch(op)
    {
        case LIR_eq:  SETEQ(r); break;
        case LIR_lt:  SETLT(r); break;
        case LIR_le:  SETLE(r); break;
        case LIR_gt:  SETGT(r); break;
        case LIR_ge:  SETGE(r); break;
        case LIR_ult: SETLO(r); break;
        case LIR_ule: SETLS(r); break;
        case LIR_ugt: SETHI(r); break;
        case LIR_uge: SETHS(r); break;
        case LIR_ov:
            
            
            
            
            if (!ins->oprnd1()->isop(LIR_mul)) {
                SETVS(r);
            } else {
                SETNE(r);
            }
            break;
        default:      NanoAssert(0);  break;
    }
    asm_cmp(ins);
}

void
Assembler::asm_arith(LInsp ins)
{
    LOpcode op = ins->opcode();
    LInsp   lhs = ins->oprnd1();
    LInsp   rhs = ins->oprnd2();

    RegisterMask    allow = GpRegs;

    
    Register        rr = prepResultReg(ins, allow);

    
    
    Register        ra = ( lhs->isUnusedOrHasUnknownReg()
                         ? findSpecificRegFor(lhs, rr)
                         : lhs->getReg() );

    
    NanoAssert(isKnownReg(rr));
    NanoAssert(isKnownReg(ra));
    allow &= ~rmask(rr);
    allow &= ~rmask(ra);

    
    
    
    
    
    
    
    
    
    
    
    if (rhs->isconst() && op != LIR_mul)
    {
        if ((op == LIR_add || op == LIR_iaddp) && lhs->isop(LIR_ialloc)) {
            
            
            Register    rs = prepResultReg(ins, allow);
            int         d = findMemFor(lhs) + rhs->imm32();

            NanoAssert(isKnownReg(rs));
            asm_add_imm(rs, FP, d);
        }

        int32_t imm32 = rhs->imm32();

        switch (op)
        {
            case LIR_iaddp: asm_add_imm(rr, ra, imm32);     break;
            case LIR_add:   asm_add_imm(rr, ra, imm32, 1);  break;
            case LIR_sub:   asm_sub_imm(rr, ra, imm32, 1);  break;
            case LIR_and:   asm_and_imm(rr, ra, imm32);     break;
            case LIR_or:    asm_orr_imm(rr, ra, imm32);     break;
            case LIR_xor:   asm_eor_imm(rr, ra, imm32);     break;
            case LIR_lsh:   LSLi(rr, ra, imm32);            break;
            case LIR_rsh:   ASRi(rr, ra, imm32);            break;
            case LIR_ush:   LSRi(rr, ra, imm32);            break;

            default:
                NanoAssertMsg(0, "Unsupported");
                break;
        }

        
        return;
    }

    

    Register rb;
    if (lhs == rhs) {
        rb = ra;
    } else {
        rb = asm_binop_rhs_reg(ins);
        if (!isKnownReg(rb))
            rb = findRegFor(rhs, allow);
        allow &= ~rmask(rb);
    }
    NanoAssert(isKnownReg(rb));

    switch (op)
    {
        case LIR_iaddp: ADDs(rr, ra, rb, 0);    break;
        case LIR_add:   ADDs(rr, ra, rb, 1);    break;
        case LIR_sub:   SUBs(rr, ra, rb, 1);    break;
        case LIR_and:   ANDs(rr, ra, rb, 0);    break;
        case LIR_or:    ORRs(rr, ra, rb, 0);    break;
        case LIR_xor:   EORs(rr, ra, rb, 0);    break;

        case LIR_mul:
            
            
            
            
            
            
            
            if ((ARM_ARCH > 5) || (rr != rb)) {
                
                
                
                
                
                
                
                
                ALUr_shi(AL, cmp, 1, IP, IP, rr, ASR_imm, 31);
                SMULL(rr, IP, rb, ra);
            } else {
                
                
                
                
                if (rr != ra) {
                    
                    
                    
                    
                    ALUr_shi(AL, cmp, 1, IP, IP, rr, ASR_imm, 31);
                    SMULL(rr, IP, ra, rb);
                } else {
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    

                    NanoAssert(rr != IP);

                    ALUr_shi(AL, cmp, 1, IP, rr, rr, ASR_imm, 31);
                    ALUr_shi(AL, mov, 1, IP, IP, IP, LSR_imm, 16);
                    MUL(rr, IP, IP);
                    ALUi(MI, rsb, 0, IP, IP, 0);
                    ALUr(AL, mov, 1, IP, ra, ra);
                }
            }
            break;
        
        
        
        
        case LIR_lsh:
            LSL(rr, ra, IP);
            ANDi(IP, rb, 0x1f);
            break;
        case LIR_rsh:
            ASR(rr, ra, IP);
            ANDi(IP, rb, 0x1f);
            break;
        case LIR_ush:
            LSR(rr, ra, IP);
            ANDi(IP, rb, 0x1f);
            break;
        default:
            NanoAssertMsg(0, "Unsupported");
            break;
    }
}

void
Assembler::asm_neg_not(LInsp ins)
{
    LOpcode op = ins->opcode();
    Register rr = prepResultReg(ins, GpRegs);

    LIns* lhs = ins->oprnd1();
    
    
    Register ra = ( lhs->isUnusedOrHasUnknownReg()
                  ? findSpecificRegFor(lhs, rr)
                  : lhs->getReg() );
    NanoAssert(isKnownReg(ra));

    if (op == LIR_not)
        MVN(rr, ra);
    else
        RSBS(rr, ra);
}

void
Assembler::asm_ld(LInsp ins)
{
    LOpcode op = ins->opcode();
    LIns* base = ins->oprnd1();
    int d = ins->disp();

    Register rr = prepResultReg(ins, GpRegs);
    Register ra = getBaseReg(op, base, d, GpRegs);

    
    if (op == LIR_ld || op == LIR_ldc) {
        LDR(rr, ra, d);
        return;
    }

    
    if (op == LIR_ldcs) {
        LDRH(rr, ra, d);
        return;
    }

    
    if (op == LIR_ldcb) {
        LDRB(rr, ra, d);
        return;
    }

    NanoAssertMsg(0, "Unsupported instruction in asm_ld");
}

void
Assembler::asm_cmov(LInsp ins)
{
    NanoAssert(ins->opcode() == LIR_cmov);
    LIns* condval = ins->oprnd1();
    LIns* iftrue  = ins->oprnd2();
    LIns* iffalse = ins->oprnd3();

    NanoAssert(condval->isCmp());
    NanoAssert(!iftrue->isQuad() && !iffalse->isQuad());

    const Register rr = prepResultReg(ins, GpRegs);

    
    
    const Register iffalsereg = findRegFor(iffalse, GpRegs & ~rmask(rr));
    switch (condval->opcode()) {
        
        case LIR_eq:    MOVNE(rr, iffalsereg);  break;
        case LIR_lt:    MOVGE(rr, iffalsereg);  break;
        case LIR_le:    MOVGT(rr, iffalsereg);  break;
        case LIR_gt:    MOVLE(rr, iffalsereg);  break;
        case LIR_ge:    MOVLT(rr, iffalsereg);  break;
        case LIR_ult:   MOVHS(rr, iffalsereg);  break;
        case LIR_ule:   MOVHI(rr, iffalsereg);  break;
        case LIR_ugt:   MOVLS(rr, iffalsereg);  break;
        case LIR_uge:   MOVLO(rr, iffalsereg);  break;
        case LIR_ov:
            
            
            
            
            if (!condval->oprnd1()->isop(LIR_mul)) {
                MOVVC(rr, iffalsereg);
            } else {
                MOVEQ(rr, iffalsereg);
            }
            break;
        default: debug_only( NanoAssert(0) );   break;
    }
     findSpecificRegFor(iftrue, rr);
    asm_cmp(condval);
}

void
Assembler::asm_qhi(LInsp ins)
{
    Register rr = prepResultReg(ins, GpRegs);
    LIns *q = ins->oprnd1();
    int d = findMemFor(q);
    LDR(rr, FP, d+4);
}

void
Assembler::asm_qlo(LInsp ins)
{
    Register rr = prepResultReg(ins, GpRegs);
    LIns *q = ins->oprnd1();
    int d = findMemFor(q);
    LDR(rr, FP, d);
}

void
Assembler::asm_param(LInsp ins)
{
    uint32_t a = ins->paramArg();
    uint32_t kind = ins->paramKind();
    if (kind == 0) {
        
        AbiKind abi = _thisfrag->lirbuf->abi;
        uint32_t abi_regcount = abi == ABI_CDECL ? 4 : abi == ABI_FASTCALL ? 2 : abi == ABI_THISCALL ? 1 : 0;
        if (a < abi_regcount) {
            
            prepResultReg(ins, rmask(argRegs[a]));
        } else {
            
            Register r = prepResultReg(ins, GpRegs);
            int d = (a - abi_regcount) * sizeof(intptr_t) + 8;
            LDR(r, FP, d);
        }
    } else {
        
        prepResultReg(ins, rmask(savedRegs[a]));
    }
}

void
Assembler::asm_int(LInsp ins)
{
    Register rr = prepResultReg(ins, GpRegs);
    asm_ld_imm(rr, ins->imm32());
}

void
Assembler::asm_ret(LIns *ins)
{
    genEpilogue();

    
    
    
    

    MOV(IP, R0);

    
    MOV(SP,FP);

    assignSavedRegs();
    LIns *value = ins->oprnd1();
    if (ins->isop(LIR_ret)) {
        findSpecificRegFor(value, R0);
    }
    else {
        NanoAssert(ins->isop(LIR_fret));
        if (ARM_VFP) {
            Register reg = findRegFor(value, FpRegs);
            FMRRD(R0, R1, reg);
        } else {
            NanoAssert(value->isop(LIR_qjoin));
            findSpecificRegFor(value->oprnd1(), R0); 
            findSpecificRegFor(value->oprnd2(), R1); 
        }
    }
}

void
Assembler::asm_promote(LIns *ins)
{
    


    (void)ins;
    NanoAssert(0);
}

void
Assembler::asm_jtbl(LIns* ins, NIns** table)
{
    Register indexreg = findRegFor(ins->oprnd1(), GpRegs);
    Register tmp = registerAlloc(GpRegs & ~rmask(indexreg));
    _allocator.addFree(tmp);
    LDR_scaled(PC, tmp, indexreg, 2);      
    asm_ld_imm(tmp, (int32_t)table);       
}

}
#endif 
