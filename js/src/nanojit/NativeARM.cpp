






































#include "nanojit.h"

#ifdef AVMPLUS_PORTING_API
#include "portapi_nanojit.h"
#endif

#ifdef UNDER_CE
#include <cmnintrin.h>
#endif

#if defined(AVMPLUS_LINUX)
#include <asm/unistd.h>
extern "C" void __clear_cache(char *BEG, char *END);
#endif

#ifdef FEATURE_NANOJIT

namespace nanojit
{

#ifdef NJ_VERBOSE
const char* regNames[] = {"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","FP","IP","SP","LR","PC",
                          "d0","d1","d2","d3","d4","d5","d6","d7","s14"};
#endif

const Register Assembler::argRegs[] = { R0, R1, R2, R3 };
const Register Assembler::retRegs[] = { R0, R1 };

void
Assembler::nInit(AvmCore*)
{
    
    avmplus::AvmCore::cmov_available = true;
}

NIns*
Assembler::genPrologue(RegisterMask needSaving)
{
    



    
    
    uint32_t stackNeeded = 4 * _activation.highwatermark + NJ_STACK_OFFSET;
    uint32_t savingCount = 0;

    uint32_t savingMask = 0;
    savingCount = 9; 
    savingMask = SavedRegs | rmask(FRAME_PTR);
    (void)needSaving;

    
    uint32_t stackPushed = 4 * (2+savingCount);
    uint32_t aligned = alignUp(stackNeeded + stackPushed, NJ_ALIGN_STACK);
    int32_t amt = aligned - stackPushed;

    
    if (amt)
        SUBi(SP, amt); 

    verbose_only( verbose_outputf("         %p:",_nIns); )
    verbose_only( verbose_output("         patch entry"); )
    NIns *patchEntry = _nIns;

    MR(FRAME_PTR, SP);
    PUSH_mask(savingMask|rmask(LR));
    return patchEntry;
}

void
Assembler::nFragExit(LInsp guard)
{
    SideExit* exit = guard->exit();
    Fragment *frag = exit->target;
    GuardRecord *lr;

    if (frag && frag->fragEntry) {
        JMP(frag->fragEntry);
        lr = 0;
    } else {
        
        lr = placeGuardRecord(guard);

        
        
        BL_far(_epilogue);

        
        lr->jmp = _nIns;
    }

    
    MR(SP, FRAME_PTR);

#ifdef NJ_VERBOSE
    if (_frago->core()->config.show_stats) {
        
        
        int fromfrag = int((Fragment*)_thisfrag);
        LDi(argRegs[1], fromfrag);
    }
#endif

    
    LDi(R2, int(lr));
}

NIns*
Assembler::genEpilogue(RegisterMask restore)
{
    BX(LR); 
    MR(R0,R2); 
    RegisterMask savingMask = restore | rmask(FRAME_PTR) | rmask(LR);
    POP_mask(savingMask); 
    return _nIns;
}
    
void
Assembler::asm_call(LInsp ins)
{
    const CallInfo* call = ins->callInfo();
    Reservation *callRes = getresv(ins);

    uint32_t atypes = call->_argtypes;
    uint32_t roffset = 0;

    
#ifdef NJ_ARM_VFP
    ArgSize rsize = (ArgSize)(atypes & 3);
#endif
    atypes >>= 2;

    
    
    
    
    bool arg0IsInt32FollowedByFloat = false;
    while ((atypes & 3) != ARGSIZE_NONE) {
        if (((atypes >> 2) & 3) == ARGSIZE_LO &&
            ((atypes >> 0) & 3) == ARGSIZE_F &&
            ((atypes >> 4) & 3) == ARGSIZE_NONE)
        {
            arg0IsInt32FollowedByFloat = true;
            break;
        }
        atypes >>= 2;
    }

#ifdef NJ_ARM_VFP
    if (rsize == ARGSIZE_F) {
        NanoAssert(ins->opcode() == LIR_fcall);
        NanoAssert(callRes);

        

        Register rr = callRes->reg;
        int d = disp(callRes);
        freeRsrcOf(ins, rr != UnknownReg);

        if (rr != UnknownReg) {
            NanoAssert(IsFpReg(rr));
            FMDRR(rr,R0,R1);
        } else {
            NanoAssert(d);
            STR(R0, FP, d+0);
            STR(R1, FP, d+4);
        }
    }
#endif

    CALL(call);

    ArgSize sizes[10];
    uint32_t argc = call->get_sizes(sizes);
    for(uint32_t i = 0; i < argc; i++) {
        uint32_t j = argc - i - 1;
        ArgSize sz = sizes[j];
        LInsp arg = ins->arg(j);
        

        Register r = (i + roffset) < 4 ? argRegs[i+roffset] : UnknownReg;
#ifdef NJ_ARM_VFP
        if (sz == ARGSIZE_F) {
            if (r == R0 || r == R2) {
                roffset++;
            } else if (r == R1) {
                r = R2;
                roffset++;
            } else {
                r = UnknownReg;
            }

            
            Register sr = findRegFor(arg, FpRegs);

            if (r != UnknownReg) {
                
                
                FMRRD(r, nextreg(r), sr);
            } else {
                asm_pusharg(arg);
            }
        } else {
            asm_arg(sz, arg, r);
        }
#else
        NanoAssert(sz == ARGSIZE_LO || sz == ARGSIZE_Q);
        asm_arg(sz, arg, r);
#endif

        if (i == 0 && arg0IsInt32FollowedByFloat)
            roffset = 1;
    }
}
    
void
Assembler::nMarkExecute(Page* page, int32_t count, bool enable)
{
#ifdef UNDER_CE
    DWORD dwOld;
    VirtualProtect(page, NJ_PAGE_SIZE, PAGE_EXECUTE_READWRITE, &dwOld);
#endif
#ifdef AVMPLUS_PORTING_API
    NanoJIT_PortAPI_MarkExecutable(page, (void*)((int32_t)page+count));
#endif
    (void)page;
    (void)count;
    (void)enable;
}
            
Register
Assembler::nRegisterAllocFromSet(int set)
{
    
#if defined(UNDER_CE)
    Register r;
    r = (Register)_CountLeadingZeros(set);
    r = (Register)(31-r);
    _allocator.free &= ~rmask(r);
    return r;
#elif defined(__ARMCC__)
    register int i;
    __asm { clz i,set }
    Register r = Register(31-i);
    _allocator.free &= ~rmask(r);
    return r;
#else
    
    int i=0;
    while (!(set & rmask((Register)i)))
        i ++;
    _allocator.free &= ~rmask((Register)i);
    return (Register) i;
#endif
}

void
Assembler::nRegisterResetAll(RegAlloc& a)
{
    
    a.clear();
    a.used = 0;
    a.free = rmask(R0) | rmask(R1) | rmask(R2) | rmask(R3) | rmask(R4) | rmask(R5) | FpRegs;
    debug_only(a.managed = a.free);
}

void
Assembler::nPatchBranch(NIns* branch, NIns* target)
{
    

    
    
    

    int32_t offset = PC_OFFSET_FROM(target, branch);

    

    
    
    if (isS24(offset)) {
        
        *branch = (NIns)( COND_AL | (0xA<<24) | ((offset>>2) & 0xFFFFFF) );
    } else {
        
        *branch++ = (NIns)( COND_AL | (0x51<<20) | (PC<<16) | (PC<<12) | ( 0x004 ) );
        *branch = (NIns)target;
    }
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
    else if (op == LIR_param)
        prefer = rmask(imm2register(i->imm8()));

    if (_allocator.free & allow & prefer)
        allow &= prefer;
    return allow;
}

void
Assembler::asm_qjoin(LIns *ins)
{
    int d = findMemFor(ins);
    AvmAssert(d);
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
    findRegFor2(GpRegs, value, rA, base, rB);
    Register ra = rA->reg;
    Register rb = rB->reg;
    STR(ra, rb, dr);
}

void
Assembler::asm_restore(LInsp i, Reservation *resv, Register r)
{
    (void)resv;
    int d = findMemFor(i);

    if (IsFpReg(r)) {
        if (isS8(d >> 2)) {
            FLDD(r, FP, d);
        } else {
            FLDD(r, Scratch, 0);
            arm_ADDi(Scratch, FP, d);
        }
    } else {
        LDR(r, FP, d);
    }

    verbose_only(
        if (_verbose)
            outputf("        restore %s",_thisfrag->lirbuf->names->formatRef(i));
    )
}

void
Assembler::asm_spill(LInsp i, Reservation *resv, bool pop)
{
    (void)i;
    (void)pop;
    
    if (resv->arIndex) {
        int d = disp(resv);
        
        Register rr = resv->reg;
        if (IsFpReg(rr)) {
            if (isS8(d >> 2)) {
                FSTD(rr, FP, d);
            } else {
                FSTD(rr, Scratch, 0);
                arm_ADDi(Scratch, FP, d);
            }
        } else {
            STR(rr, FP, d);
        }

        verbose_only(if (_verbose){
                outputf("        spill %s",_thisfrag->lirbuf->names->formatRef(i));
            }
        )
    }
}

void
Assembler::asm_load64(LInsp ins)
{
    

    LIns* base = ins->oprnd1();
    int offset = ins->oprnd2()->constval();

    Reservation *resv = getresv(ins);
    Register rr = resv->reg;
    int d = disp(resv);

    freeRsrcOf(ins, false);

#ifdef NJ_ARM_VFP
    Register rb = findRegFor(base, GpRegs);

    NanoAssert(rb != UnknownReg);
    NanoAssert(rr == UnknownReg || IsFpReg(rr));

    if (rr != UnknownReg) {
        if (!isS8(offset >> 2) || (offset&3) != 0) {
            FLDD(rr,Scratch,0);
            arm_ADDi(Scratch, rb, offset);
        } else {
            FLDD(rr,rb,offset);
        }
    } else {
        asm_mmq(FP, d, rb, offset);
    }

    
#else
    NanoAssert(resv->reg == UnknownReg && d != 0);
    Register rb = findRegFor(base, GpRegs);
    asm_mmq(FP, d, rb, offset);
#endif

    
}

void
Assembler::asm_store64(LInsp value, int dr, LInsp base)
{
    

#ifdef NJ_ARM_VFP
    Reservation *valResv = getresv(value);
    Register rb = findRegFor(base, GpRegs);

    if (value->isconstq()) {
        const int32_t* p = (const int32_t*) (value-2);

        STR(Scratch, rb, dr);
        LD32_nochk(Scratch, p[0]);
        STR(Scratch, rb, dr+4);
        LD32_nochk(Scratch, p[1]);

        return;
    }

    Register rv = findRegFor(value, FpRegs);

    NanoAssert(rb != UnknownReg);
    NanoAssert(rv != UnknownReg);

    Register baseReg = rb;
    intptr_t baseOffset = dr;

    if (!isS8(dr)) {
        baseReg = Scratch;
        baseOffset = 0;
    }

    FSTD(rv, baseReg, baseOffset);

    if (!isS8(dr)) {
        arm_ADDi(Scratch, rb, dr);
    }

    
    
    if (value->isconstq()) {
        const int32_t* p = (const int32_t*) (value-2);

        underrunProtect(12);

        asm_quad_nochk(rv, p);
    }
#else
    int da = findMemFor(value);
    Register rb = findRegFor(base, GpRegs);
    asm_mmq(rb, dr, FP, da);
#endif
    
}



void
Assembler::asm_quad_nochk(Register rr, const int32_t* p)
{
    
    
    
    

    
    
    
    
    

    FLDD(rr, PC, -16);

    *(--_nIns) = (NIns) p[1];
    *(--_nIns) = (NIns) p[0];

    JMP_nochk(_nIns+2);
}

void
Assembler::asm_quad(LInsp ins)
{
    

    Reservation *res = getresv(ins);
    int d = disp(res);
    Register rr = res->reg;

    NanoAssert(d || rr != UnknownReg);

    const int32_t* p = (const int32_t*) (ins-2);

#ifdef NJ_ARM_VFP
    freeRsrcOf(ins, false);

    if (rr == UnknownReg) {
        underrunProtect(12);

        
        
        

        STR(Scratch, FP, d+4);
        LDR(Scratch, PC, -20);
        STR(Scratch, FP, d);
        LDR(Scratch, PC, -16);

        *(--_nIns) = (NIns) p[1];
        *(--_nIns) = (NIns) p[0];
        JMP_nochk(_nIns+2);
    } else {
        if (d)
            FSTD(rr, FP, d);

        underrunProtect(16);
        asm_quad_nochk(rr, p);
    }
#else
    freeRsrcOf(ins, false);
    if (d) {
        underrunProtect(LD32_size * 2 + 8);
        STR(Scratch, FP, d+4);
        LD32_nochk(Scratch, p[1]);
        STR(Scratch, FP, d);
        LD32_nochk(Scratch, p[0]);
    }
#endif

    
}

bool
Assembler::asm_qlo(LInsp ins, LInsp q)
{
    (void)ins; (void)q;
    return false;
}

void
Assembler::asm_nongp_copy(Register r, Register s)
{
    if ((rmask(r) & FpRegs) && (rmask(s) & FpRegs)) {
        
        FCPYD(r, s);
    } else if ((rmask(r) & GpRegs) && (rmask(s) & FpRegs)) {
        
        
        NanoAssert(0);
        
    } else {
        NanoAssert(0);
    }
}

Register
Assembler::asm_binop_rhs_reg(LInsp ins)
{
    return UnknownReg;
}




void
Assembler::asm_mmq(Register rd, int dd, Register rs, int ds)
{
    
    
    

    
    
    
    NanoAssert(rs != PC);

    
    Register t = registerAlloc(GpRegs & ~(rmask(rd)|rmask(rs)));
    _allocator.addFree(t);

    
    
    STR(Scratch, rd, dd+4);
    STR(t, rd, dd);
    LDR(Scratch, rs, ds+4);
    LDR(t, rs, ds);
}

void
Assembler::asm_pusharg(LInsp arg)
{
    Reservation* argRes = getresv(arg);
    bool quad = arg->isQuad();

    if (argRes && argRes->reg != UnknownReg) {
        if (!quad) {
            STR_preindex(argRes->reg, SP, -4);
        } else {
            FSTD(argRes->reg, SP, 0);
            SUBi(SP, 8);
        }
    } else {
        int d = findMemFor(arg);

        if (!quad) {
            STR_preindex(Scratch, SP, -4);
            LDR(Scratch, FP, d);
        } else {
            STR_preindex(Scratch, SP, -4);
            LDR(Scratch, FP, d+4);
            STR_preindex(Scratch, SP, -4);
            LDR(Scratch, FP, d);
        }
    }
}

void
Assembler::nativePageReset()
{
    _nSlot = 0;
    _nExitSlot = 0;
}

void
Assembler::nativePageSetup()
{
    if (!_nIns)      _nIns     = pageAlloc();
    if (!_nExitIns)  _nExitIns = pageAlloc(true);
    
    
    if (!_nSlot)
    {
        
        
        _nIns--;
        _nExitIns--;

        
        
        _nSlot = pageDataStart(_nIns); 
    }
}

NIns*
Assembler::asm_adjustBranch(NIns* at, NIns* target)
{
    
    
    
    NanoAssert(at[1] == (NIns)( COND_AL | OP_IMM | (1<<23) | (PC<<16) | (LR<<12) | (4) ));
    NanoAssert(at[2] == (NIns)( COND_AL | (0x9<<21) | (0xFFF<<8) | (1<<4) | (IP) ));

    NIns* was = (NIns*) at[3];

    

    at[3] = (NIns)target;

#if defined(UNDER_CE)
    
    FlushInstructionCache(GetCurrentProcess(), NULL, NULL);
#elif defined(AVMPLUS_LINUX)
    __clear_cache((char*)at, (char*)(at+4));
#endif

#ifdef AVMPLUS_PORTING_API
    NanoJIT_PortAPI_FlushInstructionCache(at, at+4);
#endif

    return was;
}

void
Assembler::underrunProtect(int bytes)
{
    intptr_t u = bytes + sizeof(PageHeader)/sizeof(NIns) + 8;
    if ( (samepage(_nIns,_nSlot) && (((intptr_t)_nIns-u) <= intptr_t(_nSlot+1))) ||
         (!samepage((intptr_t)_nIns-u,_nIns)) )
    {
        NIns* target = _nIns;

        _nIns = pageAlloc(_inExit);

        
        
        
        
        
        
        
        _nIns--;

        
        
        _nSlot = pageDataStart(_nIns);

        
        
        
        
        JMP_nochk(target);
    } else if (!_nSlot) {
        
        _nSlot = pageDataStart(_nIns);
    }
}

void
Assembler::BL_far(NIns* addr)
{
    
    
    underrunProtect(16);

    
    

    
    *(--_nIns) = (NIns)((addr));
    
    *(--_nIns) = (NIns)( COND_AL | (0x9<<21) | (0xFFF<<8) | (1<<4) | (IP) );
    
    *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<23) | (PC<<16) | (LR<<12) | (4) );
    
    *(--_nIns) = (NIns)( COND_AL | (0x59<<20) | (PC<<16) | (IP<<12) | (4));

    

    asm_output1("bl %p (32-bit)", addr);
}

