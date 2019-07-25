




































#ifdef MOZ_IPC
#include "base/basictypes.h"
#endif

#include "nsXULAppAPI.h"

#include <stdlib.h>
#if defined(MOZ_WIDGET_GTK2)
#include <glib.h>
#endif

#include "prenv.h"

#include "nsIAppShell.h"
#include "nsIAppStartupNotifier.h"
#include "nsIDirectoryService.h"
#include "nsILocalFile.h"
#include "nsIToolkitChromeRegistry.h"
#include "nsIToolkitProfile.h"

#if defined(OS_LINUX)
#  define XP_LINUX
#endif

#ifdef XP_WIN
#include <process.h>
#endif

#include "nsAppDirectoryServiceDefs.h"
#include "nsAppRunner.h"
#include "nsAutoRef.h"
#include "nsDirectoryServiceDefs.h"
#include "nsExceptionHandler.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsWidgetsCID.h"
#include "nsXREDirProvider.h"

#ifdef MOZ_IPC
#include "nsX11ErrorHandler.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/message_loop.h"
#include "base/process_util.h"
#include "chrome/common/child_process.h"
#include "chrome/common/notification_service.h"

#include "mozilla/ipc/BrowserProcessSubThread.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "mozilla/ipc/IOThreadChild.h"
#include "mozilla/ipc/ProcessChild.h"
#include "ScopedXREEmbed.h"

#include "mozilla/jetpack/JetpackProcessChild.h"
#include "mozilla/plugins/PluginProcessChild.h"

#ifdef MOZ_IPDL_TESTS
#include "mozilla/_ipdltest/IPDLUnitTests.h"
#include "mozilla/_ipdltest/IPDLUnitTestProcessChild.h"

using mozilla::_ipdltest::IPDLUnitTestProcessChild;
#endif  

using mozilla::ipc::BrowserProcessSubThread;
using mozilla::ipc::GeckoChildProcessHost;
using mozilla::ipc::IOThreadChild;
using mozilla::ipc::ProcessChild;
using mozilla::ipc::ScopedXREEmbed;

using mozilla::jetpack::JetpackProcessChild;
using mozilla::plugins::PluginProcessChild;

using mozilla::startup::sChildProcessType;
#endif

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

#ifdef XP_WIN
static const PRUnichar kShellLibraryName[] =  L"shell32.dll";
#endif

nsresult
XRE_LockProfileDirectory(nsILocalFile* aDirectory,
                         nsISupports* *aLockObject)
{
  nsCOMPtr<nsIProfileLock> lock;

  nsresult rv = NS_LockProfilePath(aDirectory, nsnull, nsnull,
                                   getter_AddRefs(lock));
  if (NS_SUCCEEDED(rv))
    NS_ADDREF(*aLockObject = lock);

  return rv;
}

static PRInt32 sInitCounter;

nsresult
XRE_InitEmbedding2(nsILocalFile *aLibXULDirectory,
		   nsILocalFile *aAppDirectory,
		   nsIDirectoryServiceProvider *aAppDirProvider)
{
  
  static char* kNullCommandLine[] = { nsnull };
  gArgv = kNullCommandLine;
  gArgc = 0;

  NS_ENSURE_ARG(aLibXULDirectory);

  if (++sInitCounter > 1) 
    return NS_OK;

  if (!aAppDirectory)
    aAppDirectory = aLibXULDirectory;

  nsresult rv;

  new nsXREDirProvider; 
  if (!gDirServiceProvider)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = gDirServiceProvider->Initialize(aAppDirectory, aLibXULDirectory,
                                       aAppDirProvider);
  if (NS_FAILED(rv))
    return rv;

  rv = NS_InitXPCOM2(nsnull, aAppDirectory, gDirServiceProvider);
  if (NS_FAILED(rv))
    return rv;

  
  
  
  
  

  nsCOMPtr<nsIObserver> startupNotifier
    (do_CreateInstance(NS_APPSTARTUPNOTIFIER_CONTRACTID));
  if (!startupNotifier)
    return NS_ERROR_FAILURE;

  startupNotifier->Observe(nsnull, APPSTARTUP_TOPIC, nsnull);

  return NS_OK;
}

void
XRE_NotifyProfile()
{
  NS_ASSERTION(gDirServiceProvider, "XRE_InitEmbedding was not called!");
  gDirServiceProvider->DoStartup();
}

void
XRE_TermEmbedding()
{
  if (--sInitCounter != 0)
    return;

  NS_ASSERTION(gDirServiceProvider,
               "XRE_TermEmbedding without XRE_InitEmbedding");

  gDirServiceProvider->DoShutdown();
  NS_ShutdownXPCOM(nsnull);
  delete gDirServiceProvider;
}

