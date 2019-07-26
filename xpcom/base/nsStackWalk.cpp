







#include "mozilla/Util.h"
#include "mozilla/StackWalk.h"
#include "nsDebug.h"
#include "nsStackWalkPrivate.h"

#include "nsStackWalk.h"

using namespace mozilla;



struct CriticalAddress {
  void* mAddr;
  bool mInit;
};
static CriticalAddress gCriticalAddress;

#if defined(HAVE_DLOPEN) || defined(XP_MACOSX)
#include <dlfcn.h>
#endif

#define NSSTACKWALK_SUPPORTS_MACOSX \
    (defined(XP_MACOSX) && \
     (defined(__i386) || defined(__ppc__) || defined(HAVE__UNWIND_BACKTRACE)))

#define NSSTACKWALK_SUPPORTS_LINUX \
    (defined(linux) && \
     ((defined(__GNUC__) && (defined(__i386) || defined(PPC))) || \
      defined(HAVE__UNWIND_BACKTRACE)))

#define NSSTACKWALK_SUPPORTS_SOLARIS \
    (defined(__sun) && \
     (defined(__sparc) || defined(sparc) || defined(__i386) || defined(i386)))

#if NSSTACKWALK_SUPPORTS_MACOSX
#include <pthread.h>
#include <errno.h>
#include <CoreServices/CoreServices.h>

typedef void
malloc_logger_t(uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                uintptr_t result, uint32_t num_hot_frames_to_skip);
extern malloc_logger_t *malloc_logger;

static void
stack_callback(void *pc, void *sp, void *closure)
{
  const char *name = reinterpret_cast<char *>(closure);
  Dl_info info;

  
  
  
  
  if (gCriticalAddress.mAddr || dladdr(pc, &info) == 0  ||
      info.dli_sname == NULL || strcmp(info.dli_sname, name) != 0)
    return;
  gCriticalAddress.mAddr = pc;
}

#ifdef DEBUG
#define MAC_OS_X_VERSION_10_7_HEX 0x00001070

static int32_t OSXVersion()
{
  static int32_t gOSXVersion = 0x0;
  if (gOSXVersion == 0x0) {
    OSErr err = ::Gestalt(gestaltSystemVersion, (SInt32*)&gOSXVersion);
    MOZ_ASSERT(err == noErr);
  }
  return gOSXVersion;
}

static bool OnLionOrLater()
{
  return (OSXVersion() >= MAC_OS_X_VERSION_10_7_HEX);
}
#endif

static void
my_malloc_logger(uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                 uintptr_t result, uint32_t num_hot_frames_to_skip)
{
  static bool once = false;
  if (once)
    return;
  once = true;

  
  
  const char *name = "new_sem_from_pool";
  NS_StackWalk(stack_callback,  0,  0,
               const_cast<char*>(name), 0, nullptr);
}








void
StackWalkInitCriticalAddress()
{
  if(gCriticalAddress.mInit)
    return;
  gCriticalAddress.mInit = true;
  
  
  
  
  

  
  malloc_logger_t *old_malloc_logger = malloc_logger;
  malloc_logger = my_malloc_logger;

  pthread_cond_t cond;
  int r = pthread_cond_init(&cond, 0);
  MOZ_ASSERT(r == 0);
  pthread_mutex_t mutex;
  r = pthread_mutex_init(&mutex,0);
  MOZ_ASSERT(r == 0);
  r = pthread_mutex_lock(&mutex);
  MOZ_ASSERT(r == 0);
  struct timespec abstime = {0, 1};
  r = pthread_cond_timedwait_relative_np(&cond, &mutex, &abstime);

  
  malloc_logger = old_malloc_logger;

  
  
  
  MOZ_ASSERT(OnLionOrLater() || gCriticalAddress.mAddr != NULL);
  MOZ_ASSERT(r == ETIMEDOUT);
  r = pthread_mutex_unlock(&mutex);
  MOZ_ASSERT(r == 0);
  r = pthread_mutex_destroy(&mutex);
  MOZ_ASSERT(r == 0);
  r = pthread_cond_destroy(&cond);
  MOZ_ASSERT(r == 0);
}

