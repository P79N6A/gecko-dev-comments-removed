









































#include "nsStackWalk.h"

#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_IA64)) 

#include "nscore.h"
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include "plstr.h"
#include "nsMemory.h" 
#include "mozilla/FunctionTimer.h"

#include "nspr.h"
#if defined(_M_IX86) || defined(_M_AMD64)
#include <imagehlp.h>



#if API_VERSION_NUMBER >= 9
#define USING_WXP_VERSION 1
#endif
#endif







PR_BEGIN_EXTERN_C

typedef DWORD (__stdcall *SYMSETOPTIONSPROC)(DWORD);
extern SYMSETOPTIONSPROC _SymSetOptions;

typedef BOOL (__stdcall *SYMINITIALIZEPROC)(HANDLE, LPSTR, BOOL);
extern SYMINITIALIZEPROC _SymInitialize;

typedef BOOL (__stdcall *SYMCLEANUPPROC)(HANDLE);
extern SYMCLEANUPPROC _SymCleanup;

typedef BOOL (__stdcall *STACKWALKPROC)(DWORD,
                                        HANDLE,
                                        HANDLE,
                                        LPSTACKFRAME,
                                        LPVOID,
                                        PREAD_PROCESS_MEMORY_ROUTINE,
                                        PFUNCTION_TABLE_ACCESS_ROUTINE,
                                        PGET_MODULE_BASE_ROUTINE,
                                        PTRANSLATE_ADDRESS_ROUTINE);
extern  STACKWALKPROC _StackWalk;

#ifdef USING_WXP_VERSION
typedef BOOL (__stdcall *STACKWALKPROC64)(DWORD,
                                          HANDLE,
                                          HANDLE,
                                          LPSTACKFRAME64,
                                          PVOID,
                                          PREAD_PROCESS_MEMORY_ROUTINE64,
                                          PFUNCTION_TABLE_ACCESS_ROUTINE64,
                                          PGET_MODULE_BASE_ROUTINE64,
                                          PTRANSLATE_ADDRESS_ROUTINE64);
extern  STACKWALKPROC64 _StackWalk64;
#endif

typedef LPVOID (__stdcall *SYMFUNCTIONTABLEACCESSPROC)(HANDLE, DWORD);
extern  SYMFUNCTIONTABLEACCESSPROC _SymFunctionTableAccess;

#ifdef USING_WXP_VERSION
typedef LPVOID (__stdcall *SYMFUNCTIONTABLEACCESSPROC64)(HANDLE, DWORD64);
extern  SYMFUNCTIONTABLEACCESSPROC64 _SymFunctionTableAccess64;
#endif

typedef DWORD (__stdcall *SYMGETMODULEBASEPROC)(HANDLE, DWORD);
extern  SYMGETMODULEBASEPROC _SymGetModuleBase;

#ifdef USING_WXP_VERSION
typedef DWORD64 (__stdcall *SYMGETMODULEBASEPROC64)(HANDLE, DWORD64);
extern  SYMGETMODULEBASEPROC64 _SymGetModuleBase64;
#endif

typedef BOOL (__stdcall *SYMGETSYMFROMADDRPROC)(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL);
extern  SYMGETSYMFROMADDRPROC _SymGetSymFromAddr;

#ifdef USING_WXP_VERSION
typedef BOOL (__stdcall *SYMFROMADDRPROC)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
extern  SYMFROMADDRPROC _SymFromAddr;
#endif

typedef DWORD ( __stdcall *SYMLOADMODULE)(HANDLE, HANDLE, PSTR, PSTR, DWORD, DWORD);
extern  SYMLOADMODULE _SymLoadModule;

#ifdef USING_WXP_VERSION
typedef DWORD ( __stdcall *SYMLOADMODULE64)(HANDLE, HANDLE, PCSTR, PCSTR, DWORD64, DWORD);
extern  SYMLOADMODULE64 _SymLoadModule64;
#endif

typedef DWORD ( __stdcall *SYMUNDNAME)(PIMAGEHLP_SYMBOL, PSTR, DWORD);
extern  SYMUNDNAME _SymUnDName;