const char*
XRE_ChildProcessTypeToString(GeckoProcessType aProcessType)
{
  return (aProcessType < GeckoProcessType_End) ?
    kGeckoProcessTypeString[aProcessType] : nsnull;
}

GeckoProcessType
XRE_StringToChildProcessType(const char* aProcessTypeString)
{
  for (int i = 0;
       i < (int) NS_ARRAY_LENGTH(kGeckoProcessTypeString);
       ++i) {
    if (!strcmp(kGeckoProcessTypeString[i], aProcessTypeString)) {
      return static_cast<GeckoProcessType>(i);
    }
  }
  return GeckoProcessType_Invalid;
}

#ifdef MOZ_IPC
namespace mozilla {
namespace startup {
GeckoProcessType sChildProcessType = GeckoProcessType_Default;
}
}

#if defined(MOZ_CRASHREPORTER)



PRBool
XRE_TakeMinidumpForChild(PRUint32 aChildPid, nsILocalFile** aDump)
{
  return CrashReporter::TakeMinidumpForChild(aChildPid, aDump);
}

#if !defined(XP_MACOSX)
PRBool
XRE_SetRemoteExceptionHandler(const char* aPipe)
{
#if defined(XP_WIN)
  return CrashReporter::SetRemoteExceptionHandler(nsDependentCString(aPipe));
#elif defined(OS_LINUX)
  return CrashReporter::SetRemoteExceptionHandler();
#else
#  error "OOP crash reporter unsupported on this platform"
#endif
}
#endif 
#endif 

#if defined(XP_WIN)
void
SetTaskbarGroupId(const nsString& aId)
{
    typedef HRESULT (WINAPI * SetCurrentProcessExplicitAppUserModelIDPtr)(PCWSTR AppID);

    SetCurrentProcessExplicitAppUserModelIDPtr funcAppUserModelID = nsnull;

    HMODULE hDLL = ::LoadLibraryW(kShellLibraryName);

    funcAppUserModelID = (SetCurrentProcessExplicitAppUserModelIDPtr)
                          GetProcAddress(hDLL, "SetCurrentProcessExplicitAppUserModelID");

    if (!funcAppUserModelID) {
        ::FreeLibrary(hDLL);
        return;
    }

    if (FAILED(funcAppUserModelID(aId.get()))) {
        NS_WARNING("SetCurrentProcessExplicitAppUserModelID failed for child process.");
    }

    if (hDLL)
        ::FreeLibrary(hDLL);
}
#endif

nsresult
XRE_InitChildProcess(int aArgc,
                     char* aArgv[],
                     GeckoProcessType aProcess)
{
  NS_ENSURE_ARG_MIN(aArgc, 2);
  NS_ENSURE_ARG_POINTER(aArgv);
  NS_ENSURE_ARG_POINTER(aArgv[0]);

  sChildProcessType = aProcess;

  gArgv = aArgv;
  gArgc = aArgc;

  SetupErrorHandling(aArgv[0]);
  
#if defined(MOZ_WIDGET_GTK2)
  g_thread_init(NULL);
#endif

  if (PR_GetEnv("MOZ_DEBUG_CHILD_PROCESS")) {
#ifdef OS_POSIX
      printf("\n\nCHILDCHILDCHILDCHILD\n  debug me @%d\n\n", getpid());
      sleep(30);
#elif defined(OS_WIN)
      printf("\n\nCHILDCHILDCHILDCHILD\n  debug me @%d\n\n", _getpid());
      Sleep(30000);
#endif
  }

  
  
  const char* const parentPIDString = aArgv[aArgc-1];
  NS_ABORT_IF_FALSE(parentPIDString, "NULL parent PID");
  --aArgc;

  char* end = 0;
  base::ProcessId parentPID = strtol(parentPIDString, &end, 10);
  NS_ABORT_IF_FALSE(!*end, "invalid parent PID");

  base::ProcessHandle parentHandle;
  bool ok = base::OpenProcessHandle(parentPID, &parentHandle);
  NS_ABORT_IF_FALSE(ok, "can't open handle to parent");

#if defined(XP_WIN)
  
  
  
  const char* const appModelUserId = aArgv[aArgc-1];
  --aArgc;
  if (appModelUserId) {
    
    if (*appModelUserId != '-') {
      nsString appId;
      appId.AssignWithConversion(nsDependentCString(appModelUserId));
      
      appId.Trim(NS_LITERAL_CSTRING("\"").get());
      
      SetTaskbarGroupId(appId);
    }
  }
#endif

  base::AtExitManager exitManager;
  NotificationService notificationService;

  NS_LogInit();

  int rv = XRE_InitCommandLine(aArgc, aArgv);
  if (NS_FAILED(rv)) {
    NS_LogTerm();
    return NS_ERROR_FAILURE;
  }

  
  MessageLoopForUI uiMessageLoop;
  {
    nsAutoPtr<ProcessChild> process;

    switch (aProcess) {
    case GeckoProcessType_Default:
      NS_RUNTIMEABORT("This makes no sense");
      break;

    case GeckoProcessType_Plugin:
      process = new PluginProcessChild(parentHandle);
      break;

    case GeckoProcessType_Jetpack:
      process = new JetpackProcessChild(parentHandle);
      break;

    case GeckoProcessType_IPDLUnitTest:
#ifdef MOZ_IPDL_TESTS
      process = new IPDLUnitTestProcessChild(parentHandle);
#else 
      NS_RUNTIMEABORT("rebuild with --enable-ipdl-tests");
#endif
      break;

    default:
      NS_RUNTIMEABORT("Unknown main thread class");
    }

    if (!process->Init()) {
      NS_LogTerm();
      return NS_ERROR_FAILURE;
    }

    
    uiMessageLoop.MessageLoop::Run();

    
    
    process->CleanUp();
  }

  NS_LogTerm();
  return XRE_DeinitCommandLine();
}

