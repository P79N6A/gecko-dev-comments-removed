



































#include "secrng.h"
#include "secerr.h"

#ifdef XP_WIN
#include <windows.h>
#include <shlobj.h>     

#if defined(_WIN32_WCE)
#include <stdlib.h>	
#include "prprf.h"	
#else
#include <time.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <stdio.h>
#include "prio.h"
#include "prerror.h"

static PRInt32  filesToRead;
static DWORD    totalFileBytes;
static DWORD    maxFileBytes	= 250000;	
static DWORD    dwNumFiles, dwReadEvery, dwFileToRead;
static PRBool   usedWindowsPRNG;

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

    {
#if defined(_WIN32_WCE)
    
    FILETIME sTime;
    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st,&sTime);
#else
    time_t  sTime;
    
    time(&sTime);
#endif
    nBytes = sizeof(sTime) > maxbuf ? maxbuf : sizeof(sTime);
    memcpy(((char *)buf) + n, &sTime, nBytes);
    n += nBytes;
    }

    return n;
}

typedef PRInt32 (* Handler)(const PRUnichar *);
#define MAX_DEPTH 2
#define MAX_FOLDERS 4
#define MAX_FILES 1024

static void
EnumSystemFilesInFolder(Handler func, PRUnichar* szSysDir, int maxDepth) 
{
    int                 iContinue;
    unsigned int        uFolders  = 0;
    unsigned int        uFiles    = 0;
    HANDLE              lFindHandle;
    WIN32_FIND_DATAW    fdData;
    PRUnichar           szFileName[_MAX_PATH];

    if (maxDepth < 0)
    	return;
    
    _snwprintf(szFileName, _MAX_PATH, L"%s\\*.*", szSysDir);

    lFindHandle = FindFirstFileW(szFileName, &fdData);
    if (lFindHandle == INVALID_HANDLE_VALUE)
        return;
    do {
	iContinue = 1;
	if (wcscmp(fdData.cFileName, L".") == 0 ||
            wcscmp(fdData.cFileName, L"..") == 0) {
	    
	} else {
	    
	    _snwprintf(szFileName, _MAX_PATH, L"%s\\%s", szSysDir, 
		       fdData.cFileName);
	    if (fdData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		if (++uFolders <= MAX_FOLDERS)
		    EnumSystemFilesInFolder(func, szFileName, maxDepth - 1);
	    } else {
		iContinue = (++uFiles <= MAX_FILES) && !(*func)(szFileName);
	    }
	}
	if (iContinue)
	    iContinue = FindNextFileW(lFindHandle, &fdData);
    } while (iContinue);
    FindClose(lFindHandle);
}

static BOOL
EnumSystemFiles(Handler func)
{
    PRUnichar szSysDir[_MAX_PATH];
    static const int folders[] = {
    	CSIDL_BITBUCKET,  
	CSIDL_RECENT,
#ifndef WINCE		     
	CSIDL_INTERNET_CACHE, 
	CSIDL_HISTORY,
#endif
	0
    };
    int i = 0;
    if (_MAX_PATH > (i = GetTempPathW(_MAX_PATH, szSysDir))) {
        if (i > 0 && szSysDir[i-1] == L'\\')
	    szSysDir[i-1] = L'\0'; 
        EnumSystemFilesInFolder(func, szSysDir, MAX_DEPTH);
    }
    for(i = 0; folders[i]; i++) {
        DWORD rv = SHGetSpecialFolderPathW(NULL, szSysDir, folders[i], 0);
        if (szSysDir[0])
            EnumSystemFilesInFolder(func, szSysDir, MAX_DEPTH);
        szSysDir[0] =  L'\0';
    }
    return PR_TRUE;
}

static PRInt32
CountFiles(const PRUnichar *file)
{
    dwNumFiles++;
    return 0;
}

static int
ReadSingleFile(const char *filename)
{
    PRFileDesc *    file;
    unsigned char   buffer[1024];

    file = PR_Open(filename, PR_RDONLY, 0);
    if (file != NULL) {
	while (PR_Read(file, buffer, sizeof buffer) > 0)
	    ;
        PR_Close(file);
    }
    return (file != NULL);
}

static PRInt32
ReadOneFile(const PRUnichar *szFileName)
{
    char narrowFileName[_MAX_PATH];

    if (dwNumFiles == dwFileToRead) {
	int success = WideCharToMultiByte(CP_ACP, 0, szFileName, -1, 
					  narrowFileName, _MAX_PATH, 
					  NULL, NULL);
	if (success)
	    success = ReadSingleFile(narrowFileName);
    	if (!success)
	    dwFileToRead++; 
    }
    dwNumFiles++;
    return dwNumFiles > dwFileToRead;
}

