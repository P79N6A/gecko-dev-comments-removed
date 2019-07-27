



#include "mozilla/DebugOnly.h"

#if defined(MOZ_WIDGET_QT)
#include "nsQAppInstance.h"
#endif

#include "base/basictypes.h"

#include "nsXULAppAPI.h"

#include <stdlib.h>
#if defined(MOZ_WIDGET_GTK)
#include <glib.h>
#endif

#include "prenv.h"

#include "nsIAppShell.h"
#include "nsIAppStartupNotifier.h"
#include "nsIDirectoryService.h"
#include "nsIFile.h"
#include "nsIToolkitChromeRegistry.h"
#include "nsIToolkitProfile.h"

#ifdef XP_WIN
#include <process.h>
#include "mozilla/ipc/WindowsMessageLoop.h"
#endif

#include "nsAppDirectoryServiceDefs.h"
#include "nsAppRunner.h"
#include "nsAutoRef.h"
#include "nsDirectoryServiceDefs.h"
#include "nsExceptionHandler.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsJSUtils.h"
#include "nsWidgetsCID.h"
#include "nsXREDirProvider.h"

#include "mozilla/Omnijar.h"
#if defined(XP_MACOSX)
#include "nsVersionComparator.h"
#include "chrome/common/mach_ipc_mac.h"
#endif
#include "nsX11ErrorHandler.h"
#include "nsGDKErrorHandler.h"
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

#include "mozilla/plugins/PluginProcessChild.h"
#include "mozilla/dom/ContentProcess.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ContentChild.h"

#include "mozilla/ipc/TestShellParent.h"
#include "mozilla/ipc/XPCShellEnvironment.h"
#include "mozilla/WindowsDllBlocklist.h"

#include "GMPProcessChild.h"
#include "GMPLoader.h"

#include "GeckoProfiler.h"

#if defined(MOZ_SANDBOX) && defined(XP_WIN)
#define TARGET_SANDBOX_EXPORTS
#include "mozilla/sandboxing/loggingCallbacks.h"
#endif

#ifdef MOZ_IPDL_TESTS
#include "mozilla/_ipdltest/IPDLUnitTests.h"
#include "mozilla/_ipdltest/IPDLUnitTestProcessChild.h"

using mozilla::_ipdltest::IPDLUnitTestProcessChild;
#endif  

#ifdef MOZ_B2G_LOADER
#include "nsLocalFile.h"
#include "nsXREAppData.h"
#endif

using namespace mozilla;

using mozilla::ipc::BrowserProcessSubThread;
using mozilla::ipc::GeckoChildProcessHost;
using mozilla::ipc::IOThreadChild;
using mozilla::ipc::ProcessChild;
using mozilla::ipc::ScopedXREEmbed;

using mozilla::plugins::PluginProcessChild;
using mozilla::dom::ContentProcess;
using mozilla::dom::ContentParent;
using mozilla::dom::ContentChild;

using mozilla::gmp::GMPLoader;
using mozilla::gmp::CreateGMPLoader;
using mozilla::gmp::GMPProcessChild;

using mozilla::ipc::TestShellParent;
using mozilla::ipc::TestShellCommandParent;
using mozilla::ipc::XPCShellEnvironment;

using mozilla::startup::sChildProcessType;

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

#ifdef XP_WIN
static const wchar_t kShellLibraryName[] =  L"shell32.dll";
#endif

nsresult
XRE_LockProfileDirectory(nsIFile* aDirectory,
                         nsISupports* *aLockObject)
{
  nsCOMPtr<nsIProfileLock> lock;

  nsresult rv = NS_LockProfilePath(aDirectory, nullptr, nullptr,
                                   getter_AddRefs(lock));
  if (NS_SUCCEEDED(rv))
    NS_ADDREF(*aLockObject = lock);

  return rv;
}

static int32_t sInitCounter;

nsresult
XRE_InitEmbedding2(nsIFile *aLibXULDirectory,
		   nsIFile *aAppDirectory,
		   nsIDirectoryServiceProvider *aAppDirProvider)
{
  
  static char* kNullCommandLine[] = { nullptr };
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

  rv = NS_InitXPCOM2(nullptr, aAppDirectory, gDirServiceProvider);
  if (NS_FAILED(rv))
    return rv;

  
  
  
  
  

  nsCOMPtr<nsIObserver> startupNotifier
    (do_CreateInstance(NS_APPSTARTUPNOTIFIER_CONTRACTID));
  if (!startupNotifier)
    return NS_ERROR_FAILURE;

  startupNotifier->Observe(nullptr, APPSTARTUP_TOPIC, nullptr);

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
  NS_ShutdownXPCOM(nullptr);
  delete gDirServiceProvider;
}