MessageLoop*
XRE_GetIOMessageLoop()
{
  if (sChildProcessType == GeckoProcessType_Default) {
    return BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO);
  }
  return IOThreadChild::message_loop();
}

namespace {

class MainFunctionRunnable : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  MainFunctionRunnable(MainFunction aFunction,
                       void* aData)
  : mFunction(aFunction),
    mData(aData)
  { 
    NS_ASSERTION(aFunction, "Don't give me a null pointer!");
  }

private:
  MainFunction mFunction;
  void* mData;
};

} 

NS_IMETHODIMP
MainFunctionRunnable::Run()
{
  mFunction(mData);
  return NS_OK;
}

nsresult
XRE_InitParentProcess(int aArgc,
                      char* aArgv[],
                      MainFunction aMainFunction,
                      void* aMainFunctionData)
{
  NS_ENSURE_ARG_MIN(aArgc, 1);
  NS_ENSURE_ARG_POINTER(aArgv);
  NS_ENSURE_ARG_POINTER(aArgv[0]);

  int rv = XRE_InitCommandLine(aArgc, aArgv);
  if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;

  ScopedXREEmbed embed;

  {
    embed.Start();

    nsCOMPtr<nsIAppShell> appShell(do_GetService(kAppShellCID));
    NS_ENSURE_TRUE(appShell, NS_ERROR_FAILURE);

    if (aMainFunction) {
      nsCOMPtr<nsIRunnable> runnable =
        new MainFunctionRunnable(aMainFunction, aMainFunctionData);
      NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

      nsresult rv = NS_DispatchToCurrentThread(runnable);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    if (NS_FAILED(appShell->Run())) {
      NS_WARNING("Failed to run appshell");
      return NS_ERROR_FAILURE;
    }
  }

  return XRE_DeinitCommandLine();
}

#ifdef MOZ_IPDL_TESTS



int
XRE_RunIPDLTest(int aArgc, char** aArgv)
{
    if (aArgc < 2) {
        fprintf(stderr, "TEST-UNEXPECTED-FAIL | <---> | insufficient #args, need at least 2\n");
        return 1;
    }

    void* data = reinterpret_cast<void*>(aArgv[aArgc-1]);

    nsresult rv =
        XRE_InitParentProcess(
            --aArgc, aArgv, mozilla::_ipdltest::IPDLUnitTestMain, data);
    NS_ENSURE_SUCCESS(rv, 1);

    return 0;
}
#endif  

nsresult
XRE_RunAppShell()
{
    nsCOMPtr<nsIAppShell> appShell(do_GetService(kAppShellCID));
    NS_ENSURE_TRUE(appShell, NS_ERROR_FAILURE);

    return appShell->Run();
}

void
XRE_ShutdownChildProcess()
{
  NS_ABORT_IF_FALSE(MessageLoopForUI::current(), "Wrong thread!");

  MessageLoop* ioLoop = XRE_GetIOMessageLoop();
  NS_ABORT_IF_FALSE(!!ioLoop, "Bad shutdown order");

  
  
  
  
  
  
  MessageLoop::current()->Quit(); 
}

#ifdef MOZ_X11
void
XRE_InstallX11ErrorHandler()
{
  InstallX11ErrorHandler();
}
#endif

#endif 

