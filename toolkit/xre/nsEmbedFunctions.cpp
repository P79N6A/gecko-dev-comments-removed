




































#include "nsXULAppAPI.h"

#include <stdlib.h>

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
#include "nsXREDirProvider.h"

#ifdef MOZ_IPC
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/message_loop.h"
#include "base/thread.h"
#include "chrome/common/child_process.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "mozilla/ipc/GeckoThread.h"
#include "ScopedXREEmbed.h"

#include "mozilla/plugins/PluginThreadChild.h"
#include "ContentProcessThread.h"
#include "ContentProcessParent.h"

#include "mozilla/ipc/TestShellParent.h"
#include "mozilla/ipc/XPCShellEnvironment.h"
#include "mozilla/test/TestParent.h"
#include "mozilla/test/TestProcessParent.h"
#include "mozilla/test/TestThreadChild.h"
#include "mozilla/Monitor.h"

using mozilla::ipc::BrowserProcessSubThread;
using mozilla::ipc::GeckoChildProcessHost;
using mozilla::ipc::GeckoThread;
using mozilla::ipc::ScopedXREEmbed;

using mozilla::plugins::PluginThreadChild;
using mozilla::dom::ContentProcessThread;
using mozilla::dom::ContentProcessParent;
using mozilla::ipc::TestShellParent;
using mozilla::ipc::XPCShellEnvironment;

using mozilla::test::TestParent;
using mozilla::test::TestProcessParent;
using mozilla::test::TestThreadChild;

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

nsresult
XRE_InitChildProcess(int aArgc,
                     char* aArgv[],
                     GeckoProcessType aProcess)
{
  NS_ENSURE_ARG_MIN(aArgc, 1);
  NS_ENSURE_ARG_POINTER(aArgv);
  NS_ENSURE_ARG_POINTER(aArgv[0]);

  if (PR_GetEnv("MOZ_DEBUG_CHILD_PROCESS")) {
#ifdef OS_POSIX
      printf("\n\nCHILDCHILDCHILDCHILD\n  debug me @%d\n\n", getpid());
      sleep(30);
#elif defined(OS_WIN)
      Sleep(30000);
#endif
  }

  base::AtExitManager exitManager;
  CommandLine::Init(aArgc, aArgv);
  MessageLoopForIO mainMessageLoop;

  {
    ChildThread* mainThread;

    switch (aProcess) {
    case GeckoProcessType_Default:
      mainThread = new GeckoThread();
      break;

    case GeckoProcessType_Plugin:
      mainThread = new PluginThreadChild();
      break;

    case GeckoProcessType_Content:
      mainThread = new ContentProcessThread();
      break;

    case GeckoProcessType_TestHarness:
      mainThread = new TestThreadChild();
      break;

    default:
      NS_RUNTIMEABORT("Unknown main thread class");
    }

    sChildProcessType = aProcess;
    ChildProcess process(mainThread);

    
    MessageLoop::current()->Run();
  }

  return NS_OK;
}

GeckoProcessType
XRE_GetProcessType()
{
  return sChildProcessType;
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

  base::AtExitManager exitManager;
  CommandLine::Init(aArgc, aArgv);
  ScopedXREEmbed embed;
  MessageLoopForUI mainMessageLoop;

  {
    
#if defined(OS_LINUX)
    
    
    scoped_ptr<base::Thread> x11Thread(
      new BrowserProcessSubThread(BrowserProcessSubThread::BACKGROUND_X11));
    if (NS_UNLIKELY(!x11Thread->Start())) {
      NS_ERROR("Failed to create chromium's X11 thread!");
      return NS_ERROR_FAILURE;
    }
#endif
    scoped_ptr<base::Thread> ipcThread(
      new BrowserProcessSubThread(BrowserProcessSubThread::IO));
    base::Thread::Options options;
    options.message_loop_type = MessageLoop::TYPE_IO;
    if (NS_UNLIKELY(!ipcThread->StartWithOptions(options))) {
      NS_ERROR("Failed to create chromium's IO thread!");
      return NS_ERROR_FAILURE;
    }

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

  return NS_OK;
}

NS_SPECIALIZE_TEMPLATE
class nsAutoRefTraits<XPCShellEnvironment> :
  public nsPointerRefTraits<XPCShellEnvironment>
{
public:
  void Release(XPCShellEnvironment* aEnv) {
    XPCShellEnvironment::DestroyEnvironment(aEnv);
  }
};

namespace {

class QuitRunnable : public nsRunnable
{
public:
  NS_IMETHOD Run() {
    nsCOMPtr<nsIAppShell> appShell(do_GetService(kAppShellCID));
    NS_ENSURE_TRUE(appShell, NS_ERROR_FAILURE);

    return appShell->Exit();
  }
};

int
TestShellMain(int argc, char** argv)
{
  
  nsIRunnable* quitRunnable = new QuitRunnable();
  NS_ENSURE_TRUE(quitRunnable, 1);

  nsresult rv = NS_DispatchToCurrentThread(quitRunnable);
  NS_ENSURE_SUCCESS(rv, 1);

  nsAutoRef<XPCShellEnvironment> env(XPCShellEnvironment::CreateEnvironment());
  NS_ENSURE_TRUE(env, 1);

  ContentProcessParent* childProcess = ContentProcessParent::GetSingleton();
  if (!childProcess)
    return 1;

  TestShellParent* testShellParent = childProcess->CreateTestShell();

  env->DefineIPCCommands(testShellParent);

  const char* filename = argc > 1 ? argv[1] : nsnull;
  env->Process(filename);

  return env->ExitCode();
}

struct TestShellData {
  int* result;
  int argc;
  char** argv;
};

void
TestShellMainWrapper(void* aData)
{
  NS_ASSERTION(aData, "Don't give me a null pointer!");
  TestShellData& testShellData = *static_cast<TestShellData*>(aData);

  *testShellData.result =
    TestShellMain(testShellData.argc, testShellData.argv);
}

} 

int
XRE_RunTestShell(int aArgc, char* aArgv[])
{
    int result;

    TestShellData data = { &result, aArgc, aArgv };

    nsresult rv =
      XRE_InitParentProcess(aArgc, aArgv, TestShellMainWrapper, &data);
    NS_ENSURE_SUCCESS(rv, 1);

    return result;
}




static void
IPCTestHarnessMain(void* data)
{
    TestProcessParent* subprocess = new TestProcessParent(); 
    bool launched = subprocess->SyncLaunch();
    NS_ASSERTION(launched, "can't launch subprocess");

    TestParent* parent = new TestParent(); 
    parent->Open(subprocess->GetChannel());
    parent->DoStuff();
}

int
XRE_RunIPCTestHarness(int aArgc, char* aArgv[])
{
    nsresult rv =
        XRE_InitParentProcess(
            aArgc, aArgv, IPCTestHarnessMain, NULL);
    NS_ENSURE_SUCCESS(rv, 1);
    return 0;
}
#endif
