







































#include "nscore.h"
#include <windows.h>
#include <stdio.h>
#include "nsStackFrameWin.h"
#include "plstr.h"









PR_BEGIN_EXTERN_C

SYMSETOPTIONSPROC _SymSetOptions;

SYMINITIALIZEPROC _SymInitialize;

SYMCLEANUPPROC _SymCleanup;

STACKWALKPROC _StackWalk;
#ifdef USING_WXP_VERSION
STACKWALKPROC64 _StackWalk64;
#else
#define _StackWalk64 0
#endif

SYMFUNCTIONTABLEACCESSPROC _SymFunctionTableAccess;
#ifdef USING_WXP_VERSION
SYMFUNCTIONTABLEACCESSPROC64 _SymFunctionTableAccess64;
#else
#define _SymFunctionTableAccess64 0
#endif

SYMGETMODULEBASEPROC _SymGetModuleBase;
#ifdef USING_WXP_VERSION
SYMGETMODULEBASEPROC64 _SymGetModuleBase64;
#else
#define _SymGetModuleBase64 0
#endif

SYMGETSYMFROMADDRPROC _SymGetSymFromAddr;
#ifdef USING_WXP_VERSION
SYMFROMADDRPROC _SymFromAddr;
#else
#define _SymFromAddr 0
#endif

SYMLOADMODULE _SymLoadModule;
#ifdef USING_WXP_VERSION
SYMLOADMODULE64 _SymLoadModule64;
#else
#define _SymLoadModule64 0
#endif

SYMUNDNAME _SymUnDName;

SYMGETMODULEINFO _SymGetModuleInfo;
#ifdef USING_WXP_VERSION
SYMGETMODULEINFO64 _SymGetModuleInfo64;
#else
#define _SymGetModuleInfo64 0
#endif

ENUMLOADEDMODULES _EnumerateLoadedModules;
#ifdef USING_WXP_VERSION
ENUMLOADEDMODULES64 _EnumerateLoadedModules64;
#else
#define _EnumerateLoadedModules64 0
#endif

SYMGETLINEFROMADDRPROC _SymGetLineFromAddr;
#ifdef USING_WXP_VERSION
SYMGETLINEFROMADDRPROC64 _SymGetLineFromAddr64;
#else
#define _SymGetLineFromAddr64 0
#endif

HANDLE hStackWalkMutex;

PR_END_EXTERN_C



void PrintError(char *prefix)
{
    LPVOID lpMsgBuf;
    DWORD lastErr = GetLastError();
    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      lastErr,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
      (LPTSTR) &lpMsgBuf,
      0,
      NULL
    );
    fprintf(stderr, "### ERROR: %s: %s", prefix, lpMsgBuf);
    fflush(stderr);
    LocalFree( lpMsgBuf );
}

