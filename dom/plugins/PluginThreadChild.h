






































#ifndef dom_plugins_PluginThreadChild_h
#define dom_plugins_PluginThreadChild_h 1

#include "chrome/common/child_thread.h"
#include "base/file_path.h"

#include "mozilla/ipc/GeckoThread.h"
#include "mozilla/plugins/NPAPIPluginChild.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s)  printf("[PluginThreadChild] %s", s)

namespace mozilla {
namespace plugins {




class PluginThreadChild : public mozilla::ipc::GeckoThread {
public:
    PluginThreadChild();
    ~PluginThreadChild();

private:
    
    virtual void Init();
    virtual void CleanUp();

    
    
    
    
    NPAPIPluginChild mPlugin;
    IPC::Channel* mChannel;

    DISALLOW_EVIL_CONSTRUCTORS(PluginThreadChild);
};

}  
}  

#endif  
