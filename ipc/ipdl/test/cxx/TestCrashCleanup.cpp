#include "TestCrashCleanup.h"

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
    delete static_cast<TestCrashCleanupParent*>(gParentActor);
    gParentActor = NULL;

    
    
    Mutex mutex("TestCrashCleanup.DeleteTheWorld.mutex");
    CondVar cvar(mutex, "TestCrashCleanup.DeleteTheWorld.cvar");

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

TestCrashCleanupParent::TestCrashCleanupParent() : mCleanedUp(false)
{
    MOZ_COUNT_CTOR(TestCrashCleanupParent);
}

TestCrashCleanupParent::~TestCrashCleanupParent()
{
    MOZ_COUNT_DTOR(TestCrashCleanupParent);

    if (!mCleanedUp)
        fail("should have been ActorDestroy()d!");
}

void
TestCrashCleanupParent::Main()
{
    
    MessageLoop::current()->PostTask(
        FROM_HERE, NewRunnableFunction(DeleteTheWorld));

    if (CallDIEDIEDIE())
        fail("expected an error!");

    Close();

    MessageLoop::current()->PostTask(FROM_HERE, NewRunnableFunction(Done));
}





TestCrashCleanupChild::TestCrashCleanupChild()
{
    MOZ_COUNT_CTOR(TestCrashCleanupChild);
}

TestCrashCleanupChild::~TestCrashCleanupChild()
{
    MOZ_COUNT_DTOR(TestCrashCleanupChild);
}

bool
TestCrashCleanupChild::AnswerDIEDIEDIE()
{
    _exit(0);
    NS_RUNTIMEABORT("unreached");
    return false;
}


} 
} 
