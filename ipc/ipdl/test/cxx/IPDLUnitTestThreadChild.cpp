





































#include "IPDLUnitTestThreadChild.h"

#include "IPDLUnitTests.h"

using mozilla::ipc::GeckoThread;

namespace mozilla {
namespace _ipdltest {

IPDLUnitTestThreadChild::IPDLUnitTestThreadChild(ProcessHandle aParentHandle) :
    GeckoThread(aParentHandle)
{
}

IPDLUnitTestThreadChild::~IPDLUnitTestThreadChild()
{
}

void
IPDLUnitTestThreadChild::Init()
{
    GeckoThread::Init();
    IPDLUnitTestChildInit(channel(), GetParentProcessHandle(), owner_loop());
}

void
IPDLUnitTestThreadChild::CleanUp()
{
    GeckoThread::CleanUp();
}

} 
} 
