










#include "xptcprivate.h"

#include "mozilla/StandardInteger.h"

extern "C" uint32_t
invoke_count_words(uint32_t paramCount, nsXPTCVariant* s)
{
    
    
    uint32_t result = 1;
    for (uint32_t i = 0; i < paramCount; i++, result++, s++)
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
    return (result + 1) & ~(uint32_t)1;
}

extern "C" void
invoke_copy_to_stack(uint32_t* d, uint32_t paramCount,
                     nsXPTCVariant* s)
{
    
    d++;

    for (uint32_t i = 0; i < paramCount; i++, d++, s++)
    {
        if (s->IsPtrData())
        {
            *((void**)d) = s->ptr;
            continue;
        }

        switch(s->type)
        {
        case nsXPTType::T_I8     : *d = (uint32_t) s->val.i8;   break;
        case nsXPTType::T_I16    : *d = (uint32_t) s->val.i16;  break;
        case nsXPTType::T_I32    : *d = (uint32_t) s->val.i32;  break;
        case nsXPTType::T_I64    :
            if ((intptr_t)d & 4) d++;
            *((int64_t*) d)  = s->val.i64;    d++;
            break;
        case nsXPTType::T_U8     : *d = (uint32_t) s->val.u8;   break;
        case nsXPTType::T_U16    : *d = (uint32_t) s->val.u16;  break;
        case nsXPTType::T_U32    : *d = (uint32_t) s->val.u32;  break;
        case nsXPTType::T_U64    :
            if ((intptr_t)d & 4) d++;
            *((uint64_t*) d) = s->val.u64;    d++;
            break;
        case nsXPTType::T_FLOAT  : *((float*)   d) = s->val.f;  break;
        case nsXPTType::T_DOUBLE :
            if ((intptr_t)d & 4) d++;
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

extern "C" nsresult _NS_InvokeByIndex(nsISupports* that, uint32_t methodIndex,
                                        uint32_t paramCount,
                                        nsXPTCVariant* params);

EXPORT_XPCOM_API(nsresult)
NS_InvokeByIndex(nsISupports* that, uint32_t methodIndex,
                   uint32_t paramCount, nsXPTCVariant* params)
{
    return _NS_InvokeByIndex(that, methodIndex, paramCount, params);
}
