






































#include "mozilla/ipc/IOThreadChild.h"

#include "ContentProcess.h"

using mozilla::ipc::IOThreadChild;

namespace mozilla {
namespace dom {

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
    mXREEmbed.Stop();
}

} 
} 
