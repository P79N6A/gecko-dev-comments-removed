#include "TestMultiMgrs.h"

#include "IPDLUnitTests.h"      

namespace mozilla {
namespace _ipdltest {




void
TestMultiMgrsParent::Main()
{
    TestMultiMgrsLeftParent* leftie = new TestMultiMgrsLeftParent();
    if (!SendPTestMultiMgrsLeftConstructor(leftie))
        fail("error sending ctor");

    TestMultiMgrsRightParent* rightie = new TestMultiMgrsRightParent();
    if (!SendPTestMultiMgrsRightConstructor(rightie))
        fail("error sending ctor");

    TestMultiMgrsBottomParent* bottomL = new TestMultiMgrsBottomParent();
    if (!leftie->SendPTestMultiMgrsBottomConstructor(bottomL))
        fail("error sending ctor");

    TestMultiMgrsBottomParent* bottomR = new TestMultiMgrsBottomParent();
    if (!rightie->SendPTestMultiMgrsBottomConstructor(bottomR))
        fail("error sending ctor");

    if (!leftie->HasChild(bottomL))
        fail("leftie didn't have a child it was supposed to!");
    if (leftie->HasChild(bottomR))
        fail("leftie had rightie's child!");

    if (!rightie->HasChild(bottomR))
        fail("rightie didn't have a child it was supposed to!");
    if (rightie->HasChild(bottomL))
        fail("rightie had rightie's child!");

    if (!SendCheck())
        fail("couldn't kick off the child-side check");
}

bool
TestMultiMgrsParent::RecvOK()
{
    Close();
    return true;
}




bool
TestMultiMgrsLeftChild::RecvPTestMultiMgrsBottomConstructor(
    PTestMultiMgrsBottomChild* actor)
{
    static_cast<TestMultiMgrsChild*>(Manager())->mBottomL = actor;
    return true;
}

bool
TestMultiMgrsRightChild::RecvPTestMultiMgrsBottomConstructor(
    PTestMultiMgrsBottomChild* actor)
{
    static_cast<TestMultiMgrsChild*>(Manager())->mBottomR = actor;
    return true;
}

bool
TestMultiMgrsChild::RecvCheck()
{
    nsTArray<PTestMultiMgrsLeftChild*> la;
    ManagedPTestMultiMgrsLeftChild(la);
    nsTArray<PTestMultiMgrsRightChild*> lr;
    ManagedPTestMultiMgrsRightChild(lr);

    if (1 != la.Length())
        fail("where's leftie?");
    if (1 != lr.Length())
        fail("where's rightie?");

    TestMultiMgrsLeftChild* leftie =
        static_cast<TestMultiMgrsLeftChild*>(la[0]);
    TestMultiMgrsRightChild* rightie =
        static_cast<TestMultiMgrsRightChild*>(lr[0]);

    if (!leftie->HasChild(mBottomL))
        fail("leftie didn't have a child it was supposed to!");
    if (leftie->HasChild(mBottomR))
        fail("leftie had rightie's child!");

    if (!rightie->HasChild(mBottomR))
        fail("rightie didn't have a child it was supposed to!");
    if (rightie->HasChild(mBottomL))
        fail("rightie had leftie's child!");

    if (!SendOK())
        fail("couldn't send OK()");

    return true;
}


} 
} 
