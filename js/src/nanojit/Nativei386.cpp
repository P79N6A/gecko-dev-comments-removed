







































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


    void Assembler::nInit(AvmCore* core)
    {
        (void) core;
        VMPI_getDate();
    }

    void Assembler::nBeginAssembly() {
        max_stk_args = 0;
    }

    NIns* Assembler::genPrologue()
    {
        


        uint32_t stackNeeded = max_stk_args + STACK_GRANULARITY * _activation.tos;

        uint32_t stackPushed =
            STACK_GRANULARITY + 
            STACK_GRANULARITY; 

        uint32_t aligned = alignUp(stackNeeded + stackPushed, NJ_ALIGN_STACK);
        uint32_t amt = aligned - stackPushed;

        
        
        if (amt)
        {
            SUBi(SP, amt);
        }

        verbose_only( outputAddr=true; asm_output("[frag entry]"); )
        NIns *fragEntry = _nIns;
        MR(FP, SP); 
        PUSHr(FP); 

        return fragEntry;
    }

    void Assembler::nFragExit(LInsp guard)
    {
        SideExit *exit = guard->record()->exit;
        bool trees = config.tree_opt;
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
            
            if (destKnown && !trees) {
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

        
        LDi(EAX, int(lr));
    }

    NIns *Assembler::genEpilogue()
    {
        RET();
        POPr(FP); 

        return  _nIns;
    }

    void Assembler::asm_call(LInsp ins)
    {
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
            if (config.fixed_esp) {
                
                
                
                
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
            if (!config.fixed_esp) 
                stkd = 0;
        }

        for(uint32_t i=0; i < argc; i++)
        {
            uint32_t j = argc-i-1;
            ArgSize sz = sizes[j];
            Register r = UnknownReg;
            if (n < max_regs && sz != ARGSIZE_F) {
                r = argRegs[n++]; 
            }
            asm_arg(sz, ins->arg(j), r, stkd);
            if (!config.fixed_esp) 
                stkd = 0;
        }

        if (config.fixed_esp) {
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
    #ifdef WIN32
        _asm
        {
            mov ecx, regs
            bsf eax, set                    
            btr RegAlloc::free[ecx], eax    
            mov r, eax
        }
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
        if (!config.sse2)
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

    RegisterMask Assembler::hint(LIns* i, RegisterMask allow)
    {
        uint32_t op = i->opcode();
        int prefer = allow;
        if (op == LIR_icall) {
            prefer &= rmask(retRegs[0]);
        }
        else if (op == LIR_fcall) {
            prefer &= rmask(FST0);
        }
        else if (op == LIR_param) {
            if (i->paramKind() == 0) {
                uint32_t max_regs = max_abi_regs[_thisfrag->lirbuf->abi];
                if (i->paramArg() < max_regs)
                    prefer &= rmask(argRegs[i->paramArg()]);
            } else {
                if (i->paramArg() < NumSavedRegs)
                    prefer &= rmask(savedRegs[i->paramArg()]);
            }
        }
        else if (op == LIR_callh || (op == LIR_rsh && i->oprnd1()->opcode()==LIR_callh)) {
            prefer &= rmask(retRegs[1]);
        }
        else if (i->isCmp()) {
            prefer &= AllowableFlagRegs;
        }
        else if (i->isconst()) {
            prefer &= ScratchRegs;
        }
        return (_allocator.free & prefer) ? prefer : allow;
    }

    void Assembler::asm_qjoin(LIns *ins)
    {
        int d = findMemFor(ins);
        AvmAssert(d);
        LIns* lo = ins->oprnd1();
        LIns* hi = ins->oprnd2();

        Register rr = ins->getReg();
        if (isKnownReg(rr) && (rmask(rr) & FpRegs))
            evict(rr, ins);

        if (hi->isconst())
        {
            STi(FP, d+4, hi->imm32());
        }
        else
        {
            Register r = findRegFor(hi, GpRegs);
            ST(FP, d+4, r);
        }

        if (lo->isconst())
        {
            STi(FP, d, lo->imm32());
        }
        else
        {
            
            Register r = findRegFor(lo, GpRegs);
            ST(FP, d, r);
        }

        freeRsrcOf(ins, false); 
    }

    void Assembler::asm_load(int d, Register r)
    {
        if (rmask(r) & FpRegs)
        {
            if (rmask(r) & XmmRegs) {
                SSE_LDQ(r, d, FP);
            } else {
                FLDQ(d, FP);
            }
        }
        else
        {
            LD(r, d, FP);
        }
    }

    
    
    void Assembler::asm_restore(LInsp i, Reservation* , Register r)
    {
        uint32_t arg;
        uint32_t abi_regcount;
        if (i->isop(LIR_alloc)) {
            verbose_only( if (_logc->lcbits & LC_RegAlloc) {
                            outputForEOL("  <= remat %s size %d",
                            _thisfrag->lirbuf->names->formatRef(i), i->size()); } )
            LEA(r, disp(i), FP);
        }
        else if (i->isconst()) {
            if (!i->getArIndex()) {
                i->markAsClear();
            }
            LDi(r, i->imm32());
        }
        else if (i->isop(LIR_param) && i->paramKind() == 0 &&
            (arg = i->paramArg()) >= (abi_regcount = max_abi_regs[_thisfrag->lirbuf->abi])) {
            
            if (!i->getArIndex()) {
                i->markAsClear();
            }
            
            
            
            
            
            
            int d = (arg - abi_regcount) * sizeof(intptr_t) + 8;
            LD(r, d, FP);
        }
        else {
            int d = findMemFor(i);
            verbose_only( if (_logc->lcbits & LC_RegAlloc) {
                            outputForEOL("  <= restore %s",
                            _thisfrag->lirbuf->names->formatRef(i)); } )
            asm_load(d,r);
        }
    }

    void Assembler::asm_store32(LIns *value, int dr, LIns *base)
    {
        if (value->isconst())
        {
            Register rb = getBaseReg(LIR_sti, base, dr, GpRegs);
            int c = value->imm32();
            STi(rb, dr, c);
        }
        else
        {
            
            Register ra, rb;
            if (base->isop(LIR_alloc)) {
                rb = FP;
                dr += findMemFor(base);
                ra = findRegFor(value, GpRegs);
            } else if (base->isconst()) {
                
                dr += base->imm32();
                ra = findRegFor(value, GpRegs);
                rb = UnknownReg;
            } else {
                findRegFor2b(GpRegs, value, ra, base, rb);
            }
            ST(rb, dr, ra);
        }
    }

    void Assembler::asm_spill(Register rr, int d, bool pop, bool quad)
    {
        (void)quad;
        if (d)
        {
            
            if (rmask(rr) & FpRegs)
            {
                if (rmask(rr) & XmmRegs) {
                    SSE_STQ(d, FP, rr);
                } else {
                    FSTQ((pop?1:0), d, FP);
                }
            }
            else
            {
                ST(FP, d, rr);
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
        Register rr = ins->getReg();

        if (isKnownReg(rr) && rmask(rr) & XmmRegs)
        {
            freeRsrcOf(ins, false);
            Register rb = getBaseReg(ins->opcode(), base, db, GpRegs);
            SSE_LDQ(rr, db, rb);
        }
        else
        {
            int dr = disp(ins);
            Register rb;
            if (base->isop(LIR_alloc)) {
                rb = FP;
                db += findMemFor(base);
            } else {
                rb = findRegFor(base, GpRegs);
            }
            ins->setReg(UnknownReg);

            
            if (dr)
                asm_mmq(FP, dr, rb, db);

            freeRsrcOf(ins, false);

            if (isKnownReg(rr))
            {
                NanoAssert(rmask(rr)&FpRegs);
                _allocator.retire(rr);
                FLDQ(db, rb);
            }
        }
    }

    void Assembler::asm_store64(LInsp value, int dr, LInsp base)
    {
        if (value->isconstq())
        {
            
            
            Register rb;
            if (base->isop(LIR_alloc)) {
                rb = FP;
                dr += findMemFor(base);
            } else {
                rb = findRegFor(base, GpRegs);
            }
            STi(rb, dr+4, value->imm64_1());
            STi(rb, dr,   value->imm64_0());
            return;
        }

        if (value->isop(LIR_ldq) || value->isop(LIR_ldqc) || value->isop(LIR_qjoin))
        {
            
            
            

            
            
            
            

            if (config.sse2) {
                Register rv = findRegFor(value, XmmRegs);
                Register rb;
                if (base->isop(LIR_alloc)) {
                    rb = FP;
                    dr += findMemFor(base);
                } else {
                    rb = findRegFor(base, GpRegs);
                }
                SSE_STQ(dr, rb, rv);
                return;
            }

            int da = findMemFor(value);
            Register rb;
            if (base->isop(LIR_alloc)) {
                rb = FP;
                dr += findMemFor(base);
            } else {
                rb = findRegFor(base, GpRegs);
            }
            asm_mmq(rb, dr, FP, da);
            return;
        }

        Register rb;
        if (base->isop(LIR_alloc)) {
            rb = FP;
            dr += findMemFor(base);
        } else {
            rb = findRegFor(base, GpRegs);
        }

        
        
        bool pop = value->isUnusedOrHasUnknownReg();
        Register rv = ( pop
                      ? findRegFor(value, config.sse2 ? XmmRegs : FpRegs)
                      : value->getReg() );

        if (rmask(rv) & XmmRegs) {
            SSE_STQ(dr, rb, rv);
        } else {
            FSTQ(pop?1:0, dr, rb);
        }
    }

    


    void Assembler::asm_mmq(Register rd, int dd, Register rs, int ds)
    {
        
        
        
        if (config.sse2)
        {
            
            Register t = registerAllocTmp(XmmRegs);
            SSE_STQ(dd, rd, t);
            SSE_LDQ(t, ds, rs);
        }
        else
        {
            
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
        NanoAssert(cond->isCond());

        
        if (condop >= LIR_feq && condop <= LIR_fge) {
            return asm_fbranch(branchOnFalse, cond, targ);
        }

        if (branchOnFalse) {
            
            switch (condop) {
            case LIR_ov:    JNO(targ);      break;
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
            case LIR_ov:    JO(targ);       break;
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
        LOpcode condop = cond->opcode();

        
        if (condop == LIR_ov)
            return;

        LInsp lhs = cond->oprnd1();
        LInsp rhs = cond->oprnd2();

        NanoAssert((!lhs->isQuad() && !rhs->isQuad()) || (lhs->isQuad() && rhs->isQuad()));

        
        NanoAssert(!lhs->isQuad() && !rhs->isQuad());

        
        if (rhs->isconst())
        {
            int c = rhs->imm32();
            if (c == 0 && cond->isop(LIR_eq)) {
                Register r = findRegFor(lhs, GpRegs);
                TEST(r,r);
            } else if (!rhs->isQuad()) {
                Register r = getBaseReg(condop, lhs, c, GpRegs);
                CMPi(r, c);
            }
        }
        else
        {
            Register ra, rb;
            findRegFor2b(GpRegs, lhs, ra, rhs, rb);
            CMP(ra, rb);
        }
    }

    void Assembler::asm_fcond(LInsp ins)
    {
        LOpcode opcode = ins->opcode();

        
        Register r = prepResultReg(ins, AllowableFlagRegs);

        
        MOVZX8(r,r);

        if (config.sse2) {
            
            
            
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
        asm_fcmp(ins);
    }

    void Assembler::asm_cond(LInsp ins)
    {
        
        LOpcode op = ins->opcode();
        Register r = prepResultReg(ins, AllowableFlagRegs);
        
        MOVZX8(r,r);
        switch (op) {
        case LIR_ov:    SETO(r);        break;
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
        asm_cmp(ins);
    }

    void Assembler::asm_arith(LInsp ins)
    {
        LOpcode op = ins->opcode();
        LInsp lhs = ins->oprnd1();

        if (op == LIR_mod) {
            asm_div_mod(ins);
            return;
        }

        LInsp rhs = ins->oprnd2();

        bool forceReg;
        RegisterMask allow = GpRegs;
        Register rb = UnknownReg;

        switch (op) {
        case LIR_div:
            
            
            forceReg = true;
            rb = findRegFor(rhs, (GpRegs ^ (rmask(EAX)|rmask(EDX))));
            allow = rmask(EAX);
            evictIfActive(EDX);
            break;
        case LIR_mul:
            forceReg = true;
            break;
        case LIR_lsh:
        case LIR_rsh:
        case LIR_ush:
            forceReg = !rhs->isconst();
            if (forceReg) {
                rb = findSpecificRegFor(rhs, ECX);
                allow &= ~rmask(rb);
            }
            break;
        case LIR_add:
        case LIR_addp:
            if (lhs->isop(LIR_alloc) && rhs->isconst()) {
                
                Register rr = prepResultReg(ins, allow);
                int d = findMemFor(lhs) + rhs->imm32();
                LEA(rr, d, FP);
                return;
            }
            
        default:
            forceReg = !rhs->isconst();
            break;
        }

        
        if (forceReg && lhs != rhs && !isKnownReg(rb)) {
            rb = findRegFor(rhs, allow);
            allow &= ~rmask(rb);
        }

        Register rr = prepResultReg(ins, allow);
        
        
        Register ra = ( lhs->isUnusedOrHasUnknownReg()
                      ? findSpecificRegForUnallocated(lhs, rr)
                      : lhs->getReg() );

        if (forceReg)
        {
            if (lhs == rhs)
                rb = ra;

            switch (op) {
            case LIR_add:
            case LIR_addp:
                ADD(rr, rb);
                break;
            case LIR_sub:
                SUB(rr, rb);
                break;
            case LIR_mul:
                MUL(rr, rb);
                break;
            case LIR_and:
                AND(rr, rb);
                break;
            case LIR_or:
                OR(rr, rb);
                break;
            case LIR_xor:
                XOR(rr, rb);
                break;
            case LIR_lsh:
                SHL(rr, rb);
                break;
            case LIR_rsh:
                SAR(rr, rb);
                break;
            case LIR_ush:
                SHR(rr, rb);
                break;
            case LIR_div:
                DIV(rb);
                CDQ();
                break;
            default:
                NanoAssertMsg(0, "Unsupported");
            }
        }
        else
        {
            int c = rhs->imm32();
            switch (op) {
            case LIR_addp:
                
                LEA(rr, c, ra);
                ra = rr; 
                break;
            case LIR_add:
                ADDi(rr, c);
                break;
            case LIR_sub:
                SUBi(rr, c);
                break;
            case LIR_and:
                ANDi(rr, c);
                break;
            case LIR_or:
                ORi(rr, c);
                break;
            case LIR_xor:
                XORi(rr, c);
                break;
            case LIR_lsh:
                SHLi(rr, c);
                break;
            case LIR_rsh:
                SARi(rr, c);
                break;
            case LIR_ush:
                SHRi(rr, c);
                break;
            default:
                NanoAssertMsg(0, "Unsupported");
                break;
            }
        }

        if ( rr != ra )
            MR(rr,ra);
    }

    
    void Assembler::asm_div_mod(LInsp mod)
    {
        LInsp div = mod->oprnd1();

        

        NanoAssert(mod->isop(LIR_mod));
        NanoAssert(div->isop(LIR_div));

        LInsp divLhs = div->oprnd1();
        LInsp divRhs = div->oprnd2();

        prepResultReg(mod, rmask(EDX));
        prepResultReg(div, rmask(EAX));

        Register rDivRhs = findRegFor(divRhs, (GpRegs ^ (rmask(EAX)|rmask(EDX))));

        Register rDivLhs = ( divLhs->isUnusedOrHasUnknownReg()
                           ? findSpecificRegFor(divLhs, EAX)
                           : divLhs->getReg() );

        DIV(rDivRhs);
        CDQ();     

        if ( EAX != rDivLhs )
            MR(EAX, rDivLhs);
    }

    void Assembler::asm_neg_not(LInsp ins)
    {
        LOpcode op = ins->opcode();
        Register rr = prepResultReg(ins, GpRegs);

        LIns* lhs = ins->oprnd1();
        
        
        Register ra = ( lhs->isUnusedOrHasUnknownReg()
                      ? findSpecificRegForUnallocated(lhs, rr)
                      : lhs->getReg() );

        if (op == LIR_not)
            NOT(rr);
        else
            NEG(rr);

        if ( rr != ra )
            MR(rr,ra);
    }

    void Assembler::asm_ld(LInsp ins)
    {
        LOpcode op = ins->opcode();
        LIns* base = ins->oprnd1();
        int32_t d = ins->disp();
        Register rr = prepResultReg(ins, GpRegs);

        if (base->isconst()) {
            intptr_t addr = base->imm32();
            addr += d;
            if (op == LIR_ldcb)
                LD8Zdm(rr, addr);
            else if (op == LIR_ldcs)
                LD16Zdm(rr, addr);
            else
                LDdm(rr, addr);
            return;
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

            


            Register rleft = ( lhs->isUnusedOrHasUnknownReg()
                             ? findSpecificRegForUnallocated(lhs, rr)
                             : lhs->getReg() );

            
            Register rright = ( rr != rleft && rhs->isUnusedOrHasUnknownReg()
                              ? findSpecificRegForUnallocated(rhs, rr)
                              : findRegFor(rhs, GpRegs & ~(rmask(rleft))) );

            if (op == LIR_ldcb)
                LD8Zsib(rr, d, rleft, rright, scale);
            else if (op == LIR_ldcs)
                LD16Zsib(rr, d, rleft, rright, scale);
            else
                LDsib(rr, d, rleft, rright, scale);

            return;
        }

        Register ra = getBaseReg(op, base, d, GpRegs);
        if (op == LIR_ldcb)
            LD8Z(rr, d, ra);
        else if (op == LIR_ldcs)
            LD16Z(rr, d, ra);
        else
            LD(rr, d, ra);
    }

    void Assembler::asm_cmov(LInsp ins)
    {
        LOpcode op = ins->opcode();
        LIns* condval = ins->oprnd1();
        LIns* iftrue  = ins->oprnd2();
        LIns* iffalse = ins->oprnd3();

        NanoAssert(condval->isCmp());
        NanoAssert(op == LIR_qcmov || (!iftrue->isQuad() && !iffalse->isQuad()));

        const Register rr = prepResultReg(ins, GpRegs);

        
        
        const Register iffalsereg = findRegFor(iffalse, GpRegs & ~rmask(rr));
        if (op == LIR_cmov) {
            switch (condval->opcode())
            {
                
                case LIR_eq:    MRNE(rr, iffalsereg);   break;
                case LIR_ov:    MRNO(rr, iffalsereg);   break;
                case LIR_lt:    MRGE(rr, iffalsereg);   break;
                case LIR_le:    MRG(rr, iffalsereg);    break;
                case LIR_gt:    MRLE(rr, iffalsereg);   break;
                case LIR_ge:    MRL(rr, iffalsereg);    break;
                case LIR_ult:   MRAE(rr, iffalsereg);   break;
                case LIR_ule:   MRA(rr, iffalsereg);    break;
                case LIR_ugt:   MRBE(rr, iffalsereg);   break;
                case LIR_uge:   MRB(rr, iffalsereg);    break;
                default: NanoAssert(0); break;
            }
        } else if (op == LIR_qcmov) {
            NanoAssert(0);
        }
         findSpecificRegFor(iftrue, rr);
        asm_cmp(condval);
    }

    void Assembler::asm_qhi(LInsp ins)
    {
        Register rr = prepResultReg(ins, GpRegs);
        LIns *q = ins->oprnd1();
        int d = findMemFor(q);
        LD(rr, d+4, FP);
    }

    void Assembler::asm_param(LInsp ins)
    {
        uint32_t a = ins->paramArg();
        uint32_t kind = ins->paramKind();
        if (kind == 0) {
            
            AbiKind abi = _thisfrag->lirbuf->abi;
            uint32_t abi_regcount = max_abi_regs[abi];
            if (a < abi_regcount) {
                
                prepResultReg(ins, rmask(argRegs[a]));
            } else {
                
                Register r = prepResultReg(ins, GpRegs);
                int d = (a - abi_regcount) * sizeof(intptr_t) + 8;
                LD(r, d, FP);
            }
        }
        else {
            
            prepResultReg(ins, rmask(savedRegs[a]));
        }
    }

    void Assembler::asm_int(LInsp ins)
    {
        Register rr = prepResultReg(ins, GpRegs);
        int32_t val = ins->imm32();
        if (val == 0)
            XOR(rr,rr);
        else
            LDi(rr, val);
    }

    void Assembler::asm_quad(LInsp ins)
    {
        Register rr = ins->getReg();
        if (isKnownReg(rr))
        {
            
            _allocator.retire(rr);
            ins->setReg(UnknownReg);
            NanoAssert((rmask(rr) & FpRegs) != 0);

            const double d = ins->imm64f();
            const uint64_t q = ins->imm64();
            if (rmask(rr) & XmmRegs) {
                if (q == 0.0) {
                    
                    SSE_XORPDr(rr, rr);
                } else if (d == 1.0) {
                    
                    static const double k_ONE = 1.0;
                    LDSDm(rr, &k_ONE);
                } else if (d && d == (int)d) {
                    
                    Register gr = registerAllocTmp(GpRegs);
                    SSE_CVTSI2SD(rr, gr);
                    SSE_XORPDr(rr,rr);  
                    LDi(gr, (int)d);
                } else {
                    findMemFor(ins);
                    const int d = disp(ins);
                    SSE_LDQ(rr, d, FP);
                }
            } else {
                if (q == 0.0) {
                    
                    FLDZ();
                } else if (d == 1.0) {
                    FLD1();
                } else {
                    findMemFor(ins);
                    int d = disp(ins);
                    FLDQ(d,FP);
                }
            }
        }

        
        int d = disp(ins);
        freeRsrcOf(ins, false);
        if (d)
        {
            STi(FP,d+4,ins->imm64_1());
            STi(FP,d,  ins->imm64_0());
        }
    }

    void Assembler::asm_qlo(LInsp ins)
    {
        LIns *q = ins->oprnd1();

        if (!config.sse2)
        {
            Register rr = prepResultReg(ins, GpRegs);
            int d = findMemFor(q);
            LD(rr, d, FP);
        }
        else
        {
            Register rr = ins->getReg();
            if (!isKnownReg(rr)) {
                
                int d = disp(ins);
                freeRsrcOf(ins, false);
                Register qr = findRegFor(q, XmmRegs);
                SSE_MOVDm(d, FP, qr);
            } else {
                freeRsrcOf(ins, false);
                Register qr = findRegFor(q, XmmRegs);
                SSE_MOVD(rr,qr);
            }
        }
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
        if (config.sse2)
        {
            LIns *lhs = ins->oprnd1();

            Register rr = prepResultReg(ins, XmmRegs);
            Register ra;

            
            
            if (lhs->isUnusedOrHasUnknownReg()) {
                ra = findSpecificRegForUnallocated(lhs, rr);
            } else {
                ra = lhs->getReg();
                if ((rmask(ra) & XmmRegs) == 0) {
                    



                    ra = findRegFor(lhs, XmmRegs);
                }
            }

            SSE_XORPD(rr, negateMask);

            if (rr != ra)
                SSE_MOVSD(rr, ra);
        }
        else
        {
            Register rr = prepResultReg(ins, FpRegs);

            LIns* lhs = ins->oprnd1();

            

            
            
            if (lhs->isUnusedOrHasUnknownReg())
                findSpecificRegForUnallocated(lhs, rr);

            NanoAssert(lhs->getReg()==FST0);
            
            FCHS();

            
            
        }
    }

    void Assembler::asm_arg(ArgSize sz, LInsp p, Register r, int32_t& stkd)
    {
        if (sz == ARGSIZE_Q)
        {
            
            if (isKnownReg(r))
            {
                
                int da = findMemFor(p);
                LEA(r, da, FP);
            }
            else
            {
                NanoAssert(0); 
            }
        }
        else if (sz == ARGSIZE_I || sz == ARGSIZE_U)
        {
            if (isKnownReg(r)) {
                
                if (p->isconst()) {
                    LDi(r, p->imm32());
                } else {
                    if (p->isUsed()) {
                        if (!p->hasKnownReg()) {
                            
                            int d = findMemFor(p);
                            if (p->isop(LIR_alloc)) {
                                LEA(r, d, FP);
                            } else {
                                LD(r, d, FP);
                            }
                        } else {
                            
                            MR(r, p->getReg());
                        }
                    }
                    else {
                        
                        
                        findSpecificRegFor(p, r);
                    }
                }
            }
            else {
                if (config.fixed_esp)
                    asm_stkarg(p, stkd);
                else
                    asm_pusharg(p);
            }
        }
        else
        {
            NanoAssert(sz == ARGSIZE_F);
            asm_farg(p, stkd);
        }
    }

    void Assembler::asm_pusharg(LInsp p)
    {
        
        if (!p->isUsed() && p->isconst())
        {
            
            PUSHi(p->imm32());
        }
        else if (!p->isUsed() || p->isop(LIR_alloc))
        {
            Register ra = findRegFor(p, GpRegs);
            PUSHr(ra);
        }
        else if (!p->hasKnownReg())
        {
            PUSHm(disp(p), FP);
        }
        else
        {
            PUSHr(p->getReg());
        }
    }

    void Assembler::asm_stkarg(LInsp p, int32_t& stkd)
    {
        
        if (!p->isUsed() && p->isconst())
        {
            
            STi(SP, stkd, p->imm32());
        }
        else {
            Register ra;
            if (!p->isUsed() || p->getReg() == UnknownReg || p->isop(LIR_alloc))
                ra = findRegFor(p, GpRegs & (~SavedRegs));
            else
                ra = p->getReg();
            ST(SP, stkd, ra);
        }

        stkd += sizeof(int32_t);
    }

    void Assembler::asm_farg(LInsp p, int32_t& stkd)
    {
        NanoAssert(p->isQuad());
        Register r = findRegFor(p, FpRegs);
        if (rmask(r) & XmmRegs) {
            SSE_STQ(stkd, SP, r);
        } else {
            FSTPQ(stkd, SP);
            
            
            
            
            
            
            
            
            
            



            evictIfActive(FST0);
        }
        if (!config.fixed_esp)
            SUBi(ESP,8);

        stkd += sizeof(double);
    }

    void Assembler::asm_fop(LInsp ins)
    {
        LOpcode op = ins->opcode();
        if (config.sse2)
        {
            LIns *lhs = ins->oprnd1();
            LIns *rhs = ins->oprnd2();

            RegisterMask allow = XmmRegs;
            Register rb = UnknownReg;
            if (lhs != rhs) {
                rb = findRegFor(rhs,allow);
                allow &= ~rmask(rb);
            }

            Register rr = prepResultReg(ins, allow);
            Register ra;

            
            if (lhs->isUnusedOrHasUnknownReg()) {
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
            Register rr = prepResultReg(ins, rmask(FST0));

            
            int db = findMemFor(rhs);

            

            
            
            if (lhs->isUnusedOrHasUnknownReg())
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

    void Assembler::asm_i2f(LInsp ins)
    {
        
        Register rr = prepResultReg(ins, FpRegs);
        if (rmask(rr) & XmmRegs)
        {
            
            Register gr = findRegFor(ins->oprnd1(), GpRegs);
            SSE_CVTSI2SD(rr, gr);
            SSE_XORPDr(rr,rr);  
        }
        else
        {
            int d = findMemFor(ins->oprnd1());
            FILD(d, FP);
        }
    }

    Register Assembler::asm_prep_fcall(Reservation* , LInsp ins)
    {
        return prepResultReg(ins, rmask(FST0));
    }

    void Assembler::asm_u2f(LInsp ins)
    {
        
        Register rr = prepResultReg(ins, FpRegs);
        if (rmask(rr) & XmmRegs)
        {
            
            
            Register gr = registerAllocTmp(GpRegs);

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

            static const double k_NEGONE = 2147483648.0;
            SSE_ADDSDm(rr, &k_NEGONE);

            SSE_CVTSI2SD(rr, gr);
            SSE_XORPDr(rr,rr);  

            LIns* op1 = ins->oprnd1();
            Register xr;
            if (op1->isUsed() && (xr = op1->getReg(), isKnownReg(xr)) && (rmask(xr) & GpRegs))
            {
                LEA(gr, 0x80000000, xr);
            }
            else
            {
                const int d = findMemFor(ins->oprnd1());
                SUBi(gr, 0x80000000);
                LD(gr, d, FP);
            }
        }
        else
        {
            const int disp = -8;
            const Register base = SP;
            Register gr = findRegFor(ins->oprnd1(), GpRegs);
            NanoAssert(rr == FST0);
            FILDQ(disp, base);
            STi(base, disp+4, 0);   
            ST(base, disp, gr);     
        }
    }

    void Assembler::asm_nongp_copy(Register r, Register s)
    {
        if ((rmask(r) & XmmRegs) && (rmask(s) & XmmRegs)) {
            SSE_MOVSD(r, s);
        } else if ((rmask(r) & GpRegs) && (rmask(s) & XmmRegs)) {
            SSE_MOVD(r, s);
        } else {
            if (rmask(r) & XmmRegs) {
                
                NanoAssertMsg(false, "Should not move data from GPR to XMM");
            } else {
                
                NanoAssertMsg(false, "Should not move data from GPR/XMM to x87 FPU");
            }
        }
    }

    NIns* Assembler::asm_fbranch(bool branchOnFalse, LIns *cond, NIns *targ)
    {
        NIns* at;
        LOpcode opcode = cond->opcode();

        if (config.sse2) {
            
            
            
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
        NanoAssert(lhs->isQuad() && rhs->isQuad());

        if (config.sse2) {
            
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
                    findRegFor2b(XmmRegs, lhs, ra, rhs, rb);

                    TEST_AH(mask);
                    LAHF();
                    SSE_UCOMISD(ra, rb);
                }
            } else {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                

                Register ra, rb;
                findRegFor2b(XmmRegs, lhs, ra, rhs, rb);
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
            int pop = lhs->isUnusedOrHasUnknownReg();
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
                int d = findMemFor(rhs);
                
                TEST_AH(mask);
                FNSTSW_AX();
                FCOM((pop?1:0), d, FP);
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
        if (!_nIns)
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
        if (!_nExitIns)
            codeAlloc(exitStart, exitEnd, _nExitIns verbose_only(, exitBytes));
    }

    
    void Assembler::underrunProtect(int n)
    {
        NIns *eip = _nIns;
        NanoAssertMsg(n<=LARGEST_UNDERRUN_PROT, "constant LARGEST_UNDERRUN_PROT is too small");
        if (eip - n < (_inExit ? exitStart : codeStart)) {
            if (_inExit)
                codeAlloc(exitStart, exitEnd, _nIns verbose_only(, exitBytes));
            else
                codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
            JMP(eip);
        }
    }

    void Assembler::asm_ret(LInsp ins)
    {
        genEpilogue();

        
        MR(SP,FP);

        assignSavedRegs();
        LIns *val = ins->oprnd1();
        if (ins->isop(LIR_ret)) {
            findSpecificRegFor(val, retRegs[0]);
        } else {
            findSpecificRegFor(val, FST0);
            fpu_pop();
        }
    }

    void Assembler::asm_promote(LIns *) {
        
        TODO(asm_promote);
    }

    #endif 
}
