






































#ifndef dom_plugins_PluginProcessChild_h
#define dom_plugins_PluginProcessChild_h 1

#include "mozilla/ipc/ProcessChild.h"
#include "mozilla/plugins/PluginModuleChild.h"

namespace mozilla {
namespace plugins {


class PluginProcessChild : public mozilla::ipc::ProcessChild {
protected:
    typedef mozilla::ipc::ProcessChild ProcessChild;

public:
    PluginProcessChild(ProcessHandle parentHandle) : ProcessChild(parentHandle)
    { }

    virtual ~PluginProcessChild()
    { }

    NS_OVERRIDE virtual bool Init();
    NS_OVERRIDE virtual void CleanUp();

    
    static void AppendNotesToCrashReport(const nsCString& aNotes);

protected:
    static PluginProcessChild* current() {
        return static_cast<PluginProcessChild*>(ProcessChild::current());
    }

private:
    PluginModuleChild mPlugin;

    DISALLOW_EVIL_CONSTRUCTORS(PluginProcessChild);
};

}  
}  

#endif  
