

















































#include "primpl.h"
#include "windowsx.h"





struct PRMethodCallbackStr * _pr_callback_funcs;





void PR_MDRegisterCallbacks(struct PRMethodCallbackStr *f)
{
    _pr_callback_funcs = f;
}









int  PR_MD_printf(const char *fmt, ...)
{
    char buffer[1024];
    int ret = 0;
    va_list args;

    va_start(args, fmt);

#ifdef DEBUG
    PR_vsnprintf(buffer, sizeof(buffer), fmt, args);
    {   
        if (_pr_callback_funcs != NULL && _pr_callback_funcs->auxOutput != NULL) {
            (* _pr_callback_funcs->auxOutput)(buffer);
        } else {
            OutputDebugString(buffer);
        }
    }
#endif

    va_end(args);
    return ret;
}





int  PR_MD_sscanf(const char *buf, const char *fmt, ...)
{
	int		retval;
	va_list	arglist;

	va_start(arglist, fmt);
	retval = vsscanf((const unsigned char *)buf, (const unsigned char *)fmt, arglist);
	va_end(arglist);
	return retval;
}





size_t PR_MD_strftime(char *s, size_t len, const char *fmt, const struct tm *p) 
{
    if( _pr_callback_funcs ) {
        return (*_pr_callback_funcs->strftime)(s, len, fmt, p);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}






void *PR_MD_malloc( size_t size )
{
    if( _pr_callback_funcs ) {
        return (*_pr_callback_funcs->malloc)( size );
    } else {
        return GlobalAllocPtr(GPTR, (DWORD)size);
    }
} 





void *PR_MD_calloc( size_t n, size_t size )
{
    void *p;
    size_t sz;
    
    if( _pr_callback_funcs ) {
        return (*_pr_callback_funcs->calloc)( n, size );
    } else {
        sz = n * size;
        p = GlobalAllocPtr(GPTR, (DWORD)sz );
        memset( p, 0x00, sz );
        return p;
    }
} 





void *PR_MD_realloc( void* old_blk, size_t size )
{
    if( _pr_callback_funcs ) {
        return (*_pr_callback_funcs->realloc)( old_blk, size );
    } else {
        return GlobalReAllocPtr( old_blk, (DWORD)size, GPTR);
    }
} 





void PR_MD_free( void *ptr )
{
    if( _pr_callback_funcs ) {
        (*_pr_callback_funcs->free)( ptr );
        return;
    } else {
        GlobalFreePtr( ptr );
        return;
    }
} 





char *PR_MD_getenv( const char *name )
{
    if( _pr_callback_funcs ) {
        return (*_pr_callback_funcs->getenv)( name );
    } else {
        return 0;
    }
} 









void PR_MD_perror( const char *prefix )
{
    return;
} 





int  PR_MD_putenv(const char *assoc)
{
    if( _pr_callback_funcs ) {
        return (*_pr_callback_funcs->putenv)(assoc);
    } else {
        PR_ASSERT(0);
        return NULL;
    }
}





int  PR_MD_fprintf(FILE *fPtr, const char *fmt, ...)
{
    char buffer[1024];
    va_list args;

    va_start(args, fmt);
    PR_vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (fPtr == NULL) 
    {
        if (_pr_callback_funcs != NULL && _pr_callback_funcs->auxOutput != NULL) 
        {
            (* _pr_callback_funcs->auxOutput)(buffer);
        } 
        else 
        {
            OutputDebugString(buffer);
        }
    } 
    else 
    {
        fwrite(buffer, 1, strlen(buffer), fPtr); 
    }

    va_end(args);
    return 0;
}


