





































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
	const Register Assembler::savedRegs[] = { R4, R5, R6, R7 };

	void Assembler::nInit(AvmCore*)
	{
		
		has_cmov = false;
	}

	NIns* Assembler::genPrologue(RegisterMask needSaving)
	{
		



		
		
		uint32_t stackNeeded = 4 * _activation.highwatermark + NJ_STACK_OFFSET;
		uint32_t savingCount = 0;

		uint32_t savingMask = 0;
		savingCount = 5; 
		savingMask = 0xF0;
		(void)needSaving;

		
		uint32_t stackPushed = 4 * (2+savingCount);
		uint32_t aligned = alignUp(stackNeeded + stackPushed, NJ_ALIGN_STACK);
		int32_t amt = aligned - stackPushed;

		
		if (amt)
		{
			
			if (amt>508)
			{
				int size = 508;
				while (size>0)
				{
					SUBi(SP, size);
					amt -= size;
					size = amt;
					if (size>508)
						size=508;
				}
			}
			else
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

			BL(_epilogue);

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
		(void)restore;
		if (false) {
			
			BX(R3); 
			POPr(R3); 
			POP_mask(0xF0); 
		} else {
			
			POP_mask(0xF0|rmask(PC));
		}
		MR(R0,R2); 
		return _nIns;
	}
	
	void Assembler::asm_call(LInsp ins)
	{
        const CallInfo* call = ins->callInfo();
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
		
		int i=0;
		while (!(set & rmask((Register)i)))
			i ++;
		_allocator.free &= ~rmask((Register)i);
		return (Register) i;
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

		

		NanoAssert(-(1<<21) <= offset && offset < (1<<21)); 
		*branch++ = (NIns)(0xF000 | (offset>>12)&0x7FF);
		*branch =   (NIns)(0xF800 | (offset>>1)&0x7FF);
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
			prefer = rmask(imm2register(argRegs[i->imm8()]));

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
			_nPool = 0;
			_nSlot = 0;
			_nExitPool = 0;
			_nExitSlot = 0;
	}

	void Assembler::nativePageSetup()
	{
		if (!_nIns)		 _nIns	   = pageAlloc();
		if (!_nExitIns)  _nExitIns = pageAlloc(true);
		
	
		if (!_nPool) {
			_nSlot = _nPool = (int*)_nIns;

			
			
			
			_nPool = (int*)((int)_nIns - (sizeof(int32_t)*NJ_CPOOL_SIZE));
            
			
			_nSlot = _nPool + (NJ_CPOOL_SIZE-1);

			
			_nIns = (NIns*)_nPool;

			
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
		NIns* save = _nIns;
		NIns* was =  (NIns*) (((((*(at+2))&0x7ff)<<12) | (((*(at+1))&0x7ff)<<1)) + (at-2+2));

		_nIns = at + 2;
		BL(target);

		flushCache(_nIns, _nIns+2);

#ifdef AVMPLUS_PORTING_API
		
		NanoJIT_PortAPI_FlushInstructionCache(save, _nIns+2);
#endif
		
		_nIns = save;

		return was;
	}

	void Assembler::STi(Register b, int32_t d, int32_t v)
	{
		ST(b, d, Scratch);
		LDi(Scratch, v);
	}

	bool isB11(NIns *target, NIns *cur)
	{
		NIns *br_base = (cur-1)+2;
		int br_off = int(target) - int(br_base);
		return (-(1<<11) <= br_off && br_off < (1<<11));
	}

	void Assembler::underrunProtect(int bytes)
	{
		
		intptr_t u = bytes + 4;
		if (!samepage(_nIns-u, _nIns-1)) {
			NIns* target = _nIns;
			_nIns = pageAlloc(_inExit);
			
			if (isB11(target, _nIns))
			{
				NIns *br_base = (_nIns-1)+2;
				int br_off = int(target) - int(br_base);
				*(--_nIns) = (NIns)(0xE000 | ((br_off>>1)&0x7FF));
			}
			else
			{
				int offset = int(target)-int(_nIns-2+2);
				*(--_nIns) = (NIns)(0xF800 | ((offset>>1)&0x7FF) );
				*(--_nIns) = (NIns)(0xF000 | ((offset>>12)&0x7FF) );
			}
		}
	}

	bool isB22(NIns *target, NIns *cur)
	{
		int offset = int(target)-int(cur-2+2);
		return (-(1<<22) <= offset && offset < (1<<22));
	}

	void Assembler::BL(NIns* target)
	{
		underrunProtect(4);
		NanoAssert(isB22(target,_nIns));
		int offset = int(target)-int(_nIns-2+2);
		*(--_nIns) = (NIns)(0xF800 | ((offset>>1)&0x7FF) );
		*(--_nIns) = (NIns)(0xF000 | ((offset>>12)&0x7FF) );
		asm_output2("bl %X offset=%d",(int)target, offset);
	}


	void Assembler::B(NIns *target)
	{
		underrunProtect(2);
		NanoAssert(isB11(target,_nIns));
		NIns *br_base = (_nIns-1)+2;
		int br_off = int(target) - int(br_base);
		NanoAssert(-(1<<11) <= br_off && br_off < (1<<11));
		*(--_nIns) = (NIns)(0xE000 | ((br_off>>1)&0x7FF));
		asm_output2("b %X offset=%d", (int)target, br_off);
	}

	void Assembler::JMP(NIns *target)
	{
		underrunProtect(4);
		if (isB11(target,_nIns))
			B(target);
		else
			BL(target);
	}

	void Assembler::PUSH_mask(RegisterMask mask)
	{
		NanoAssert((mask&(0xff|rmask(LR)))==mask);
		underrunProtect(2);
		if (mask & rmask(LR)) {
			mask &= ~rmask(LR);
			mask |= rmask(R8);
		}
		*(--_nIns) = (NIns)(0xB400 | mask);
		asm_output1("push {%x}", mask);
	}

	void Assembler::POPr(Register r)
	{
		underrunProtect(2);
		NanoAssert(((unsigned)r)<8 || r == PC);
		if (r == PC)
			r = R8;
		*(--_nIns) = (NIns)(0xBC00 | (1<<(r)));
		asm_output1("pop {%s}",gpn(r));
	}

	void Assembler::POP_mask(RegisterMask mask)
	{
		NanoAssert((mask&(0xff|rmask(PC)))==mask);
		underrunProtect(2);
		if (mask & rmask(PC)) {
			mask &= ~rmask(PC);
			mask |= rmask(R8);
		}
		*(--_nIns) = (NIns)(0xBC00 | mask);
		asm_output1("pop {%x}", mask);
	}

	void Assembler::MOVi(Register r, int32_t v)
	{
		NanoAssert(isU8(v));
		underrunProtect(2);
		*(--_nIns) = (NIns)(0x2000 | r<<8 | v);
		asm_output2("mov %s,#%d",gpn(r),v);
	}

	void Assembler::LDi(Register r, int32_t v)
	{
		if (isU8(v)) {
			MOVi(r,v);
		} else if (isU8(-v)) {
			NEG(r);
			MOVi(r,-v);
		} else {
			underrunProtect(2);
			LD32_nochk(r, v);
		}
	}

	void Assembler::B_cond(int c, NIns *target)
	{
		#ifdef NJ_VERBOSE
		static const char *ccname[] = { "eq","ne","hs","lo","mi","pl","vs","vc","hi","ls","ge","lt","gt","le","al","nv" };
		#endif

		underrunProtect(6);
		int tt = int(target) - int(_nIns-1+2);
		if (tt < (1<<8) && tt >= -(1<<8))	{
			*(--_nIns) = (NIns)(0xD000 | ((c)<<8) | (tt>>1)&0xFF );
			asm_output3("b%s %X offset=%d", ccname[c], target, tt);
		} else {
			NIns *skip = _nIns;
			BL(target);
			c ^= 1;
			*(--_nIns) = (NIns)(0xD000 | c<<8 | 1 );
			asm_output2("b%s %X", ccname[c], skip);
		}
	}

	void Assembler::STR_sp(int32_t offset, Register reg)
	{
		NanoAssert((offset&3)==0);
		int32_t off = offset>>2;
		NanoAssert(isU8(off));
		underrunProtect(2);
		*(--_nIns) = (NIns)(0x9000 | ((reg)<<8) | off );
		asm_output3("str %s, %d(%s)", gpn(reg), offset, gpn(SP));
	}

	void Assembler::STR_index(Register base, Register off, Register reg)
	{
		underrunProtect(2);
		*(--_nIns) = (NIns)(0x5000 | (off<<6) | (base<<3) | (reg));
		asm_output3("str %s,(%s+%s)",gpn(reg),gpn(base),gpn(off));
	}

	void Assembler::STR_m(Register base, int32_t offset, Register reg)
	{
		NanoAssert(offset >= 0 && offset < 128 && (offset&3)==0);
		underrunProtect(2);
		int32_t off = offset>>2;
		*(--_nIns) = (NIns)(0x6000 | off<<6 | base<<3 | reg);
		asm_output3("str %s,%d(%s)", gpn(reg), offset, gpn(base)); 
	}

	void Assembler::LDMIA(Register base, RegisterMask regs)
	{
		underrunProtect(2);
		NanoAssert((regs&rmask(base))==0 && isU8(regs));
		*(--_nIns) = (NIns)(0xC800 | base<<8 | regs);
		asm_output2("ldmia %s!,{%x}", gpn(base), regs);
	}

	void Assembler::STMIA(Register base, RegisterMask regs)
	{
		underrunProtect(2);
		NanoAssert((regs&rmask(base))==0 && isU8(regs));
		*(--_nIns) = (NIns)(0xC000 | base<<8 | regs);
		asm_output2("stmia %s!,{%x}", gpn(base), regs);
	}

	void Assembler::ST(Register base, int32_t offset, Register reg)
	{
		NanoAssert((offset&3)==0);
		int off = offset>>2;
		if (base==SP) {
			STR_sp(offset, reg);
		} else if ((offset)<0) {										
			STR_index(base, Scratch, reg);
			NEG(Scratch);
			if (offset < -255) {
				NanoAssert(offset >= -1020);
				SHLi(Scratch, 2);
				MOVi(Scratch, -off);
			}
			else {
				MOVi(Scratch, -offset);
			}
		} else {
			underrunProtect(6);
			if (off<32) {
				STR_m(base, offset, reg);
			}
			else {
				STR_index(base, Scratch, reg);
				if (offset > 255) {
					SHLi(Scratch, 2);
					MOVi(Scratch, off);
				}
				else {
					MOVi(Scratch, offset);
				}
			}
		}
	}

	void Assembler::ADDi8(Register r, int32_t i)
	{
		underrunProtect(2);
		NanoAssert(isU8(i));
		*(--_nIns) = (NIns)(0x3000 | r<<8 | i);
		asm_output2("add %s,#%d", gpn(r), i);
	}

	void Assembler::ADDi(Register r, int32_t i)
	{
		if (i < 0 && i != 0x80000000) {
			SUBi(r, -i);
		}
		else if (r == SP) {
			NanoAssert((i&3)==0 && i >= 0 && i < (1<<9));
			underrunProtect(2);
			*(--_nIns) = (NIns)(0xB000 | i>>2);
			asm_output2("add %s,#%d", gpn(SP), i);
		}
		else if (isU8(i)) {
			ADDi8(r,i);
		}
		else if (i >= 0 && i <= (255+255)) {
			ADDi8(r,i-255);
			ADDi8(r,255);
		}
		else {
			ADD(r, Scratch);
			LDi(Scratch, i);
		}
	}

	void Assembler::SUBi8(Register r, int32_t i)
	{
		underrunProtect(2);
		NanoAssert(isU8(i));
		*(--_nIns) = (NIns)(0x3800 | r<<8 | i);
		asm_output2("sub %s,#%d", gpn(r), i);
	}

	void Assembler::SUBi(Register r, int32_t i)
	{
		if (i < 0 && i != 0x80000000) {
			ADDi(r, -i);
		}
		else if (r == SP) {
			NanoAssert((i&3)==0 && i >= 0 && i < (1<<9));
			underrunProtect(2);
			*(--_nIns) = (NIns)(0xB080 | i>>2);
			asm_output2("sub %s,#%d", gpn(SP), i);
		}
		else if (isU8(i)) {
			SUBi8(r,i);
		}
		else if (i >= 0 && i <= (255+255)) {
			SUBi8(r,i-255);
			SUBi8(r,255);
		}
		else {
			SUB(r, Scratch);
			LDi(Scratch, i);
		}
	}

	void Assembler::CALL(const CallInfo *ci)
	{
        intptr_t addr = ci->_address;
		if (isB22((NIns*)addr, _nIns)) {
			int offset = int(addr)-int(_nIns-2+2);
			*(--_nIns) = (NIns)(0xF800 | ((offset>>1)&0x7FF) );
			*(--_nIns) = (NIns)(0xF000 | ((offset>>12)&0x7FF) );
			asm_output2("call %08X:%s", addr, ci->_name);
		}
		else
		{
			underrunProtect(2*(10));
		
			if ( (((int(_nIns))&0xFFFF)%4) != 0)
				 *(--_nIns) = (NIns)0;

			*(--_nIns) = (NIns)(0xF800 | (((-14)&0xFFF)>>1) );
			*(--_nIns) = (NIns)(0xF000 | (((-14)>>12)&0x7FF) );

			*(--_nIns) = (NIns)(0x4600 | (1<<7) | (Scratch<<3) | (IP&7));
			*(--_nIns) = (NIns)0;
			*(--_nIns) = (short)((addr) >> 16);
			*(--_nIns) = (short)((addr) & 0xFFFF);
			*(--_nIns) = (NIns)(0x4700 | (IP<<3));
			*(--_nIns) = (NIns)(0xE000 | (4>>1));
			*(--_nIns) = (NIns)(0x4800 | (Scratch<<8) | (1));
			asm_output2("call %08X:%s", addr, ci->_name);
		}
	}

	void Assembler::LD32_nochk(Register r, int32_t imm)
	{
		
		int offset = (int)(_nSlot) - (int)(_nIns);
		if ((offset>=NJ_MAX_CPOOL_OFFSET || offset<0) ||
			(_nSlot < _nPool))
		{
			
			
			
			
			underrunProtect(sizeof(int32_t)*NJ_CPOOL_SIZE+1);

			NIns* skip = _nIns;

			_nPool = (int*)(((int)_nIns - (sizeof(int32_t)*NJ_CPOOL_SIZE)) &~3);
			_nSlot = _nPool + (NJ_CPOOL_SIZE-1);
			_nIns = (NIns*)_nPool;

			
			B(skip);
			
		}

		*(_nSlot--) = (int)imm;

		NIns *data = (NIns*)(_nSlot+1);;

		int data_off = int(data) - (int(_nIns+1)&~3);
		*(--_nIns) = (NIns)(0x4800 | r<<8 | data_off>>2);
		asm_output3("ldr %s,%d(PC) [%X]",gpn(r),data_off,(int)data);
	}
    #endif 
}
