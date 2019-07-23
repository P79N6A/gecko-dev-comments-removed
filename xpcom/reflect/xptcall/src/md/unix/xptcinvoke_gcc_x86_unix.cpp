






































#ifdef __GNUC__            

#include "xptcprivate.h"
#include "xptc_platforms_unixish_x86.h"
#include "xptc_gcc_x86_unix.h"

extern "C" {
#ifndef XP_WIN32
static
#endif
void ATTRIBUTE_USED __attribute__ ((regparm(3)))
invoke_copy_to_stack(PRUint32 paramCount, nsXPTCVariant* s, PRUint32* d)
{
    for(PRUint32 i = paramCount; i >0; i--, d++, s++)
    {
        if(s->IsPtrData())
        {
            *((void**)d) = s->ptr;
            continue;
        }

        switch(s->type)
        {
        case nsXPTType::T_I64    : *((PRInt64*) d) = s->val.i64; d++;    break;
        case nsXPTType::T_U64    : *((PRUint64*)d) = s->val.u64; d++;    break;
        case nsXPTType::T_DOUBLE : *((double*)  d) = s->val.d;   d++;    break;
        default                  : *((void**)d)    = s->val.p;           break;
        }
    }
}
} 


#if (__GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ == 0))
PRUint32
xptc_invoke_copy_to_stack_keeper (void)
{
    PRUint32 dummy1;
    void ATTRIBUTE_USED __attribute__ ((regparm(3))) (*dummy2)
	(PRUint32, nsXPTCVariant*, PRUint32*) = invoke_copy_to_stack;


    __asm__ __volatile__ (
	""
	: "=&a" (dummy1)
	: "g"   (dummy2)
    );

    return dummy1 & 0xF0F00000;
}
#endif



























#if defined(XP_WIN32) || defined(XP_OS2)
extern "C" {
    nsresult _NS_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                               PRUint32 paramCount, nsXPTCVariant* params);
    EXPORT_XPCOM_API(nsresult)
    NS_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                     PRUint32 paramCount, nsXPTCVariant* params) { 
        return _NS_InvokeByIndex(that, methodIndex, paramCount, params);
    }
}
#endif

__asm__ (
	".text\n\t"


	".align 2\n\t"
#if defined(XP_WIN32) || defined(XP_OS2)
	".globl " SYMBOL_UNDERSCORE "_NS_InvokeByIndex\n\t"
	SYMBOL_UNDERSCORE "_NS_InvokeByIndex:\n\t"
#else
	".globl " SYMBOL_UNDERSCORE "NS_InvokeByIndex\n\t"
	".type  " SYMBOL_UNDERSCORE "NS_InvokeByIndex,@function\n"
	SYMBOL_UNDERSCORE "NS_InvokeByIndex:\n\t"
#endif
	"pushl %ebp\n\t"
	"movl  %esp, %ebp\n\t"
#ifdef MOZ_PRESERVE_PIC 
	"pushl %ebx\n\t"
	"call  0f\n\t"
	".subsection 1\n"
	"0:\n\t"
	"movl (%esp), %ebx\n\t"
	"ret\n\t"
	".previous\n\t"
	"addl  $_GLOBAL_OFFSET_TABLE_, %ebx\n\t"
#endif
	"movl  0x10(%ebp), %eax\n\t"
	"leal  0(,%eax,8),%edx\n\t"
	"movl  %esp, %ecx\n\t"
	"subl  %edx, %ecx\n\t"





	"movl  %ecx, %esp\n\t"          
	"movl  0x14(%ebp), %edx\n\t"
	"call  " SYMBOL_UNDERSCORE "invoke_copy_to_stack\n\t"
	"movl  0x08(%ebp), %ecx\n\t"	
#ifdef CFRONT_STYLE_THIS_ADJUST
	"movl  (%ecx), %edx\n\t"
	"movl  0x0c(%ebp), %eax\n\t"    
	"shll  $3, %eax\n\t"	        
	"addl  $8, %eax\n\t"	        
	"addl  %eax, %edx\n\t"
	"movswl (%edx), %eax\n\t"       
	"addl  %eax, %ecx\n\t"
	"pushl %ecx\n\t"
	"addl  $4, %edx\n\t"	        
#else 
	"pushl %ecx\n\t"
	"movl  (%ecx), %edx\n\t"
	"movl  0x0c(%ebp), %eax\n\t"    
#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100 
	"leal  (%edx,%eax,4), %edx\n\t"
#else 
	"leal  8(%edx,%eax,4), %edx\n\t"
#endif 
#endif
	"call  *(%edx)\n\t"
#ifdef MOZ_PRESERVE_PIC
	"movl  -4(%ebp), %ebx\n\t"
#endif
	"movl  %ebp, %esp\n\t"
	"popl  %ebp\n\t"
	"ret\n"
#if !defined(XP_WIN32) && !defined(XP_OS2)
	".size " SYMBOL_UNDERSCORE "NS_InvokeByIndex, . -" SYMBOL_UNDERSCORE "NS_InvokeByIndex\n\t"
#endif
);

#else
#error "can't find a compiler to use"
#endif 
