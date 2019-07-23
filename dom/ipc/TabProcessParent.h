


#ifndef mozilla_tabs_TabProcessParent_h
#define mozilla_tabs_TabProcessParent_h

#include "mozilla/ipc/GeckoChildProcessHost.h"

namespace mozilla {
namespace tabs {

class TabProcessParent
    : private mozilla::ipc::GeckoChildProcessHost
{
public:
    TabProcessParent();
    ~TabProcessParent();

    


    bool Launch();

    IPC::Channel* GetChannel() {
        return channelp();
    }

    virtual bool CanShutdown() {
        return true;
    }

    base::WaitableEvent* GetShutDownEvent() {
        return GetProcessEvent();
    }

private:
    static char const *const kTabProcessName;

    DISALLOW_EVIL_CONSTRUCTORS(TabProcessParent);
};

}
}

#endif
