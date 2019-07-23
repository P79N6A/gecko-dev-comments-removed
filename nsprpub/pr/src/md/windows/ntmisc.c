









































#include "primpl.h"
#include <math.h>     
#include <windows.h>

char *_PR_MD_GET_ENV(const char *name)
{
    return getenv(name);
}






PRIntn _PR_MD_PUT_ENV(const char *name)
{
    return(putenv(name));
}
















#ifdef __GNUC__
const PRTime _pr_filetime_offset = 116444736000000000LL;
const PRTime _pr_filetime_divisor = 10LL;
#else
const PRTime _pr_filetime_offset = 116444736000000000i64;
const PRTime _pr_filetime_divisor = 10i64;
#endif

#ifdef WINCE

#define FILETIME_TO_INT64(ft) \
  (((PRInt64)ft.dwHighDateTime) << 32 | (PRInt64)ft.dwLowDateTime)

static void
LowResTime(LPFILETIME lpft)
{
    GetCurrentFT(lpft);
}

typedef struct CalibrationData {
    long double freq;         
    long double offset;       
    long double timer_offset; 

    
    PRInt64 last;

    PRBool calibrated;

    CRITICAL_SECTION data_lock;
    CRITICAL_SECTION calibration_lock;
    PRInt64 granularity;
} CalibrationData;

static CalibrationData calibration;

typedef void (*GetSystemTimeAsFileTimeFcn)(LPFILETIME);
static GetSystemTimeAsFileTimeFcn ce6_GetSystemTimeAsFileTime = NULL;

static void
NowCalibrate(void)
{
    FILETIME ft, ftStart;
    LARGE_INTEGER liFreq, now;

    if (calibration.freq == 0.0) {
	if(!QueryPerformanceFrequency(&liFreq)) {
	    
	    calibration.freq = -1.0;
	} else {
	    calibration.freq = (long double) liFreq.QuadPart;
	}
    }
    if (calibration.freq > 0.0) {
	PRInt64 calibrationDelta = 0;
	



	timeBeginPeriod(1);
	LowResTime(&ftStart);
	do {
	    LowResTime(&ft);
	} while (memcmp(&ftStart,&ft, sizeof(ft)) == 0);
	timeEndPeriod(1);

	calibration.granularity = 
	    (FILETIME_TO_INT64(ft) - FILETIME_TO_INT64(ftStart))/10;

	QueryPerformanceCounter(&now);

	calibration.offset = (long double) FILETIME_TO_INT64(ft);
	calibration.timer_offset = (long double) now.QuadPart;
	




	calibration.offset -= _pr_filetime_offset;
	calibration.offset *= 0.1;
	calibration.last = 0;

	calibration.calibrated = PR_TRUE;
    }
}

#define CALIBRATIONLOCK_SPINCOUNT 0
#define DATALOCK_SPINCOUNT 4096
#define LASTLOCK_SPINCOUNT 4096

void
_MD_InitTime(void)
{
    
    HANDLE h = GetModuleHandleW(L"coredll.dll");
    ce6_GetSystemTimeAsFileTime = (GetSystemTimeAsFileTimeFcn)
        GetProcAddressA(h, "GetSystemTimeAsFileTime");

    
    if (ce6_GetSystemTimeAsFileTime == NULL) {
        memset(&calibration, 0, sizeof(calibration));
        NowCalibrate();
        InitializeCriticalSection(&calibration.calibration_lock);
        InitializeCriticalSection(&calibration.data_lock);
    }
}

void
_MD_CleanupTime(void)
{
    if (ce6_GetSystemTimeAsFileTime == NULL) {
        DeleteCriticalSection(&calibration.calibration_lock);
        DeleteCriticalSection(&calibration.data_lock);
    }
}

#define MUTEX_SETSPINCOUNT(m, c)















