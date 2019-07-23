#include "TestRPCErrorCleanup.h"

#include "IPDLUnitTests.h"      
#include "IPDLUnitTestSubprocess.h"

namespace mozilla {
namespace _ipdltest {







void DeleteTheWorld()
{
    delete static_cast<TestRPCErrorCleanupParent*>(gParentActor);
    gParentActor = NULL;
    delete gSubprocess;
    gSubprocess = NULL;
}

void Done()
{
  static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);
  nsCOMPtr<nsIAppShell> appShell (do_GetService(kAppShellCID));
  appShell->Exit();

  passed(__FILE__);
}

TestRPCErrorCleanupParent::TestRPCErrorCleanupParent()
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

    
    
    Close();

    
    
    
    
    MessageLoop::current()->PostTask(FROM_HERE, NewRunnableFunction(Done));
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