void
Assembler::BL(NIns* addr)
{
    intptr_t offs = PC_OFFSET_FROM(addr,_nIns-1);

    

    if (isS24(offs)) {
        
        
        underrunProtect(4);
        offs = PC_OFFSET_FROM(addr,_nIns-1);
    }

    if (isS24(offs)) {
        
        *(--_nIns) = (NIns)( COND_AL | (0xB<<24) | (((offs)>>2) & 0xFFFFFF) );
        asm_output1("bl %p", addr);
    } else {
        BL_far(addr);
    }
}

void
Assembler::CALL(const CallInfo *ci)
{
    intptr_t addr = ci->_address;

    BL((NIns*)addr);
    asm_output1("   (call %s)", ci->_name);
}

void
Assembler::LD32_nochk(Register r, int32_t imm)
{
    if (imm == 0) {
        XOR(r, r);
        return;
    }

    
    

    *(++_nSlot) = (int)imm;

    

    int offset = PC_OFFSET_FROM(_nSlot,_nIns-1);

    NanoAssert(isS12(offset) && (offset < 0));

    asm_output2("  (%d(PC) = 0x%x)", offset, imm);

    LDR_nochk(r,PC,offset);
}















void
Assembler::B_cond_chk(ConditionCode _c, NIns* _t, bool _chk)
{
    int32 offs = PC_OFFSET_FROM(_t,_nIns-1);
    
    if (isS24(offs)) {
        if (_chk) underrunProtect(4);
        offs = PC_OFFSET_FROM(_t,_nIns-1);
    }

    if (isS24(offs)) {
        *(--_nIns) = (NIns)( ((_c)<<28) | (0xA<<24) | (((offs)>>2) & 0xFFFFFF) );
    } else if (_c == AL) {
        if(_chk) underrunProtect(8);
        *(--_nIns) = (NIns)(_t);
        *(--_nIns) = (NIns)( COND_AL | (0x51<<20) | (PC<<16) | (PC<<12) | 0x4 );
    } else if (samepage(_nIns,_nSlot)) {
        if(_chk) underrunProtect(8);
        *(++_nSlot) = (NIns)(_t);
        offs = PC_OFFSET_FROM(_nSlot,_nIns-1);
        NanoAssert(offs < 0);
        *(--_nIns) = (NIns)( ((_c)<<28) | (0x51<<20) | (PC<<16) | (PC<<12) | ((-offs) & 0xFFFFFF) );
    } else {
        if(_chk) underrunProtect(12);
        *(--_nIns) = (NIns)(_t);
        *(--_nIns) = (NIns)( COND_AL | (0xA<<24) | ((-4)>>2) & 0xFFFFFF );
        *(--_nIns) = (NIns)( ((_c)<<28) | (0x51<<20) | (PC<<16) | (PC<<12) | 0x0 );
    }

    asm_output2("%s %p", _c == AL ? "jmp" : "b(cnd)", (void*)(_t));
}

