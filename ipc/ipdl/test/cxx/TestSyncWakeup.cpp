#if defined(OS_POSIX)
#include <unistd.h>             
#endif

#include "TestSyncWakeup.h"

#include "IPDLUnitTests.h"      

namespace mozilla {
namespace _ipdltest {




TestSyncWakeupParent::TestSyncWakeupParent()
{
    MOZ_COUNT_CTOR(TestSyncWakeupParent);
}

TestSyncWakeupParent::~TestSyncWakeupParent()
{
    MOZ_COUNT_DTOR(TestSyncWakeupParent);
}

void
TestSyncWakeupParent::Main()
{
    if (!SendStart())
        fail("sending Start()");
}

bool
TestSyncWakeupParent::AnswerStackFrame()
{
    if (!CallStackFrame())
        fail("calling StackFrame()");
    return true;
}

bool
TestSyncWakeupParent::RecvSync1()
{
    if (!SendNote1())
        fail("sending Note1()");

    
    
    
#if defined(OS_POSIX)
    
    
    
    puts(" (sleeping for 5 seconds. sorry!)");
    sleep(5);
#endif

    return true;
}

bool
TestSyncWakeupParent::RecvSync2()
{
    if (!SendNote2())
        fail("sending Note2()");

#if defined(OS_POSIX)
    
    sleep(5);
    puts(" (sleeping for 5 seconds. sorry!)");
#endif

    return true;
}




TestSyncWakeupChild::TestSyncWakeupChild() : mDone(false)
{
    MOZ_COUNT_CTOR(TestSyncWakeupChild);
}

TestSyncWakeupChild::~TestSyncWakeupChild()
{
    MOZ_COUNT_DTOR(TestSyncWakeupChild);
}

bool
TestSyncWakeupChild::RecvStart()
{
    
    
    if (!SendSync1())
        fail("sending Sync()");

    
    
    return true;
}

bool
TestSyncWakeupChild::RecvNote1()
{
    
    
    if (!CallStackFrame())
        fail("calling StackFrame()");

    if (!mDone)
        fail("should have received Note2()!");

    Close();

    return true;
}

bool
TestSyncWakeupChild::AnswerStackFrame()
{
    if (!SendSync2())
        fail("sending Sync()");

    return true;
}

bool
TestSyncWakeupChild::RecvNote2()
{
    mDone = true;
    return true;
}

} 
} 
