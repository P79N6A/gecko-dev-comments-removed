





#include "mozilla/ipc/IOThreadChild.h"

#include "ContentProcess.h"

using mozilla::ipc::IOThreadChild;

namespace mozilla {
namespace dom {

void
ContentProcess::SetAppDir(const nsACString& aPath)
{
  mXREEmbed.SetAppDir(aPath);
}

bool
ContentProcess::Init()
{
    mContent.Init(IOThreadChild::message_loop(),
                         ParentHandle(),
                         IOThreadChild::channel());
    mXREEmbed.Start();
    mContent.InitXPCOM();
    
    return true;
}

void
ContentProcess::CleanUp()
{
#if defined(XP_WIN) && defined(MOZ_CONTENT_SANDBOX)
    mContent.CleanUpSandboxEnvironment();
#endif
    mXREEmbed.Stop();
}

} 
} 
