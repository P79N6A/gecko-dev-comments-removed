




































#include "primpl.h"

#include <string.h>





















typedef struct _PR_Fd_Cache
{
    PRLock *ml;
    PRIntn count;
    PRStack *stack;
    PRFileDesc *head, *tail;
    PRIntn limit_low, limit_high;
} _PR_Fd_Cache;

static _PR_Fd_Cache _pr_fd_cache;
static PRFileDesc **stack2fd = &(((PRFileDesc*)NULL)->higher);






PRFileDesc *_PR_Getfd(void)
{
    PRFileDesc *fd;
    









    if (0 == _pr_fd_cache.limit_high)
    {
        PRStackElem *pop;
        PR_ASSERT(NULL != _pr_fd_cache.stack);
        pop = PR_StackPop(_pr_fd_cache.stack);
        if (NULL == pop) goto allocate;
        fd = (PRFileDesc*)((PRPtrdiff)pop - (PRPtrdiff)stack2fd);
    }
    else
    {
        do
        {
            if (NULL == _pr_fd_cache.head) goto allocate;  
            if (_pr_fd_cache.count < _pr_fd_cache.limit_low) goto allocate;

            
            PR_Lock(_pr_fd_cache.ml);  
            fd = _pr_fd_cache.head;  
            if (NULL == fd)  
            {
                PR_ASSERT(0 == _pr_fd_cache.count);
                PR_ASSERT(NULL == _pr_fd_cache.tail);
            }
            else
            {
                _pr_fd_cache.count -= 1;
                _pr_fd_cache.head = fd->higher;
                if (NULL == _pr_fd_cache.head)
                {
                    PR_ASSERT(0 == _pr_fd_cache.count);
                    _pr_fd_cache.tail = NULL;
                }
                PR_ASSERT(&_pr_faulty_methods == fd->methods);
                PR_ASSERT(PR_INVALID_IO_LAYER == fd->identity);
                PR_ASSERT(_PR_FILEDESC_FREED == fd->secret->state);
            }
            PR_Unlock(_pr_fd_cache.ml);

        } while (NULL == fd);  
    }

finished:
    fd->dtor = NULL;
    fd->lower = fd->higher = NULL;
    fd->identity = PR_NSPR_IO_LAYER;
    memset(fd->secret, 0, sizeof(PRFilePrivate));
    return fd;

allocate:
    fd = PR_NEW(PRFileDesc);
    if (NULL != fd)
    {
        fd->secret = PR_NEW(PRFilePrivate);
        if (NULL == fd->secret) PR_DELETE(fd);
    }
    if (NULL != fd) goto finished;
    else return NULL;

}  





void _PR_Putfd(PRFileDesc *fd)
{
    PR_ASSERT(PR_NSPR_IO_LAYER == fd->identity);
    fd->methods = &_pr_faulty_methods;
    fd->identity = PR_INVALID_IO_LAYER;
    fd->secret->state = _PR_FILEDESC_FREED;

    if (0 == _pr_fd_cache.limit_high)
    {
        PR_StackPush(_pr_fd_cache.stack, (PRStackElem*)(&fd->higher));
    }
    else
    {
        if (_pr_fd_cache.count > _pr_fd_cache.limit_high)
        {
            PR_Free(fd->secret);
            PR_Free(fd);
        }
        else
        {
            PR_Lock(_pr_fd_cache.ml);
            if (NULL == _pr_fd_cache.tail)
            {
                PR_ASSERT(0 == _pr_fd_cache.count);
                PR_ASSERT(NULL == _pr_fd_cache.head);
                _pr_fd_cache.head = _pr_fd_cache.tail = fd;
            }
            else
            {
                PR_ASSERT(NULL == _pr_fd_cache.tail->higher);
                _pr_fd_cache.tail->higher = fd;
                _pr_fd_cache.tail = fd;  
            }
            fd->higher = NULL;  
            _pr_fd_cache.count += 1;  
            PR_Unlock(_pr_fd_cache.ml);
        }
    }
}  

