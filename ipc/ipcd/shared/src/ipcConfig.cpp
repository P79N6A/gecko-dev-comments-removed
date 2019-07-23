




































#ifdef XP_WIN
#else
#include <string.h>
#include "ipcConfig.h"
#include "ipcLog.h"
#include "prenv.h"
#include "plstr.h"

#ifdef XP_OS2
static const char kDefaultSocketPrefix[] = "\\socket\\mozilla-";
static const char kDefaultSocketSuffix[] = "-ipc\\ipcd";
#else
static const char kDefaultSocketPrefix[] = "/tmp/.mozilla-";
static const char kDefaultSocketSuffix[] = "-ipc/ipcd";
#endif

void IPC_GetDefaultSocketPath(char *buf, PRUint32 bufLen)
{
    const char *logName;
    int len;

    PL_strncpyz(buf, kDefaultSocketPrefix, bufLen);
    buf    += (sizeof(kDefaultSocketPrefix) - 1);
    bufLen -= (sizeof(kDefaultSocketPrefix) - 1);

    logName = PR_GetEnv("LOGNAME");
    if (!logName || !logName[0]) {
        logName = PR_GetEnv("USER");
        if (!logName || !logName[0]) {
            LOG(("could not determine username from environment\n"));
            goto end;
        }
    }
    PL_strncpyz(buf, logName, bufLen);
    len = strlen(logName);
    buf    += len;
    bufLen -= len;

end:
    PL_strncpyz(buf, kDefaultSocketSuffix, bufLen);
}

#endif
