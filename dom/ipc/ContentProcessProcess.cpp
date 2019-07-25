






































#include "mozilla/ipc/IOThreadChild.h"

#include "ContentProcessProcess.h"

using mozilla::ipc::IOThreadChild;

namespace mozilla {
namespace dom {

bool
ContentProcessProcess::Init()
{
    mXREEmbed.Start();
    mContentProcess.Init(IOThreadChild::message_loop(),
                         ParentHandle(),
                         IOThreadChild::channel());
    return true;
}

void
ContentProcessProcess::CleanUp()
{
    mXREEmbed.Stop();
}

} 
} 
