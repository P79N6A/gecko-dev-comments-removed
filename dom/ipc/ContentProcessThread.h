






































#ifndef dom_tabs_ContentProcessThread_h
#define dom_tabs_ContentProcessThread_h 1

#include "chrome/common/child_thread.h"
#include "base/file_path.h"

#include "mozilla/ipc/GeckoThread.h"
#include "ContentProcessChild.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s)  printf("[ContentProcessThread] %s", s)

namespace mozilla {
namespace dom {





class ContentProcessThread : public mozilla::ipc::GeckoThread
{
public:
    ContentProcessThread();
    ~ContentProcessThread();

private:
    
    virtual void Init();
    virtual void CleanUp();

    ContentProcessChild mContentProcess;
    IPC::Channel* mChannel;

    DISALLOW_EVIL_CONSTRUCTORS(ContentProcessThread);
};

}  
}  

#endif  
