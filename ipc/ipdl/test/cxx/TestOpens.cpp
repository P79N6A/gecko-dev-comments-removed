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
struct RunnableMethodTraits<mozilla::_ipdltest2::TestOpensOpenedChild>
{
    static void RetainCallee(mozilla::_ipdltest2::TestOpensOpenedChild* obj) { }
    static void ReleaseCallee(mozilla::_ipdltest2::TestOpensOpenedChild* obj) { }
};

using namespace mozilla::ipc;

using base::ProcessHandle;
using base::Thread;

namespace mozilla {

using namespace _ipdltest;
using namespace _ipdltest2;

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
           Transport* aTransport, base::ProcessId aOtherPid)
{
    AssertNotMainThread();

    
    
    
    if (!aParent->Open(aTransport, aOtherPid,
                       XRE_GetIOMessageLoop(), ipc::ParentSide))
        fail("opening Parent");
}

PTestOpensOpenedParent*
TestOpensParent::AllocPTestOpensOpenedParent(Transport* transport,
                                             ProcessId otherPid)
{
    gMainThread = MessageLoop::current();

    gParentThread = new Thread("ParentThread");
    if (!gParentThread->Start())
        fail("starting parent thread");

    TestOpensOpenedParent* a = new TestOpensOpenedParent(transport);
    gParentThread->message_loop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(OpenParent, a, transport, otherPid));

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

static void
ShutdownTestOpensOpenedParent(TestOpensOpenedParent* parent,
                              Transport* transport)
{
    delete parent;

    
    
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        new DeleteTask<Transport>(transport));
}

void
TestOpensOpenedParent::ActorDestroy(ActorDestroyReason why)
{
    AssertNotMainThread();

    if (NormalShutdown != why)
        fail("unexpected destruction!");

    
    
    
    gMainThread->PostTask(
        FROM_HERE,
        NewRunnableFunction(ShutdownTestOpensOpenedParent,
                            this, mTransport));
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
           Transport* aTransport, base::ProcessId aOtherPid)
{
    AssertNotMainThread();

    
    
    
    if (!aChild->Open(aTransport, aOtherPid,
                      XRE_GetIOMessageLoop(), ipc::ChildSide))
        fail("opening Child");

    
    if (!aChild->SendHello())
        fail("sending Hello");
}

PTestOpensOpenedChild*
TestOpensChild::AllocPTestOpensOpenedChild(Transport* transport,
                                           ProcessId otherPid)
{
    gMainThread = MessageLoop::current();

    gChildThread = new Thread("ChildThread");
    if (!gChildThread->Start())
        fail("starting child thread");

    TestOpensOpenedChild* a = new TestOpensOpenedChild(transport);
    gChildThread->message_loop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(OpenChild, a, transport, otherPid));

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

static void
ShutdownTestOpensOpenedChild(TestOpensOpenedChild* child,
                             Transport* transport)
{
    delete child;

    
    
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        new DeleteTask<Transport>(transport));

    
    gMainThread->PostTask(
        FROM_HERE,
        NewRunnableMethod(gOpensChild, &TestOpensChild::Close));
}

void
TestOpensOpenedChild::ActorDestroy(ActorDestroyReason why)
{
    AssertNotMainThread();

    if (NormalShutdown != why)
        fail("unexpected destruction!");  

    
    
    
    
    gMainThread->PostTask(
        FROM_HERE,
        NewRunnableFunction(ShutdownTestOpensOpenedChild,
                            this, mTransport));
}

} 
