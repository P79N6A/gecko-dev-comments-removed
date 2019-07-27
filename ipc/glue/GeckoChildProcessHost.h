



#ifndef __IPC_GLUE_GECKOCHILDPROCESSHOST_H__
#define __IPC_GLUE_GECKOCHILDPROCESSHOST_H__

#include "base/file_path.h"
#include "base/process_util.h"
#include "base/waitable_event.h"
#include "chrome/common/child_process_host.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/ipc/FileDescriptor.h"
#include "mozilla/Monitor.h"
#include "mozilla/StaticPtr.h"

#include "nsCOMPtr.h"
#include "nsXULAppAPI.h"        
#include "nsString.h"

#if defined(XP_WIN) && defined(MOZ_SANDBOX)
#include "sandboxBroker.h"
#endif

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

  explicit GeckoChildProcessHost(GeckoProcessType aProcessType,
                                 ChildPrivileges aPrivileges=base::PRIVILEGES_DEFAULT);

  ~GeckoChildProcessHost();

  static nsresult GetArchitecturesForBinary(const char *path, uint32_t *result);

  static uint32_t GetSupportedArchitecturesForProcessType(GeckoProcessType type);

  
  
  
  bool AsyncLaunch(StringVector aExtraOpts=StringVector(),
                   base::ProcessArchitecture arch=base::GetCurrentProcessArchitecture());

  virtual bool WaitUntilConnected(int32_t aTimeoutMs = 0);

  
  
  
  
  
  
  
  
  
  
  
  
  bool LaunchAndWaitForProcessHandle(StringVector aExtraOpts=StringVector());

  
  
  
  bool SyncLaunch(StringVector aExtraOpts=StringVector(),
                  int32_t timeoutMs=0,
                  base::ProcessArchitecture arch=base::GetCurrentProcessArchitecture());

  virtual bool PerformAsyncLaunch(StringVector aExtraOpts=StringVector(),
                                  base::ProcessArchitecture aArch=base::GetCurrentProcessArchitecture());

  virtual void OnChannelConnected(int32_t peer_pid);
  virtual void OnMessageReceived(const IPC::Message& aMsg);
  virtual void OnChannelError();
  virtual void GetQueuedMessages(std::queue<IPC::Message>& queue);

  virtual void InitializeChannel();

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

  GeckoProcessType GetProcessType() {
    return mProcessType;
  }

#ifdef XP_MACOSX
  task_t GetChildTask() {
    return mChildTask;
  }
#endif

  



  void Join();

  
  void SetAlreadyDead();

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

#ifdef MOZ_SANDBOX
  SandboxBroker mSandboxBroker;
  std::vector<std::wstring> mAllowedFilesRead;
  std::vector<std::wstring> mAllowedFilesReadWrite;
  bool mEnableSandboxLogging;
  int32_t mSandboxLevel;
#endif
#endif 

#if defined(OS_POSIX)
  base::file_handle_mapping_vector mFileMap;
#endif

  base::WaitableEventWatcher::Delegate* mDelegate;

  ProcessHandle mChildProcessHandle;
#if defined(OS_MACOSX)
  task_t mChildTask;
#endif

  void OpenPrivilegedHandle(base::ProcessId aPid);

private:
  DISALLOW_EVIL_CONSTRUCTORS(GeckoChildProcessHost);

  
  bool PerformAsyncLaunchInternal(std::vector<std::string>& aExtraOpts,
                                  base::ProcessArchitecture arch);

  bool RunPerformAsyncLaunch(StringVector aExtraOpts=StringVector(),
			     base::ProcessArchitecture aArch=base::GetCurrentProcessArchitecture());

  static void GetPathToBinary(FilePath& exePath);

  
  
  
  
  
  
  
  std::queue<IPC::Message> mQueue;
};

#ifdef MOZ_NUWA_PROCESS
class GeckoExistingProcessHost final : public GeckoChildProcessHost
{
public:
  GeckoExistingProcessHost(GeckoProcessType aProcessType,
                           base::ProcessHandle aProcess,
                           const FileDescriptor& aFileDescriptor,
                           ChildPrivileges aPrivileges=base::PRIVILEGES_DEFAULT);

  ~GeckoExistingProcessHost();

  virtual bool PerformAsyncLaunch(StringVector aExtraOpts=StringVector(),
          base::ProcessArchitecture aArch=base::GetCurrentProcessArchitecture()) override;

  virtual void InitializeChannel() override;

private:
  base::ProcessHandle mExistingProcessHandle;
  mozilla::ipc::FileDescriptor mExistingFileDescriptor;
};
#endif 

} 
} 

#endif 
