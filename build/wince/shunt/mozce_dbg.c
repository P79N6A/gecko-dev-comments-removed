





































#include "mozce_internal.h"

#include <stdarg.h>


#ifndef SHUNT_LOG_ENABLED

void mozce_DebugInit() { };
void mozce_DebugDeinit() { };
void mozce_DebugWriteToLog(char *str) { };

#else

#define LOGFILE     "\\Storage Card\\shuntlog.txt"

FILE *gpDebugFile = NULL;

void mozce_DebugInit()
{
    if ( NULL == gpDebugFile )
        gpDebugFile = fopen(LOGFILE, "a+");
}

void mozce_DebugDeinit()
{
    if ( gpDebugFile ) {
        fclose( gpDebugFile );
        gpDebugFile = NULL;
    }
}

void mozce_DebugWriteToLog(char *str)
{
    if ( NULL == gpDebugFile )
        mozce_DebugInit();

    if ( gpDebugFile ) {
        fprintf(gpDebugFile, "%s", str);
        fflush(gpDebugFile);
    }
}

#endif
