



#include "secrng.h"

#ifdef XP_WIN
#include <windows.h>
#include <time.h>

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

size_t RNG_GetNoise(void *buf, size_t maxbuf)
{
    DWORD   dwHigh, dwLow, dwVal;
    int     n = 0;
    int     nBytes;
    time_t  sTime;

    if (maxbuf <= 0)
        return 0;

    CurrentClockTickTime(&dwHigh, &dwLow);

    
    nBytes = sizeof(dwLow) > maxbuf ? maxbuf : sizeof(dwLow);
    memcpy((char *)buf, &dwLow, nBytes);
    n += nBytes;
    maxbuf -= nBytes;

    if (maxbuf <= 0)
        return n;

    nBytes = sizeof(dwHigh) > maxbuf ? maxbuf : sizeof(dwHigh);
    memcpy(((char *)buf) + n, &dwHigh, nBytes);
    n += nBytes;
    maxbuf -= nBytes;

    if (maxbuf <= 0)
        return n;

    
    dwVal = GetTickCount();

    nBytes = sizeof(dwVal) > maxbuf ? maxbuf : sizeof(dwVal);
    memcpy(((char *)buf) + n, &dwVal, nBytes);
    n += nBytes;
    maxbuf -= nBytes;

    if (maxbuf <= 0)
        return n;

    
    time(&sTime);
    nBytes = sizeof(sTime) > maxbuf ? maxbuf : sizeof(sTime);
    memcpy(((char *)buf) + n, &sTime, nBytes);
    n += nBytes;

    return n;
}

void RNG_SystemInfoForRNG(void)
{
    DWORD           dwVal;
    char            buffer[256];
    int             nBytes;
    MEMORYSTATUS    sMem;
    HANDLE          hVal;
    DWORD           dwSerialNum;
    DWORD           dwComponentLen;
    DWORD           dwSysFlags;
    char            volName[128];
    DWORD           dwSectors, dwBytes, dwFreeClusters, dwNumClusters;

    nBytes = RNG_GetNoise(buffer, 20);  
    RNG_RandomUpdate(buffer, nBytes);

    sMem.dwLength = sizeof(sMem);
    GlobalMemoryStatus(&sMem);                
    RNG_RandomUpdate(&sMem, sizeof(sMem));

    dwVal = GetLogicalDrives();
    RNG_RandomUpdate(&dwVal, sizeof(dwVal));  

    dwVal = sizeof(buffer);
    if (GetComputerName(buffer, &dwVal))
        RNG_RandomUpdate(buffer, dwVal);

    hVal = GetCurrentProcess();               
                                              
    RNG_RandomUpdate(&hVal, sizeof(hVal));

    dwVal = GetCurrentProcessId();            
    RNG_RandomUpdate(&dwVal, sizeof(dwVal));

    dwVal = GetCurrentThreadId();             
    RNG_RandomUpdate(&dwVal, sizeof(dwVal));

    volName[0] = '\0';
    buffer[0] = '\0';
    GetVolumeInformation(NULL,
                         volName,
                         sizeof(volName),
                         &dwSerialNum,
                         &dwComponentLen,
                         &dwSysFlags,
                         buffer,
                         sizeof(buffer));

    RNG_RandomUpdate(volName,         strlen(volName));
    RNG_RandomUpdate(&dwSerialNum,    sizeof(dwSerialNum));
    RNG_RandomUpdate(&dwComponentLen, sizeof(dwComponentLen));
    RNG_RandomUpdate(&dwSysFlags,     sizeof(dwSysFlags));
    RNG_RandomUpdate(buffer,          strlen(buffer));

    if (GetDiskFreeSpace(NULL, &dwSectors, &dwBytes, &dwFreeClusters, 
                         &dwNumClusters)) {
        RNG_RandomUpdate(&dwSectors,      sizeof(dwSectors));
        RNG_RandomUpdate(&dwBytes,        sizeof(dwBytes));
        RNG_RandomUpdate(&dwFreeClusters, sizeof(dwFreeClusters));
        RNG_RandomUpdate(&dwNumClusters,  sizeof(dwNumClusters));
    }

    nBytes = RNG_GetNoise(buffer, 20);  
    RNG_RandomUpdate(buffer, nBytes);
}







#define RtlGenRandom SystemFunction036
DECLSPEC_IMPORT BOOLEAN WINAPI RtlGenRandom(
    PVOID RandomBuffer,
    ULONG RandomBufferLength);

size_t RNG_SystemRNG(void *dest, size_t maxLen)
{
    size_t bytes = 0;

    if (RtlGenRandom(dest, maxLen)) {
	bytes = maxLen;
    }
    return bytes;
}
#endif  
