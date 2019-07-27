


#include "TestUrgentHangs.h"

#include "IPDLUnitTests.h"      
#include "prthread.h"
#if defined(OS_POSIX)
#include <unistd.h>
#else
#include <windows.h>
#endif

template<>
struct RunnableMethodTraits<mozilla::_ipdltest::TestUrgentHangsParent>
{
    static void RetainCallee(mozilla::_ipdltest::TestUrgentHangsParent* obj) { }
    static void ReleaseCallee(mozilla::_ipdltest::TestUrgentHangsParent* obj) { }
};

namespace mozilla {
namespace _ipdltest {




TestUrgentHangsParent::TestUrgentHangsParent()
  : mInnerCount(0),
    mInnerUrgentCount(0)
{
    MOZ_COUNT_CTOR(TestUrgentHangsParent);
}

TestUrgentHangsParent::~TestUrgentHangsParent()
{
    MOZ_COUNT_DTOR(TestUrgentHangsParent);
}

void
TestUrgentHangsParent::Main()
{
    SetReplyTimeoutMs(1000);

    
    
    if (!SendTest1_1())
        fail("sending Test1_1");

    
    if (SendTest2())
        fail("sending Test2");

    
    if (SendTest3())
        fail("sending Test3");

    
    MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        NewRunnableMethod(this, &TestUrgentHangsParent::SecondStage),
        3000);
}

void
TestUrgentHangsParent::SecondStage()
{
    
    
    if (!SendTest4())
        fail("sending Test4");

    
    
    if (SendTest4_1())
        fail("sending Test4_1");

    MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        NewRunnableMethod(this, &TestUrgentHangsParent::ThirdStage),
        3000);
}

void
TestUrgentHangsParent::ThirdStage()
{
    
    
    

    
    
    if (!SendTest5())
        fail("sending Test5");

    
    
    if (SendTest5_1())
        fail("sending Test5_1");

    
    MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        NewRunnableMethod(this, &TestUrgentHangsParent::Close),
        3000);
}

bool
TestUrgentHangsParent::RecvTest1_2()
{
    if (!SendTest1_3())
        fail("sending Test1_3");
    return true;
}

bool
TestUrgentHangsParent::RecvTestInner()
{
    mInnerCount++;
    return true;
}

bool
TestUrgentHangsParent::RecvTestInnerUrgent()
{
    mInnerUrgentCount++;
    return true;
}




bool
TestUrgentHangsChild::RecvTest1_1()
{
    if (!SendTest1_2())
        fail("sending Test1_2");

    return true;
}

bool
TestUrgentHangsChild::RecvTest1_3()
{
    PR_Sleep(PR_SecondsToInterval(2));

    return true;
}

bool
TestUrgentHangsChild::RecvTest2()
{
    PR_Sleep(PR_SecondsToInterval(2));

    
    if (SendTestInner())
        fail("sending TestInner");

    return true;
}

bool
TestUrgentHangsChild::RecvTest3()
{
    fail("RecvTest3 should never be called");
    return true;
}

bool
TestUrgentHangsChild::RecvTest4()
{
    PR_Sleep(PR_SecondsToInterval(2));

    
    
    if (!SendTestInner())
        fail("sending TestInner");

    return true;
}

bool
TestUrgentHangsChild::RecvTest4_1()
{
    
    
    if (SendTestInner())
        fail("sending TestInner");

    return true;
}

bool
TestUrgentHangsChild::RecvTest5()
{
    PR_Sleep(PR_SecondsToInterval(2));

    
    
    if (!SendTestInnerUrgent())
        fail("sending TestInner");

    return true;
}

bool
TestUrgentHangsChild::RecvTest5_1()
{
    
    
    if (!SendTestInnerUrgent())
        fail("sending TestInner");

    return true;
}

TestUrgentHangsChild::TestUrgentHangsChild()
{
    MOZ_COUNT_CTOR(TestUrgentHangsChild);
}

TestUrgentHangsChild::~TestUrgentHangsChild()
{
    MOZ_COUNT_DTOR(TestUrgentHangsChild);
}

} 
} 