PR_IMPLEMENT(PRTime)
PR_Now(void)
{
    long double lowresTime, highresTimerValue;
    FILETIME ft;
    LARGE_INTEGER now;
    PRBool calibrated = PR_FALSE;
    PRBool needsCalibration = PR_FALSE;
    PRInt64 returnedTime;
    long double cachedOffset = 0.0;

    if (ce6_GetSystemTimeAsFileTime) {
        union {
            FILETIME ft;
            PRTime prt;
        } currentTime;

        PR_ASSERT(sizeof(FILETIME) == sizeof(PRTime));

        ce6_GetSystemTimeAsFileTime(&currentTime.ft);

        


        return currentTime.prt/_pr_filetime_divisor -
            _pr_filetime_offset/_pr_filetime_divisor;
    }

    do {
	if (!calibration.calibrated || needsCalibration) {
	    EnterCriticalSection(&calibration.calibration_lock);
	    EnterCriticalSection(&calibration.data_lock);

	    
	    if (calibration.offset == cachedOffset) {
		



		MUTEX_SETSPINCOUNT(&calibration.data_lock, 0);

		NowCalibrate();

		calibrated = PR_TRUE;

		
		MUTEX_SETSPINCOUNT(&calibration.data_lock, DATALOCK_SPINCOUNT);
	    }
	    LeaveCriticalSection(&calibration.data_lock);
	    LeaveCriticalSection(&calibration.calibration_lock);
	}

	
	LowResTime(&ft);
	lowresTime =
            ((long double)(FILETIME_TO_INT64(ft) - _pr_filetime_offset)) * 0.1;

	if (calibration.freq > 0.0) {
	    long double highresTime, diff;
	    DWORD timeAdjustment, timeIncrement;
	    BOOL timeAdjustmentDisabled;

	    
	    long double skewThreshold = 15625.25;

	    
	    QueryPerformanceCounter(&now);
	    highresTimerValue = (long double)now.QuadPart;

	    EnterCriticalSection(&calibration.data_lock);
	    highresTime = calibration.offset + 1000000L *
		(highresTimerValue-calibration.timer_offset)/calibration.freq;
	    cachedOffset = calibration.offset;

	    



	    calibration.last = PR_MAX(calibration.last,(PRInt64)highresTime);
	    returnedTime = calibration.last;
	    LeaveCriticalSection(&calibration.data_lock);

	    
	    skewThreshold = calibration.granularity;
	    
	    diff = lowresTime - highresTime;

	    






	    if (fabs(diff) > 2*skewThreshold) {
		if (calibrated) {
		    










		    returnedTime = (PRInt64)lowresTime;
		    needsCalibration = PR_FALSE;
		} else {
		    










		    needsCalibration = PR_TRUE;
		}
	    } else {
		
		returnedTime = (PRInt64)highresTime;
		needsCalibration = PR_FALSE;
	    }
	} else {
	    
	    returnedTime = (PRInt64)lowresTime;
	}
    } while (needsCalibration);

    return returnedTime;
}

#else

PR_IMPLEMENT(PRTime)
PR_Now(void)
{
    PRTime prt;
    FILETIME ft;
    SYSTEMTIME st;

    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);
    _PR_FileTimeToPRTime(&ft, &prt);
    return prt;       
}

#endif
















