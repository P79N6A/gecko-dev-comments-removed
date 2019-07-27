










#include "SandboxInternal.h"
#include "SandboxLogging.h"

#include <unistd.h>
#include <sys/syscall.h>

#include "mozilla/unused.h"
#include "mozilla/dom/Exceptions.h"
#include "nsContentUtils.h"
#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif
#include "mozilla/StackWalk.h"
#include "nsString.h"
#include "nsThreadUtils.h"

namespace mozilla {




static void
SandboxLogJSStack(void)
{
  if (!NS_IsMainThread()) {
    
    
    
    return;
  }
  if (!nsContentUtils::XPConnect()) {
    
    
    return;
  }
  nsCOMPtr<nsIStackFrame> frame = dom::GetCurrentJSStack();
  for (int i = 0; frame != nullptr; ++i) {
    nsAutoString fileName, funName;
    int32_t lineNumber;

    
    fileName.SetIsVoid(true);
    unused << frame->GetFilename(fileName);
    lineNumber = 0;
    unused << frame->GetLineNumber(&lineNumber);
    funName.SetIsVoid(true);
    unused << frame->GetName(funName);

    if (!funName.IsVoid() || !fileName.IsVoid()) {
      SANDBOX_LOG_ERROR("JS frame %d: %s %s line %d", i,
                        funName.IsVoid() ?
                        "(anonymous)" : NS_ConvertUTF16toUTF8(funName).get(),
                        fileName.IsVoid() ?
                        "(no file)" : NS_ConvertUTF16toUTF8(fileName).get(),
                        lineNumber);
    }

    nsCOMPtr<nsIStackFrame> nextFrame;
    nsresult rv = frame->GetCaller(getter_AddRefs(nextFrame));
    NS_ENSURE_SUCCESS_VOID(rv);
    frame = nextFrame;
  }
}

static void SandboxPrintStackFrame(uint32_t aFrameNumber, void *aPC, void *aSP,
                                   void *aClosure)
{
  char buf[1024];
  MozCodeAddressDetails details;

  MozDescribeCodeAddress(aPC, &details);
  MozFormatCodeAddressDetails(buf, sizeof(buf), aFrameNumber, aPC, &details);
  SANDBOX_LOG_ERROR("frame %s", buf);
}

static void
SandboxLogCStack()
{
  
  
  
  
  
  

  MozStackWalk(SandboxPrintStackFrame,  3,  0,
               nullptr, 0, nullptr);
  SANDBOX_LOG_ERROR("end of stack.");
}

static void
SandboxCrash(int nr, siginfo_t *info, void *void_context)
{
  pid_t pid = getpid(), tid = syscall(__NR_gettid);
  bool dumped = false;

#ifdef MOZ_CRASHREPORTER
  dumped = CrashReporter::WriteMinidumpForSigInfo(nr, info, void_context);
#endif
  if (!dumped) {
    SANDBOX_LOG_ERROR("crash reporter is disabled (or failed);"
                      " trying stack trace:");
    SandboxLogCStack();
  }

  
  SandboxLogJSStack();

  
  
  
  signal(SIGSYS, SIG_DFL);
  syscall(__NR_tgkill, pid, tid, nr);
}

static void __attribute__((constructor))
SandboxSetCrashFunc()
{
  gSandboxCrashFunc = SandboxCrash;
}

#ifndef ANDROID
SandboxCrashFunc gSandboxCrashFunc;
#endif

} 
