







































#include "xptcprivate.h"
#include "xptc_platforms_unixish_x86.h"

extern "C" {



PRUint32
invoke_count_words(PRUint32 paramCount, nsXPTCVariant* s)
{
    PRUint32 result = 0;
    for(PRUint32 i = 0; i < paramCount; i++, s++)
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
invoke_copy_to_stack(PRUint32 paramCount, nsXPTCVariant* s, PRUint32* d)
{
    for(PRUint32 i = 0; i < paramCount; i++, d++, s++)
    {
        if(s->IsPtrData())
        {
            *((void**)d) = s->ptr;
            continue;
        }







        *((void**)d) = s->val.p;

        switch(s->type)
        {
        case nsXPTType::T_I64    : *((PRInt64*) d) = s->val.i64; d++;    break;
        case nsXPTType::T_U64    : *((PRUint64*)d) = s->val.u64; d++;    break;
        case nsXPTType::T_DOUBLE : *((double*)  d) = s->val.d;   d++;    break;
        }
    }
}

}
