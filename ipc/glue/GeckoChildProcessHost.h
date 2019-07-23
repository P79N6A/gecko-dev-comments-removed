



































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
  GeckoChildProcessHost(GeckoProcessType aProcessType=GeckoProcessType_Default);

  bool SyncLaunch(std::vector<std::wstring> aExtraOpts=std::vector<std::wstring>());
  bool AsyncLaunch(std::vector<std::wstring> aExtraOpts=std::vector<std::wstring>());

  virtual void OnChannelConnected(int32 peer_pid);
  virtual void OnMessageReceived(const IPC::Message& aMsg);
  virtual void OnChannelError();

  virtual bool CanShutdown() { return true; }

  IPC::Channel* GetChannel() {
    return channelp();
  }

  base::WaitableEvent* GetShutDownEvent() {
    return GetProcessEvent();
  }

protected:
  GeckoProcessType mProcessType;
  Monitor mMonitor;
  bool mLaunched;
  FilePath mProcessPath;

#if defined(OS_POSIX)
  base::file_handle_mapping_vector mFileMap;
#endif

private:
  DISALLOW_EVIL_CONSTRUCTORS(GeckoChildProcessHost);
};

} 
} 

#endif 
