





#include "GeckoChildProcessHost.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/process_watcher.h"
#ifdef MOZ_WIDGET_COCOA
#include "chrome/common/mach_ipc_mac.h"
#include "base/rand_util.h"
#include "nsILocalFileMac.h"
#endif

#include "MainThreadUtils.h"
#include "prprf.h"
#include "prenv.h"
#include "nsXPCOMPrivate.h"

#include "nsExceptionHandler.h"

#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/ipc/BrowserProcessSubThread.h"
#include "mozilla/Omnijar.h"
#include <sys/stat.h>

#ifdef XP_WIN
#include "nsIWinTaskbar.h"
#define NS_TASKBAR_CONTRACTID "@mozilla.org/windows-taskbar;1"

#if defined(MOZ_CONTENT_SANDBOX)
#include "mozilla/Preferences.h"
#include "mozilla/warnonlysandbox/warnOnlySandbox.h"
#endif
#endif

#include "nsTArray.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsNativeCharsetUtils.h"

using mozilla::MonitorAutoLock;
using mozilla::ipc::GeckoChildProcessHost;

#ifdef ANDROID



static const int kMagicAndroidSystemPropFd = 5;
#endif

static const bool kLowRightsSubprocesses =
  
  
#ifdef MOZ_WIDGET_GONK
  true
#else
  false
#endif
  ;

static bool
ShouldHaveDirectoryService()
{
  return GeckoProcessType_Default == XRE_GetProcessType();
}

template<>
struct RunnableMethodTraits<GeckoChildProcessHost>
{
    static void RetainCallee(GeckoChildProcessHost* obj) { }
    static void ReleaseCallee(GeckoChildProcessHost* obj) { }
};


base::ChildPrivileges
GeckoChildProcessHost::DefaultChildPrivileges()
{
  return (kLowRightsSubprocesses ?
          base::PRIVILEGES_UNPRIVILEGED : base::PRIVILEGES_INHERIT);
}

GeckoChildProcessHost::GeckoChildProcessHost(GeckoProcessType aProcessType,
                                             ChildPrivileges aPrivileges)
  : ChildProcessHost(RENDER_PROCESS), 
    mProcessType(aProcessType),
    mPrivileges(aPrivileges),
    mMonitor("mozilla.ipc.GeckChildProcessHost.mMonitor"),
    mProcessState(CREATING_CHANNEL),
    mDelegate(nullptr),
#if defined(MOZ_CONTENT_SANDBOX) && defined(XP_WIN)
    mEnableContentSandbox(false),
    mWarnOnlyContentSandbox(false),
#endif
    mChildProcessHandle(0)
#if defined(MOZ_WIDGET_COCOA)
  , mChildTask(MACH_PORT_NULL)
#endif
{
    MOZ_COUNT_CTOR(GeckoChildProcessHost);
}

GeckoChildProcessHost::~GeckoChildProcessHost()

{
  AssertIOThread();

  MOZ_COUNT_DTOR(GeckoChildProcessHost);

  if (mChildProcessHandle > 0)
    ProcessWatcher::EnsureProcessTerminated(mChildProcessHandle
#if defined(NS_BUILD_REFCNT_LOGGING)
                                            , false 
#endif
    );

#if defined(MOZ_WIDGET_COCOA)
  if (mChildTask != MACH_PORT_NULL)
    mach_port_deallocate(mach_task_self(), mChildTask);
#endif
}


void
GeckoChildProcessHost::GetPathToBinary(FilePath& exePath)
{
  if (ShouldHaveDirectoryService()) {
    MOZ_ASSERT(gGREPath);
#ifdef OS_WIN
    exePath = FilePath(char16ptr_t(gGREPath));
#else
    nsCString path;
    NS_CopyUnicodeToNative(nsDependentString(gGREPath), path);
    exePath = FilePath(path.get());
#endif
#ifdef MOZ_WIDGET_COCOA
    
    
    exePath = exePath.AppendASCII(MOZ_CHILD_PROCESS_BUNDLE);
#endif
  }

  if (exePath.empty()) {
#ifdef OS_WIN
    exePath = FilePath::FromWStringHack(CommandLine::ForCurrentProcess()->program());
#else
    exePath = FilePath(CommandLine::ForCurrentProcess()->argv()[0]);
#endif
    exePath = exePath.DirName();
  }

  exePath = exePath.AppendASCII(MOZ_CHILD_PROCESS_NAME);
}

