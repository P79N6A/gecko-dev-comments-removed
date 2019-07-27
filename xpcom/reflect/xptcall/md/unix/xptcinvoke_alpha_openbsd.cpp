






#include "xptcprivate.h"


static void
invoke_copy_to_stack(uint64_t* d, uint32_t paramCount, nsXPTCVariant* s)
__asm__("invoke_copy_to_stack") __attribute__((used));

static void
invoke_copy_to_stack(uint64_t* d, uint32_t paramCount, nsXPTCVariant* s)
{
    const uint8_t NUM_ARG_REGS = 6-1;        

    for(uint32_t i = 0; i < paramCount; i++, d++, s++)
    {
        if(s->IsPtrData())
        {
            *d = (uint64_t)s->ptr;
            continue;
        }
        switch(s->type)
        {
        case nsXPTType::T_I8     : *d = (uint64_t)s->val.i8;     break;
        case nsXPTType::T_I16    : *d = (uint64_t)s->val.i16;    break;
        case nsXPTType::T_I32    : *d = (uint64_t)s->val.i32;    break;
        case nsXPTType::T_I64    : *d = (uint64_t)s->val.i64;    break;
        case nsXPTType::T_U8     : *d = (uint64_t)s->val.u8;     break;
        case nsXPTType::T_U16    : *d = (uint64_t)s->val.u16;    break;
        case nsXPTType::T_U32    : *d = (uint64_t)s->val.u32;    break;
        case nsXPTType::T_U64    : *d = (uint64_t)s->val.u64;    break;
        case nsXPTType::T_FLOAT  :
            if(i < NUM_ARG_REGS)
            {
                
                
                union { uint64_t u64; double d; } t;
                t.d = (double)s->val.f;
                *d = t.u64;
            }
            else
                
                *d = (uint64_t)s->val.u32;
            break;
        case nsXPTType::T_DOUBLE : *d = (uint64_t)s->val.u64;    break;
        case nsXPTType::T_BOOL   : *d = (uint64_t)s->val.b;      break;
        case nsXPTType::T_CHAR   : *d = (uint64_t)s->val.c;      break;
        case nsXPTType::T_WCHAR  : *d = (uint64_t)s->val.wc;     break;
        default:
            
            *d = (uint64_t)s->val.p;
            break;
        }
    }
}






__asm__(
    "#### NS_InvokeByIndex ####\n"
".text\n\t"
    ".align 5\n\t"
    ".globl NS_InvokeByIndex\n\t"
    ".ent NS_InvokeByIndex\n"
"NS_InvokeByIndex:\n\t"
    ".frame $15,32,$26,0\n\t"
    ".mask 0x4008000,-32\n\t"
    "ldgp $29,0($27)\n"
"$NS_InvokeByIndex..ng:\n\t"
    "subq $30,32,$30\n\t"
    "stq $26,0($30)\n\t"
    "stq $15,8($30)\n\t"
    "bis $30,$30,$15\n\t"
    ".prologue 1\n\t"

    





    "bis $31,5,$2\n\t"      
    "cmplt $2,$18,$1\n\t"
    "cmovne $1,$18,$2\n\t"
    "s8addq $2,16,$1\n\t"   
    "bic $1,15,$1\n\t"      
    "subq $30,$1,$30\n\t"

    "stq $16,0($30)\n\t"    
    "stq $17,16($15)\n\t"   

    "addq $30,8,$16\n\t"    
    "bis $18,$18,$17\n\t"   
    "bis $19,$19,$18\n\t"   
    "bsr $26,$invoke_copy_to_stack..ng\n\t"     

    






    "ldq $16,0($30)\n\t"    
    "ldq $17,8($30)\n\t"
    "ldq $18,16($30)\n\t"
    "ldq $19,24($30)\n\t"
    "ldq $20,32($30)\n\t"
    "ldq $21,40($30)\n\t"
    "ldt $f17,8($30)\n\t"   
    "ldt $f18,16($30)\n\t"
    "ldt $f19,24($30)\n\t"
    "ldt $f20,32($30)\n\t"
    "ldt $f21,40($30)\n\t"

    "addq $30,48,$30\n\t"   

    


    "bis $16,$16,$1\n\t"    
    "ldq $2,16($15)\n\t"    
    "ldq $1,0($1)\n\t"      
    "s8addq $2,$31,$2\n\t"  
    "addq $1,$2,$1\n\t"
    "ldq $27,0($1)\n\t"     
    "jsr $26,($27),0\n\t"   
    "ldgp $29,0($26)\n\t"

    "bis $15,$15,$30\n\t"
    "ldq $26,0($30)\n\t"
    "ldq $15,8($30)\n\t"
    "addq $30,32,$30\n\t"
    "ret $31,($26),1\n\t"
    ".end NS_InvokeByIndex"
    );
