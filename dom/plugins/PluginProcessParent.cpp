






































#include "mozilla/plugins/PluginProcessParent.h"

#include "base/string_util.h"
#include "mozilla/ipc/BrowserProcessSubThread.h"
#include "mozilla/plugins/PluginMessageUtils.h"

using std::vector;
using std::string;

using mozilla::ipc::BrowserProcessSubThread;
using mozilla::ipc::GeckoChildProcessHost;
using mozilla::plugins::PluginProcessParent;

template<>
struct RunnableMethodTraits<PluginProcessParent>
{
    static void RetainCallee(PluginProcessParent* obj) { }
    static void ReleaseCallee(PluginProcessParent* obj) { }
};

PluginProcessParent::PluginProcessParent(const std::string& aPluginFilePath) :
    GeckoChildProcessHost(GeckoProcessType_Plugin),
    mPluginFilePath(aPluginFilePath)
{
}

PluginProcessParent::~PluginProcessParent()
{
}

bool
PluginProcessParent::Launch(PRInt32 timeoutMs)
{
    vector<string> args;
    args.push_back(MungePluginDsoPath(mPluginFilePath));
    return SyncLaunch(args, timeoutMs);
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
