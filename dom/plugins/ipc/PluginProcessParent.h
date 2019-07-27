





#ifndef dom_plugins_PluginProcessParent_h
#define dom_plugins_PluginProcessParent_h 1

#include "mozilla/Attributes.h"
#include "base/basictypes.h"

#include "base/file_path.h"
#include "base/task.h"
#include "base/thread.h"
#include "base/waitable_event.h"
#include "chrome/common/child_process_host.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "mozilla/UniquePtr.h"
#include "nsCOMPtr.h"
#include "nsIRunnable.h"

namespace mozilla {
namespace plugins {

class LaunchCompleteTask : public Task
{
public:
    LaunchCompleteTask()
        : mLaunchSucceeded(false)
    {
    }

    void SetLaunchSucceeded() { mLaunchSucceeded = true; }

protected:
    bool mLaunchSucceeded;
};

class PluginProcessParent : public mozilla::ipc::GeckoChildProcessHost
{
public:
    explicit PluginProcessParent(const std::string& aPluginFilePath);
    ~PluginProcessParent();

    








    bool Launch(UniquePtr<LaunchCompleteTask> aLaunchCompleteTask = UniquePtr<LaunchCompleteTask>(),
                int32_t aSandboxLevel = 0);

    void Delete();

    virtual bool CanShutdown() MOZ_OVERRIDE
    {
        return true;
    }

    const std::string& GetPluginFilePath() { return mPluginFilePath; }

    using mozilla::ipc::GeckoChildProcessHost::GetShutDownEvent;
    using mozilla::ipc::GeckoChildProcessHost::GetChannel;

    void SetCallRunnableImmediately(bool aCallImmediately);
    virtual bool WaitUntilConnected(int32_t aTimeoutMs = 0) MOZ_OVERRIDE;

    virtual void OnChannelConnected(int32_t peer_pid) MOZ_OVERRIDE;
    virtual void OnChannelError() MOZ_OVERRIDE;

    bool IsConnected();

private:
    void RunLaunchCompleteTask();

    std::string mPluginFilePath;
    UniquePtr<LaunchCompleteTask> mLaunchCompleteTask;
    MessageLoop* mMainMsgLoop;
    bool mRunCompleteTaskImmediately;

    DISALLOW_EVIL_CONSTRUCTORS(PluginProcessParent);
};


} 
} 

#endif 
