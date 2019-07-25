#include "TestSyncError.h"

#include "IPDLUnitTests.h"      

namespace mozilla {
namespace _ipdltest {




TestSyncErrorParent::TestSyncErrorParent()
{
    MOZ_COUNT_CTOR(TestSyncErrorParent);
}

TestSyncErrorParent::~TestSyncErrorParent()
{
    MOZ_COUNT_DTOR(TestSyncErrorParent);
}

void
TestSyncErrorParent::Main()
{
    if (!SendStart())
        fail("sending Start");
}

bool
TestSyncErrorParent::RecvError()
{
    return false;
}





TestSyncErrorChild::TestSyncErrorChild()
{
    MOZ_COUNT_CTOR(TestSyncErrorChild);
}

TestSyncErrorChild::~TestSyncErrorChild()
{
    MOZ_COUNT_DTOR(TestSyncErrorChild);
}

bool
TestSyncErrorChild::RecvStart()
{
    if (SendError())
        fail("Error() should have return false");

    Close();

    return true;
}


} 
} 
