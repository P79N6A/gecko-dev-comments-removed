







































#include "nanojit.h"

#ifdef _MSC_VER
    
    #pragma warning(disable:4310) // cast truncates constant value
#endif

namespace nanojit
{
    #if defined FEATURE_NANOJIT && defined NANOJIT_IA32

    #ifdef NJ_VERBOSE
        const char *regNames[] = {
            "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
            "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7",
            "f0"
        };
    #endif

    #define TODO(x) do{ verbose_only(outputf(#x);) NanoAssertMsgf(false, "%s", #x); } while(0)

    const Register Assembler::argRegs[] = { ECX, EDX };
    const Register Assembler::retRegs[] = { EAX, EDX };
    const Register Assembler::savedRegs[] = { EBX, ESI, EDI };

    const static uint8_t max_abi_regs[] = {
        2, 
        1, 
        0, 
        0  
    };

    void Assembler::nInit(AvmCore*)
    {
    }

    void Assembler::nBeginAssembly() {
        max_stk_args = 0;
    }

    NIns* Assembler::genPrologue()
    {
        
        uint32_t stackNeeded = max_stk_args + STACK_GRANULARITY * _activation.stackSlotsNeeded();

        uint32_t stackPushed =
            STACK_GRANULARITY + 
            STACK_GRANULARITY; 

        uint32_t aligned = alignUp(stackNeeded + stackPushed, NJ_ALIGN_STACK);
        uint32_t amt = aligned - stackPushed;

        
        
        if (amt)
        {
            SUBi(SP, amt);
        }

        verbose_only( asm_output("[frag entry]"); )
        NIns *fragEntry = _nIns;
        MR(FP, SP); 
        PUSHr(FP); 

        return fragEntry;
    }

    void Assembler::nFragExit(LInsp guard)
    {
        SideExit *exit = guard->record()->exit;
        Fragment *frag = exit->target;
        GuardRecord *lr = 0;
        bool destKnown = (frag && frag->fragEntry);

        
        
        if (guard->isop(LIR_xtbl)) {
            lr = guard->record();
            Register r = EDX;
            SwitchInfo* si = guard->record()->exit->switchInfo;
            if (!_epilogue)
                _epilogue = genEpilogue();
            emitJumpTable(si, _epilogue);
            JMP_indirect(r);
            LEAmi4(r, si->table, r);
        } else {
            
            if (destKnown) {
                JMP(frag->fragEntry);
                lr = 0;
            } else {  
                if (!_epilogue)
                    _epilogue = genEpilogue();
                lr = guard->record();
                JMP_long(_epilogue);
                lr->jmp = _nIns;
            }
        }

        
        verbose_only(
           if (_logc->lcbits & LC_FragProfile) {
              INCLi( &guard->record()->profCount );
           }
        )

        
        MR(SP,FP);

        
        asm_int(EAX, int(lr), true);
    }

    NIns *Assembler::genEpilogue()
    {
        RET();
        POPr(FP); 

        return  _nIns;
    }

    void Assembler::asm_call(LInsp ins)
    {
        Register retReg = ( ins->isop(LIR_fcall) ? FST0 : retRegs[0] );
        deprecated_prepResultReg(ins, rmask(retReg));

        
        

        evictScratchRegsExcept(0);

        const CallInfo* call = ins->callInfo();
        
        uint32_t iargs = call->count_iargs();
        int32_t fargs = call->count_args() - iargs;

        bool indirect = call->isIndirect();
        if (indirect) {
            
            iargs --;
        }

        AbiKind abi = call->_abi;
        uint32_t max_regs = max_abi_regs[abi];
        if (max_regs > iargs)
            max_regs = iargs;

        int32_t istack = iargs-max_regs;  
        int32_t extra = 0;
        const int32_t pushsize = 4*istack + 8*fargs; 

#if _MSC_VER
        
        
        
        uint32_t align = 4;
#else
        uint32_t align = NJ_ALIGN_STACK;
#endif

        if (pushsize) {
            if (_config.i386_fixed_esp) {
                
                
                
                
                if (abi != ABI_CDECL)
                    SUBi(SP, pushsize);
            } else {
                
                
                extra = alignUp(pushsize, align) - pushsize;
                if (call->_abi == ABI_CDECL) {
                    
                    ADDi(SP, extra+pushsize);
                } else if (extra > 0) {
                    ADDi(SP, extra);
                }
            }
        }

        NanoAssert(ins->isop(LIR_pcall) || ins->isop(LIR_fcall));
        if (!indirect) {
            CALL(call);
        }
        else {
            
            
            
            
            CALLr(call, EAX);
        }

        
        NanoAssert(_allocator.isFree(FST0));
        
        uint32_t n = 0;

        ArgSize sizes[MAXARGS];
        uint32_t argc = call->get_sizes(sizes);
        int32_t stkd = 0;

        if (indirect) {
            argc--;
            asm_arg(ARGSIZE_P, ins->arg(argc), EAX, stkd);
            if (!_config.i386_fixed_esp)
                stkd = 0;
        }

        for(uint32_t i=0; i < argc; i++)
        {
            uint32_t j = argc-i-1;
            ArgSize sz = sizes[j];
            Register r = UnspecifiedReg;
            if (n < max_regs && sz != ARGSIZE_F) {
                r = argRegs[n++]; 
            }
            asm_arg(sz, ins->arg(j), r, stkd);
            if (!_config.i386_fixed_esp)
                stkd = 0;
        }

        if (_config.i386_fixed_esp) {
            if (pushsize > max_stk_args)
                max_stk_args = pushsize;
        } else if (extra > 0) {
            SUBi(SP, extra);
        }
    }