PRBool
EnsureImageHlpInitialized()
{
    static PRBool gInitialized = PR_FALSE;

    if (gInitialized)
        return gInitialized;

    
    hStackWalkMutex = CreateMutex(
      NULL,                       
      FALSE,                      
      NULL);                      

    if (hStackWalkMutex == NULL) {
        PrintError("CreateMutex");
        return PR_FALSE;
    }

    HMODULE module = ::LoadLibrary("DBGHELP.DLL");
    if (!module) {
        module = ::LoadLibrary("IMAGEHLP.DLL");
        if (!module) return PR_FALSE;
    }

    _SymSetOptions = (SYMSETOPTIONSPROC) ::GetProcAddress(module, "SymSetOptions");
    if (!_SymSetOptions) return PR_FALSE;

    _SymInitialize = (SYMINITIALIZEPROC) ::GetProcAddress(module, "SymInitialize");
    if (!_SymInitialize) return PR_FALSE;

    _SymCleanup = (SYMCLEANUPPROC)GetProcAddress(module, "SymCleanup");
    if (!_SymCleanup) return PR_FALSE;

#ifdef USING_WXP_VERSION
    _StackWalk64 = (STACKWALKPROC64)GetProcAddress(module, "StackWalk64");
#endif
    _StackWalk = (STACKWALKPROC)GetProcAddress(module, "StackWalk");
    if (!_StackWalk64  && !_StackWalk) return PR_FALSE;

#ifdef USING_WXP_VERSION
    _SymFunctionTableAccess64 = (SYMFUNCTIONTABLEACCESSPROC64) GetProcAddress(module, "SymFunctionTableAccess64");
#endif
    _SymFunctionTableAccess = (SYMFUNCTIONTABLEACCESSPROC) GetProcAddress(module, "SymFunctionTableAccess");
    if (!_SymFunctionTableAccess64 && !_SymFunctionTableAccess) return PR_FALSE;

#ifdef USING_WXP_VERSION
    _SymGetModuleBase64 = (SYMGETMODULEBASEPROC64)GetProcAddress(module, "SymGetModuleBase64");
#endif
    _SymGetModuleBase = (SYMGETMODULEBASEPROC)GetProcAddress(module, "SymGetModuleBase");
    if (!_SymGetModuleBase64 && !_SymGetModuleBase) return PR_FALSE;

    _SymGetSymFromAddr = (SYMGETSYMFROMADDRPROC)GetProcAddress(module, "SymGetSymFromAddr");
#ifdef USING_WXP_VERSION
    _SymFromAddr = (SYMFROMADDRPROC)GetProcAddress(module, "SymFromAddr");
#endif
    if (!_SymFromAddr && !_SymGetSymFromAddr) return PR_FALSE;

#ifdef USING_WXP_VERSION
    _SymLoadModule64 = (SYMLOADMODULE64)GetProcAddress(module, "SymLoadModule64");
#endif
    _SymLoadModule = (SYMLOADMODULE)GetProcAddress(module, "SymLoadModule");
    if (!_SymLoadModule64 && !_SymLoadModule) return PR_FALSE;

    _SymUnDName = (SYMUNDNAME)GetProcAddress(module, "SymUnDName");
    if (!_SymUnDName) return PR_FALSE;

#ifdef USING_WXP_VERSION
    _SymGetModuleInfo64 = (SYMGETMODULEINFO64)GetProcAddress(module, "SymGetModuleInfo64");
#endif
    _SymGetModuleInfo = (SYMGETMODULEINFO)GetProcAddress(module, "SymGetModuleInfo");
    if (!_SymGetModuleInfo64 && !_SymGetModuleInfo) return PR_FALSE;

#ifdef USING_WXP_VERSION
    _EnumerateLoadedModules64 = (ENUMLOADEDMODULES64)GetProcAddress(module, "EnumerateLoadedModules64");
#endif
    _EnumerateLoadedModules = (ENUMLOADEDMODULES)GetProcAddress(module, "EnumerateLoadedModules");
    if (!_EnumerateLoadedModules64 && !_EnumerateLoadedModules) return PR_FALSE;

#ifdef USING_WXP_VERSION
    _SymGetLineFromAddr64 = (SYMGETLINEFROMADDRPROC64)GetProcAddress(module, "SymGetLineFromAddr64");
#endif
    _SymGetLineFromAddr = (SYMGETLINEFROMADDRPROC)GetProcAddress(module, "SymGetLineFromAddr");
    if (!_SymGetLineFromAddr64 && !_SymGetLineFromAddr) return PR_FALSE;

    return gInitialized = PR_TRUE;
}


static BOOL CALLBACK callbackEspecial(
  LPSTR aModuleName,
  DWORD aModuleBase,
  ULONG aModuleSize,
  PVOID aUserContext)
{
    BOOL retval = TRUE;
    DWORD addr = *(DWORD*)aUserContext;

    




    const BOOL addressIncreases = TRUE;

    


    if (addressIncreases
       ? (addr >= aModuleBase && addr <= (aModuleBase + aModuleSize))
       : (addr <= aModuleBase && addr >= (aModuleBase - aModuleSize))
        ) {
        retval = _SymLoadModule(GetCurrentProcess(), NULL, aModuleName, NULL, aModuleBase, aModuleSize);
    }

    return retval;
}

