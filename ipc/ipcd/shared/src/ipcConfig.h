




































#ifndef ipcProto_h__
#define ipcProto_h__

#if defined(XP_WIN)



#include "prprf.h"

#define IPC_WINDOW_CLASS              "Mozilla:IPCWindowClass"
#define IPC_WINDOW_NAME               "Mozilla:IPCWindow"
#define IPC_CLIENT_WINDOW_CLASS       "Mozilla:IPCAppWindowClass"
#define IPC_CLIENT_WINDOW_NAME_PREFIX "Mozilla:IPCAppWindow:"
#define IPC_SYNC_EVENT_NAME           "Local\\MozillaIPCSyncEvent"
#define IPC_DAEMON_APP_NAME           "mozilla-ipcd.exe"
#define IPC_PATH_SEP_CHAR             '\\'
#define IPC_MODULES_DIR               "ipc\\modules"

#define IPC_CLIENT_WINDOW_NAME_MAXLEN (sizeof(IPC_CLIENT_WINDOW_NAME_PREFIX) + 20)



inline void IPC_GetClientWindowName(PRUint32 pid, char *buf)
{
    PR_snprintf(buf, IPC_CLIENT_WINDOW_NAME_MAXLEN, "%s%u",
                IPC_CLIENT_WINDOW_NAME_PREFIX, pid);
}

#else
#include "nscore.h"



#define IPC_PORT                0
#define IPC_SOCKET_TYPE         "ipc"
#define IPC_DAEMON_APP_NAME     "mozilla-ipcd"
#ifdef XP_OS2
#define IPC_PATH_SEP_CHAR       '\\'
#define IPC_MODULES_DIR         "ipc\\modules"
#else
#define IPC_PATH_SEP_CHAR       '/'
#define IPC_MODULES_DIR         "ipc/modules"
#endif

NS_HIDDEN_(void) IPC_GetDefaultSocketPath(char *buf, PRUint32 bufLen);

#endif



#define IPC_STARTUP_PIPE_NAME   "ipc:startup-pipe"
#define IPC_STARTUP_PIPE_MAGIC  0x1C

#endif 
