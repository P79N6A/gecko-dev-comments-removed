





































#include "IPDLUnitTestThreadChild.h"

#include "IPDLUnitTests.h"

using mozilla::ipc::GeckoThread;

namespace mozilla {
namespace _ipdltest {

IPDLUnitTestThreadChild::IPDLUnitTestThreadChild() :
    GeckoThread()
{
}

IPDLUnitTestThreadChild::~IPDLUnitTestThreadChild()
{
}

void
IPDLUnitTestThreadChild::Init()
{
    GeckoThread::Init();
    IPDLUnitTestChildInit(channel(), owner_loop());
}

void
IPDLUnitTestThreadChild::CleanUp()
{
    GeckoThread::CleanUp();
    IPDLUnitTestChildCleanUp();
}

} 
} 
