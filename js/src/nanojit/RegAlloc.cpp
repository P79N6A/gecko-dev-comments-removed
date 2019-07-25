






































#include "nanojit.h"

namespace nanojit
{
    #ifdef FEATURE_NANOJIT

    #ifdef _DEBUG

    bool RegAlloc::isConsistent(Register r, LIns* i) const
    {
        NanoAssert(r != deprecated_UnknownReg);
        return (isFree(r)  && !getActive(r)     && !i) ||
               (!isFree(r) &&  getActive(r)== i && i );
    }

    #endif 
    #endif 
}
