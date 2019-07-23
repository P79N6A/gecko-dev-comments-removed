






































#include "nanojit.h"

namespace nanojit
{
    #ifdef FEATURE_NANOJIT

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

            if (ins->isop(LIR_param) && ins->paramKind()==1 && r == Assembler::savedRegs[ins->paramArg()]) {
                
                continue;
            }

            s += VMPI_strlen(s);
            const char* rname = ins->isQuad() ? fpn(r) : gpn(r);
            VMPI_sprintf(s, " %s(%s)", rname, names->formatRef(ins));
        }
    }
    #endif 

    #ifdef _DEBUG

    uint32_t RegAlloc::countActive()
    {
        int cnt = 0;
        for(Register i=FirstReg; i <= LastReg; i = nextreg(i))
            cnt += active[i] ? 1 : 0;
        return cnt;
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
