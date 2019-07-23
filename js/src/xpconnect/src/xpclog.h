









































#ifndef xpclog_h___
#define xpclog_h___

#include "nsIXPConnect.h"
#include "prtypes.h"
#include "prlog.h"














#ifdef DEBUG
#define XPC_LOG_INTERNAL(number,_args)  \
    do{if(XPC_Log_Check(number)){XPC_Log_print _args;}}while(0)

#define XPC_LOG_ALWAYS(_args)   XPC_LOG_INTERNAL(1,_args)
#define XPC_LOG_ERROR(_args)    XPC_LOG_INTERNAL(2,_args)
#define XPC_LOG_WARNING(_args)  XPC_LOG_INTERNAL(3,_args)
#define XPC_LOG_DEBUG(_args)    XPC_LOG_INTERNAL(4,_args)
#define XPC_LOG_FLUSH()         PR_LogFlush()
#define XPC_LOG_INDENT()        XPC_Log_Indent()
#define XPC_LOG_OUTDENT()       XPC_Log_Outdent()
#define XPC_LOG_CLEAR_INDENT()  XPC_Log_Clear_Indent()
#define XPC_LOG_FINISH()        XPC_Log_Finish()

JS_BEGIN_EXTERN_C

void   XPC_Log_print(const char *fmt, ...);
PRBool XPC_Log_Check(int i);
void   XPC_Log_Indent();
void   XPC_Log_Outdent();
void   XPC_Log_Clear_Indent();
void   XPC_Log_Finish();

JS_END_EXTERN_C

#else

#define XPC_LOG_ALWAYS(_args)  ((void)0)
#define XPC_LOG_ERROR(_args)   ((void)0)
#define XPC_LOG_WARNING(_args) ((void)0)
#define XPC_LOG_DEBUG(_args)   ((void)0)
#define XPC_LOG_FLUSH()        ((void)0)
#define XPC_LOG_INDENT()       ((void)0)
#define XPC_LOG_OUTDENT()      ((void)0)
#define XPC_LOG_CLEAR_INDENT() ((void)0)
#define XPC_LOG_FINISH()       ((void)0)
#endif

#endif 
