







#include "xptcprivate.h"

extern "C" {



uint32_t
invoke_count_words(uint32_t paramCount, nsXPTCVariant* s)
{
    uint32_t result = 0;
    for(uint32_t i = 0; i < paramCount; i++, s++)
    {
        if(s->IsPtrData())
        {
            result++;
            continue;
        }
        result++;
        switch(s->type)
        {
        case nsXPTType::T_I64    :
        case nsXPTType::T_U64    :
        case nsXPTType::T_DOUBLE :
            result++;
            break;
        }
    }
    return result;
}

void
invoke_copy_to_stack(uint32_t paramCount, nsXPTCVariant* s, uint32_t* d)
{
    for(uint32_t i = 0; i < paramCount; i++, d++, s++)
    {
        if(s->IsPtrData())
        {
            *((void**)d) = s->ptr;
            continue;
        }







        *((void**)d) = s->val.p;

        switch(s->type)
        {
        case nsXPTType::T_I64    : *((int64_t*) d) = s->val.i64; d++;    break;
        case nsXPTType::T_U64    : *((uint64_t*)d) = s->val.u64; d++;    break;
        case nsXPTType::T_DOUBLE : *((double*)  d) = s->val.d;   d++;    break;
        }
    }
}

}
