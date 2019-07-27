





#include "mozilla/plugins/PluginProcessParent.h"

#include "base/string_util.h"
#include "base/process_util.h"

#include "mozilla/ipc/BrowserProcessSubThread.h"
#include "mozilla/plugins/PluginMessageUtils.h"
#include "mozilla/Telemetry.h"
#include "nsThreadUtils.h"

using std::vector;
using std::string;

using mozilla::ipc::BrowserProcessSubThread;
using mozilla::ipc::GeckoChildProcessHost;
using mozilla::plugins::PluginProcessParent;
using base::ProcessArchitecture;

template<>
struct RunnableMethodTraits<PluginProcessParent>
{
    static void RetainCallee(PluginProcessParent* obj) { }
    static void ReleaseCallee(PluginProcessParent* obj) { }
};

PluginProcessParent::PluginProcessParent(const std::string& aPluginFilePath) :
    GeckoChildProcessHost(GeckoProcessType_Plugin),
    mPluginFilePath(aPluginFilePath),
    mMainMsgLoop(MessageLoop::current()),
    mRunCompleteTaskImmediately(false)
{
}

PluginProcessParent::~PluginProcessParent()
{
}

bool
PluginProcessParent::Launch(UniquePtr<LaunchCompleteTask> aLaunchCompleteTask)
{
    ProcessArchitecture currentArchitecture = base::GetCurrentProcessArchitecture();
    uint32_t containerArchitectures = GetSupportedArchitecturesForProcessType(GeckoProcessType_Plugin);

    uint32_t pluginLibArchitectures = currentArchitecture;
#ifdef XP_MACOSX
    nsresult rv = GetArchitecturesForBinary(mPluginFilePath.c_str(), &pluginLibArchitectures);
    if (NS_FAILED(rv)) {
        
        pluginLibArchitectures = currentArchitecture;
    }
#endif

    ProcessArchitecture selectedArchitecture = currentArchitecture;
    if (!(pluginLibArchitectures & containerArchitectures & currentArchitecture)) {
        
        
        if (base::PROCESS_ARCH_X86_64 & pluginLibArchitectures & containerArchitectures) {
            selectedArchitecture = base::PROCESS_ARCH_X86_64;
        }
        else if (base::PROCESS_ARCH_I386 & pluginLibArchitectures & containerArchitectures) {
            selectedArchitecture = base::PROCESS_ARCH_I386;
        }
        else if (base::PROCESS_ARCH_PPC & pluginLibArchitectures & containerArchitectures) {
            selectedArchitecture = base::PROCESS_ARCH_PPC;
        }
        else if (base::PROCESS_ARCH_ARM & pluginLibArchitectures & containerArchitectures) {
          selectedArchitecture = base::PROCESS_ARCH_ARM;
        }
        else {
            return false;
        }
    }

    mLaunchCompleteTask = Move(aLaunchCompleteTask);

    vector<string> args;
    args.push_back(MungePluginDsoPath(mPluginFilePath));

    bool result = AsyncLaunch(args, selectedArchitecture);
    if (!result) {
        mLaunchCompleteTask = nullptr;
    }
    return result;
}

void
PluginProcessParent::Delete()
{
  MessageLoop* currentLoop = MessageLoop::current();
  MessageLoop* ioLoop = XRE_GetIOMessageLoop();

  if (currentLoop == ioLoop) {
      delete this;
      return;
  }

  ioLoop->PostTask(FROM_HERE,
                   NewRunnableMethod(this, &PluginProcessParent::Delete));
}

void
PluginProcessParent::SetCallRunnableImmediately(bool aCallImmediately)
{
    mRunCompleteTaskImmediately = aCallImmediately;
}

bool
PluginProcessParent::WaitUntilConnected(int32_t aTimeoutMs)
{
    bool result = GeckoChildProcessHost::WaitUntilConnected(aTimeoutMs);
    if (mRunCompleteTaskImmediately && mLaunchCompleteTask) {
        if (result) {
            mLaunchCompleteTask->SetLaunchSucceeded();
        }
        mLaunchCompleteTask->Run();
        mLaunchCompleteTask = nullptr;
    }
    return result;
}

void
PluginProcessParent::OnChannelConnected(int32_t peer_pid)
{
    GeckoChildProcessHost::OnChannelConnected(peer_pid);
    if (mLaunchCompleteTask && !mRunCompleteTaskImmediately) {
        mLaunchCompleteTask->SetLaunchSucceeded();
        mMainMsgLoop->PostTask(FROM_HERE, mLaunchCompleteTask.release());
    }
}

void
PluginProcessParent::OnChannelError()
{
    GeckoChildProcessHost::OnChannelError();
    if (mLaunchCompleteTask && !mRunCompleteTaskImmediately) {
        mMainMsgLoop->PostTask(FROM_HERE, mLaunchCompleteTask.release());
    }
}

bool
PluginProcessParent::IsConnected()
{
    mozilla::MonitorAutoLock lock(mMonitor);
    return mProcessState == PROCESS_CONNECTED;
}