    Register Assembler::nRegisterAllocFromSet(RegisterMask set)
    {
        Register r;
        RegAlloc &regs = _allocator;
    #ifdef _MSC_VER
        _asm
        {
            mov ecx, regs
            bsf eax, set                    
            btr RegAlloc::free[ecx], eax    
            mov r, eax
        }
	#elif defined __SUNPRO_CC
        
        
        
         asm(
             "bsf    %1, %%edi\n\t"
             "btr    %%edi, (%2)\n\t"
             "movl   %%edi, %0\n\t"
             : "=a"(r) : "d"(set), "c"(&regs.free) : "%edi", "memory" );
    #else
        asm(
            "bsf    %1, %%eax\n\t"
            "btr    %%eax, %2\n\t"
            "movl   %%eax, %0\n\t"
            : "=m"(r) : "m"(set), "m"(regs.free) : "%eax", "memory" );
    #endif 
        return r;
    }

    void Assembler::nRegisterResetAll(RegAlloc& a)
    {
        
        a.clear();
        a.free = SavedRegs | ScratchRegs;
        if (!_config.i386_sse2)
            a.free &= ~XmmRegs;
        debug_only( a.managed = a.free; )
    }

    void Assembler::nPatchBranch(NIns* branch, NIns* targ)
    {
        intptr_t offset = intptr_t(targ) - intptr_t(branch);
        if (branch[0] == JMP32) {
            *(int32_t*)&branch[1] = offset - 5;
        } else if (branch[0] == JCC32) {
            *(int32_t*)&branch[2] = offset - 6;
        } else
            NanoAssertMsg(0, "Unknown branch type in nPatchBranch");
    }

    RegisterMask Assembler::hint(LIns* ins)
    {
        uint32_t op = ins->opcode();
        int prefer = 0;

        if (op == LIR_icall) {
            prefer = rmask(retRegs[0]);
        }
        else if (op == LIR_fcall) {
            prefer = rmask(FST0);
        }
        else if (op == LIR_param) {
            uint8_t arg = ins->paramArg();
            if (ins->paramKind() == 0) {
                uint32_t max_regs = max_abi_regs[_thisfrag->lirbuf->abi];
                if (arg < max_regs)
                    prefer = rmask(argRegs[arg]);
            } else {
                if (arg < NumSavedRegs)
                    prefer = rmask(savedRegs[arg]);
            }
        }
        else if (ins->isCmp()) {
            prefer = AllowableFlagRegs;
        }
        else if (ins->isconst()) {
            prefer = ScratchRegs;
        }

        return prefer;
    }

    
    
    void Assembler::asm_restore(LInsp ins, Register r)
    {
        NanoAssert(ins->getReg() == r);

        uint32_t arg;
        uint32_t abi_regcount;
        if (ins->isop(LIR_alloc)) {
            
            
            
            NanoAssert(ins->isInAr());  
            LEA(r, arDisp(ins), FP);

        } else if (ins->isconst()) {
            asm_int(r, ins->imm32(), false);
            ins->clearReg();

        } else if (ins->isconstq()) {
            asm_quad(r, ins->imm64(), ins->imm64f(), false);
            ins->clearReg();

        } else if (ins->isop(LIR_param) && ins->paramKind() == 0 &&
            (arg = ins->paramArg()) >= (abi_regcount = max_abi_regs[_thisfrag->lirbuf->abi])) {
            

            
            
            
            
            
            
            
            
            int d = (arg - abi_regcount) * sizeof(intptr_t) + 8;
            LD(r, d, FP);
            ins->clearReg();

        } else {
            int d = findMemFor(ins);
            if (ins->isI32()) {
                NanoAssert(rmask(r) & GpRegs);
                LD(r, d, FP);
            } else {
                NanoAssert(ins->isF64());
                if (rmask(r) & XmmRegs) {
                    SSE_LDQ(r, d, FP);
                } else {
                    NanoAssert(rmask(r) & x87Regs);
                    FLDQ(d, FP);
                }
            }
        }
    }

    void Assembler::asm_store32(LOpcode op, LIns* value, int dr, LIns* base)
    {
        if (value->isconst()) {
            Register rb = getBaseReg(base, dr, GpRegs);
            int c = value->imm32();
            switch (op) {
                case LIR_stb:
                    ST8i(rb, dr, c);
                    break;
                case LIR_sts:
                    ST16i(rb, dr, c);
                    break;
                case LIR_sti:
                    STi(rb, dr, c);
                    break;
                default:
                    NanoAssertMsg(0, "asm_store32 should never receive this LIR opcode");
                    break;
            }
        }
        else
        {
            
            const RegisterMask SrcRegs = (op == LIR_stb) ?
                            (1<<EAX | 1<<ECX | 1<<EDX | 1<<EBX) :
                            GpRegs;

            Register ra, rb;
            if (base->isconst()) {
                
                rb = UnspecifiedReg;
                dr += base->imm32();
                ra = findRegFor(value, SrcRegs);
            } else {
                getBaseReg2(SrcRegs, value, ra, GpRegs, base, rb, dr);
            }
            switch (op) {
                case LIR_stb:
                    ST8(rb, dr, ra);
                    break;
                case LIR_sts:
                    ST16(rb, dr, ra);
                    break;
                case LIR_sti:
                    ST(rb, dr, ra);
                    break;
                default:
                    NanoAssertMsg(0, "asm_store32 should never receive this LIR opcode");
                    break;
            }
        }
    }

    void Assembler::asm_spill(Register rr, int d, bool pop, bool quad)
    {
        (void)quad;
        if (d)
        {
            if (rmask(rr) & GpRegs) {
                ST(FP, d, rr);
            } else if (rmask(rr) & XmmRegs) {
                SSE_STQ(d, FP, rr);
            } else {
                NanoAssert(rmask(rr) & x87Regs);
                FSTQ((pop?1:0), d, FP);
            }
        }
        else if (pop && (rmask(rr) & x87Regs))
        {
            
            FSTP(FST0);
        }
    }

