




































#include <windows.h>
#include <time.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <primpl.h>

static BOOL
CurrentClockTickTime(LPDWORD lpdwHigh, LPDWORD lpdwLow)
{
    LARGE_INTEGER   liCount;

    if (!QueryPerformanceCounter(&liCount))
        return FALSE;

    *lpdwHigh = liCount.u.HighPart;
    *lpdwLow = liCount.u.LowPart;
    return TRUE;
}

extern PRSize _PR_MD_GetRandomNoise( void *buf, PRSize size )
{
    DWORD   dwHigh, dwLow, dwVal;
    size_t  n = 0;
    size_t  nBytes;
    time_t  sTime;

    if (size <= 0)
        return 0;

    CurrentClockTickTime(&dwHigh, &dwLow);

    
    nBytes = sizeof(dwLow) > size ? size : sizeof(dwLow);
    memcpy((char *)buf, &dwLow, nBytes);
    n += nBytes;
    size -= nBytes;

    if (size <= 0)
        return n;

    nBytes = sizeof(dwHigh) > size ? size : sizeof(dwHigh);
    memcpy(((char *)buf) + n, &dwHigh, nBytes);
    n += nBytes;
    size -= nBytes;

    if (size <= 0)
        return n;

    
    dwVal = GetTickCount();

    nBytes = sizeof(dwVal) > size ? size : sizeof(dwVal);
    memcpy(((char *)buf) + n, &dwVal, nBytes);
    n += nBytes;
    size -= nBytes;

    if (size <= 0)
        return n;

    
    time(&sTime);
    nBytes = sizeof(sTime) > size ? size : sizeof(sTime);
    memcpy(((char *)buf) + n, &sTime, nBytes);
    n += nBytes;

    return n;
}

