






































#ifndef dom_plugins_PluginProcessParent_h
#define dom_plugins_PluginProcessParent_h 1

#include "base/basictypes.h"

#include "base/file_path.h"
#include "base/scoped_ptr.h"
#include "base/thread.h"
#include "base/waitable_event.h"
#include "chrome/common/child_process_host.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s) printf("[PluginProcessParent] %s\n", s)

namespace mozilla {
namespace plugins {


class PluginProcessParent : mozilla::ipc::GeckoChildProcessHost
{
public:
    PluginProcessParent(const std::string& aPluginFilePath);
    ~PluginProcessParent();

    


    bool Launch();

    IPC::Channel* GetChannel() {
        return channelp();
    }

    virtual bool CanShutdown()
    {
        return true;
    }

    base::WaitableEvent* GetShutDownEvent()
    {
        return GetProcessEvent();
    }

private:
    static const char* kPluginProcessName;
    std::string mPluginFilePath;

    DISALLOW_EVIL_CONSTRUCTORS(PluginProcessParent);
};


} 
} 

#endif 
