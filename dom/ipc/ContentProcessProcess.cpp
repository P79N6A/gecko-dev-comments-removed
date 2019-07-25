






































#include "mozilla/ipc/IOThreadChild.h"

#include "ContentProcessProcess.h"

using mozilla::ipc::IOThreadChild;

namespace mozilla {
namespace dom {

bool
ContentProcessProcess::Init()
{
    mContentProcess.Init(IOThreadChild::message_loop(),
                         ParentHandle(),
                         IOThreadChild::channel());
    mXREEmbed.Start();
    
    return true;
}

void
ContentProcessProcess::CleanUp()
{
    mXREEmbed.Stop();
}

} 
} 