typedef DWORD ( __stdcall *SYMGETMODULEINFO)( HANDLE, DWORD, PIMAGEHLP_MODULE);
extern  SYMGETMODULEINFO _SymGetModuleInfo;

#ifdef USING_WXP_VERSION
typedef BOOL ( __stdcall *SYMGETMODULEINFO64)( HANDLE, DWORD64, PIMAGEHLP_MODULE64);
extern  SYMGETMODULEINFO64 _SymGetModuleInfo64;
#endif

typedef BOOL ( __stdcall *ENUMLOADEDMODULES)( HANDLE, PENUMLOADED_MODULES_CALLBACK, PVOID);
extern  ENUMLOADEDMODULES _EnumerateLoadedModules;

#ifdef USING_WXP_VERSION
typedef BOOL ( __stdcall *ENUMLOADEDMODULES64)( HANDLE, PENUMLOADED_MODULES_CALLBACK64, PVOID);
extern  ENUMLOADEDMODULES64 _EnumerateLoadedModules64;
#endif

typedef BOOL (__stdcall *SYMGETLINEFROMADDRPROC)(HANDLE, DWORD, PDWORD, PIMAGEHLP_LINE);
extern  SYMGETLINEFROMADDRPROC _SymGetLineFromAddr;

#ifdef USING_WXP_VERSION
typedef BOOL (__stdcall *SYMGETLINEFROMADDRPROC64)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
extern  SYMGETLINEFROMADDRPROC64 _SymGetLineFromAddr64;
#endif

extern HANDLE hStackWalkMutex; 

HANDLE GetCurrentPIDorHandle();

PRBool EnsureSymInitialized();

PRBool EnsureImageHlpInitialized();











BOOL SymGetModuleInfoEspecial(HANDLE aProcess, DWORD aAddr, PIMAGEHLP_MODULE aModuleInfo, PIMAGEHLP_LINE aLineInfo);

struct WalkStackData {
  PRUint32 skipFrames;
  HANDLE thread;
  HANDLE process;
  HANDLE eventStart;
  HANDLE eventEnd;
  void **pcs;
  PRUint32 pc_size;
  PRUint32 pc_count;
};

void PrintError(char *prefix, WalkStackData* data);
unsigned int WINAPI WalkStackThread(void* data);
void WalkStackMain64(struct WalkStackData* data);
#if !defined(_WIN64)
void WalkStackMain(struct WalkStackData* data);
#endif









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

DWORD gStackWalkThread;
CRITICAL_SECTION gDbgHelpCS;

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
    fprintf(stderr, "### ERROR: %s: %s",
                    prefix, lpMsgBuf ? lpMsgBuf : "(null)\n");
    fflush(stderr);
    LocalFree( lpMsgBuf );
}

