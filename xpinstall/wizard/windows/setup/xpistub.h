










































#ifdef XP_MAC
#include <Files.h>
#endif

#define nsresult long

PR_BEGIN_EXTERN_C










typedef void     (*pfnXPIStart)   (const char* URL, const char* UIName);







typedef void    (*pfnXPIProgress)(const char* msg, PRInt32 val, PRInt32 max);







typedef void     (*pfnXPIFinal)   (const char* URL, PRInt32 finalStatus);



















PR_EXTERN(nsresult) XPI_Init( 
#ifdef XP_MAC
                              const FSSpec&     aXPIStubDir,
                              const FSSpec&     aProgramDir,
#else
                              const char*       aProgramDir,
#endif
                              const char*       aLogName,
                              pfnXPIProgress    progressCB);











PR_EXTERN(PRInt32) XPI_Install( 
#ifdef XP_MAC
                                 const FSSpec& file,
#else
                                 const char*    file,
#endif
                                 const char* args, 
                                 long flags         );






PR_EXTERN(void) XPI_Exit();

PR_END_EXTERN_C