static BOOL CALLBACK callbackEspecial64(
  PTSTR aModuleName,
  DWORD64 aModuleBase,
  ULONG aModuleSize,
  PVOID aUserContext)
{
#ifdef USING_WXP_VERSION
    BOOL retval = TRUE;
    DWORD64 addr = *(DWORD64*)aUserContext;

    




    const BOOL addressIncreases = TRUE;

    


    if (addressIncreases
       ? (addr >= aModuleBase && addr <= (aModuleBase + aModuleSize))
       : (addr <= aModuleBase && addr >= (aModuleBase - aModuleSize))
        ) {
        retval = _SymLoadModule64(GetCurrentProcess(), NULL, aModuleName, NULL, aModuleBase, aModuleSize);
    }

    return retval;
#else
    return FALSE;
#endif
}










BOOL SymGetModuleInfoEspecial(HANDLE aProcess, DWORD aAddr, PIMAGEHLP_MODULE aModuleInfo, PIMAGEHLP_LINE aLineInfo)
{
    BOOL retval = FALSE;

    


    aModuleInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULE);
    if (nsnull != aLineInfo) {
      aLineInfo->SizeOfStruct = sizeof(IMAGEHLP_LINE);
    }

    



    retval = _SymGetModuleInfo(aProcess, aAddr, aModuleInfo);

    if (FALSE == retval) {
        BOOL enumRes = FALSE;

        



        enumRes = _EnumerateLoadedModules(aProcess, (PENUMLOADED_MODULES_CALLBACK)callbackEspecial, (PVOID)&aAddr);
        if (FALSE != enumRes)
        {
            



            retval = _SymGetModuleInfo(aProcess, aAddr, aModuleInfo);
        }
    }

    



    if (FALSE != retval && nsnull != aLineInfo && nsnull != _SymGetLineFromAddr) {
        DWORD displacement = 0;
        BOOL lineRes = FALSE;
        lineRes = _SymGetLineFromAddr(aProcess, aAddr, &displacement, aLineInfo);
    }

    return retval;
}

#ifdef USING_WXP_VERSION
BOOL SymGetModuleInfoEspecial64(HANDLE aProcess, DWORD64 aAddr, PIMAGEHLP_MODULE64 aModuleInfo, PIMAGEHLP_LINE64 aLineInfo)
{
    BOOL retval = FALSE;

    


    aModuleInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    if (nsnull != aLineInfo) {
        aLineInfo->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    }

    



    retval = _SymGetModuleInfo64(aProcess, aAddr, aModuleInfo);

    if (FALSE == retval) {
        BOOL enumRes = FALSE;

        



        enumRes = _EnumerateLoadedModules64(aProcess, (PENUMLOADED_MODULES_CALLBACK64)callbackEspecial64, (PVOID)&aAddr);
        if (FALSE != enumRes)
        {
            



            retval = _SymGetModuleInfo64(aProcess, aAddr, aModuleInfo);
        }
    }

    



    if (FALSE != retval && nsnull != aLineInfo && nsnull != _SymGetLineFromAddr64) {
        DWORD displacement = 0;
        BOOL lineRes = FALSE;
        lineRes = _SymGetLineFromAddr64(aProcess, aAddr, &displacement, aLineInfo);
    }

    return retval;
}
#endif

HANDLE
GetCurrentPIDorHandle()
{
    if (_SymGetModuleBase64)
        return GetCurrentProcess();  

    return (HANDLE) GetCurrentProcessId(); 
}

