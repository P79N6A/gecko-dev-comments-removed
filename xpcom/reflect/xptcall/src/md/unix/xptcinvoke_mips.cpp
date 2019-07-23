












































#include "xptcprivate.h"


extern "C" uint32
invoke_count_words(PRUint32 paramCount, nsXPTCVariant* s)
{
    
    
    PRUint32 result = 1;
    for (PRUint32 i = 0; i < paramCount; i++, s++)
    {
        result++;

        if (s->IsPtrData())
            continue;

        switch(s->type)
        {
        case nsXPTType::T_I64    :
        case nsXPTType::T_U64    :
        case nsXPTType::T_DOUBLE :
	    if (result & 1)
		result++;
	    result++;
	    break;
        }
    }
    return (result + 1) & ~(PRUint32)1;
}

extern "C" void
invoke_copy_to_stack(PRUint32* d, PRUint32 paramCount,
                     nsXPTCVariant* s)
{
    
    d++;

    for (PRUint32 i = 0; i < paramCount; i++, d++, s++)
    {
        if (s->IsPtrData())
        {
            *((void**)d) = s->ptr;
            continue;
        }

        *((void**)d) = s->val.p;

        switch(s->type)
        {
        case nsXPTType::T_I64    :
            if ((PRWord)d & 4) d++;
            *((PRInt64*) d)  = s->val.i64;    d++;
            break;
        case nsXPTType::T_U64    :
            if ((PRWord)d & 4) d++;
            *((PRUint64*) d) = s->val.u64;    d++;
            break;
        case nsXPTType::T_DOUBLE :
            if ((PRWord)d & 4) d++;
            *((double*)   d) = s->val.d;      d++;
            break;
        }
    }
}

extern "C" nsresult _XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                                        PRUint32 paramCount,
                                        nsXPTCVariant* params);

extern "C"
XPTC_PUBLIC_API(nsresult)
XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{
    return _XPTC_InvokeByIndex(that, methodIndex, paramCount, params);
}    

