#ifndef mozilla__ipdltest_TestOpens_h
#define mozilla__ipdltest_TestOpens_h 1

#include "mozilla/_ipdltest/IPDLUnitTests.h"

#include "mozilla/_ipdltest/PTestOpensParent.h"
#include "mozilla/_ipdltest/PTestOpensChild.h"

#include "mozilla/_ipdltest/PTestOpensOpenedParent.h"
#include "mozilla/_ipdltest/PTestOpensOpenedChild.h"

namespace mozilla {
namespace _ipdltest {



class TestOpensParent : public PTestOpensParent
{
public:
    TestOpensParent() {}
    virtual ~TestOpensParent() {}

    static bool RunTestInProcesses() { return true; }
    static bool RunTestInThreads() { return false; }

    void Main();

protected:
    NS_OVERRIDE
    virtual PTestOpensOpenedParent*
    AllocPTestOpensOpened(Transport* transport, ProcessId otherProcess);

    NS_OVERRIDE
    virtual void ActorDestroy(ActorDestroyReason why);
};

class TestOpensOpenedParent : public PTestOpensOpenedParent
{
public:
    TestOpensOpenedParent(Transport* aTransport)
        : mTransport(aTransport)
    {}
    virtual ~TestOpensOpenedParent() {}

protected:
    NS_OVERRIDE
    virtual bool RecvHello();
    NS_OVERRIDE
    virtual bool RecvHelloSync();
    NS_OVERRIDE
    virtual bool AnswerHelloRpc();

    NS_OVERRIDE
    virtual void ActorDestroy(ActorDestroyReason why);

    Transport* mTransport;
};




class TestOpensChild : public PTestOpensChild
{
public:
    TestOpensChild();
    virtual ~TestOpensChild() {}

protected:
    NS_OVERRIDE
    virtual bool RecvStart();

    NS_OVERRIDE
    virtual PTestOpensOpenedChild*
    AllocPTestOpensOpened(Transport* transport, ProcessId otherProcess);

    NS_OVERRIDE
    virtual void ActorDestroy(ActorDestroyReason why);
};

class TestOpensOpenedChild : public PTestOpensOpenedChild
{
public:
    TestOpensOpenedChild(Transport* aTransport)
        : mGotHi(false)
        , mTransport(aTransport)
    {}
    virtual ~TestOpensOpenedChild() {}

protected:
    NS_OVERRIDE
    virtual bool RecvHi();
    NS_OVERRIDE
    virtual bool AnswerHiRpc();

    NS_OVERRIDE
    virtual void ActorDestroy(ActorDestroyReason why);

    bool mGotHi;
    Transport* mTransport;
};


} 
} 


#endif 