static PRInt32
ReadFiles(const PRUnichar *szFileName)
{
    char narrowFileName[_MAX_PATH];

    if ((dwNumFiles % dwReadEvery) == 0) {
	++filesToRead;
    }
    if (filesToRead) {
	DWORD prevFileBytes = totalFileBytes;
	int   iContinue     = WideCharToMultiByte(CP_ACP, 0, szFileName, -1, 
						  narrowFileName, _MAX_PATH, 
						  NULL, NULL);
	if (iContinue) {
	    RNG_FileForRNG(narrowFileName);
	}
	if (prevFileBytes < totalFileBytes) {
	    --filesToRead;
	}
    }
    dwNumFiles++;
    return (totalFileBytes >= maxFileBytes);
}

static void
ReadSystemFiles(void)
{
    
    dwNumFiles = 0;
    if (!EnumSystemFiles(CountFiles))
        return;

    RNG_RandomUpdate(&dwNumFiles, sizeof(dwNumFiles));

    
    
    filesToRead = 10;
    if (dwNumFiles == 0)
        return;

    dwReadEvery = dwNumFiles / 10;
    if (dwReadEvery == 0)
        dwReadEvery = 1;  

    dwNumFiles = 0;
    totalFileBytes = 0;
    EnumSystemFiles(ReadFiles);
}

void RNG_SystemInfoForRNG(void)
{
    DWORD           dwVal;
    char            buffer[256];
    int             nBytes;
    MEMORYSTATUS    sMem;
    HANDLE          hVal;
#if !defined(_WIN32_WCE)
    DWORD           dwSerialNum;
    DWORD           dwComponentLen;
    DWORD           dwSysFlags;
    char            volName[128];
    DWORD           dwSectors, dwBytes, dwFreeClusters, dwNumClusters;
#endif

    nBytes = RNG_GetNoise(buffer, 20);  
    RNG_RandomUpdate(buffer, nBytes);

    sMem.dwLength = sizeof(sMem);
    GlobalMemoryStatus(&sMem);                
    RNG_RandomUpdate(&sMem, sizeof(sMem));
#if !defined(_WIN32_WCE)
    dwVal = GetLogicalDrives();
    RNG_RandomUpdate(&dwVal, sizeof(dwVal));  
#endif

#if !defined(_WIN32_WCE)
    dwVal = sizeof(buffer);
    if (GetComputerName(buffer, &dwVal))
        RNG_RandomUpdate(buffer, dwVal);
#endif

    hVal = GetCurrentProcess();               
                                              
    RNG_RandomUpdate(&hVal, sizeof(hVal));

    dwVal = GetCurrentProcessId();            
    RNG_RandomUpdate(&dwVal, sizeof(dwVal));

    dwVal = GetCurrentThreadId();             
    RNG_RandomUpdate(&dwVal, sizeof(dwVal));

#if !defined(_WIN32_WCE)
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
#endif

    
    if (!usedWindowsPRNG)
	ReadSystemFiles();

    nBytes = RNG_GetNoise(buffer, 20);  
    RNG_RandomUpdate(buffer, nBytes);
}

static void rng_systemJitter(void)
{   
    dwNumFiles = 0;
    EnumSystemFiles(ReadOneFile);
    dwFileToRead++;
    if (dwFileToRead >= dwNumFiles) {
	dwFileToRead = 0;
    }
}


#if defined(_WIN32_WCE)
void RNG_FileForRNG(const char *filename)
{
    PRFileDesc *    file;
    int             nBytes;
    PRFileInfo      infoBuf;
    unsigned char   buffer[1024];

    if (PR_GetFileInfo(filename, &infoBuf) != PR_SUCCESS)
        return;

    RNG_RandomUpdate((unsigned char*)&infoBuf, sizeof(infoBuf));

    file = PR_Open(filename, PR_RDONLY, 0);
    if (file != NULL) {
        for (;;) {
            PRInt32 bytes = PR_Read(file, buffer, sizeof buffer);

            if (bytes <= 0)
                break;

            RNG_RandomUpdate(buffer, bytes);
            totalFileBytes += bytes;
            if (totalFileBytes > maxFileBytes)
                break;
        }

        PR_Close(file);
    }

    nBytes = RNG_GetNoise(buffer, 20);  
    RNG_RandomUpdate(buffer, nBytes);
}