PR_IMPLEMENT(PRStatus) PR_SetFDCacheSize(PRIntn low, PRIntn high)
{
    




    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (low > high) low = high;  
    
    PR_Lock(_pr_fd_cache.ml);
    if (0 == high)  
    {
        if (0 != _pr_fd_cache.limit_high)  
        {
            _pr_fd_cache.limit_high = 0;  
            






            while (NULL != _pr_fd_cache.head)
            {
                PRFileDesc *fd = _pr_fd_cache.head;
                _pr_fd_cache.head = fd->higher;
                PR_StackPush(_pr_fd_cache.stack, (PRStackElem*)(&fd->higher));
            }
            _pr_fd_cache.limit_low = 0;
            _pr_fd_cache.tail = NULL;
            _pr_fd_cache.count = 0;
        }
    }
    else  
    {
        PRBool was_using_stack = (0 == _pr_fd_cache.limit_high);
        _pr_fd_cache.limit_low = low;
        _pr_fd_cache.limit_high = high;
        if (was_using_stack)  
        {
            PRStackElem *pop;
            while (NULL != (pop = PR_StackPop(_pr_fd_cache.stack)))
            {
                PRFileDesc *fd = (PRFileDesc*)
                    ((PRPtrdiff)pop - (PRPtrdiff)stack2fd);
                if (NULL == _pr_fd_cache.tail) _pr_fd_cache.tail = fd;
                fd->higher = _pr_fd_cache.head;
                _pr_fd_cache.head = fd;
                _pr_fd_cache.count += 1;
            }
        }
    }
    PR_Unlock(_pr_fd_cache.ml);
    return PR_SUCCESS;
}  

void _PR_InitFdCache(void)
{
    





    const char *low = PR_GetEnv("NSPR_FD_CACHE_SIZE_LOW");
    const char *high = PR_GetEnv("NSPR_FD_CACHE_SIZE_HIGH");

    




    _pr_fd_cache.limit_low = 0;
#if defined(DEBUG)
    _pr_fd_cache.limit_high = FD_SETSIZE;
#else
    _pr_fd_cache.limit_high = 0;
#endif  

    if (NULL != low) _pr_fd_cache.limit_low = atoi(low);
    if (NULL != high) _pr_fd_cache.limit_high = atoi(high);

    if (_pr_fd_cache.limit_low < 0)
        _pr_fd_cache.limit_low = 0;
    if (_pr_fd_cache.limit_low > FD_SETSIZE)
        _pr_fd_cache.limit_low = FD_SETSIZE;

    if (_pr_fd_cache.limit_high > FD_SETSIZE)
        _pr_fd_cache.limit_high = FD_SETSIZE;

    if (_pr_fd_cache.limit_high < _pr_fd_cache.limit_low)
        _pr_fd_cache.limit_high = _pr_fd_cache.limit_low;

    _pr_fd_cache.ml = PR_NewLock();
    PR_ASSERT(NULL != _pr_fd_cache.ml);
    _pr_fd_cache.stack = PR_CreateStack("FD");
    PR_ASSERT(NULL != _pr_fd_cache.stack);

}  

void _PR_CleanupFdCache(void)
{
    PRFileDesc *fd, *next;
    PRStackElem *pop;

    for (fd = _pr_fd_cache.head; fd != NULL; fd = next)
    {
        next = fd->higher;
        PR_DELETE(fd->secret);
        PR_DELETE(fd);
    }
    _pr_fd_cache.head = NULL;
    _pr_fd_cache.tail = NULL;
    _pr_fd_cache.count = 0;
    PR_DestroyLock(_pr_fd_cache.ml);
    _pr_fd_cache.ml = NULL;
    while ((pop = PR_StackPop(_pr_fd_cache.stack)) != NULL)
    {
        fd = (PRFileDesc*)((PRPtrdiff)pop - (PRPtrdiff)stack2fd);
        PR_DELETE(fd->secret);
        PR_DELETE(fd);
    }
    PR_DestroyStack(_pr_fd_cache.stack);
    _pr_fd_cache.stack = NULL;
}  


