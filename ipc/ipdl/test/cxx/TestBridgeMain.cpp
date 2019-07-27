#include "TestBridgeMain.h"

#include "IPDLUnitTests.h"      
#include "IPDLUnitTestSubprocess.h"

using namespace std;

template<>
struct RunnableMethodTraits<mozilla::_ipdltest::TestBridgeMainSubChild>
{
    static void RetainCallee(mozilla::_ipdltest::TestBridgeMainSubChild* obj) { }
    static void ReleaseCallee(mozilla::_ipdltest::TestBridgeMainSubChild* obj) { }
};

namespace mozilla {
namespace _ipdltest {




void
TestBridgeMainParent::Main()
{
    if (!SendStart())
        fail("sending Start");
}

PTestBridgeMainSubParent*
TestBridgeMainParent::AllocPTestBridgeMainSubParent(Transport* transport,
                                                    ProcessId otherPid)
{
    nsAutoPtr<TestBridgeMainSubParent> a(new TestBridgeMainSubParent(transport));
    if (!a->Open(transport, otherPid, XRE_GetIOMessageLoop(), ipc::ParentSide)) {
        return nullptr;
    }
    return a.forget();
}

void
TestBridgeMainParent::ActorDestroy(ActorDestroyReason why)
{
    if (NormalShutdown != why)
        fail("unexpected destruction!");  
    passed("ok");
    QuitParent();
}

bool
TestBridgeMainSubParent::RecvHello()
{
    return SendHi();
}

bool
TestBridgeMainSubParent::RecvHelloSync()
{
    return true;
}

bool
TestBridgeMainSubParent::AnswerHelloRpc()
{
    return CallHiRpc();
}

void
TestBridgeMainSubParent::ActorDestroy(ActorDestroyReason why)
{
    if (NormalShutdown != why)
        fail("unexpected destruction!");

    
    
    
    MessageLoop::current()->PostTask(
        FROM_HERE,
        new DeleteTask<TestBridgeMainSubParent>(this));
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        new DeleteTask<Transport>(mTransport));
}



TestBridgeMainChild* gBridgeMainChild;

TestBridgeMainChild::TestBridgeMainChild()
    : mSubprocess(nullptr)
{
    gBridgeMainChild = this;
}

bool
TestBridgeMainChild::RecvStart()
{
    vector<string> subsubArgs;
    subsubArgs.push_back("TestBridgeSub");

    mSubprocess = new IPDLUnitTestSubprocess();
    if (!mSubprocess->SyncLaunch(subsubArgs))
        fail("problem launching subprocess");

    IPC::Channel* transport = mSubprocess->GetChannel();
    if (!transport)
        fail("no transport");

    TestBridgeSubParent* bsp = new TestBridgeSubParent();
    bsp->Open(transport, base::GetProcId(mSubprocess->GetChildProcessHandle()));

    bsp->Main();
    return true;
}

void
TestBridgeMainChild::ActorDestroy(ActorDestroyReason why)
{
    if (NormalShutdown != why)
        fail("unexpected destruction!");  
    
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        new DeleteTask<IPDLUnitTestSubprocess>(mSubprocess));
    QuitChild();
}

void
TestBridgeSubParent::Main()
{
    if (!SendPing())
        fail("sending Ping");
}

bool
TestBridgeSubParent::RecvBridgeEm()
{
    if (!PTestBridgeMainSub::Bridge(gBridgeMainChild, this))
        fail("bridging Main and Sub");
    return true;
}

void
TestBridgeSubParent::ActorDestroy(ActorDestroyReason why)
{
    if (NormalShutdown != why)
        fail("unexpected destruction!");
    gBridgeMainChild->Close();

    
    
    
    MessageLoop::current()->PostTask(
        FROM_HERE,
        new DeleteTask<TestBridgeSubParent>(this));
}




static TestBridgeSubChild* gBridgeSubChild;

TestBridgeSubChild::TestBridgeSubChild()
{
    gBridgeSubChild = this;   
}

bool
TestBridgeSubChild::RecvPing()
{
    if (!SendBridgeEm())
        fail("sending BridgeEm");
    return true;
}

PTestBridgeMainSubChild*
TestBridgeSubChild::AllocPTestBridgeMainSubChild(Transport* transport,
                                                 ProcessId otherPid)
{
    nsAutoPtr<TestBridgeMainSubChild> a(new TestBridgeMainSubChild(transport));
    if (!a->Open(transport, otherPid, XRE_GetIOMessageLoop(), ipc::ChildSide)) {
        return nullptr;
    }

    if (!a->SendHello())
        fail("sending Hello");

    return a.forget();
}

void
TestBridgeSubChild::ActorDestroy(ActorDestroyReason why)
{
    if (NormalShutdown != why)
        fail("unexpected destruction!");
    QuitChild();
}

bool
TestBridgeMainSubChild::RecvHi()
{
    if (!SendHelloSync())
        fail("sending HelloSync");
    if (!CallHelloRpc())
        fail("calling HelloRpc");
    if (!mGotHi)
        fail("didn't answer HiRpc");

    
    
    MessageLoop::current()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &TestBridgeMainSubChild::Close));
    return true;
}

bool
TestBridgeMainSubChild::AnswerHiRpc()
{
    mGotHi = true;              
    return true;
}

void
TestBridgeMainSubChild::ActorDestroy(ActorDestroyReason why)
{
    if (NormalShutdown != why)
        fail("unexpected destruction!");  

    gBridgeSubChild->Close();

    
    
    
    MessageLoop::current()->PostTask(
        FROM_HERE,
        new DeleteTask<TestBridgeMainSubChild>(this));
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        new DeleteTask<Transport>(mTransport));
}

} 
} 