PRBool
EnsureImageHlpInitialized()
{
    static PRBool gInitialized = PR_FALSE;

    if (gInitialized)
        return gInitialized;

    
    
    
    
    
    HANDLE readyEvent = ::CreateEvent(NULL, FALSE ,
                            FALSE , NULL);
    unsigned int threadID;
    HANDLE hStackWalkThread = (HANDLE)
      _beginthreadex(NULL, 0, WalkStackThread, (void*)readyEvent,
                     0, &threadID);
    gStackWalkThread = threadID;
    if (hStackWalkThread == NULL) {
        PrintError("CreateThread");
        return PR_FALSE;
    }
    ::CloseHandle(hStackWalkThread);

    
    ::WaitForSingleObject(readyEvent, INFINITE);
    ::CloseHandle(readyEvent);

    ::InitializeCriticalSection(&gDbgHelpCS);

    HMODULE module = ::LoadLibraryW(L"DBGHELP.DLL");
    if (!module) {
        module = ::LoadLibraryW(L"IMAGEHLP.DLL");
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
#error "Should not have compiled this code"
#endif
    frame64.AddrPC.Mode      = AddrModeFlat;
    frame64.AddrStack.Mode   = AddrModeFlat;
    frame64.AddrFrame.Mode   = AddrModeFlat;
    frame64.AddrReturn.Mode  = AddrModeFlat;

    
    while (1) {

        
        EnterCriticalSection(&gDbgHelpCS);
        ok = _StackWalk64(
#ifdef _M_AMD64
          IMAGE_FILE_MACHINE_AMD64,
#elif defined _M_IA64
          IMAGE_FILE_MACHINE_IA64,
#elif defined _M_IX86
          IMAGE_FILE_MACHINE_I386,
#else
#error "Should not have compiled this code"
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
        LeaveCriticalSection(&gDbgHelpCS);

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

        if (data->pc_count < data->pc_size)
            data->pcs[data->pc_count] = (void*)addr;
        ++data->pc_count;

        if (frame64.AddrReturn.Offset == 0)
            break;
    }
    return;
#endif
}


#if !defined(_WIN64)
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

        
        EnterCriticalSection(&gDbgHelpCS);
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
        LeaveCriticalSection(&gDbgHelpCS);

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

        if (data->pc_count < data->pc_size)
            data->pcs[data->pc_count] = (void*)addr;
        ++data->pc_count;

        if (frame.AddrReturn.Offset == 0)
            break;
    }

    return;

}
#endif

unsigned int WINAPI
WalkStackThread(void* aData)
{
    BOOL msgRet;
    MSG msg;

    
    
    ::PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    
    HANDLE readyEvent = (HANDLE)aData;
    ::SetEvent(readyEvent);

    while ((msgRet = ::GetMessage(&msg, (HWND)-1, 0, 0)) != 0) {
        if (msgRet == -1) {
            PrintError("GetMessage");
        } else {
            DWORD ret;

            struct WalkStackData *data = (WalkStackData *)msg.lParam;
            if (!data)
              continue;

            
            
            ret = ::WaitForSingleObject(data->eventStart, INFINITE);
            if (ret != WAIT_OBJECT_0)
                PrintError("WaitForSingleObject");

            
            
            ret = ::SuspendThread( data->thread );
            if (ret == -1) {
                PrintError("ThreadSuspend");
            }
            else {
#if defined(_WIN64)
                WalkStackMain64(data);
#else
                if (_StackWalk64)
                    WalkStackMain64(data);
                else
                    WalkStackMain(data);
#endif

                ret = ::ResumeThread(data->thread);
                if (ret == -1) {
                    PrintError("ThreadResume");
                }
            }

            ::SetEvent(data->eventEnd);
        }
    }

    return 0;
}









EXPORT_XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, PRUint32 aSkipFrames,
             void *aClosure)
{
    HANDLE myProcess, myThread;
    DWORD walkerReturn;
    struct WalkStackData data;

    if (!EnsureImageHlpInitialized())
        return PR_FALSE;

    
    if (!::DuplicateHandle(::GetCurrentProcess(),
                           ::GetCurrentProcess(),
                           ::GetCurrentProcess(),
                           &myProcess,
                           PROCESS_ALL_ACCESS, FALSE, 0)) {
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

    data.skipFrames = aSkipFrames;
    data.thread = myThread;
    data.process = myProcess;
    data.eventStart = ::CreateEvent(NULL, FALSE ,
                          FALSE , NULL);
    data.eventEnd = ::CreateEvent(NULL, FALSE ,
                        FALSE , NULL);
    void *local_pcs[1024];
    data.pcs = local_pcs;
    data.pc_count = 0;
    data.pc_size = NS_ARRAY_LENGTH(local_pcs);

    ::PostThreadMessage(gStackWalkThread, WM_USER, 0, (LPARAM)&data);

    walkerReturn = ::SignalObjectAndWait(data.eventStart,
                       data.eventEnd, INFINITE, FALSE);
    if (walkerReturn != WAIT_OBJECT_0)
        PrintError("SignalObjectAndWait (1)");
    if (data.pc_count > data.pc_size) {
        data.pcs = (void**) malloc(data.pc_count * sizeof(void*));
        data.pc_size = data.pc_count;
        data.pc_count = 0;
        ::PostThreadMessage(gStackWalkThread, WM_USER, 0, (LPARAM)&data);
        walkerReturn = ::SignalObjectAndWait(data.eventStart,
                           data.eventEnd, INFINITE, FALSE);
        if (walkerReturn != WAIT_OBJECT_0)
            PrintError("SignalObjectAndWait (2)");
    }

    ::CloseHandle(myThread);
    ::CloseHandle(myProcess);
    ::CloseHandle(data.eventStart);
    ::CloseHandle(data.eventEnd);

    for (PRUint32 i = 0; i < data.pc_count; ++i)
        (*aCallback)(data.pcs[i], aClosure);

    if (data.pc_size > NS_ARRAY_LENGTH(local_pcs))
        free(data.pcs);

    return NS_OK;
}


static BOOL CALLBACK callbackEspecial(
  PCSTR aModuleName,
  ULONG aModuleBase,
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
        retval = _SymLoadModule(GetCurrentProcess(), NULL, (PSTR)aModuleName, NULL, aModuleBase, aModuleSize);
        if (!retval)
            PrintError("SymLoadModule");
    }

    return retval;
}

