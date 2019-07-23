





































#include "nanojit.h"

namespace nanojit
{	
	#ifdef FEATURE_NANOJIT

	


	void RegAlloc::clear()
	{
		free = 0;
		used = 0;
		memset(active, 0, NJ_MAX_REGISTERS * sizeof(LIns*));
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

	void RegAlloc::removeFree(Register r)
	{
		NanoAssert(isFree(r));
		free &= ~rmask(r);
	}

	void RegAlloc::addActive(Register r, LIns* v)
	{
		
		NanoAssert(v && r != UnknownReg && active[r] == NULL );
		active[r] = v;
	}

	void RegAlloc::removeActive(Register r)
	{
		
		NanoAssert(r != UnknownReg);
		NanoAssert(active[r] != NULL);

		
		active[r] = NULL;
	}

	LIns* RegAlloc::getActive(Register r)
	{
		NanoAssert(r != UnknownReg);
		return active[r];
	}

	void RegAlloc::retire(Register r)
	{
		NanoAssert(r != UnknownReg);
		NanoAssert(active[r] != NULL);
		active[r] = NULL;
		free |= rmask(r);
	}

	
	LIns* Assembler::findVictim(RegAlloc &regs, RegisterMask allow, RegisterMask prefer)
	{
		NanoAssert(allow != 0 && (allow&prefer)==prefer);
		LIns *i, *a=0, *p = 0;
        int acost=10, pcost=10;
		for (Register r=FirstReg; r <= LastReg; r = nextreg(r))
		{
            if ((allow & rmask(r)) && (i = regs.getActive(r)) != 0)
            {
                int cost = getresv(i)->cost;
                if (!a || cost < acost || cost == acost && nbr(i) < nbr(a)) {
                    a = i;
                    acost = cost;
                }
                if (prefer & rmask(r)) {
                    if (!p || cost < pcost || cost == pcost && nbr(i) < nbr(p)) {
                        p = i;
                        pcost = cost;
                    }
                }
			}
		}
        return acost < pcost ? a : p;
	}

	#ifdef  NJ_VERBOSE
	 void RegAlloc::formatRegisters(RegAlloc& regs, char* s, Fragment *frag)
	{
		if (!frag || !frag->lirbuf)
			return;
		LirNameMap *names = frag->lirbuf->names;
		for(int i=0; i<NJ_MAX_REGISTERS; i++)
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
