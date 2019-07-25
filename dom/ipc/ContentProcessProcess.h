






































#ifndef dom_tabs_ContentProcessThread_h
#define dom_tabs_ContentProcessThread_h 1

#include "mozilla/ipc/ProcessChild.h"
#include "mozilla/ipc/ScopedXREEmbed.h"
#include "ContentProcessChild.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s)  printf("[ContentProcessProcess] %s", s)

namespace mozilla {
namespace dom {





class ContentProcessProcess : public mozilla::ipc::ProcessChild
{
    typedef mozilla::ipc::ProcessChild ProcessChild;

public:
    ContentProcessProcess(ProcessHandle mParentHandle)
        : ProcessChild(mParentHandle)
    { }

    ~ContentProcessProcess()
    { }

    NS_OVERRIDE
    virtual bool Init();
    NS_OVERRIDE
    virtual void CleanUp();

private:
    ContentProcessChild mContentProcess;
    mozilla::ipc::ScopedXREEmbed mXREEmbed;

    DISALLOW_EVIL_CONSTRUCTORS(ContentProcessProcess);
};

}  
}  

#endif  
