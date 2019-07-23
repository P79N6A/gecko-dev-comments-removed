




































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

#include "nsAppDirectoryServiceDefs.h"
#include "nsAppRunner.h"
#include "nsAutoRef.h"
#include "nsDirectoryServiceDefs.h"
#include "nsStaticComponents.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsWidgetsCID.h"
#include "nsXPFEComponentsCID.h"
#include "nsXREDirProvider.h"

#ifdef MOZ_IPC
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/message_loop.h"
#include "base/process_util.h"
#include "chrome/common/child_process.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "mozilla/ipc/GeckoThread.h"
#include "ScopedXREEmbed.h"

#include "mozilla/plugins/PluginThreadChild.h"
#include "mozilla/dom/ContentProcessThread.h"
#include "mozilla/dom/ContentProcessParent.h"
#include "mozilla/dom/ContentProcessChild.h"

#include "mozilla/ipc/TestShellParent.h"
#include "mozilla/ipc/XPCShellEnvironment.h"
#include "mozilla/Monitor.h"

#ifdef MOZ_IPDL_TESTS
#include "mozilla/_ipdltest/IPDLUnitTests.h"
#include "mozilla/_ipdltest/IPDLUnitTestThreadChild.h"

using mozilla::_ipdltest::IPDLUnitTestThreadChild;
#endif  

using mozilla::ipc::GeckoChildProcessHost;
using mozilla::ipc::GeckoThread;
using mozilla::ipc::BrowserProcessSubThread;
using mozilla::ipc::ScopedXREEmbed;

using mozilla::plugins::PluginThreadChild;
using mozilla::dom::ContentProcessThread;
using mozilla::dom::ContentProcessParent;
using mozilla::dom::ContentProcessChild;
using mozilla::ipc::TestShellParent;
using mozilla::ipc::TestShellCommandParent;
using mozilla::ipc::XPCShellEnvironment;

using mozilla::Monitor;
using mozilla::MonitorAutoEnter;
#endif

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

void
XRE_GetStaticComponents(nsStaticModuleInfo const **aStaticComponents,
                        PRUint32 *aComponentCount)
{
  *aStaticComponents = kPStaticModules;
  *aComponentCount = kStaticModuleCount;
}

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

static nsStaticModuleInfo *sCombined;
static PRInt32 sInitCounter;

nsresult
XRE_InitEmbedding(nsILocalFile *aLibXULDirectory,
                  nsILocalFile *aAppDirectory,
                  nsIDirectoryServiceProvider *aAppDirProvider,
                  nsStaticModuleInfo const *aStaticComponents,
                  PRUint32 aStaticComponentCount)
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

  
  PRUint32 combinedCount = kStaticModuleCount + aStaticComponentCount;

  sCombined = new nsStaticModuleInfo[combinedCount];
  if (!sCombined)
    return NS_ERROR_OUT_OF_MEMORY;

  memcpy(sCombined, kPStaticModules,
         sizeof(nsStaticModuleInfo) * kStaticModuleCount);
  memcpy(sCombined + kStaticModuleCount, aStaticComponents,
         sizeof(nsStaticModuleInfo) * aStaticComponentCount);

  rv = NS_InitXPCOM3(nsnull, aAppDirectory, gDirServiceProvider,
                     sCombined, combinedCount);
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
  delete [] sCombined;
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
static GeckoProcessType sChildProcessType = GeckoProcessType_Default;

static MessageLoop* sIOMessageLoop;