    void Assembler::asm_load64(LInsp ins)
    {
        LIns* base = ins->oprnd1();
        int db = ins->disp();

        Register rr = UnspecifiedReg;   
        bool inReg = ins->isInReg();
        if (inReg)
            rr = ins->getReg();

        if (inReg && (rmask(rr) & XmmRegs))
        {
            deprecated_freeRsrcOf(ins, false);
            Register rb = getBaseReg(base, db, GpRegs);
            switch (ins->opcode()) {
                case LIR_ldf:
                case LIR_ldfc:
                    SSE_LDQ(rr, db, rb);
                    break;
                case LIR_ld32f:
                case LIR_ldc32f:
                    SSE_CVTSS2SD(rr, rr);
                    SSE_LDSS(rr, db, rb);
                    SSE_XORPDr(rr,rr);
                    break;
                default:
                    NanoAssertMsg(0, "asm_load64 should never receive this LIR opcode");
                    break;
            }
        }
        else
        {
            bool inAr = ins->isInAr();
            int dr = 0;
            if (inAr)
                dr = arDisp(ins);
            Register rb;
            if (base->isop(LIR_alloc)) {
                rb = FP;
                db += findMemFor(base);
            } else {
                rb = findRegFor(base, GpRegs);
            }
            ins->clearReg();

            switch (ins->opcode()) {
                case LIR_ldf:
                case LIR_ldfc:
                    
                    if (inAr)
                        asm_mmq(FP, dr, rb, db);
                    deprecated_freeRsrcOf(ins, false);
                    if (inReg)
                    {
                        NanoAssert(rmask(rr)&x87Regs);
                        _allocator.retire(rr);
                        FLDQ(db, rb);
                    }
                    break;
                case LIR_ld32f:
                case LIR_ldc32f:
                    deprecated_freeRsrcOf(ins, false);
                    if (inReg)
                    {
                        NanoAssert(rmask(rr)&x87Regs);
                        _allocator.retire(rr);
                        
                        
                        if (inAr)
                            FSTQ(0, dr, FP);
                        FLD32(db, rb);
                    }
                    else
                    {
                        
                        
                        NanoAssert(inAr);
                        FSTPQ(dr, FP);
                        FLD32(db, rb);
                    }
                    break;
                default:
                    NanoAssertMsg(0, "asm_load64 should never receive this LIR opcode");
                    break;
            }
        }
    }

    void Assembler::asm_store64(LOpcode op, LInsp value, int dr, LInsp base)
    {
        Register rb = getBaseReg(base, dr, GpRegs);

        if (op == LIR_st32f) {
            bool pop = !value->isInReg();
            Register rv = ( pop
                          ? findRegFor(value, _config.i386_sse2 ? XmmRegs : FpRegs)
                          : value->getReg() );

            if (rmask(rv) & XmmRegs) {
                
                Register rt = registerAllocTmp(XmmRegs);

                
                SSE_STSS(dr, rb, rt);
                SSE_CVTSD2SS(rt, rv);
                SSE_XORPDr(rt, rt);     

            } else {
                FST32(pop?1:0, dr, rb);
            }

        } else if (value->isconstq()) {
            STi(rb, dr+4, value->imm64_1());
            STi(rb, dr,   value->imm64_0());

        } else if (value->isop(LIR_ldf) || value->isop(LIR_ldfc)) {
            
            
            

            
            
            
            

            if (_config.i386_sse2) {
                Register rv = findRegFor(value, XmmRegs);
                SSE_STQ(dr, rb, rv);
            } else {
                int da = findMemFor(value);
                asm_mmq(rb, dr, FP, da);
            }

        } else {
            bool pop = !value->isInReg();
            Register rv = ( pop
                          ? findRegFor(value, _config.i386_sse2 ? XmmRegs : FpRegs)
                          : value->getReg() );

            if (rmask(rv) & XmmRegs) {
                SSE_STQ(dr, rb, rv);
            } else {
                FSTQ(pop?1:0, dr, rb);
            }
        }
    }

    
    
    void Assembler::asm_mmq(Register rd, int dd, Register rs, int ds)
    {
        
        
        
        if (_config.i386_sse2) {
            Register t = registerAllocTmp(XmmRegs);
            SSE_STQ(dd, rd, t);
            SSE_LDQ(t, ds, rs);
        } else {
            
            
            Register t = registerAllocTmp(GpRegs & ~(rmask(rd)|rmask(rs)));
            ST(rd, dd+4, t);
            LD(t, ds+4, rs);
            ST(rd, dd, t);
            LD(t, ds, rs);
        }
    }

    NIns* Assembler::asm_branch(bool branchOnFalse, LInsp cond, NIns* targ)
    {
        LOpcode condop = cond->opcode();
        NanoAssert(cond->isCmp());

        
        if (condop >= LIR_feq && condop <= LIR_fge) {
            return asm_fbranch(branchOnFalse, cond, targ);
        }

        if (branchOnFalse) {
            
            switch (condop) {
            case LIR_eq:    JNE(targ);      break;
            case LIR_lt:    JNL(targ);      break;
            case LIR_le:    JNLE(targ);     break;
            case LIR_gt:    JNG(targ);      break;
            case LIR_ge:    JNGE(targ);     break;
            case LIR_ult:   JNB(targ);      break;
            case LIR_ule:   JNBE(targ);     break;
            case LIR_ugt:   JNA(targ);      break;
            case LIR_uge:   JNAE(targ);     break;
            default:        NanoAssert(0);  break;
            }
        } else {
            
            switch (condop) {
            case LIR_eq:    JE(targ);       break;
            case LIR_lt:    JL(targ);       break;
            case LIR_le:    JLE(targ);      break;
            case LIR_gt:    JG(targ);       break;
            case LIR_ge:    JGE(targ);      break;
            case LIR_ult:   JB(targ);       break;
            case LIR_ule:   JBE(targ);      break;
            case LIR_ugt:   JA(targ);       break;
            case LIR_uge:   JAE(targ);      break;
            default:        NanoAssert(0);  break;
            }
        }
        NIns* at = _nIns;
        asm_cmp(cond);
        return at;
    }

    void Assembler::asm_branch_xov(LOpcode, NIns* target)
    {
        JO(target);
    }

