






































#include "nanojit.h"

namespace nanojit
{
    #ifdef FEATURE_NANOJIT

    


    void RegAlloc::clear()
    {
        VMPI_memset(this, 0, sizeof(*this));
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
        
        NanoAssert(v);
        NanoAssert(r != UnknownReg);
        NanoAssert(active[r] == NULL);
        active[r] = v;
        useActive(r);
    }

    void RegAlloc::useActive(Register r)
    {
        NanoAssert(r != UnknownReg);
        NanoAssert(active[r] != NULL);
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

    #ifdef  NJ_VERBOSE
    void RegAlloc::formatRegisters(char* s, Fragment *frag)
    {
        if (!frag || !frag->lirbuf)
            return;
        LirNameMap *names = frag->lirbuf->names;
        for (Register r = FirstReg; r <= LastReg; r = nextreg(r))
        {
            LIns *ins = getActive(r);
            if (!ins)
                continue;
            NanoAssertMsg(!isFree(r), "Coding error; register is both free and active! " );

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
