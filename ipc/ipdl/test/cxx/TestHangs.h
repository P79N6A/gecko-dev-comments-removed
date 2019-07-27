#ifndef mozilla__ipdltest_TestHangs_h
#define mozilla__ipdltest_TestHangs_h 1

#include "mozilla/_ipdltest/IPDLUnitTests.h"

#include "mozilla/_ipdltest/PTestHangsParent.h"
#include "mozilla/_ipdltest/PTestHangsChild.h"

namespace mozilla {
namespace _ipdltest {


class TestHangsParent :
    public PTestHangsParent
{
public:
    TestHangsParent();
    virtual ~TestHangsParent();

    static bool RunTestInProcesses() { return true; }

    
    
    static bool RunTestInThreads() { return false; }

    void Main();

protected:
    virtual bool ShouldContinueFromReplyTimeout() override;

    virtual bool RecvNonce() {
        return true;
    }

    virtual bool AnswerStackFrame() override;

    virtual void ActorDestroy(ActorDestroyReason why) override
    {
        if (AbnormalShutdown != why)
            fail("unexpected destruction!");  
        passed("ok");
        QuitParent();
    }

    void CleanUp();

    bool mDetectedHang;
};


class TestHangsChild :
    public PTestHangsChild
{
public:
    TestHangsChild();
    virtual ~TestHangsChild();

protected:
    virtual bool RecvStart() override {
        if (!SendNonce())
            fail("sending Nonce");
        return true;
    }

    virtual bool AnswerStackFrame() override
    {
        if (CallStackFrame())
            fail("should have failed");
        return true;
    }

    virtual bool AnswerHang() override;

    virtual void ActorDestroy(ActorDestroyReason why) override
    {
        if (AbnormalShutdown != why)
            fail("unexpected destruction!");
        QuitChild();
    }
};


} 
} 


#endif 