static int assembleCmdLine(char *const *argv, char **cmdLine)
{
    char *const *arg;
    char *p, *q;
    size_t cmdLineSize;
    int numBackslashes;
    int i;
    int argNeedQuotes;

    


    cmdLineSize = 0;
    for (arg = argv; *arg; arg++) {
        






        cmdLineSize += 2 * strlen(*arg)  
                + 2                      
                + 1;                     
    }
    p = *cmdLine = PR_MALLOC((PRUint32) cmdLineSize);
    if (p == NULL) {
        return -1;
    }

    for (arg = argv; *arg; arg++) {
        
        if (arg != argv) {
            *p++ = ' '; 
        }
        q = *arg;
        numBackslashes = 0;
        argNeedQuotes = 0;

        



        if (**arg == '\0' || strpbrk(*arg, " \f\n\r\t\v")) {
            argNeedQuotes = 1;
        }

        if (argNeedQuotes) {
            *p++ = '"';
        }
        while (*q) {
            if (*q == '\\') {
                numBackslashes++;
                q++;
            } else if (*q == '"') {
                if (numBackslashes) {
                    



                    for (i = 0; i < 2 * numBackslashes; i++) {
                        *p++ = '\\';
                    }
                    numBackslashes = 0;
                }
                
                *p++ = '\\';
                *p++ = *q++;
            } else {
                if (numBackslashes) {
                    



                    for (i = 0; i < numBackslashes; i++) {
                        *p++ = '\\';
                    }
                    numBackslashes = 0;
                }
                *p++ = *q++;
            }
        }

        
        if (numBackslashes) {
            



            if (argNeedQuotes) {
                numBackslashes *= 2;
            }
            for (i = 0; i < numBackslashes; i++) {
                *p++ = '\\';
            }
        }
        if (argNeedQuotes) {
            *p++ = '"';
        }
    } 

    *p = '\0';
    return 0;
}










static int assembleEnvBlock(char **envp, char **envBlock)
{
    char *p;
    char *q;
    char **env;
    char *curEnv;
    char *cwdStart, *cwdEnd;
    size_t envBlockSize;

    if (envp == NULL) {
        *envBlock = NULL;
        return 0;
    }

#ifdef WINCE
    {
        PRUnichar *wideCurEnv = mozce_GetEnvString();
        int len = WideCharToMultiByte(CP_ACP, 0, wideCurEnv, -1,
                                      NULL, 0, NULL, NULL);
        curEnv = (char *) PR_MALLOC(len * sizeof(char));
        WideCharToMultiByte(CP_ACP, 0, wideCurEnv, -1,
                            curEnv, len, NULL, NULL);
        free(wideCurEnv);
    }
#else
    curEnv = GetEnvironmentStrings();
#endif

    cwdStart = curEnv;
    while (*cwdStart) {
        if (cwdStart[0] == '=' && cwdStart[1] != '\0'
                && cwdStart[2] == ':' && cwdStart[3] == '=') {
            break;
        }
        cwdStart += strlen(cwdStart) + 1;
    }
    cwdEnd = cwdStart;
    if (*cwdEnd) {
        cwdEnd += strlen(cwdEnd) + 1;
        while (*cwdEnd) {
            if (cwdEnd[0] != '=' || cwdEnd[1] == '\0'
                    || cwdEnd[2] != ':' || cwdEnd[3] != '=') {
                break;
            }
            cwdEnd += strlen(cwdEnd) + 1;
        }
    }
    envBlockSize = cwdEnd - cwdStart;

    for (env = envp; *env; env++) {
        envBlockSize += strlen(*env) + 1;
    }
    envBlockSize++;

    p = *envBlock = PR_MALLOC((PRUint32) envBlockSize);
    if (p == NULL) {
#ifdef WINCE
        PR_Free(curEnv);
#else
        FreeEnvironmentStrings(curEnv);
#endif
        return -1;
    }

    q = cwdStart;
    while (q < cwdEnd) {
        *p++ = *q++;
    }
#ifdef WINCE
    PR_Free(curEnv);
#else
    FreeEnvironmentStrings(curEnv);
#endif

    for (env = envp; *env; env++) {
        q = *env;
        while (*q) {
            *p++ = *q++;
        }
        *p++ = '\0';
    }
    *p = '\0';
    return 0;
}





static int compare(const void *arg1, const void *arg2)
{
    return _stricmp(* (char**)arg1, * (char**)arg2);
}

