












































#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __IBMC__
#include <builtin.h>
#endif

#define INCL_BASE
#include <os2.h>

#define INCL_DEBUGONLY
#include "nsInnoTekPluginWrapper.h"




static const char szPrefix[] = "ipluginw: ";





int     npdprintf(const char *pszFormat, ...)
{
    static int      fInited = 0;
    static FILE *   phFile;
    char    szMsg[4096];
    int     rc;
    va_list args;

    strcpy(szMsg, szPrefix);
    rc = strlen(szMsg);
    va_start(args, pszFormat);
    rc += vsprintf(&szMsg[rc], pszFormat, args);
    va_end(args);
    if (rc > (int)sizeof(szMsg) - 32)
    {
        
#ifdef __IBMC__
        _interrupt(3);
#else
        asm("int $3");
#endif
    }

    if (rc > 0 && szMsg[rc - 1] != '\n')
    {
        szMsg[rc++] = '\n';
        szMsg[rc] = '\0';
    }

    fwrite(&szMsg[0], 1, rc, stderr);
    if (!fInited)
    {
        fInited = 1;
        phFile = fopen("ipluginw.log", "w");
        if (phFile)
        {
            DATETIME dt;
            DosGetDateTime(&dt);
            fprintf(phFile, "*** Log Opened on %04d-%02d-%02d %02d:%02d:%02d ***\n",
                    dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds);
            fprintf(phFile, "*** Build Date: " __DATE__ " " __TIME__ " ***\n");
        }
    }
    if (phFile)
    {
        fwrite(&szMsg[0], 1, rc, phFile);
        fflush(phFile);
    }

    return rc;
}





void _Optlink ReleaseInt3(unsigned uEAX, unsigned uEDX, unsigned uECX)
{
#ifdef __IBMC__
    _interrupt(3);
#else
    asm("int $3");
#endif
}
