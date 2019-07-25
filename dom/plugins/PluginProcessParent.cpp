






































#include "mozilla/plugins/PluginProcessParent.h"

#include "base/string_util.h"
#include "mozilla/ipc/BrowserProcessSubThread.h"

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
PluginProcessParent::Launch()
{
    std::vector<std::string> args;
#if defined(XP_WIN)
    args.push_back("\""+ mPluginFilePath +"\"");
#else
    args.push_back(mPluginFilePath);
#endif
    return SyncLaunch(args);
}

void
PluginProcessParent::Delete()
{
  MessageLoop* currentLoop = MessageLoop::current();
  MessageLoop* ioLoop = 
    BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO);

  if (currentLoop == ioLoop) {
      delete this;
      return;
  }

  ioLoop->PostTask(FROM_HERE,
                   NewRunnableMethod(this, &PluginProcessParent::Delete));
}
