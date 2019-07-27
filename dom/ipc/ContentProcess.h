





#ifndef dom_tabs_ContentThread_h
#define dom_tabs_ContentThread_h 1

#include "mozilla/ipc/ProcessChild.h"
#include "mozilla/ipc/ScopedXREEmbed.h"
#include "ContentChild.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s)  printf("[ContentProcess] %s", s)

namespace mozilla {
namespace dom {





class ContentProcess : public mozilla::ipc::ProcessChild
{
    typedef mozilla::ipc::ProcessChild ProcessChild;

public:
    explicit ContentProcess(ProcessHandle mParentHandle)
        : ProcessChild(mParentHandle)
    { }

    ~ContentProcess()
    { }

    virtual bool Init() MOZ_OVERRIDE;
    virtual void CleanUp() MOZ_OVERRIDE;

    void SetAppDir(const nsACString& aPath);

private:
    ContentChild mContent;
    mozilla::ipc::ScopedXREEmbed mXREEmbed;

    DISALLOW_EVIL_CONSTRUCTORS(ContentProcess);
};

}  
}  

#endif  