PRProcess * _PR_CreateWindowsProcess(
    const char *path,
    char *const *argv,
    char *const *envp,
    const PRProcessAttr *attr)
{
#ifdef WINCE
    STARTUPINFOW startupInfo;
    PRUnichar *wideCmdLine;
    PRUnichar *wideCwd;
    int len = 0;
#else
    STARTUPINFO startupInfo;
#endif
    PROCESS_INFORMATION procInfo;
    BOOL retVal;
    char *cmdLine = NULL;
    char *envBlock = NULL;
    char **newEnvp = NULL;
    const char *cwd = NULL; 
    PRProcess *proc = NULL;
    PRBool hasFdInheritBuffer;

    proc = PR_NEW(PRProcess);
    if (!proc) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        goto errorExit;
    }

    if (assembleCmdLine(argv, &cmdLine) == -1) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        goto errorExit;
    }

#ifndef WINCE
    



    hasFdInheritBuffer = (attr && attr->fdInheritBuffer);
    if ((envp == NULL) && hasFdInheritBuffer) {
        envp = environ;
    }

    if (envp != NULL) {
        int idx;
        int numEnv;
        PRBool found = PR_FALSE;

        numEnv = 0;
        while (envp[numEnv]) {
            numEnv++;
        }
        newEnvp = (char **) PR_MALLOC((numEnv + 2) * sizeof(char *));
        for (idx = 0; idx < numEnv; idx++) {
            newEnvp[idx] = envp[idx];
            if (hasFdInheritBuffer && !found
                    && !strncmp(newEnvp[idx], "NSPR_INHERIT_FDS=", 17)) {
                newEnvp[idx] = attr->fdInheritBuffer;
                found = PR_TRUE;
            }
        }
        if (hasFdInheritBuffer && !found) {
            newEnvp[idx++] = attr->fdInheritBuffer;
        }
        newEnvp[idx] = NULL;
        qsort((void *) newEnvp, (size_t) idx, sizeof(char *), compare);
    }
    if (assembleEnvBlock(newEnvp, &envBlock) == -1) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        goto errorExit;
    }

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    if (attr) {
        PRBool redirected = PR_FALSE;

        




        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        if (attr->stdinFd) {
            startupInfo.hStdInput = (HANDLE) attr->stdinFd->secret->md.osfd;
            redirected = PR_TRUE;
        }
        if (attr->stdoutFd) {
            startupInfo.hStdOutput = (HANDLE) attr->stdoutFd->secret->md.osfd;
            redirected = PR_TRUE;
        }
        if (attr->stderrFd) {
            startupInfo.hStdError = (HANDLE) attr->stderrFd->secret->md.osfd;
            redirected = PR_TRUE;
        }
        if (redirected) {
            startupInfo.dwFlags |= STARTF_USESTDHANDLES;
        }
        cwd = attr->currentDirectory;
    }
#endif

#ifdef WINCE
    len = MultiByteToWideChar(CP_ACP, 0, cmdLine, -1, NULL, 0);
    wideCmdLine = (PRUnichar *)PR_MALLOC(len * sizeof(PRUnichar));
    MultiByteToWideChar(CP_ACP, 0, cmdLine, -1, wideCmdLine, len);
    len = MultiByteToWideChar(CP_ACP, 0, cwd, -1, NULL, 0);
    wideCwd = PR_MALLOC(len * sizeof(PRUnichar));
    MultiByteToWideChar(CP_ACP, 0, cwd, -1, wideCwd, len);
    retVal = CreateProcessW(NULL,
                            wideCmdLine,
                            NULL,  

                            NULL,  

                            TRUE,  
                            0,     
                            envBlock,  





                            wideCwd,  
                            &startupInfo,
                            &procInfo
                           );
    PR_Free(wideCmdLine);
    PR_Free(wideCwd);
#else
    retVal = CreateProcess(NULL,
                           cmdLine,
                           NULL,  

                           NULL,  

                           TRUE,  
                           0,     
                           envBlock,  





                           cwd,  
                           &startupInfo,
                           &procInfo
                          );
#endif

    if (retVal == FALSE) {
        
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
        goto errorExit;
    }

    CloseHandle(procInfo.hThread);
    proc->md.handle = procInfo.hProcess;
    proc->md.id = procInfo.dwProcessId;

    PR_DELETE(cmdLine);
    if (newEnvp) {
        PR_DELETE(newEnvp);
    }
    if (envBlock) {
        PR_DELETE(envBlock);
    }
    return proc;

