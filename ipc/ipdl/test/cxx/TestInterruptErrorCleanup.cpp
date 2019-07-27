#include "TestInterruptErrorCleanup.h"

#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"

#include "IPDLUnitTests.h"      
#include "IPDLUnitTestSubprocess.h"

using mozilla::CondVar;
using mozilla::Mutex;
using mozilla::MutexAutoLock;

namespace mozilla {
namespace _ipdltest {




namespace {




void DeleteSubprocess(Mutex* mutex, CondVar* cvar)
{
    MutexAutoLock lock(*mutex);

    delete gSubprocess;
    gSubprocess = nullptr;

    cvar->Notify();
}

void DeleteTheWorld()
{
    delete static_cast<TestInterruptErrorCleanupParent*>(gParentActor);
    gParentActor = nullptr;

    
    
    Mutex mutex("TestInterruptErrorCleanup.DeleteTheWorld.mutex");
    CondVar cvar(mutex, "TestInterruptErrorCleanup.DeleteTheWorld.cvar");

    MutexAutoLock lock(mutex);

    XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(DeleteSubprocess, &mutex, &cvar));

    cvar.Wait();
}

void Done()
{
  static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);
  nsCOMPtr<nsIAppShell> appShell (do_GetService(kAppShellCID));
  appShell->Exit();

  passed(__FILE__);
}

} 

TestInterruptErrorCleanupParent::TestInterruptErrorCleanupParent()
    : mGotProcessingError(false)
{
    MOZ_COUNT_CTOR(TestInterruptErrorCleanupParent);
}

TestInterruptErrorCleanupParent::~TestInterruptErrorCleanupParent()
{
    MOZ_COUNT_DTOR(TestInterruptErrorCleanupParent);
}

void
TestInterruptErrorCleanupParent::Main()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    MessageLoop::current()->PostTask(
        FROM_HERE, NewRunnableFunction(DeleteTheWorld));

    
    if (CallError())
        fail("expected an error!");

    if (!mGotProcessingError)
        fail("expected a ProcessingError() notification");

    
    
    Close();

    
    
    
    
    MessageLoop::current()->PostTask(FROM_HERE, NewRunnableFunction(Done));
}

void
TestInterruptErrorCleanupParent::ProcessingError(Result aCode, const char* aReason)
{
    if (aCode != MsgDropped)
        fail("unexpected processing error");
    mGotProcessingError = true;
}




TestInterruptErrorCleanupChild::TestInterruptErrorCleanupChild()
{
    MOZ_COUNT_CTOR(TestInterruptErrorCleanupChild);
}

TestInterruptErrorCleanupChild::~TestInterruptErrorCleanupChild()
{
    MOZ_COUNT_DTOR(TestInterruptErrorCleanupChild);
}

bool
TestInterruptErrorCleanupChild::AnswerError()
{
    _exit(0);
    NS_RUNTIMEABORT("unreached");
    return false;
}


} 
} 
