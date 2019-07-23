






































#ifndef dom_tabs_TabThread_h
#define dom_tabs_TabThread_h 1

#include "chrome/common/child_thread.h"
#include "base/file_path.h"

#include "mozilla/ipc/GeckoThread.h"
#include "TabChild.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s)  printf("[TabThread] %s", s)

namespace mozilla {
namespace tabs {




class TabThread : public mozilla::ipc::GeckoThread {
public:
    TabThread();
    ~TabThread();

private:
    
    virtual void Init();
    virtual void CleanUp();

    TabChild mTab;
    IPC::Channel* mChannel;

    DISALLOW_EVIL_CONSTRUCTORS(TabThread);
};

}  
}  

#endif  
