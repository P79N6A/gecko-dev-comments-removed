







#include "mozilla/ArrayUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/IntegerPrintfMacros.h"
#include "mozilla/StackWalk.h"

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

using namespace mozilla;



struct CriticalAddress
{
  void* mAddr;
  bool mInit;
};
static CriticalAddress gCriticalAddress;



#if defined(HAVE__UNWIND_BACKTRACE) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#if defined(HAVE_DLOPEN) || defined(XP_DARWIN)
#include <dlfcn.h>
#endif

#define MOZ_STACKWALK_SUPPORTS_MACOSX \
  (defined(XP_DARWIN) && \
   (defined(__i386) || defined(__ppc__) || defined(HAVE__UNWIND_BACKTRACE)))

#define MOZ_STACKWALK_SUPPORTS_LINUX \
  (defined(linux) && \
   ((defined(__GNUC__) && (defined(__i386) || defined(PPC))) || \
    defined(HAVE__UNWIND_BACKTRACE)))

#if MOZ_STACKWALK_SUPPORTS_MACOSX
#include <pthread.h>
#include <sys/errno.h>
#ifdef MOZ_WIDGET_COCOA
#include <CoreServices/CoreServices.h>
#endif

typedef void
malloc_logger_t(uint32_t aType,
                uintptr_t aArg1, uintptr_t aArg2, uintptr_t aArg3,
                uintptr_t aResult, uint32_t aNumHotFramesToSkip);
extern malloc_logger_t* malloc_logger;

static void
stack_callback(uint32_t aFrameNumber, void* aPc, void* aSp, void* aClosure)
{
  const char* name = static_cast<char*>(aClosure);
  Dl_info info;

  
  
  
  
  if (gCriticalAddress.mAddr || dladdr(aPc, &info) == 0  ||
      !info.dli_sname || strcmp(info.dli_sname, name) != 0) {
    return;
  }
  gCriticalAddress.mAddr = aPc;
}

#if defined(MOZ_WIDGET_COCOA) && defined(DEBUG)
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
my_malloc_logger(uint32_t aType,
                 uintptr_t aArg1, uintptr_t aArg2, uintptr_t aArg3,
                 uintptr_t aResult, uint32_t aNumHotFramesToSkip)
{
  static bool once = false;
  if (once) {
    return;
  }
  once = true;

  
  
  const char* name = "new_sem_from_pool";
  MozStackWalk(stack_callback,  0,  0,
               const_cast<char*>(name), 0, nullptr);
}








MFBT_API void
StackWalkInitCriticalAddress()
{
  if (gCriticalAddress.mInit) {
    return;
  }
  gCriticalAddress.mInit = true;
  
  
  
  
  

  
  malloc_logger_t* old_malloc_logger = malloc_logger;
  malloc_logger = my_malloc_logger;

  pthread_cond_t cond;
  int r = pthread_cond_init(&cond, 0);
  MOZ_ASSERT(r == 0);
  pthread_mutex_t mutex;
  r = pthread_mutex_init(&mutex, 0);
  MOZ_ASSERT(r == 0);
  r = pthread_mutex_lock(&mutex);
  MOZ_ASSERT(r == 0);
  struct timespec abstime = { 0, 1 };
  r = pthread_cond_timedwait_relative_np(&cond, &mutex, &abstime);

  
  malloc_logger = old_malloc_logger;

  
  
  
#ifdef MOZ_WIDGET_COCOA
  MOZ_ASSERT(OnLionOrLater() || gCriticalAddress.mAddr != nullptr);
#endif
  MOZ_ASSERT(r == ETIMEDOUT);
  r = pthread_mutex_unlock(&mutex);
  MOZ_ASSERT(r == 0);
  r = pthread_mutex_destroy(&mutex);
  MOZ_ASSERT(r == 0);
  r = pthread_cond_destroy(&cond);
  MOZ_ASSERT(r == 0);
}

static bool
IsCriticalAddress(void* aPC)
{
  return gCriticalAddress.mAddr == aPC;
}
#else
static bool
IsCriticalAddress(void* aPC)
{
  return false;
}



