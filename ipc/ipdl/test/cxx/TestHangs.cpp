#include "base/process_util.h"

#include "TestHangs.h"

#include "IPDLUnitTests.h"      

using base::KillProcess;


static const int kTimeoutSecs = 5;

namespace mozilla {
namespace _ipdltest {




TestHangsParent::TestHangsParent() : mFramesToGo(2)
{
    MOZ_COUNT_CTOR(TestHangsParent);
}

TestHangsParent::~TestHangsParent()
{
    MOZ_COUNT_DTOR(TestHangsParent);
}

void
TestHangsParent::Main()
{
    SetReplyTimeoutMs(1000 * kTimeoutSecs);

    if (CallStackFrame())
        fail("should have timed out!");

    Close();
}

bool
TestHangsParent::ShouldContinueFromReplyTimeout()
{
    
    
    
    
    

    
    
    if (!KillProcess(OtherProcess(), 0, false))
        fail("terminating child process");

    return false;
}

bool
TestHangsParent::AnswerStackFrame()
{
    if (--mFramesToGo) {
        if (CallStackFrame())
            fail("should have timed out!");
    }
    else {
        if (CallHang())
            fail("should have timed out!");
    }

    return true;
}





TestHangsChild::TestHangsChild()
{
    MOZ_COUNT_CTOR(TestHangsChild);
}

TestHangsChild::~TestHangsChild()
{
    MOZ_COUNT_DTOR(TestHangsChild);
}

bool
TestHangsChild::AnswerHang()
{
    puts(" (child process is hanging now)");

    
    
    PR_Sleep(PR_SecondsToInterval(100000));

    fail("should have been killed!");
    return false;               
}

} 
} 