#ifdef MOZ_WIDGET_COCOA
class AutoCFTypeObject {
public:
  explicit AutoCFTypeObject(CFTypeRef object)
  {
    mObject = object;
  }
  ~AutoCFTypeObject()
  {
    ::CFRelease(mObject);
  }
private:
  CFTypeRef mObject;
};
#endif

nsresult GeckoChildProcessHost::GetArchitecturesForBinary(const char *path, uint32_t *result)
{
  *result = 0;

#ifdef MOZ_WIDGET_COCOA
  CFURLRef url = ::CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                           (const UInt8*)path,
                                                           strlen(path),
                                                           false);
  if (!url) {
    return NS_ERROR_FAILURE;
  }
  AutoCFTypeObject autoPluginContainerURL(url);

  CFArrayRef pluginContainerArchs = ::CFBundleCopyExecutableArchitecturesForURL(url);
  if (!pluginContainerArchs) {
    return NS_ERROR_FAILURE;
  }
  AutoCFTypeObject autoPluginContainerArchs(pluginContainerArchs);

  CFIndex pluginArchCount = ::CFArrayGetCount(pluginContainerArchs);
  for (CFIndex i = 0; i < pluginArchCount; i++) {
    CFNumberRef currentArch = static_cast<CFNumberRef>(::CFArrayGetValueAtIndex(pluginContainerArchs, i));
    int currentArchInt = 0;
    if (!::CFNumberGetValue(currentArch, kCFNumberIntType, &currentArchInt)) {
      continue;
    }
    switch (currentArchInt) {
      case kCFBundleExecutableArchitectureI386:
        *result |= base::PROCESS_ARCH_I386;
        break;
      case kCFBundleExecutableArchitectureX86_64:
        *result |= base::PROCESS_ARCH_X86_64;
        break;
      case kCFBundleExecutableArchitecturePPC:
        *result |= base::PROCESS_ARCH_PPC;
        break;
      default:
        break;
    }
  }

  return (*result ? NS_OK : NS_ERROR_FAILURE);
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

uint32_t GeckoChildProcessHost::GetSupportedArchitecturesForProcessType(GeckoProcessType type)
{
#ifdef MOZ_WIDGET_COCOA
  if (type == GeckoProcessType_Plugin) {

    
    static uint32_t pluginContainerArchs = 0;
    if (pluginContainerArchs == 0) {
      FilePath exePath;
      GetPathToBinary(exePath);
      nsresult rv = GetArchitecturesForBinary(exePath.value().c_str(), &pluginContainerArchs);
      NS_ASSERTION(NS_SUCCEEDED(rv) && pluginContainerArchs != 0, "Getting architecture of plugin container failed!");
      if (NS_FAILED(rv) || pluginContainerArchs == 0) {
        pluginContainerArchs = base::GetCurrentProcessArchitecture();
      }
    }
    return pluginContainerArchs;
  }
#endif

  return base::GetCurrentProcessArchitecture();
}

void
GeckoChildProcessHost::PrepareLaunch()
{
#ifdef MOZ_CRASHREPORTER
  if (CrashReporter::GetEnabled()) {
    CrashReporter::OOPInit();
  }
#endif

#ifdef XP_WIN
  if (mProcessType == GeckoProcessType_Plugin) {
    InitWindowsGroupID();
  }

#if defined(MOZ_CONTENT_SANDBOX)
  
  if (mProcessType == GeckoProcessType_Content) {
    nsAdoptingString contentSandboxPref =
      Preferences::GetString("browser.tabs.remote.sandbox");
    if (contentSandboxPref.EqualsLiteral("on")) {
      mEnableContentSandbox = true;
    } else if (contentSandboxPref.EqualsLiteral("warn")) {
      mEnableContentSandbox = true;
      mWarnOnlyContentSandbox = true;
    }
  }
#endif
#endif
}

#ifdef XP_WIN
void GeckoChildProcessHost::InitWindowsGroupID()
{
  
  
  
  nsCOMPtr<nsIWinTaskbar> taskbarInfo =
    do_GetService(NS_TASKBAR_CONTRACTID);
  if (taskbarInfo) {
    bool isSupported = false;
    taskbarInfo->GetAvailable(&isSupported);
    nsAutoString appId;
    if (isSupported && NS_SUCCEEDED(taskbarInfo->GetDefaultGroupId(appId))) {
      mGroupId.Append(appId);
    } else {
      mGroupId.Assign('-');
    }
  }
}
#endif