static bool IsCriticalAddress(void* aPC)
{
  return gCriticalAddress.mAddr == aPC;
}
#else
static bool IsCriticalAddress(void* aPC)
{
  return false;
}



void
StackWalkInitCriticalAddress()
{
  gCriticalAddress.mInit = true;
}
#endif

#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_IA64)) 

#include "nscore.h"
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <malloc.h>
#include "plstr.h"

#include "nspr.h"
#include <imagehlp.h>



#if API_VERSION_NUMBER < 9
#error Too old imagehlp.h
#endif







extern "C" {

extern HANDLE hStackWalkMutex; 

bool EnsureSymInitialized();

bool EnsureImageHlpInitialized();

struct WalkStackData {
  uint32_t skipFrames;
  HANDLE thread;
  bool walkCallingThread;
  HANDLE process;
  HANDLE eventStart;
  HANDLE eventEnd;
  void **pcs;
  uint32_t pc_size;
  uint32_t pc_count;
  uint32_t pc_max;
  void **sps;
  uint32_t sp_size;
  uint32_t sp_count;
  void *platformData;
};

void PrintError(char *prefix, WalkStackData* data);
unsigned int WINAPI WalkStackThread(void* data);
void WalkStackMain64(struct WalkStackData* data);


DWORD gStackWalkThread;
CRITICAL_SECTION gDbgHelpCS;

}



void PrintError(const char *prefix)
{
    LPVOID lpMsgBuf;
    DWORD lastErr = GetLastError();
    FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      lastErr,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
      (LPSTR) &lpMsgBuf,
      0,
      NULL
    );
    fprintf(stderr, "### ERROR: %s: %s",
                    prefix, lpMsgBuf ? lpMsgBuf : "(null)\n");
    fflush(stderr);
    LocalFree(lpMsgBuf);
}

bool
EnsureImageHlpInitialized()
{
    static bool gInitialized = false;

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
        return false;
    }
    ::CloseHandle(hStackWalkThread);

    
    ::WaitForSingleObject(readyEvent, INFINITE);
    ::CloseHandle(readyEvent);

    ::InitializeCriticalSection(&gDbgHelpCS);

    return gInitialized = true;
}