MFBT_API void
StackWalkInitCriticalAddress()
{
  gCriticalAddress.mInit = true;
}
#endif

#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_IA64)) 

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <malloc.h>
#include "mozilla/ArrayUtils.h"

#include <imagehlp.h>



#if API_VERSION_NUMBER < 9
#error Too old imagehlp.h
#endif

struct WalkStackData
{
  
  
  bool walkCallingThread;
  uint32_t skipFrames;
  HANDLE thread;
  HANDLE process;
  HANDLE eventStart;
  HANDLE eventEnd;
  void** pcs;
  uint32_t pc_size;
  uint32_t pc_count;
  uint32_t pc_max;
  void** sps;
  uint32_t sp_size;
  uint32_t sp_count;
  void* platformData;
};

DWORD gStackWalkThread;
CRITICAL_SECTION gDbgHelpCS;


static void
PrintError(const char* aPrefix)
{
  LPVOID lpMsgBuf;
  DWORD lastErr = GetLastError();
  FormatMessageA(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    nullptr,
    lastErr,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
    (LPSTR)&lpMsgBuf,
    0,
    nullptr
  );
  fprintf(stderr, "### ERROR: %s: %s",
          aPrefix, lpMsgBuf ? lpMsgBuf : "(null)\n");
  fflush(stderr);
  LocalFree(lpMsgBuf);
}

static unsigned int WINAPI WalkStackThread(void* aData);

static bool
EnsureWalkThreadReady()
{
  static bool walkThreadReady = false;
  static HANDLE stackWalkThread = nullptr;
  static HANDLE readyEvent = nullptr;

  if (walkThreadReady) {
    return walkThreadReady;
  }

  if (!stackWalkThread) {
    readyEvent = ::CreateEvent(nullptr, FALSE ,
                               FALSE ,
                               nullptr);
    if (!readyEvent) {
      PrintError("CreateEvent");
      return false;
    }

    unsigned int threadID;
    stackWalkThread = (HANDLE)_beginthreadex(nullptr, 0, WalkStackThread,
                                             (void*)readyEvent, 0, &threadID);
    if (!stackWalkThread) {
      PrintError("CreateThread");
      ::CloseHandle(readyEvent);
      readyEvent = nullptr;
      return false;
    }
    gStackWalkThread = threadID;
    ::CloseHandle(stackWalkThread);
  }

  MOZ_ASSERT((stackWalkThread && readyEvent) ||
             (!stackWalkThread && !readyEvent));

  
  
  DWORD waitRet = ::WaitForSingleObject(readyEvent, 1000);
  if (waitRet == WAIT_TIMEOUT) {
    
    
    
    
    return false;
  }
  ::CloseHandle(readyEvent);
  stackWalkThread = nullptr;
  readyEvent = nullptr;


  ::InitializeCriticalSection(&gDbgHelpCS);

  return walkThreadReady = true;
}

static void
WalkStackMain64(struct WalkStackData* aData)
{
  
  CONTEXT context;
  if (!aData->platformData) {
    memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(aData->thread, &context)) {
      if (aData->walkCallingThread) {
        PrintError("GetThreadContext");
      }
      return;
    }
  } else {
    context = *static_cast<CONTEXT*>(aData->platformData);
  }

#if defined(_M_IX86) || defined(_M_IA64)
  
  STACKFRAME64 frame64;
  memset(&frame64, 0, sizeof(frame64));
#ifdef _M_IX86
  frame64.AddrPC.Offset    = context.Eip;
  frame64.AddrStack.Offset = context.Esp;
  frame64.AddrFrame.Offset = context.Ebp;
#elif defined _M_IA64
  frame64.AddrPC.Offset    = context.StIIP;
  frame64.AddrStack.Offset = context.SP;
  frame64.AddrFrame.Offset = context.RsBSP;
#endif
  frame64.AddrPC.Mode      = AddrModeFlat;
  frame64.AddrStack.Mode   = AddrModeFlat;
  frame64.AddrFrame.Mode   = AddrModeFlat;
  frame64.AddrReturn.Mode  = AddrModeFlat;
