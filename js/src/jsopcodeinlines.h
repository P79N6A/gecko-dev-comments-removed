





































#include "jsautooplen.h"

namespace js {





JS_ALWAYS_INLINE jsbytecode *
AdvanceOverBlockchainOp(jsbytecode *pc)
{
    if (*pc == JSOP_NULLBLOCKCHAIN)
        return pc + JSOP_NULLBLOCKCHAIN_LENGTH;
    if (*pc == JSOP_BLOCKCHAIN)
        return pc + JSOP_BLOCKCHAIN_LENGTH;
    return pc;
}

}
