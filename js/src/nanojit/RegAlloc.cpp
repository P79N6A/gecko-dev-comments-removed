






































#include "nanojit.h"

namespace nanojit
{
    #ifdef FEATURE_NANOJIT

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