#endif

  
  int skip = (aData->walkCallingThread ? 3 : 0) + aData->skipFrames;

  
  while (true) {
    DWORD64 addr;
    DWORD64 spaddr;

#if defined(_M_IX86) || defined(_M_IA64)
    
    
    EnterCriticalSection(&gDbgHelpCS);
    BOOL ok = StackWalk64(
#if defined _M_IA64
      IMAGE_FILE_MACHINE_IA64,
#elif defined _M_IX86
      IMAGE_FILE_MACHINE_I386,
#endif
      aData->process,
      aData->thread,
      &frame64,
      &context,
      nullptr,
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
      if (aData->walkCallingThread) {
        PrintError("WalkStack64");
      }
    }

    if (!ok) {
      break;
    }

#elif defined(_M_AMD64)
    
    
    ULONG64 imageBase;
    PRUNTIME_FUNCTION runtimeFunction =
      RtlLookupFunctionEntry(context.Rip, &imageBase, NULL);

    if (!runtimeFunction) {
      
      
      break;
    }

    PVOID dummyHandlerData;
    ULONG64 dummyEstablisherFrame;
    RtlVirtualUnwind(UNW_FLAG_NHANDLER,
                     imageBase,
                     context.Rip,
                     runtimeFunction,
                     &context,
                     &dummyHandlerData,
                     &dummyEstablisherFrame,
                     nullptr);

    addr = context.Rip;
    spaddr = context.Rsp;

#else
#error "unknown platform"
#endif

    if (addr == 0) {
      break;
    }

    if (skip-- > 0) {
      continue;
    }

    if (aData->pc_count < aData->pc_size) {
      aData->pcs[aData->pc_count] = (void*)addr;
    }
    ++aData->pc_count;

    if (aData->sp_count < aData->sp_size) {
      aData->sps[aData->sp_count] = (void*)spaddr;
    }
    ++aData->sp_count;

    if (aData->pc_max != 0 && aData->pc_count == aData->pc_max) {
      break;
    }

#if defined(_M_IX86) || defined(_M_IA64)
    if (frame64.AddrReturn.Offset == 0) {
      break;
    }
#endif
  }
}