void
WalkStackMain64(struct WalkStackData* data)
{
    
    
    
    CONTEXT context;
    HANDLE myProcess = data->process;
    HANDLE myThread = data->thread;
    DWORD64 addr;
    DWORD64 spaddr;
    STACKFRAME64 frame64;
    
    int skip = (data->walkCallingThread ? 3 : 0) + data->skipFrames;
    BOOL ok;

    
    if (!data->platformData) {
        memset(&context, 0, sizeof(CONTEXT));
        context.ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(myThread, &context)) {
            if (data->walkCallingThread) {
                PrintError("GetThreadContext");
            }
            return;
        }
    } else {
        context = *static_cast<CONTEXT*>(data->platformData);
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
        ok = StackWalk64(
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
          SymFunctionTableAccess64, 
          SymGetModuleBase64,       
          0
        );
        LeaveCriticalSection(&gDbgHelpCS);

        if (ok) {
            addr = frame64.AddrPC.Offset;
            spaddr = frame64.AddrStack.Offset;
         } else {
            addr = 0;
            spaddr = 0;
            if (data->walkCallingThread) {
                PrintError("WalkStack64");
            }
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

        if (data->sp_count < data->sp_size)
            data->sps[data->sp_count] = (void*)spaddr;
        ++data->sp_count;

        if (data->pc_max != 0 && data->pc_count == data->pc_max)
            break;

        if (frame64.AddrReturn.Offset == 0)
            break;
    }
    return;
}


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
                WalkStackMain64(data);

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
NS_StackWalk(NS_WalkStackCallback aCallback, uint32_t aSkipFrames,
             uint32_t aMaxFrames, void *aClosure, uintptr_t aThread,
             void *aPlatformData)
{
    StackWalkInitCriticalAddress();
    static HANDLE myProcess = NULL;
    HANDLE myThread;
    DWORD walkerReturn;
    struct WalkStackData data;

    if (!EnsureImageHlpInitialized())
        return NS_ERROR_FAILURE;

    HANDLE targetThread = ::GetCurrentThread();
    data.walkCallingThread = true;
    if (aThread) {
        HANDLE threadToWalk = reinterpret_cast<HANDLE> (aThread);
        
        data.walkCallingThread = (threadToWalk == targetThread);
        targetThread = threadToWalk;
    }

    
    
    const bool shouldBeThreadSafe = !!aThread;

    
    if (!myProcess) {
        if (!::DuplicateHandle(::GetCurrentProcess(),
                               ::GetCurrentProcess(),
                               ::GetCurrentProcess(),
                               &myProcess,
                               PROCESS_ALL_ACCESS, FALSE, 0)) {
            if (!shouldBeThreadSafe) {
                PrintError("DuplicateHandle (process)");
            }
            return NS_ERROR_FAILURE;
        }
    }
    if (!::DuplicateHandle(::GetCurrentProcess(),
                           targetThread,
                           ::GetCurrentProcess(),
                           &myThread,
                           THREAD_ALL_ACCESS, FALSE, 0)) {
        if (!shouldBeThreadSafe) {
            PrintError("DuplicateHandle (thread)");
        }
        return NS_ERROR_FAILURE;
    }

    data.skipFrames = aSkipFrames;
    data.thread = myThread;
    data.process = myProcess;
    void *local_pcs[1024];
    data.pcs = local_pcs;
    data.pc_count = 0;
    data.pc_size = ArrayLength(local_pcs);
    data.pc_max = aMaxFrames;
    void *local_sps[1024];
    data.sps = local_sps;
    data.sp_count = 0;
    data.sp_size = ArrayLength(local_sps);
    data.platformData = aPlatformData;

    if (aThread) {
        
        
        WalkStackMain64(&data);

        if (data.pc_count > data.pc_size) {
            data.pcs = (void**) _alloca(data.pc_count * sizeof(void*));
            data.pc_size = data.pc_count;
            data.pc_count = 0;
            data.sps = (void**) _alloca(data.sp_count * sizeof(void*));
            data.sp_size = data.sp_count;
            data.sp_count = 0;
            WalkStackMain64(&data);
        }
    } else {
        data.eventStart = ::CreateEvent(NULL, FALSE ,
                              FALSE , NULL);
        data.eventEnd = ::CreateEvent(NULL, FALSE ,
                            FALSE , NULL);

        ::PostThreadMessage(gStackWalkThread, WM_USER, 0, (LPARAM)&data);

        walkerReturn = ::SignalObjectAndWait(data.eventStart,
                           data.eventEnd, INFINITE, FALSE);
        if (walkerReturn != WAIT_OBJECT_0 && !shouldBeThreadSafe)
            PrintError("SignalObjectAndWait (1)");
        if (data.pc_count > data.pc_size) {
            data.pcs = (void**) _alloca(data.pc_count * sizeof(void*));
            data.pc_size = data.pc_count;
            data.pc_count = 0;
            data.sps = (void**) _alloca(data.sp_count * sizeof(void*));
            data.sp_size = data.sp_count;
            data.sp_count = 0;
            ::PostThreadMessage(gStackWalkThread, WM_USER, 0, (LPARAM)&data);
            walkerReturn = ::SignalObjectAndWait(data.eventStart,
                               data.eventEnd, INFINITE, FALSE);
            if (walkerReturn != WAIT_OBJECT_0 && !shouldBeThreadSafe)
                PrintError("SignalObjectAndWait (2)");
        }

        ::CloseHandle(data.eventStart);
        ::CloseHandle(data.eventEnd);
    }

    ::CloseHandle(myThread);

    for (uint32_t i = 0; i < data.pc_count; ++i)
        (*aCallback)(data.pcs[i], data.sps[i], aClosure);

    return data.pc_count == 0 ? NS_ERROR_FAILURE : NS_OK;
}