static BOOL CALLBACK callbackEspecial64(
  PCSTR aModuleName,
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
        retval = _SymLoadModule64(GetCurrentProcess(), NULL, (PSTR)aModuleName, NULL, aModuleBase, aModuleSize);
        if (!retval)
            PrintError("SymLoadModule64");
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
            if (!retval)
                PrintError("SymGetModuleInfo");
        }
    }

    



    if (FALSE != retval && nsnull != aLineInfo && nsnull != _SymGetLineFromAddr) {
        DWORD displacement = 0;
        BOOL lineRes = FALSE;
        lineRes = _SymGetLineFromAddr(aProcess, aAddr, &displacement, aLineInfo);
    }

    return retval;
}










#ifdef SSRVOPT_SETCONTEXT
#define NS_IMAGEHLP_MODULE64_SIZE (((offsetof(IMAGEHLP_MODULE64, LoadedPdbName) + sizeof(DWORD64) - 1) / sizeof(DWORD64)) * sizeof(DWORD64))
#else
#define NS_IMAGEHLP_MODULE64_SIZE sizeof(IMAGEHLP_MODULE64)
#endif

#ifdef USING_WXP_VERSION
BOOL SymGetModuleInfoEspecial64(HANDLE aProcess, DWORD64 aAddr, PIMAGEHLP_MODULE64 aModuleInfo, PIMAGEHLP_LINE64 aLineInfo)
{
    BOOL retval = FALSE;

    


    aModuleInfo->SizeOfStruct = NS_IMAGEHLP_MODULE64_SIZE;
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
            if (!retval)
                PrintError("SymGetModuleInfo64");
        }
    }

    



    if (FALSE != retval && nsnull != aLineInfo && nsnull != _SymGetLineFromAddr64) {
        DWORD displacement = 0;
        BOOL lineRes = FALSE;
        lineRes = _SymGetLineFromAddr64(aProcess, aAddr, &displacement, aLineInfo);
        if (!lineRes) {
            
            memset(aLineInfo, 0, sizeof(*aLineInfo));
        }
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

    NS_TIME_FUNCTION;

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
NS_DescribeCodeAddress(void *aPC, nsCodeAddressDetails *aDetails)
{
    aDetails->library[0] = '\0';
    aDetails->loffset = 0;
    aDetails->filename[0] = '\0';
    aDetails->lineno = 0;
    aDetails->function[0] = '\0';
    aDetails->foffset = 0;

    if (!EnsureSymInitialized())
        return NS_ERROR_FAILURE;

    HANDLE myProcess = ::GetCurrentProcess();
    BOOL ok;

    
    EnterCriticalSection(&gDbgHelpCS);

#ifdef USING_WXP_VERSION
    if (_StackWalk64) {
        
        
        
        

        DWORD64 addr = (DWORD64)aPC;
        IMAGEHLP_MODULE64 modInfo;
        IMAGEHLP_LINE64 lineInfo;
        BOOL modInfoRes;
        modInfoRes = SymGetModuleInfoEspecial64(myProcess, addr, &modInfo, &lineInfo);

        if (modInfoRes) {
            PL_strncpyz(aDetails->library, modInfo.ModuleName,
                        sizeof(aDetails->library));
            aDetails->loffset = (char*) aPC - (char*) modInfo.BaseOfImage;
            
            if (lineInfo.FileName) {
                PL_strncpyz(aDetails->filename, lineInfo.FileName,
                            sizeof(aDetails->filename));
                aDetails->lineno = lineInfo.LineNumber;
            }
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
        
        
        
        

        DWORD_PTR addr = (DWORD_PTR)aPC;
        IMAGEHLP_MODULE modInfo;
        IMAGEHLP_LINE lineInfo;
        BOOL modInfoRes;
        modInfoRes = SymGetModuleInfoEspecial(myProcess, addr, &modInfo, &lineInfo);

        if (modInfoRes) {
            PL_strncpyz(aDetails->library, modInfo.ModuleName,
                        sizeof(aDetails->library));
            aDetails->loffset = (char*) aPC - (char*) modInfo.BaseOfImage;
            PL_strncpyz(aDetails->filename, lineInfo.FileName,
                        sizeof(aDetails->filename));
            aDetails->lineno = lineInfo.LineNumber;
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

    LeaveCriticalSection(&gDbgHelpCS); 
    return NS_OK;
}

EXPORT_XPCOM_API(nsresult)
NS_FormatCodeAddressDetails(void *aPC, const nsCodeAddressDetails *aDetails,
                            char *aBuffer, PRUint32 aBufferSize)
{
#ifdef USING_WXP_VERSION
    if (_StackWalk64) {
        if (aDetails->function[0])
            _snprintf(aBuffer, aBufferSize, "%s!%s+0x%016lX",
                      aDetails->library, aDetails->function, aDetails->foffset);
        else
            _snprintf(aBuffer, aBufferSize, "0x%016lX", aPC);
    } else {
#endif
        if (aDetails->function[0])
            _snprintf(aBuffer, aBufferSize, "%s!%s+0x%08lX",
                      aDetails->library, aDetails->function, aDetails->foffset);
        else
            _snprintf(aBuffer, aBufferSize, "0x%08lX", aPC);
#ifdef USING_WXP_VERSION
    }
#endif
    aBuffer[aBufferSize - 1] = '\0';

    PRUint32 len = strlen(aBuffer);
    if (aDetails->filename[0]) {
        _snprintf(aBuffer + len, aBufferSize - len, " (%s, line %d)\n",
                  aDetails->filename, aDetails->lineno);
    } else {
        aBuffer[len] = '\n';
        if (++len != aBufferSize)
            aBuffer[len] = '\0';
    }
    aBuffer[aBufferSize - 2] = '\n';
    aBuffer[aBufferSize - 1] = '\0';
    return NS_OK;
}



#elif HAVE_DLADDR && (HAVE__UNWIND_BACKTRACE || (defined(linux) && defined(__GNUC__) && (defined(__i386) || defined(PPC))) || (defined(__sun) && (defined(__sparc) || defined(sparc) || defined(__i386) || defined(i386))) || (defined(XP_MACOSX) && (defined(__ppc__) || defined(__i386))))

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nscore.h"
#include <stdio.h>
#include "plstr.h"





#if (__GLIBC_MINOR__ >= 1) && !defined(__USE_GNU)
#define __USE_GNU
#endif

#if defined(HAVE_DLOPEN) || defined(XP_MACOSX)
#include <dlfcn.h>
#endif





#if defined(MOZ_DEMANGLE_SYMBOLS)
#include <cxxabi.h>
#include <stdlib.h> 
#endif 

void DemangleSymbol(const char * aSymbol, 
                    char * aBuffer,
                    int aBufLen)
{
    aBuffer[0] = '\0';

#if defined(MOZ_DEMANGLE_SYMBOLS)
    
    char * demangled = abi::__cxa_demangle(aSymbol,0,0,0);
    
    if (demangled)
    {
        strncpy(aBuffer,demangled,aBufLen);
        free(demangled);
    }
#endif 
}


#if defined(__sun) && (defined(__sparc) || defined(sparc) || defined(__i386) || defined(i386))





#include <synch.h>
#include <ucontext.h>
#include <sys/frame.h>
#include <sys/regset.h>
#include <sys/stack.h>

static int    load_address ( void * pc, void * arg );
static struct bucket * newbucket ( void * pc );
static struct frame * cs_getmyframeptr ( void );
static void   cs_walk_stack ( void * (*read_func)(char * address),
                              struct frame * fp,
                              int (*operate_func)(void *, void *),
                              void * usrarg );
static void   cs_operate ( void (*operate_func)(void *, void *),
                           void * usrarg );

#ifndef STACK_BIAS
#define STACK_BIAS 0
#endif 

#define LOGSIZE 4096


typedef int demf_t(const char *, char *, size_t);

static demf_t *demf;

static int initialized = 0;

#if defined(sparc) || defined(__sparc)
#define FRAME_PTR_REGISTER REG_SP
#endif

#if defined(i386) || defined(__i386)
#define FRAME_PTR_REGISTER EBP
#endif

struct bucket {
    void * pc;
    int index;
    struct bucket * next;
};

struct my_user_args {
    NS_WalkStackCallback callback;
    PRUint32 skipFrames;
    void *closure;
};


static void myinit();

#pragma init (myinit)

static void
myinit()
{

    if (! initialized) {
#ifndef __GNUC__
        void *handle;
        const char *libdem = "libdemangle.so.1";

        
        if ((handle = dlopen(libdem, RTLD_LAZY)) != NULL) {
            demf = (demf_t *)dlsym(handle,
                           "cplus_demangle"); 
                



        }
#endif 
    }    
    initialized = 1;
}


static int
load_address(void * pc, void * arg )
{
    static struct bucket table[2048];
    static mutex_t lock;
    struct bucket * ptr;
    struct my_user_args * args = (struct my_user_args *) arg;

    unsigned int val = NS_PTR_TO_INT32(pc);

    ptr = table + ((val >> 2)&2047);

    mutex_lock(&lock);
    while (ptr->next) {
        if (ptr->next->pc == pc)
            break;
        ptr = ptr->next;
    }

    if (ptr->next) {
        mutex_unlock(&lock);
    } else {
        (args->callback)(pc, args->closure);

        ptr->next = newbucket(pc);
        mutex_unlock(&lock);
    }
    return 0;
}


static struct bucket *
newbucket(void * pc)
{
    struct bucket * ptr = (struct bucket *) malloc(sizeof (*ptr));
    static int index; 
                     
    ptr->index = index++;
    ptr->next = NULL;
    ptr->pc = pc;    
    return (ptr);    
}


static struct frame *
csgetframeptr()
{
    ucontext_t u;
    struct frame *fp;

    (void) getcontext(&u);

    fp = (struct frame *)
        ((char *)u.uc_mcontext.gregs[FRAME_PTR_REGISTER] +
        STACK_BIAS);

    

    return ((struct frame *)((ulong_t)fp->fr_savfp + STACK_BIAS));
}


static void
cswalkstack(struct frame *fp, int (*operate_func)(void *, void *),
    void *usrarg)
{

    while (fp != 0 && fp->fr_savpc != 0) {

        if (operate_func((void *)fp->fr_savpc, usrarg) != 0)
            break;
        




        fp = (struct frame *)((ulong_t)fp->fr_savfp +
            (fp->fr_savfp?(ulong_t)STACK_BIAS:0));
    }
}


static void
cs_operate(int (*operate_func)(void *, void *), void * usrarg)
{
    cswalkstack(csgetframeptr(), operate_func, usrarg);
}

EXPORT_XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, PRUint32 aSkipFrames,
             void *aClosure)
{
    struct my_user_args args;

    if (!initialized)
        myinit();

    args.callback = aCallback;
    args.skipFrames = aSkipFrames; 
    args.closure = aClosure;
    cs_operate(load_address, &args);
    return NS_OK;
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

    char dembuff[4096];
    Dl_info info;

    if (dladdr(aPC, & info)) {
        if (info.dli_fname) {
            PL_strncpyz(aDetails->library, info.dli_fname,
                        sizeof(aDetails->library));
            aDetails->loffset = (char*)aPC - (char*)info.dli_fbase;
        }
        if (info.dli_sname) {
            aDetails->foffset = (char*)aPC - (char*)info.dli_saddr;
#ifdef __GNUC__
            DemangleSymbol(info.dli_sname, dembuff, sizeof(dembuff));
#else
            if (!demf || demf(info.dli_sname, dembuff, sizeof (dembuff)))
                dembuff[0] = 0;
#endif 
            PL_strncpyz(aDetails->function,
                        (dembuff[0] != '\0') ? dembuff : info.dli_sname,
                        sizeof(aDetails->function));
        }
    }

    return NS_OK;
}

EXPORT_XPCOM_API(nsresult)
NS_FormatCodeAddressDetails(void *aPC, const nsCodeAddressDetails *aDetails,
                            char *aBuffer, PRUint32 aBufferSize)
{
    snprintf(aBuffer, aBufferSize, "%p %s:%s+0x%lx\n",
             aPC,
             aDetails->library[0] ? aDetails->library : "??",
             aDetails->function[0] ? aDetails->function : "??",
             aDetails->foffset);
    return NS_OK;
}

#else 

#if (defined(linux) && defined(__GNUC__) && (defined(__i386) || defined(PPC))) || (defined(XP_MACOSX) && (defined(__i386) || defined(__ppc__))) 

#if __GLIBC__ > 2 || __GLIBC_MINOR > 1
#define HAVE___LIBC_STACK_END 1
#else
#define HAVE___LIBC_STACK_END 0
#endif

#if HAVE___LIBC_STACK_END
extern void *__libc_stack_end; 
#endif

#ifdef XP_MACOSX
struct AddressRange {
  void* mStart;
  void* mEnd;
};

static AddressRange gCriticalRange;

static void FindFunctionAddresses(const char* aName, AddressRange* aRange)
{
  aRange->mStart = dlsym(RTLD_DEFAULT, aName);
  if (!aRange->mStart)
    return;
  aRange->mEnd = aRange->mStart;
  while (PR_TRUE) {
    Dl_info info;
    if (!dladdr(aRange->mEnd, &info))
      break;
    if (strcmp(info.dli_sname, aName))
      break;
    aRange->mEnd = (char*)aRange->mEnd + 1;
  }
}

static void InitCriticalRanges()
{
  if (gCriticalRange.mStart)
    return;
  
  
  
  
  
  FindFunctionAddresses("pthread_cond_wait$UNIX2003", &gCriticalRange);
}

static PRBool InCriticalRange(void* aPC)
{
  return gCriticalRange.mStart &&
    gCriticalRange.mStart <= aPC && aPC < gCriticalRange.mEnd;
}
#else
static void InitCriticalRanges() {}
static PRBool InCriticalRange(void* aPC) { return PR_FALSE; }
#endif

EXPORT_XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, PRUint32 aSkipFrames,
             void *aClosure)
{
  
  InitCriticalRanges();

  
  void **bp;
#if defined(__i386) 
  __asm__( "movl %%ebp, %0" : "=g"(bp));
#else
  
  
  
  bp = (void**) __builtin_frame_address(0);
#endif

  int skip = aSkipFrames;
  while (1) {
    void **next = (void**)*bp;
    
    
    
    
    if (next <= bp ||
#if HAVE___LIBC_STACK_END
        next > __libc_stack_end ||
#endif
        (long(next) & 3)) {
      break;
    }
#if (defined(__ppc__) && defined(XP_MACOSX)) || defined(__powerpc64__)
    
    void *pc = *(bp+2);
#else 
    void *pc = *(bp+1);
#endif
    if (InCriticalRange(pc)) {
      printf("Aborting stack trace, PC in critical range\n");
      return NS_ERROR_UNEXPECTED;
    }
    if (--skip < 0) {
      (*aCallback)(pc, aClosure);
    }
    bp = next;
  }
  return NS_OK;
}

#elif defined(HAVE__UNWIND_BACKTRACE)


#include <unwind.h>

struct unwind_info {
    NS_WalkStackCallback callback;
    int skip;
    void *closure;
};

static _Unwind_Reason_Code
unwind_callback (struct _Unwind_Context *context, void *closure)
{
    unwind_info *info = static_cast<unwind_info *>(closure);
    if (--info->skip < 0) {
        void *pc = reinterpret_cast<void *>(_Unwind_GetIP(context));
        (*info->callback)(pc, info->closure);
    }
    return _URC_NO_REASON;
}

EXPORT_XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, PRUint32 aSkipFrames,
             void *aClosure)
{
    unwind_info info;
    info.callback = aCallback;
    info.skip = aSkipFrames + 1;
    info.closure = aClosure;

    _Unwind_Backtrace(unwind_callback, &info);

    return NS_OK;
}

#endif

EXPORT_XPCOM_API(nsresult)
NS_DescribeCodeAddress(void *aPC, nsCodeAddressDetails *aDetails)
{
  aDetails->library[0] = '\0';
  aDetails->loffset = 0;
  aDetails->filename[0] = '\0';
  aDetails->lineno = 0;
  aDetails->function[0] = '\0';
  aDetails->foffset = 0;

  Dl_info info;
  int ok = dladdr(aPC, &info);
  if (!ok) {
    return NS_OK;
  }

  PL_strncpyz(aDetails->library, info.dli_fname, sizeof(aDetails->library));
  aDetails->loffset = (char*)aPC - (char*)info.dli_fbase;

  const char * symbol = info.dli_sname;
  int len;
  if (!symbol || !(len = strlen(symbol))) {
    return NS_OK;
  }

  char demangled[4096] = "\0";

  DemangleSymbol(symbol, demangled, sizeof(demangled));

  if (strlen(demangled)) {
    symbol = demangled;
    len = strlen(symbol);
  }

  PL_strncpyz(aDetails->function, symbol, sizeof(aDetails->function));
  aDetails->foffset = (char*)aPC - (char*)info.dli_saddr;
  return NS_OK;
}

EXPORT_XPCOM_API(nsresult)
NS_FormatCodeAddressDetails(void *aPC, const nsCodeAddressDetails *aDetails,
                            char *aBuffer, PRUint32 aBufferSize)
{
  if (!aDetails->library[0]) {
    snprintf(aBuffer, aBufferSize, "UNKNOWN %p\n", aPC);
  } else if (!aDetails->function[0]) {
    snprintf(aBuffer, aBufferSize, "UNKNOWN [%s +0x%08lX]\n",
                                   aDetails->library, aDetails->loffset);
  } else {
    snprintf(aBuffer, aBufferSize, "%s+0x%08lX [%s +0x%08lX]\n",
                                   aDetails->function, aDetails->foffset,
                                   aDetails->library, aDetails->loffset);
  }
  return NS_OK;
}

#endif

#else 

EXPORT_XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, PRUint32 aSkipFrames,
             void *aClosure)
{
    return NS_ERROR_NOT_IMPLEMENTED;
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
    return NS_ERROR_NOT_IMPLEMENTED;
}

EXPORT_XPCOM_API(nsresult)
NS_FormatCodeAddressDetails(void *aPC, const nsCodeAddressDetails *aDetails,
                            char *aBuffer, PRUint32 aBufferSize)
{
    aBuffer[0] = '\0';
    return NS_ERROR_NOT_IMPLEMENTED;
}

#endif