bool
GeckoChildProcessHost::SyncLaunch(std::vector<std::string> aExtraOpts, int aTimeoutMs, base::ProcessArchitecture arch)
{
  PrepareLaunch();

  PRIntervalTime timeoutTicks = (aTimeoutMs > 0) ? 
    PR_MillisecondsToInterval(aTimeoutMs) : PR_INTERVAL_NO_TIMEOUT;
  MessageLoop* ioLoop = XRE_GetIOMessageLoop();
  NS_ASSERTION(MessageLoop::current() != ioLoop, "sync launch from the IO thread NYI");

  ioLoop->PostTask(FROM_HERE,
                   NewRunnableMethod(this,
                                     &GeckoChildProcessHost::RunPerformAsyncLaunch,
                                     aExtraOpts, arch));
  
  
  MonitorAutoLock lock(mMonitor);
  PRIntervalTime waitStart = PR_IntervalNow();
  PRIntervalTime current;

  
  
  while (mProcessState != PROCESS_CONNECTED) {
    
    if (mProcessState == PROCESS_ERROR) {
      break;
    }

    lock.Wait(timeoutTicks);

    if (timeoutTicks != PR_INTERVAL_NO_TIMEOUT) {
      current = PR_IntervalNow();
      PRIntervalTime elapsed = current - waitStart;
      if (elapsed > timeoutTicks) {
        break;
      }
      timeoutTicks = timeoutTicks - elapsed;
      waitStart = current;
    }
  }

  return mProcessState == PROCESS_CONNECTED;
}

bool
GeckoChildProcessHost::AsyncLaunch(std::vector<std::string> aExtraOpts)
{
  PrepareLaunch();

  MessageLoop* ioLoop = XRE_GetIOMessageLoop();
  ioLoop->PostTask(FROM_HERE,
                   NewRunnableMethod(this,
                                     &GeckoChildProcessHost::RunPerformAsyncLaunch,
                                     aExtraOpts, base::GetCurrentProcessArchitecture()));

  
  
  MonitorAutoLock lock(mMonitor);
  while (mProcessState < CHANNEL_INITIALIZED) {
    lock.Wait();
  }

  return true;
}

bool
GeckoChildProcessHost::LaunchAndWaitForProcessHandle(StringVector aExtraOpts)
{
  PrepareLaunch();

  MessageLoop* ioLoop = XRE_GetIOMessageLoop();
  ioLoop->PostTask(FROM_HERE,
                   NewRunnableMethod(this,
                                     &GeckoChildProcessHost::RunPerformAsyncLaunch,
                                     aExtraOpts, base::GetCurrentProcessArchitecture()));

  MonitorAutoLock lock(mMonitor);
  while (mProcessState < PROCESS_CREATED) {
    lock.Wait();
  }
  MOZ_ASSERT(mProcessState == PROCESS_ERROR || mChildProcessHandle);

  return mProcessState < PROCESS_ERROR;
}

void
GeckoChildProcessHost::InitializeChannel()
{
  CreateChannel();

  MonitorAutoLock lock(mMonitor);
  mProcessState = CHANNEL_INITIALIZED;
  lock.Notify();
}

void
GeckoChildProcessHost::Join()
{
  AssertIOThread();

  if (!mChildProcessHandle) {
    return;
  }

  
  base::KillProcess(mChildProcessHandle, 0, true);
  SetAlreadyDead();
}

void
GeckoChildProcessHost::SetAlreadyDead()
{
  mChildProcessHandle = 0;
}

int32_t GeckoChildProcessHost::mChildCounter = 0;




bool
GeckoChildProcessHost::PerformAsyncLaunch(std::vector<std::string> aExtraOpts, base::ProcessArchitecture arch)
{
  
  const char* origLogName = PR_GetEnv("NSPR_LOG_FILE");
  if (!origLogName) {
    return PerformAsyncLaunchInternal(aExtraOpts, arch);
  }

  
  
  
  
  nsAutoCString setChildLogName("NSPR_LOG_FILE=");
  setChildLogName.Append(origLogName);

  
  
  
  
  static char* restoreOrigLogName = 0;
  if (!restoreOrigLogName)
    restoreOrigLogName = strdup(setChildLogName.get());

  
  setChildLogName.AppendLiteral(".child-");
  setChildLogName.AppendInt(++mChildCounter);

  
  
  PR_SetEnv(setChildLogName.get());
  bool retval = PerformAsyncLaunchInternal(aExtraOpts, arch);

  
  PR_SetEnv(restoreOrigLogName);

  return retval;
}