static BOOL CALLBACK callbackEspecial64(
  PCSTR aModuleName,
  DWORD64 aModuleBase,
  ULONG aModuleSize,
  PVOID aUserContext)
{
    BOOL retval = TRUE;
    DWORD64 addr = *(DWORD64*)aUserContext;

    




    const BOOL addressIncreases = TRUE;

    


    if (addressIncreases
       ? (addr >= aModuleBase && addr <= (aModuleBase + aModuleSize))
       : (addr <= aModuleBase && addr >= (aModuleBase - aModuleSize))
        ) {
        retval = !!SymLoadModule64(GetCurrentProcess(), NULL, (PSTR)aModuleName, NULL, aModuleBase, aModuleSize);
        if (!retval)
            PrintError("SymLoadModule64");
    }

    return retval;
}




















#ifdef SSRVOPT_SETCONTEXT
#define NS_IMAGEHLP_MODULE64_SIZE (((offsetof(IMAGEHLP_MODULE64, LoadedPdbName) + sizeof(DWORD64) - 1) / sizeof(DWORD64)) * sizeof(DWORD64))
#else
#define NS_IMAGEHLP_MODULE64_SIZE sizeof(IMAGEHLP_MODULE64)
#endif

BOOL SymGetModuleInfoEspecial64(HANDLE aProcess, DWORD64 aAddr, PIMAGEHLP_MODULE64 aModuleInfo, PIMAGEHLP_LINE64 aLineInfo)
{
    BOOL retval = FALSE;

    


    aModuleInfo->SizeOfStruct = NS_IMAGEHLP_MODULE64_SIZE;
    if (nullptr != aLineInfo) {
        aLineInfo->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    }

    



    retval = SymGetModuleInfo64(aProcess, aAddr, aModuleInfo);

    if (FALSE == retval) {
        BOOL enumRes = FALSE;

        



        
        
        
        
        
        enumRes = EnumerateLoadedModules64(aProcess, (PENUMLOADED_MODULES_CALLBACK64)callbackEspecial64, (PVOID)&aAddr);
        if (FALSE != enumRes)
        {
            



            retval = SymGetModuleInfo64(aProcess, aAddr, aModuleInfo);
        }
    }

    



    if (FALSE != retval && nullptr != aLineInfo) {
        DWORD displacement = 0;
        BOOL lineRes = FALSE;
        lineRes = SymGetLineFromAddr64(aProcess, aAddr, &displacement, aLineInfo);
        if (!lineRes) {
            
            memset(aLineInfo, 0, sizeof(*aLineInfo));
        }
    }

    return retval;
}

bool
EnsureSymInitialized()
{
    static bool gInitialized = false;
    bool retStat;

    if (gInitialized)
        return gInitialized;

    if (!EnsureImageHlpInitialized())
        return false;

    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    retStat = SymInitialize(GetCurrentProcess(), NULL, TRUE);
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
    ok = SymFromAddr(myProcess, addr, &displacement, pSymbol);

    if (ok) {
        PL_strncpyz(aDetails->function, pSymbol->Name,
                    sizeof(aDetails->function));
        aDetails->foffset = static_cast<ptrdiff_t>(displacement);
    }

    LeaveCriticalSection(&gDbgHelpCS); 
    return NS_OK;
}

EXPORT_XPCOM_API(nsresult)
NS_FormatCodeAddressDetails(void *aPC, const nsCodeAddressDetails *aDetails,
                            char *aBuffer, uint32_t aBufferSize)
{
    if (aDetails->function[0]) {
        _snprintf(aBuffer, aBufferSize, "%s+0x%08lX [%s +0x%016lX]",
                  aDetails->function, aDetails->foffset,
                  aDetails->library, aDetails->loffset);
    } else if (aDetails->library[0]) {
        _snprintf(aBuffer, aBufferSize, "UNKNOWN [%s +0x%016lX]",
                  aDetails->library, aDetails->loffset);
    } else {
        _snprintf(aBuffer, aBufferSize, "UNKNOWN 0x%016lX", aPC);
    }

    aBuffer[aBufferSize - 1] = '\0';

    uint32_t len = strlen(aBuffer);
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



#elif HAVE_DLADDR && (HAVE__UNWIND_BACKTRACE || NSSTACKWALK_SUPPORTS_LINUX || NSSTACKWALK_SUPPORTS_SOLARIS || NSSTACKWALK_SUPPORTS_MACOSX)

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nscore.h"
#include <stdio.h>
#include "plstr.h"





#if (__GLIBC_MINOR__ >= 1) && !defined(__USE_GNU)
#define __USE_GNU
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


#if NSSTACKWALK_SUPPORTS_SOLARIS





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
                              int (*operate_func)(void *, void *, void *),
                              void * usrarg );
