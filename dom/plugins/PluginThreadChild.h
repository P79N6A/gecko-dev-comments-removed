






































#ifndef dom_plugins_PluginThreadChild_h
#define dom_plugins_PluginThreadChild_h 1

#include "base/basictypes.h"

#include "mozilla/ipc/MozillaChildThread.h"
#include "base/file_path.h"
#include "base/process.h"

#include "mozilla/plugins/PluginModuleChild.h"

namespace mozilla {
namespace plugins {




class PluginThreadChild : public mozilla::ipc::MozillaChildThread {
public:
    PluginThreadChild(ProcessHandle aParentHandle);
    ~PluginThreadChild();

    static PluginThreadChild* current() {
        return gInstance;
    }

    
    static void AppendNotesToCrashReport(const nsCString& aNotes);

private:
    static PluginThreadChild* gInstance;

    
    virtual void Init();
    virtual void CleanUp();

    PluginModuleChild mPlugin;
    IPC::Channel* mChannel;

    DISALLOW_EVIL_CONSTRUCTORS(PluginThreadChild);
};

}  
}  

#endif  
