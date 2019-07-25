#include "TestRPCErrorCleanup.h"

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
    gSubprocess = NULL;

    cvar->Notify();
}

void DeleteTheWorld()
{
    delete static_cast<TestRPCErrorCleanupParent*>(gParentActor);
    gParentActor = NULL;

    
    
    Mutex mutex("TestRPCErrorCleanup.DeleteTheWorld.mutex");
    CondVar cvar(mutex, "TestRPCErrorCleanup.DeleteTheWorld.cvar");

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

TestRPCErrorCleanupParent::TestRPCErrorCleanupParent()
    : mGotProcessingError(false)
{
    MOZ_COUNT_CTOR(TestRPCErrorCleanupParent);
}

TestRPCErrorCleanupParent::~TestRPCErrorCleanupParent()
{
    MOZ_COUNT_DTOR(TestRPCErrorCleanupParent);
}

void
TestRPCErrorCleanupParent::Main()
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
TestRPCErrorCleanupParent::ProcessingError(Result what)
{
    if (what != MsgDropped)
        fail("unexpected processing error");
    mGotProcessingError = true;
}




TestRPCErrorCleanupChild::TestRPCErrorCleanupChild()
{
    MOZ_COUNT_CTOR(TestRPCErrorCleanupChild);
}

TestRPCErrorCleanupChild::~TestRPCErrorCleanupChild()
{
    MOZ_COUNT_DTOR(TestRPCErrorCleanupChild);
}

bool
TestRPCErrorCleanupChild::AnswerError()
{
    _exit(0);
    NS_RUNTIMEABORT("unreached");
    return false;
}


} 
} 