const char*
XRE_ChildProcessTypeToString(GeckoProcessType aProcessType)
{
  return (aProcessType < GeckoProcessType_End) ?
    kGeckoProcessTypeString[aProcessType] : nullptr;
}

namespace mozilla {
namespace startup {
GeckoProcessType sChildProcessType = GeckoProcessType_Default;
}
}

void
XRE_SetProcessType(const char* aProcessTypeString)
{
  static bool called = false;
  if (called) {
    MOZ_CRASH();
  }
  called = true;

  sChildProcessType = GeckoProcessType_Invalid;
  for (int i = 0;
       i < (int) ArrayLength(kGeckoProcessTypeString);
       ++i) {
    if (!strcmp(kGeckoProcessTypeString[i], aProcessTypeString)) {
      sChildProcessType = static_cast<GeckoProcessType>(i);
      return;
    }
  }
}

#if defined(MOZ_CRASHREPORTER)



bool
XRE_TakeMinidumpForChild(uint32_t aChildPid, nsIFile** aDump,
                         uint32_t* aSequence)
{
  return CrashReporter::TakeMinidumpForChild(aChildPid, aDump, aSequence);
}

bool
XRE_SetRemoteExceptionHandler(const char* aPipe)
{
#if defined(XP_WIN) || defined(XP_MACOSX)
  return CrashReporter::SetRemoteExceptionHandler(nsDependentCString(aPipe));
#elif defined(OS_LINUX)
  return CrashReporter::SetRemoteExceptionHandler();
#else
#  error "OOP crash reporter unsupported on this platform"
#endif
}
#endif 

#if defined(XP_WIN)
void
SetTaskbarGroupId(const nsString& aId)
{
    typedef HRESULT (WINAPI * SetCurrentProcessExplicitAppUserModelIDPtr)(PCWSTR AppID);

    SetCurrentProcessExplicitAppUserModelIDPtr funcAppUserModelID = nullptr;

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
                     GMPLoader* aGMPLoader)
{
  NS_ENSURE_ARG_MIN(aArgc, 2);
  NS_ENSURE_ARG_POINTER(aArgv);
  NS_ENSURE_ARG_POINTER(aArgv[0]);

#ifdef HAS_DLL_BLOCKLIST
  DllBlocklist_Initialize();
#endif

#if !defined(MOZ_WIDGET_ANDROID) && !defined(MOZ_WIDGET_GONK)
  
  
  GMPProcessChild::SetGMPLoader(aGMPLoader);
#else
  
  
  
  nsAutoPtr<GMPLoader> loader(CreateGMPLoader(nullptr));
  GMPProcessChild::SetGMPLoader(loader);
#endif

#if defined(XP_WIN)
  
  
  
  
  
  
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    
    
    
    if (_fileno(stdout) == -1 ||
        _get_osfhandle(fileno(stdout)) == -1)
        freopen("CONOUT$", "w", stdout);
    
    
    if (_fileno(stderr) == -1 ||
        _get_osfhandle(fileno(stderr)) == -1)
        freopen("CONOUT$", "w", stderr);
    if (_fileno(stdin) == -1 || _get_osfhandle(fileno(stdin)) == -1)
        freopen("CONIN$", "r", stdin);
  }
#endif

  char aLocal;
  profiler_init(&aLocal);

  PROFILER_LABEL("Startup", "XRE_InitChildProcess",
    js::ProfileEntry::Category::OTHER);

  
  
#ifdef XP_MACOSX
  if (aArgc < 1)
    return NS_ERROR_FAILURE;
  const char* const mach_port_name = aArgv[--aArgc];

  const int kTimeoutMs = 1000;

  MachSendMessage child_message(0);
  if (!child_message.AddDescriptor(MachMsgPortDescriptor(mach_task_self()))) {
    NS_WARNING("child AddDescriptor(mach_task_self()) failed.");
    return NS_ERROR_FAILURE;
  }

  ReceivePort child_recv_port;
  mach_port_t raw_child_recv_port = child_recv_port.GetPort();
  if (!child_message.AddDescriptor(MachMsgPortDescriptor(raw_child_recv_port))) {
    NS_WARNING("Adding descriptor to message failed");
    return NS_ERROR_FAILURE;
  }

  MachPortSender child_sender(mach_port_name);
  kern_return_t err = child_sender.SendMessage(child_message, kTimeoutMs);
  if (err != KERN_SUCCESS) {
    NS_WARNING("child SendMessage() failed");
    return NS_ERROR_FAILURE;
  }

  MachReceiveMessage parent_message;
  err = child_recv_port.WaitForMessage(&parent_message, kTimeoutMs);
  if (err != KERN_SUCCESS) {
    NS_WARNING("child WaitForMessage() failed");
    return NS_ERROR_FAILURE;
  }

  if (parent_message.GetTranslatedPort(0) == MACH_PORT_NULL) {
    NS_WARNING("child GetTranslatedPort(0) failed");
    return NS_ERROR_FAILURE;
  }
  err = task_set_bootstrap_port(mach_task_self(),
                                parent_message.GetTranslatedPort(0));
  if (err != KERN_SUCCESS) {
    NS_WARNING("child task_set_bootstrap_port() failed");
    return NS_ERROR_FAILURE;
  }
