



































#ifndef __IPC_GLUE_GECKOCHILDPROCESSHOST_H__
#define __IPC_GLUE_GECKOCHILDPROCESSHOST_H__

#include "base/file_path.h"
#include "base/process_util.h"
#include "base/scoped_ptr.h"
#include "base/waitable_event.h"
#include "chrome/common/child_process_host.h"

#include "mozilla/Monitor.h"

#include "nsXULAppAPI.h"        

namespace mozilla {
namespace ipc {

class GeckoChildProcessHost : public ChildProcessHost
{
protected:
  typedef mozilla::Monitor Monitor;

public:
  typedef base::ProcessHandle ProcessHandle;

  GeckoChildProcessHost(GeckoProcessType aProcessType=GeckoProcessType_Default,
                        base::WaitableEventWatcher::Delegate* aDelegate=nsnull);

  ~GeckoChildProcessHost();

  bool SyncLaunch(std::vector<std::string> aExtraOpts=std::vector<std::string>());
  bool AsyncLaunch(std::vector<std::string> aExtraOpts=std::vector<std::string>());
  bool PerformAsyncLaunch(std::vector<std::string> aExtraOpts=std::vector<std::string>());

  virtual void OnChannelConnected(int32 peer_pid);
  virtual void OnMessageReceived(const IPC::Message& aMsg);
  virtual void OnChannelError();

  void InitializeChannel();

  virtual bool CanShutdown() { return true; }

  virtual void OnWaitableEventSignaled(base::WaitableEvent *event);

  IPC::Channel* GetChannel() {
    return channelp();
  }

  base::WaitableEvent* GetShutDownEvent() {
    return GetProcessEvent();
  }

  ProcessHandle GetChildProcessHandle() {
    return mChildProcessHandle;
  }

protected:
  GeckoProcessType mProcessType;
  Monitor mMonitor;
  bool mLaunched;
  bool mChannelInitialized;
  FilePath mProcessPath;

#if defined(OS_POSIX)
  base::file_handle_mapping_vector mFileMap;
#endif

  base::WaitableEventWatcher::Delegate* mDelegate;

  ProcessHandle mChildProcessHandle;

private:
  DISALLOW_EVIL_CONSTRUCTORS(GeckoChildProcessHost);
};

} 
} 

#endif 
