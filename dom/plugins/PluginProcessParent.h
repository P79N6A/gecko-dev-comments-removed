






































#ifndef dom_plugins_PluginProcessParent_h
#define dom_plugins_PluginProcessParent_h 1

#include "base/basictypes.h"

#include "base/file_path.h"
#include "base/scoped_ptr.h"
#include "base/thread.h"
#include "base/waitable_event.h"
#include "chrome/common/child_process_host.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"

namespace mozilla {
namespace plugins {


class PluginProcessParent : public mozilla::ipc::GeckoChildProcessHost
{
public:
    PluginProcessParent(const std::string& aPluginFilePath);
    ~PluginProcessParent();

    



    bool Launch(PRInt32 timeoutMs);

    void Delete();

    virtual bool CanShutdown()
    {
        return true;
    }

    const std::string& GetPluginFilePath() { return mPluginFilePath; }

    using mozilla::ipc::GeckoChildProcessHost::GetShutDownEvent;
    using mozilla::ipc::GeckoChildProcessHost::GetChannel;
    using mozilla::ipc::GeckoChildProcessHost::GetChildProcessHandle;

private:
    std::string mPluginFilePath;

    DISALLOW_EVIL_CONSTRUCTORS(PluginProcessParent);
};


} 
} 

#endif 
