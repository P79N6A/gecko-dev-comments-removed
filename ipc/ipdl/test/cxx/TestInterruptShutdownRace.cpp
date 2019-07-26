#include "TestInterruptShutdownRace.h"

#include "IPDLUnitTests.h"      
#include "IPDLUnitTestSubprocess.h"

template<>
struct RunnableMethodTraits<mozilla::_ipdltest::TestInterruptShutdownRaceParent>
{
    static void RetainCallee(mozilla::_ipdltest::TestInterruptShutdownRaceParent* obj) { }
    static void ReleaseCallee(mozilla::_ipdltest::TestInterruptShutdownRaceParent* obj) { }
};


namespace mozilla {
namespace _ipdltest {




namespace {




void DeleteSubprocess()
{
    delete gSubprocess;
    gSubprocess = nullptr;
}

void Done()
{
    passed(__FILE__);
    QuitParent();
}

} 

TestInterruptShutdownRaceParent::TestInterruptShutdownRaceParent()
{
    MOZ_COUNT_CTOR(TestInterruptShutdownRaceParent);
}

TestInterruptShutdownRaceParent::~TestInterruptShutdownRaceParent()
{
    MOZ_COUNT_DTOR(TestInterruptShutdownRaceParent);
}

void
TestInterruptShutdownRaceParent::Main()
{
    if (!SendStart())
        fail("sending Start");
}

bool
TestInterruptShutdownRaceParent::RecvStartDeath()
{
    
    
    MessageLoop::current()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this,
                          &TestInterruptShutdownRaceParent::StartShuttingDown));
    return true;
}

void
TestInterruptShutdownRaceParent::StartShuttingDown()
{
    
    
    
    
    PR_Sleep(2000);

    if (CallExit())
        fail("connection was supposed to be interrupted");

    Close();

    delete static_cast<TestInterruptShutdownRaceParent*>(gParentActor);
    gParentActor = nullptr;

    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     NewRunnableFunction(DeleteSubprocess));

    
    MessageLoop::current()->PostTask(FROM_HERE,
                                     NewRunnableFunction(Done));

    
}

bool
TestInterruptShutdownRaceParent::RecvOrphan()
{
    
    
    
    
    return true;
}




TestInterruptShutdownRaceChild::TestInterruptShutdownRaceChild()
{
    MOZ_COUNT_CTOR(TestInterruptShutdownRaceChild);
}

TestInterruptShutdownRaceChild::~TestInterruptShutdownRaceChild()
{
    MOZ_COUNT_DTOR(TestInterruptShutdownRaceChild);
}

bool
TestInterruptShutdownRaceChild::RecvStart()
{
    if (!SendStartDeath())
        fail("sending StartDeath");

    
    
    PR_Sleep(1000);

    if (!SendOrphan())
        fail("sending Orphan");

    return true;
}

bool
TestInterruptShutdownRaceChild::AnswerExit()
{
    _exit(0);
    NS_RUNTIMEABORT("unreached");
    return false;
}


} 
} 
