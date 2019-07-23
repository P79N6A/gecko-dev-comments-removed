




































#include "GeckoChildProcessHost.h"

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "chrome/common/chrome_switches.h"

#include "prprf.h"

#include "mozilla/ipc/GeckoThread.h"

using mozilla::MonitorAutoEnter;
using mozilla::ipc::GeckoChildProcessHost;

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
{
    MOZ_COUNT_CTOR(GeckoChildProcessHost);
    
    MessageLoop* ioLoop = 
      BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO);
    ioLoop->PostTask(FROM_HERE,
                     NewRunnableMethod(this,
                                       &GeckoChildProcessHost::InitializeChannel));
}

GeckoChildProcessHost::~GeckoChildProcessHost()
{
    MOZ_COUNT_DTOR(GeckoChildProcessHost);
}

bool
GeckoChildProcessHost::SyncLaunch(std::vector<std::string> aExtraOpts)
{
  MessageLoop* ioLoop = 
    BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO);
  NS_ASSERTION(MessageLoop::current() != ioLoop, "sync launch from the IO thread NYI");

  ioLoop->PostTask(FROM_HERE,
                   NewRunnableMethod(this,
                                     &GeckoChildProcessHost::PerformAsyncLaunch,
                                     aExtraOpts));

  
  
  MonitorAutoEnter mon(mMonitor);
  while (!mLaunched) {
    mon.Wait();
  }

  return true;
}

bool
GeckoChildProcessHost::AsyncLaunch(std::vector<std::string> aExtraOpts)
{
  MessageLoop* ioLoop = 
    BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO);
  ioLoop->PostTask(FROM_HERE,
                   NewRunnableMethod(this,
                                     &GeckoChildProcessHost::PerformAsyncLaunch,
                                     aExtraOpts));

  
  
  MonitorAutoEnter mon(mMonitor);
  while (!mChannelInitialized) {
    mon.Wait();
  }

  return true;
}

void
GeckoChildProcessHost::InitializeChannel()
{
  CreateChannel();

  MonitorAutoEnter mon(mMonitor);
  mChannelInitialized = true;
  mon.Notify();
}

bool
GeckoChildProcessHost::PerformAsyncLaunch(std::vector<std::string> aExtraOpts)
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
  
  
  
  
  

  FilePath exePath = FilePath(CommandLine::ForCurrentProcess()->argv()[0]);
  exePath = exePath.DirName();
  exePath = exePath.AppendASCII(MOZ_CHILD_PROCESS_NAME);

  
  
  int srcChannelFd, dstChannelFd;
  channel().GetClientFileDescriptorMapping(&srcChannelFd, &dstChannelFd);
  mFileMap.push_back(std::pair<int,int>(srcChannelFd, dstChannelFd));
  
  
  

  std::vector<std::string> childArgv;

  childArgv.push_back(exePath.value());

  childArgv.insert(childArgv.end(), aExtraOpts.begin(), aExtraOpts.end());

  childArgv.push_back(pidstring);
  childArgv.push_back(childProcessType);

  base::LaunchApp(childArgv, mFileMap, false, &process);


#elif defined(OS_WIN)

  FilePath exePath =
    FilePath::FromWStringHack(CommandLine::ForCurrentProcess()->program());
  exePath = exePath.DirName();

  exePath = exePath.AppendASCII(MOZ_CHILD_PROCESS_NAME);

  CommandLine cmdLine(exePath.ToWStringHack());
  cmdLine.AppendSwitchWithValue(switches::kProcessChannelID, channel_id());

  for (std::vector<std::string>::iterator it = aExtraOpts.begin();
       it != aExtraOpts.end();
       ++it) {
      cmdLine.AppendLooseValue(UTF8ToWide(*it));
  }

  cmdLine.AppendLooseValue(UTF8ToWide(pidstring));
  cmdLine.AppendLooseValue(UTF8ToWide(childProcessType));

  base::LaunchApp(cmdLine, false, false, &process);

#else
#  error Sorry
#endif

  if (!process) {
    return false;
  }
  SetHandle(process);

  return true;
}

void
GeckoChildProcessHost::OnChannelConnected(int32 peer_pid)
{
  MonitorAutoEnter mon(mMonitor);
  mLaunched = true;

  if (!base::OpenProcessHandle(peer_pid, &mChildProcessHandle))
      NS_RUNTIMEABORT("can't open handle to child process");

  mon.Notify();
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