nsresult
XRE_InitChildProcess(int aArgc,
                     char* aArgv[],
                     GeckoProcessType aProcess)
{
  NS_ENSURE_ARG_MIN(aArgc, 2);
  NS_ENSURE_ARG_POINTER(aArgv);
  NS_ENSURE_ARG_POINTER(aArgv[0]);

  sChildProcessType = aProcess;
  
#if defined(MOZ_WIDGET_GTK2)
  g_thread_init(NULL);
#endif

  if (PR_GetEnv("MOZ_DEBUG_CHILD_PROCESS")) {
#ifdef OS_POSIX
      printf("\n\nCHILDCHILDCHILDCHILD\n  debug me @%d\n\n", getpid());
      sleep(30);
#elif defined(OS_WIN)
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

  base::AtExitManager exitManager;

  NS_LogInit();

  int rv = XRE_InitCommandLine(aArgc, aArgv);
  if (NS_FAILED(rv)) {
    NS_LogTerm();
    return NS_ERROR_FAILURE;
  }

  MessageLoopForIO mainMessageLoop;

  {
    ChildThread* mainThread;

    switch (aProcess) {
    case GeckoProcessType_Default:
      mainThread = new GeckoThread(parentHandle);
      break;

    case GeckoProcessType_Plugin:
      mainThread = new PluginThreadChild(parentHandle);
      break;

    case GeckoProcessType_Content:
      mainThread = new ContentProcessThread(parentHandle);
      break;

    case GeckoProcessType_IPDLUnitTest:
#ifdef MOZ_IPDL_TESTS
      mainThread = new IPDLUnitTestThreadChild(parentHandle);
#else
      NS_RUNTIMEABORT("rebuild with --enable-ipdl-tests");
#endif
      break;

    default:
      NS_RUNTIMEABORT("Unknown main thread class");
    }

    ChildProcess process(mainThread);

    
    sIOMessageLoop = MessageLoop::current();

    sIOMessageLoop->Run();

    sIOMessageLoop = nsnull;
  }

  NS_LogTerm();
  return XRE_DeinitCommandLine();
}

MessageLoop*
XRE_GetIOMessageLoop()
{
  if (sChildProcessType == GeckoProcessType_Default) {
    NS_ASSERTION(!sIOMessageLoop, "Shouldn't be set on parent process!");
    return BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO);
  }
  return sIOMessageLoop;
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

template<>
struct RunnableMethodTraits<ContentProcessChild>
{
    static void RetainCallee(ContentProcessChild* obj) { }
    static void ReleaseCallee(ContentProcessChild* obj) { }
};

void
XRE_ShutdownChildProcess()
{
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "Wrong thread!");

    MessageLoop* uiLoop = MessageLoop::current();
    MessageLoop* ioLoop = XRE_GetIOMessageLoop();

    NS_ABORT_IF_FALSE(!!ioLoop, "Bad shutdown order");
    ioLoop->PostTask(FROM_HERE, new MessageLoop::QuitTask());

    NS_ABORT_IF_FALSE(!!uiLoop, "Bad shutdown order");
    if (GeckoProcessType_Content == XRE_GetProcessType()) {
        uiLoop->PostTask(
            FROM_HERE,
            NewRunnableMethod(ContentProcessChild::GetSingleton(),
                              &ContentProcessChild::Quit));
    }
    else {
        uiLoop->Quit();
    }
}

namespace {
TestShellParent* gTestShellParent = nsnull;
}

bool
XRE_SendTestShellCommand(JSContext* aCx,
                         JSString* aCommand,
                         void* aCallback)
{
    if (!gTestShellParent) {
        ContentProcessParent* parent = ContentProcessParent::GetSingleton();
        NS_ENSURE_TRUE(parent, false);

        gTestShellParent = parent->CreateTestShell();
        NS_ENSURE_TRUE(gTestShellParent, false);
    }

    nsDependentString command((PRUnichar*)JS_GetStringChars(aCommand),
                              JS_GetStringLength(aCommand));
    if (!aCallback) {
        return gTestShellParent->SendExecuteCommand(command);
    }

    TestShellCommandParent* callback = static_cast<TestShellCommandParent*>(
        gTestShellParent->SendPTestShellCommandConstructor(command));
    NS_ENSURE_TRUE(callback, false);

    jsval callbackVal = *reinterpret_cast<jsval*>(aCallback);
    NS_ENSURE_TRUE(callback->SetCallback(aCx, callbackVal), false);

    return true;
}

bool
XRE_ShutdownTestShell()
{
  if (!gTestShellParent)
    return true;
  return ContentProcessParent::GetSingleton()->DestroyTestShell(gTestShellParent);
}

#endif 


nsresult
XRE_InitCommandLine(int aArgc, char* aArgv[])
{
  nsresult rv = NS_OK;

#if defined(MOZ_IPC)

#if defined(OS_WIN)
  CommandLine::Init(aArgc, aArgv);
#else
  
  char** canonArgs = new char*[aArgc];

  
  nsCOMPtr<nsILocalFile> binFile;
  rv = XRE_GetBinaryPath(aArgv[0], getter_AddRefs(binFile));
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  nsCAutoString canonBinPath;
  rv = binFile->GetNativePath(canonBinPath);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  canonArgs[0] = strdup(canonBinPath.get());

  for (int i = 1; i < aArgc; ++i) {
    if (aArgv[i]) {
      canonArgs[i] = strdup(aArgv[i]);
    }
  }
 
  NS_ASSERTION(!CommandLine::IsInitialized(), "Bad news!");
  CommandLine::Init(aArgc, canonArgs);

  for (int i = 0; i < aArgc; ++i)
      free(canonArgs[i]);
  delete[] canonArgs;
#endif
#endif
  return rv;
}

nsresult
XRE_DeinitCommandLine()
{
  nsresult rv = NS_OK;

#if defined(MOZ_IPC)
  CommandLine::Terminate();
#endif

  return rv;
}

GeckoProcessType
XRE_GetProcessType()
{
#ifdef MOZ_IPC
  return sChildProcessType;
#else
  return GeckoProcessType_Default;
#endif
}