bool
GeckoChildProcessHost::RunPerformAsyncLaunch(std::vector<std::string> aExtraOpts,
                                             base::ProcessArchitecture aArch)
{
  InitializeChannel();
  return PerformAsyncLaunch(aExtraOpts, aArch);
}

void
#if defined(XP_WIN)
AddAppDirToCommandLine(CommandLine& aCmdLine)
#else
AddAppDirToCommandLine(std::vector<std::string>& aCmdLine)
#endif
{
  
  
  if (ShouldHaveDirectoryService()) {
    nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID));
    NS_ASSERTION(directoryService, "Expected XPCOM to be available");
    if (directoryService) {
      nsCOMPtr<nsIFile> appDir;
      
      
      nsresult rv = directoryService->Get(NS_XPCOM_CURRENT_PROCESS_DIR,
                                          NS_GET_IID(nsIFile),
                                          getter_AddRefs(appDir));
      if (NS_SUCCEEDED(rv)) {
        nsAutoCString path;
        MOZ_ALWAYS_TRUE(NS_SUCCEEDED(appDir->GetNativePath(path)));
#if defined(XP_WIN)
        aCmdLine.AppendLooseValue(UTF8ToWide("-appdir"));
        aCmdLine.AppendLooseValue(UTF8ToWide(path.get()));
#else
        aCmdLine.push_back("-appdir");
        aCmdLine.push_back(path.get());
#endif
      }
    }
  }
}

bool
GeckoChildProcessHost::PerformAsyncLaunchInternal(std::vector<std::string>& aExtraOpts, base::ProcessArchitecture arch)
{
  
  
  if (!GetChannel()) {
    return false;
  }

  base::ProcessHandle process = 0;

  
  
  char pidstring[32];
  PR_snprintf(pidstring, sizeof(pidstring) - 1,
	      "%ld", base::Process::Current().pid());

  const char* const childProcessType =
      XRE_ChildProcessTypeToString(mProcessType);


#if defined(OS_POSIX)
  
  
  
  
  

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_BSD)
  base::environment_map newEnvVars;
  ChildPrivileges privs = mPrivileges;
  if (privs == base::PRIVILEGES_DEFAULT) {
    privs = DefaultChildPrivileges();
  }
  
  
  
  
  if (ShouldHaveDirectoryService()) {
    MOZ_ASSERT(gGREPath);
    nsCString path;
    NS_CopyUnicodeToNative(nsDependentString(gGREPath), path);
# if defined(OS_LINUX) || defined(OS_BSD)
#  if defined(MOZ_WIDGET_ANDROID)
    path += "/lib";
#  endif  
    const char *ld_library_path = PR_GetEnv("LD_LIBRARY_PATH");
    nsCString new_ld_lib_path;
    if (ld_library_path && *ld_library_path) {
      new_ld_lib_path.Assign(path.get());
      new_ld_lib_path.Append(':');
      new_ld_lib_path.Append(ld_library_path);
      newEnvVars["LD_LIBRARY_PATH"] = new_ld_lib_path.get();
    } else {
      newEnvVars["LD_LIBRARY_PATH"] = path.get();
    }

#  if (MOZ_WIDGET_GTK == 3)
    if (mProcessType == GeckoProcessType_Plugin) {
      const char *ld_preload = PR_GetEnv("LD_PRELOAD");
      nsCString new_ld_preload;

      new_ld_preload.Assign(path.get());
      new_ld_preload.AppendLiteral("/" DLL_PREFIX "mozgtk2" DLL_SUFFIX);

      if (ld_preload && *ld_preload) {
        new_ld_preload.AppendLiteral(":");
        new_ld_preload.Append(ld_preload);
      }
      newEnvVars["LD_PRELOAD"] = new_ld_preload.get();
    }
#  endif 


# elif OS_MACOSX
    newEnvVars["DYLD_LIBRARY_PATH"] = path.get();
    
    
    
    
    
    
    
    
    
    
    const char* prevInterpose = PR_GetEnv("DYLD_INSERT_LIBRARIES");
    nsCString interpose;
    if (prevInterpose) {
      interpose.Assign(prevInterpose);
      interpose.Append(':');
    }
    interpose.Append(path.get());
    interpose.AppendLiteral("/libplugin_child_interpose.dylib");
    newEnvVars["DYLD_INSERT_LIBRARIES"] = interpose.get();
# endif  
  }
