







































#ifdef _MAC

#include <CoreServices/CoreServices.h>
#endif

#if defined AVMPLUS_UNIX || defined AVMPLUS_MAC
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>
#endif
#include "nanojit.h"

namespace nanojit
{
	#ifdef FEATURE_NANOJIT

	#ifdef NJ_VERBOSE
		const char *regNames[] = {
			"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
			"xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7",
			"f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7"
		};
	#endif

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
        OSDep::getDate();
	}

	NIns* Assembler::genPrologue()
	{
		


		uint32_t stackNeeded = STACK_GRANULARITY * _activation.highwatermark;

		uint32_t stackPushed =
            STACK_GRANULARITY + 
            STACK_GRANULARITY + 
			STACK_GRANULARITY;  
		
		if (!_thisfrag->lirbuf->explicitSavedRegs)
			stackPushed += NumSavedRegs * STACK_GRANULARITY;
		
		uint32_t aligned = alignUp(stackNeeded + stackPushed, NJ_ALIGN_STACK);
		uint32_t amt = aligned - stackPushed;

		
		
		if (amt) 
			SUBi(SP, amt);

		verbose_only( outputAddr=true; asm_output("[frag entry]"); )
        NIns *fragEntry = _nIns;
		MR(FP, SP); 
        PUSHr(FP); 

		if (!_thisfrag->lirbuf->explicitSavedRegs) {
			PUSHr(FP); 
			for (int i = 0; i < NumSavedRegs; ++i)
				PUSHr(savedRegs[i]);
		}

		return fragEntry;
	}

	void Assembler::nFragExit(LInsp guard)
	{
		SideExit *exit = guard->record()->exit;
		bool trees = _frago->core()->config.tree_opt;
        Fragment *frag = exit->target;
        GuardRecord *lr = 0;
		bool destKnown = (frag && frag->fragEntry);
		
		
		if (guard->isop(LIR_xtbl)) {
			lr = guard->record();
			Register r = EBX;
			SwitchInfo* si = guard->record()->exit->switchInfo;
			emitJumpTable(si, _epilogue);
			JMP_indirect(r);
			LEAmi4(r, si->table, r);
		} else {
			
			if (destKnown && !trees) {
				JMP(frag->fragEntry);
				lr = 0;
			} else {  
				lr = guard->record();
				JMP_long(_epilogue);
				lr->jmp = _nIns;
			}
		}
		
        MR(SP,FP);

		
        LDi(EAX, int(lr));
	}

    NIns *Assembler::genEpilogue()
    {
        RET();

		if (!_thisfrag->lirbuf->explicitSavedRegs) {
			for (int i = NumSavedRegs - 1; i >= 0; --i)
				POPr(savedRegs[i]);
			POPr(FP); 
		}

        POPr(FP); 
        MR(SP,FP); 
        return  _nIns;
    }
	
	void Assembler::asm_call(LInsp ins)
	{
        const CallInfo* call = ins->callInfo();
		
		uint32_t iargs = call->count_iargs();
		int32_t fargs = call->count_args() - iargs - call->isIndirect();

        bool imt = call->isInterface();
        if (imt)
            iargs --;

        uint32_t max_regs = max_abi_regs[call->_abi];
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
		    
		    
		    extra = alignUp(pushsize, align) - pushsize;
            if (call->_abi == ABI_CDECL) {
				
                ADDi(SP, extra+pushsize);
            } else if (extra > 0) {
				ADDi(SP, extra);
            }
        }

        bool indirect = false;
        if (ins->isop(LIR_call) || ins->isop(LIR_fcall)) {
    		CALL(call);
        }
        else {
            
            
            
            
            NanoAssert(ins->isop(LIR_calli) || ins->isop(LIR_fcalli));
            CALLr(call, EAX);
            indirect = true;
        }

		
		NanoAssert(_allocator.isFree(FST0));
		
        
		
		uint32_t n = 0;

        ArgSize sizes[2*MAXARGS];
        uint32_t argc = call->get_sizes(sizes);
        if (indirect) {
            argc--;
            asm_arg(ARGSIZE_LO, ins->arg(argc), EAX);
        }

        if (imt) {
            
            NanoAssert(call->_abi == ABI_CDECL);
            argc--;
            asm_arg(ARGSIZE_LO, ins->arg(argc), EDX);
        }

		for(uint32_t i=0; i < argc; i++)
		{
			uint32_t j = argc-i-1;
            ArgSize sz = sizes[j];
            Register r = UnknownReg;
            if (n < max_regs && sz != ARGSIZE_F) { 
		        r = argRegs[n++]; 
            }
            asm_arg(sz, ins->arg(j), r);
		}

		if (extra > 0)
			SUBi(SP, extra);
	}

	void Assembler::nMarkExecute(Page* page, int flags)
	{
		NanoAssert(sizeof(Page) == NJ_PAGE_SIZE);
		#if defined WIN32 || defined WIN64
			DWORD dwIgnore;
			static const DWORD kProtFlags[4] = 
			{
				PAGE_READONLY,			
				PAGE_READWRITE,			
				PAGE_EXECUTE_READ,		
				PAGE_EXECUTE_READWRITE	
			};
			DWORD prot = kProtFlags[flags & (PAGE_WRITE|PAGE_EXEC)];
			BOOL res = VirtualProtect(page, NJ_PAGE_SIZE, prot, &dwIgnore);
			if (!res)
			{
				
				NanoAssertMsg(false, "FATAL ERROR: VirtualProtect() failed\n");
			}
		#elif defined AVMPLUS_UNIX || defined AVMPLUS_MAC
			static const int kProtFlags[4] = 
			{
				PROT_READ,						
				PROT_READ|PROT_WRITE,			
				PROT_READ|PROT_EXEC,			
				PROT_READ|PROT_WRITE|PROT_EXEC	
			};
			int prot = kProtFlags[flags & (PAGE_WRITE|PAGE_EXEC)];
			intptr_t addr = (intptr_t)page;
			addr &= ~((uintptr_t)NJ_PAGE_SIZE - 1);
			NanoAssert(addr == (intptr_t)page);
			#if defined SOLARIS
			if (mprotect((char *)addr, NJ_PAGE_SIZE, prot) == -1) 
			#else
			if (mprotect((void *)addr, NJ_PAGE_SIZE, prot) == -1) 
			#endif
			{
				
				NanoAssertMsg(false, "FATAL ERROR: mprotect(PROT_EXEC) failed\n");
                abort();
            }
        #else
			(void)page;
		#endif
	}
			
	Register Assembler::nRegisterAllocFromSet(int set)
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
	#elif defined WIN64
		unsigned long tr, fr;
		_BitScanForward(&tr, set);
		_bittestandreset(&fr, tr);
		regs.free = fr;
		r = tr;
	#else
		asm(
			"bsf	%1, %%eax\n\t"
			"btr	%%eax, %2\n\t"
			"movl	%%eax, %0\n\t"
			: "=m"(r) : "m"(set), "m"(regs.free) : "%eax", "memory" );
	#endif 
		return r;
	}

	void Assembler::nRegisterResetAll(RegAlloc& a)
	{
		
		a.clear();
		a.used = 0;
		a.free = SavedRegs | ScratchRegs;
        if (!config.sse2)
            a.free &= ~XmmRegs;
		debug_only( a.managed = a.free; )
	}

	NIns* Assembler::nPatchBranch(NIns* branch, NIns* targ)
	{
        NIns* was = 0;
		intptr_t offset = intptr_t(targ) - intptr_t(branch);
		if (branch[0] == JMP32) {
            was = branch + *(int32_t*)&branch[1] + 5;
		    *(int32_t*)&branch[1] = offset - 5;
            VALGRIND_DISCARD_TRANSLATIONS(&branch[1], sizeof(int32_t));
		} else if (branch[0] == JCC32) {
            was = branch + *(int32_t*)&branch[2] + 6;
		    *(int32_t*)&branch[2] = offset - 6;
            VALGRIND_DISCARD_TRANSLATIONS(&branch[2], sizeof(int32_t));
		} else
		    NanoAssertMsg(0, "Unknown branch type in nPatchBranch");
        return was;
	}

	RegisterMask Assembler::hint(LIns* i, RegisterMask allow)
	{
		uint32_t op = i->opcode();
		int prefer = allow;
        if (op == LIR_call || op == LIR_calli) {
			prefer &= rmask(retRegs[0]);
        }
        else if (op == LIR_fcall || op == LIR_fcalli) {
            prefer &= rmask(FST0);
        }
        else if (op == LIR_param) {
            uint32_t max_regs = max_abi_regs[_thisfrag->lirbuf->abi];
            if (i->imm8() < max_regs)
    			prefer &= rmask(Register(i->imm8()));
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

        Reservation *resv = getresv(ins);
        Register rr = resv->reg;

        if (rr != UnknownReg && (rmask(rr) & FpRegs))
            evict(rr);

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
	
	void Assembler::asm_restore(LInsp i, Reservation *resv, Register r)
	{
        if (i->isop(LIR_alloc)) {
			verbose_only( if (_verbose) { outputForEOL("  <= remat %s size %d", _thisfrag->lirbuf->names->formatRef(i), i->size()); } )
            LEA(r, disp(resv), FP);
        }
        else if (i->isconst()) {
            if (!resv->arIndex) {
                i->clearResv();
            }
            LDi(r, i->imm32());
        }
        else {
            int d = findMemFor(i);
			verbose_only( if (_verbose) { outputForEOL("  <= restore %s", _thisfrag->lirbuf->names->formatRef(i)); } )
			asm_load(d,r);
        }
	}

    void Assembler::asm_store32(LIns *value, int dr, LIns *base)
    {
        if (value->isconst())
        {
			Register rb = getBaseReg(base, dr, GpRegs);
            int c = value->imm32();
			STi(rb, dr, c);
        }
        else
        {
		    
		    Reservation *rA, *rB;
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
    		    findRegFor2(GpRegs, value, rA, base, rB);
		        ra = rA->reg;
		        rb = rB->reg;
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
		int db = ins->oprnd2()->imm32();
		Reservation *resv = getresv(ins);
		Register rr = resv->reg;

		if (rr != UnknownReg && rmask(rr) & XmmRegs)
		{
			freeRsrcOf(ins, false);
			Register rb = getBaseReg(base, db, GpRegs);
			SSE_LDQ(rr, db, rb);
		}
		else
		{
			int dr = disp(resv);
			Register rb;
            if (base->isop(LIR_alloc)) {
                rb = FP;
                db += findMemFor(base);
            } else {
                rb = findRegFor(base, GpRegs);
            }
			resv->reg = UnknownReg;

			
			if (dr)
				asm_mmq(FP, dr, rb, db);

			freeRsrcOf(ins, false);

			if (rr != UnknownReg)
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

		
		
		Reservation* rA = getresv(value);
		Register rv;
		int pop = !rA || rA->reg==UnknownReg;
		if (pop) {
		    rv = findRegFor(value, config.sse2 ? XmmRegs : FpRegs);
		} else {
		    rv = rA->reg;
		}

		if (rmask(rv) & XmmRegs) {
            SSE_STQ(dr, rb, rv);
		} else {
			FSTQ(pop, dr, rb);
		}
	}

    


    void Assembler::asm_mmq(Register rd, int dd, Register rs, int ds)
    {
        
        
        
        if (config.sse2)
        {
            
            Register t = registerAlloc(XmmRegs);
            _allocator.addFree(t);
            SSE_STQ(dd, rd, t);
            SSE_LDQ(t, ds, rs);
        }
        else
        {
            
            Register t = registerAlloc(GpRegs & ~(rmask(rd)|rmask(rs)));
            _allocator.addFree(t);
            ST(rd, dd+4, t);
            LD(t, ds+4, rs);
            ST(rd, dd, t);
            LD(t, ds, rs);
        }
    }

	NIns* Assembler::asm_branch(bool branchOnFalse, LInsp cond, NIns* targ, bool isfar)
	{
		NIns* at = 0;
		LOpcode condop = cond->opcode();
		NanoAssert(cond->isCond());

		if (condop >= LIR_feq && condop <= LIR_fge)
		{
			return asm_jmpcc(branchOnFalse, cond, targ);
		}

		
		if (branchOnFalse)
		{
			if (condop == LIR_eq)
				JNE(targ, isfar);
			else if (condop == LIR_ov)
				JNO(targ, isfar);
			else if (condop == LIR_cs)
				JNC(targ, isfar);
			else if (condop == LIR_lt)
				JNL(targ, isfar);
			else if (condop == LIR_le)
				JNLE(targ, isfar);
			else if (condop == LIR_gt)
				JNG(targ, isfar);
			else if (condop == LIR_ge)
				JNGE(targ, isfar);
			else if (condop == LIR_ult)
				JNB(targ, isfar);
			else if (condop == LIR_ule)
				JNBE(targ, isfar);
			else if (condop == LIR_ugt)
				JNA(targ, isfar);
			else 
				JNAE(targ, isfar);
		}
		else 
		{
			if (condop == LIR_eq)
				JE(targ, isfar);
			else if (condop == LIR_ov)
				JO(targ, isfar);
			else if (condop == LIR_cs)
				JC(targ, isfar);
			else if (condop == LIR_lt)
				JL(targ, isfar);
			else if (condop == LIR_le)
				JLE(targ, isfar);
			else if (condop == LIR_gt)
				JG(targ, isfar);
			else if (condop == LIR_ge)
				JGE(targ, isfar);
			else if (condop == LIR_ult)
				JB(targ, isfar);
			else if (condop == LIR_ule)
				JBE(targ, isfar);
			else if (condop == LIR_ugt)
				JA(targ, isfar);
			else 
				JAE(targ, isfar);
		}
		at = _nIns;
		asm_cmp(cond);
		return at;
	}

	void Assembler::asm_switch(LIns* ins, NIns* exit)
	{
		LIns* diff = ins->oprnd1();
		findSpecificRegFor(diff, EBX);
		JMP(exit);
   	}

	void Assembler::asm_cmp(LIns *cond)
	{
        LOpcode condop = cond->opcode();
        
        
        if ((condop == LIR_ov) || (condop == LIR_cs))
            return;
        
        LInsp lhs = cond->oprnd1();
		LInsp rhs = cond->oprnd2();
		Reservation *rA, *rB;

		NanoAssert((!lhs->isQuad() && !rhs->isQuad()) || (lhs->isQuad() && rhs->isQuad()));

		
		NanoAssert(!lhs->isQuad() && !rhs->isQuad());

		
		if (rhs->isconst())
		{
			int c = rhs->imm32();
			if (c == 0 && cond->isop(LIR_eq)) {
				Register r = findRegFor(lhs, GpRegs);
				TEST(r,r);
			}
			else if (!rhs->isQuad()) {
				Register r = getBaseReg(lhs, c, GpRegs);
				CMPi(r, c);
			}
		}
		else
		{
			findRegFor2(GpRegs, lhs, rA, rhs, rB);
			Register ra = rA->reg;
			Register rb = rB->reg;
			CMP(ra, rb);
		}
	}

	void Assembler::asm_loop(LInsp ins, NInsList& loopJumps)
	{
		JMP_long(0);
        loopJumps.add(_nIns);

		
		
	    if (ins->record()->exit->target != _thisfrag)
	        MR(SP,FP);
	}	

	void Assembler::asm_fcond(LInsp ins)
	{
		
		Register r = prepResultReg(ins, AllowableFlagRegs);
		asm_setcc(r, ins);

		
		MOVZX8(r,r);
		SETNP(r);

		asm_fcmp(ins);
	}
				
	void Assembler::asm_cond(LInsp ins)
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
	
	void Assembler::asm_arith(LInsp ins)
	{
		LOpcode op = ins->opcode();			
		LInsp lhs = ins->oprnd1();
		LInsp rhs = ins->oprnd2();

		Register rb = UnknownReg;
		RegisterMask allow = GpRegs;
		bool forceReg = (op == LIR_mul || !rhs->isconst());

        
		if ((lhs != rhs || (op == LIR_lsh || op == LIR_rsh || op == LIR_ush)) && forceReg)
		{
			if ((rb = asm_binop_rhs_reg(ins)) == UnknownReg) {
				rb = findRegFor(rhs, allow);
			}
			allow &= ~rmask(rb);
		}
		else if ((op == LIR_add||op == LIR_addp) && lhs->isop(LIR_alloc) && rhs->isconst()) {
			
			Register rr = prepResultReg(ins, allow);
			int d = findMemFor(lhs) + rhs->imm32();
			LEA(rr, d, FP);
		}

		Register rr = prepResultReg(ins, allow);
		Reservation* rA = getresv(lhs);
		Register ra;
		
		if (rA == 0 || (ra = rA->reg) == UnknownReg)
			ra = findSpecificRegFor(lhs, rr);
		

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
	
	void Assembler::asm_neg_not(LInsp ins)
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
			MR(rr,ra); 
	}
				
	void Assembler::asm_ld(LInsp ins)
	{
		LOpcode op = ins->opcode();			
		LIns* base = ins->oprnd1();
		LIns* disp = ins->oprnd2();
		Register rr = prepResultReg(ins, GpRegs);
		int d = disp->imm32();

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

			Register rleft;
			Reservation *rL = getresv(lhs);

			


			if (rL == NULL || rL->reg == UnknownReg)
				rleft = findSpecificRegFor(lhs, rr);
			else
				rleft = rL->reg;

			Register rright = UnknownReg;
			Reservation *rR = getresv(rhs);

			
			if (rr != rleft && (rR == NULL || rR->reg == UnknownReg))
				rright = findSpecificRegFor(rhs, rr);
			if (rright == UnknownReg)
				rright = findRegFor(rhs, GpRegs & ~(rmask(rleft)));

			if (op == LIR_ldcb)
				LD8Zsib(rr, d, rleft, rright, scale);
			else if (op == LIR_ldcs)
				LD16Zsib(rr, d, rleft, rright, scale);
			else
				LDsib(rr, d, rleft, rright, scale);

			return;
		}

		Register ra = getBaseReg(base, d, GpRegs);
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
		NanoAssert(condval->isCmp());

		LIns* values = ins->oprnd2();

		NanoAssert(values->opcode() == LIR_2);
		LIns* iftrue = values->oprnd1();
		LIns* iffalse = values->oprnd2();

		NanoAssert(op == LIR_qcmov || (!iftrue->isQuad() && !iffalse->isQuad()));
		
		const Register rr = prepResultReg(ins, GpRegs);

		
		
		const Register iffalsereg = findRegFor(iffalse, GpRegs & ~rmask(rr));
		if (op == LIR_cmov) {
			switch (condval->opcode())
			{
				
				case LIR_eq:	MRNE(rr, iffalsereg);	break;
				case LIR_ov:    MRNO(rr, iffalsereg);   break;
				case LIR_cs:    MRNC(rr, iffalsereg);   break;
				case LIR_lt:	MRGE(rr, iffalsereg);	break;
				case LIR_le:	MRG(rr, iffalsereg);	break;
				case LIR_gt:	MRLE(rr, iffalsereg);	break;
				case LIR_ge:	MRL(rr, iffalsereg);	break;
				case LIR_ult:	MRAE(rr, iffalsereg);	break;
				case LIR_ule:	MRA(rr, iffalsereg);	break;
				case LIR_ugt:	MRBE(rr, iffalsereg);	break;
				case LIR_uge:	MRB(rr, iffalsereg);	break;
			    default: debug_only( NanoAssert(0); ) break;
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
		uint32_t a = ins->imm8();
		uint32_t kind = ins->imm8b();
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
    	Reservation *rR = getresv(ins);
		Register rr = rR->reg;
		if (rr != UnknownReg)
		{
			
			_allocator.retire(rr);
			rR->reg = UnknownReg;
			NanoAssert((rmask(rr) & FpRegs) != 0);

            const double d = ins->imm64f();
            const uint64_t q = ins->imm64();
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
		}
	}

	void Assembler::asm_fneg(LInsp ins)
	{
		if (config.sse2)
		{
			LIns *lhs = ins->oprnd1();

			Register rr = prepResultReg(ins, XmmRegs);
			Reservation *rA = getresv(lhs);
			Register ra;

			
			if (rA == 0 || (ra = rA->reg) == UnknownReg) {
				ra = findSpecificRegFor(lhs, rr);
			} else if ((rmask(ra) & XmmRegs) == 0) {
				



				ra = findRegFor(lhs, XmmRegs);
			}
			

#if defined __SUNPRO_CC
			
			static uint32_t temp[] = {0, 0, 0, 0, 0, 0, 0};
			static uint32_t *negateMask = (uint32_t *)alignUp(temp, 16);
			negateMask[1] = 0x80000000;
#else
			static const AVMPLUS_ALIGN16(uint32_t) negateMask[] = {0,0x80000000,0,0};
#endif
			SSE_XORPD(rr, negateMask);

			if (rr != ra)
				SSE_MOVSD(rr, ra);
		}
		else
		{
			Register rr = prepResultReg(ins, FpRegs);

			LIns* lhs = ins->oprnd1();

			
			Reservation* rA = getresv(lhs);
			
			if (rA == 0 || rA->reg == UnknownReg)
				findSpecificRegFor(lhs, rr);
			

			NanoAssert(getresv(lhs)!=0 && getresv(lhs)->reg==FST0);
			
			FCHS();

			
			
		}
	}

    void Assembler::asm_arg(ArgSize sz, LInsp p, Register r)
    {
        if (sz == ARGSIZE_Q) 
        {
			
			if (r != UnknownReg)
			{
				
				int da = findMemFor(p);
				LEA(r, da, FP);
			}
			else
			{
				NanoAssert(0); 
			}
		}
        else if (sz == ARGSIZE_LO)
		{
			if (r != UnknownReg) {
				
                if (p->isconst()) {
					LDi(r, p->imm32());
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
                            
                            MR(r, rA->reg);
                        }
                    } 
                    else {
                        
                        
    					findSpecificRegFor(p, r);
                    }
                }
			}
            else {
				asm_pusharg(p);
			}
		}
        else
		{
            NanoAssert(sz == ARGSIZE_F);
			asm_farg(p);
		}
    }

	void Assembler::asm_pusharg(LInsp p)
	{
		
		Reservation* rA = getresv(p);
		if (rA == 0 && p->isconst())
		{
			
			PUSHi(p->imm32());
		}
		else if (rA == 0 || p->isop(LIR_alloc))
		{
			Register ra = findRegFor(p, GpRegs);
			PUSHr(ra);
		}
		else if (rA->reg == UnknownReg)
		{
			PUSHm(disp(rA), FP);
		}
		else
		{
			PUSHr(rA->reg);
		}
	}

	void Assembler::asm_farg(LInsp p)
	{
        NanoAssert(p->isQuad());
		Register r = findRegFor(p, FpRegs);
		if (rmask(r) & XmmRegs) {
			SSE_STQ(0, SP, r); 
		} else {
			FSTPQ(0, SP);
			



			evict(FST0);
		}
        SUBi(ESP,8);
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
			Reservation *rA = getresv(lhs);
			Register ra;

			
			if (rA == 0 || (ra = rA->reg) == UnknownReg) {
				ra = findSpecificRegFor(lhs, rr);
			} else if ((rmask(ra) & XmmRegs) == 0) {
				



				ra = findRegFor(lhs, XmmRegs);
			}
            else {
    			
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

			
			Reservation* rA = getresv(lhs);
			
			if (rA == 0 || rA->reg == UnknownReg)
				findSpecificRegFor(lhs, rr);
			

			NanoAssert(getresv(lhs)!=0 && getresv(lhs)->reg==FST0);
			
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
		} 
		else 
		{
			int d = findMemFor(ins->oprnd1());
			FILD(d, FP);
		}
	}

	Register Assembler::asm_prep_fcall(Reservation *rR, LInsp ins)
	{
		if (rR) {
    		Register rr;
			if ((rr=rR->reg) != UnknownReg && (rmask(rr) & XmmRegs))
				evict(rr);
		}
		return prepResultReg(ins, rmask(FST0));
	}

	void Assembler::asm_u2f(LInsp ins)
	{
		
		Register rr = prepResultReg(ins, FpRegs);
		if (rmask(rr) & XmmRegs) 
		{
			
			
			Register gr = registerAlloc(GpRegs);

			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			static const double k_NEGONE = 2147483648.0;
			SSE_ADDSDm(rr, &k_NEGONE);

			SSE_CVTSI2SD(rr, gr);

			Reservation* resv = getresv(ins->oprnd1());
			Register xr;
			if (resv && (xr = resv->reg) != UnknownReg && (rmask(xr) & GpRegs))
			{
				LEA(gr, 0x80000000, xr);
			}
			else
			{
				const int d = findMemFor(ins->oprnd1());
				SUBi(gr, 0x80000000);
				LD(gr, d, FP);
			}
			
			
			_allocator.addFree(gr); 
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

    NIns * Assembler::asm_jmpcc(bool branchOnFalse, LIns *cond, NIns *targ)
    {
        LOpcode c = cond->opcode();
        if (config.sse2 && c != LIR_feq) {
            LIns *lhs = cond->oprnd1();
            LIns *rhs = cond->oprnd2();
            if (c == LIR_flt) {
                LIns *t = lhs; lhs = rhs; rhs = t;
                c = LIR_fgt;
            }
            else if (c == LIR_fle) {
                LIns *t = lhs; lhs = rhs; rhs = t;
                c = LIR_fge;
            }

            if (c == LIR_fgt) {
                if (branchOnFalse) { JNA(targ, false); } else { JA(targ, false); }
            }
            else { 
                if (branchOnFalse) { JNAE(targ, false); } else { JAE(targ, false); }
            }
            NIns *at = _nIns;
            Reservation *rA, *rB;
            findRegFor2(XmmRegs, lhs, rA, rhs, rB);
            SSE_UCOMISD(rA->reg, rB->reg);
            return at;
        }

    	if (branchOnFalse)
			JP(targ, false);
		else
			JNP(targ, false);
		NIns *at = _nIns;
		asm_fcmp(cond);
        return at;
    }

    void Assembler::asm_setcc(Register r, LIns *cond)
    {
        LOpcode c = cond->opcode();
        if (config.sse2 && c != LIR_feq) {
    		MOVZX8(r,r);
            LIns *lhs = cond->oprnd1();
            LIns *rhs = cond->oprnd2();
            if (c == LIR_flt) {
                LIns *t = lhs; lhs = rhs; rhs = t;
                SETA(r);
            }
            else if (c == LIR_fle) {
                LIns *t = lhs; lhs = rhs; rhs = t;
                SETAE(r);
            }
            else if (c == LIR_fgt) {
                SETA(r);
            }
            else { 
                SETAE(r);
            }
            Reservation *rA, *rB;
            findRegFor2(XmmRegs, lhs, rA, rhs, rB);
            SSE_UCOMISD(rA->reg, rB->reg);
            return;
        }
		
		MOVZX8(r,r);
		SETNP(r);
        asm_fcmp(cond);
    }

	void Assembler::asm_fcmp(LIns *cond)
	{
		LOpcode condop = cond->opcode();
		NanoAssert(condop >= LIR_feq && condop <= LIR_fge);
	    LIns* lhs = cond->oprnd1();
	    LIns* rhs = cond->oprnd2();

        int mask;
	    if (condop == LIR_feq)
		    mask = 0x44;
	    else if (condop == LIR_fle)
		    mask = 0x41;
	    else if (condop == LIR_flt)
		    mask = 0x05;
        else if (condop == LIR_fge) {
            
            condop = LIR_fle;
            LIns* t = lhs; lhs = rhs; rhs = t;
            mask = 0x41;
        } else { 
            
            condop = LIR_flt;
            LIns* t = lhs; lhs = rhs; rhs = t;
		    mask = 0x05;
        }

        if (config.sse2)
        {
            
            
            
            

            if (condop == LIR_feq && lhs == rhs) {
                
                Register r = findRegFor(lhs, XmmRegs);
                SSE_UCOMISD(r, r);
            } 
            else {
                evict(EAX);
                TEST_AH(mask);
                LAHF();

                Reservation *rA, *rB;
                findRegFor2(XmmRegs, lhs, rA, rhs, rB);
                SSE_UCOMISD(rA->reg, rB->reg);
            }
        }
        else
        {
            evict(EAX);
            TEST_AH(mask);
		    FNSTSW_AX();
		    NanoAssert(lhs->isQuad() && rhs->isQuad());
		    Reservation *rA;
		    if (lhs != rhs)
		    {
			    
			    int d = findMemFor(rhs);
			    rA = getresv(lhs);
			    int pop = !rA || rA->reg == UnknownReg; 
			    findSpecificRegFor(lhs, FST0);
			    
			    FCOM(pop, d, FP);
		    }
		    else
		    {
			    
			    rA = getresv(lhs);
			    int pop = !rA || rA->reg == UnknownReg; 
			    findSpecificRegFor(lhs, FST0);
			    
			    if (pop)
				    FCOMPP();
			    else
				    FCOMP();
			    FLDr(FST0); 
		    }
        }
	}
	
	void Assembler::nativePageReset()
	{
	}

	Register Assembler::asm_binop_rhs_reg(LInsp ins)
	{
		LOpcode op = ins->opcode();
		LIns *rhs = ins->oprnd2();

		if (op == LIR_lsh || op == LIR_rsh || op == LIR_ush)
			return findSpecificRegFor(rhs, ECX);

		return UnknownReg;	
	}

	void Assembler::nativePageSetup()
	{
		if (!_nIns)		 _nIns	   = pageAlloc();
		if (!_nExitIns)  _nExitIns = pageAlloc(true);
	}
	
    
    
    
    void Assembler::resetInstructionPointer()
    {
        _nIns = _startingIns;
    }
    
    
    void Assembler::recordStartingInstructionPointer()
    {
        _startingIns = _nIns;
    }

	
    void Assembler::underrunProtect(int n)
    {
		NanoAssertMsg(n<=LARGEST_UNDERRUN_PROT, "constant LARGEST_UNDERRUN_PROT is too small"); 
        NIns *eip = this->_nIns;
        Page *p = (Page*)pageTop(eip-1);
        NIns *top = (NIns*) &p->code[0];
        if (eip - n < top) {
            
            
            VALGRIND_DISCARD_TRANSLATIONS(pageTop(p), NJ_PAGE_SIZE);
			_nIns = pageAlloc(_inExit);
            JMP(eip);
        }
    }
	
	#endif 
}