size_t RNG_SystemRNG(void *dest, size_t maxLen)
{
    size_t bytes = 0;
    usedWindowsPRNG = PR_FALSE;
    if (CeGenRandom(maxLen, dest)) {
	bytes = maxLen;
	usedWindowsPRNG = PR_TRUE;
    }
    if (bytes == 0) {
	bytes = rng_systemFromNoise(dest,maxLen);
    }
    return bytes;
}


#else 

void RNG_FileForRNG(const char *filename)
{
    FILE*           file;
    int             nBytes;
    struct stat     stat_buf;
    unsigned char   buffer[1024];

    


    memset(&stat_buf, 0, sizeof stat_buf);

    if (stat((char *)filename, &stat_buf) < 0)
        return;

    RNG_RandomUpdate((unsigned char*)&stat_buf, sizeof(stat_buf));

    file = fopen((char *)filename, "r");
    if (file != NULL) {
        for (;;) {
            size_t  bytes = fread(buffer, 1, sizeof(buffer), file);

            if (bytes == 0)
                break;

            RNG_RandomUpdate(buffer, bytes);
            totalFileBytes += bytes;
            if (totalFileBytes > maxFileBytes)
                break;
        }

        fclose(file);
    }

    nBytes = RNG_GetNoise(buffer, 20);  
    RNG_RandomUpdate(buffer, nBytes);
}









#ifndef WIN64
typedef unsigned long HCRYPTPROV;
#endif

#define CRYPT_VERIFYCONTEXT 0xF0000000

#define PROV_RSA_FULL 1

typedef BOOL
(WINAPI *CryptAcquireContextAFn)(
    HCRYPTPROV *phProv,
    LPCSTR pszContainer,
    LPCSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags);

typedef BOOL
(WINAPI *CryptReleaseContextFn)(
    HCRYPTPROV hProv,
    DWORD dwFlags);

typedef BOOL
(WINAPI *CryptGenRandomFn)(
    HCRYPTPROV hProv,
    DWORD dwLen,
    BYTE *pbBuffer);





typedef BOOLEAN
(APIENTRY *RtlGenRandomFn)(
    PVOID RandomBuffer,
    ULONG RandomBufferLength);

size_t RNG_SystemRNG(void *dest, size_t maxLen)
{
    HMODULE hModule;
    RtlGenRandomFn pRtlGenRandom;
    CryptAcquireContextAFn pCryptAcquireContextA;
    CryptReleaseContextFn pCryptReleaseContext;
    CryptGenRandomFn pCryptGenRandom;
    HCRYPTPROV hCryptProv;
    size_t bytes = 0;

    usedWindowsPRNG = PR_FALSE;
    hModule = LoadLibrary("advapi32.dll");
    if (hModule == NULL) {
	return rng_systemFromNoise(dest,maxLen);
    }
    pRtlGenRandom = (RtlGenRandomFn)
	GetProcAddress(hModule, "SystemFunction036");
    if (pRtlGenRandom) {
	if (pRtlGenRandom(dest, maxLen)) {
	    bytes = maxLen;
	    usedWindowsPRNG = PR_TRUE;
	} else {
	    bytes = rng_systemFromNoise(dest,maxLen);
	}
	goto done;
    }
    pCryptAcquireContextA = (CryptAcquireContextAFn)
	GetProcAddress(hModule, "CryptAcquireContextA");
    pCryptReleaseContext = (CryptReleaseContextFn)
	GetProcAddress(hModule, "CryptReleaseContext");
    pCryptGenRandom = (CryptGenRandomFn)
	GetProcAddress(hModule, "CryptGenRandom");
    if (!pCryptAcquireContextA || !pCryptReleaseContext || !pCryptGenRandom) {
	bytes = rng_systemFromNoise(dest,maxLen);
	goto done;
    }
    if (pCryptAcquireContextA(&hCryptProv, NULL, NULL,
	PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
	if (pCryptGenRandom(hCryptProv, maxLen, dest)) {
	    bytes = maxLen;
	    usedWindowsPRNG = PR_TRUE;
	}
	pCryptReleaseContext(hCryptProv, 0);
    }
    if (bytes == 0) {
	bytes = rng_systemFromNoise(dest,maxLen);
    }
done:
    FreeLibrary(hModule);
    return bytes;
}
#endif  

#endif  
