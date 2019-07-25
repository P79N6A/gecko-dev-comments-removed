





































#include "IPDLUnitTestThreadChild.h"

#include "IPDLUnitTests.h"

using mozilla::ipc::MozillaChildThread;

namespace mozilla {
namespace _ipdltest {

IPDLUnitTestThreadChild::IPDLUnitTestThreadChild(ProcessHandle aParentHandle) :
    MozillaChildThread(aParentHandle)
{
}

IPDLUnitTestThreadChild::~IPDLUnitTestThreadChild()
{
}

void
IPDLUnitTestThreadChild::Init()
{
    MozillaChildThread::Init();
    IPDLUnitTestChildInit(channel(), GetParentProcessHandle(), owner_loop());
}

} 
} 