static void   cs_operate ( void (*operate_func)(void *, void *, void *),
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
    uint32_t skipFrames;
    uint32_t maxFrames;
    uint32_t numFrames;
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
load_address(void * pc, void * arg)
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

    int stop = 0;
    if (ptr->next) {
        mutex_unlock(&lock);
    } else {
        (args->callback)(pc, args->closure);
        args->numFrames++;
        if (args->maxFrames != 0 && args->numFrames == args->maxFrames)
          stop = 1;   

        ptr->next = newbucket(pc);
        mutex_unlock(&lock);
    }
    return stop;
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
cswalkstack(struct frame *fp, int (*operate_func)(void *, void *, void *),
    void *usrarg)
{

    while (fp != 0 && fp->fr_savpc != 0) {

        if (operate_func((void *)fp->fr_savpc, NULL, usrarg) != 0)
            break;
        




        fp = (struct frame *)((ulong_t)fp->fr_savfp +
            (fp->fr_savfp?(ulong_t)STACK_BIAS:0));
    }
}


static void
cs_operate(int (*operate_func)(void *, void *, void *), void * usrarg)
{
    cswalkstack(csgetframeptr(), operate_func, usrarg);
}

EXPORT_XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, uint32_t aSkipFrames,
             uint32_t aMaxFrames, void *aClosure, uintptr_t aThread,
             void *aPlatformData)
{
    MOZ_ASSERT(!aThread);
    MOZ_ASSERT(!aPlatformData);
    struct my_user_args args;

    StackWalkInitCriticalAddress();

    if (!initialized)
        myinit();

    args.callback = aCallback;
    args.skipFrames = aSkipFrames; 
    args.maxFrames = aMaxFrames;
    args.numFrames = 0;
    args.closure = aClosure;
    cs_operate(load_address, &args);
    return args.numFrames == 0 ? NS_ERROR_FAILURE : NS_OK;
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
                            char *aBuffer, uint32_t aBufferSize)
{
    snprintf(aBuffer, aBufferSize, "%p %s:%s+0x%lx\n",
             aPC,
             aDetails->library[0] ? aDetails->library : "??",
             aDetails->function[0] ? aDetails->function : "??",
             aDetails->foffset);
    return NS_OK;
}

#else 

#if __GLIBC__ > 2 || __GLIBC_MINOR > 1
#define HAVE___LIBC_STACK_END 1
#else
#define HAVE___LIBC_STACK_END 0
#endif

#if HAVE___LIBC_STACK_END
extern void *__libc_stack_end; 
#endif
namespace mozilla {
nsresult
FramePointerStackWalk(NS_WalkStackCallback aCallback, uint32_t aSkipFrames,
                      uint32_t aMaxFrames, void *aClosure, void **bp,
                      void *aStackEnd)
{
  

  int32_t skip = aSkipFrames;
  uint32_t numFrames = 0;
  while (1) {
    void **next = (void**)*bp;
    
    
    
    
    
    
    if (next <= bp ||
        next > aStackEnd ||
        (long(next) & 3)) {
      break;
    }
#if (defined(__ppc__) && defined(XP_MACOSX)) || defined(__powerpc64__)
    
    void *pc = *(bp+2);
    bp += 3;
#else 
    void *pc = *(bp+1);
    bp += 2;
#endif
    if (IsCriticalAddress(pc)) {
      printf("Aborting stack trace, PC is critical\n");
      return NS_ERROR_UNEXPECTED;
    }
    if (--skip < 0) {
      
      
      
      
      (*aCallback)(pc, bp, aClosure);
      numFrames++;
      if (aMaxFrames != 0 && numFrames == aMaxFrames)
        break;
    }
    bp = next;
  }
  return numFrames == 0 ? NS_ERROR_FAILURE : NS_OK;
}

}

