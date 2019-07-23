





































#ifndef mozilla__ipdltest_IPDLUnitTests_h
#define mozilla__ipdltest_IPDLUnitTests_h 1

#include "base/message_loop.h"
#include "base/process.h"
#include "chrome/common/ipc_channel.h"

#include "mozilla/ipc/GeckoThread.h"

#include "nsIAppShell.h"

#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsServiceManagerUtils.h" 
#include "nsWidgetsCID.h"       
#include "nsXULAppAPI.h"


#define MOZ_IPDL_TESTFAIL_LABEL "TEST-UNEXPECTED-FAIL"
#define MOZ_IPDL_TESTPASS_LABEL "TEST-PASS"


namespace mozilla {
namespace _ipdltest {



const char* const IPDLUnitTestName();





inline void fail(const char* fmt, ...)
{
  va_list ap;

  fprintf(stderr, MOZ_IPDL_TESTFAIL_LABEL " | %s | ", IPDLUnitTestName());

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fputc('\n', stderr);

  NS_RUNTIMEABORT("failed test");
}

inline void passed(const char* fmt, ...)
{
  va_list ap;

  printf(MOZ_IPDL_TESTPASS_LABEL " | %s | ", IPDLUnitTestName());

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);

  fputc('\n', stdout);
}




void IPDLUnitTestMain(void* aData);

void QuitParent();




void IPDLUnitTestChildInit(IPC::Channel* transport,
                           base::ProcessHandle parent,
                           MessageLoop* worker);

inline void QuitChild()
{
  XRE_ShutdownChildProcess();
}

} 
} 


#endif 
