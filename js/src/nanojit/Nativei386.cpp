







































#ifdef _MAC

#include <CoreServices/CoreServices.h>
#endif

#if defined AVMPLUS_UNIX
#include <sys/mman.h>
#include <errno.h>
#endif
#include "nanojit.h"

namespace nanojit
{
	#ifdef FEATURE_NANOJIT

	#ifdef NJ_VERBOSE
		const char *regNames[] = {
#if defined NANOJIT_IA32
			"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
			"xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7",
			"f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7"
#elif defined NANOJIT_AMD64
			"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
			"r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
			"xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7",
            "xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15"
#endif
		};
	#endif

#if defined NANOJIT_IA32
    const Register Assembler::argRegs[] = { ECX, EDX };
    const Register Assembler::retRegs[] = { EAX, EDX };
    const Register Assembler::savedRegs[] = { EBX, ESI, EDI };
#elif defined NANOJIT_AMD64
#if defined WIN64
	const Register Assembler::argRegs[] = { R8, R9, RCX, RDX };
#else
	const Register Assembler::argRegs[] = { RDI, RSI, RDX, RCX, R8, R9 };
#endif
	const Register Assembler::retRegs[] = { RAX, RDX };
	const Register Assembler::savedRegs[] = { R13, R14, R15 };
#endif

    const static uint8_t max_abi_regs[] = {
        2, 
        1, 
        0, 
        0  
    };


	void Assembler::nInit(AvmCore* core)
	{
        OSDep::getDate();
#ifdef NANOJIT_AMD64
        avmplus::AvmCore::cmov_available =
        avmplus::AvmCore::sse2_available = true;
#endif
	}

	NIns* Assembler::genPrologue()
	{
		


		uint32_t stackNeeded = STACK_GRANULARITY * _activation.highwatermark;

		uint32_t stackPushed =
            STACK_GRANULARITY + 
            STACK_GRANULARITY; 
		uint32_t aligned = alignUp(stackNeeded + stackPushed, NJ_ALIGN_STACK);
		uint32_t amt = aligned - stackPushed;

		
		
		if (amt) 
		{
#if defined NANOJIT_IA32
			SUBi(SP, amt);
#elif defined NANOJIT_AMD64
			SUBQi(SP, amt);
#endif
		}

		verbose_only( verbose_outputf("        %p:",_nIns); )
		verbose_only( verbose_output("        patch entry:"); )
        NIns *patchEntry = _nIns;
		MR(FP, SP); 
        PUSHr(FP); 

        
        asm_align_code();

		return patchEntry;
	}

    void Assembler::asm_align_code() {
        static char nop[][9] = {
                {0x90},
                {0x66,0x90},
                {0x0f,0x1f,0x00},
                {0x0f,0x1f,0x40,0x00},
                {0x0f,0x1f,0x44,0x00,0x00},
                {0x66,0x0f,0x1f,0x44,0x00,0x00},
                {0x0f,0x1f,0x80,0x00,0x00,0x00,0x00},
                {0x0f,0x1f,0x84,0x00,0x00,0x00,0x00,0x00},
                {0x66,0x0f,0x1f,0x84,0x00,0x00,0x00,0x00,0x00},
        };
        unsigned n;
        while((n = uintptr_t(_nIns) & 15) != 0) {
            if (n > 9)
                n = 9;
            underrunProtect(n);
            _nIns -= n;
            memcpy(_nIns, nop[n-1], n);
            asm_output1("nop%d", n);
        }
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
#if defined NANOJIT_AMD64
            
            underrunProtect(14);
            _nIns -= 8;
            *(intptr_t *)_nIns = intptr_t(_epilogue);
            lr->jmp = _nIns;
            JMPm_nochk(0);
#else
            JMP_long(_epilogue);
			lr->jmp = _nIns;
#endif
#if 0			
			
			
			if (tress && destKnown)
				patch(lr);
#endif
		}
		
        MR(SP,FP);

        #ifdef NJ_VERBOSE
        if (_frago->core()->config.show_stats) {
			
			
		#if defined NANOJIT_IA32
            int fromfrag = int((Fragment*)_thisfrag);
            LDi(argRegs[1], fromfrag);
		#elif defined NANOJIT_AMD64
			LDQi(argRegs[1], intptr_t(_thisfrag));
		#endif
        }
        #endif

		
	#if defined NANOJIT_IA32
        LDi(EAX, int(lr));
	#elif defined NANOJIT_AMD64
		LDQi(RAX, intptr_t(lr));
	#endif
	}

    NIns *Assembler::genEpilogue()
    {
        RET();
        POPr(FP); 
        MR(SP,FP); 
        return  _nIns;
    }
	
