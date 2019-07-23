







































#include "nanojit.h"

#ifdef AVMPLUS_PORTING_API
#include "portapi_nanojit.h"
#endif

#ifdef UNDER_CE
#include <cmnintrin.h>
#endif

#if defined(AVMPLUS_LINUX)
#include <signal.h>
#include <setjmp.h>
#include <asm/unistd.h>
extern "C" void __clear_cache(void *BEG, void *END);
#endif


#ifdef UNDER_CE
#undef NJ_ARM_EABI
#else
#define NJ_ARM_EABI
#endif

#ifdef FEATURE_NANOJIT

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

    
    
    
    
    
    
    
    NanoAssert(AvmCore::config.arch >= 5);

#if defined(__ARMCC__)
    
    leading_zeroes = __clz(data);
#elif defined(__GNUC__)
    
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

    
    uint32_t    rot;
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
}

NIns*
Assembler::genPrologue()
{
    



    
    
    uint32_t stackNeeded = max_out_args + STACK_GRANULARITY * _activation.highwatermark;
    uint32_t savingCount = 2;

    uint32_t savingMask = rmask(FP) | rmask(LR);

    if (!_thisfrag->lirbuf->explicitSavedRegs) {
        for (int i = 0; i < NumSavedRegs; ++i)
            savingMask |= rmask(savedRegs[i]);
        savingCount += NumSavedRegs;
    }

    
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
        
        

        GuardRecord *   gr = guard->record();

        
        
        
        JMP_far(_epilogue);

        
        
        
        
        
        
        asm_ld_imm(R2, int(gr));

        
        
        gr->jmp = _nIns;
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
    
    
    NanoAssert(AvmCore::config.arch >= 5);

    RegisterMask savingMask = rmask(FP) | rmask(PC);
    if (!_thisfrag->lirbuf->explicitSavedRegs)
        for (int i = 0; i < NumSavedRegs; ++i)
            savingMask |= rmask(savedRegs[i]);

    POP_mask(savingMask); 

    
    
    
    
    
    MOV(SP,FP);

    
    
    MOV(R0,R2); 

    return _nIns;
}