void
Assembler::asm_add_imm(Register rd, Register rn, int32_t imm)
{

    int rot = 16;
    uint32_t immval;
    bool pos;

    if (imm >= 0) {
        immval = (uint32_t) imm;
        pos = true;
    } else {
        immval = (uint32_t) (-imm);
        pos = false;
    }

    while (immval && ((immval & 0x3) == 0)) {
        immval >>= 2;
        rot--;
    }

    rot &= 0xf;

    if (immval < 256) {
        underrunProtect(4);
        if (pos)
            *(--_nIns) = (NIns)( COND_AL | OP_IMM | OP_STAT | (1<<23) | (rn<<16) | (rd<<12) | (rot << 8) | immval );
        else
            *(--_nIns) = (NIns)( COND_AL | OP_IMM | OP_STAT | (1<<22) | (rn<<16) | (rd<<12) | (rot << 8) | immval );
        asm_output3("add %s,%s,%d",gpn(rd),gpn(rn),imm);
    } else {
        

        
        NanoAssert(rn != Scratch);

        *(--_nIns) = (NIns)( COND_AL | OP_STAT | (1<<23) | (rn<<16) | (rd<<12) | (Scratch));
        asm_output3("add %s,%s,%s",gpn(rd),gpn(rn),gpn(Scratch));

        LD32_nochk(Scratch, imm);
    }
}





