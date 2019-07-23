






































#include "TabThread.h"

#include "prlink.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "chrome/common/child_process.h"
#include "chrome/common/chrome_switches.h"

using mozilla::ipc::GeckoThread;

namespace mozilla {
namespace tabs {

TabThread::TabThread() :
    GeckoThread(),
    mTab()
{
}

TabThread::~TabThread()
{
}

void
TabThread::Init()
{
    GeckoThread::Init();

    
    
    
    mTab.Init(owner_loop(), channel());
}

void
TabThread::CleanUp()
{
    GeckoThread::CleanUp();
}

} 
} 
