



































#include "GeckoChildProcessHost.h"

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "chrome/common/chrome_switches.h"

using mozilla::ipc::GeckoChildProcessHost;

GeckoChildProcessHost::GeckoChildProcessHost(GeckoChildProcessType aProcessType)
  : ChildProcessHost(RENDER_PROCESS), 
    mProcessType(aProcessType)
{
}

bool
GeckoChildProcessHost::Launch(std::vector<std::wstring> aExtraOpts)
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

  
  
  
  
  MessageLoop* loop = MessageLoop::current();
  bool old_state = loop->NestableTasksAllowed();
  loop->SetNestableTasksAllowed(true);
  
  loop->Run();
  loop->SetNestableTasksAllowed(old_state);

  return true;
}

void
GeckoChildProcessHost::OnChannelConnected(int32 peer_pid)
{
    MessageLoop::current()->Quit();
}
void
GeckoChildProcessHost::OnMessageReceived(const IPC::Message& aMsg)
{
}

void
GeckoChildProcessHost::OnChannelError()
{
  
}
