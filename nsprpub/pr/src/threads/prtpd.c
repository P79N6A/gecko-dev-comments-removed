

































































#ifndef XP_BEOS

#include "primpl.h"

#include <string.h>

#if defined(WIN95)






#pragma warning(disable : 4101)
#endif

#define _PR_TPD_LIMIT 128               /* arbitary limit on the TPD slots */
static PRInt32 _pr_tpd_length = 0;      
static PRInt32 _pr_tpd_highwater = 0;   
static PRThreadPrivateDTOR *_pr_tpd_destructors = NULL;
                                        






void _PR_InitTPD(void)
{
    _pr_tpd_destructors = (PRThreadPrivateDTOR*)
        PR_CALLOC(_PR_TPD_LIMIT * sizeof(PRThreadPrivateDTOR*));
    PR_ASSERT(NULL != _pr_tpd_destructors);
    _pr_tpd_length = _PR_TPD_LIMIT;
}




void _PR_CleanupTPD(void)
{
}  





















PR_IMPLEMENT(PRStatus) PR_NewThreadPrivateIndex(
    PRUintn *newIndex, PRThreadPrivateDTOR dtor)
{
    PRStatus rv;
    PRInt32 index;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    PR_ASSERT(NULL != newIndex);
    PR_ASSERT(NULL != _pr_tpd_destructors);

    index = PR_ATOMIC_INCREMENT(&_pr_tpd_highwater) - 1;  
    if (_PR_TPD_LIMIT <= index)
    {
        PR_SetError(PR_TPD_RANGE_ERROR, 0);
        rv = PR_FAILURE;  
    }
    else
    {
        _pr_tpd_destructors[index] = dtor;  
        *newIndex = (PRUintn)index;  
        rv = PR_SUCCESS;  
    }

    return rv;
}














PR_IMPLEMENT(PRStatus) PR_SetThreadPrivate(PRUintn index, void *priv)
{
    PRThread *self = PR_GetCurrentThread();

    




    if ((index >= _PR_TPD_LIMIT) || (index >= _pr_tpd_highwater))
    {
        PR_SetError(PR_TPD_RANGE_ERROR, 0);
        return PR_FAILURE;
    }

    PR_ASSERT(((NULL == self->privateData) && (0 == self->tpdLength))
        || ((NULL != self->privateData) && (0 != self->tpdLength)));

    if ((NULL == self->privateData) || (self->tpdLength <= index))
    {
        void *extension = PR_CALLOC(_pr_tpd_length * sizeof(void*));
        if (NULL == extension)
        {
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
            return PR_FAILURE;
        }
        if (self->privateData) {
            (void)memcpy(
                extension, self->privateData,
                self->tpdLength * sizeof(void*));
            PR_DELETE(self->privateData);
        }
        self->tpdLength = _pr_tpd_length;
        self->privateData = (void**)extension;
    }
    



    else if (self->privateData[index] && _pr_tpd_destructors[index])
    {
        void *data = self->privateData[index];
        self->privateData[index] = NULL;
        (*_pr_tpd_destructors[index])(data);
    }

    PR_ASSERT(index < self->tpdLength);
    self->privateData[index] = priv;

    return PR_SUCCESS;
}










PR_IMPLEMENT(void*) PR_GetThreadPrivate(PRUintn index)
{
    PRThread *self = PR_GetCurrentThread();
    void *tpd = ((NULL == self->privateData) || (index >= self->tpdLength)) ?
        NULL : self->privateData[index];

    return tpd;
}






void _PR_DestroyThreadPrivate(PRThread* self)
{
#define _PR_TPD_DESTRUCTOR_ITERATIONS 4

    if (NULL != self->privateData)  
    {
        PRBool clean;
        PRUint32 index;
        PRInt32 passes = _PR_TPD_DESTRUCTOR_ITERATIONS;
        PR_ASSERT(0 != self->tpdLength);
        do
        {
            clean = PR_TRUE;
            for (index = 0; index < self->tpdLength; ++index)
            {
                void *priv = self->privateData[index];  
                if (NULL != priv)  
                {
                    if (NULL != _pr_tpd_destructors[index])
                    {
                        self->privateData[index] = NULL;  
                        (*_pr_tpd_destructors[index])(priv);  
                        clean = PR_FALSE;  
                    }
                }
            }
        } while ((--passes > 0) && !clean);  
        




        memset(self->privateData, 0, self->tpdLength * sizeof(void*));
    }
}  

#endif 
