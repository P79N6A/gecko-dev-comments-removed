






#include "xptcprivate.h"
#include "xptc_gcc_x86_unix.h"

extern "C" {
static void ATTRIBUTE_USED __attribute__ ((regparm(3)))
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
        case nsXPTType::T_I64    : *((int64_t*) d) = s->val.i64; d++;    break;
        case nsXPTType::T_U64    : *((uint64_t*)d) = s->val.u64; d++;    break;
        case nsXPTType::T_DOUBLE : *((double*)  d) = s->val.d;   d++;    break;
        default                  : *((void**)d)    = s->val.p;           break;
        }
    }
}
} 

















__asm__ (
	".text\n\t"


	".align 2\n\t"
	".globl " SYMBOL_UNDERSCORE "NS_InvokeByIndex_P\n\t"
#ifndef XP_MACOSX
	".type  " SYMBOL_UNDERSCORE "NS_InvokeByIndex_P,@function\n"
#endif
	SYMBOL_UNDERSCORE "NS_InvokeByIndex_P:\n\t"
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
	"call  " SYMBOL_UNDERSCORE "invoke_copy_to_stack\n\t"
	"movl  0x08(%ebp), %ecx\n\t"	
	"movl  (%ecx), %edx\n\t"
	"movl  0x0c(%ebp), %eax\n\t"    
	"leal  (%edx,%eax,4), %edx\n\t"
	"call  *(%edx)\n\t"
	"movl  %ebp, %esp\n\t"
	"popl  %ebp\n\t"
	"ret\n"
#ifndef XP_MACOSX
	".size " SYMBOL_UNDERSCORE "NS_InvokeByIndex_P, . -" SYMBOL_UNDERSCORE "NS_InvokeByIndex_P\n\t"
#endif
);
