





#ifndef dom_plugins_PluginProcessParent_h
#define dom_plugins_PluginProcessParent_h 1

#include "mozilla/Attributes.h"
#include "base/basictypes.h"

#include "base/file_path.h"
#include "base/thread.h"
#include "base/waitable_event.h"
#include "chrome/common/child_process_host.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"

namespace mozilla {
namespace plugins {


class PluginProcessParent : public mozilla::ipc::GeckoChildProcessHost
{
public:
    explicit PluginProcessParent(const std::string& aPluginFilePath);
    ~PluginProcessParent();

    



    bool Launch(int32_t timeoutMs);

    void Delete();

    virtual bool CanShutdown() MOZ_OVERRIDE
    {
        return true;
    }

    const std::string& GetPluginFilePath() { return mPluginFilePath; }

    using mozilla::ipc::GeckoChildProcessHost::GetShutDownEvent;
    using mozilla::ipc::GeckoChildProcessHost::GetChannel;

private:
    std::string mPluginFilePath;

    DISALLOW_EVIL_CONSTRUCTORS(PluginProcessParent);
};


} 
} 

#endif 
