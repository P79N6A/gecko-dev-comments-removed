





































#include "GeckoChildProcessHost.h"

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/process_watcher.h"
#ifdef XP_MACOSX
#include "chrome/common/mach_ipc_mac.h"
#include "base/rand_util.h"
#include "nsILocalFileMac.h"
#endif

#include "prprf.h"
#include "prenv.h"

#if defined(OS_LINUX)
#  define XP_LINUX 1
#endif
#include "nsExceptionHandler.h"

#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsILocalFile.h"

#include "mozilla/ipc/BrowserProcessSubThread.h"
#include "mozilla/Omnijar.h"
#include <sys/stat.h>

#ifdef XP_WIN
#include "nsIWinTaskbar.h"
#define NS_TASKBAR_CONTRACTID "@mozilla.org/windows-taskbar;1"
#endif

#ifdef ANDROID
#include "APKOpen.h"
#endif

using mozilla::MonitorAutoLock;
using mozilla::ipc::GeckoChildProcessHost;

#ifdef ANDROID



static const int kMagicAndroidSystemPropFd = 5;
#endif

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

GeckoChildProcessHost::GeckoChildProcessHost(GeckoProcessType aProcessType,
                                             base::WaitableEventWatcher::Delegate* aDelegate)
  : ChildProcessHost(RENDER_PROCESS), 
    mProcessType(aProcessType),
    mMonitor("mozilla.ipc.GeckChildProcessHost.mMonitor"),
    mLaunched(false),
    mChannelInitialized(false),
    mDelegate(aDelegate),
    mChildProcessHandle(0)
#if defined(XP_MACOSX)
  , mChildTask(MACH_PORT_NULL)
#endif
{
    MOZ_COUNT_CTOR(GeckoChildProcessHost);
    
    MessageLoop* ioLoop = XRE_GetIOMessageLoop();
    ioLoop->PostTask(FROM_HERE,
                     NewRunnableMethod(this,
                                       &GeckoChildProcessHost::InitializeChannel));
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

#if defined(XP_MACOSX)
  if (mChildTask != MACH_PORT_NULL)
    mach_port_deallocate(mach_task_self(), mChildTask);
#endif
}

void GetPathToBinary(FilePath& exePath)
{
#if defined(OS_WIN)
  exePath = FilePath::FromWStringHack(CommandLine::ForCurrentProcess()->program());
  exePath = exePath.DirName();
  exePath = exePath.AppendASCII(MOZ_CHILD_PROCESS_NAME);
#elif defined(OS_POSIX)
  if (ShouldHaveDirectoryService()) {
    nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID));
    NS_ASSERTION(directoryService, "Expected XPCOM to be available");
    if (directoryService) {
      nsCOMPtr<nsIFile> greDir;
      nsresult rv = directoryService->Get(NS_GRE_DIR, NS_GET_IID(nsIFile), getter_AddRefs(greDir));
      if (NS_SUCCEEDED(rv)) {
        nsCString path;
        greDir->GetNativePath(path);
        exePath = FilePath(path.get());
      }
    }
  }
  if (exePath.empty()) {
    exePath = FilePath(CommandLine::ForCurrentProcess()->argv()[0]);
    exePath = exePath.DirName();
  }

#ifdef OS_MACOSX
  
  
  exePath = exePath.AppendASCII(MOZ_CHILD_PROCESS_BUNDLE);
#endif

  exePath = exePath.AppendASCII(MOZ_CHILD_PROCESS_NAME);
#endif
}