#endif

  SetupErrorHandling(aArgv[0]);  

#if defined(MOZ_CRASHREPORTER)
  if (aArgc < 1)
    return NS_ERROR_FAILURE;
  const char* const crashReporterArg = aArgv[--aArgc];
  
#  if defined(XP_WIN) || defined(XP_MACOSX)
  
  
  
  if (0 != strcmp("-", crashReporterArg) && 
      !XRE_SetRemoteExceptionHandler(crashReporterArg)) {
    
    NS_WARNING("Could not setup crash reporting\n");
  }
#  elif defined(OS_LINUX)
  
  
  if (0 != strcmp("false", crashReporterArg) && 
      !XRE_SetRemoteExceptionHandler(nullptr)) {
    
    NS_WARNING("Could not setup crash reporting\n");
  }
#  else
#    error "OOP crash reporting unsupported on this platform"
#  endif   
#endif 

  gArgv = aArgv;
  gArgc = aArgc;

#ifdef MOZ_X11
  XInitThreads();
#endif
#if defined(MOZ_WIDGET_GTK)
  g_thread_init(nullptr);
#endif

#if defined(MOZ_WIDGET_QT)
  nsQAppInstance::AddRef();
#endif

  if (PR_GetEnv("MOZ_DEBUG_CHILD_PROCESS")) {
#ifdef OS_POSIX
      printf("\n\nCHILDCHILDCHILDCHILD\n  debug me @ %d\n\n", getpid());
      sleep(30);
#elif defined(OS_WIN)
      
      
      NS_DebugBreak(NS_DEBUG_BREAK,
                    "Invoking NS_DebugBreak() to debug child process",
                    nullptr, __FILE__, __LINE__);
#endif
  }

  
  
  const char* const parentPIDString = aArgv[aArgc-1];
  MOZ_ASSERT(parentPIDString, "NULL parent PID");
  --aArgc;

  char* end = 0;
  base::ProcessId parentPID = strtol(parentPIDString, &end, 10);
  MOZ_ASSERT(!*end, "invalid parent PID");

#if defined(XP_WIN)
  
  
  
  const char* const appModelUserId = aArgv[--aArgc];
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

  nsresult rv = XRE_InitCommandLine(aArgc, aArgv);
  if (NS_FAILED(rv)) {
    profiler_shutdown();
    NS_LogTerm();
    return NS_ERROR_FAILURE;
  }

  MessageLoop::Type uiLoopType;
  switch (XRE_GetProcessType()) {
  case GeckoProcessType_Content:
      
      uiLoopType = MessageLoop::TYPE_MOZILLA_CHILD;
      break;
  case GeckoProcessType_GMPlugin:
      uiLoopType = MessageLoop::TYPE_DEFAULT;
      break;
  default:
      uiLoopType = MessageLoop::TYPE_UI;
      break;
  }

  {
    
    
    
    

    
    MessageLoop uiMessageLoop(uiLoopType);
    {
      nsAutoPtr<ProcessChild> process;

#ifdef XP_WIN
      mozilla::ipc::windows::InitUIThread();
#endif

      switch (XRE_GetProcessType()) {
      case GeckoProcessType_Default:
        NS_RUNTIMEABORT("This makes no sense");
        break;

      case GeckoProcessType_Plugin:
        process = new PluginProcessChild(parentPID);
        break;

      case GeckoProcessType_Content: {
          process = new ContentProcess(parentPID);
          
          nsCString appDir;
          for (int idx = aArgc; idx > 0; idx--) {
            if (aArgv[idx] && !strcmp(aArgv[idx], "-appdir")) {
              appDir.Assign(nsDependentCString(aArgv[idx+1]));
              static_cast<ContentProcess*>(process.get())->SetAppDir(appDir);
              break;
            }
          }
        }
        break;

      case GeckoProcessType_IPDLUnitTest:
#ifdef MOZ_IPDL_TESTS
        process = new IPDLUnitTestProcessChild(parentPID);
#else 
        NS_RUNTIMEABORT("rebuild with --enable-ipdl-tests");
#endif
        break;

      case GeckoProcessType_GMPlugin:
        process = new gmp::GMPProcessChild(parentPID);
        break;

      default:
        NS_RUNTIMEABORT("Unknown main thread class");
      }

      if (!process->Init()) {
        profiler_shutdown();
        NS_LogTerm();
        return NS_ERROR_FAILURE;
      }

#if defined(MOZ_SANDBOX) && defined(XP_WIN)
      
      
      mozilla::sandboxing::InitLoggingIfRequired();
#endif

      
      uiMessageLoop.MessageLoop::Run();

      
      
      process->CleanUp();
      mozilla::Omnijar::CleanUp();
    }
  }

  profiler_shutdown();
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

  ScopedXREEmbed embed;

  gArgc = aArgc;
  gArgv = aArgv;
  nsresult rv = XRE_InitCommandLine(gArgc, gArgv);
  if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;

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
#if defined(XP_MACOSX)
    {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      MessageLoop* loop = MessageLoop::current();
      bool couldNest = loop->NestableTasksAllowed();

      loop->SetNestableTasksAllowed(true);
      loop->PostTask(FROM_HERE, new MessageLoop::QuitTask());
      loop->Run();

      loop->SetNestableTasksAllowed(couldNest);
    }
