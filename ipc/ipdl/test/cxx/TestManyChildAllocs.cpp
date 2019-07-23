#include "TestManyChildAllocs.h"

#include "IPDLUnitTests.h"      


#define NALLOCS 10

namespace mozilla {
namespace _ipdltest {



TestManyChildAllocsParent::TestManyChildAllocsParent()
{
    MOZ_COUNT_CTOR(TestManyChildAllocsParent);
}

TestManyChildAllocsParent::~TestManyChildAllocsParent()
{
    MOZ_COUNT_DTOR(TestManyChildAllocsParent);
}

void
TestManyChildAllocsParent::Main()
{
    if (!SendGo())
        fail("can't send Go()");
}

bool
TestManyChildAllocsParent::RecvDone()
{
    
    
    Close();

    return true;
}

bool
TestManyChildAllocsParent::DeallocPTestManyChildAllocsSub(
    PTestManyChildAllocsSubParent* __a)
{
    delete __a; return true;
}

PTestManyChildAllocsSubParent*
TestManyChildAllocsParent::AllocPTestManyChildAllocsSub()
{
    return new TestManyChildAllocsSubParent();
}




TestManyChildAllocsChild::TestManyChildAllocsChild()
{
    MOZ_COUNT_CTOR(TestManyChildAllocsChild);
}

TestManyChildAllocsChild::~TestManyChildAllocsChild()
{
    MOZ_COUNT_DTOR(TestManyChildAllocsChild);
}

bool TestManyChildAllocsChild::RecvGo()
{
    for (int i = 0; i < NALLOCS; ++i) {
        PTestManyChildAllocsSubChild* child =
            SendPTestManyChildAllocsSubConstructor();

        if (!child)
            fail("can't send ctor()");

        if (!child->SendHello())
            fail("can't send Hello()");
    }

    size_t len = ManagedPTestManyChildAllocsSubChild().Length();
    if (NALLOCS != len)
        fail("expected %lu kids, got %lu", NALLOCS, len);

    if (!SendDone())
        fail("can't send Done()");

    return true;
}

bool
TestManyChildAllocsChild::DeallocPTestManyChildAllocsSub(
    PTestManyChildAllocsSubChild* __a)
{
    delete __a; return true;
}

PTestManyChildAllocsSubChild*
TestManyChildAllocsChild::AllocPTestManyChildAllocsSub()
{
    return new TestManyChildAllocsSubChild();
}


} 
} 
