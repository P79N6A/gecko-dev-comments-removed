#include "TestRPCShutdownRace.h"

#include "IPDLUnitTests.h"      
#include "IPDLUnitTestSubprocess.h"

template<>
struct RunnableMethodTraits<mozilla::_ipdltest::TestRPCShutdownRaceParent>
{
    static void RetainCallee(mozilla::_ipdltest::TestRPCShutdownRaceParent* obj) { }
    static void ReleaseCallee(mozilla::_ipdltest::TestRPCShutdownRaceParent* obj) { }
};


namespace mozilla {
namespace _ipdltest {




namespace {




void DeleteSubprocess()
{
    delete gSubprocess;
    gSubprocess = NULL;
}

void Done()
{
    passed(__FILE__);
    QuitParent();
}

} 

TestRPCShutdownRaceParent::TestRPCShutdownRaceParent()
{
    MOZ_COUNT_CTOR(TestRPCShutdownRaceParent);
}

TestRPCShutdownRaceParent::~TestRPCShutdownRaceParent()
{
    MOZ_COUNT_DTOR(TestRPCShutdownRaceParent);
}

void
TestRPCShutdownRaceParent::Main()
{
    if (!SendStart())
        fail("sending Start");
}

bool
TestRPCShutdownRaceParent::RecvStartDeath()
{
    
    
    MessageLoop::current()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this,
                          &TestRPCShutdownRaceParent::StartShuttingDown));
    return true;
}

void
TestRPCShutdownRaceParent::StartShuttingDown()
{
    
    
    
    
    PR_Sleep(2000);

    if (CallExit())
        fail("connection was supposed to be interrupted");

    Close();

    delete static_cast<TestRPCShutdownRaceParent*>(gParentActor);
    gParentActor = NULL;

    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     NewRunnableFunction(DeleteSubprocess));

    
    MessageLoop::current()->PostTask(FROM_HERE,
                                     NewRunnableFunction(Done));

    
}

bool
TestRPCShutdownRaceParent::RecvOrphan()
{
    
    
    
    
    return true;
}




TestRPCShutdownRaceChild::TestRPCShutdownRaceChild()
{
    MOZ_COUNT_CTOR(TestRPCShutdownRaceChild);
}

TestRPCShutdownRaceChild::~TestRPCShutdownRaceChild()
{
    MOZ_COUNT_DTOR(TestRPCShutdownRaceChild);
}

bool
TestRPCShutdownRaceChild::RecvStart()
{
    if (!SendStartDeath())
        fail("sending StartDeath");

    if (!SendOrphan())
        fail("sending Orphan");

    return true;
}

bool
TestRPCShutdownRaceChild::AnswerExit()
{
    _exit(0);
    NS_RUNTIMEABORT("unreached");
    return false;
}


} 
} 
