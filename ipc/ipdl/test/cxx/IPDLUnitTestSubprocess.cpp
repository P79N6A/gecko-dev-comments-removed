





































#include "IPDLUnitTestSubprocess.h"

using mozilla::ipc::GeckoChildProcessHost;

namespace mozilla {
namespace _ipdltest {

IPDLUnitTestSubprocess::IPDLUnitTestSubprocess() :
    GeckoChildProcessHost(GeckoProcessType_IPDLUnitTest)
{
}

IPDLUnitTestSubprocess::~IPDLUnitTestSubprocess()
{
}

} 
} 
