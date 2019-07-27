













#include "mozilla/ArrayUtils.h"

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsMemory.h"
#include "nsProcess.h"
#include "prio.h"
#include "prenv.h"
#include "nsCRT.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"

#include <stdlib.h>

#if defined(PROCESSMODEL_WINAPI)
#include "prmem.h"
#include "nsString.h"
#include "nsLiteralString.h"
#include "nsReadableUtils.h"
#else
#ifdef XP_MACOSX
#include <crt_externs.h>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/errno.h>
#endif
#include <sys/types.h>
#include <signal.h>
#endif

using namespace mozilla;

#ifdef XP_MACOSX
cpu_type_t pref_cpu_types[2] = {
#if defined(__i386__)
  CPU_TYPE_X86,
#elif defined(__x86_64__)
  CPU_TYPE_X86_64,
#elif defined(__ppc__)
  CPU_TYPE_POWERPC,
#endif
  CPU_TYPE_ANY
};
#endif




NS_IMPL_ISUPPORTS(nsProcess, nsIProcess,
                  nsIObserver)


nsProcess::nsProcess()
  : mThread(nullptr)
  , mLock("nsProcess.mLock")
  , mShutdown(false)
  , mBlocking(false)
  , mPid(-1)
  , mObserver(nullptr)
  , mWeakObserver(nullptr)
  , mExitValue(-1)
#if !defined(XP_MACOSX)
  , mProcess(nullptr)
#endif
{
}


nsProcess::~nsProcess()
{
}

NS_IMETHODIMP
nsProcess::Init(nsIFile* aExecutable)
{
  if (mExecutable) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  if (NS_WARN_IF(!aExecutable)) {
    return NS_ERROR_INVALID_ARG;
  }
  bool isFile;

  
  nsresult rv = aExecutable->IsFile(&isFile);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (!isFile) {
    return NS_ERROR_FAILURE;
  }

  
  mExecutable = aExecutable;
  
#ifdef XP_WIN
  rv = mExecutable->GetTarget(mTargetPath);
  if (NS_FAILED(rv) || mTargetPath.IsEmpty())
#endif
    rv = mExecutable->GetPath(mTargetPath);

  return rv;
}


#if defined(XP_WIN)