    void Assembler::asm_switch(LIns* ins, NIns* exit)
    {
        LIns* diff = ins->oprnd1();
        findSpecificRegFor(diff, EDX);
        JMP(exit);
    }

    void Assembler::asm_jtbl(LIns* ins, NIns** table)
    {
        Register indexreg = findRegFor(ins->oprnd1(), GpRegs);
        JMP_indexed(indexreg, 2, table);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void Assembler::asm_cmp(LIns *cond)
    {
        LInsp lhs = cond->oprnd1();
        LInsp rhs = cond->oprnd2();

        NanoAssert(lhs->isI32() && rhs->isI32());

        
        if (rhs->isconst()) {
            int c = rhs->imm32();
            
            
            Register r = findRegFor(lhs, GpRegs);
            if (c == 0 && cond->isop(LIR_eq)) {
                TEST(r, r);
            } else {
                CMPi(r, c);
            }

        } else {
            Register ra, rb;
            findRegFor2(GpRegs, lhs, ra, GpRegs, rhs, rb);
            CMP(ra, rb);
        }
    }

    void Assembler::asm_fcond(LInsp ins)
    {
        LOpcode opcode = ins->opcode();
        Register r = prepareResultReg(ins, AllowableFlagRegs);

        
        MOVZX8(r,r);

        if (_config.i386_sse2) {
            
            
            
            switch (opcode) {
            case LIR_feq:   SETNP(r);       break;
            case LIR_flt:
            case LIR_fgt:   SETA(r);        break;
            case LIR_fle:
            case LIR_fge:   SETAE(r);       break;
            default:        NanoAssert(0);  break;
            }
        } else {
            SETNP(r);
        }

        freeResourcesOf(ins);

        asm_fcmp(ins);
    }

    void Assembler::asm_cond(LInsp ins)
    {
        LOpcode op = ins->opcode();

        Register r = prepareResultReg(ins, AllowableFlagRegs);

        
        MOVZX8(r,r);
        switch (op) {
        case LIR_eq:    SETE(r);        break;
        case LIR_lt:    SETL(r);        break;
        case LIR_le:    SETLE(r);       break;
        case LIR_gt:    SETG(r);        break;
        case LIR_ge:    SETGE(r);       break;
        case LIR_ult:   SETB(r);        break;
        case LIR_ule:   SETBE(r);       break;
        case LIR_ugt:   SETA(r);        break;
        case LIR_uge:   SETAE(r);       break;
        default:        NanoAssert(0);  break;
        }

        freeResourcesOf(ins);

        asm_cmp(ins);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void Assembler::asm_arith(LInsp ins)
    {
        LOpcode op = ins->opcode();

        
        if (op == LIR_mod) {
            asm_div_mod(ins);
            return;
        }

        LInsp lhs = ins->oprnd1();
        LInsp rhs = ins->oprnd2();

        
        
        if ((op == LIR_add || op == LIR_iaddp) && lhs->isop(LIR_alloc) && rhs->isconst()) {
            
            Register rr = prepareResultReg(ins, GpRegs);
            int d = findMemFor(lhs) + rhs->imm32();

            LEA(rr, d, FP);

            freeResourcesOf(ins);

            return;
        }

        bool isConstRhs;
        RegisterMask allow = GpRegs;
        Register rb = UnspecifiedReg;

        switch (op) {
        case LIR_div:
            
            
            isConstRhs = false;
            rb = findRegFor(rhs, (GpRegs & ~(rmask(EAX)|rmask(EDX))));
            allow = rmask(EAX);
            evictIfActive(EDX);
            break;
        case LIR_mul:
        case LIR_mulxov:
            isConstRhs = false;
            if (lhs != rhs) {
                rb = findRegFor(rhs, allow);
                allow &= ~rmask(rb);
            }
            break;
        case LIR_lsh:
        case LIR_rsh:
        case LIR_ush:
            isConstRhs = rhs->isconst();
            if (!isConstRhs) {
                rb = findSpecificRegFor(rhs, ECX);
                allow &= ~rmask(rb);
            }
            break;
        default:
            isConstRhs = rhs->isconst();
            if (!isConstRhs && lhs != rhs) {
                rb = findRegFor(rhs, allow);
                allow &= ~rmask(rb);
            }
            break;
        }

        
        Register rr = prepareResultReg(ins, allow);

        
        Register ra = !lhs->isInReg() ? rr : lhs->getReg();

        if (!isConstRhs) {
            if (lhs == rhs)
                rb = ra;

            switch (op) {
            case LIR_add:
            case LIR_addp:
            case LIR_addxov:    ADD(rr, rb); break;     
            case LIR_sub:
            case LIR_subxov:    SUB(rr, rb); break;
            case LIR_mul:
            case LIR_mulxov:    MUL(rr, rb); break;
            case LIR_and:       AND(rr, rb); break;
            case LIR_or:        OR( rr, rb); break;
            case LIR_xor:       XOR(rr, rb); break;
            case LIR_lsh:       SHL(rr, rb); break;
            case LIR_rsh:       SAR(rr, rb); break;
            case LIR_ush:       SHR(rr, rb); break;
            case LIR_div:
                DIV(rb);
                CDQ(); 
                break;
            default:            NanoAssert(0);  break;
            }

        } else {
            int c = rhs->imm32();
            switch (op) {
            case LIR_addp:
            case LIR_add:
                
                LEA(rr, c, ra);
                ra = rr; 
                break;
            case LIR_addxov:    ADDi(rr, c);    break;
            case LIR_sub:
            case LIR_subxov:    SUBi(rr, c);    break;
            case LIR_and:       ANDi(rr, c);    break;
            case LIR_or:        ORi( rr, c);    break;
            case LIR_xor:       XORi(rr, c);    break;
            case LIR_lsh:       SHLi(rr, c);    break;
            case LIR_rsh:       SARi(rr, c);    break;
            case LIR_ush:       SHRi(rr, c);    break;
            default:            NanoAssert(0);  break;
            }
        }

        if (rr != ra)
            MR(rr, ra);

        freeResourcesOf(ins);
        if (!lhs->isInReg()) {
            NanoAssert(ra == rr);
            findSpecificRegForUnallocated(lhs, ra);
        }
    }

    
    void Assembler::asm_div_mod(LInsp mod)
    {
        LInsp div = mod->oprnd1();

        
        NanoAssert(mod->isop(LIR_mod));
        NanoAssert(div->isop(LIR_div));

        LInsp divL = div->oprnd1();
        LInsp divR = div->oprnd2();

        prepareResultReg(mod, rmask(EDX));
        prepareResultReg(div, rmask(EAX));

        Register rDivR = findRegFor(divR, (GpRegs & ~(rmask(EAX)|rmask(EDX))));
        Register rDivL = !divL->isInReg() ? EAX : divL->getReg();

        DIV(rDivR);
        CDQ();     
        if (EAX != rDivL)
            MR(EAX, rDivL);

        freeResourcesOf(mod);
        freeResourcesOf(div);
        if (!divL->isInReg()) {
            NanoAssert(rDivL == EAX);
            findSpecificRegForUnallocated(divL, EAX);
        }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void Assembler::asm_neg_not(LInsp ins)
    {
        LIns* lhs = ins->oprnd1();

        Register rr = prepareResultReg(ins, GpRegs);

        
        Register ra = !lhs->isInReg() ? rr : lhs->getReg();

        if (ins->isop(LIR_not)) {
            NOT(rr);
        } else {
            NanoAssert(ins->isop(LIR_neg));
            NEG(rr);
        }
        if (rr != ra)
            MR(rr, ra);

        freeResourcesOf(ins);
        if (!lhs->isInReg()) {
            NanoAssert(ra == rr);
            findSpecificRegForUnallocated(lhs, ra);
        }
    }

    void Assembler::asm_load32(LInsp ins)
    {
        LOpcode op = ins->opcode();
        LIns* base = ins->oprnd1();
        int32_t d = ins->disp();
        Register rr = deprecated_prepResultReg(ins, GpRegs);

        if (base->isconst()) {
            intptr_t addr = base->imm32();
            addr += d;
            switch(op) {
                case LIR_ldzb:
                case LIR_ldcb:
                    LD8Zdm(rr, addr);
                    return;
                case LIR_ldsb:
                case LIR_ldcsb:
                    LD8Sdm(rr, addr);
                    return;
                case LIR_ldzs:
                case LIR_ldcs:
                    LD16Zdm(rr, addr);
                    return;
                case LIR_ldss:
                case LIR_ldcss:
                    LD16Sdm(rr, addr);
                    return;
                case LIR_ld:
                case LIR_ldc:
                    LDdm(rr, addr);
                    return;
                default:
                    NanoAssertMsg(0, "asm_load32 should never receive this LIR opcode");
                    return;
            }
        }

        
        if (base->opcode() == LIR_piadd) {
            int scale = 0;
            LIns *lhs = base->oprnd1();
            LIns *rhs = base->oprnd2();

            


            if (rhs->opcode() == LIR_pilsh && rhs->oprnd2()->isconst()) {
                scale = rhs->oprnd2()->imm32();
                if (scale >= 1 && scale <= 3)
                    rhs = rhs->oprnd1();
                else
                    scale = 0;
            }

            


            Register rleft = ( !lhs->isInReg()
                             ? findSpecificRegForUnallocated(lhs, rr)
                             : lhs->getReg() );

            
            Register rright = ( rr != rleft && !rhs->isInReg()
                              ? findSpecificRegForUnallocated(rhs, rr)
                              : findRegFor(rhs, GpRegs & ~(rmask(rleft))) );

            switch(op) {
                case LIR_ldzb:
                case LIR_ldcb:
                    LD8Zsib(rr, d, rleft, rright, scale);
                    return;
                case LIR_ldsb:
                case LIR_ldcsb:
                    LD8Ssib(rr, d, rleft, rright, scale);
                    return;
                case LIR_ldzs:
                case LIR_ldcs:
                    LD16Zsib(rr, d, rleft, rright, scale);
                    return;
                case LIR_ldss:
                case LIR_ldcss:
                    LD16Ssib(rr, d, rleft, rright, scale);
                    return;
                case LIR_ld:
                case LIR_ldc:
                    LDsib(rr, d, rleft, rright, scale);
                    return;
                default:
                    NanoAssertMsg(0, "asm_load32 should never receive this LIR opcode");
                    return;
            }
        }

        Register ra = getBaseReg(base, d, GpRegs);
        switch(op) {
            case LIR_ldzb:
            case LIR_ldcb:
                LD8Z(rr, d, ra);
                return;
            case LIR_ldsb:
            case LIR_ldcsb:
                LD8S(rr, d, ra);
                return;
            case LIR_ldzs:
            case LIR_ldcs:
                LD16Z(rr, d, ra);
                return;
            case LIR_ldss:
            case LIR_ldcss:
                LD16S(rr, d, ra);
                return;
            case LIR_ld:
            case LIR_ldc:
                LD(rr, d, ra);
                return;
            default:
                NanoAssertMsg(0, "asm_load32 should never receive this LIR opcode");
                return;
        }
    }

    void Assembler::asm_cmov(LInsp ins)
    {
        LIns* condval = ins->oprnd1();
        LIns* iftrue  = ins->oprnd2();
        LIns* iffalse = ins->oprnd3();

        NanoAssert(condval->isCmp());
        NanoAssert(ins->isop(LIR_cmov) && iftrue->isI32() && iffalse->isI32());

        const Register rr = deprecated_prepResultReg(ins, GpRegs);

        
        
        const Register iffalsereg = findRegFor(iffalse, GpRegs & ~rmask(rr));
        switch (condval->opcode()) {
            
            case LIR_eq:    MRNE(rr, iffalsereg);   break;
            case LIR_lt:    MRGE(rr, iffalsereg);   break;
            case LIR_le:    MRG( rr, iffalsereg);   break;
            case LIR_gt:    MRLE(rr, iffalsereg);   break;
            case LIR_ge:    MRL( rr, iffalsereg);   break;
            case LIR_ult:   MRAE(rr, iffalsereg);   break;
            case LIR_ule:   MRA( rr, iffalsereg);   break;
            case LIR_ugt:   MRBE(rr, iffalsereg);   break;
            case LIR_uge:   MRB( rr, iffalsereg);   break;
            default: NanoAssert(0); break;
        }
         findSpecificRegFor(iftrue, rr);
        asm_cmp(condval);
    }

    void Assembler::asm_param(LInsp ins)
    {
        uint32_t a = ins->paramArg();
        uint32_t kind = ins->paramKind();
        if (kind == 0) {
            
            AbiKind abi = _thisfrag->lirbuf->abi;
            uint32_t abi_regcount = max_abi_regs[abi];
            if (a < abi_regcount) {
                
                deprecated_prepResultReg(ins, rmask(argRegs[a]));
            } else {
                
                Register r = deprecated_prepResultReg(ins, GpRegs);
                int d = (a - abi_regcount) * sizeof(intptr_t) + 8;
                LD(r, d, FP);
            }
        }
        else {
            
            deprecated_prepResultReg(ins, rmask(savedRegs[a]));
        }
    }

    void Assembler::asm_int(LInsp ins)
    {
        Register rr = prepareResultReg(ins, GpRegs);

        asm_int(rr, ins->imm32(), true);

        freeResourcesOf(ins);
    }

    void Assembler::asm_int(Register r, int32_t val, bool canClobberCCs)
    {
        if (val == 0 && canClobberCCs)
            XOR(r, r);
        else
            LDi(r, val);
    }

    void Assembler::asm_quad(Register r, uint64_t q, double d, bool canClobberCCs)
    {
        
        
        
        
        
        
        
        

        if (rmask(r) & XmmRegs) {
            if (q == 0) {
                
                SSE_XORPDr(r, r);
            } else if (d && d == (int)d && canClobberCCs) {
                
                Register tr = registerAllocTmp(GpRegs);
                SSE_CVTSI2SD(r, tr);
                SSE_XORPDr(r, r);   
                asm_int(tr, (int)d, canClobberCCs);
            } else {
                const uint64_t* p = findQuadConstant(q);
                LDSDm(r, (const double*)p);
            }
        } else {
            NanoAssert(r == FST0);
            if (q == 0) {
                
                FLDZ();
            } else if (d == 1.0) {
                FLD1();
            } else {
                const uint64_t* p = findQuadConstant(q);
                FLDQdm((const double*)p);
            }
        }
    }

    void Assembler::asm_quad(LInsp ins)
    {
        if (ins->isInReg()) {
            Register rr = ins->getReg();
            NanoAssert(rmask(rr) & FpRegs);
            asm_quad(rr, ins->imm64(), ins->imm64f(), true);
        }

        freeResourcesOf(ins);
    }

    
#if defined __SUNPRO_CC
    
    
    static uint32_t negateMask_temp[] = {0, 0, 0, 0, 0, 0, 0};

    static uint32_t* negateMaskInit()
    {
        uint32_t* negateMask = (uint32_t*)alignUp(negateMask_temp, 16);
        negateMask[1] = 0x80000000;
        return negateMask;
    }

    static uint32_t *negateMask = negateMaskInit();
#else
    static const AVMPLUS_ALIGN16(uint32_t) negateMask[] = {0,0x80000000,0,0};
#endif

    void Assembler::asm_fneg(LInsp ins)
    {
        LIns *lhs = ins->oprnd1();

        if (_config.i386_sse2) {
            Register rr = prepareResultReg(ins, XmmRegs);

            
            Register ra;
            if (!lhs->isInReg()) {
                ra = rr;
            } else if (!(rmask(lhs->getReg()) & XmmRegs)) {
                
                
                evict(lhs);
                ra = rr;
            } else {
                ra = lhs->getReg();
            }

            SSE_XORPD(rr, negateMask);

            if (rr != ra)
                SSE_MOVSD(rr, ra);

            freeResourcesOf(ins);
            if (!lhs->isInReg()) {
                NanoAssert(ra == rr);
                findSpecificRegForUnallocated(lhs, ra);
            }

        } else {
            verbose_only( Register rr = ) prepareResultReg(ins, x87Regs);
            NanoAssert(FST0 == rr);

            NanoAssert(!lhs->isInReg() || FST0 == lhs->getReg());

            FCHS();

            freeResourcesOf(ins);
            if (!lhs->isInReg())
                findSpecificRegForUnallocated(lhs, FST0);
        }
    }

    void Assembler::asm_arg(ArgSize sz, LInsp ins, Register r, int32_t& stkd)
    {
        
        

        if (sz == ARGSIZE_I || sz == ARGSIZE_U)
        {
            if (r != UnspecifiedReg) {
                if (ins->isconst()) {
                    
                    asm_int(r, ins->imm32(), true);
                } else if (ins->isInReg()) {
                    if (r != ins->getReg())
                        MR(r, ins->getReg());
                } else if (ins->isInAr()) {
                    int d = arDisp(ins);
                    NanoAssert(d != 0);
                    if (ins->isop(LIR_alloc)) {
                        LEA(r, d, FP);
                    } else {
                        LD(r, d, FP);
                    }

                } else {
                    
                    
                    findSpecificRegForUnallocated(ins, r);
                }
            }
            else {
                if (_config.i386_fixed_esp)
                    asm_stkarg(ins, stkd);
                else
                    asm_pusharg(ins);
            }
        }
        else
        {
            NanoAssert(sz == ARGSIZE_F);
            asm_farg(ins, stkd);
        }
    }

    void Assembler::asm_pusharg(LInsp ins)
    {
        
        if (!ins->isUsed() && ins->isconst())
        {
            PUSHi(ins->imm32());    
        }
        else if (!ins->isUsed() || ins->isop(LIR_alloc))
        {
            Register ra = findRegFor(ins, GpRegs);
            PUSHr(ra);
        }
        else if (ins->isInReg())
        {
            PUSHr(ins->getReg());
        }
        else
        {
            NanoAssert(ins->isInAr());
            PUSHm(arDisp(ins), FP);
        }
    }

    void Assembler::asm_stkarg(LInsp ins, int32_t& stkd)
    {
        
        if (!ins->isUsed() && ins->isconst())
        {
            
            STi(SP, stkd, ins->imm32());
        }
        else {
            Register ra;
            if (!ins->isInReg() || ins->isop(LIR_alloc))
                ra = findRegFor(ins, GpRegs & (~SavedRegs));
            else
                ra = ins->getReg();
            ST(SP, stkd, ra);
        }

        stkd += sizeof(int32_t);
    }

    void Assembler::asm_farg(LInsp ins, int32_t& stkd)
    {
        NanoAssert(ins->isF64());
        Register r = findRegFor(ins, FpRegs);
        if (rmask(r) & XmmRegs) {
            SSE_STQ(stkd, SP, r);
        } else {
            FSTPQ(stkd, SP);

            
            
            
            
            
            
            

            



            evictIfActive(FST0);
        }
        if (!_config.i386_fixed_esp)
            SUBi(ESP, 8);

        stkd += sizeof(double);
    }

    void Assembler::asm_fop(LInsp ins)
    {
        LOpcode op = ins->opcode();
        if (_config.i386_sse2)
        {
            LIns *lhs = ins->oprnd1();
            LIns *rhs = ins->oprnd2();

            RegisterMask allow = XmmRegs;
            Register rb = UnspecifiedReg;
            if (lhs != rhs) {
                rb = findRegFor(rhs,allow);
                allow &= ~rmask(rb);
            }

            Register rr = deprecated_prepResultReg(ins, allow);
            Register ra;

            
            if (!lhs->isInReg()) {
                ra = findSpecificRegForUnallocated(lhs, rr);
            } else if ((rmask(lhs->getReg()) & XmmRegs) == 0) {
                
                
                
                ra = findRegFor(lhs, XmmRegs);
            } else {
                
                ra = findRegFor(lhs, allow);
            }

            if (lhs == rhs)
                rb = ra;

            if (op == LIR_fadd)
                SSE_ADDSD(rr, rb);
            else if (op == LIR_fsub)
                SSE_SUBSD(rr, rb);
            else if (op == LIR_fmul)
                SSE_MULSD(rr, rb);
            else 
                SSE_DIVSD(rr, rb);

            if (rr != ra)
                SSE_MOVSD(rr, ra);
        }
        else
        {
            
            
            LIns* rhs = ins->oprnd1();
            LIns* lhs = ins->oprnd2();
            Register rr = deprecated_prepResultReg(ins, rmask(FST0));

            if (rhs->isconstq())
            {
                const uint64_t* p = findQuadConstant(rhs->imm64());

                

                
                
                if (!lhs->isInReg())
                    findSpecificRegForUnallocated(lhs, rr);

                NanoAssert(lhs->getReg()==FST0);
                
                if (op == LIR_fadd)
                    { FADDdm((const double*)p); }
                else if (op == LIR_fsub)
                    { FSUBRdm((const double*)p); }
                else if (op == LIR_fmul)
                    { FMULdm((const double*)p); }
                else if (op == LIR_fdiv)
                    { FDIVRdm((const double*)p); }
            }
            else
            {
                
                int db = findMemFor(rhs);

                

                
                
                if (!lhs->isInReg())
                    findSpecificRegForUnallocated(lhs, rr);

                NanoAssert(lhs->getReg()==FST0);
                
                if (op == LIR_fadd)
                    { FADD(db, FP); }
                else if (op == LIR_fsub)
                    { FSUBR(db, FP); }
                else if (op == LIR_fmul)
                    { FMUL(db, FP); }
                else if (op == LIR_fdiv)
                    { FDIVR(db, FP); }
            }
        }
    }

    void Assembler::asm_i2f(LInsp ins)
    {
        LIns* lhs = ins->oprnd1();

        Register rr = prepareResultReg(ins, FpRegs);
        if (rmask(rr) & XmmRegs) {
            
            Register ra = findRegFor(lhs, GpRegs);
            SSE_CVTSI2SD(rr, ra);
            SSE_XORPDr(rr, rr);     
        } else {
            int d = findMemFor(lhs);
            FILD(d, FP);
        }

        freeResourcesOf(ins);
    }

    void Assembler::asm_u2f(LInsp ins)
    {
        LIns* lhs = ins->oprnd1();

        Register rr = prepareResultReg(ins, FpRegs);
        if (rmask(rr) & XmmRegs) {
            Register rt = registerAllocTmp(GpRegs);

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

            static const double k_NEGONE = 2147483648.0;
            SSE_ADDSDm(rr, &k_NEGONE);

            SSE_CVTSI2SD(rr, rt);
            SSE_XORPDr(rr,rr);  

            if (lhs->isInRegMask(GpRegs)) {
                Register ra = lhs->getReg();
                LEA(rt, 0x80000000, ra);

            } else {
                const int d = findMemFor(lhs);
                SUBi(rt, 0x80000000);
                LD(rt, d, FP);
            }

        } else {
            const int disp = -8;
            const Register base = SP;
            Register ra = findRegFor(lhs, GpRegs);
            NanoAssert(rr == FST0);
            FILDQ(disp, base);
            STi(base, disp+4, 0);   
            ST(base, disp, ra);     
        }

        freeResourcesOf(ins);
    }

    void Assembler::asm_f2i(LInsp ins)
    {
        LIns *lhs = ins->oprnd1();

        if (_config.i386_sse2) {
            Register rr = prepareResultReg(ins, GpRegs);
            Register ra = findRegFor(lhs, XmmRegs);
            SSE_CVTSD2SI(rr, ra);
        } else {
            int pop = !lhs->isInReg();
            findSpecificRegFor(lhs, FST0);
            if (ins->isInReg())
                evict(ins);
            int d = findMemFor(ins);
            FIST((pop?1:0), d, FP);
        }

        freeResourcesOf(ins);
    }

    void Assembler::asm_nongp_copy(Register rd, Register rs)
    {
        if ((rmask(rd) & XmmRegs) && (rmask(rs) & XmmRegs)) {
            
            SSE_MOVSD(rd, rs);
        } else if ((rmask(rd) & GpRegs) && (rmask(rs) & XmmRegs)) {
            
            SSE_MOVD(rd, rs);
        } else {
            NanoAssertMsgf(false, "bad asm_nongp_copy(%s, %s)", gpn(rd), gpn(rs));
        }
    }

    NIns* Assembler::asm_fbranch(bool branchOnFalse, LIns *cond, NIns *targ)
    {
        NIns* at;
        LOpcode opcode = cond->opcode();

        if (_config.i386_sse2) {
            
            
            
            if (branchOnFalse) {
                
                switch (opcode) {
                case LIR_feq:   JP(targ);       break;
                case LIR_flt:
                case LIR_fgt:   JNA(targ);      break;
                case LIR_fle:
                case LIR_fge:   JNAE(targ);     break;
                default:        NanoAssert(0);  break;
                }
            } else {
                
                switch (opcode) {
                case LIR_feq:   JNP(targ);      break;
                case LIR_flt:
                case LIR_fgt:   JA(targ);       break;
                case LIR_fle:
                case LIR_fge:   JAE(targ);      break;
                default:        NanoAssert(0);  break;
                }
            }
        } else {
            if (branchOnFalse)
                JP(targ);
            else
                JNP(targ);
        }

        at = _nIns;
        asm_fcmp(cond);

        return at;
    }

    
    
    
    void Assembler::asm_fcmp(LIns *cond)
    {
        LOpcode condop = cond->opcode();
        NanoAssert(condop >= LIR_feq && condop <= LIR_fge);
        LIns* lhs = cond->oprnd1();
        LIns* rhs = cond->oprnd2();
        NanoAssert(lhs->isF64() && rhs->isF64());

        if (_config.i386_sse2) {
            
            if (condop == LIR_flt) {
                condop = LIR_fgt;
                LIns* t = lhs; lhs = rhs; rhs = t;
            } else if (condop == LIR_fle) {
                condop = LIR_fge;
                LIns* t = lhs; lhs = rhs; rhs = t;
            }

            if (condop == LIR_feq) {
                if (lhs == rhs) {
                    

                    
                    
                    
                    

                    Register r = findRegFor(lhs, XmmRegs);
                    SSE_UCOMISD(r, r);
                } else {
                    
                    
                    
                    int mask = 0x44;

                    
                    
                    
                    
                    
                    

                    evictIfActive(EAX);
                    Register ra, rb;
                    findRegFor2(XmmRegs, lhs, ra, XmmRegs, rhs, rb);

                    TEST_AH(mask);
                    LAHF();
                    SSE_UCOMISD(ra, rb);
                }
            } else {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                

                Register ra, rb;
                findRegFor2(XmmRegs, lhs, ra, XmmRegs, rhs, rb);
                SSE_UCOMISD(ra, rb);
            }

        } else {
            
            
            if (condop == LIR_fgt) {
                condop = LIR_flt;
                LIns* t = lhs; lhs = rhs; rhs = t;
            } else if (condop == LIR_fge) {
                condop = LIR_fle;
                LIns* t = lhs; lhs = rhs; rhs = t;
            }

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

            int mask = 0;   
            switch (condop) {
            case LIR_feq:   mask = 0x44;    break;
            case LIR_flt:   mask = 0x05;    break;
            case LIR_fle:   mask = 0x41;    break;
            default:        NanoAssert(0);  break;
            }

            evictIfActive(EAX);
            int pop = !lhs->isInReg();
            findSpecificRegFor(lhs, FST0);

            if (lhs == rhs) {
                
                TEST_AH(mask);
                FNSTSW_AX();        
                if (pop)
                    FCOMPP();
                else
                    FCOMP();
                FLDr(FST0); 
            } else {
                TEST_AH(mask);
                FNSTSW_AX();        
                if (rhs->isconstq())
                {
                    const uint64_t* p = findQuadConstant(rhs->imm64());
                    FCOMdm((pop?1:0), (const double*)p);
                }
                else
                {
                    int d = findMemFor(rhs);
                    FCOM((pop?1:0), d, FP);
                }
            }
        }
    }

    
    
    verbose_only(
    void Assembler::asm_inc_m32(uint32_t* pCtr)
    {
       INCLi(pCtr);
    }
    )

    void Assembler::nativePageReset()
    {}

    void Assembler::nativePageSetup()
    {
        NanoAssert(!_inExit);
        if (!_nIns)
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
    }

    
    void Assembler::underrunProtect(int n)
    {
        NIns *eip = _nIns;
        NanoAssertMsg(n<=LARGEST_UNDERRUN_PROT, "constant LARGEST_UNDERRUN_PROT is too small");
        
        if (eip - n < codeStart) {
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
            JMP(eip);
        }
    }

    void Assembler::asm_ret(LInsp ins)
    {
        genEpilogue();

        
        MR(SP,FP);

        releaseRegisters();
        assignSavedRegs();

        LIns *val = ins->oprnd1();
        if (ins->isop(LIR_ret)) {
            findSpecificRegFor(val, retRegs[0]);
        } else {
            NanoAssert(ins->isop(LIR_fret));
            findSpecificRegFor(val, FST0);
            fpu_pop();
        }
    }

    void Assembler::swapCodeChunks() {
        if (!_nExitIns)
            codeAlloc(exitStart, exitEnd, _nExitIns verbose_only(, exitBytes));
        SWAP(NIns*, _nIns, _nExitIns);
        SWAP(NIns*, codeStart, exitStart);
        SWAP(NIns*, codeEnd, exitEnd);
        verbose_only( SWAP(size_t, codeBytes, exitBytes); )
    }

    #endif
}