errorExit:
    if (cmdLine) {
        PR_DELETE(cmdLine);
    }
    if (newEnvp) {
        PR_DELETE(newEnvp);
    }
    if (envBlock) {
        PR_DELETE(envBlock);
    }
    if (proc) {
        PR_DELETE(proc);
    }
    return NULL;
}  

PRStatus _PR_DetachWindowsProcess(PRProcess *process)
{
    CloseHandle(process->md.handle);
    PR_DELETE(process);
    return PR_SUCCESS;
}





PRStatus _PR_WaitWindowsProcess(PRProcess *process,
    PRInt32 *exitCode)
{
    DWORD dwRetVal;

    dwRetVal = WaitForSingleObject(process->md.handle, INFINITE);
    if (dwRetVal == WAIT_FAILED) {
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
        return PR_FAILURE;
    }
    PR_ASSERT(dwRetVal == WAIT_OBJECT_0);
    if (exitCode != NULL &&
            GetExitCodeProcess(process->md.handle, exitCode) == FALSE) {
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
        return PR_FAILURE;
    }
    CloseHandle(process->md.handle);
    PR_DELETE(process);
    return PR_SUCCESS;
}

PRStatus _PR_KillWindowsProcess(PRProcess *process)
{
    




    if (TerminateProcess(process->md.handle, 256)) {
	return PR_SUCCESS;
    }
    PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
    return PR_FAILURE;
}

PRStatus _MD_WindowsGetHostName(char *name, PRUint32 namelen)
{
    PRIntn rv;
    PRInt32 syserror;

    rv = gethostname(name, (PRInt32) namelen);
    if (0 == rv) {
        return PR_SUCCESS;
    }
    syserror = WSAGetLastError();
    PR_ASSERT(WSANOTINITIALISED != syserror);
	_PR_MD_MAP_GETHOSTNAME_ERROR(syserror);
    return PR_FAILURE;
}

PRStatus _MD_WindowsGetSysInfo(PRSysInfo cmd, char *name, PRUint32 namelen)
{
	OSVERSIONINFO osvi;

	PR_ASSERT((cmd == PR_SI_SYSNAME) || (cmd == PR_SI_RELEASE));

	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (! GetVersionEx (&osvi) ) {
		_PR_MD_MAP_DEFAULT_ERROR(GetLastError());
    	return PR_FAILURE;
	}

	switch (osvi.dwPlatformId) {
		case VER_PLATFORM_WIN32_NT:
			if (PR_SI_SYSNAME == cmd)
				(void)PR_snprintf(name, namelen, "Windows_NT");
			else if (PR_SI_RELEASE == cmd)
				(void)PR_snprintf(name, namelen, "%d.%d",osvi.dwMajorVersion, 
            							osvi.dwMinorVersion);
			break;
		case VER_PLATFORM_WIN32_WINDOWS:
			if (PR_SI_SYSNAME == cmd) {
				if ((osvi.dwMajorVersion > 4) || 
					((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion > 0)))
					(void)PR_snprintf(name, namelen, "Windows_98");
				else
					(void)PR_snprintf(name, namelen, "Windows_95");
			} else if (PR_SI_RELEASE == cmd) {
				(void)PR_snprintf(name, namelen, "%d.%d",osvi.dwMajorVersion, 
            							osvi.dwMinorVersion);
			}
			break;
#ifdef VER_PLATFORM_WIN32_CE
    case VER_PLATFORM_WIN32_CE:
			if (PR_SI_SYSNAME == cmd)
				(void)PR_snprintf(name, namelen, "Windows_CE");
			else if (PR_SI_RELEASE == cmd)
				(void)PR_snprintf(name, namelen, "%d.%d",osvi.dwMajorVersion, 
            							osvi.dwMinorVersion);
			break;
#endif
   		default:
			if (PR_SI_SYSNAME == cmd)
				(void)PR_snprintf(name, namelen, "Windows_Unknown");
			else if (PR_SI_RELEASE == cmd)
				(void)PR_snprintf(name, namelen, "%d.%d",0,0);
			break;
	}
	return PR_SUCCESS;
}