static unsigned int WINAPI
WalkStackThread(void* aData)
{
  BOOL msgRet;
  MSG msg;

  
  
  ::PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

  
  HANDLE readyEvent = (HANDLE)aData;
  ::SetEvent(readyEvent);

  while ((msgRet = ::GetMessage(&msg, (HWND)-1, 0, 0)) != 0) {
    if (msgRet == -1) {
      PrintError("GetMessage");
    } else {
      DWORD ret;

      struct WalkStackData* data = (WalkStackData*)msg.lParam;
      if (!data) {
        continue;
      }

      
      
      ret = ::WaitForSingleObject(data->eventStart, INFINITE);
      if (ret != WAIT_OBJECT_0) {
        PrintError("WaitForSingleObject");
      }

      
      
      ret = ::SuspendThread(data->thread);
      if (ret == -1) {
        PrintError("ThreadSuspend");
      } else {
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









MFBT_API bool
MozStackWalk(MozWalkStackCallback aCallback, uint32_t aSkipFrames,
             uint32_t aMaxFrames, void* aClosure, uintptr_t aThread,
             void* aPlatformData)
{
  StackWalkInitCriticalAddress();
  static HANDLE myProcess = nullptr;
  HANDLE myThread;
  DWORD walkerReturn;
  struct WalkStackData data;

  if (!EnsureWalkThreadReady()) {
    return false;
  }

  HANDLE currentThread = ::GetCurrentThread();
  HANDLE targetThread =
    aThread ? reinterpret_cast<HANDLE>(aThread) : currentThread;
  data.walkCallingThread = (targetThread == currentThread);

  
  if (!myProcess) {
    if (!::DuplicateHandle(::GetCurrentProcess(),
                           ::GetCurrentProcess(),
                           ::GetCurrentProcess(),
                           &myProcess,
                           PROCESS_ALL_ACCESS, FALSE, 0)) {
      if (data.walkCallingThread) {
        PrintError("DuplicateHandle (process)");
      }
      return false;
    }
  }
  if (!::DuplicateHandle(::GetCurrentProcess(),
                         targetThread,
                         ::GetCurrentProcess(),
                         &myThread,
                         THREAD_ALL_ACCESS, FALSE, 0)) {
    if (data.walkCallingThread) {
      PrintError("DuplicateHandle (thread)");
    }
    return false;
  }

  data.skipFrames = aSkipFrames;
  data.thread = myThread;
  data.process = myProcess;
  void* local_pcs[1024];
  data.pcs = local_pcs;
  data.pc_count = 0;
  data.pc_size = ArrayLength(local_pcs);
  data.pc_max = aMaxFrames;
  void* local_sps[1024];
  data.sps = local_sps;
  data.sp_count = 0;
  data.sp_size = ArrayLength(local_sps);
  data.platformData = aPlatformData;

  if (aThread) {
    
    
    WalkStackMain64(&data);

    if (data.pc_count > data.pc_size) {
      data.pcs = (void**)_alloca(data.pc_count * sizeof(void*));
      data.pc_size = data.pc_count;
      data.pc_count = 0;
      data.sps = (void**)_alloca(data.sp_count * sizeof(void*));
      data.sp_size = data.sp_count;
      data.sp_count = 0;
      WalkStackMain64(&data);
    }
  } else {
    data.eventStart = ::CreateEvent(nullptr, FALSE ,
                                    FALSE , nullptr);
    data.eventEnd = ::CreateEvent(nullptr, FALSE ,
                                  FALSE , nullptr);

    ::PostThreadMessage(gStackWalkThread, WM_USER, 0, (LPARAM)&data);

    walkerReturn = ::SignalObjectAndWait(data.eventStart,
                                         data.eventEnd, INFINITE, FALSE);
    if (walkerReturn != WAIT_OBJECT_0 && data.walkCallingThread) {
      PrintError("SignalObjectAndWait (1)");
    }
    if (data.pc_count > data.pc_size) {
      data.pcs = (void**)_alloca(data.pc_count * sizeof(void*));
      data.pc_size = data.pc_count;
      data.pc_count = 0;
      data.sps = (void**)_alloca(data.sp_count * sizeof(void*));
      data.sp_size = data.sp_count;
      data.sp_count = 0;
      ::PostThreadMessage(gStackWalkThread, WM_USER, 0, (LPARAM)&data);
      walkerReturn = ::SignalObjectAndWait(data.eventStart,
                                           data.eventEnd, INFINITE, FALSE);
      if (walkerReturn != WAIT_OBJECT_0 && data.walkCallingThread) {
        PrintError("SignalObjectAndWait (2)");
      }
    }

    ::CloseHandle(data.eventStart);
    ::CloseHandle(data.eventEnd);
  }

  ::CloseHandle(myThread);

  for (uint32_t i = 0; i < data.pc_count; ++i) {
    (*aCallback)(i + 1, data.pcs[i], data.sps[i], aClosure);
  }

  return data.pc_count != 0;
}


static BOOL CALLBACK
callbackEspecial64(
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
    retval = !!SymLoadModule64(GetCurrentProcess(), nullptr,
                               (PSTR)aModuleName, nullptr,
                               aModuleBase, aModuleSize);
    if (!retval) {
      PrintError("SymLoadModule64");
    }
  }

  return retval;
}




















#ifdef SSRVOPT_SETCONTEXT
#define NS_IMAGEHLP_MODULE64_SIZE (((offsetof(IMAGEHLP_MODULE64, LoadedPdbName) + sizeof(DWORD64) - 1) / sizeof(DWORD64)) * sizeof(DWORD64))
#else
#define NS_IMAGEHLP_MODULE64_SIZE sizeof(IMAGEHLP_MODULE64)
#endif

BOOL SymGetModuleInfoEspecial64(HANDLE aProcess, DWORD64 aAddr,
                                PIMAGEHLP_MODULE64 aModuleInfo,
                                PIMAGEHLP_LINE64 aLineInfo)
{
  BOOL retval = FALSE;

  


  aModuleInfo->SizeOfStruct = NS_IMAGEHLP_MODULE64_SIZE;
  if (aLineInfo) {
    aLineInfo->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
  }

  



  retval = SymGetModuleInfo64(aProcess, aAddr, aModuleInfo);
  if (retval == FALSE) {
    



    
    
    
    
    
    BOOL enumRes = EnumerateLoadedModules64(
      aProcess,
      (PENUMLOADED_MODULES_CALLBACK64)callbackEspecial64,
      (PVOID)&aAddr);
    if (enumRes != FALSE) {
      



      retval = SymGetModuleInfo64(aProcess, aAddr, aModuleInfo);
    }
  }

  



  if (retval != FALSE && aLineInfo) {
    DWORD displacement = 0;
    BOOL lineRes = FALSE;
    lineRes = SymGetLineFromAddr64(aProcess, aAddr, &displacement, aLineInfo);
    if (!lineRes) {
      
      memset(aLineInfo, 0, sizeof(*aLineInfo));
    }
  }

  return retval;
}

static bool
EnsureSymInitialized()
{
  static bool gInitialized = false;
  bool retStat;

  if (gInitialized) {
    return gInitialized;
  }

  if (!EnsureWalkThreadReady()) {
    return false;
  }

  SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
  retStat = SymInitialize(GetCurrentProcess(), nullptr, TRUE);
  if (!retStat) {
    PrintError("SymInitialize");
  }

  gInitialized = retStat;
  

  return retStat;
}


MFBT_API bool
MozDescribeCodeAddress(void* aPC, MozCodeAddressDetails* aDetails)
{
  aDetails->library[0] = '\0';
  aDetails->loffset = 0;
  aDetails->filename[0] = '\0';
  aDetails->lineno = 0;
  aDetails->function[0] = '\0';
  aDetails->foffset = 0;

  if (!EnsureSymInitialized()) {
    return false;
  }

  HANDLE myProcess = ::GetCurrentProcess();
  BOOL ok;

  
  EnterCriticalSection(&gDbgHelpCS);

  
  
  
  

  DWORD64 addr = (DWORD64)aPC;
  IMAGEHLP_MODULE64 modInfo;
  IMAGEHLP_LINE64 lineInfo;
  BOOL modInfoRes;
  modInfoRes = SymGetModuleInfoEspecial64(myProcess, addr, &modInfo, &lineInfo);

  if (modInfoRes) {
    strncpy(aDetails->library, modInfo.ModuleName,
                sizeof(aDetails->library));
    aDetails->library[mozilla::ArrayLength(aDetails->library) - 1] = '\0';
    aDetails->loffset = (char*)aPC - (char*)modInfo.BaseOfImage;

    if (lineInfo.FileName) {
      strncpy(aDetails->filename, lineInfo.FileName,
                  sizeof(aDetails->filename));
      aDetails->filename[mozilla::ArrayLength(aDetails->filename) - 1] = '\0';
      aDetails->lineno = lineInfo.LineNumber;
    }
  }

  ULONG64 buffer[(sizeof(SYMBOL_INFO) +
    MAX_SYM_NAME * sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
  PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
  pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  pSymbol->MaxNameLen = MAX_SYM_NAME;

  DWORD64 displacement;
  ok = SymFromAddr(myProcess, addr, &displacement, pSymbol);

  if (ok) {
    strncpy(aDetails->function, pSymbol->Name,
                sizeof(aDetails->function));
    aDetails->function[mozilla::ArrayLength(aDetails->function) - 1] = '\0';
    aDetails->foffset = static_cast<ptrdiff_t>(displacement);
  }

  LeaveCriticalSection(&gDbgHelpCS); 
  return true;
}


#elif HAVE_DLADDR && (HAVE__UNWIND_BACKTRACE || MOZ_STACKWALK_SUPPORTS_LINUX || MOZ_STACKWALK_SUPPORTS_MACOSX)

#include <stdlib.h>
#include <string.h>
#include <stdio.h>





#if (__GLIBC_MINOR__ >= 1) && !defined(__USE_GNU)
#define __USE_GNU
#endif



#if defined(MOZ_DEMANGLE_SYMBOLS)
#include <cxxabi.h>
#endif 

void DemangleSymbol(const char* aSymbol,
                    char* aBuffer,
                    int aBufLen)
{
  aBuffer[0] = '\0';

#if defined(MOZ_DEMANGLE_SYMBOLS)
  
  char* demangled = abi::__cxa_demangle(aSymbol, 0, 0, 0);

  if (demangled) {
    strncpy(aBuffer, demangled, aBufLen);
    aBuffer[aBufLen - 1] = '\0';
    free(demangled);
  }
#endif 
}

#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1)
#define HAVE___LIBC_STACK_END 1
#else
#define HAVE___LIBC_STACK_END 0
#endif

#if HAVE___LIBC_STACK_END
extern MOZ_EXPORT void* __libc_stack_end; 
#endif
namespace mozilla {
bool
FramePointerStackWalk(MozWalkStackCallback aCallback, uint32_t aSkipFrames,
                      uint32_t aMaxFrames, void* aClosure, void** bp,
                      void* aStackEnd)
{
  

  int32_t skip = aSkipFrames;
  uint32_t numFrames = 0;
  while (1) {
    void** next = (void**)*bp;
    
    
    
    
    
    
    if (next <= bp ||
        next > aStackEnd ||
        (long(next) & 3)) {
      break;
    }
#if (defined(__ppc__) && defined(XP_MACOSX)) || defined(__powerpc64__)
    
    void* pc = *(bp + 2);
    bp += 3;
#else 
    void* pc = *(bp + 1);
    bp += 2;
#endif
    if (IsCriticalAddress(pc)) {
      return false;
    }
    if (--skip < 0) {
      
      
      
      
      numFrames++;
      (*aCallback)(numFrames, pc, bp, aClosure);
      if (aMaxFrames != 0 && numFrames == aMaxFrames) {
        break;
      }
    }
    bp = next;
  }
  return numFrames != 0;
}

}

#define X86_OR_PPC (defined(__i386) || defined(PPC) || defined(__ppc__))
#if X86_OR_PPC && (MOZ_STACKWALK_SUPPORTS_MACOSX || MOZ_STACKWALK_SUPPORTS_LINUX) 

MFBT_API bool
MozStackWalk(MozWalkStackCallback aCallback, uint32_t aSkipFrames,
             uint32_t aMaxFrames, void* aClosure, uintptr_t aThread,
             void* aPlatformData)
{
  MOZ_ASSERT(!aThread);
  MOZ_ASSERT(!aPlatformData);
  StackWalkInitCriticalAddress();

  
  void** bp;
#if defined(__i386)
  __asm__("movl %%ebp, %0" : "=g"(bp));
#else
  
  
  
  bp = (void**)__builtin_frame_address(0);
#endif

  void* stackEnd;
#if HAVE___LIBC_STACK_END
  stackEnd = __libc_stack_end;
#else
  stackEnd = reinterpret_cast<void*>(-1);
#endif
  return FramePointerStackWalk(aCallback, aSkipFrames, aMaxFrames,
                               aClosure, bp, stackEnd);
}

#elif defined(HAVE__UNWIND_BACKTRACE)


#include <unwind.h>

struct unwind_info
{
  MozWalkStackCallback callback;
  int skip;
  int maxFrames;
  int numFrames;
  bool isCriticalAbort;
  void* closure;
};

static _Unwind_Reason_Code
unwind_callback(struct _Unwind_Context* context, void* closure)
{
  unwind_info* info = static_cast<unwind_info*>(closure);
  void* pc = reinterpret_cast<void*>(_Unwind_GetIP(context));
  
  if (IsCriticalAddress(pc)) {
    info->isCriticalAbort = true;
    
    
    
    return _URC_FOREIGN_EXCEPTION_CAUGHT;
  }
  if (--info->skip < 0) {
    info->numFrames++;
    (*info->callback)(info->numFrames, pc, nullptr, info->closure);
    if (info->maxFrames != 0 && info->numFrames == info->maxFrames) {
      
      return _URC_FOREIGN_EXCEPTION_CAUGHT;
    }
  }
  return _URC_NO_REASON;
}

MFBT_API bool
MozStackWalk(MozWalkStackCallback aCallback, uint32_t aSkipFrames,
             uint32_t aMaxFrames, void* aClosure, uintptr_t aThread,
             void* aPlatformData)
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

  
  
  
  
  
  
  
  
  if (info.isCriticalAbort) {
    return false;
  }
  return info.numFrames != 0;
}

#endif

bool MFBT_API
MozDescribeCodeAddress(void* aPC, MozCodeAddressDetails* aDetails)
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
    return true;
  }

  strncpy(aDetails->library, info.dli_fname, sizeof(aDetails->library));
  aDetails->library[mozilla::ArrayLength(aDetails->library) - 1] = '\0';
  aDetails->loffset = (char*)aPC - (char*)info.dli_fbase;

  const char* symbol = info.dli_sname;
  if (!symbol || symbol[0] == '\0') {
    return true;
  }

  DemangleSymbol(symbol, aDetails->function, sizeof(aDetails->function));

  if (aDetails->function[0] == '\0') {
    
    strncpy(aDetails->function, symbol, sizeof(aDetails->function));
    aDetails->function[mozilla::ArrayLength(aDetails->function) - 1] = '\0';
  }

  aDetails->foffset = (char*)aPC - (char*)info.dli_saddr;
  return true;
}

#else 

MFBT_API bool
MozStackWalk(MozWalkStackCallback aCallback, uint32_t aSkipFrames,
             uint32_t aMaxFrames, void* aClosure, uintptr_t aThread,
             void* aPlatformData)
{
  MOZ_ASSERT(!aThread);
  MOZ_ASSERT(!aPlatformData);
  return false;
}

namespace mozilla {
MFBT_API bool
FramePointerStackWalk(MozWalkStackCallback aCallback, uint32_t aSkipFrames,
                      void* aClosure, void** aBp)
{
  return false;
}
}

MFBT_API bool
MozDescribeCodeAddress(void* aPC, MozCodeAddressDetails* aDetails)
{
  aDetails->library[0] = '\0';
  aDetails->loffset = 0;
  aDetails->filename[0] = '\0';
  aDetails->lineno = 0;
  aDetails->function[0] = '\0';
  aDetails->foffset = 0;
  return false;
}

#endif

MFBT_API void
MozFormatCodeAddressDetails(char* aBuffer, uint32_t aBufferSize,
                            uint32_t aFrameNumber, void* aPC,
                            const MozCodeAddressDetails* aDetails)
{
  MozFormatCodeAddress(aBuffer, aBufferSize,
                       aFrameNumber, aPC, aDetails->function,
                       aDetails->library, aDetails->loffset,
                       aDetails->filename, aDetails->lineno);
}

MFBT_API void
MozFormatCodeAddress(char* aBuffer, uint32_t aBufferSize, uint32_t aFrameNumber,
                     const void* aPC, const char* aFunction,
                     const char* aLibrary, ptrdiff_t aLOffset,
                     const char* aFileName, uint32_t aLineNo)
{
  const char* function = aFunction && aFunction[0] ? aFunction : "???";
  if (aFileName && aFileName[0]) {
    
    snprintf(aBuffer, aBufferSize,
             "#%02u: %s (%s:%u)",
             aFrameNumber, function, aFileName, aLineNo);
  } else if (aLibrary && aLibrary[0]) {
    
    
    
    snprintf(aBuffer, aBufferSize,
             "#%02u: %s[%s +0x%" PRIxPTR "]",
             aFrameNumber, function, aLibrary, static_cast<uintptr_t>(aLOffset));
  } else {
    
    
    snprintf(aBuffer, aBufferSize,
             "#%02u: ??? (???:???" ")",
             aFrameNumber);
  }
}
