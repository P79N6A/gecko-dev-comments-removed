#ifndef mozilla__ipdltest_TestBridgeMain_h
#define mozilla__ipdltest_TestBridgeMain_h 1

#include "mozilla/_ipdltest/IPDLUnitTests.h"

#include "mozilla/_ipdltest/PTestBridgeMainParent.h"
#include "mozilla/_ipdltest/PTestBridgeMainChild.h"

#include "mozilla/_ipdltest/PTestBridgeSubParent.h"
#include "mozilla/_ipdltest/PTestBridgeSubChild.h"

#include "mozilla/_ipdltest/PTestBridgeMainSubParent.h"
#include "mozilla/_ipdltest/PTestBridgeMainSubChild.h"

namespace mozilla {
namespace _ipdltest {




class TestBridgeMainParent :
    public PTestBridgeMainParent
{
public:
    TestBridgeMainParent() {}
    virtual ~TestBridgeMainParent() {}

    static bool RunTestInProcesses() { return true; }
    static bool RunTestInThreads() { return false; }

    void Main();

protected:
    virtual PTestBridgeMainSubParent*
    AllocPTestBridgeMainSubParent(Transport* transport,
                                  ProcessId otherProcess) override;

    virtual void ActorDestroy(ActorDestroyReason why) override;
};

class TestBridgeMainSubParent :
    public PTestBridgeMainSubParent
{
public:
    explicit TestBridgeMainSubParent(Transport* aTransport)
        : mTransport(aTransport)
    {}
    virtual ~TestBridgeMainSubParent() {}

protected:
    virtual bool RecvHello() override;
    virtual bool RecvHelloSync() override;
    virtual bool AnswerHelloRpc() override;

    virtual void ActorDestroy(ActorDestroyReason why) override;

    Transport* mTransport;
};




class TestBridgeSubParent;

class TestBridgeMainChild :
    public PTestBridgeMainChild
{
public:
    TestBridgeMainChild();
    virtual ~TestBridgeMainChild() {}

protected:
    virtual bool RecvStart() override;

    virtual PTestBridgeMainSubChild*
    AllocPTestBridgeMainSubChild(Transport* transport,
                                 ProcessId otherProcess) override
    {
        
        
        
        MOZ_CRASH();
    }

    virtual void ActorDestroy(ActorDestroyReason why) override;

    IPDLUnitTestSubprocess* mSubprocess;
};

class TestBridgeSubParent :
    public PTestBridgeSubParent
{
public:
    TestBridgeSubParent() {}
    virtual ~TestBridgeSubParent() {}

    void Main();

protected:
    virtual bool RecvBridgeEm() override;

    virtual void ActorDestroy(ActorDestroyReason why) override;
};




class TestBridgeSubChild :
    public PTestBridgeSubChild
{
public:
    TestBridgeSubChild();
    virtual ~TestBridgeSubChild() {}

protected:
    virtual bool RecvPing() override;

    virtual PTestBridgeMainSubChild*
    AllocPTestBridgeMainSubChild(Transport* transport,
                                 ProcessId otherProcess) override;

    virtual void ActorDestroy(ActorDestroyReason why) override;
};

class TestBridgeMainSubChild :
    public PTestBridgeMainSubChild
{
public:
    explicit TestBridgeMainSubChild(Transport* aTransport)
        : mGotHi(false)
        , mTransport(aTransport)
    {}
    virtual ~TestBridgeMainSubChild() {}

protected:
    virtual bool RecvHi() override;
    virtual bool AnswerHiRpc() override;

    virtual void ActorDestroy(ActorDestroyReason why) override;

    bool mGotHi;
    Transport* mTransport;
};

} 
} 


#endif 