PRBool
EnsureSymInitialized()
{
    static PRBool gInitialized = PR_FALSE;
    PRBool retStat;

    if (gInitialized)
        return gInitialized;

    if (!EnsureImageHlpInitialized())
        return PR_FALSE;

    _SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    retStat = _SymInitialize(GetCurrentPIDorHandle(), NULL, TRUE);
    if (!retStat)
        PrintError("SymInitialize");

    gInitialized = retStat;
    

    return retStat;
}










EXPORT_XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, PRUint32 aSkipFrames,
             void *aClosure)
{
    HANDLE myProcess, myThread, walkerThread;
    DWORD walkerReturn;
    struct WalkStackData data;

    if (!EnsureSymInitialized())
        return NS_ERROR_FAILURE;

    
    if (!::DuplicateHandle(::GetCurrentProcess(),
                           ::GetCurrentProcess(),
                           ::GetCurrentProcess(),
                           &myProcess,
                           THREAD_ALL_ACCESS, FALSE, 0)) {
        PrintError("DuplicateHandle (process)");
        return NS_ERROR_FAILURE;
    }
    if (!::DuplicateHandle(::GetCurrentProcess(),
                           ::GetCurrentThread(),
                           ::GetCurrentProcess(),
                           &myThread,
                           THREAD_ALL_ACCESS, FALSE, 0)) {
        PrintError("DuplicateHandle (thread)");
        ::CloseHandle(myProcess);
        return NS_ERROR_FAILURE;
    }

    data.callback = aCallback;
    data.skipFrames = aSkipFrames;
    data.closure = aClosure;
    data.thread = myThread;
    data.process = myProcess;
    walkerThread = ::CreateThread( NULL, 0, WalkStackThread, (LPVOID) &data, 0, NULL ) ;
    if (walkerThread) {
        walkerReturn = ::WaitForSingleObject(walkerThread, 2000); 
        if (walkerReturn != WAIT_OBJECT_0) {
            PrintError("ThreadWait");
        }
        ::CloseHandle(walkerThread);
    }
    else {
        PrintError("ThreadCreate");
    }
    ::CloseHandle(myThread);
    ::CloseHandle(myProcess);
    return NS_OK;
}

DWORD WINAPI
WalkStackThread(LPVOID lpdata)
{
    struct WalkStackData *data = (WalkStackData *)lpdata;
    DWORD ret ;

    
    
    ret = ::SuspendThread( data->thread );
    if (ret == -1) {
        PrintError("ThreadSuspend");
    }
    else {
        if (_StackWalk64)
            WalkStackMain64(data);
        else
            WalkStackMain(data);
        ret = ::ResumeThread(data->thread);
        if (ret == -1) {
            PrintError("ThreadResume");
        }
    }

    return 0;
}

void
WalkStackMain64(struct WalkStackData* data)
{
#ifdef USING_WXP_VERSION
    
    
    
    CONTEXT context;
    HANDLE myProcess = data->process;
    HANDLE myThread = data->thread;
    DWORD64 addr;
    STACKFRAME64 frame64;
    int skip = 3 + data->skipFrames; 
    BOOL ok;

    
    memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(myThread, &context)) {
        PrintError("GetThreadContext");
        return;
    }

    
    memset(&frame64, 0, sizeof(frame64));
#ifdef _M_IX86
    frame64.AddrPC.Offset    = context.Eip;
    frame64.AddrStack.Offset = context.Esp;
    frame64.AddrFrame.Offset = context.Ebp;
#elif defined _M_AMD64
    frame64.AddrPC.Offset    = context.Rip;
    frame64.AddrStack.Offset = context.Rsp;
    frame64.AddrFrame.Offset = context.Rbp;
#elif defined _M_IA64
    frame64.AddrPC.Offset    = context.StIIP;
    frame64.AddrStack.Offset = context.SP;
    frame64.AddrFrame.Offset = context.RsBSP;
#else
    PrintError("Unknown platform. No stack walking.");
    return;
