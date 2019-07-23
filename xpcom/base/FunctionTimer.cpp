






































#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "prenv.h"

#include "mozilla/FunctionTimer.h"

#ifdef _MSC_VER
#define vsnprintf _vsnprintf
#endif

using namespace mozilla;




static TimeStamp sAppStart;

FunctionTimerLog *FunctionTimer::sLog = nsnull;
char *FunctionTimer::sBuf1 = nsnull;
char *FunctionTimer::sBuf2 = nsnull;
int FunctionTimer::sBufSize = FunctionTimer::InitTimers();

int
FunctionTimer::InitTimers()
{
    if (PR_GetEnv("MOZ_FT") == NULL)
        return 0;

    
    TimeStamp::Startup();

    sLog = new FunctionTimerLog(PR_GetEnv("MOZ_FT"));
    sBuf1 = (char *) malloc(BUF_LOG_LENGTH);
    sBuf2 = (char *) malloc(BUF_LOG_LENGTH);
    sAppStart = TimeStamp::Now();

    return BUF_LOG_LENGTH;
}

FunctionTimerLog::FunctionTimerLog(const char *fname)
{
    if (strcmp(fname, "stdout") == 0) {
        mFile = stdout;
    } else if (strcmp(fname, "stderr") == 0) {
        mFile = stderr;
    } else {
        FILE *fp = fopen(fname, "wb");
        if (!fp) {
            NS_WARNING("FunctionTimerLog: Failed to open file specified, logging disabled!");
        }
        mFile = fp;
    }
}

FunctionTimerLog::~FunctionTimerLog()
{
    if (mFile && mFile != stdout && mFile != stderr)
        fclose((FILE*)mFile);
}

void
FunctionTimerLog::LogString(const char *str)
{
    if (mFile) {
        TimeDuration elapsed = TimeStamp::Now() - sAppStart;
        fprintf((FILE*)mFile, "[% 9.2f] %s\n", elapsed.ToSeconds() * 1000.0, str);
    }
}

int
FunctionTimer::ft_vsnprintf(char *str, int maxlen, const char *fmt, va_list args)
{
    return vsnprintf(str, maxlen, fmt, args);
}

int
FunctionTimer::ft_snprintf(char *str, int maxlen, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int rval = ft_vsnprintf(str, maxlen, fmt, ap);

    va_end(ap);

    return rval;
}
