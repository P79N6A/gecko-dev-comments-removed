




































#include "ipcLog.h"

#ifdef IPC_LOGGING

#include <string.h>
#include <ctype.h>

#include "prenv.h"
#include "prprf.h"
#include "prthread.h"
#include "plstr.h"

PRBool ipcLogEnabled = PR_FALSE;
char ipcLogPrefix[10] = {0};




#if defined(XP_UNIX) || defined(XP_OS2) || defined(XP_BEOS)
#include <sys/types.h>
#include <unistd.h>

static inline PRUint32
WritePrefix(char *buf, PRUint32 bufLen)
{
    return PR_snprintf(buf, bufLen, "[%u:%p] %s ",
                       (unsigned) getpid(),
                       PR_GetCurrentThread(),
                       ipcLogPrefix);
}
#endif




#ifdef XP_WIN
#include <windows.h>

static inline PRUint32
WritePrefix(char *buf, PRUint32 bufLen)
{
    return PR_snprintf(buf, bufLen, "[%u:%p] %s ",
                       GetCurrentProcessId(),
                       PR_GetCurrentThread(),
                       ipcLogPrefix);
}
#endif





void
IPC_InitLog(const char *prefix)
{
    if (PR_GetEnv("IPC_LOG_ENABLE")) {
        ipcLogEnabled = PR_TRUE;
        PL_strncpyz(ipcLogPrefix, prefix, sizeof(ipcLogPrefix));
    }
}

void
IPC_Log(const char *fmt, ... )
{
    va_list ap;
    va_start(ap, fmt);
    PRUint32 nb = 0;
    char buf[512];

    if (ipcLogPrefix[0])
        nb = WritePrefix(buf, sizeof(buf));

    PR_vsnprintf(buf + nb, sizeof(buf) - nb, fmt, ap);
    buf[sizeof(buf) - 1] = '\0';

    fwrite(buf, strlen(buf), 1, stdout);

    va_end(ap);
}

void
IPC_LogBinary(const PRUint8 *data, PRUint32 len)
{
    PRUint32 i, j, ln;
    for (i=0; i<len; ) {
        char line[100] = "";
        const PRUint8 *p;

        ln = 0;
        
        p = &data[i];
        for (j=0; j<PR_MIN(8, len - i); ++j, ++p)
            ln += PR_snprintf(line + ln, sizeof(line) - ln, "%02x  ", *p);

        for (; ln < 32; ++ln)
            line[ln] = ' ';

        p = &data[i];
        for (j=0; j<PR_MIN(8, len - i); ++j, ++p)
            ln += PR_snprintf(line + ln, sizeof(line) - ln, "%c", isprint(*p) ? *p : '.');

        line[ln] = '\0';

        i += (p - &data[i]);

        LOG(("%s\n", line));
    }
}

#endif
