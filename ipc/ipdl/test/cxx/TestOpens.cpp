#include "TestOpens.h"

#include "IPDLUnitTests.h"      

template<>
struct RunnableMethodTraits<mozilla::_ipdltest::TestOpensOpenedChild>
{
    static void RetainCallee(mozilla::_ipdltest::TestOpensOpenedChild* obj) { }
    static void ReleaseCallee(mozilla::_ipdltest::TestOpensOpenedChild* obj) { }
};

namespace mozilla {
namespace _ipdltest {




void
TestOpensParent::Main()
{
    if (!SendStart())
        fail("sending Start");
}

PTestOpensOpenedParent*
TestOpensParent::AllocPTestOpensOpened(Transport* transport,
                                       ProcessId otherProcess)
{
    ProcessHandle h;
    if (!base::OpenProcessHandle(otherProcess, &h)) {
        return nsnull;
    }

    nsAutoPtr<TestOpensOpenedParent> a(new TestOpensOpenedParent(transport));
    if (!a->Open(transport, h, XRE_GetIOMessageLoop(), AsyncChannel::Parent)) {
        return nsnull;
    }
    return a.forget();
}

void
TestOpensParent::ActorDestroy(ActorDestroyReason why)
{
    if (NormalShutdown != why)
        fail("unexpected destruction!");  
    passed("ok");
    QuitParent();
}

bool
TestOpensOpenedParent::RecvHello()
{
    return SendHi();
}

bool
TestOpensOpenedParent::RecvHelloSync()
{
    return true;
}

bool
TestOpensOpenedParent::AnswerHelloRpc()
{
    return CallHiRpc();
}

void
TestOpensOpenedParent::ActorDestroy(ActorDestroyReason why)
{
    if (NormalShutdown != why)
        fail("unexpected destruction!");

    
    
    
    MessageLoop::current()->PostTask(
        FROM_HERE,
        new DeleteTask<TestOpensOpenedParent>(this));
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        new DeleteTask<Transport>(mTransport));
}




static TestOpensChild* gOpensChild;

TestOpensChild::TestOpensChild()
{
    gOpensChild = this;
}

bool
TestOpensChild::RecvStart()
{
    if (!PTestOpensOpened::Open(this))
        fail("opening PTestOpensOpened");
    return true;
}

PTestOpensOpenedChild*
TestOpensChild::AllocPTestOpensOpened(Transport* transport,
                                      ProcessId otherProcess)
{
    ProcessHandle h;
    if (!base::OpenProcessHandle(otherProcess, &h)) {
        return nsnull;
    }

    nsAutoPtr<TestOpensOpenedChild> a(new TestOpensOpenedChild(transport));
    if (!a->Open(transport, h, XRE_GetIOMessageLoop(), AsyncChannel::Child)) {
        return nsnull;
    }

    if (!a->SendHello())
        fail("sending Hello");

    return a.forget();
}

void
TestOpensChild::ActorDestroy(ActorDestroyReason why)
{
    if (NormalShutdown != why)
        fail("unexpected destruction!");
    QuitChild();
}

bool
TestOpensOpenedChild::RecvHi()
{
    if (!SendHelloSync())
        fail("sending HelloSync");
    if (!CallHelloRpc())
        fail("calling HelloRpc");
    if (!mGotHi)
        fail("didn't answer HiRpc");

    
    
    MessageLoop::current()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &TestOpensOpenedChild::Close));
    return true;
}

bool
TestOpensOpenedChild::AnswerHiRpc()
{
    mGotHi = true;              
    return true;
}

void
TestOpensOpenedChild::ActorDestroy(ActorDestroyReason why)
{
    if (NormalShutdown != why)
        fail("unexpected destruction!");  

    gOpensChild->Close();

    
    
    
    MessageLoop::current()->PostTask(
        FROM_HERE,
        new DeleteTask<TestOpensOpenedChild>(this));
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        new DeleteTask<Transport>(mTransport));
}

} 
} 
