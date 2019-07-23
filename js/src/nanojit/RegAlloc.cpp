





































#include "nanojit.h"

namespace nanojit
{	
	#ifdef FEATURE_NANOJIT

	


	void RegAlloc::clear()
	{
		free = 0;
		used = 0;
		memset(active, 0, (LastReg+1) * sizeof(LIns*));
	}

	bool RegAlloc::isFree(Register r) 
	{
		NanoAssert(r != UnknownReg);
		return (free & rmask(r)) != 0;
	}
		
	void RegAlloc::addFree(Register r)
	{
		NanoAssert(!isFree(r));
		free |= rmask(r);
	}

	void RegAlloc::addActive(Register r, LIns* v)
	{
		
		NanoAssert(v && r != UnknownReg && active[r] == NULL );
		active[r] = v;
        useActive(r);
	}

    void RegAlloc::useActive(Register r)
    {
        NanoAssert(r != UnknownReg && active[r] != NULL);
        usepri[r] = priority++;
    }

	void RegAlloc::removeActive(Register r)
	{
		
		NanoAssert(r != UnknownReg);
		NanoAssert(active[r] != NULL);

		
		active[r] = NULL;
	}

	void RegAlloc::retire(Register r)
	{
		NanoAssert(r != UnknownReg);
		NanoAssert(active[r] != NULL);
		active[r] = NULL;
		free |= rmask(r);
	}

	
    
	LIns* Assembler::findVictim(RegAlloc &regs, RegisterMask allow)
	{
		NanoAssert(allow != 0);
		LIns *i, *a=0;
        int allow_pri = 0x7fffffff;
		for (Register r=FirstReg; r <= LastReg; r = nextreg(r))
		{
            if ((allow & rmask(r)) && (i = regs.getActive(r)) != 0)
            {
                int pri = canRemat(i) ? 0 : regs.getPriority(r);
                if (!a || pri < allow_pri) {
                    a = i;
                    allow_pri = pri;
                }
			}
		}
        NanoAssert(a != 0);
        return a;
	}

	#ifdef  NJ_VERBOSE
	 void RegAlloc::formatRegisters(RegAlloc& regs, char* s, Fragment *frag)
	{
		if (!frag || !frag->lirbuf)
			return;
		LirNameMap *names = frag->lirbuf->names;
		for(int i=0; i<(LastReg+1); i++)
		{
			LIns* ins = regs.active[i];
			Register r = (Register)i;
			if (ins && regs.isFree(r))
				{ NanoAssertMsg( 0, "Coding error; register is both free and active! " ); }
			
			
			if (!ins)
				continue;				

			s += strlen(s);
			const char* rname = ins->isQuad() ? fpn(r) : gpn(r);
			sprintf(s, " %s(%s)", rname, names->formatRef(ins));
		}
	}
	#endif 

	#ifdef _DEBUG

	uint32_t RegAlloc::countFree()
	{
		int cnt = 0;
		for(Register i=FirstReg; i <= LastReg; i = nextreg(i))
			cnt += isFree(i) ? 1 : 0;
		return cnt;
	}

	uint32_t RegAlloc::countActive()
	{
		int cnt = 0;
		for(Register i=FirstReg; i <= LastReg; i = nextreg(i))
			cnt += active[i] ? 1 : 0;
		return cnt;
	}

	void RegAlloc::checkCount()
	{
		NanoAssert(count == (countActive() + countFree()));
	}

    bool RegAlloc::isConsistent(Register r, LIns* i)
    {
		NanoAssert(r != UnknownReg);
        return (isFree(r)  && !getActive(r)     && !i) ||
               (!isFree(r) &&  getActive(r)== i && i );
    }

	#endif 
	#endif 
}