#endif  

  FilePath exePath;
  GetPathToBinary(exePath);

#ifdef MOZ_WIDGET_ANDROID
  
  chmod(exePath.value().c_str(), 0700);
#endif  

#ifdef ANDROID
  
  
  
  const char *apws = getenv("ANDROID_PROPERTY_WORKSPACE");
  if (apws) {
    int fd = atoi(apws);
    mFileMap.push_back(std::pair<int, int>(fd, kMagicAndroidSystemPropFd));

    char buf[32];
    char *szptr = strchr(apws, ',');

    snprintf(buf, sizeof(buf), "%d%s", kMagicAndroidSystemPropFd, szptr);
    newEnvVars["ANDROID_PROPERTY_WORKSPACE"] = buf;
  }
#endif  

#ifdef MOZ_WIDGET_GONK
  if (const char *ldPreloadPath = getenv("LD_PRELOAD")) {
    newEnvVars["LD_PRELOAD"] = ldPreloadPath;
  }
#endif 

  
  
  int srcChannelFd, dstChannelFd;
  channel().GetClientFileDescriptorMapping(&srcChannelFd, &dstChannelFd);
  mFileMap.push_back(std::pair<int,int>(srcChannelFd, dstChannelFd));

  
  

  std::vector<std::string> childArgv;

  childArgv.push_back(exePath.value());

  childArgv.insert(childArgv.end(), aExtraOpts.begin(), aExtraOpts.end());

  if (Omnijar::IsInitialized()) {
    
    
    nsAutoCString path;
    nsCOMPtr<nsIFile> file = Omnijar::GetPath(Omnijar::GRE);
    if (file && NS_SUCCEEDED(file->GetNativePath(path))) {
      childArgv.push_back("-greomni");
      childArgv.push_back(path.get());
    }
    file = Omnijar::GetPath(Omnijar::APP);
    if (file && NS_SUCCEEDED(file->GetNativePath(path))) {
      childArgv.push_back("-appomni");
      childArgv.push_back(path.get());
    }
  }

  
  AddAppDirToCommandLine(childArgv);

  childArgv.push_back(pidstring);

#if defined(MOZ_CRASHREPORTER)
#  if defined(OS_LINUX) || defined(OS_BSD)
  int childCrashFd, childCrashRemapFd;
  if (!CrashReporter::CreateNotificationPipeForChild(
        &childCrashFd, &childCrashRemapFd))
    return false;
  if (0 <= childCrashFd) {
    mFileMap.push_back(std::pair<int,int>(childCrashFd, childCrashRemapFd));
    
    childArgv.push_back("true");
  }
  else {
    
    childArgv.push_back("false");
  }
#  elif defined(MOZ_WIDGET_COCOA)
  childArgv.push_back(CrashReporter::GetChildNotificationPipe());
#  endif  
#endif

#ifdef MOZ_WIDGET_COCOA
  
  
  
  
  
  std::string mach_connection_name = StringPrintf("org.mozilla.machname.%d",
                                                  base::RandInt(0, std::numeric_limits<int>::max()));
  childArgv.push_back(mach_connection_name.c_str());
#endif

  childArgv.push_back(childProcessType);

  base::LaunchApp(childArgv, mFileMap,
#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_BSD)
                  newEnvVars, privs,
#endif
                  false, &process, arch);

  
  
  
  GetChannel()->CloseClientFileDescriptor();

