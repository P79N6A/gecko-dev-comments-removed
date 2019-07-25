





































#include "mozilla/ipc/IOThreadChild.h"

#include "IPDLUnitTestProcessChild.h"
#include "IPDLUnitTests.h"

using mozilla::ipc::IOThreadChild;

namespace mozilla {
namespace _ipdltest {

bool
IPDLUnitTestProcessChild::Init()
{
    IPDLUnitTestChildInit(IOThreadChild::channel(),
                          ParentHandle(),
                          IOThreadChild::message_loop());
    return true;
}

} 
} 
