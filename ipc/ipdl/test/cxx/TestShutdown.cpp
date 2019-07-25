#include "TestShutdown.h"

namespace mozilla {
namespace _ipdltest {



void
TestShutdownParent::Main()
{
    if (!SendStart())
        fail("sending Start()");
}

void
TestShutdownParent::ActorDestroy(ActorDestroyReason why)
{
    if (AbnormalShutdown != why)
        fail("should have ended test with crash!");

    passed("ok");

    QuitParent();
}

void
TestShutdownSubParent::ActorDestroy(ActorDestroyReason why)
{
    if (Manager()->ManagedPTestShutdownSubParent().Length() == 0)
        fail("manager should still have managees!");

    if (mExpectCrash && AbnormalShutdown != why)
        fail("expected crash!");
    else if (!mExpectCrash && AbnormalShutdown == why)
        fail("wasn't expecting crash!");

    if (mExpectCrash && 0 == ManagedPTestShutdownSubsubParent().Length())
        fail("expected to *still* have kids");
}

void
TestShutdownSubsubParent::ActorDestroy(ActorDestroyReason why)
{
    if (Manager()->ManagedPTestShutdownSubsubParent().Length() == 0)
        fail("manager should still have managees!");

    if (mExpectParentDeleted && AncestorDeletion != why)
        fail("expected ParentDeleted == why");
    else if (!mExpectParentDeleted && AncestorDeletion == why)
        fail("wasn't expecting parent delete");
}




bool
TestShutdownChild::RecvStart()
{
    
    
    {
        bool expectCrash = false, expectParentDeleted = false;

        PTestShutdownSubChild* c1 =
            SendPTestShutdownSubConstructor(expectCrash);
        if (!c1)
            fail("problem sending ctor");

        PTestShutdownSubChild* c2 =
            SendPTestShutdownSubConstructor(expectCrash);
        if (!c2)
            fail("problem sending ctor");

        PTestShutdownSubsubChild* c1s1 =
            c1->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c1s1)
            fail("problem sending ctor");
        PTestShutdownSubsubChild* c1s2 =
            c1->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c1s2)
            fail("problem sending ctor");

        PTestShutdownSubsubChild* c2s1 =
            c2->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c2s1)
            fail("problem sending ctor");
        PTestShutdownSubsubChild* c2s2 =
            c2->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c2s2)
            fail("problem sending ctor");

        if (!PTestShutdownSubsubChild::Send__delete__(c1s1))
            fail("problem sending dtor");
        if (!PTestShutdownSubsubChild::Send__delete__(c1s2))
            fail("problem sending dtor");
        if (!PTestShutdownSubsubChild::Send__delete__(c2s1))
            fail("problem sending dtor");
        if (!PTestShutdownSubsubChild::Send__delete__(c2s2))
            fail("problem sending dtor");

        if (!c1->CallStackFrame())
            fail("problem creating dummy stack frame");
        if (!c2->CallStackFrame())
            fail("problem creating dummy stack frame");
    }

    
    {
        bool expectCrash = false, expectParentDeleted = true;

        PTestShutdownSubChild* c1 =
            SendPTestShutdownSubConstructor(expectCrash);
        if (!c1)
            fail("problem sending ctor");

        PTestShutdownSubChild* c2 =
            SendPTestShutdownSubConstructor(expectCrash);
        if (!c2)
            fail("problem sending ctor");

        PTestShutdownSubsubChild* c1s1 =
            c1->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c1s1)
            fail("problem sending ctor");
        PTestShutdownSubsubChild* c1s2 =
            c1->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c1s2)
            fail("problem sending ctor");

        PTestShutdownSubsubChild* c2s1 =
            c2->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c2s1)
            fail("problem sending ctor");
        PTestShutdownSubsubChild* c2s2 =
            c2->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c2s2)
            fail("problem sending ctor");

        
        if (!c1->CallStackFrame())
            fail("problem creating dummy stack frame");
        if (!c2->CallStackFrame())
            fail("problem creating dummy stack frame");
    }

    
    {
        bool expectCrash = true, expectParentDeleted = false;

        PTestShutdownSubChild* c1 =
            SendPTestShutdownSubConstructor(expectCrash);
        if (!c1)
            fail("problem sending ctor");

        PTestShutdownSubChild* c2 =
            SendPTestShutdownSubConstructor(expectCrash);
        if (!c2)
            fail("problem sending ctor");

        PTestShutdownSubsubChild* c1s1 =
            c1->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c1s1)
            fail("problem sending ctor");
        PTestShutdownSubsubChild* c1s2 =
            c1->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c1s2)
            fail("problem sending ctor");

        PTestShutdownSubsubChild* c2s1 =
            c2->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c2s1)
            fail("problem sending ctor");
        PTestShutdownSubsubChild* c2s2 =
            c2->SendPTestShutdownSubsubConstructor(expectParentDeleted);
        if (!c2s2)
            fail("problem sending ctor");

        
        
        if (!SendSync())
            fail("can't synchronize with parent");

        
        
        _exit(0);
    }
}

void
TestShutdownChild::ActorDestroy(ActorDestroyReason why)
{
    fail("hey wait ... we should have crashed!");
}

bool
TestShutdownSubChild::AnswerStackFrame()
{
    if (!PTestShutdownSubChild::Send__delete__(this))
        fail("problem sending dtor");

    

    return true;
}

void
TestShutdownSubChild::ActorDestroy(ActorDestroyReason why)
{
    if (Manager()->ManagedPTestShutdownSubChild().Length() == 0)
        fail("manager should still have managees!");

    if (mExpectCrash && AbnormalShutdown != why)
        fail("expected crash!");
    else if (!mExpectCrash && AbnormalShutdown == why)
        fail("wasn't expecting crash!");

    if (mExpectCrash && 0 == ManagedPTestShutdownSubsubChild().Length())
        fail("expected to *still* have kids");
}

void
TestShutdownSubsubChild::ActorDestroy(ActorDestroyReason why)
{
    if (Manager()->ManagedPTestShutdownSubsubChild().Length() == 0)
        fail("manager should still have managees!");

    if (mExpectParentDeleted && AncestorDeletion != why)
        fail("expected ParentDeleted == why");
    else if (!mExpectParentDeleted && AncestorDeletion == why)
        fail("wasn't expecting parent delete");
}


} 
} 
