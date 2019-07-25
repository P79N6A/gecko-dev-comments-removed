






#include "xptcprivate.h"

extern "C" {
void __attribute__ ((__used__)) __attribute__ ((regparm(3)))
invoke_copy_to_stack(uint32_t paramCount, nsXPTCVariant* s, uint32_t* d)
{
    for(uint32_t i = paramCount; i >0; i--, d++, s++)
    {
        if(s->IsPtrData())
        {
            *((void**)d) = s->ptr;
            continue;
        }

        switch(s->type)
        {
        case nsXPTType::T_I8     : *((int8_t*)  d) = s->val.i8;          break;
        case nsXPTType::T_I16    : *((int16_t*) d) = s->val.i16;         break;
        case nsXPTType::T_I32    : *((int32_t*) d) = s->val.i32;         break;
        case nsXPTType::T_I64    : *((int64_t*) d) = s->val.i64; d++;    break;
        case nsXPTType::T_U8     : *((uint8_t*) d) = s->val.u8;          break;
        case nsXPTType::T_U16    : *((uint16_t*)d) = s->val.u16;         break;
        case nsXPTType::T_U32    : *((uint32_t*)d) = s->val.u32;         break;
        case nsXPTType::T_U64    : *((uint64_t*)d) = s->val.u64; d++;    break;
        case nsXPTType::T_FLOAT  : *((float*)   d) = s->val.f;           break;
        case nsXPTType::T_DOUBLE : *((double*)  d) = s->val.d;   d++;    break;
        case nsXPTType::T_BOOL   : *((bool*)    d) = s->val.b;           break;
        case nsXPTType::T_CHAR   : *((char*)    d) = s->val.c;           break;
        case nsXPTType::T_WCHAR  : *((wchar_t*) d) = s->val.wc;          break;
        default:
            
            *((void**)d) = s->val.p;
            break;
        }
    }
}
} 

















__asm__ (
	".text\n\t"


	".align 2\n\t"
	".globl _NS_InvokeByIndex_P\n\t"
	"_NS_InvokeByIndex_P:\n\t"
	"pushl %ebp\n\t"
	"movl  %esp, %ebp\n\t"
	"movl  0x10(%ebp), %eax\n\t"
	"leal  0(,%eax,8),%edx\n\t"

        
	"subl  %edx, %esp\n\t"       




	"andl  $0xfffffff0, %esp\n\t"   







	"subl  $0xc, %esp\n\t"          
	"movl  %esp, %ecx\n\t"          
	"movl  8(%ebp), %edx\n\t"       
	"pushl %edx\n\t"                

	"movl  0x14(%ebp), %edx\n\t"
	"call  _invoke_copy_to_stack\n\t"
	"movl  0x08(%ebp), %ecx\n\t"	
	"movl  (%ecx), %edx\n\t"
	"movl  0x0c(%ebp), %eax\n\t"    
	"leal  (%edx,%eax,4), %edx\n\t"
	"call  *(%edx)\n\t"
	"movl  %ebp, %esp\n\t"
	"popl  %ebp\n\t"
	"ret\n"
	".section .drectve\n\t"
	".ascii \" -export:NS_InvokeByIndex_P\"\n\t"
	".text\n\t"
);