#ifdef MOZ_WIDGET_COCOA
  
  const int kTimeoutMs = 10000;

  MachReceiveMessage child_message;
  ReceivePort parent_recv_port(mach_connection_name.c_str());
  kern_return_t err = parent_recv_port.WaitForMessage(&child_message, kTimeoutMs);
  if (err != KERN_SUCCESS) {
    std::string errString = StringPrintf("0x%x %s", err, mach_error_string(err));
    CHROMIUM_LOG(ERROR) << "parent WaitForMessage() failed: " << errString;
    return false;
  }

  task_t child_task = child_message.GetTranslatedPort(0);
  if (child_task == MACH_PORT_NULL) {
    CHROMIUM_LOG(ERROR) << "parent GetTranslatedPort(0) failed.";
    return false;
  }

  if (child_message.GetTranslatedPort(1) == MACH_PORT_NULL) {
    CHROMIUM_LOG(ERROR) << "parent GetTranslatedPort(1) failed.";
    return false;
  }
  MachPortSender parent_sender(child_message.GetTranslatedPort(1));

  MachSendMessage parent_message(0);
  if (!parent_message.AddDescriptor(MachMsgPortDescriptor(bootstrap_port))) {
    CHROMIUM_LOG(ERROR) << "parent AddDescriptor(" << bootstrap_port << ") failed.";
    return false;
  }

  err = parent_sender.SendMessage(parent_message, kTimeoutMs);
  if (err != KERN_SUCCESS) {
    std::string errString = StringPrintf("0x%x %s", err, mach_error_string(err));
    CHROMIUM_LOG(ERROR) << "parent SendMessage() failed: " << errString;
    return false;
  }
#endif


#elif defined(OS_WIN)

  FilePath exePath;
  GetPathToBinary(exePath);

  CommandLine cmdLine(exePath.ToWStringHack());
  cmdLine.AppendSwitchWithValue(switches::kProcessChannelID, channel_id());

  for (std::vector<std::string>::iterator it = aExtraOpts.begin();
       it != aExtraOpts.end();
       ++it) {
      cmdLine.AppendLooseValue(UTF8ToWide(*it));
  }

  if (Omnijar::IsInitialized()) {
    
    
    nsAutoString path;
    nsCOMPtr<nsIFile> file = Omnijar::GetPath(Omnijar::GRE);
    if (file && NS_SUCCEEDED(file->GetPath(path))) {
      cmdLine.AppendLooseValue(UTF8ToWide("-greomni"));
      cmdLine.AppendLooseValue(path.get());
    }
    file = Omnijar::GetPath(Omnijar::APP);
    if (file && NS_SUCCEEDED(file->GetPath(path))) {
      cmdLine.AppendLooseValue(UTF8ToWide("-appomni"));
      cmdLine.AppendLooseValue(path.get());
    }
  }

#if defined(XP_WIN)
  bool shouldSandboxCurrentProcess = false;
  switch (mProcessType) {
    case GeckoProcessType_Content:
#if defined(MOZ_CONTENT_SANDBOX)
      if (!mEnableContentSandbox) {
        break;
      }
      if (!PR_GetEnv("MOZ_DISABLE_CONTENT_SANDBOX")) {
        mSandboxBroker.SetSecurityLevelForContentProcess(mWarnOnlyContentSandbox);
        cmdLine.AppendLooseValue(UTF8ToWide("-sandbox"));
        shouldSandboxCurrentProcess = true;
      }
#endif 
      break;
    case GeckoProcessType_Plugin:
      
      
      
      
      break;
    case GeckoProcessType_IPDLUnitTest:
      
      
      
      
      break;
    case GeckoProcessType_GMPlugin:
#ifdef MOZ_SANDBOX
      if (!PR_GetEnv("MOZ_DISABLE_GMP_SANDBOX")) {
        mSandboxBroker.SetSecurityLevelForGMPlugin();
        cmdLine.AppendLooseValue(UTF8ToWide("-sandbox"));
        shouldSandboxCurrentProcess = true;
      }
#endif
      break;
    case GeckoProcessType_Default:
    default:
      MOZ_CRASH("Bad process type in GeckoChildProcessHost");
      break;
  };

#ifdef MOZ_SANDBOX
  if (shouldSandboxCurrentProcess) {
    for (auto it = mAllowedFilesRead.begin();
         it != mAllowedFilesRead.end();
         ++it) {
      mSandboxBroker.AllowReadFile(it->c_str());
    }
  }
#endif

#endif 

  
  AddAppDirToCommandLine(cmdLine);

  
  
  

  
  cmdLine.AppendLooseValue(mGroupId.get());

  
  cmdLine.AppendLooseValue(UTF8ToWide(pidstring));

