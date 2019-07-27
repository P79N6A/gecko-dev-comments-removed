#include "base/process_util.h"

#include "TestHangs.h"

#include "IPDLUnitTests.h"      

using base::KillProcess;

template<>
struct RunnableMethodTraits<mozilla::_ipdltest::TestHangsParent>
{
    static void RetainCallee(mozilla::_ipdltest::TestHangsParent* obj) { }
    static void ReleaseCallee(mozilla::_ipdltest::TestHangsParent* obj) { }
};

namespace mozilla {
namespace _ipdltest {




TestHangsParent::TestHangsParent() : mDetectedHang(false)
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
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    if (!SendStart())
        fail("sending Start");

    
    
    
    
    
    
    
    PR_Sleep(5000);

    
    
    
    if (CallStackFrame() && mDetectedHang)
        fail("should have timed out!");

    
}

bool
TestHangsParent::ShouldContinueFromReplyTimeout()
{
    mDetectedHang = true;

    
    
    

    PR_Sleep(5000);

    
    
    MessageLoop::current()->PostTask(
        FROM_HERE, NewRunnableMethod(this, &TestHangsParent::CleanUp));

    GetIPCChannel()->CloseWithTimeout();

    return false;
}

bool
TestHangsParent::AnswerStackFrame()
{
    if (PTestHangs::HANG != state()) {
        if (CallStackFrame())
            fail("should have timed out!");
    }
    else {
        
        
        SetReplyTimeoutMs(2);

        if (CallHang())
            fail("should have timed out!");
    }

    return true;
}

void
TestHangsParent::CleanUp()
{
    ipc::ScopedProcessHandle otherProcessHandle;
    if (!base::OpenProcessHandle(OtherPid(), &otherProcessHandle.rwget())) {
        fail("couldn't open child process");
    } else {
        if (!KillProcess(otherProcessHandle, 0, false)) {
            fail("terminating child process");
        }
    }
    Close();
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
    puts(" (child process is 'hanging' now)");

    
    
    
    PR_Sleep(1000);

    return true;
}

} 
} 
