




































#ifndef prlog_h___
#define prlog_h___

#include "prtypes.h"

PR_BEGIN_EXTERN_C






























































































typedef enum PRLogModuleLevel {
    PR_LOG_NONE = 0,                
    PR_LOG_ALWAYS = 1,              
    PR_LOG_ERROR = 2,               
    PR_LOG_WARNING = 3,             
    PR_LOG_DEBUG = 4,               

    PR_LOG_NOTICE = PR_LOG_DEBUG,   
    PR_LOG_WARN = PR_LOG_WARNING,   
    PR_LOG_MIN = PR_LOG_DEBUG,      
    PR_LOG_MAX = PR_LOG_DEBUG       
} PRLogModuleLevel;






typedef struct PRLogModuleInfo {
    const char *name;
    PRLogModuleLevel level;
    struct PRLogModuleInfo *next;
} PRLogModuleInfo;




NSPR_API(PRLogModuleInfo*) PR_NewLogModule(const char *name);





NSPR_API(PRBool) PR_SetLogFile(const char *name);





NSPR_API(void) PR_SetLogBuffering(PRIntn buffer_size);







NSPR_API(void) PR_LogPrint(const char *fmt, ...);




NSPR_API(void) PR_LogFlush(void);

#if defined(DEBUG) || defined(FORCE_PR_LOG)
#define PR_LOGGING 1

#define PR_LOG_TEST(_module,_level) \
    ((_module)->level >= (_level))








#define PR_LOG(_module,_level,_args)     \
    PR_BEGIN_MACRO             \
      if (PR_LOG_TEST(_module,_level)) { \
      PR_LogPrint _args;         \
      }                     \
    PR_END_MACRO

#else 

#undef PR_LOGGING
#define PR_LOG_TEST(module,level) 0
#define PR_LOG(module,level,args)

#endif 

#ifndef NO_NSPR_10_SUPPORT

#ifdef PR_LOGGING
#define PR_LOG_BEGIN    PR_LOG
#define PR_LOG_END      PR_LOG
#define PR_LOG_DEFINE   PR_NewLogModule
#else
#define PR_LOG_BEGIN(module,level,args)
#define PR_LOG_END(module,level,args)
#define PR_LOG_DEFINE(_name)    NULL
#endif 

#endif 

#if defined(DEBUG) || defined(FORCE_PR_ASSERT)

NSPR_API(void) PR_Assert(const char *s, const char *file, PRIntn ln);
#define PR_ASSERT(_expr) \
    ((_expr)?((void)0):PR_Assert(# _expr,__FILE__,__LINE__))

#define PR_NOT_REACHED(_reasonStr) \
    PR_Assert(_reasonStr,__FILE__,__LINE__)

#else

#define PR_ASSERT(expr) ((void) 0)
#define PR_NOT_REACHED(reasonStr)

#endif 






#define PR_STATIC_ASSERT(condition) \
    extern void pr_static_assert(int arg[(condition) ? 1 : -1])

PR_END_EXTERN_C

#endif 
