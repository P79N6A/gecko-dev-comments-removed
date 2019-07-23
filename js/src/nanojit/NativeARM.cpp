





































#include "nanojit.h"

#ifdef AVMPLUS_PORTING_API
#include "portapi_nanojit.h"
#endif

#ifdef UNDER_CE
#include <cmnintrin.h>
#endif

#if defined(AVMPLUS_LINUX)
#include <asm/unistd.h>
#endif

namespace nanojit
{
	#ifdef FEATURE_NANOJIT

#ifdef NJ_VERBOSE
	const char* regNames[] = {"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","IP","SP","LR","PC"};

#endif
    const Register Assembler::argRegs[] = { R0, R1, R2, R3 };
    const Register Assembler::retRegs[] = { R0, R1 };

	void Assembler::nInit(AvmCore*)
	{
		
		has_cmov = true;
	}

	NIns* Assembler::genPrologue(RegisterMask needSaving)
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
		{ 
			SUBi(SP, amt); 
		}

		verbose_only( verbose_outputf("         %p:",_nIns); )
        verbose_only( verbose_output("         patch entry"); )
        NIns *patchEntry = _nIns;

		MR(FRAME_PTR, SP);
		PUSH_mask(savingMask|rmask(LR));
		return patchEntry;
	}

	void Assembler::nFragExit(LInsp guard)
	{
		SideExit* exit = guard->exit();
		Fragment *frag = exit->target;
		GuardRecord *lr;
		if (frag && frag->fragEntry)
		{
			JMP(frag->fragEntry);
			lr = 0;
		}
		else
		{
			
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

	NIns* Assembler::genEpilogue(RegisterMask restore)
	{
		BX(LR); 
		MR(R0,R2); 
		RegisterMask savingMask = restore | rmask(FRAME_PTR) | rmask(LR);
		POP_mask(savingMask); 
		return _nIns;
	}
	
	void Assembler::asm_call(LInsp ins)
	{
        const CallInfo* call = callInfoFor(ins->fid());
		uint32_t atypes = call->_argtypes;
		uint32_t roffset = 0;

		
		
		
		
		bool arg0IsInt32FollowedByFloat = false;
		while ((atypes & 3) != ARGSIZE_NONE) {
			if (((atypes >> 4) & 3) == ARGSIZE_LO &&
				((atypes >> 2) & 3) == ARGSIZE_F &&
				((atypes >> 6) & 3) == ARGSIZE_NONE)
			{
				arg0IsInt32FollowedByFloat = true;
				break;
			}
			atypes >>= 2;
		}

		CALL(call);
        ArgSize sizes[10];
        uint32_t argc = call->get_sizes(sizes);
		for(uint32_t i=0; i < argc; i++)
		{
            uint32_t j = argc - i - 1;
            ArgSize sz = sizes[j];
            NanoAssert(sz == ARGSIZE_LO || sz == ARGSIZE_Q);
    		
            Register r = (i+roffset) < 4 ? argRegs[i+roffset] : UnknownReg;
            asm_arg(sz, ins->arg(j), r);

			if (i == 0 && arg0IsInt32FollowedByFloat)
				roffset = 1;
		}
	}
	
	void Assembler::nMarkExecute(Page* page, int32_t count, bool enable)
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
			
	Register Assembler::nRegisterAllocFromSet(int set)
	{
		
#ifndef UNDER_CE
#ifdef __ARMCC__
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
#else
		Register r;
		r = (Register)_CountLeadingZeros(set);
		r = (Register)(31-r);
		_allocator.free &= ~rmask(r);
		return r;
#endif

	}

	void Assembler::nRegisterResetAll(RegAlloc& a)
	{
		
		a.clear();
		a.used = 0;
		a.free = rmask(R0) | rmask(R1) | rmask(R2) | rmask(R3) | rmask(R4) | rmask(R5);
		debug_only(a.managed = a.free);
	}

	void Assembler::nPatchBranch(NIns* branch, NIns* target)
	{
		

		
		
		

		
		int32_t offset = int(target) - int(branch+2);

		

		
		
		if (-(1<<24) <= offset & offset < (1<<24)) {
			
			*branch = (NIns)( COND_AL | (0xA<<24) | ((offset >>2) & 0xFFFFFF) );
		} else {
			
			*branch++ = (NIns)( COND_AL | (0x51<<20) | (PC<<16) | (PC<<12) | ( 0x004 ) );
			*branch = (NIns)target;
		}
	}

	RegisterMask Assembler::hint(LIns* i, RegisterMask allow )
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

    void Assembler::asm_qjoin(LIns *ins)
    {
		int d = findMemFor(ins);
		AvmAssert(d);
		LIns* lo = ins->oprnd1();
		LIns* hi = ins->oprnd2();
							
		Register r = findRegFor(hi, GpRegs);
		ST(FP, d+4, r);

        
		r = findRegFor(lo, GpRegs);
		ST(FP, d, r);
        freeRsrcOf(ins, false);	
    }

    void Assembler::asm_store32(LIns *value, int dr, LIns *base)
    {
	    
	    Reservation *rA, *rB;
	    findRegFor2(GpRegs, value, rA, base, rB);
	    Register ra = rA->reg;
	    Register rb = rB->reg;
	    ST(rb, dr, ra);
    }

	void Assembler::asm_restore(LInsp i, Reservation *resv, Register r)
	{
		(void)resv;
        int d = findMemFor(i);
	    LD(r, d, FP);
		verbose_only(if (_verbose) {
			outputf("        restore %s",_thisfrag->lirbuf->names->formatRef(i));
		})
	}

	void Assembler::asm_spill(LInsp i, Reservation *resv, bool pop)
	{
    (void)i;
		(void)pop;
		if (resv->arIndex)
		{
			int d = disp(resv);
			
			Register rr = resv->reg;
			ST(FP, d, rr);
			verbose_only(if (_verbose){
				outputf("        spill %s",_thisfrag->lirbuf->names->formatRef(i));
			})
		}
	}

	void Assembler::asm_load64(LInsp ins)
	{
		LIns* base = ins->oprnd1();
		int db = ins->oprnd2()->constval();
		Reservation *resv = getresv(ins);
		int dr = disp(resv);
		NanoAssert(resv->reg == UnknownReg && dr != 0);

		Register rb = findRegFor(base, GpRegs);
		resv->reg = UnknownReg;
		asm_mmq(FP, dr, rb, db);
		freeRsrcOf(ins, false);
	}

	void Assembler::asm_store64(LInsp value, int dr, LInsp base)
	{
		int da = findMemFor(value);
	    Register rb = findRegFor(base, GpRegs);
	    asm_mmq(rb, dr, FP, da);
	}

	void Assembler::asm_quad(LInsp ins)
	{
		Reservation *rR = getresv(ins);
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
		(void)ins; (void)q;
		return false;
	}

	void Assembler::asm_nongp_copy(Register r, Register s)
	{
		
		(void)r; (void)s;
		NanoAssert(false);
	}

	Register Assembler::asm_binop_rhs_reg(LInsp ins)
	{
		return UnknownReg;
	}

    


    void Assembler::asm_mmq(Register rd, int dd, Register rs, int ds)
    {
        
        
        
		
		Register t = registerAlloc(GpRegs & ~(rmask(rd)|rmask(rs)));
		_allocator.addFree(t);
		ST(rd, dd+4, t);
		LD(t, ds+4, rs);
		ST(rd, dd, t);
		LD(t, ds, rs);
    }

	void Assembler::asm_pusharg(LInsp p)
	{
		
		Reservation* rA = getresv(p);
		if (rA == 0)
		{
			Register ra = findRegFor(p, GpRegs);
			ST(SP,0,ra);
		}
		else if (rA->reg == UnknownReg)
		{
			ST(SP,0,Scratch);
			LD(Scratch,disp(rA),FP);
		}
		else
		{
			ST(SP,0,rA->reg);
		}
	}

	void Assembler::nativePageReset()
	{
			_nSlot = 0;
			_nExitSlot = 0;
	}

	void Assembler::nativePageSetup()
	{
		if (!_nIns)		 _nIns	   = pageAlloc();
		if (!_nExitIns)  _nExitIns = pageAlloc(true);
		
	
		if (!_nSlot)
		{
			
			
			_nIns--;
			_nExitIns--;

			
			
			_nSlot = pageDataStart(_nIns); 
		}
	}

	void Assembler::flushCache(NIns* n1, NIns* n2) {
#if defined(UNDER_CE)
 		
		FlushInstructionCache(GetCurrentProcess(), NULL, NULL);
#elif defined(AVMPLUS_LINUX)
		
		
		register unsigned long _beg __asm("a1") = (unsigned long)(n1);
		register unsigned long _end __asm("a2") = (unsigned long)(n2);
		register unsigned long _flg __asm("a3") = 0;
		register unsigned long _swi __asm("r7") = 0xF0002;
		__asm __volatile ("swi 0 	@ sys_cacheflush" : "=r" (_beg) : "0" (_beg), "r" (_end), "r" (_flg), "r" (_swi));
#endif
	}

	NIns* Assembler::asm_adjustBranch(NIns* at, NIns* target)
	{
		
		
		
		NanoAssert(at[1] == (NIns)( COND_AL | OP_IMM | (1<<23) | (PC<<16) | (LR<<12) | (4) ));
		NanoAssert(at[2] == (NIns)( COND_AL | (0x9<<21) | (0xFFF<<8) | (1<<4) | (IP) ));

		NIns* was = (NIns*) at[3];

		at[3] = (NIns)target;

		flushCache(at, at+4);

#ifdef AVMPLUS_PORTING_API
		NanoJIT_PortAPI_FlushInstructionCache(at, at+4);
#endif

		return was;
	}

	void Assembler::underrunProtect(int bytes)
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

	void Assembler::BL_far(NIns* addr) {
		
		
		underrunProtect(16);

		
		*(--_nIns) = (NIns)((addr));
		
		*(--_nIns) = (NIns)( COND_AL | (0x9<<21) | (0xFFF<<8) | (1<<4) | (IP) );
		
		*(--_nIns) = (NIns)( COND_AL | OP_IMM | (1<<23) | (PC<<16) | (LR<<12) | (4) );
		
		*(--_nIns) = (NIns)( COND_AL | (0x59<<20) | (PC<<16) | (IP<<12) | (4));
		asm_output1("bl %p (32-bit)", addr);
	}

	void Assembler::BL(NIns* addr) {
		intptr_t offs = PC_OFFSET_FROM(addr,(intptr_t)_nIns-4);
		if (JMP_S24_OFFSET_OK(offs)) {
			
			underrunProtect(4);
			*(--_nIns) = (NIns)( COND_AL | (0xB<<24) | (((offs)>>2) & 0xFFFFFF) ); \
			asm_output1("bl %p", addr);
		} else {
			BL_far(addr);
		}
	}

	void Assembler::CALL(const CallInfo *ci)
	{
        intptr_t addr = ci->_address;
		BL((NIns*)addr);
		asm_output1("   (call %s)", ci->_name);
	}

	void Assembler::LD32_nochk(Register r, int32_t imm)
	{
		
		underrunProtect(8);

		*(++_nSlot) = (int)imm;

		

		int offset = PC_OFFSET_FROM(_nSlot,(intptr_t)(_nIns)-4);

		NanoAssert(JMP_S24_OFFSET_OK(offset) && (offset < 0));

		*(--_nIns) = (NIns)( COND_AL | (0x51<<20) | (PC<<16) | ((r)<<12) | ((-offset) & 0xFFFFFF) );
		asm_output2("ld %s,%d",gpn(r),imm);
	}
    #endif 
}
