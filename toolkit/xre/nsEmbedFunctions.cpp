




































#include "nsXULAppAPI.h"

#include <stdlib.h>

#include "prenv.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/message_loop.h"
#include "base/thread.h"
#include "chrome/common/child_process.h"

#include "nsIAppShell.h"
#include "nsIAppStartupNotifier.h"
#include "nsIDirectoryService.h"
#include "nsILocalFile.h"
#include "nsIToolkitChromeRegistry.h"
#include "nsIToolkitProfile.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsAppRunner.h"
#include "nsDirectoryServiceDefs.h"
#include "nsStaticComponents.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsWidgetsCID.h"
#include "nsXREDirProvider.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "mozilla/ipc/GeckoThread.h"
#include "ScopedXREEmbed.h"

#include "mozilla/plugins/PluginThreadChild.h"

using mozilla::ipc::BrowserProcessSubThread;
using mozilla::ipc::GeckoChildProcessHost;
using mozilla::ipc::GeckoThread;
using mozilla::ipc::ScopedXREEmbed;

using mozilla::plugins::PluginThreadChild;

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

nsresult
XRE_InitChildProcess(int aArgc,
                     char* aArgv[],
                     const char* aMainThreadClass)
{
  NS_ENSURE_ARG_MIN(aArgc, 1);
  NS_ENSURE_ARG_POINTER(aArgv);
  NS_ENSURE_ARG_POINTER(aArgv[0]);

  if (PR_GetEnv("MOZ_DEBUG_CHILD_PROCESS")) {
      printf("\n\nCHILDCHILDCHILDCHILD\n  debug me @%d\n\n", getpid());
      sleep(30);
  }

  base::AtExitManager exitManager;
  CommandLine::Init(aArgc, aArgv);
  MessageLoopForIO mainMessageLoop;

  {
    GeckoThread* mainThread;

    if (!aMainThreadClass)
      mainThread = new GeckoThread();
    else if (!strcmp("PluginThreadChild", aMainThreadClass))
      mainThread = new PluginThreadChild();
    else {
        NS_RUNTIMEABORT("Unknown main thread class");
    }

    ChildProcess process(mainThread);

    
    MessageLoop::current()->Run();
  }

  return NS_OK;
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

    ScopedXREEmbed embed;
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

namespace {

class CreateChildProcess : public Task
{
public:
  virtual void Run() {
    GeckoChildProcessHost* host = new GeckoChildProcessHost();
    if (!host->Init()) {
      delete host;
    }
    
    
  }
};

} 

nsresult
XRE_LaunchChildProcess()
{
  MessageLoop* ioLoop = 
      BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO);
  NS_ENSURE_TRUE(ioLoop, NS_ERROR_NOT_INITIALIZED);

  ioLoop->PostTask(FROM_HERE, new CreateChildProcess());
  return NS_OK;
}