#endif
    frame64.AddrPC.Mode      = AddrModeFlat;
    frame64.AddrStack.Mode   = AddrModeFlat;
    frame64.AddrFrame.Mode   = AddrModeFlat;
    frame64.AddrReturn.Mode  = AddrModeFlat;

    
    while (1) {

        ok = 0;

        
        DWORD dwWaitResult;
        dwWaitResult = WaitForSingleObject(hStackWalkMutex, INFINITE);
        if (dwWaitResult == WAIT_OBJECT_0) {

            ok = _StackWalk64(
#ifdef _M_AMD64
              IMAGE_FILE_MACHINE_AMD64,
#elif defined _M_IA64
              IMAGE_FILE_MACHINE_IA64,
#elif defined _M_IX86
              IMAGE_FILE_MACHINE_I386,
#else
              0,
#endif
              myProcess,
              myThread,
              &frame64,
              &context,
              NULL,
              _SymFunctionTableAccess64, 
              _SymGetModuleBase64,       
              0
            );

            ReleaseMutex(hStackWalkMutex);  

            if (ok)
                addr = frame64.AddrPC.Offset;
            else {
                addr = 0;
                PrintError("WalkStack64");
            }

            if (!ok || (addr == 0)) {
                break;
            }

            if (skip-- > 0) {
                continue;
            }

            (*data->callback)((void*)addr, data->closure);

#if 0
            
            if (strcmp(modInfo.ModuleName, "kernel32") == 0)
                break;
#else
            if (frame64.AddrReturn.Offset == 0)
                break;
#endif
        }
        else {
            PrintError("LockError64");
        } 
    }
    return;
#endif
}


void
WalkStackMain(struct WalkStackData* data)
{
    
    
    
    CONTEXT context;
    HANDLE myProcess = data->process;
    HANDLE myThread = data->thread;
    DWORD addr;
    STACKFRAME frame;
    int skip = data->skipFrames; 
    BOOL ok;

    
    memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(myThread, &context)) {
        PrintError("GetThreadContext");
        return;
    }

    
#if defined _M_IX86
    memset(&frame, 0, sizeof(frame));
    frame.AddrPC.Offset    = context.Eip;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrStack.Mode   = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrFrame.Mode   = AddrModeFlat;
#else
    PrintError("Unknown platform. No stack walking.");
    return;
#endif

    
    while (1) {

        ok = 0;

        
        DWORD dwWaitResult;
        dwWaitResult = WaitForSingleObject(hStackWalkMutex, INFINITE);
        if (dwWaitResult == WAIT_OBJECT_0) {

            ok = _StackWalk(
                IMAGE_FILE_MACHINE_I386,
                myProcess,
                myThread,
                &frame,
                &context,
                0,                        
                _SymFunctionTableAccess,  
                _SymGetModuleBase,        
                0                         
              );

            ReleaseMutex(hStackWalkMutex);  

            if (ok)
                addr = frame.AddrPC.Offset;
            else {
                addr = 0;
                PrintError("WalkStack");
            }

            if (!ok || (addr == 0)) {
                break;
            }

            if (skip-- > 0) {
                continue;
            }

            (*data->callback)((void*)addr, data->closure);

#if 0
            
            if (strcmp(modInfo.ImageName, "kernel32.dll") == 0)
                break;
#else
            if (frame.AddrReturn.Offset == 0)
                break;
#endif
        }
        else {
            PrintError("LockError");
        }
        
    }

    return;

}

