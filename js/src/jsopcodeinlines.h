






































#include "jsautooplen.h"

JS_ALWAYS_INLINE jsbytecode *
js_AdvanceOverBlockchain(jsbytecode *pc)
{
    if (*pc == JSOP_NULLBLOCKCHAIN)
        return pc + JSOP_NULLBLOCKCHAIN_LENGTH;
    else if (*pc == JSOP_BLOCKCHAIN)
        return pc + JSOP_BLOCKCHAIN_LENGTH;
    else
        return pc;
}
