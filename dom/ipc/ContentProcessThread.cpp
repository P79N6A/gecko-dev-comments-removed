






































#include "ContentProcessThread.h"

#include "prlink.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "chrome/common/child_process.h"
#include "chrome/common/chrome_switches.h"

using mozilla::ipc::MozillaChildThread;

namespace mozilla {
namespace dom {

ContentProcessThread::ContentProcessThread(ProcessHandle mParentHandle) :
    MozillaChildThread(mParentHandle, MessageLoop::TYPE_MOZILLA_UI),
    mContentProcess()
{
}

ContentProcessThread::~ContentProcessThread()
{
}

void
ContentProcessThread::Init()
{
    MozillaChildThread::Init();
    mXREEmbed.Start();

    
    
    
    mContentProcess.Init(owner_loop(), GetParentProcessHandle(), channel());
}

void
ContentProcessThread::CleanUp()
{
    mXREEmbed.Stop();
    MozillaChildThread::CleanUp();
}

} 
} 
