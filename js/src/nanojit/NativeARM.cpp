






































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
const char* regNames[] = {"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","fp","ip","sp","lr","pc",
                          "d0","d1","d2","d3","d4","d5","d6","d7","s14"};
const char* condNames[] = {"eq","ne","cs","cc","mi","pl","vs","vc","hi","ls","ge","lt","gt","le","","nv"};
const char* shiftNames[] = { "lsl", "lsl", "lsr", "lsr", "asr", "asr", "ror", "ror" };
#endif

const Register Assembler::argRegs[] = { R0, R1, R2, R3 };
const Register Assembler::retRegs[] = { R0, R1 };
const Register Assembler::savedRegs[] = { R4, R5, R6, R7, R8, R9, R10 };

void
Assembler::nInit(AvmCore*)
{
}

NIns*
Assembler::genPrologue()
{
    



    
    
    uint32_t stackNeeded = STACK_GRANULARITY * _activation.highwatermark + NJ_STACK_OFFSET;
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
        SUBi(SP, amt);

    verbose_only( verbose_outputf("         %p:",_nIns); )
    verbose_only( verbose_output("         patch entry"); )
    NIns *patchEntry = _nIns;

    MOV(FP, SP);
    PUSH_mask(savingMask);
    return patchEntry;
}

void
Assembler::nFragExit(LInsp guard)
{
    SideExit* exit = guard->record()->exit;
    Fragment *frag = exit->target;
    GuardRecord *lr;

    if (frag && frag->fragEntry) {
        JMP_far(frag->fragEntry);
        lr = 0;
    } else {
        
        lr = guard->record();

        
        
        JMP_far(_epilogue);

        
        lr->jmp = _nIns;
    }

    
    MOV(SP, FP);

#ifdef NJ_VERBOSE
    if (_frago->core()->config.show_stats) {
        
        
        int fromfrag = int((Fragment*)_thisfrag);
        LDi(argRegs[1], fromfrag);
    }
#endif

    
    
    
    
    LDi(R2, int(lr));
}