#if defined NANOJIT_IA32
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
            verbose_only(if (_verbose)
                outputf("        %p:", _nIns);
            )
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

#elif defined NANOJIT_AMD64

	void Assembler::asm_call(LInsp ins)
	{
		Register fpu_reg = XMM0;
        const CallInfo* call = ins->callInfo();
		int n = 0;

		CALL(call);

        ArgSize sizes[10];
        uint32_t argc = call->get_sizes(sizes);

		for(uint32_t i=0; i < argc; i++)
		{
			uint32_t j = argc-i-1;
            ArgSize sz = sizes[j];
            Register r = UnknownReg;
            if (sz != ARGSIZE_F) {
			    r = argRegs[n++]; 
			} else {
				r = fpu_reg;
				fpu_reg = nextreg(fpu_reg);
			}
			findSpecificRegFor(ins->arg(j), r);
		}
	}
#endif
	
	void Assembler::nMarkExecute(Page* page, int32_t count, bool enable)
	{
		#if defined WIN32 || defined WIN64
			DWORD dwIgnore;
			VirtualProtect(&page->code, count*NJ_PAGE_SIZE, PAGE_EXECUTE_READWRITE, &dwIgnore);
		#elif defined AVMPLUS_UNIX
			intptr_t addr = (intptr_t)&page->code;
			addr &= ~((uintptr_t)NJ_PAGE_SIZE - 1);
			#if defined SOLARIS
			if (mprotect((char *)addr, count*NJ_PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC) == -1) {
			#else
			if (mprotect((void *)addr, count*NJ_PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC) == -1) {
			#endif
                AvmDebugLog(("FATAL ERROR: mprotect(PROT_EXEC) failed\n"));
                abort();
            }
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
#if defined NANOJIT_IA32
        if (!avmplus::AvmCore::use_sse2())
            a.free &= ~XmmRegs;
#endif
		debug_only( a.managed = a.free; )
	}

	void Assembler::nPatchBranch(NIns* branch, NIns* location)
	{
#if defined NANOJIT_IA32
		intptr_t offset = intptr_t(location) - intptr_t(branch);
		if (branch[0] == JMPc)
			*(uint32_t*)&branch[1] = offset - 5;
		else
			*(uint32_t*)&branch[2] = offset - 6;
#else
        if (branch[0] == 0xFF && branch[1] == 0x25) {
            NIns *mem;

            mem = &branch[6] + *(int32_t *)&branch[2];
            *(intptr_t *)mem = intptr_t(location);
        } else {
            NanoAssertMsg(0, "Unknown branch type in nPatchBranch");
        }
#endif
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
        else if (op == LIR_callh || op == LIR_rsh && i->oprnd1()->opcode()==LIR_callh) {
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

	void Assembler::asm_load(int d, Register r)
	{
		if (rmask(r) & FpRegs)
		{
#if defined NANOJIT_IA32
			if (rmask(r) & XmmRegs) {
#endif
				SSE_LDQ(r, d, FP);
#if defined NANOJIT_IA32
			} else {
				FLDQ(d, FP); 
			}
#endif
		}
#if defined NANOJIT_AMD64
		else if (i->opcode() == LIR_param)
		{
			LDQ(r, d, FP);
		}
#endif
		else
		{
			LD(r, d, FP);
		}
	}
	
	void Assembler::asm_restore(LInsp i, Reservation *resv, Register r)
	{
        if (i->isop(LIR_alloc)) {
            LEA(r, disp(resv), FP);
            verbose_only(if (_verbose) {
                outputf("        remat %s size %d", _thisfrag->lirbuf->names->formatRef(i), i->size());
            })
        }
        else if (i->isconst()) {
            if (!resv->arIndex) {
                reserveFree(i);
            }
            LDi(r, i->constval());
        }
        else {
            int d = findMemFor(i);
			asm_load(d,r);
			verbose_only(if (_verbose) {
				outputf("        restore %s", _thisfrag->lirbuf->names->formatRef(i));
			})
        }
	}

    void Assembler::asm_store32(LIns *value, int dr, LIns *base)
    {
        if (value->isconst())
        {
			Register rb = getBaseReg(base, dr, GpRegs);
            int c = value->constval();
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
                
                dr += base->constval();
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
#if defined NANOJIT_IA32
                if (rmask(rr) & XmmRegs) {
#endif
                    SSE_STQ(d, FP, rr);
#if defined NANOJIT_IA32
                } else {
					FSTQ((pop?1:0), d, FP);
                }
#endif
			}
#if defined NANOJIT_AMD64
			else if (quad)
			{
				STQ(FP, d, rr);
			}
#endif
			else
			{
				ST(FP, d, rr);
			}
		}
#if defined NANOJIT_IA32
		else if (pop && (rmask(rr) & x87Regs))
		{
			
			FSTP(FST0);
		}
#endif	
	}

	void Assembler::asm_load64(LInsp ins)
	{
		LIns* base = ins->oprnd1();
		int db = ins->oprnd2()->constval();
		Reservation *resv = getresv(ins);
		Register rr = resv->reg;

		if (rr != UnknownReg && rmask(rr) & XmmRegs)
		{
			freeRsrcOf(ins, false);
			Register rb = getBaseReg(base, db, GpRegs);
			SSE_LDQ(rr, db, rb);
		}
#if defined NANOJIT_AMD64
		else if (rr != UnknownReg && rmask(rr) & GpRegs)
		{
			freeRsrcOf(ins, false);
			Register rb = findRegFor(base, GpRegs);
			LDQ(rr, db, rb);
		}
		else
		{
            int d = disp(resv);
            Register rb = findRegFor(base, GpRegs);

            
            rr = registerAlloc(GpRegs);

            STQ(FP, d, rr);
            LDQ(rr, db, rb);

            
            _allocator.addFree(rr);

			freeRsrcOf(ins, false);
		}
#elif defined NANOJIT_IA32
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
#endif
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
			const int32_t* p = (const int32_t*) (value-2);
			STi(rb, dr+4, p[1]);
			STi(rb, dr, p[0]);
            return;
		}

#if defined NANOJIT_IA32
        if (value->isop(LIR_ldq) || value->isop(LIR_ldqc) || value->isop(LIR_qjoin))
		{
			
			
			

			
			
			
			

			if (avmplus::AvmCore::use_sse2()) {
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
		    rv = findRegFor(value, avmplus::AvmCore::use_sse2() ? XmmRegs : FpRegs);
		} else {
		    rv = rA->reg;
		}

		if (rmask(rv) & XmmRegs) {
            SSE_STQ(dr, rb, rv);
		} else {
			FSTQ(pop, dr, rb);
		}
#elif defined NANOJIT_AMD64
		


	    Register rb = findRegFor(base, GpRegs);
        Reservation *rV = getresv(value);
        
        if (rV != NULL && rV->reg != UnknownReg) {
            if (rmask(rV->reg) & GpRegs) {
                STQ(rb, dr, rV->reg);
            } else {
                SSE_STQ(dr, rb, rV->reg);
            }
        } else {
            Register rv;
            
            





            if (value->isop(LIR_u2f) 
                || value->isop(LIR_i2f)
                || (value->opcode() >= LIR_fneg && value->opcode() <= LIR_fmul)
                || value->opcode() == LIR_fdiv
                || value->opcode() == LIR_fcall) {
                rv = findRegFor(value, XmmRegs);
                SSE_STQ(dr, rb, rv);
            } else {
                rv = findRegFor(value, GpRegs);
                STQ(rb, dr, rv);
            }
        }
#endif
	}

    


    void Assembler::asm_mmq(Register rd, int dd, Register rs, int ds)
    {
        
        
        
#if defined NANOJIT_IA32
        if (avmplus::AvmCore::use_sse2())
        {
#endif

            Register t = registerAlloc(XmmRegs);
            _allocator.addFree(t);
            SSE_STQ(dd, rd, t);
            SSE_LDQ(t, ds, rs);
#if defined NANOJIT_IA32
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
#endif
    }

	void Assembler::asm_quad(LInsp ins)
	{
#if defined NANOJIT_IA32
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
		if (d)
		{
			const int32_t* p = (const int32_t*) (ins-2);
			STi(FP,d+4,p[1]);
			STi(FP,d,p[0]);
		}
#elif defined NANOJIT_AMD64
		Reservation *rR = getresv(ins);
		int64_t val = *(int64_t *)(ins - 2);

		if (rR->reg != UnknownReg)
		{
			if (rmask(rR->reg) & GpRegs)
			{
				LDQi(rR->reg, val);
			}
			else if (rmask(rR->reg) & XmmRegs)
			{
				if (ins->constvalf() == 0.0)
				{
					SSE_XORPDr(rR->reg, rR->reg);
				}
				else
				{
					
					Register rd = rR->reg;
					Register rs = registerAlloc(GpRegs);
	
					SSE_MOVD(rd, rs);
					LDQi(rs, val);

					_allocator.addFree(rs);
				}
			}
		}
		else
		{
			const int32_t* p = (const int32_t*) (ins-2);
			int dr = disp(rR);
			STi(FP, dr+4, p[1]);
			STi(FP, dr, p[0]);
		}

		freeRsrcOf(ins, false);
#endif
	}
	
	bool Assembler::asm_qlo(LInsp ins, LInsp q)
	{
#if defined NANOJIT_IA32
		if (!avmplus::AvmCore::use_sse2())
		{
			return false;
		}
#endif

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

		return true;
	}

	void Assembler::asm_fneg(LInsp ins)
	{
#if defined NANOJIT_IA32
		if (avmplus::AvmCore::use_sse2())
		{
#endif
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
#if defined NANOJIT_IA32
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
#endif
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
			
			PUSHi(p->constval());
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
#if defined NANOJIT_IA32
        NanoAssert(p->isQuad());
		Register r = findRegFor(p, FpRegs);
		if (rmask(r) & XmmRegs) {
			SSE_STQ(0, SP, r); 
		} else {
			FSTPQ(0, SP);
		}
        SUBi(ESP,8);
		
		
#endif
	}

	void Assembler::asm_fop(LInsp ins)
	{
		LOpcode op = ins->opcode();
#if defined NANOJIT_IA32
		if (avmplus::AvmCore::use_sse2()) 
		{
#endif
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
#if defined NANOJIT_IA32
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
#endif
	}

	void Assembler::asm_i2f(LInsp ins)
	{
		
		Register rr = prepResultReg(ins, FpRegs);
#if defined NANOJIT_IA32
		if (rmask(rr) & XmmRegs) 
		{
#endif

			Register gr = findRegFor(ins->oprnd1(), GpRegs);
			SSE_CVTSI2SD(rr, gr);
#if defined NANOJIT_IA32
		} 
		else 
		{
			int d = findMemFor(ins->oprnd1());
			FILD(d, FP);
		}
#endif
	}

	Register Assembler::asm_prep_fcall(Reservation *rR, LInsp ins)
	{
	 	#if defined NANOJIT_IA32
		if (rR) {
    		Register rr;
			if ((rr=rR->reg) != UnknownReg && (rmask(rr) & XmmRegs))
				evict(rr);
		}
		return prepResultReg(ins, rmask(FST0));
		#elif defined NANOJIT_AMD64
		evict(RAX);
		return prepResultReg(ins, rmask(XMM0));
		#endif
	}

	void Assembler::asm_u2f(LInsp ins)
	{
		
		Register rr = prepResultReg(ins, FpRegs);
#if defined NANOJIT_IA32
		if (rmask(rr) & XmmRegs) 
		{
#endif


			Register gr = registerAlloc(GpRegs);

			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			static const double k_NEGONE = 2147483648.0;
#if defined NANOJIT_IA32
			SSE_ADDSDm(rr, &k_NEGONE);
#elif defined NANOJIT_AMD64
			
			if (_dblNegPtr != NULL)
			{
				underrunProtect(10);
			}
			if (_dblNegPtr == NULL)
			{
				underrunProtect(30);
				uint8_t *base, *begin;
				base = (uint8_t *)((intptr_t)_nIns & ~((intptr_t)NJ_PAGE_SIZE-1));
				base += sizeof(PageHeader) + _pageData;
				begin = base;
				
				if ((uintptr_t)base & 0xF) {
					base = (NIns *)((uintptr_t)base & ~(0xF));
					base += 16;
				}
				_pageData += (int32_t)(base - begin) + sizeof(double);
				_negOnePtr = (NIns *)base;
				*(double *)_negOnePtr = k_NEGONE;
			}
			SSE_ADDSDm(rr, _negOnePtr);
#endif

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
#if defined NANOJIT_IA32
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
#endif
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
        if (avmplus::AvmCore::use_sse2() && c != LIR_feq) {
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
                if (branchOnFalse) { JNA(targ); } else { JA(targ); }
            }
            else { 
                if (branchOnFalse) { JNAE(targ); } else { JAE(targ); }
            }
            NIns *at = _nIns;
            Reservation *rA, *rB;
            findRegFor2(XmmRegs, lhs, rA, rhs, rB);
            SSE_UCOMISD(rA->reg, rB->reg);
            return at;
        }

    	if (branchOnFalse)
			JP(targ);
		else
			JNP(targ);
		NIns *at = _nIns;
		asm_fcmp(cond);
        return at;
    }

    void Assembler::asm_setcc(Register r, LIns *cond)
    {
        LOpcode c = cond->opcode();
        if (avmplus::AvmCore::use_sse2() && c != LIR_feq) {
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

#if defined NANOJIT_IA32
        if (avmplus::AvmCore::use_sse2())
        {
#endif





            if (condop == LIR_feq && lhs == rhs) {
                
                Register r = findRegFor(lhs, XmmRegs);
                SSE_UCOMISD(r, r);
            } 
            else {
#if defined NANOJIT_IA32
                evict(EAX);
                TEST_AH(mask);
                LAHF();
#elif defined NANOJIT_AMD64
                evict(RAX);
                TEST_AL(mask);
                POPr(RAX);
                PUSHFQ();
#endif
                Reservation *rA, *rB;
                findRegFor2(XmmRegs, lhs, rA, rhs, rB);
                SSE_UCOMISD(rA->reg, rB->reg);
            }
#if defined NANOJIT_IA32
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
#endif
	}
	
	NIns* Assembler::asm_adjustBranch(NIns* at, NIns* target)
	{
        NIns* was;
#if defined NANOJIT_AMD64
		was = (NIns*)( *(intptr_t*)(at) );
        *(intptr_t *)(at) = intptr_t(target);
#else
		NIns* save = _nIns;
		was = (NIns*)( (intptr_t)*(int32_t*)(at+1)+(intptr_t)(at+5) );
		_nIns = at +5; 
		intptr_t tt = (intptr_t)target - (intptr_t)_nIns;
		IMM32(tt);
		*(--_nIns) = JMPc;
        _nIns = save;
#endif
		return was;
	}
	
	void Assembler::nativePageReset()
	{
#if defined NANOJIT_AMD64
        


		_pageData = 0;
		_dblNegPtr = NULL;
		_negOnePtr = NULL;
#endif
	}

	Register Assembler::asm_binop_rhs_reg(LInsp ins)
	{
		LOpcode op = ins->opcode();
		LIns *rhs = ins->oprnd2();

		if (op == LIR_lsh || op == LIR_rsh || op == LIR_ush) {
#if defined NANOJIT_IA32 
			return findSpecificRegFor(rhs, ECX);
#elif defined NANOJIT_AMD64
			return findSpecificRegFor(rhs, RCX);
#endif
		}

		return UnknownReg;	
	}

#if defined NANOJIT_AMD64
    void Assembler::asm_qbinop(LIns *ins)
    {
        LInsp lhs = ins->oprnd1();
        LInsp rhs = ins->oprnd2();
        LOpcode op = ins->opcode();

        Register rr = prepResultReg(ins, GpRegs);
        Reservation *rA = getresv(lhs);
        Register ra;

        if (rA == NULL || (ra = rA->reg) == UnknownReg) {
            ra = findSpecificRegFor(lhs, rr);
        }

        if (rhs->isconst())
        {
            int c = rhs->constval();

            if (op == LIR_qiadd)
            {
                ADDQi(rr, c);
            } else if (op == LIR_qiand) {
                ANDQi(rr, c);
            } else if (op == LIR_qilsh) {
                SHLQi(rr, c);
            } else if (op == LIR_qior) {
                ORQi(rr, c);
            }
        } else {
            Register rv;

            if (lhs == rhs) {
                rv = ra;
            } else {
                rv = findRegFor(rhs, GpRegs & ~(rmask(rr)));
            }

            if (op == LIR_qiadd) {
                ADDQ(rr, rv);
            } else if (op == LIR_qiand) {
                ANDQ(rr, rv); 
            } else if (op == LIR_qior) {
                ORQ(rr, rv);
            } else {
                NanoAssert(rhs->isconst());
            }
        }

        if (rr != ra) {
            MR(rr, ra);
        }
    }
#endif

	void Assembler::nativePageSetup()
	{
		if (!_nIns)		 _nIns	   = pageAlloc();
		if (!_nExitIns)  _nExitIns = pageAlloc(true);
	}
	
	
    void Assembler::underrunProtect(int n)
    {
        NIns *eip = this->_nIns;
        Page *p = (Page*)pageTop(eip-1);
        NIns *top = (NIns*) &p->code[0];
        if (eip - n < top) {
			_nIns = pageAlloc(_inExit);
            JMP(eip);
        }
    }

	
	#endif 
}
