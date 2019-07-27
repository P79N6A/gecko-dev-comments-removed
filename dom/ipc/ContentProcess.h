





#ifndef dom_tabs_ContentThread_h
#define dom_tabs_ContentThread_h 1

#include "mozilla/ipc/ProcessChild.h"
#include "mozilla/ipc/ScopedXREEmbed.h"
#include "ContentChild.h"

namespace mozilla {
namespace dom {





class ContentProcess : public mozilla::ipc::ProcessChild
{
    typedef mozilla::ipc::ProcessChild ProcessChild;

public:
    explicit ContentProcess(ProcessId aParentPid)
        : ProcessChild(aParentPid)
    { }

    ~ContentProcess()
    { }

    virtual bool Init() override;
    virtual void CleanUp() override;

    void SetAppDir(const nsACString& aPath);

private:
    ContentChild mContent;
    mozilla::ipc::ScopedXREEmbed mXREEmbed;

    DISALLOW_EVIL_CONSTRUCTORS(ContentProcess);
};

}  
}  

#endif  