NIns*
Assembler::genEpilogue()
{
    BX(LR); 

    RegisterMask savingMask = rmask(FP) | rmask(LR);

    if (!_thisfrag->lirbuf->explicitSavedRegs)
        for (int i = 0; i < NumSavedRegs; ++i)
            savingMask |= rmask(savedRegs[i]);

    POP_mask(savingMask); 

    MOV(SP,FP);

    
    MOV(R0,R2); 

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
#ifndef UNDER_CE
    
    
    
    
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
#endif

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

    BL((NIns*)(call->_address));

    ArgSize sizes[10];
    uint32_t argc = call->get_sizes(sizes);
    for(uint32_t i = 0; i < argc; i++) {
        uint32_t j = argc - i - 1;
        ArgSize sz = sizes[j];
        LInsp arg = ins->arg(j);
        

        Register r = (i + roffset) < 4 ? argRegs[i+roffset] : UnknownReg;
#ifdef NJ_ARM_VFP
        if (sz == ARGSIZE_F) {
#ifdef UNDER_CE
            if (r >= R0 && r <= R2) {
                
                roffset++;
                FMRRD(r, nextreg(r), sr);
            } else if (r == R3) {
                
                
                STR_preindex(IP, SP, -4);
                FMRDL(IP, sr);
                FMRDH(r, sr);
            } else {
                asm_pusharg(arg);
            }
#else
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
#endif
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
Assembler::nMarkExecute(Page* page, int flags)
{
	NanoAssert(sizeof(Page) == NJ_PAGE_SIZE);
#ifdef UNDER_CE
	static const DWORD kProtFlags[4] = 
	{
		PAGE_READONLY,			
		PAGE_READWRITE,			
		PAGE_EXECUTE_READ,		
		PAGE_EXECUTE_READWRITE	
	};
	DWORD prot = kProtFlags[flags & (PAGE_WRITE|PAGE_EXEC)];
    DWORD dwOld;
    BOOL res = VirtualProtect(page, NJ_PAGE_SIZE, prot, &dwOld);
	if (!res)
	{
		
		NanoAssertMsg(false, "FATAL ERROR: VirtualProtect() failed\n");
	}
#endif
#ifdef AVMPLUS_PORTING_API
    NanoJIT_PortAPI_MarkExecutable(page, (void*)((char*)page+NJ_PAGE_SIZE), flags);
    
#endif
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
    a.free =
        rmask(R0) | rmask(R1) | rmask(R2) | rmask(R3) | rmask(R4) |
        rmask(R5) | rmask(R6) | rmask(R7) | rmask(R8) | rmask(R9) |
        rmask(R10);
#ifdef NJ_ARM_VFP
    a.free |= FpRegs;
#endif

    debug_only(a.managed = a.free);
}

NIns*
Assembler::nPatchBranch(NIns* at, NIns* target)
{
    
    

    NIns* was = 0;

    if (at[0] == (NIns)( COND_AL | (0x51<<20) | (PC<<16) | (PC<<12) | (4) )) {
        
        was = (NIns*) at[1];
    } else {
        
        
        NanoAssert((at[0] & 0xff000000) == (COND_AL | (0xA<<24)));
        was = (NIns*) (((intptr_t)at + 8) + (intptr_t)((at[0] & 0xffffff) << 2));
    }

    
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

    return was;
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
            FLDD(r, IP, 0);
            arm_ADDi(IP, FP, d);
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
                arm_ADDi(IP, FP, d);
            }
        } else {
            STR(rr, FP, d);
        }
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
            FLDD(rr,IP,0);
            arm_ADDi(IP, rb, offset);
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
    
    Register rb = findRegFor(base, GpRegs);

    if (value->isconstq()) {
        const int32_t* p = (const int32_t*) (value-2);

        STR(IP, rb, dr);
        LD32_nochk(IP, p[0]);
        STR(IP, rb, dr+4);
        LD32_nochk(IP, p[1]);

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
        arm_ADDi(IP, rb, dr);
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

        
        
        

        STR(IP, FP, d+4);
        LDR(IP, PC, -20);
        STR(IP, FP, d);
        LDR(IP, PC, -16);

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
        STR(IP, FP, d+4);
        LD32_nochk(IP, p[1]);
        STR(IP, FP, d);
        LD32_nochk(IP, p[0]);
    }
#endif

    
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
Assembler::asm_binop_rhs_reg(LInsp)
{
    return UnknownReg;
}




void
Assembler::asm_mmq(Register rd, int dd, Register rs, int ds)
{
    
    
    

    
    
    
    NanoAssert(rs != PC);

    
    Register t = registerAlloc(GpRegs & ~(rmask(rd)|rmask(rs)));
    _allocator.addFree(t);

    
    
    STR(IP, rd, dd+4);
    STR(t, rd, dd);
    LDR(IP, rs, ds+4);
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
            STR_preindex(IP, SP, -4);
            LDR(IP, FP, d);
        } else {
            STR_preindex(IP, SP, -4);
            LDR(IP, FP, d+4);
            STR_preindex(IP, SP, -4);
            LDR(IP, FP, d);
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

void
Assembler::underrunProtect(int bytes)
{
	NanoAssertMsg(bytes<=LARGEST_UNDERRUN_PROT, "constant LARGEST_UNDERRUN_PROT is too small"); 
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
Assembler::JMP_far(NIns* addr)
{
    
    
    underrunProtect(8);

    intptr_t offs = PC_OFFSET_FROM(addr,_nIns-2);

    if (isS24(offs>>2)) {
        BKPT_nochk();
        *(--_nIns) = (NIns)( COND_AL | (0xA<<24) | ((offs>>2) & 0xFFFFFF) );

        asm_output("b %p", addr);
    } else {
        
        *(--_nIns) = (NIns)((addr));
        
        
        *(--_nIns) = (NIns)( COND_AL | (0x51<<20) | (PC<<16) | (PC<<12) | (4));

        asm_output("b %p (32-bit)", addr);
    }
}

void
Assembler::BL(NIns* addr)
{
    intptr_t offs = PC_OFFSET_FROM(addr,_nIns-1);

    

    
    if (isS24(offs>>2)) {
        underrunProtect(4);

        
        offs = PC_OFFSET_FROM(addr,_nIns-1);
        *(--_nIns) = (NIns)( COND_AL | (0xB<<24) | ((offs>>2) & 0xFFFFFF) );

        asm_output("bl %p", addr);
    } else {
        underrunProtect(12);

        
        *(--_nIns) = (NIns)((addr));
        
        *(--_nIns) = (NIns)( COND_AL | (0x51<<20) | (PC<<16) | (PC<<12) | (4));
        
        *(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<23) | (PC<<16) | (LR<<12) | (4) );

        asm_output("bl %p (32-bit)", addr);
    }
}

void
Assembler::LD32_nochk(Register r, int32_t imm)
{
    if (imm == 0) {
        EOR(r, r, r);
        return;
    }

    
    

    *(++_nSlot) = (int)imm;

    

    int offset = PC_OFFSET_FROM(_nSlot,_nIns-1);

    NanoAssert(isS12(offset) && (offset < 0));

    asm_output("  (%d(PC) = 0x%x)", offset, imm);

    LDR_nochk(r,PC,offset);
}















void
Assembler::B_cond_chk(ConditionCode _c, NIns* _t, bool _chk)
{
    int32_t offs = PC_OFFSET_FROM(_t,_nIns-1);
    
    if (isS24(offs>>2)) {
        if (_chk) underrunProtect(4);
        offs = PC_OFFSET_FROM(_t,_nIns-1);
    }

    if (isS24(offs>>2)) {
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

    asm_output("%s %p", _c == AL ? "jmp" : "b(cnd)", (void*)(_t));
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
        asm_output("add %s,%s,%d",gpn(rd),gpn(rn),imm);
    } else {
        

        
        NanoAssert(rn != IP);

        *(--_nIns) = (NIns)( COND_AL | OP_STAT | (1<<23) | (rn<<16) | (rd<<12) | (IP));
        asm_output("add %s,%s,%s",gpn(rd),gpn(rn),gpn(IP));

        LD32_nochk(IP, imm);
    }
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

    FMSTAT();
    FCMPD(ra, rb);
}

Register
Assembler::asm_prep_fcall(Reservation*, LInsp)
{
    
    return UnknownReg;
}

NIns*
Assembler::asm_branch(bool branchOnFalse, LInsp cond, NIns* targ, bool isfar)
{
    
    
    (void)isfar;

    NIns* at = 0;
    LOpcode condop = cond->opcode();
    NanoAssert(cond->isCond());

    if (condop >= LIR_feq && condop <= LIR_fge)
    {
        ConditionCode cc = NV;

        if (branchOnFalse) {
            switch (condop) {
                case LIR_feq: cc = NE; break;
                case LIR_flt: cc = PL; break;
                case LIR_fgt: cc = LE; break;
                case LIR_fle: cc = HI; break;
                case LIR_fge: cc = LT; break;
                default: NanoAssert(0); break;
            }
        } else {
            switch (condop) {
                case LIR_feq: cc = EQ; break;
                case LIR_flt: cc = MI; break;
                case LIR_fgt: cc = GT; break;
                case LIR_fle: cc = LS; break;
                case LIR_fge: cc = GE; break;
                default: NanoAssert(0); break;
            }
        }

        B_cond(cc, targ);
        asm_output("b(%d) 0x%08x", cc, (unsigned int) targ);

        NIns *at = _nIns;
        asm_fcmp(cond);
        return at;
    }

    
    if (branchOnFalse) {
        if (condop == LIR_eq)
            JNE(targ);
        else if (condop == LIR_ov)
            JNO(targ);
        else if (condop == LIR_cs)
            JNC(targ);
        else if (condop == LIR_lt)
            JNL(targ);
        else if (condop == LIR_le)
            JNLE(targ);
        else if (condop == LIR_gt)
            JNG(targ);
        else if (condop == LIR_ge)
            JNGE(targ);
        else if (condop == LIR_ult)
            JNB(targ);
        else if (condop == LIR_ule)
            JNBE(targ);
        else if (condop == LIR_ugt)
            JNA(targ);
        else 
            JNAE(targ);
    } else 
    {
        if (condop == LIR_eq)
            JE(targ);
        else if (condop == LIR_ov)
            JO(targ);
        else if (condop == LIR_cs)
            JC(targ);
        else if (condop == LIR_lt)
            JL(targ);
        else if (condop == LIR_le)
            JLE(targ);
        else if (condop == LIR_gt)
            JG(targ);
        else if (condop == LIR_ge)
            JGE(targ);
        else if (condop == LIR_ult)
            JB(targ);
        else if (condop == LIR_ule)
            JBE(targ);
        else if (condop == LIR_ugt)
            JA(targ);
        else 
            JAE(targ);
    }
    at = _nIns;
    asm_cmp(cond);
    return at;
}

void
Assembler::asm_cmp(LIns *cond)
{
    LOpcode condop = cond->opcode();

    
    if ((condop == LIR_ov) || (condop == LIR_cs))
        return;

    LInsp lhs = cond->oprnd1();
    LInsp rhs = cond->oprnd2();
    Reservation *rA, *rB;

    
    NanoAssert(!lhs->isQuad() && !rhs->isQuad());

    
    if (rhs->isconst()) {
        int c = rhs->constval();
        if (c == 0 && cond->isop(LIR_eq)) {
            Register r = findRegFor(lhs, GpRegs);
            TEST(r,r);
            
        }
        else if (!rhs->isQuad()) {
            Register r = getBaseReg(lhs, c, GpRegs);
            CMPi(r, c);
        }
    } else {
        findRegFor2(GpRegs, lhs, rA, rhs, rB);
        Register ra = rA->reg;
        Register rb = rB->reg;
        CMP(ra, rb);
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
        case LIR_feq: SET(r,EQ,NE); break;
        case LIR_flt: SET(r,MI,PL); break;
        case LIR_fgt: SET(r,GT,LE); break;
        case LIR_fle: SET(r,LS,HI); break;
        case LIR_fge: SET(r,GE,LT); break;
        default: NanoAssert(0); break;
    }

    asm_fcmp(ins);
}

void
Assembler::asm_cond(LInsp ins)
{
    
    LOpcode op = ins->opcode();
    Register r = prepResultReg(ins, AllowableFlagRegs);
    
    MOVZX8(r,r);
    if (op == LIR_eq)
        SETE(r);
    else if (op == LIR_ov)
        SETO(r);
    else if (op == LIR_cs)
        SETC(r);
    else if (op == LIR_lt)
        SETL(r);
    else if (op == LIR_le)
        SETLE(r);
    else if (op == LIR_gt)
        SETG(r);
    else if (op == LIR_ge)
        SETGE(r);
    else if (op == LIR_ult)
        SETB(r);
    else if (op == LIR_ule)
        SETBE(r);
    else if (op == LIR_ugt)
        SETA(r);
    else 
        SETAE(r);
    asm_cmp(ins);
}

void
Assembler::asm_arith(LInsp ins)
{
    LOpcode op = ins->opcode();
    LInsp lhs = ins->oprnd1();
    LInsp rhs = ins->oprnd2();

    Register rb = UnknownReg;
    RegisterMask allow = GpRegs;
    bool forceReg = (op == LIR_mul || !rhs->isconst());

    
    
    
    if (!forceReg) {
        if (rhs->isconst() && !isU8(rhs->constval()))
            forceReg = true;
    }

    if (lhs != rhs && forceReg) {
        if ((rb = asm_binop_rhs_reg(ins)) == UnknownReg) {
            rb = findRegFor(rhs, allow);
        }
        allow &= ~rmask(rb);
    } else if ((op == LIR_add||op == LIR_addp) && lhs->isop(LIR_alloc) && rhs->isconst()) {
        
        Register rr = prepResultReg(ins, allow);
        int d = findMemFor(lhs) + rhs->constval();
        LEA(rr, d, FP);
    }

    Register rr = prepResultReg(ins, allow);
    Reservation* rA = getresv(lhs);
    Register ra;
    
    if (rA == 0 || (ra = rA->reg) == UnknownReg)
        ra = findSpecificRegFor(lhs, rr);
    

    if (forceReg) {
        if (lhs == rhs)
            rb = ra;

        if (op == LIR_add || op == LIR_addp)
            ADD(rr, rb);
        else if (op == LIR_sub)
            SUB(rr, rb);
        else if (op == LIR_mul)
            MUL(rr, rb);
        else if (op == LIR_and)
            AND(rr, rr, rb);
        else if (op == LIR_or)
            ORR(rr, rr, rb);
        else if (op == LIR_xor)
            EOR(rr, rr, rb);
        else if (op == LIR_lsh)
            SHL(rr, rb);
        else if (op == LIR_rsh)
            SAR(rr, rb);
        else if (op == LIR_ush)
            SHR(rr, rb);
        else
            NanoAssertMsg(0, "Unsupported");
    } else {
        int c = rhs->constval();
        if (op == LIR_add || op == LIR_addp)
            ADDi(rr, c);
        else if (op == LIR_sub)
                    SUBi(rr, c);
        else if (op == LIR_and)
            ANDi(rr, rr, c);
        else if (op == LIR_or)
            ORRi(rr, rr, c);
        else if (op == LIR_xor)
            EORi(rr, rr, c);
        else if (op == LIR_lsh)
            SHLi(rr, c);
        else if (op == LIR_rsh)
            SARi(rr, c);
        else if (op == LIR_ush)
            SHRi(rr, c);
        else
            NanoAssertMsg(0, "Unsupported");
    }

    if (rr != ra)
        MOV(rr,ra);
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
    

    if (op == LIR_not)
        NOT(rr);
    else
        NEG(rr);

    if ( rr != ra )
        MOV(rr,ra);
}

void
Assembler::asm_ld(LInsp ins)
{
    LOpcode op = ins->opcode();
    LIns* base = ins->oprnd1();
    LIns* disp = ins->oprnd2();
    Register rr = prepResultReg(ins, GpRegs);
    int d = disp->constval();
    Register ra = getBaseReg(base, d, GpRegs);

    
    if (op == LIR_ld || op == LIR_ldc) {
        LD(rr, d, ra);
        return;
    }

    
    if (op == LIR_ldcs) {
        LDRH(rr, d, ra);
        return;
    }

    
    if (op == LIR_ldcb) {
        LDRB(rr, d, ra);
        return;
    }

    NanoAssertMsg(0, "Unsupported instruction in asm_ld");
}

void
Assembler::asm_cmov(LInsp ins)
{
    LOpcode op = ins->opcode();
    LIns* condval = ins->oprnd1();
    NanoAssert(condval->isCmp());

    LIns* values = ins->oprnd2();

    NanoAssert(values->opcode() == LIR_2);
    LIns* iftrue = values->oprnd1();
    LIns* iffalse = values->oprnd2();

    NanoAssert(op == LIR_qcmov || (!iftrue->isQuad() && !iffalse->isQuad()));

    const Register rr = prepResultReg(ins, GpRegs);

    
    
    const Register iffalsereg = findRegFor(iffalse, GpRegs & ~rmask(rr));
    if (op == LIR_cmov) {
        switch (condval->opcode()) {
            
            case LIR_eq:    MOVNE(rr, iffalsereg);   break;
            case LIR_ov:    MOVNO(rr, iffalsereg);   break;
            case LIR_cs:    MOVNC(rr, iffalsereg);   break;
            case LIR_lt:    MOVGE(rr, iffalsereg);   break;
            case LIR_le:    MOVG(rr, iffalsereg);    break;
            case LIR_gt:    MOVLE(rr, iffalsereg);   break;
            case LIR_ge:    MOVL(rr, iffalsereg);    break;
            case LIR_ult:   MOVAE(rr, iffalsereg);   break;
            case LIR_ule:   MOVA(rr, iffalsereg);    break;
            case LIR_ugt:   MOVBE(rr, iffalsereg);   break;
            case LIR_uge:   MOVB(rr, iffalsereg);    break;
            default: debug_only( NanoAssert(0) );   break;
        }
    } else if (op == LIR_qcmov) {
        NanoAssert(0);
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
    LD(rr, d+4, FP);
}

void
Assembler::asm_qlo(LInsp ins)
{
    Register rr = prepResultReg(ins, GpRegs);
    LIns *q = ins->oprnd1();
    int d = findMemFor(q);
    LD(rr, d, FP);

#if 0
    LIns *q = ins->oprnd1();

    Reservation *resv = getresv(ins);
    Register rr = resv->reg;
    if (rr == UnknownReg) {
        
        int d = disp(resv);
        freeRsrcOf(ins, false);
        Register qr = findRegFor(q, XmmRegs);
        SSE_MOVDm(d, FP, qr);
    } else {
        freeRsrcOf(ins, false);
        Register qr = findRegFor(q, XmmRegs);
        SSE_MOVD(rr,qr);
    }
#endif
}


void
Assembler::asm_param(LInsp ins)
{
    uint32_t a = ins->imm8();
    uint32_t kind = ins->imm8b();
    if (kind == 0) {
        
        AbiKind abi = _thisfrag->lirbuf->abi;
        uint32_t abi_regcount = abi == ABI_FASTCALL ? 2 : abi == ABI_THISCALL ? 1 : 0;
        if (a < abi_regcount) {
            
            prepResultReg(ins, rmask(argRegs[a]));
        } else {
            
            Register r = prepResultReg(ins, GpRegs);
            int d = (a - abi_regcount) * sizeof(intptr_t) + 8;
            LD(r, d, FP);
        }
    } else {
        
        prepResultReg(ins, rmask(savedRegs[a]));
    }
}

void
Assembler::asm_short(LInsp ins)
{
    Register rr = prepResultReg(ins, GpRegs);
    int32_t val = ins->imm16();
    if (val == 0)
        EOR(rr,rr,rr);
    else
        LDi(rr, val);
}

void
Assembler::asm_int(LInsp ins)
{
    Register rr = prepResultReg(ins, GpRegs);
    int32_t val = ins->imm32();
    if (val == 0)
        EOR(rr,rr,rr);
    else
        LDi(rr, val);
}

#if 0
void
Assembler::asm_quad(LInsp ins)
{
    Reservation *rR = getresv(ins);
    Register rr = rR->reg;
    if (rr != UnknownReg)
    {
        
        _allocator.retire(rr);
        rR->reg = UnknownReg;
        NanoAssert((rmask(rr) & FpRegs) != 0);

        const double d = ins->constvalf();
        const uint64_t q = ins->constvalq();
        if (rmask(rr) & XmmRegs) {
            if (q == 0.0) {
                
                SSE_XORPDr(rr, rr);
            } else if (d == 1.0) {
                
                static const double k_ONE = 1.0;
                LDSDm(rr, &k_ONE);
            } else {
                findMemFor(ins);
                const int d = disp(rR);
                SSE_LDQ(rr, d, FP);
            }
        } else {
            if (q == 0.0) {
                
                FLDZ();
            } else if (d == 1.0) {
                FLD1();
            } else {
                findMemFor(ins);
                int d = disp(rR);
                FLDQ(d,FP);
            }
        }
    }

    
    int d = disp(rR);
    freeRsrcOf(ins, false);
    if (d) {
        const int32_t* p = (const int32_t*) (ins-2);
        STi(FP,d+4,p[1]);
        STi(FP,d,p[0]);
    }
}
#endif

void
Assembler::asm_arg(ArgSize sz, LInsp p, Register r)
{
    if (sz == ARGSIZE_Q) {
        
        if (r != UnknownReg) {
            
            int da = findMemFor(p);
            LEA(r, da, FP);
        } else {
            NanoAssert(0); 
        }
    } else if (sz == ARGSIZE_LO) {
        if (r != UnknownReg) {
            
            if (p->isconst()) {
                LDi(r, p->constval());
            } else {
                Reservation* rA = getresv(p);
                if (rA) {
                    if (rA->reg == UnknownReg) {
                        
                        int d = findMemFor(p);
                        if (p->isop(LIR_alloc)) {
                            LEA(r, d, FP);
                        } else {
                            LD(r, d, FP);
                        }
                    } else {
                        
                        MOV(r, rA->reg);
                    }
                } else {
                    
                    
                    findSpecificRegFor(p, r);
                }
            }
        } else {
            asm_pusharg(p);
        }
    } else {
        NanoAssert(sz == ARGSIZE_F);
        asm_farg(p);
    }
}

}
#endif 