#ifdef XP_MACOSX
class AutoCFTypeObject {
public:
  AutoCFTypeObject(CFTypeRef object)
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

nsresult GeckoChildProcessHost::GetArchitecturesForBinary(const char *path, uint32 *result)
{
  *result = 0;

#ifdef XP_MACOSX
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

uint32 GeckoChildProcessHost::GetSupportedArchitecturesForProcessType(GeckoProcessType type)
{
#ifdef XP_MACOSX
  if (type == GeckoProcessType_Plugin) {
    
    static uint32 pluginContainerArchs = 0;
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

#ifdef XP_WIN
void GeckoChildProcessHost::InitWindowsGroupID()
{
  
  
  
  nsCOMPtr<nsIWinTaskbar> taskbarInfo =
    do_GetService(NS_TASKBAR_CONTRACTID);
  if (taskbarInfo) {
    PRBool isSupported = PR_FALSE;
    taskbarInfo->GetAvailable(&isSupported);
    nsAutoString appId;
    if (isSupported && NS_SUCCEEDED(taskbarInfo->GetDefaultGroupId(appId))) {
      mGroupId.Append(appId);
    } else {
      mGroupId.AssignLiteral("-");
    }
  }
}
#endif

bool
GeckoChildProcessHost::SyncLaunch(std::vector<std::string> aExtraOpts, int aTimeoutMs, base::ProcessArchitecture arch)
{
#ifdef XP_WIN
  InitWindowsGroupID();
#endif

  PRIntervalTime timeoutTicks = (aTimeoutMs > 0) ? 
    PR_MillisecondsToInterval(aTimeoutMs) : PR_INTERVAL_NO_TIMEOUT;
  MessageLoop* ioLoop = XRE_GetIOMessageLoop();
  NS_ASSERTION(MessageLoop::current() != ioLoop, "sync launch from the IO thread NYI");

  ioLoop->PostTask(FROM_HERE,
                   NewRunnableMethod(this,
                                     &GeckoChildProcessHost::PerformAsyncLaunch,
                                     aExtraOpts, arch));
  
  
  MonitorAutoLock lock(mMonitor);
  PRIntervalTime waitStart = PR_IntervalNow();
  PRIntervalTime current;

  
  
  while (!mLaunched) {
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

  return mLaunched;
}

bool
GeckoChildProcessHost::AsyncLaunch(std::vector<std::string> aExtraOpts)
{
#ifdef XP_WIN
  InitWindowsGroupID();
#endif

  MessageLoop* ioLoop = XRE_GetIOMessageLoop();
  ioLoop->PostTask(FROM_HERE,
                   NewRunnableMethod(this,
                                     &GeckoChildProcessHost::PerformAsyncLaunch,
                                     aExtraOpts, base::GetCurrentProcessArchitecture()));

  
  
  MonitorAutoLock lock(mMonitor);
  while (!mChannelInitialized) {
    lock.Wait();
  }

  return true;
}

void
GeckoChildProcessHost::InitializeChannel()
{
  CreateChannel();

  MonitorAutoLock lock(mMonitor);
  mChannelInitialized = true;
  lock.Notify();
}

PRInt32 GeckoChildProcessHost::mChildCounter = 0;




bool
GeckoChildProcessHost::PerformAsyncLaunch(std::vector<std::string> aExtraOpts, base::ProcessArchitecture arch)
{
  
  const char* origLogName = PR_GetEnv("NSPR_LOG_FILE");
  const char* separateLogs = PR_GetEnv("GECKO_SEPARATE_NSPR_LOGS");
  if (!origLogName || !separateLogs || !*separateLogs ||
      *separateLogs == '0' || *separateLogs == 'N' || *separateLogs == 'n') {
    return PerformAsyncLaunchInternal(aExtraOpts, arch);
  }

  
  
  
  
  nsCAutoString setChildLogName("NSPR_LOG_FILE=");
  setChildLogName.Append(origLogName);

  
  
  
  
  static char* restoreOrigLogName = 0;
  if (!restoreOrigLogName)
    restoreOrigLogName = strdup(PromiseFlatCString(setChildLogName).get());

  
  setChildLogName.AppendLiteral(".child-");
  setChildLogName.AppendInt(++mChildCounter);

  
  
  PR_SetEnv(PromiseFlatCString(setChildLogName).get());
  bool retval = PerformAsyncLaunchInternal(aExtraOpts, arch);

  
  PR_SetEnv(restoreOrigLogName);

  return retval;
}

bool
GeckoChildProcessHost::PerformAsyncLaunchInternal(std::vector<std::string>& aExtraOpts, base::ProcessArchitecture arch)
{
  

  
  
  if (!GetChannel()) {
    return false;
  }

  base::ProcessHandle process;

  
  
  char pidstring[32];
  PR_snprintf(pidstring, sizeof(pidstring) - 1,
	      "%ld", base::Process::Current().pid());

  const char* const childProcessType =
      XRE_ChildProcessTypeToString(mProcessType);


#if defined(OS_POSIX)
  
  
  
  
  

#if defined(OS_LINUX) || defined(OS_MACOSX)
  base::environment_map newEnvVars;
  
  
  
  
  if (ShouldHaveDirectoryService()) {
    nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID));
    NS_ASSERTION(directoryService, "Expected XPCOM to be available");
    if (directoryService) {
      nsCOMPtr<nsIFile> greDir;
      nsresult rv = directoryService->Get(NS_GRE_DIR, NS_GET_IID(nsIFile), getter_AddRefs(greDir));
      if (NS_SUCCEEDED(rv)) {
        nsCString path;
        greDir->GetNativePath(path);
# ifdef OS_LINUX
#  ifdef ANDROID
        path += "/lib";
#  endif  
        newEnvVars["LD_LIBRARY_PATH"] = path.get();
# elif OS_MACOSX
        newEnvVars["DYLD_LIBRARY_PATH"] = path.get();
        
        
        
        
        
        
        
        
        
        
        const char* prevInterpose = PR_GetEnv("DYLD_INSERT_LIBRARIES");
        nsCString interpose;
        if (prevInterpose) {
          interpose.Assign(prevInterpose);
          interpose.AppendLiteral(":");
        }
        interpose.Append(path.get());
        interpose.AppendLiteral("/libplugin_child_interpose.dylib");
        newEnvVars["DYLD_INSERT_LIBRARIES"] = interpose.get();
# endif  
      }
    }
  }
#endif  

  FilePath exePath;
  GetPathToBinary(exePath);

#ifdef ANDROID
  
  chmod(exePath.value().c_str(), 0700);
  int cacheCount = 0;
  const struct lib_cache_info * cache = getLibraryCache();
  nsCString cacheStr;
  while (cache &&
         cacheCount++ < MAX_LIB_CACHE_ENTRIES &&
         strlen(cache->name)) {
    mFileMap.push_back(std::pair<int,int>(cache->fd, cache->fd));
    cacheStr.Append(cache->name);
    cacheStr.AppendPrintf(":%d;", cache->fd);
    cache++;
  }
  
  if (cacheStr.IsEmpty())
    cacheStr.AppendLiteral("-");

  
  
  
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

  
  
  int srcChannelFd, dstChannelFd;
  channel().GetClientFileDescriptorMapping(&srcChannelFd, &dstChannelFd);
  mFileMap.push_back(std::pair<int,int>(srcChannelFd, dstChannelFd));

  
  

  std::vector<std::string> childArgv;

  childArgv.push_back(exePath.value());

  childArgv.insert(childArgv.end(), aExtraOpts.begin(), aExtraOpts.end());

  
  
  nsCAutoString path;
  nsCOMPtr<nsIFile> file = mozilla::Omnijar::GetPath(mozilla::Omnijar::GRE);
  if (file && NS_SUCCEEDED(file->GetNativePath(path))) {
    childArgv.push_back("-greomni");
    childArgv.push_back(path.get());
  }
  file = mozilla::Omnijar::GetPath(mozilla::Omnijar::APP);
  if (file && NS_SUCCEEDED(file->GetNativePath(path))) {
    childArgv.push_back("-appomni");
    childArgv.push_back(path.get());
  }

  childArgv.push_back(pidstring);

#if defined(MOZ_CRASHREPORTER)
#  if defined(OS_LINUX)
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
#  elif defined(XP_MACOSX)
  childArgv.push_back(CrashReporter::GetChildNotificationPipe());
#  endif  
#endif

#ifdef XP_MACOSX
  
  
  
  
  
  std::string mach_connection_name = StringPrintf("org.mozilla.machname.%d",
                                                  base::RandInt(0, std::numeric_limits<int>::max()));
  childArgv.push_back(mach_connection_name.c_str());
#endif

  childArgv.push_back(childProcessType);

#ifdef ANDROID
  childArgv.push_back(cacheStr.get());
#endif

  base::LaunchApp(childArgv, mFileMap,
#if defined(OS_LINUX) || defined(OS_MACOSX)
                  newEnvVars,
#endif
                  false, &process, arch);

#ifdef XP_MACOSX
  
  const int kTimeoutMs = 10000;

  MachReceiveMessage child_message;
  ReceivePort parent_recv_port(mach_connection_name.c_str());
  kern_return_t err = parent_recv_port.WaitForMessage(&child_message, kTimeoutMs);
  if (err != KERN_SUCCESS) {
    std::string errString = StringPrintf("0x%x %s", err, mach_error_string(err));
    LOG(ERROR) << "parent WaitForMessage() failed: " << errString;
    return false;
  }

  task_t child_task = child_message.GetTranslatedPort(0);
  if (child_task == MACH_PORT_NULL) {
    LOG(ERROR) << "parent GetTranslatedPort(0) failed.";
    return false;
  }

  if (child_message.GetTranslatedPort(1) == MACH_PORT_NULL) {
    LOG(ERROR) << "parent GetTranslatedPort(1) failed.";
    return false;
  }
  MachPortSender parent_sender(child_message.GetTranslatedPort(1));

  MachSendMessage parent_message(0);
  if (!parent_message.AddDescriptor(bootstrap_port)) {
    LOG(ERROR) << "parent AddDescriptor(" << bootstrap_port << ") failed.";
    return false;
  }

  err = parent_sender.SendMessage(parent_message, kTimeoutMs);
  if (err != KERN_SUCCESS) {
    std::string errString = StringPrintf("0x%x %s", err, mach_error_string(err));
    LOG(ERROR) << "parent SendMessage() failed: " << errString;
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

  cmdLine.AppendLooseValue(std::wstring(mGroupId.get()));

  
  
  nsAutoString path;
  nsCOMPtr<nsIFile> file = mozilla::Omnijar::GetPath(mozilla::Omnijar::GRE);
  if (file && NS_SUCCEEDED(file->GetPath(path))) {
    cmdLine.AppendLooseValue(UTF8ToWide("-greomni"));
    cmdLine.AppendLooseValue(path.get());
  }
  file = mozilla::Omnijar::GetPath(mozilla::Omnijar::APP);
  if (file && NS_SUCCEEDED(file->GetPath(path))) {
    cmdLine.AppendLooseValue(UTF8ToWide("-appomni"));
    cmdLine.AppendLooseValue(path.get());
  }

  cmdLine.AppendLooseValue(UTF8ToWide(pidstring));

#if defined(MOZ_CRASHREPORTER)
  cmdLine.AppendLooseValue(
    UTF8ToWide(CrashReporter::GetChildNotificationPipe()));
#endif

  cmdLine.AppendLooseValue(UTF8ToWide(childProcessType));

  base::LaunchApp(cmdLine, false, false, &process);

#else
#  error Sorry
#endif

  if (!process) {
    return false;
  }
  SetHandle(process);
#if defined(XP_MACOSX)
  mChildTask = child_task;
#endif

  return true;
}

void
GeckoChildProcessHost::OnChannelConnected(int32 peer_pid)
{
  MonitorAutoLock lock(mMonitor);
  mLaunched = true;

  if (!base::OpenPrivilegedProcessHandle(peer_pid, &mChildProcessHandle))
      NS_RUNTIMEABORT("can't open handle to child process");

  lock.Notify();
}




void
GeckoChildProcessHost::OnMessageReceived(const IPC::Message& aMsg)
{
}
void
GeckoChildProcessHost::OnChannelError()
{
  
}

void
GeckoChildProcessHost::OnWaitableEventSignaled(base::WaitableEvent *event)
{
  if (mDelegate) {
    mDelegate->OnWaitableEventSignaled(event);
  }
  ChildProcessHost::OnWaitableEventSignaled(event);
}