#endif  
    return appShell->Run();
}

template<>
struct RunnableMethodTraits<ContentChild>
{
    static void RetainCallee(ContentChild* obj) { }
    static void ReleaseCallee(ContentChild* obj) { }
};

void
XRE_ShutdownChildProcess()
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

  mozilla::DebugOnly<MessageLoop*> ioLoop = XRE_GetIOMessageLoop();
  MOZ_ASSERT(!!ioLoop, "Bad shutdown order");

  
  
  
  
  
  
  MessageLoop::current()->Quit();
#if defined(XP_MACOSX)
  nsCOMPtr<nsIAppShell> appShell(do_GetService(kAppShellCID));
  if (appShell) {
      
      
      
      
      appShell->Exit();
  }
#endif 
}

namespace {
ContentParent* gContentParent; 
TestShellParent* GetOrCreateTestShellParent()
{
    if (!gContentParent) {
        nsRefPtr<ContentParent> parent = ContentParent::GetNewOrUsedBrowserProcess();
        parent.forget(&gContentParent);
    } else if (!gContentParent->IsAlive()) {
        return nullptr;
    }
    TestShellParent* tsp = gContentParent->GetTestShellSingleton();
    if (!tsp) {
        tsp = gContentParent->CreateTestShell();
    }
    return tsp;
}
}

bool
XRE_SendTestShellCommand(JSContext* aCx,
                         JSString* aCommand,
                         void* aCallback)
{
    JS::RootedString cmd(aCx, aCommand);
    TestShellParent* tsp = GetOrCreateTestShellParent();
    NS_ENSURE_TRUE(tsp, false);

    nsAutoJSString command;
    NS_ENSURE_TRUE(command.init(aCx, cmd), false);

    if (!aCallback) {
        return tsp->SendExecuteCommand(command);
    }

    TestShellCommandParent* callback = static_cast<TestShellCommandParent*>(
        tsp->SendPTestShellCommandConstructor(command));
    NS_ENSURE_TRUE(callback, false);

    JS::Value callbackVal = *reinterpret_cast<JS::Value*>(aCallback);
    NS_ENSURE_TRUE(callback->SetCallback(aCx, callbackVal), false);

    return true;
}

bool
XRE_ShutdownTestShell()
{
    if (!gContentParent) {
        return true;
    }
    bool ret = true;
    if (gContentParent->IsAlive()) {
        ret = gContentParent->DestroyTestShell(
            gContentParent->GetTestShellSingleton());
    }
    NS_RELEASE(gContentParent);
    return ret;
}

#ifdef MOZ_X11
void
XRE_InstallX11ErrorHandler()
{
#if (MOZ_WIDGET_GTK == 3)
  InstallGdkErrorHandler();
#else
  InstallX11ErrorHandler();
#endif
}
#endif

#ifdef MOZ_B2G_LOADER
extern const nsXREAppData* gAppData;








void
XRE_ProcLoaderPreload(const char* aProgramDir, const nsXREAppData* aAppData)
{
    void PreloadXPT(nsIFile *);

    nsresult rv;
    nsCOMPtr<nsIFile> omnijarFile;
    rv = NS_NewNativeLocalFile(nsCString(aProgramDir),
			       true,
			       getter_AddRefs(omnijarFile));
    MOZ_RELEASE_ASSERT(NS_SUCCEEDED(rv));

    rv = omnijarFile->AppendNative(NS_LITERAL_CSTRING(NS_STRINGIFY(OMNIJAR_NAME)));
    MOZ_RELEASE_ASSERT(NS_SUCCEEDED(rv));

    



    gAppData = aAppData;

    PreloadXPT(omnijarFile);

    gAppData = nullptr;
}
#endif 
