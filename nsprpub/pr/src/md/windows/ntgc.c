








































#include <windows.h>
#include "primpl.h"

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np) 
{
#if defined(_X86_)
    CONTEXT context;
    context.ContextFlags = CONTEXT_INTEGER;

    if (_PR_IS_NATIVE_THREAD(t)) {
        context.ContextFlags |= CONTEXT_CONTROL;
        if (GetThreadContext(t->md.handle, &context)) {
            t->md.gcContext[0] = context.Eax;
            t->md.gcContext[1] = context.Ebx;
            t->md.gcContext[2] = context.Ecx;
            t->md.gcContext[3] = context.Edx;
            t->md.gcContext[4] = context.Esi;
            t->md.gcContext[5] = context.Edi;
            t->md.gcContext[6] = context.Esp;
            t->md.gcContext[7] = context.Ebp;
            *np = PR_NUM_GCREGS;
        } else {
            PR_ASSERT(0);
        }
    } else {
        







#if !defined WIN95 
        int *fiberData = t->md.fiber_id;

        



        




        t->md.gcContext[0] = 0;                
        t->md.gcContext[1] = fiberData[0x2e];  
        t->md.gcContext[2] = 0;                
        t->md.gcContext[3] = 0;                
        t->md.gcContext[4] = fiberData[0x2d];  
        t->md.gcContext[5] = fiberData[0x2c];  
        t->md.gcContext[6] = fiberData[0x36];  
        t->md.gcContext[7] = fiberData[0x32];  
        *np = PR_NUM_GCREGS;
#endif
    }
    return (PRWord *)&t->md.gcContext;
#else
    PR_NOT_REACHED("not implemented");
    return NULL;
#endif 
}





void *
GetMyFiberID()
{
#if defined(_X86_) && !defined(__MINGW32__)
    void *fiberData;

    



    __asm {
        mov    EDX, FS:[18h]
        mov    EAX, DWORD PTR [EDX+10h]
        mov    [fiberData], EAX
    }
  
    return fiberData;
#else
    PR_NOT_REACHED("not implemented");
    return NULL;
#endif 
}
