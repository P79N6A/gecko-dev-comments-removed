



































#ifndef __IPC_GLUE_GECKOCHILDPROCESSHOST_H__
#define __IPC_GLUE_GECKOCHILDPROCESSHOST_H__

#include "base/file_path.h"
#include "base/process_util.h"
#include "base/scoped_ptr.h"
#include "base/waitable_event.h"
#include "chrome/common/child_process_host.h"

#include "mozilla/ReentrantMonitor.h"

#include "nsXULAppAPI.h"        
#include "nsString.h"

namespace mozilla {
namespace ipc {

class GeckoChildProcessHost : public ChildProcessHost
{
protected:
  typedef mozilla::ReentrantMonitor ReentrantMonitor;

public:
  typedef base::ProcessHandle ProcessHandle;

  GeckoChildProcessHost(GeckoProcessType aProcessType=GeckoProcessType_Default,
                        base::WaitableEventWatcher::Delegate* aDelegate=nsnull);

  ~GeckoChildProcessHost();

  static nsresult GetArchitecturesForBinary(const char *path, uint32 *result);

  static uint32 GetSupportedArchitecturesForProcessType(GeckoProcessType type);

  bool SyncLaunch(std::vector<std::string> aExtraOpts=std::vector<std::string>(),
                  int32 timeoutMs=0,
                  base::ProcessArchitecture arch=base::GetCurrentProcessArchitecture());
  bool AsyncLaunch(std::vector<std::string> aExtraOpts=std::vector<std::string>());
  bool PerformAsyncLaunch(std::vector<std::string> aExtraOpts=std::vector<std::string>(),
                          base::ProcessArchitecture arch=base::GetCurrentProcessArchitecture());

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

#ifdef XP_MACOSX
  task_t GetChildTask() {
    return mChildTask;
  }
#endif


protected:
  GeckoProcessType mProcessType;
  ReentrantMonitor mReentrantMonitor;
  bool mLaunched;
  bool mChannelInitialized;
  FilePath mProcessPath;

  static PRInt32 mChildCounter;

#ifdef XP_WIN
  void InitWindowsGroupID();
  nsString mGroupId;
#endif

#if defined(OS_POSIX)
  base::file_handle_mapping_vector mFileMap;
#endif

  base::WaitableEventWatcher::Delegate* mDelegate;

  ProcessHandle mChildProcessHandle;
#if defined(OS_MACOSX)
  task_t mChildTask;
#endif

private:
  DISALLOW_EVIL_CONSTRUCTORS(GeckoChildProcessHost);

  
  bool PerformAsyncLaunchInternal(std::vector<std::string>& aExtraOpts,
                                  base::ProcessArchitecture arch);
};

} 
} 

#endif 
