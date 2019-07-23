






































#include "xptcprivate.h"

#ifndef WIN32
#error "This code is for Win32 only"
#endif

static void __fastcall
invoke_copy_to_stack(PRUint32* d, PRUint32 paramCount, nsXPTCVariant* s)
{
    for(; paramCount > 0; paramCount--, d++, s++)
    {
        if(s->IsPtrData())
        {
            *((void**)d) = s->ptr;
            continue;
        }
        switch(s->type)
        {
        case nsXPTType::T_I8     : *((PRInt8*)  d) = s->val.i8;          break;
        case nsXPTType::T_I16    : *((PRInt16*) d) = s->val.i16;         break;
        case nsXPTType::T_I32    : *((PRInt32*) d) = s->val.i32;         break;
        case nsXPTType::T_I64    : *((PRInt64*) d) = s->val.i64; d++;    break;
        case nsXPTType::T_U8     : *((PRUint8*) d) = s->val.u8;          break;
        case nsXPTType::T_U16    : *((PRUint16*)d) = s->val.u16;         break;
        case nsXPTType::T_U32    : *((PRUint32*)d) = s->val.u32;         break;
        case nsXPTType::T_U64    : *((PRUint64*)d) = s->val.u64; d++;    break;
        case nsXPTType::T_FLOAT  : *((float*)   d) = s->val.f;           break;
        case nsXPTType::T_DOUBLE : *((double*)  d) = s->val.d;   d++;    break;
        case nsXPTType::T_BOOL   : *((PRBool*)  d) = s->val.b;           break;
        case nsXPTType::T_CHAR   : *((char*)    d) = s->val.c;           break;
        case nsXPTType::T_WCHAR  : *((wchar_t*) d) = s->val.wc;          break;
        default:
            
            *((void**)d) = s->val.p;
            break;
        }
    }
}

#pragma warning(disable : 4035) // OK to have no return value


#pragma optimize( "y", off )
extern "C" NS_EXPORT __declspec(naked) nsresult NS_FROZENCALL
NS_InvokeByIndex_P(nsISupports* that, PRUint32 methodIndex,
                 PRUint32 paramCount, nsXPTCVariant* params)
{
    __asm {
        push    ebp
        mov     ebp,esp
        mov     edx,paramCount      
        test    edx,edx             
        jz      noparams
        mov     eax,edx             
        shl     eax,3               
        sub     esp,eax             
        mov     ecx,esp
        push    params
        call    invoke_copy_to_stack 
noparams:
        mov     ecx,that            
        push    ecx                 
        mov     edx,[ecx]           
        mov     eax,methodIndex
        call    [edx][eax*4]        
        mov     esp,ebp
        pop     ebp
        ret
    }
}
#pragma warning(default : 4035) // restore default
