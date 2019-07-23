




































#ifndef ipcLog_h__
#define ipcLog_h__

#include "nscore.h"
#include "prtypes.h"

#ifdef DEBUG
#define IPC_LOGGING
#endif

#ifdef IPC_LOGGING

extern PRBool ipcLogEnabled;
extern NS_HIDDEN_(void) IPC_InitLog(const char *prefix);
extern NS_HIDDEN_(void) IPC_Log(const char *fmt, ...);
extern NS_HIDDEN_(void) IPC_LogBinary(const PRUint8 *data, PRUint32 len);

#define IPC_LOG(_args)         \
    PR_BEGIN_MACRO             \
        if (ipcLogEnabled)     \
            IPC_Log _args;     \
    PR_END_MACRO

#define LOG(args)     IPC_LOG(args)
#define LOG_ENABLED() ipcLogEnabled

#else
#define IPC_InitLog(prefix)
#define IPC_LogBinary(data, len)
#define LOG(args)
#define LOG_ENABLED() (0)
#endif

#endif 
