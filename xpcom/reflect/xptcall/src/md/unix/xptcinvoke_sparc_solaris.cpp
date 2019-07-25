






































#include "xptcprivate.h"




#if !defined(__sparc) && !defined(__sparc__)
#error "This code is for Sparc only"
#endif

typedef unsigned nsXPCVariant;

extern "C" PRUint32
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
        switch(s->type)
        {
        case nsXPTType::T_I8     :
        case nsXPTType::T_I16    :
        case nsXPTType::T_I32    :
            result++;
            break;
        case nsXPTType::T_I64    :
            result+=2;
            break;
        case nsXPTType::T_U8     :
        case nsXPTType::T_U16    :
        case nsXPTType::T_U32    :
            result++;
            break;
        case nsXPTType::T_U64    :
            result+=2;
            break;
        case nsXPTType::T_FLOAT  :
            result++;
            break;
        case nsXPTType::T_DOUBLE :
            result+=2;
            break;
        case nsXPTType::T_BOOL   :
        case nsXPTType::T_CHAR   :
        case nsXPTType::T_WCHAR  :
            result++;
            break;
        default:
            
            result++;
            break;
        }
    }
    
    
    if (result & 0x3) result += 4 - (result & 0x3);     
    return result;
}

extern "C" PRUint32
invoke_copy_to_stack(PRUint32* d, PRUint32 paramCount, nsXPTCVariant* s)
{





    uint32 *l_d = d;
    nsXPTCVariant *l_s = s;
    uint32 l_paramCount = paramCount;
    uint32 regCount = 0;	

    typedef struct {
        uint32 hi;
        uint32 lo;
    } DU;               
                        

    for(uint32 i = 0; i < l_paramCount; i++, l_d++, l_s++)
    {
	if (regCount < 5) regCount++;
        if(l_s->IsPtrData())
        {
            *((void**)l_d) = l_s->ptr;
            continue;
        }
        switch(l_s->type)
        {
        case nsXPTType::T_I8     : *((int32*)   l_d) = l_s->val.i8;          break;
        case nsXPTType::T_I16    : *((int32*)  l_d) = l_s->val.i16;         break;
        case nsXPTType::T_I32    : *((int32*)  l_d) = l_s->val.i32;         break;
        case nsXPTType::T_I64    : 
        case nsXPTType::T_U64    : 
        case nsXPTType::T_DOUBLE : *((uint32*) l_d++) = ((DU *)l_s)->hi;
				   if (regCount < 5) regCount++;
                                   *((uint32*) l_d) = ((DU *)l_s)->lo;
                                   break;
        case nsXPTType::T_U8     : *((uint32*)  l_d) = l_s->val.u8;          break;
        case nsXPTType::T_U16    : *((uint32*) l_d) = l_s->val.u16;         break;
        case nsXPTType::T_U32    : *((uint32*) l_d) = l_s->val.u32;         break;
        case nsXPTType::T_FLOAT  : *((float*)  l_d) = l_s->val.f;           break;
        case nsXPTType::T_BOOL   : *((PRBool*) l_d) = l_s->val.b;           break;
        case nsXPTType::T_CHAR   : *((uint32*)   l_d) = l_s->val.c;           break;
        case nsXPTType::T_WCHAR  : *((int32*)l_d) = l_s->val.wc;          break;
        default:
            
            *((void**)l_d) = l_s->val.p;
            break;
        }
    }
    return regCount;
}