PRStatus _MD_WindowsGetReleaseName(char *name, PRUint32 namelen)
{
	OSVERSIONINFO osvi;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (! GetVersionEx (&osvi) ) {
		_PR_MD_MAP_DEFAULT_ERROR(GetLastError());
    	return PR_FAILURE;
	}

	switch (osvi.dwPlatformId) {
		case VER_PLATFORM_WIN32_NT:
		case VER_PLATFORM_WIN32_WINDOWS:
			(void)PR_snprintf(name, namelen, "%d.%d",osvi.dwMajorVersion, 
            							osvi.dwMinorVersion);
			break;
   		default:
			(void)PR_snprintf(name, namelen, "%d.%d",0,0);
			break;
	}
	return PR_SUCCESS;
}









PRStatus _MD_CreateFileMap(PRFileMap *fmap, PRInt64 size)
{
    DWORD dwHi, dwLo;
    DWORD flProtect;
    PROsfd osfd;

    osfd = ( fmap->fd == (PRFileDesc*)-1 )?  -1 : fmap->fd->secret->md.osfd;

    dwLo = (DWORD) (size & 0xffffffff);
    dwHi = (DWORD) (((PRUint64) size >> 32) & 0xffffffff);

    if (fmap->prot == PR_PROT_READONLY) {
        flProtect = PAGE_READONLY;
        fmap->md.dwAccess = FILE_MAP_READ;
    } else if (fmap->prot == PR_PROT_READWRITE) {
        flProtect = PAGE_READWRITE;
        fmap->md.dwAccess = FILE_MAP_WRITE;
    } else {
        PR_ASSERT(fmap->prot == PR_PROT_WRITECOPY);
#ifdef WINCE
        
        PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
        return PR_FAILURE;
#else
        flProtect = PAGE_WRITECOPY;
        fmap->md.dwAccess = FILE_MAP_COPY;
#endif
    }

    fmap->md.hFileMap = CreateFileMapping(
        (HANDLE) osfd,
        NULL,
        flProtect,
        dwHi,
        dwLo,
        NULL);

    if (fmap->md.hFileMap == NULL) {
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

PRInt32 _MD_GetMemMapAlignment(void)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwAllocationGranularity;
}

extern PRLogModuleInfo *_pr_shma_lm;

void * _MD_MemMap(
    PRFileMap *fmap,
    PROffset64 offset,
    PRUint32 len)
{
    DWORD dwHi, dwLo;
    void *addr;

    dwLo = (DWORD) (offset & 0xffffffff);
    dwHi = (DWORD) (((PRUint64) offset >> 32) & 0xffffffff);
    if ((addr = MapViewOfFile(fmap->md.hFileMap, fmap->md.dwAccess,
            dwHi, dwLo, len)) == NULL) {
        {
            LPVOID lpMsgBuf; 
            
            FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                (LPTSTR) &lpMsgBuf,
                0,
                NULL 
            );
            PR_LOG( _pr_shma_lm, PR_LOG_DEBUG, ("md_memmap(): %s", lpMsgBuf ));
        }
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
    }
    return addr;
}

PRStatus _MD_MemUnmap(void *addr, PRUint32 len)
{
    if (UnmapViewOfFile(addr)) {
        return PR_SUCCESS;
    } else {
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
        return PR_FAILURE;
    }
}

PRStatus _MD_CloseFileMap(PRFileMap *fmap)
{
    CloseHandle(fmap->md.hFileMap);
    PR_DELETE(fmap);
    return PR_SUCCESS;
}



















#if defined(_M_IX86) || defined(_X86_)

