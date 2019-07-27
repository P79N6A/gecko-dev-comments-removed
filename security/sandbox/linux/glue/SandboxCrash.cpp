










#include "SandboxInternal.h"
#include "SandboxLogging.h"

#include <unistd.h>
#include <sys/syscall.h>

#include "mozilla/NullPtr.h"
#include "mozilla/unused.h"
#include "mozilla/dom/Exceptions.h"
#include "nsContentUtils.h"
#include "nsString.h"
#include "nsThreadUtils.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

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

static void
SandboxCrash(int nr, siginfo_t *info, void *void_context)
{
  pid_t pid = getpid(), tid = syscall(__NR_gettid);

#ifdef MOZ_CRASHREPORTER
  bool dumped = CrashReporter::WriteMinidumpForSigInfo(nr, info, void_context);
  if (!dumped) {
    SANDBOX_LOG_ERROR("Failed to write minidump");
  }
#endif

  
  SandboxLogJSStack();

  
  
  
  signal(SIGSYS, SIG_DFL);
  syscall(__NR_tgkill, pid, tid, nr);
}

static void __attribute__((constructor))
SandboxSetCrashFunc()
{
  gSandboxCrashFunc = SandboxCrash;
}

} 