void
Assembler::asm_arg(ArgSize sz, LInsp p, Register r)
{
    NanoAssert(0);
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
    
    
    NanoAssert(AvmCore::config.vfp || arg->isop(LIR_qjoin));

    Register    fp_reg = UnknownReg;

    if (AvmCore::config.vfp) {
        fp_reg = findRegFor(arg, FpRegs);
        NanoAssert(fp_reg != UnknownReg);
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

        
        
        
        if (AvmCore::config.vfp) {
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

        if (AvmCore::config.vfp) {
            
            
            

            
            
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
    NanoAssert(r != UnknownReg);
    if (sz & ARGSIZE_MASK_INT)
    {
        
        if (p->isconst()) {
            asm_ld_imm(r, p->imm32());
        } else {
            Reservation* rA = getresv(p);
            if (rA) {
                if (rA->reg == UnknownReg) {
                    
                    int d = findMemFor(p);
                    if (p->isop(LIR_alloc)) {
                        asm_add_imm(r, FP, d, 0);
                    } else {
                        LDR(r, FP, d);
                    }
                } else {
                    
                    MOV(r, rA->reg);
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
    Reservation*    argRes = getresv(arg);
    bool            isQuad = arg->isQuad();

    if (argRes && (argRes->reg != UnknownReg)) {
        
        
        if (!isQuad) {
            NanoAssert(IsGpReg(argRes->reg));

            STR(argRes->reg, SP, stkd);
        } else {
            
            
            
            
            
            
            NanoAssert(AvmCore::config.vfp);
            NanoAssert(IsFpReg(argRes->reg));

#ifdef NJ_ARM_EABI
            
            NanoAssert((stkd & 7) == 0);
#endif

            FSTD(argRes->reg, SP, stkd);
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

            STR_preindex(IP, SP, stkd+4);
            LDR(IP, FP, d+4);
            STR_preindex(IP, SP, stkd);
            LDR(IP, FP, d);
        }
    }
}

void
Assembler::asm_call(LInsp ins)
{
    CallInfo const *    call = ins->callInfo();
    ArgSize             sizes[MAXARGS];
    uint32_t            argc = call->get_sizes(sizes);

    
    
    
    Register            indirect_reg = UnknownReg;
#if 0   
    
    
    if (call->isIndirect()) {
        
        
        
        RegisterMask    allow = SavedRegs & GpRegs;
        indirect_reg = findRegFor(ins->arg(--argc), allow);
        NanoAssert(indirect_reg != UnknownReg);
    }
#endif

    
    
    
    
    
    
    NanoAssert(AvmCore::config.vfp || ins->isop(LIR_call));

    
    
    
    
    if(AvmCore::config.vfp) {
        
        ArgSize         rsize = (ArgSize)(call->_argtypes & ARGSIZE_MASK_ANY);

        
        
        if (rsize == ARGSIZE_F) {
            Reservation *   callRes = getresv(ins);
            Register        rr = callRes->reg;

            NanoAssert(ins->opcode() == LIR_fcall);

            
            
            
            freeRsrcOf(ins, rr != UnknownReg);

            if (rr == UnknownReg) {
                int d = disp(callRes);
                NanoAssert(d != 0);

                
                
                STR(R0, FP, d+0);
                STR(R1, FP, d+4);
            } else {
                Register    rr = callRes->reg;
                NanoAssert(IsFpReg(rr));

                
                FMDRR(rr, R0, R1);
            }
        }
    }

    
    if (indirect_reg == UnknownReg) {
        verbose_only(if (_logc->lcbits & LC_Assembly)
            outputf("        %p:", _nIns);
        )

        
        
        BranchWithLink((NIns*)(call->_address));
    } else {
        
        
        BLX(indirect_reg);
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
Assembler::nRegisterAllocFromSet(int set)
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
    a.used = 0;
    a.free =
        rmask(R0) | rmask(R1) | rmask(R2) | rmask(R3) | rmask(R4) |
        rmask(R5) | rmask(R6) | rmask(R7) | rmask(R8) | rmask(R9) |
        rmask(R10);
    if (AvmCore::config.vfp)
        a.free |= FpRegs;

    debug_only(a.managed = a.free);
}

void
Assembler::nPatchBranch(NIns* at, NIns* target)
{
    
    

    NIns* was = 0;

    
    
    
    debug_only(
        if (at[0] == (NIns)( COND_AL | (0x51<<20) | (PC<<16) | (PC<<12) | (4) )) {
            
            
            
            was = (NIns*) at[1];
        } else if ((at[0] && 0xff000000) == (NIns)( COND_AL | (0xA<<24))) {
            
            
            
            was = (NIns*) (((intptr_t)at + 8) + (intptr_t)((at[0] & 0xffffff) << 2));
        } else {
            
            
            
            
            
            
            
            
            was = (NIns*)-1;    
        }
    );

    
    NanoAssert((uint32_t)(at[0] & 0xf0000000) == COND_AL);

    
    
    
    
    
    
    
    
    

    intptr_t offs = PC_OFFSET_FROM(target, at);
    if (isS24(offs>>2)) {
        
        
        at[0] = (NIns)( COND_AL | (0xA<<24) | ((offs >> 2) & 0xffffff) );
        
        at[1] = BKPT_insn;
    } else {
        
        
        at[0] = (NIns)( COND_AL | (0x51<<20) | (PC<<16) | (PC<<12) | (4) );
        
        at[1] = (NIns)(target);
    }
    VALGRIND_DISCARD_TRANSLATIONS(at, 2*sizeof(NIns));

#if defined(UNDER_CE)
    
    FlushInstructionCache(GetCurrentProcess(), NULL, NULL);
#elif defined(AVMPLUS_LINUX)
    __clear_cache((char*)at, (char*)(at+3));
#endif

#ifdef AVMPLUS_PORTING_API
    NanoJIT_PortAPI_FlushInstructionCache(at, at+3);
#endif
}

RegisterMask
Assembler::hint(LIns* i, RegisterMask allow )
{
    uint32_t op = i->opcode();
    int prefer = ~0;

    if (op==LIR_call || op==LIR_fcall)
        prefer = rmask(R0);
    else if (op == LIR_callh)
        prefer = rmask(R1);
    else if (op == LIR_iparam)
        prefer = rmask(imm2register(i->paramArg()));

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
    Reservation *rA, *rB;
    Register ra, rb;
    if (base->isop(LIR_alloc)) {
        rb = FP;
        dr += findMemFor(base);
        ra = findRegFor(value, GpRegs);
    } else {
        findRegFor2(GpRegs, value, rA, base, rB);
        ra = rA->reg;
        rb = rB->reg;
    }

    if (!isS12(dr)) {
        STR(ra, IP, 0);
        asm_add_imm(IP, rb, dr);
    } else {
        STR(ra, rb, dr);
    }
}

void
Assembler::asm_restore(LInsp i, Reservation *resv, Register r)
{
    if (i->isop(LIR_alloc)) {
        asm_add_imm(r, FP, disp(resv));
    } else if (IsFpReg(r)) {
        NanoAssert(AvmCore::config.vfp);

        
        
        
        int d = findMemFor(i);
        if (isS8(d >> 2)) {
            FLDD(r, FP, d);
        } else {
            FLDD(r, IP, 0);
            asm_add_imm(IP, FP, d);
        }
#if 0
    
    
    
    
    
    
    } else if (i->isconst()) {
        
        
        if (!resv->arIndex)
            i->resv()->clear();
        asm_ld_imm(r, i->imm32());
#endif
    } else {
        int d = findMemFor(i);
        LDR(r, FP, d);
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
        if (IsFpReg(rr)) {
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

    Reservation *resv = getresv(ins);
    NanoAssert(resv);
    Register rr = resv->reg;
    int d = disp(resv);

    Register rb = findRegFor(base, GpRegs);
    NanoAssert(IsGpReg(rb));
    freeRsrcOf(ins, false);

    

    if (AvmCore::config.vfp && rr != UnknownReg) {
        
        NanoAssert(IsFpReg(rr));

        if (!isS8(offset >> 2) || (offset&3) != 0) {
            FLDD(rr,IP,0);
            asm_add_imm(IP, rb, offset);
        } else {
            FLDD(rr,rb,offset);
        }
    } else {
        
        
        
        NanoAssert(resv->reg == UnknownReg);
        NanoAssert(d != 0);

        
        NanoAssert((d & 0x7) == 0);

        
        asm_mmq(FP, d, rb, offset);
    }

    
}

void
Assembler::asm_store64(LInsp value, int dr, LInsp base)
{
    

    if (AvmCore::config.vfp) {
        
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

        NanoAssert(rb != UnknownReg);
        NanoAssert(rv != UnknownReg);

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

    JMP_nochk(_nIns+2);
}

void
Assembler::asm_quad(LInsp ins)
{
    Reservation *   res = getresv(ins);
    int             d = disp(res);
    Register        rr = res->reg;

    freeRsrcOf(ins, false);

    if (AvmCore::config.vfp &&
        rr != UnknownReg)
    {
        if (d)
            FSTD(rr, FP, d);

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
    if (IsFpReg(r) && IsFpReg(s)) {
        
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

    
    
    
    RegisterMask    free = _allocator.free & GpRegs;

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

void
Assembler::nativePageReset()
{
    _nSlot = 0;
    _startingSlot = 0;
    _nExitSlot = 0;
}

void
Assembler::nativePageSetup()
{
    if (!_nIns)
        codeAlloc(codeStart, codeEnd, _nIns);
    if (!_nExitIns)
        codeAlloc(exitStart, exitEnd, _nExitIns);

    
    
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
            codeAlloc(exitStart, exitEnd, _nIns);
        else
            codeAlloc(codeStart, codeEnd, _nIns);

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
            

            
            
            
            NanoAssert(AvmCore::config.arch >= 5);

            
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
    
    
    
    NanoAssert(AvmCore::config.arch >= 5);

    NanoAssert(IsGpReg(addr));
    
    
    NanoAssert(addr != LR);

    if (chk) {
        underrunProtect(4);
    }

    
    *(--_nIns) = (NIns)( (COND_AL) | (0x12<<20) | (0xFFF<<8) | (0x3<<4) | (addr) );
    asm_output("blx ip");
}







void
Assembler::asm_ldr_chk(Register d, Register b, int32_t off, bool chk)
{
    if (IsFpReg(d)) {
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

    
    
    
    
    if (AvmCore::config.thumb2 && (d != PC)) {
        
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
    NanoAssert(isS12(offset) && (offset < 0));

    
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
    

    
    
    
    NanoAssert((_t != 0) || (_c == AL));

    
    if (_chk && isS24(offs>>2) && (_t != 0)) {
        underrunProtect(4);
        
        
        offs = PC_OFFSET_FROM(_t,_nIns-1);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if (isS24(offs>>2) && (_t != 0)) {
        
        *(--_nIns) = (NIns)( ((_c)<<28) | (0xA<<24) | (((offs)>>2) & 0xFFFFFF) );
    } else if (_c == AL) {
        if(_chk) underrunProtect(8);
        *(--_nIns) = (NIns)(_t);
        *(--_nIns) = (NIns)( COND_AL | (0x51<<20) | (PC<<16) | (PC<<12) | 0x4 );
    } else if (PC_OFFSET_FROM(_nSlot, _nIns-1) > -0x1000) {
        if(_chk) underrunProtect(8);
        *(++_nSlot) = (NIns)(_t);
        offs = PC_OFFSET_FROM(_nSlot,_nIns-1);
        NanoAssert(offs < 0);
        *(--_nIns) = (NIns)( ((_c)<<28) | (0x51<<20) | (PC<<16) | (PC<<12) | ((-offs) & 0xFFFFFF) );
    } else {
        if(_chk) underrunProtect(12);
        
        *(--_nIns) = (NIns)(_t);
        
        
        
        *(--_nIns) = (NIns)( COND_AL | (0xA<<24) | 0x0 );
        
        *(--_nIns) = (NIns)( ((_c)<<28) | (0x51<<20) | (PC<<16) | (PC<<12) | 0x0 );
    }

    asm_output("b%s %p", condNames[_c], (void*)(_t));
}





void
Assembler::asm_i2f(LInsp ins)
{
    Register rr = prepResultReg(ins, FpRegs);
    Register srcr = findRegFor(ins->oprnd1(), GpRegs);

    
    NanoAssert(srcr != UnknownReg);

    FSITOD(rr, FpSingleScratch);
    FMSR(FpSingleScratch, srcr);
}

void
Assembler::asm_u2f(LInsp ins)
{
    Register rr = prepResultReg(ins, FpRegs);
    Register sr = findRegFor(ins->oprnd1(), GpRegs);

    
    NanoAssert(sr != UnknownReg);

    FUITOD(rr, FpSingleScratch);
    FMSR(FpSingleScratch, sr);
}

void
Assembler::asm_fneg(LInsp ins)
{
    LInsp lhs = ins->oprnd1();
    Register rr = prepResultReg(ins, FpRegs);

    Reservation* rA = getresv(lhs);
    Register sr;

    if (!rA || rA->reg == UnknownReg)
        sr = findRegFor(lhs, FpRegs);
    else
        sr = rA->reg;

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

    Register ra = findRegFor(lhs, FpRegs);
    Register rb = findRegFor(rhs, FpRegs);

    FMSTAT();
    FCMPD(ra, rb);
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

    
    
    ConditionCode cc = AL;

    
    bool    fp_cond;

    
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

    
    NanoAssert(AvmCore::config.vfp || !fp_cond);

    
    B_cond(cc, targ);

    
    
    NIns *at = _nIns;

    if (fp_cond)
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
    Reservation *rA, *rB;

    
    NanoAssert(!lhs->isQuad() && !rhs->isQuad());

    
    if (rhs->isconst()) {
        int c = rhs->imm32();
        if (c == 0 && cond->isop(LIR_eq)) {
            Register r = findRegFor(lhs, GpRegs);
            TST(r,r);
            
        } else if (!rhs->isQuad()) {
            Register r = getBaseReg(lhs, c, GpRegs);
            asm_cmpi(r, c);
        } else {
            NanoAssert(0);
        }
    } else {
        findRegFor2(GpRegs, lhs, rA, rhs, rB);
        Register ra = rA->reg;
        Register rb = rB->reg;
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
            CMP(r, IP);
            asm_ld_imm(IP, imm);
        }
    } else {
        if (imm < 256) {
            ALUi(AL, cmp, 1, 0, r, imm);
        } else {
            CMP(r, IP);
            asm_ld_imm(IP, imm);
        }
    }
}

void
Assembler::asm_loop(LInsp ins, NInsList& loopJumps)
{
    

    JMP_far(0);
    loopJumps.add(_nIns);

    
    
    if (ins->record()->exit->target != _thisfrag)
        MOV(SP,FP);
}

void
Assembler::asm_fcond(LInsp ins)
{
    
    Register r = prepResultReg(ins, AllowableFlagRegs);

    switch (ins->opcode()) {
        case LIR_feq: SET(r,EQ); break;
        case LIR_flt: SET(r,LO); break;
        case LIR_fle: SET(r,LS); break;
        case LIR_fge: SET(r,GE); break;
        case LIR_fgt: SET(r,GT); break;
        default: NanoAssert(0); break;
    }

    asm_fcmp(ins);
}

void
Assembler::asm_cond(LInsp ins)
{
    Register r = prepResultReg(ins, AllowableFlagRegs);
    switch(ins->opcode())
    {
        case LIR_eq:    SET(r,EQ);      break;
        case LIR_ov:    SET(r,VS);      break;
        case LIR_lt:    SET(r,LT);      break;
        case LIR_le:    SET(r,LE);      break;
        case LIR_gt:    SET(r,GT);      break;
        case LIR_ge:    SET(r,GE);      break;
        case LIR_ult:   SET(r,LO);      break;
        case LIR_ule:   SET(r,LS);      break;
        case LIR_ugt:   SET(r,HI);      break;
        case LIR_uge:   SET(r,HS);      break;
        default:        NanoAssert(0);  break;
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
    Reservation *   rA = getresv(lhs);
    Register        ra = UnknownReg;
    Register        rb = UnknownReg;

    
    if (!rA || (ra = rA->reg) == UnknownReg)
        ra = findSpecificRegFor(lhs, rr);

    
    NanoAssert(rr != UnknownReg);
    NanoAssert(ra != UnknownReg);
    allow &= ~rmask(rr);
    allow &= ~rmask(ra);

    
    
    
    
    
    
    
    
    
    
    
    if (rhs->isconst() && op != LIR_mul)
    {
        if ((op == LIR_add || op == LIR_iaddp) && lhs->isop(LIR_alloc)) {
            
            
            Register    rs = prepResultReg(ins, allow);
            int         d = findMemFor(lhs) + rhs->imm32();

            NanoAssert(rs != UnknownReg);
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

    

    if (lhs == rhs) {
        rb = ra;
    } else {
        rb = asm_binop_rhs_reg(ins);
        if (rb == UnknownReg)
            rb = findRegFor(rhs, allow);
        allow &= ~rmask(rb);
    }
    NanoAssert(rb != UnknownReg);

    switch (op)
    {
        case LIR_iaddp: ADDs(rr, ra, rb, 0);    break;
        case LIR_add:   ADDs(rr, ra, rb, 1);    break;
        case LIR_sub:   SUBs(rr, ra, rb, 1);    break;
        case LIR_and:   ANDs(rr, ra, rb, 0);    break;
        case LIR_or:    ORRs(rr, ra, rb, 0);    break;
        case LIR_xor:   EORs(rr, ra, rb, 0);    break;

        case LIR_mul:
            
            
            
            
            
            
            
            if ((AvmCore::config.arch > 5) || (rr != rb)) {
                
                
                MUL(rr, rb, ra);
            } else {
                
                

                
                if (rr != ra) {
                    
                    
                    MUL(rr, ra, rb);
                } else {
                    
                    

                    
                    
                    NanoAssert(ra != IP);
                    NanoAssert(rr != IP);

                    
                    MUL(rr, IP, rb);
                    MOV(IP, ra);
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
    Reservation *rA = getresv(lhs);
    
    Register ra;
    if (rA == 0 || (ra=rA->reg) == UnknownReg)
        ra = findSpecificRegFor(lhs, rr);
    
    NanoAssert(ra != UnknownReg);

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
    Register ra = getBaseReg(base, d, GpRegs);

    
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
        case LIR_ov:    MOVVC(rr, iffalsereg);  break;
        case LIR_lt:    MOVGE(rr, iffalsereg);  break;
        case LIR_le:    MOVGT(rr, iffalsereg);  break;
        case LIR_gt:    MOVLE(rr, iffalsereg);  break;
        case LIR_ge:    MOVLT(rr, iffalsereg);  break;
        case LIR_ult:   MOVCS(rr, iffalsereg);  break;
        case LIR_ule:   MOVHI(rr, iffalsereg);  break;
        case LIR_ugt:   MOVLS(rr, iffalsereg);  break;
        case LIR_uge:   MOVCC(rr, iffalsereg);  break;
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
        uint32_t abi_regcount = abi == ABI_FASTCALL ? 2 : abi == ABI_THISCALL ? 1 : 0;
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
    if (_nIns != _epilogue) {
        B(_epilogue);
    }
    assignSavedRegs();
    LIns *value = ins->oprnd1();
    if (ins->isop(LIR_ret)) {
        findSpecificRegFor(value, R0);
    }
    else {
        NanoAssert(ins->isop(LIR_fret));
#ifdef NJ_ARM_VFP
        Register reg = findRegFor(value, FpRegs);
        FMRRD(R0, R1, reg);
#else
        NanoAssert(value->isop(LIR_qjoin));
        findSpecificRegFor(value->oprnd1(), R0); 
        findSpecificRegFor(value->oprnd2(), R1); 
#endif
    }
}

void
Assembler::asm_promote(LIns *ins)
{
    


    (void)ins;
    NanoAssert(0);
}

}

#endif 
