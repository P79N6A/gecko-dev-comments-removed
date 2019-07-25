#include "base/thread.h"

#include "TestOpens.h"

#include "IPDLUnitTests.h"      

template<>
struct RunnableMethodTraits<mozilla::_ipdltest::TestOpensChild>
{
    static void RetainCallee(mozilla::_ipdltest::TestOpensChild* obj) { }
    static void ReleaseCallee(mozilla::_ipdltest::TestOpensChild* obj) { }
};

template<>
struct RunnableMethodTraits<mozilla::_ipdltest::TestOpensOpenedChild>
{
    static void RetainCallee(mozilla::_ipdltest::TestOpensOpenedChild* obj) { }
    static void ReleaseCallee(mozilla::_ipdltest::TestOpensOpenedChild* obj) { }
};

using namespace base;
using namespace mozilla::ipc;

namespace mozilla {
namespace _ipdltest {

static MessageLoop* gMainThread;

static void
AssertNotMainThread()
{
    if (!gMainThread)
        fail("gMainThread is not initialized");
    if (MessageLoop::current() == gMainThread)
        fail("unexpectedly called on the main thread");
}





static Thread* gParentThread;

void
TestOpensParent::Main()
{
    if (!SendStart())
        fail("sending Start");
}

static void
OpenParent(TestOpensOpenedParent* aParent,
           Transport* aTransport, ProcessHandle aOtherProcess)
{
    AssertNotMainThread();

    
    
    
    if (!aParent->Open(aTransport, aOtherProcess,
                       XRE_GetIOMessageLoop(), AsyncChannel::Parent))
        fail("opening Parent");
}

PTestOpensOpenedParent*
TestOpensParent::AllocPTestOpensOpened(Transport* transport,
                                       ProcessId otherProcess)
{
    gMainThread = MessageLoop::current();

    ProcessHandle h;
    if (!base::OpenProcessHandle(otherProcess, &h)) {
        return nsnull;
    }

    gParentThread = new Thread("ParentThread");
    if (!gParentThread->Start())
        fail("starting parent thread");

    TestOpensOpenedParent* a = new TestOpensOpenedParent(transport);
    gParentThread->message_loop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(OpenParent, a, transport, h));

    return a;
}

void
TestOpensParent::ActorDestroy(ActorDestroyReason why)
{
    
    delete gParentThread;

    if (NormalShutdown != why)
        fail("unexpected destruction!");  
    passed("ok");
    QuitParent();
}

bool
TestOpensOpenedParent::RecvHello()
{
    AssertNotMainThread();
    return SendHi();
}

bool
TestOpensOpenedParent::RecvHelloSync()
{
    AssertNotMainThread();
    return true;
}

bool
TestOpensOpenedParent::AnswerHelloRpc()
{
    AssertNotMainThread();
    return CallHiRpc();
}

void
TestOpensOpenedParent::ActorDestroy(ActorDestroyReason why)
{
    AssertNotMainThread();

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

static Thread* gChildThread;

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

static void
OpenChild(TestOpensOpenedChild* aChild,
           Transport* aTransport, ProcessHandle aOtherProcess)
{
    AssertNotMainThread();

    
    
    
    if (!aChild->Open(aTransport, aOtherProcess,
                      XRE_GetIOMessageLoop(), AsyncChannel::Child))
        fail("opening Child");

    
    if (!aChild->SendHello())
        fail("sending Hello");
}

PTestOpensOpenedChild*
TestOpensChild::AllocPTestOpensOpened(Transport* transport,
                                      ProcessId otherProcess)
{
    gMainThread = MessageLoop::current();

    ProcessHandle h;
    if (!base::OpenProcessHandle(otherProcess, &h)) {
        return nsnull;
    }

    gChildThread = new Thread("ChildThread");
    if (!gChildThread->Start())
        fail("starting child thread");

    TestOpensOpenedChild* a = new TestOpensOpenedChild(transport);
    gChildThread->message_loop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(OpenChild, a, transport, h));

    return a;
}

void
TestOpensChild::ActorDestroy(ActorDestroyReason why)
{
    
    delete gChildThread;

    if (NormalShutdown != why)
        fail("unexpected destruction!");
    QuitChild();
}

bool
TestOpensOpenedChild::RecvHi()
{
    AssertNotMainThread();

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
    AssertNotMainThread();

    mGotHi = true;              
    return true;
}

void
TestOpensOpenedChild::ActorDestroy(ActorDestroyReason why)
{
    AssertNotMainThread();

    if (NormalShutdown != why)
        fail("unexpected destruction!");  

    
    
    
    MessageLoop::current()->PostTask(
        FROM_HERE,
        new DeleteTask<TestOpensOpenedChild>(this));
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        new DeleteTask<Transport>(mTransport));

    
    gMainThread->PostTask(
        FROM_HERE,
        NewRunnableMethod(gOpensChild, &TestOpensChild::Close));
}

} 
} 
