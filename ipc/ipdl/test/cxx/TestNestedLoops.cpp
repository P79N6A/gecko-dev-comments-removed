#include "base/basictypes.h"

#include "nsThreadUtils.h"

#include "TestNestedLoops.h"

#include "IPDLUnitTests.h"      

template<>
struct RunnableMethodTraits<mozilla::_ipdltest::TestNestedLoopsParent>
{
    static void RetainCallee(mozilla::_ipdltest::TestNestedLoopsParent* obj) { }
    static void ReleaseCallee(mozilla::_ipdltest::TestNestedLoopsParent* obj) { }
};

namespace mozilla {
namespace _ipdltest {




TestNestedLoopsParent::TestNestedLoopsParent() : mBreakNestedLoop(false)
{
    MOZ_COUNT_CTOR(TestNestedLoopsParent);
}

TestNestedLoopsParent::~TestNestedLoopsParent()
{
    MOZ_COUNT_DTOR(TestNestedLoopsParent);
}

void
TestNestedLoopsParent::Main()
{
    if (!SendStart())
        fail("sending Start");

    
    puts(" (sleeping to wait for nonce ... sorry)");
    PR_Sleep(5000);

    
    if (!CallR())
        fail("calling R");

    Close();
}

bool
TestNestedLoopsParent::RecvNonce()
{
    
    
    
    MessageLoop::current()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &TestNestedLoopsParent::BreakNestedLoop));

    
    puts(" (sleeping to wait for reply to R ... sorry)");
    PR_Sleep(5000);

    
    do {
        if (!NS_ProcessNextEvent(nsnull, false))
            fail("expected at least one pending event");
    } while (!mBreakNestedLoop);

    return true;
}

void
TestNestedLoopsParent::BreakNestedLoop()
{
    mBreakNestedLoop = true;
}




TestNestedLoopsChild::TestNestedLoopsChild()
{
    MOZ_COUNT_CTOR(TestNestedLoopsChild);
}

TestNestedLoopsChild::~TestNestedLoopsChild()
{
    MOZ_COUNT_DTOR(TestNestedLoopsChild);
}

bool
TestNestedLoopsChild::RecvStart()
{
    if (!SendNonce())
        fail("sending Nonce");
    return true;
}

bool
TestNestedLoopsChild::AnswerR()
{
    return true;
}

} 
} 