#ifdef NJ_ARM_VFP

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
    Register rb = (rhs == lhs) ? ra : findRegFor(rhs, FpRegs);

    

    if (op == LIR_fadd)
        FADDD(rr,ra,rb);
    else if (op == LIR_fsub)
        FSUBD(rr,ra,rb);
    else if (op == LIR_fmul)
        FMULD(rr,ra,rb);
    else 
        FDIVD(rr,ra,rb);
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

    
    
    
    if (op == LIR_fge) {
        Register temp = ra;
        ra = rb;
        rb = temp;
        op = LIR_flt;
    } else if (op == LIR_fle) {
        Register temp = ra;
        ra = rb;
        rb = temp;
        op = LIR_fgt;
    }

    
    
    
    
    uint8_t mask = 0x0;
    
    
    
    if (op == LIR_feq)
        
        mask = 0x6;
    else if (op == LIR_flt)
        
        mask = 0x8;
    else if (op == LIR_fgt)
        
        mask = 0x2;
    else
        NanoAssert(0);










    
    
    
    
    
    
    
    CMPi(Scratch, mask);
    
    SHRi(Scratch, 28);
    MRS(Scratch);

    
    FMSTAT();
    FCMPD(ra, rb);
}

Register
Assembler::asm_prep_fcall(Reservation* rR, LInsp ins)
{
    
    return UnknownReg;
}

#endif 

}
#endif 
