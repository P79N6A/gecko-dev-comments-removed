






#include "TestHighestPrio.h"

#include "IPDLUnitTests.h"      
#if defined(OS_POSIX)
#include <unistd.h>
#else
#include <windows.h>
#endif

namespace mozilla {
namespace _ipdltest {




TestHighestPrioParent::TestHighestPrioParent()
  : msg_num_(0)
{
    MOZ_COUNT_CTOR(TestHighestPrioParent);
}

TestHighestPrioParent::~TestHighestPrioParent()
{
    MOZ_COUNT_DTOR(TestHighestPrioParent);
}

void
TestHighestPrioParent::Main()
{
    if (!SendStart())
        fail("sending Start");
}

bool
TestHighestPrioParent::RecvMsg1()
{
    MOZ_ASSERT(msg_num_ == 0);
    msg_num_ = 1;
    return true;
}

bool
TestHighestPrioParent::RecvMsg2()
{

    MOZ_ASSERT(msg_num_ == 1);
    msg_num_ = 2;

    if (!SendStartInner())
        fail("sending StartInner");

    return true;
}

bool
TestHighestPrioParent::RecvMsg3()
{
    MOZ_ASSERT(msg_num_ == 2);
    msg_num_ = 3;
    return true;
}

bool
TestHighestPrioParent::RecvMsg4()
{
    MOZ_ASSERT(msg_num_ == 3);
    msg_num_ = 4;
    return true;
}





TestHighestPrioChild::TestHighestPrioChild()
{
    MOZ_COUNT_CTOR(TestHighestPrioChild);
}

TestHighestPrioChild::~TestHighestPrioChild()
{
    MOZ_COUNT_DTOR(TestHighestPrioChild);
}

bool
TestHighestPrioChild::RecvStart()
{
    if (!SendMsg1())
        fail("sending Msg1");

    if (!SendMsg2())
        fail("sending Msg2");

    Close();
    return true;
}

bool
TestHighestPrioChild::RecvStartInner()
{
    if (!SendMsg3())
        fail("sending Msg3");

    if (!SendMsg4())
        fail("sending Msg4");

    return true;
}

} 
} 