#pragma warning(disable: 4035)
PRInt32 _PR_MD_ATOMIC_INCREMENT(PRInt32 *val)
{    
#if defined(__GNUC__)
  PRInt32 result;
  asm volatile ("lock ; xadd %0, %1" 
                : "=r"(result), "=m"(*val)
                : "0"(1), "m"(*val));
  return result + 1;
#else
    __asm
    {
        mov ecx, val
        mov eax, 1
        lock xadd dword ptr [ecx], eax
        inc eax
    }
#endif 
}
#pragma warning(default: 4035)

#pragma warning(disable: 4035)
PRInt32 _PR_MD_ATOMIC_DECREMENT(PRInt32 *val)
{
#if defined(__GNUC__)
  PRInt32 result;
  asm volatile ("lock ; xadd %0, %1" 
                : "=r"(result), "=m"(*val)
                : "0"(-1), "m"(*val));
  
  return result - 1;
#else
    __asm
    {
        mov ecx, val
        mov eax, 0ffffffffh
        lock xadd dword ptr [ecx], eax
        dec eax
    }
#endif 
}
#pragma warning(default: 4035)

#pragma warning(disable: 4035)
PRInt32 _PR_MD_ATOMIC_ADD(PRInt32 *intp, PRInt32 val)
{
#if defined(__GNUC__)
  PRInt32 result;
  
  asm volatile ("lock ; xadd %0, %1" 
                : "=r"(result), "=m"(*intp)
                : "0"(val), "m"(*intp));
  return result + val;
#else
    __asm
    {
        mov ecx, intp
        mov eax, val
        mov edx, eax
        lock xadd dword ptr [ecx], eax
        add eax, edx
    }
#endif 
}
#pragma warning(default: 4035)

#ifdef _PR_HAVE_ATOMIC_CAS

#pragma warning(disable: 4035)
void 
PR_StackPush(PRStack *stack, PRStackElem *stack_elem)
{
#if defined(__GNUC__)
  void **tos = (void **) stack;
  void *tmp;
  
 retry:
  if (*tos == (void *) -1)
    goto retry;
  
  __asm__("xchg %0,%1"
          : "=r" (tmp), "=m"(*tos)
          : "0" (-1), "m"(*tos));
  
  if (tmp == (void *) -1)
    goto retry;
  
  *(void **)stack_elem = tmp;
  __asm__("" : : : "memory");
  *tos = stack_elem;
#else
    __asm
    {
	mov ebx, stack
	mov ecx, stack_elem
retry:	mov eax,[ebx]
	cmp eax,-1
	je retry
	mov eax,-1
	xchg dword ptr [ebx], eax
	cmp eax,-1
	je  retry
	mov [ecx],eax
	mov [ebx],ecx
    }
#endif 
}
#pragma warning(default: 4035)

#pragma warning(disable: 4035)
PRStackElem * 
PR_StackPop(PRStack *stack)
{
#if defined(__GNUC__)
  void **tos = (void **) stack;
  void *tmp;
  
 retry:
  if (*tos == (void *) -1)
    goto retry;
  
  __asm__("xchg %0,%1"
          : "=r" (tmp), "=m"(*tos)
          : "0" (-1), "m"(*tos));

  if (tmp == (void *) -1)
    goto retry;
  
  if (tmp != (void *) 0)
    {
      void *next = *(void **)tmp;
      *tos = next;
      *(void **)tmp = 0;
    }
  else
    *tos = tmp;
  
  return tmp;
#else
    __asm
    {
	mov ebx, stack
retry:	mov eax,[ebx]
	cmp eax,-1
	je retry
	mov eax,-1
	xchg dword ptr [ebx], eax
	cmp eax,-1
	je  retry
	cmp eax,0
	je  empty
	mov ecx,[eax]
	mov [ebx],ecx
	mov [eax],0
	jmp done
empty:
	mov [ebx],eax
done:	
	}
#endif 
}
#pragma warning(default: 4035)

#endif 

#endif 
