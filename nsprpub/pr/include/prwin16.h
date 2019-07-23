




































#ifndef prwin16_h___
#define prwin16_h___




#if (defined(XP_PC) && !defined(_WIN32) && !defined(XP_OS2) && defined(MOZILLA_CLIENT)) || defined(WIN16)
#include <stdio.h>

PR_BEGIN_EXTERN_C
































typedef PRInt32 (PR_CALLBACK *PRStdinRead)( void *buf, PRInt32 amount);
typedef PRInt32 (PR_CALLBACK *PRStdoutWrite)( void *buf, PRInt32 amount);
typedef PRInt32 (PR_CALLBACK *PRStderrWrite)( void *buf, PRInt32 amount);

NSPR_API(PRStatus)
PR_MD_RegisterW16StdioCallbacks( 
    PRStdinRead inReadf,            
    PRStdoutWrite outWritef,        
    PRStderrWrite errWritef         
    );

NSPR_API(PRInt32)
_PL_W16StdioWrite( void *buf, PRInt32 amount );

NSPR_API(PRInt32)
_PL_W16StdioRead( void *buf, PRInt32 amount );

#define PR_STDIO_INIT() PR_MD_RegisterW16StdioCallbacks( \
    _PL_W16StdioRead, _PL_W16StdioWrite, _PL_W16StdioWrite ); \
    PR_INIT_CALLBACKS();





struct PRMethodCallbackStr {
    int     (PR_CALLBACK *auxOutput)(const char *outputString);
    size_t  (PR_CALLBACK *strftime)(char *s, size_t len, const char *fmt, const struct tm *p);
    void *  (PR_CALLBACK *malloc)( size_t size );
    void *  (PR_CALLBACK *calloc)(size_t n, size_t size );
    void *  (PR_CALLBACK *realloc)( void* old_blk, size_t size );
    void    (PR_CALLBACK *free)( void *ptr );
    void *  (PR_CALLBACK *getenv)( const char *name);
    int     (PR_CALLBACK *putenv)( const char *assoc);

};

NSPR_API(void) PR_MDRegisterCallbacks(struct PRMethodCallbackStr *);

int PR_CALLBACK _PL_W16CallBackPuts( const char *outputString );
size_t PR_CALLBACK _PL_W16CallBackStrftime( 
    char *s, 
    size_t len, 
    const char *fmt,
    const struct tm *p );
void * PR_CALLBACK _PL_W16CallBackMalloc( size_t size );
void * PR_CALLBACK _PL_W16CallBackCalloc( size_t n, size_t size );
void * PR_CALLBACK _PL_W16CallBackRealloc( 
    void *old_blk, 
    size_t size );
void   PR_CALLBACK _PL_W16CallBackFree( void *ptr );
void * PR_CALLBACK _PL_W16CallBackGetenv( const char *name );
int PR_CALLBACK _PL_W16CallBackPutenv( const char *assoc );









NSPR_API(int)     PR_MD_printf(const char *, ...);
NSPR_API(void)    PR_MD_exit(int);
NSPR_API(size_t)  PR_MD_strftime(char *, size_t, const char *, const struct tm *); 
NSPR_API(int)     PR_MD_sscanf(const char *, const char *, ...);
NSPR_API(void*)   PR_MD_malloc( size_t size );
NSPR_API(void*)   PR_MD_calloc( size_t n, size_t size );
NSPR_API(void*)   PR_MD_realloc( void* old_blk, size_t size );
NSPR_API(void)    PR_MD_free( void *ptr );
NSPR_API(char*)   PR_MD_getenv( const char *name );
NSPR_API(int)     PR_MD_putenv( const char *assoc );
NSPR_API(int)     PR_MD_fprintf(FILE *fPtr, const char *fmt, ...);

#define PR_INIT_CALLBACKS()                         \
    {                                               \
        static struct PRMethodCallbackStr cbf = {   \
            _PL_W16CallBackPuts,                    \
            _PL_W16CallBackStrftime,                \
            _PL_W16CallBackMalloc,                  \
            _PL_W16CallBackCalloc,                  \
            _PL_W16CallBackRealloc,                 \
            _PL_W16CallBackFree,                    \
            _PL_W16CallBackGetenv,                  \
            _PL_W16CallBackPutenv,                  \
        };                                          \
        PR_MDRegisterCallbacks( &cbf );             \
    }





NSPR_API(void *) PR_W16GetExceptionContext(void);



NSPR_API(void) PR_W16SetExceptionContext(void *context);

PR_END_EXTERN_C
#else




#define PR_STDIO_INIT()
#endif 

#endif 








