



































 





#include "primpl.h"







PRInt32
_PL_W16StdioWrite( void *buf, PRInt32 amount )
{
    int   rc;
    
    rc = fputs( buf, stdout );
    if ( rc == EOF )
    {
        
        return(PR_FAILURE);
    }
    return( strlen(buf));
} 





PRInt32
_PL_W16StdioRead( void *buf, PRInt32 amount )
{
    char *bp;

    bp = fgets( buf, (int) amount, stdin );
    if ( bp == NULL )
    {
        
        return(PR_FAILURE);
    }
    
    return( strlen(buf));
} 












int PR_CALLBACK _PL_W16CallBackPuts( const char *outputString )
{
    return( puts( outputString ));
}     





size_t PR_CALLBACK _PL_W16CallBackStrftime( 
    char *s, 
    size_t len, 
    const char *fmt,
    const struct tm *p )
{
    return( strftime( s, len, fmt, p ));
}     





void * PR_CALLBACK _PL_W16CallBackMalloc( size_t size )
{
    return( malloc( size ));
}     





void * PR_CALLBACK _PL_W16CallBackCalloc( size_t n, size_t size )
{
    return( calloc( n, size ));
}     





void * PR_CALLBACK _PL_W16CallBackRealloc( 
    void *old_blk, 
    size_t size )
{
    return( realloc( old_blk, size ));
} 





void PR_CALLBACK _PL_W16CallBackFree( void *ptr )
{
    free( ptr );
    return;
} 





void * PR_CALLBACK _PL_W16CallBackGetenv( const char *name )
{
    return( getenv( name ));
} 






int PR_CALLBACK _PL_W16CallBackPutenv( const char *assoc )
{
    return( putenv( assoc ));
} 