#define X86_OR_PPC (defined(__i386) || defined(PPC) || defined(__ppc__))
#if X86_OR_PPC && (NSSTACKWALK_SUPPORTS_MACOSX || NSSTACKWALK_SUPPORTS_LINUX) 

EXPORT_XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, uint32_t aSkipFrames,
             uint32_t aMaxFrames, void *aClosure, uintptr_t aThread,
             void *aPlatformData)
{
  MOZ_ASSERT(!aThread);
  MOZ_ASSERT(!aPlatformData);
  StackWalkInitCriticalAddress();

  
  void **bp;
#if defined(__i386) 
  __asm__( "movl %%ebp, %0" : "=g"(bp));
#else
  
  
  
  bp = (void**) __builtin_frame_address(0);
#endif

  void *stackEnd;
#if HAVE___LIBC_STACK_END
  stackEnd = __libc_stack_end;
#else
  stackEnd = reinterpret_cast<void*>(-1);
#endif
  return FramePointerStackWalk(aCallback, aSkipFrames, aMaxFrames,
                               aClosure, bp, stackEnd);

}

#elif defined(HAVE__UNWIND_BACKTRACE)


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unwind.h>

struct unwind_info {
    NS_WalkStackCallback callback;
    int skip;
    int maxFrames;
    int numFrames;
    bool isCriticalAbort;
    void *closure;
};

static _Unwind_Reason_Code
unwind_callback (struct _Unwind_Context *context, void *closure)
{
    unwind_info *info = static_cast<unwind_info *>(closure);
    void *pc = reinterpret_cast<void *>(_Unwind_GetIP(context));
    
    if (IsCriticalAddress(pc)) {
        printf("Aborting stack trace, PC is critical\n");
        info->isCriticalAbort = true;
        
        
        
        return _URC_FOREIGN_EXCEPTION_CAUGHT;
    }
    if (--info->skip < 0) {
        (*info->callback)(pc, NULL, info->closure);
        info->numFrames++;
        if (info->maxFrames != 0 && info->numFrames == info->maxFrames) {
            
            return _URC_FOREIGN_EXCEPTION_CAUGHT;
        }
    }
    return _URC_NO_REASON;
}

EXPORT_XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, uint32_t aSkipFrames,
             uint32_t aMaxFrames, void *aClosure, uintptr_t aThread,
             void *aPlatformData)
{
    MOZ_ASSERT(!aThread);
    MOZ_ASSERT(!aPlatformData);
    StackWalkInitCriticalAddress();
    unwind_info info;
    info.callback = aCallback;
    info.skip = aSkipFrames + 1;
    info.maxFrames = aMaxFrames;
    info.numFrames = 0;
    info.isCriticalAbort = false;
    info.closure = aClosure;

    (void)_Unwind_Backtrace(unwind_callback, &info);

    
    
    
    
    
    
    
    
    if (info.isCriticalAbort)
        return NS_ERROR_UNEXPECTED;
    return info.numFrames == 0 ? NS_ERROR_FAILURE : NS_OK;
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
                            char *aBuffer, uint32_t aBufferSize)
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
NS_StackWalk(NS_WalkStackCallback aCallback, uint32_t aSkipFrames,
             uint32_t aMaxFrames, void *aClosure, uintptr_t aThread,
             void *aPlatformData)
{
    MOZ_ASSERT(!aThread);
    MOZ_ASSERT(!aPlatformData);
    return NS_ERROR_NOT_IMPLEMENTED;
}

namespace mozilla {
nsresult
FramePointerStackWalk(NS_WalkStackCallback aCallback, uint32_t aSkipFrames,
                      void *aClosure, void **bp)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
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
                            char *aBuffer, uint32_t aBufferSize)
{
    aBuffer[0] = '\0';
    return NS_ERROR_NOT_IMPLEMENTED;
}

#endif