static int
assembleCmdLine(char* const* aArgv, wchar_t** aWideCmdLine, UINT aCodePage)
{
  char* const* arg;
  char* p;
  char* q;
  char* cmdLine;
  int cmdLineSize;
  int numBackslashes;
  int i;
  int argNeedQuotes;

  


  cmdLineSize = 0;
  for (arg = aArgv; *arg; ++arg) {
    






    cmdLineSize += 2 * strlen(*arg)  
                   + 2               
                   + 1;              
  }
  p = cmdLine = (char*)PR_MALLOC(cmdLineSize * sizeof(char));
  if (!p) {
    return -1;
  }

  for (arg = aArgv; *arg; ++arg) {
    
    if (arg != aArgv) {
      *p++ = ' ';
    }
    q = *arg;
    numBackslashes = 0;
    argNeedQuotes = 0;

    
    if (strpbrk(*arg, " \f\n\r\t\v")) {
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
  int32_t numChars = MultiByteToWideChar(aCodePage, 0, cmdLine, -1, nullptr, 0);
  *aWideCmdLine = (wchar_t*)PR_MALLOC(numChars * sizeof(wchar_t));
  MultiByteToWideChar(aCodePage, 0, cmdLine, -1, *aWideCmdLine, numChars);
  PR_Free(cmdLine);
  return 0;
}
#endif

void
nsProcess::Monitor(void* aArg)
{
  nsRefPtr<nsProcess> process = dont_AddRef(static_cast<nsProcess*>(aArg));

  if (!process->mBlocking) {
    PR_SetCurrentThreadName("RunProcess");
  }

#if defined(PROCESSMODEL_WINAPI)
  DWORD dwRetVal;
  unsigned long exitCode = -1;

  dwRetVal = WaitForSingleObject(process->mProcess, INFINITE);
  if (dwRetVal != WAIT_FAILED) {
    if (GetExitCodeProcess(process->mProcess, &exitCode) == FALSE) {
      exitCode = -1;
    }
  }

  
  {
    MutexAutoLock lock(process->mLock);
    CloseHandle(process->mProcess);
    process->mProcess = nullptr;
    process->mExitValue = exitCode;
    if (process->mShutdown) {
      return;
    }
  }
#else
#ifdef XP_MACOSX
  int exitCode = -1;
  int status = 0;
  pid_t result;
  do {
    result = waitpid(process->mPid, &status, 0);
  } while (result == -1 && errno == EINTR);
  if (result == process->mPid) {
    if (WIFEXITED(status)) {
      exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      exitCode = 256; 
    }
  }
#else
  int32_t exitCode = -1;
  if (PR_WaitProcess(process->mProcess, &exitCode) != PR_SUCCESS) {
    exitCode = -1;
  }
#endif

  
  {
    MutexAutoLock lock(process->mLock);
#if !defined(XP_MACOSX)
    process->mProcess = nullptr;
#endif
    process->mExitValue = exitCode;
    if (process->mShutdown) {
      return;
    }
  }
#endif

  
  
  if (NS_IsMainThread()) {
    process->ProcessComplete();
  } else {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(process, &nsProcess::ProcessComplete);
    NS_DispatchToMainThread(event);
  }
}

void
nsProcess::ProcessComplete()
{
  if (mThread) {
    nsCOMPtr<nsIObserverService> os =
      mozilla::services::GetObserverService();
    if (os) {
      os->RemoveObserver(this, "xpcom-shutdown");
    }
    PR_JoinThread(mThread);
    mThread = nullptr;
  }

  const char* topic;
  if (mExitValue < 0) {
    topic = "process-failed";
  } else {
    topic = "process-finished";
  }

  mPid = -1;
  nsCOMPtr<nsIObserver> observer;
  if (mWeakObserver) {
    observer = do_QueryReferent(mWeakObserver);
  } else if (mObserver) {
    observer = mObserver;
  }
  mObserver = nullptr;
  mWeakObserver = nullptr;

  if (observer) {
    observer->Observe(NS_ISUPPORTS_CAST(nsIProcess*, this), topic, nullptr);
  }
}


NS_IMETHODIMP
nsProcess::Run(bool aBlocking, const char** aArgs, uint32_t aCount)
{
  return CopyArgsAndRunProcess(aBlocking, aArgs, aCount, nullptr, false);
}


NS_IMETHODIMP
nsProcess::RunAsync(const char** aArgs, uint32_t aCount,
                    nsIObserver* aObserver, bool aHoldWeak)
{
  return CopyArgsAndRunProcess(false, aArgs, aCount, aObserver, aHoldWeak);
}

nsresult
nsProcess::CopyArgsAndRunProcess(bool aBlocking, const char** aArgs,
                                 uint32_t aCount, nsIObserver* aObserver,
                                 bool aHoldWeak)
{
  
  char** my_argv = nullptr;
  my_argv = (char**)moz_xmalloc(sizeof(char*) * (aCount + 2));
  if (!my_argv) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  my_argv[0] = ToNewUTF8String(mTargetPath);

  for (uint32_t i = 0; i < aCount; ++i) {
    my_argv[i + 1] = const_cast<char*>(aArgs[i]);
  }

  my_argv[aCount + 1] = nullptr;

  nsresult rv = RunProcess(aBlocking, my_argv, aObserver, aHoldWeak, false);

  free(my_argv[0]);
  free(my_argv);
  return rv;
}


NS_IMETHODIMP
nsProcess::Runw(bool aBlocking, const char16_t** aArgs, uint32_t aCount)
{
  return CopyArgsAndRunProcessw(aBlocking, aArgs, aCount, nullptr, false);
}


NS_IMETHODIMP
nsProcess::RunwAsync(const char16_t** aArgs, uint32_t aCount,
                     nsIObserver* aObserver, bool aHoldWeak)
{
  return CopyArgsAndRunProcessw(false, aArgs, aCount, aObserver, aHoldWeak);
}

nsresult
nsProcess::CopyArgsAndRunProcessw(bool aBlocking, const char16_t** aArgs,
                                  uint32_t aCount, nsIObserver* aObserver,
                                  bool aHoldWeak)
{
  
  char** my_argv = nullptr;
  my_argv = (char**)moz_xmalloc(sizeof(char*) * (aCount + 2));
  if (!my_argv) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  my_argv[0] = ToNewUTF8String(mTargetPath);

  for (uint32_t i = 0; i < aCount; i++) {
    my_argv[i + 1] = ToNewUTF8String(nsDependentString(aArgs[i]));
  }

  my_argv[aCount + 1] = nullptr;

  nsresult rv = RunProcess(aBlocking, my_argv, aObserver, aHoldWeak, true);

  for (uint32_t i = 0; i <= aCount; ++i) {
    free(my_argv[i]);
  }
  free(my_argv);
  return rv;
}

nsresult
nsProcess::RunProcess(bool aBlocking, char** aMyArgv, nsIObserver* aObserver,
                      bool aHoldWeak, bool aArgsUTF8)
{
  if (NS_WARN_IF(!mExecutable)) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  if (NS_WARN_IF(mThread)) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  if (aObserver) {
    if (aHoldWeak) {
      mWeakObserver = do_GetWeakReference(aObserver);
      if (!mWeakObserver) {
        return NS_NOINTERFACE;
      }
    } else {
      mObserver = aObserver;
    }
  }

  mExitValue = -1;
  mPid = -1;

#if defined(PROCESSMODEL_WINAPI)
  BOOL retVal;
  wchar_t* cmdLine = nullptr;

  
  
  if (aMyArgv[1] && assembleCmdLine(aMyArgv + 1, &cmdLine,
                                    aArgsUTF8 ? CP_UTF8 : CP_ACP) == -1) {
    return NS_ERROR_FILE_EXECUTION_FAILED;
  }

  




  
  NS_ConvertUTF8toUTF16 wideFile(aMyArgv[0]);

  SHELLEXECUTEINFOW sinfo;
  memset(&sinfo, 0, sizeof(SHELLEXECUTEINFOW));
  sinfo.cbSize = sizeof(SHELLEXECUTEINFOW);
  sinfo.hwnd   = nullptr;
  sinfo.lpFile = wideFile.get();
  sinfo.nShow  = SW_SHOWNORMAL;
  sinfo.fMask  = SEE_MASK_FLAG_DDEWAIT |
                 SEE_MASK_NO_CONSOLE |
                 SEE_MASK_NOCLOSEPROCESS;

  if (cmdLine) {
    sinfo.lpParameters = cmdLine;
  }

  retVal = ShellExecuteExW(&sinfo);
  if (!retVal) {
    return NS_ERROR_FILE_EXECUTION_FAILED;
  }

  mProcess = sinfo.hProcess;

  if (cmdLine) {
    PR_Free(cmdLine);
  }

  mPid = GetProcessId(mProcess);
#elif defined(XP_MACOSX)
  
  posix_spawnattr_t spawnattr;
  if (posix_spawnattr_init(&spawnattr) != 0) {
    return NS_ERROR_FAILURE;
  }

  
  size_t attr_count = ArrayLength(pref_cpu_types);
  size_t attr_ocount = 0;
  if (posix_spawnattr_setbinpref_np(&spawnattr, attr_count, pref_cpu_types,
                                    &attr_ocount) != 0 ||
      attr_ocount != attr_count) {
    posix_spawnattr_destroy(&spawnattr);
    return NS_ERROR_FAILURE;
  }

  
  pid_t newPid = 0;
  int result = posix_spawnp(&newPid, aMyArgv[0], nullptr, &spawnattr, aMyArgv,
                            *_NSGetEnviron());
  mPid = static_cast<int32_t>(newPid);

  posix_spawnattr_destroy(&spawnattr);

  if (result != 0) {
    return NS_ERROR_FAILURE;
  }
#else
  mProcess = PR_CreateProcess(aMyArgv[0], aMyArgv, nullptr, nullptr);
  if (!mProcess) {
    return NS_ERROR_FAILURE;
  }
  struct MYProcess
  {
    uint32_t pid;
  };
  MYProcess* ptrProc = (MYProcess*)mProcess;
  mPid = ptrProc->pid;
#endif

  NS_ADDREF_THIS();
  mBlocking = aBlocking;
  if (aBlocking) {
    Monitor(this);
    if (mExitValue < 0) {
      return NS_ERROR_FILE_EXECUTION_FAILED;
    }
  } else {
    mThread = PR_CreateThread(PR_SYSTEM_THREAD, Monitor, this,
                              PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                              PR_JOINABLE_THREAD, 0);
    if (!mThread) {
      NS_RELEASE_THIS();
      return NS_ERROR_FAILURE;
    }

    
    nsCOMPtr<nsIObserverService> os =
      mozilla::services::GetObserverService();
    if (os) {
      os->AddObserver(this, "xpcom-shutdown", false);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsProcess::GetIsRunning(bool* aIsRunning)
{
  if (mThread) {
    *aIsRunning = true;
  } else {
    *aIsRunning = false;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsProcess::GetPid(uint32_t* aPid)
{
  if (!mThread) {
    return NS_ERROR_FAILURE;
  }
  if (mPid < 0) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  *aPid = mPid;
  return NS_OK;
}

NS_IMETHODIMP
nsProcess::Kill()
{
  if (!mThread) {
    return NS_ERROR_FAILURE;
  }

  {
    MutexAutoLock lock(mLock);
#if defined(PROCESSMODEL_WINAPI)
    if (TerminateProcess(mProcess, 0) == 0) {
      return NS_ERROR_FAILURE;
    }
#elif defined(XP_MACOSX)
    if (kill(mPid, SIGKILL) != 0) {
      return NS_ERROR_FAILURE;
    }
#else
    if (!mProcess || (PR_KillProcess(mProcess) != PR_SUCCESS)) {
      return NS_ERROR_FAILURE;
    }
#endif
  }

  
  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    os->RemoveObserver(this, "xpcom-shutdown");
  }
  PR_JoinThread(mThread);
  mThread = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsProcess::GetExitValue(int32_t* aExitValue)
{
  MutexAutoLock lock(mLock);

  *aExitValue = mExitValue;

  return NS_OK;
}

NS_IMETHODIMP
nsProcess::Observe(nsISupports* aSubject, const char* aTopic,
                   const char16_t* aData)
{
  
  if (mThread) {
    nsCOMPtr<nsIObserverService> os =
      mozilla::services::GetObserverService();
    if (os) {
      os->RemoveObserver(this, "xpcom-shutdown");
    }
    mThread = nullptr;
  }

  mObserver = nullptr;
  mWeakObserver = nullptr;

  MutexAutoLock lock(mLock);
  mShutdown = true;

  return NS_OK;
}
