



































#include "GeckoChildProcessHost.h"

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "chrome/common/chrome_switches.h"

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
    mDelegate(aDelegate)
{
}

bool
GeckoChildProcessHost::SyncLaunch(std::vector<std::wstring> aExtraOpts)
{
  MessageLoop* loop = MessageLoop::current();
  MessageLoop* ioLoop = 
    BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO);
  NS_ASSERTION(loop != ioLoop, "sync launch from the IO thread NYI");

  ioLoop->PostTask(FROM_HERE,
                   NewRunnableMethod(this,
                                     &GeckoChildProcessHost::AsyncLaunch,
                                     aExtraOpts));

  
  
  MonitorAutoEnter mon(mMonitor);
  while (!mLaunched) {
    mon.Wait();
  }

  return true;
}

bool
GeckoChildProcessHost::AsyncLaunch(std::vector<std::wstring> aExtraOpts)
{
  

  if (!CreateChannel()) {
    return false;
  }

  FilePath exePath =
    FilePath::FromWStringHack(CommandLine::ForCurrentProcess()->program());
  exePath = exePath.DirName();

  exePath = exePath.AppendASCII(MOZ_CHILD_PROCESS_NAME);

  
  
#if defined(OS_POSIX)
  int srcChannelFd, dstChannelFd;
  channel().GetClientFileDescriptorMapping(&srcChannelFd, &dstChannelFd);
  mFileMap.push_back(std::pair<int,int>(srcChannelFd, dstChannelFd));
#endif

  CommandLine cmdLine(exePath.ToWStringHack());
  cmdLine.AppendSwitchWithValue(switches::kProcessChannelID, channel_id());

  for (std::vector<std::wstring>::iterator it = aExtraOpts.begin();
       it != aExtraOpts.end();
       ++it) {
    cmdLine.AppendLooseValue((*it).c_str());
  }

  cmdLine.AppendLooseValue(UTF8ToWide(XRE_ChildProcessTypeToString(mProcessType)));

  base::ProcessHandle process;
#if defined(OS_WIN)
  base::LaunchApp(cmdLine, false, false, &process);
#elif defined(OS_POSIX)
  base::LaunchApp(cmdLine.argv(), mFileMap, false, &process);
#else
#error Bad!
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
