





































#ifdef _MAC

#include <CoreServices/CoreServices.h>
#endif

#if defined LINUX
#include <sys/mman.h>
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

	void Assembler::nInit(AvmCore* core)
	{
        sse2 = core->use_sse2();
		
		
		has_cmov = sse2;
        OSDep::getDate();
	}

	NIns* Assembler::genPrologue(RegisterMask needSaving)
	{
		


		uint32_t stackNeeded = 4 * _activation.highwatermark;
		uint32_t savingCount = 0;

		for(Register i=FirstReg; i <= LastReg; i = nextreg(i))
			if (needSaving&rmask(i)) 
				savingCount++;

		
		
		uint32_t stackPushed = 4 * (1+savingCount);
		uint32_t aligned = alignUp(stackNeeded + stackPushed, NJ_ALIGN_STACK);
		uint32_t amt = aligned - stackPushed;

		
		
		if (amt) 
			SUBi(SP, amt);

		verbose_only( verbose_outputf("        %p:",_nIns); )
		verbose_only( verbose_output("        patch entry:"); )
        NIns *patchEntry = _nIns;
		MR(FP, SP); 

		
		
		
		
		
		PUSHr(FP);

		for(Register i=FirstReg; i <= LastReg; i = nextreg(i))
			if (needSaving&rmask(i))
				PUSHr(i);

		
		
		
		
		
		
		
		
		ANDi(SP, -NJ_ALIGN_STACK);
		MR(FP,SP);
		PUSHr(FP); 

		return patchEntry;
	}

	void Assembler::nFragExit(LInsp guard)
	{
		SideExit *exit = guard->exit();
		bool trees = _frago->core()->config.tree_opt;
        Fragment *frag = exit->target;
        GuardRecord *lr = 0;
		bool destKnown = (frag && frag->fragEntry);
		if (destKnown && !trees)
		{
			
			JMP(frag->fragEntry);
            lr = 0;
		}
		else
		{
			
			lr = placeGuardRecord(guard);
            JMP_long(_epilogue);
			lr->jmp = _nIns;
#if 0			
			
			
			if (tress && destKnown)
				patch(lr);
#endif
		}
		
        MR(SP,FP);


        #ifdef NJ_VERBOSE
        if (_frago->core()->config.show_stats) {
			
			
            int fromfrag = int((Fragment*)_thisfrag);
            LDi(argRegs[1], fromfrag);
        }
        #endif

		
        LDi(EAX, int(lr));
	}


    NIns *Assembler::genEpilogue(RegisterMask restore)
    {
        RET();
        POPr(FP); 
        MR(SP,FP); 

		
		for (Register i=UnknownReg; i >= FirstReg; i = prevreg(i))
			if (restore&rmask(i)) { POPr(i); } 
		
		POPr(FP); 
        return  _nIns;
    }
	
	void Assembler::asm_call(LInsp ins)
	{
        uint32_t fid = ins->fid();
        const CallInfo* call = callInfoFor(fid);
		
		const uint32_t iargs = call->count_iargs();
		int32_t fstack = call->count_args() - iargs;

        int32_t extra = 0;
		int32_t istack = iargs-2;  
		if (istack <= 0)
		{
			istack = 0;
		}

		const int32_t size = 4*istack + 8*fstack; 
        if (size) {
		    
		    
		    extra = alignUp(size, NJ_ALIGN_STACK) - (size); 
		    if (extra > 0)
			    ADDi(SP, extra);
        }

		CALL(call);

		
		NanoAssert(_allocator.isFree(FST0));
		
        
		
		const int max_regs = (iargs < 2) ? iargs : 2;
		int n = 0;

        ArgSize sizes[10];
        uint32_t argc = call->get_sizes(sizes);

		for(uint32_t i=0; i < argc; i++)
		{
			uint32_t j = argc-i-1;
            ArgSize sz = sizes[j];
            Register r = UnknownReg;
            if (n < max_regs && sz != ARGSIZE_F) 
			    r = argRegs[n++]; 
            asm_arg(sz, ins->arg(j), r);
		}

		if (extra > 0)
			SUBi(SP, extra);
	}
	
	void Assembler::nMarkExecute(Page* page, int32_t count, bool enable)
	{
		#ifdef _MAC
			MakeDataExecutable(page, count*NJ_PAGE_SIZE);
		#elif defined WIN32
			DWORD dwIgnore;
			VirtualProtect(&page->code, count*NJ_PAGE_SIZE, PAGE_EXECUTE_READWRITE, &dwIgnore);
		#elif defined LINUX
			intptr_t addr = (intptr_t)&page->code;
			addr &= ~(NJ_PAGE_SIZE - 1);
			mprotect((void *)addr, count*NJ_PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC);
		#endif
			(void)enable;
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
        if (!sse2)
            a.free &= ~XmmRegs;
		debug_only( a.managed = a.free; )
	}

	void Assembler::nPatchBranch(NIns* branch, NIns* location)
	{
		uint32_t offset = location - branch;
		if (branch[0] == JMPc)
			*(uint32_t*)&branch[1] = offset - 5;
		else
			*(uint32_t*)&branch[2] = offset - 6;
	}

	RegisterMask Assembler::hint(LIns* i, RegisterMask allow)
	{
		uint32_t op = i->opcode();
		int prefer = allow;
		if (op == LIR_call)
			prefer &= rmask(EAX);
		else if (op == LIR_param)
			prefer &= rmask(Register(i->imm8()));
        else if (op == LIR_callh || op == LIR_rsh && i->oprnd1()->opcode()==LIR_callh)
            prefer &= rmask(EDX);
		else if (i->isCmp())
			prefer &= AllowableFlagRegs;
        else if (i->isconst())
            prefer &= ScratchRegs;
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
			STi(FP, d+4, hi->constval());
		}
		else
		{
			Register r = findRegFor(hi, GpRegs);
			ST(FP, d+4, r);
		}

        if (lo->isconst())
		{
			STi(FP, d, lo->constval());
		}
		else
		{
			
			Register r = findRegFor(lo, GpRegs);
			ST(FP, d, r);
		}

        freeRsrcOf(ins, false);	
    }

	void Assembler::asm_restore(LInsp i, Reservation *resv, Register r)
	{
        if (i->isconst())
        {
            if (!resv->arIndex) {
                reserveFree(i);
            }
            LDi(r, i->constval());
        }
        else
        {
            int d = findMemFor(i);
            if (rmask(r) & FpRegs)
		    {
                if (rmask(r) & XmmRegs) {
                    LDQ(r, d, FP);
                } else {
			        FLDQ(d, FP); 
                }
            }
            else
		    {
			    LD(r, d, FP);
		    }
			verbose_only(if (_verbose) {
				outputf("        restore %s", _thisfrag->lirbuf->names->formatRef(i));
			})
        }
	}

    void Assembler::asm_store32(LIns *value, int dr, LIns *base)
    {
        if (value->isconst())
        {
			Register rb = findRegFor(base, GpRegs);
            int c = value->constval();
			STi(rb, dr, c);
        }
        else
        {
		    
		    Reservation *rA, *rB;
		    findRegFor2(GpRegs, value, rA, base, rB);
		    Register ra = rA->reg;
		    Register rb = rB->reg;
		    ST(rb, dr, ra);
        }
    }

	void Assembler::asm_spill(LInsp i, Reservation *resv, bool pop)
	{
		(void)i;
		int d = disp(resv);
		Register rr = resv->reg;
		if (d)
		{
			
            if (rmask(rr) & FpRegs)
			{
                if (rmask(rr) & XmmRegs) {
                    STQ(d, FP, rr);
                } else {
					FSTQ((pop?1:0), d, FP);
                }
			}
			else
			{
				ST(FP, d, rr);
			}
			verbose_only(if (_verbose) {
				outputf("        spill %s",_thisfrag->lirbuf->names->formatRef(i));
			})
		}
		else if (pop && (rmask(rr) & x87Regs))
		{
			
			FSTP(FST0);
		}
	}

	void Assembler::asm_load64(LInsp ins)
	{
		LIns* base = ins->oprnd1();
		int db = ins->oprnd2()->constval();
		Reservation *resv = getresv(ins);
		int dr = disp(resv);
		Register rr = resv->reg;

		if (rr != UnknownReg && rmask(rr) & XmmRegs)
		{
			freeRsrcOf(ins, false);
			Register rb = findRegFor(base, GpRegs);
			LDQ(rr, db, rb);
		}
		else
		{
			Register rb = findRegFor(base, GpRegs);
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
			
			
			Register rb = findRegFor(base, GpRegs);
			const int32_t* p = (const int32_t*) (value-2);
			STi(rb, dr+4, p[1]);
			STi(rb, dr, p[0]);
            return;
		}

        if (value->isop(LIR_ldq) || value->isop(LIR_qjoin))
		{
			
			
			

			
			
			
			

			if (sse2) {
                Register rv = findRegFor(value, XmmRegs);
                Register rb = findRegFor(base, GpRegs);
                STQ(dr, rb, rv);
				return;
            }

			int da = findMemFor(value);
		    Register rb = findRegFor(base, GpRegs);
		    asm_mmq(rb, dr, FP, da);
            return;
		}

		Reservation* rA = getresv(value);
		int pop = !rA || rA->reg==UnknownReg;
 		Register rv = findRegFor(value, sse2 ? XmmRegs : FpRegs);
		Register rb = findRegFor(base, GpRegs);

		if (rmask(rv) & XmmRegs) {
            STQ(dr, rb, rv);
		} else {
			FSTQ(pop, dr, rb);
		}
	}

    


    void Assembler::asm_mmq(Register rd, int dd, Register rs, int ds)
    {
        
        
        
        if (sse2)
        {
            
            Register t = registerAlloc(XmmRegs);
            _allocator.addFree(t);
            STQ(dd, rd, t);
            LDQ(t, ds, rs);
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

	void Assembler::asm_quad(LInsp ins)
	{
    	Reservation *rR = getresv(ins);
		Register rr = rR->reg;
		if (rr != UnknownReg)
		{
			
			_allocator.retire(rr);
			rR->reg = UnknownReg;
			NanoAssert((rmask(rr) & FpRegs) != 0);

			const double d = ins->constvalf();
			if (rmask(rr) & XmmRegs) {
				if (d == 0.0) {
					XORPDr(rr, rr);
				} else if (d == 1.0) {
					
					static const double k_ONE = 1.0;
					LDSDm(rr, &k_ONE);
				} else {
					findMemFor(ins);
					const int d = disp(rR);
					LDQ(rr, d, FP);
				}
			} else {
				if (d == 0.0) {
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
			const int32_t* p = (const int32_t*) (ins-2);
			STi(FP,d+4,p[1]);
			STi(FP,d,p[0]);
		}
	}
	
	bool Assembler::asm_qlo(LInsp ins, LInsp q)
	{
		if (!sse2)
		{
			return false;
		}

		Reservation *resv = getresv(ins);
		Register rr = resv->reg;
		if (rr == UnknownReg) {
			
			int d = disp(resv);
			freeRsrcOf(ins, false);
			Register qr = findRegFor(q, XmmRegs);
			STD(d, FP, qr);
		} else {
			freeRsrcOf(ins, false);
			Register qr = findRegFor(q, XmmRegs);
			MOVD(rr,qr);
		}

		return true;
	}

	void Assembler::asm_fneg(LInsp ins)
	{
		if (sse2)
		{
			LIns *lhs = ins->oprnd1();

			Register rr = prepResultReg(ins, XmmRegs);
			Reservation *rA = getresv(lhs);
			Register ra;

			
			if (rA == 0 || (ra = rA->reg) == UnknownReg)
				ra = findSpecificRegFor(lhs, rr);
			

			static const AVMPLUS_ALIGN16(uint32_t) negateMask[] = {0,0x80000000,0,0};
			XORPD(rr, negateMask);

			if (rr != ra)
				MOVSD(rr, ra);
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

	void Assembler::asm_pusharg(LInsp p)
	{
		
		Reservation* rA = getresv(p);
		if (rA == 0)
		{
			if (p->isconst())
			{
				
				PUSHi(p->constval());
			}
			else
			{
				Register ra = findRegFor(p, GpRegs);
				PUSHr(ra);
			}
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
		Register r = findRegFor(p, FpRegs);
		if (rmask(r) & XmmRegs) {
			STQ(0, SP, r); 
		} else {
			FSTPQ(0, SP);
		}
		PUSHr(ECX); 
		PUSHr(ECX);
	}

	void Assembler::asm_fop(LInsp ins)
	{
		LOpcode op = ins->opcode();
		if (sse2) 
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

			
			if (rA == 0 || (ra = rA->reg) == UnknownReg)
				ra = findSpecificRegFor(lhs, rr);
			

			if (lhs == rhs)
				rb = ra;

			if (op == LIR_fadd)
				ADDSD(rr, rb);
			else if (op == LIR_fsub)
				SUBSD(rr, rb);
			else if (op == LIR_fmul)
				MULSD(rr, rb);
			else 
				DIVSD(rr, rb);

			if (rr != ra)
				MOVSD(rr, ra);
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
			CVTSI2SD(rr, gr);
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
		const int disp = -8;
		const Register base = ESP;
		if (rmask(rr) & XmmRegs) 
		{
			
			
			Register gr = registerAlloc(GpRegs);

			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			static const double k_NEGONE = 2147483648.0;
			ADDSDm(rr, &k_NEGONE);
			CVTSI2SD(rr, gr);

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
			MOVSD(r, s);
		} else {
			if (rmask(r) & XmmRegs) {
				
				NanoAssert(false);
			} else {
				
				NanoAssert(false);
			}
		}
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
            
            LIns* t = lhs; lhs = rhs; rhs = t;
            mask = 0x41;
        } else { 
            
            LIns* t = lhs; lhs = rhs; rhs = t;
		    mask = 0x05;
        }

        if (sse2)
        {
            
            
            
            

            if (condop == LIR_feq && lhs == rhs) {
                
                Register r = findRegFor(lhs, XmmRegs);
                UCOMISD(r, r);
            } else {
                evict(EAX);
                TEST_AH(mask);
                LAHF();
                Reservation *rA, *rB;
                findRegFor2(XmmRegs, lhs, rA, rhs, rB);
                UCOMISD(rA->reg, rB->reg);
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
	
	NIns* Assembler::asm_adjustBranch(NIns* at, NIns* target)
	{
		NIns* save = _nIns;
		NIns* was = (NIns*)( (intptr_t)*(int32_t*)(at+1)+(intptr_t)(at+5) );
		_nIns = at +5; 
		intptr_t tt = (intptr_t)target - (intptr_t)_nIns;
		IMM32(tt);
		*(--_nIns) = JMPc;
		_nIns = save;
		return was;
	}
	
	void Assembler::nativePageReset()	{}

	void Assembler::nativePageSetup()
	{
		if (!_nIns)		 _nIns	   = pageAlloc();
		if (!_nExitIns)  _nExitIns = pageAlloc(true);
	}
	#endif 
}
