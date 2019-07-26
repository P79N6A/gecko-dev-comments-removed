



#ifndef __IPC_GLUE_GECKOCHILDPROCESSHOST_H__
#define __IPC_GLUE_GECKOCHILDPROCESSHOST_H__

#include "base/file_path.h"
#include "base/process_util.h"
#include "base/scoped_ptr.h"
#include "base/waitable_event.h"
#include "chrome/common/child_process_host.h"

#include "mozilla/Monitor.h"

#include "nsXULAppAPI.h"        
#include "nsString.h"

namespace mozilla {
namespace ipc {

class GeckoChildProcessHost : public ChildProcessHost
{
protected:
  typedef mozilla::Monitor Monitor;
  typedef std::vector<std::string> StringVector;

public:
  typedef base::ChildPrivileges ChildPrivileges;
  typedef base::ProcessHandle ProcessHandle;

  static ChildPrivileges DefaultChildPrivileges();

  GeckoChildProcessHost(GeckoProcessType aProcessType,
                        ChildPrivileges aPrivileges=base::PRIVILEGES_DEFAULT);

  ~GeckoChildProcessHost();

  static nsresult GetArchitecturesForBinary(const char *path, uint32_t *result);

  static uint32_t GetSupportedArchitecturesForProcessType(GeckoProcessType type);

  
  
  
  bool AsyncLaunch(StringVector aExtraOpts=StringVector());

  
  
  
  
  
  
  
  
  
  
  
  
  bool LaunchAndWaitForProcessHandle(StringVector aExtraOpts=StringVector());

  
  
  
  bool SyncLaunch(StringVector aExtraOpts=StringVector(),
                  int32_t timeoutMs=0,
                  base::ProcessArchitecture arch=base::GetCurrentProcessArchitecture());

  bool PerformAsyncLaunch(StringVector aExtraOpts=StringVector(),
                          base::ProcessArchitecture arch=base::GetCurrentProcessArchitecture());

  virtual void OnChannelConnected(int32_t peer_pid);
  virtual void OnMessageReceived(const IPC::Message& aMsg);
  virtual void OnChannelError();
  virtual void GetQueuedMessages(std::queue<IPC::Message>& queue);

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

  



  void Join();

protected:
  GeckoProcessType mProcessType;
  ChildPrivileges mPrivileges;
  Monitor mMonitor;
  FilePath mProcessPath;
  
  enum {
    
    
    CREATING_CHANNEL = 0,
    
    
    CHANNEL_INITIALIZED,
    
    
    PROCESS_CREATED,
    
    
    PROCESS_CONNECTED,
    PROCESS_ERROR
  } mProcessState;

  static int32_t mChildCounter;

  void PrepareLaunch();

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

  void OpenPrivilegedHandle(base::ProcessId aPid);

  
  
  
  
  
  
  
  std::queue<IPC::Message> mQueue;
};

} 
} 

#endif 
