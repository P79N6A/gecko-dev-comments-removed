













































#include "xptcprivate.h"


extern "C" uint32
invoke_count_words(PRUint32 paramCount, nsXPTCVariant* s)
{
    
    
    PRUint32 result = 1;
    for (PRUint32 i = 0; i < paramCount; i++, result++, s++)
    {
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

        default:
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

        switch(s->type)
        {
        case nsXPTType::T_I8     : *d = (PRUint32) s->val.i8;   break;
        case nsXPTType::T_I16    : *d = (PRUint32) s->val.i16;  break;
        case nsXPTType::T_I32    : *d = (PRUint32) s->val.i32;  break;
        case nsXPTType::T_I64    :
            if ((PRWord)d & 4) d++;
            *((PRInt64*) d)  = s->val.i64;    d++;
            break;
        case nsXPTType::T_U8     : *d = (PRUint32) s->val.u8;   break;
        case nsXPTType::T_U16    : *d = (PRUint32) s->val.u16;  break;
        case nsXPTType::T_U32    : *d = (PRUint32) s->val.u32;  break;
        case nsXPTType::T_U64    :
            if ((PRWord)d & 4) d++;
            *((PRUint64*) d) = s->val.u64;    d++;
            break;
        case nsXPTType::T_FLOAT  : *((float*)   d) = s->val.f;  break;
        case nsXPTType::T_DOUBLE :
            if ((PRWord)d & 4) d++;
            *((double*)   d) = s->val.d;      d++;
            break;
        case nsXPTType::T_BOOL   : *d = (bool)  s->val.b;     break;
        case nsXPTType::T_CHAR   : *d = (char)    s->val.c;     break;
        case nsXPTType::T_WCHAR  : *d = (wchar_t) s->val.wc;    break;
        default:
            *((void**)d) = s->val.p;
            break;
        }
    }
}

extern "C" nsresult _NS_InvokeByIndex_P(nsISupports* that, PRUint32 methodIndex,
                                        PRUint32 paramCount,
                                        nsXPTCVariant* params);

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex_P(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{
    return _NS_InvokeByIndex_P(that, methodIndex, paramCount, params);
}
