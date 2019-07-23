








































#include "xptcprivate.h"


extern "C" void
invoke_copy_to_stack(PRUint64* d, PRUint32 paramCount, nsXPTCVariant* s);

extern "C" void
invoke_copy_to_stack(PRUint64* d, PRUint32 paramCount, nsXPTCVariant* s)
{
    const PRUint8 NUM_ARG_REGS = 6-1;        

    for(PRUint32 i = 0; i < paramCount; i++, d++, s++)
    {
        if(s->IsPtrData())
        {
            *d = (PRUint64)s->ptr;
            continue;
        }
        switch(s->type)
        {
        case nsXPTType::T_I8     : *d = (PRUint64)s->val.i8;     break;
        case nsXPTType::T_I16    : *d = (PRUint64)s->val.i16;    break;
        case nsXPTType::T_I32    : *d = (PRUint64)s->val.i32;    break;
        case nsXPTType::T_I64    : *d = (PRUint64)s->val.i64;    break;
        case nsXPTType::T_U8     : *d = (PRUint64)s->val.u8;     break;
        case nsXPTType::T_U16    : *d = (PRUint64)s->val.u16;    break;
        case nsXPTType::T_U32    : *d = (PRUint64)s->val.u32;    break;
        case nsXPTType::T_U64    : *d = (PRUint64)s->val.u64;    break;
        case nsXPTType::T_FLOAT  :
            if(i < NUM_ARG_REGS)
            {
                
                
                union { PRUint64 u64; double d; } t;
                t.d = (double)s->val.f;
                *d = t.u64;
            }
            else
                
                *d = (PRUint64)s->val.u32;
            break;
        case nsXPTType::T_DOUBLE : *d = (PRUint64)s->val.u64;    break;
        case nsXPTType::T_BOOL   : *d = (PRUint64)s->val.b;      break;
        case nsXPTType::T_CHAR   : *d = (PRUint64)s->val.c;      break;
        case nsXPTType::T_WCHAR  : *d = (PRUint64)s->val.wc;     break;
        default:
            
            *d = (PRUint64)s->val.p;
            break;
        }
    }
}
