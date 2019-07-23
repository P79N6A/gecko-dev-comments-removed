



































#include "GeckoChildProcessHost.h"

#include "base/command_line.h"
#include "base/path_service.h"
#include "chrome/common/chrome_switches.h"

using mozilla::ipc::GeckoChildProcessHost;

GeckoChildProcessHost::GeckoChildProcessHost(ProcessType type)
: ChildProcessHost(type)
{
}

bool
GeckoChildProcessHost::Init()
{
  if (!CreateChannel()) {
    return false;
  }

  FilePath exePath =
    FilePath::FromWStringHack(CommandLine::ForCurrentProcess()->program());
  exePath = exePath.DirName();

#if defined(OS_WIN)
  exePath = exePath.AppendASCII("mozilla-runtime.exe");
  
#elif defined(OS_POSIX)
  exePath = exePath.AppendASCII("mozilla-runtime");

  int srcChannelFd, dstChannelFd;
  channel().GetClientFileDescriptorMapping(&srcChannelFd, &dstChannelFd);
  mFileMap.push_back(std::pair<int,int>(srcChannelFd, dstChannelFd));
#else
#error Bad!
#endif

  CommandLine cmdLine(exePath.ToWStringHack());
  cmdLine.AppendSwitchWithValue(switches::kProcessChannelID, channel_id());

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
GeckoChildProcessHost::OnMessageReceived(const IPC::Message& aMsg)
{
}

void
GeckoChildProcessHost::OnChannelError()
{
  
}