#if defined(MOZ_CRASHREPORTER)
  cmdLine.AppendLooseValue(
    UTF8ToWide(CrashReporter::GetChildNotificationPipe()));
#endif

  
  cmdLine.AppendLooseValue(UTF8ToWide(childProcessType));

#if defined(XP_WIN) && defined(MOZ_SANDBOX)
  if (shouldSandboxCurrentProcess) {
    mSandboxBroker.LaunchApp(cmdLine.program().c_str(),
                             cmdLine.command_line_string().c_str(),
                             &process);
  } else
#endif
  {
    base::LaunchApp(cmdLine, false, false, &process);
  }

#else
#  error Sorry
#endif

  if (!process) {
    MonitorAutoLock lock(mMonitor);
    mProcessState = PROCESS_ERROR;
    lock.Notify();
    return false;
  }
  
  
  
  SetHandle(process);
#if defined(MOZ_WIDGET_COCOA)
  mChildTask = child_task;
#endif

  OpenPrivilegedHandle(base::GetProcId(process));
  {
    MonitorAutoLock lock(mMonitor);
    mProcessState = PROCESS_CREATED;
    lock.Notify();
  }

  return true;
}

void
GeckoChildProcessHost::OpenPrivilegedHandle(base::ProcessId aPid)
{
  if (mChildProcessHandle) {
    MOZ_ASSERT(aPid == base::GetProcId(mChildProcessHandle));
    return;
  }
  if (!base::OpenPrivilegedProcessHandle(aPid, &mChildProcessHandle)) {
    NS_RUNTIMEABORT("can't open handle to child process");
  }
}

void
GeckoChildProcessHost::OnChannelConnected(int32_t peer_pid)
{
  OpenPrivilegedHandle(peer_pid);
  {
    MonitorAutoLock lock(mMonitor);
    mProcessState = PROCESS_CONNECTED;
    lock.Notify();
  }
}

void
GeckoChildProcessHost::OnMessageReceived(const IPC::Message& aMsg)
{
  
  
  mQueue.push(aMsg);
}

void
GeckoChildProcessHost::OnChannelError()
{
  
  
  
  
  MonitorAutoLock lock(mMonitor);
  if (mProcessState < PROCESS_CONNECTED) {
    mProcessState = PROCESS_ERROR;
    lock.Notify();
  }
  
}

void
GeckoChildProcessHost::GetQueuedMessages(std::queue<IPC::Message>& queue)
{
  
  DCHECK(MessageLoopForIO::current());
  swap(queue, mQueue);
  
}

void
GeckoChildProcessHost::OnWaitableEventSignaled(base::WaitableEvent *event)
{
  if (mDelegate) {
    mDelegate->OnWaitableEventSignaled(event);
  }
  ChildProcessHost::OnWaitableEventSignaled(event);
}

#ifdef MOZ_NUWA_PROCESS

using mozilla::ipc::GeckoExistingProcessHost;
using mozilla::ipc::FileDescriptor;

GeckoExistingProcessHost::
GeckoExistingProcessHost(GeckoProcessType aProcessType,
                         base::ProcessHandle aProcess,
                         const FileDescriptor& aFileDescriptor,
                         ChildPrivileges aPrivileges)
  : GeckoChildProcessHost(aProcessType, aPrivileges)
  , mExistingProcessHandle(aProcess)
  , mExistingFileDescriptor(aFileDescriptor)
{
  NS_ASSERTION(aFileDescriptor.IsValid(),
               "Expected file descriptor to be valid");
}

GeckoExistingProcessHost::~GeckoExistingProcessHost()
{
  
  
  
  SetAlreadyDead();
}

bool
GeckoExistingProcessHost::PerformAsyncLaunch(StringVector aExtraOpts,
                                             base::ProcessArchitecture aArch)
{
  SetHandle(mExistingProcessHandle);

  OpenPrivilegedHandle(base::GetProcId(mExistingProcessHandle));

  MonitorAutoLock lock(mMonitor);
  mProcessState = PROCESS_CREATED;
  lock.Notify();

  return true;
}

void
GeckoExistingProcessHost::InitializeChannel()
{
  CreateChannel(mExistingFileDescriptor);

  MonitorAutoLock lock(mMonitor);
  mProcessState = CHANNEL_INITIALIZED;
  lock.Notify();
}

#endif 
