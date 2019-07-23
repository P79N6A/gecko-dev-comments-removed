






































#include "ContentProcessThread.h"

#include "prlink.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "chrome/common/child_process.h"
#include "chrome/common/chrome_switches.h"

using mozilla::ipc::GeckoThread;

namespace mozilla {
namespace dom {

ContentProcessThread::ContentProcessThread(ProcessHandle mParentHandle) :
    GeckoThread(mParentHandle),
    mContentProcess()
{
}

ContentProcessThread::~ContentProcessThread()
{
}

void
ContentProcessThread::Init()
{
    GeckoThread::Init();

    
    
    
    mContentProcess.Init(owner_loop(), GetParentProcessHandle(), channel());
}

void
ContentProcessThread::CleanUp()
{
    GeckoThread::CleanUp();
}

} 
} 
