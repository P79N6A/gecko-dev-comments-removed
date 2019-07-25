






































#ifndef dom_plugins_PluginThreadChild_h
#define dom_plugins_PluginThreadChild_h 1

#include "base/basictypes.h"

#include "chrome/common/child_thread.h"
#include "base/file_path.h"

#include "mozilla/ipc/GeckoThread.h"
#include "mozilla/plugins/PluginModuleChild.h"

namespace mozilla {
namespace plugins {




class PluginThreadChild : public mozilla::ipc::GeckoThread {
public:
    PluginThreadChild(ProcessHandle aParentHandle);
    ~PluginThreadChild();

private:
    
    virtual void Init();
    virtual void CleanUp();

    
    
    
    
    PluginModuleChild mPlugin;
    IPC::Channel* mChannel;

    DISALLOW_EVIL_CONSTRUCTORS(PluginThreadChild);
};

}  
}  

#endif  
