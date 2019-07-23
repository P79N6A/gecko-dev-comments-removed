








































#include "primpl.h"

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np) 
{
    CONTEXTRECORD context;
    context.ContextFlags = CONTEXT_INTEGER;

    if (_PR_IS_NATIVE_THREAD(t)) {
        context.ContextFlags |= CONTEXT_CONTROL;
        if (QueryThreadContext(t->md.handle, CONTEXT_CONTROL, &context)) {
            t->md.gcContext[0] = context.ctx_RegEax;
            t->md.gcContext[1] = context.ctx_RegEbx;
            t->md.gcContext[2] = context.ctx_RegEcx;
            t->md.gcContext[3] = context.ctx_RegEdx;
            t->md.gcContext[4] = context.ctx_RegEsi;
            t->md.gcContext[5] = context.ctx_RegEdi;
            t->md.gcContext[6] = context.ctx_RegEsp;
            t->md.gcContext[7] = context.ctx_RegEbp;
            *np = PR_NUM_GCREGS;
        } else {
            PR_ASSERT(0);
        }
    }
    return (PRWord *)&t->md.gcContext;
}





void *
GetMyFiberID()
{
    void *fiberData = 0;

    



#ifdef HAVE_ASM
    __asm {
        mov    EDX, FS:[18h]
        mov    EAX, DWORD PTR [EDX+10h]
        mov    [fiberData], EAX
    }
#endif
  
    return fiberData;
}