EXPORT_XPCOM_API(nsresult)
NS_DescribeCodeAddress(void *aPC, nsCodeAddressDetails *aDetails)
{
    aDetails->library[0] = '\0';
    aDetails->loffset = 0;
    aDetails->filename[0] = '\0';
    aDetails->lineno = 0;
    aDetails->function[0] = '\0';
    aDetails->foffset = 0;

    HANDLE myProcess = ::GetCurrentProcess();
    BOOL ok;

    
    DWORD dwWaitResult;
    dwWaitResult = WaitForSingleObject(hStackWalkMutex, INFINITE);
    if (dwWaitResult != WAIT_OBJECT_0)
        return NS_ERROR_UNEXPECTED;

#ifdef USING_WXP_VERSION
    if (_StackWalk64) {
        
        
        
        

        DWORD64 addr = (DWORD64)aPC;
        IMAGEHLP_MODULE64 modInfo;
        modInfo.SizeOfStruct = sizeof(modInfo);
        BOOL modInfoRes;
        modInfoRes = SymGetModuleInfoEspecial64(myProcess, addr, &modInfo, nsnull);

        if (modInfoRes) {
            PL_strncpyz(aDetails->library, modInfo.ModuleName,
                        sizeof(aDetails->library));
            aDetails->loffset = (char*) aPC - (char*) modInfo.BaseOfImage;
            
            
        }

        ULONG64 buffer[(sizeof(SYMBOL_INFO) +
          MAX_SYM_NAME*sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement;
        ok = _SymFromAddr && _SymFromAddr(myProcess, addr, &displacement, pSymbol);

        if (ok) {
            PL_strncpyz(aDetails->function, pSymbol->Name,
                        sizeof(aDetails->function));
            aDetails->foffset = displacement;
        }
    } else
#endif
    {
        
        
        
        

        DWORD addr = (DWORD)aPC;
        IMAGEHLP_MODULE modInfo;
        modInfo.SizeOfStruct = sizeof(modInfo);
        BOOL modInfoRes;
        modInfoRes = SymGetModuleInfoEspecial(myProcess, addr, &modInfo, nsnull);

        if (modInfoRes) {
            PL_strncpyz(aDetails->library, modInfo.ModuleName,
                        sizeof(aDetails->library));
            aDetails->loffset = (char*) aPC - (char*) modInfo.BaseOfImage;
            
            
        }

#ifdef USING_WXP_VERSION
        ULONG64 buffer[(sizeof(SYMBOL_INFO) +
          MAX_SYM_NAME*sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement;

        ok = _SymFromAddr && _SymFromAddr(myProcess, addr, &displacement, pSymbol);
#else
        char buf[sizeof(IMAGEHLP_SYMBOL) + 512];
        PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL) buf;
        pSymbol->SizeOfStruct = sizeof(buf);
        pSymbol->MaxNameLength = 512;

        DWORD displacement;

        ok = _SymGetSymFromAddr(myProcess,
                    frame.AddrPC.Offset,
                    &displacement,
                    pSymbol);
#endif

        if (ok) {
            PL_strncpyz(aDetails->function, pSymbol->Name,
                        sizeof(aDetails->function));
            aDetails->foffset = displacement;
        }
    }

    ReleaseMutex(hStackWalkMutex);  
    return NS_OK;
}

EXPORT_XPCOM_API(nsresult)
NS_FormatCodeAddressDetails(void *aPC, const nsCodeAddressDetails *aDetails,
                            char *aBuffer, PRUint32 aBufferSize)
{
#ifdef USING_WXP_VERSION
    if (_StackWalk64) {
        if (aDetails->function[0])
            _snprintf(aBuffer, aBufferSize, "%s!%s+0x%016lX\n",
                      aDetails->library, aDetails->function, aDetails->foffset);
        else
            _snprintf(aBuffer, aBufferSize, "0x%016lX\n", aPC);
    } else {
#endif
        if (aDetails->function[0])
            _snprintf(aBuffer, aBufferSize, "%s!%s+0x%08lX\n",
                      aDetails->library, aDetails->function, aDetails->foffset);
        else
            _snprintf(aBuffer, aBufferSize, "0x%08lX\n", aPC);
#ifdef USING_WXP_VERSION
    }
#endif
    return NS_OK;
}
