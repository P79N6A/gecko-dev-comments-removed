#include "TestRPCRaces.h"

#include "IPDLUnitTests.h"      

using mozilla::ipc::MessageChannel;

template<>
struct RunnableMethodTraits<mozilla::_ipdltest::TestRPCRacesParent>
{
    static void RetainCallee(mozilla::_ipdltest::TestRPCRacesParent* obj) { }
    static void ReleaseCallee(mozilla::_ipdltest::TestRPCRacesParent* obj) { }
};


namespace mozilla {
namespace _ipdltest {

ipc::RacyRPCPolicy
MediateRace(const MessageChannel::Message& parent,
            const MessageChannel::Message& child)
{
    return (PTestRPCRaces::Msg_Child__ID == parent.type()) ?
        ipc::RRPParentWins : ipc::RRPChildWins;
}



void
TestRPCRacesParent::Main()
{
    if (!SendStart())
        fail("sending Start()");
}

bool
TestRPCRacesParent::RecvStartRace()
{
    MessageLoop::current()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &TestRPCRacesParent::OnRaceTime));
    return true;
}

void
TestRPCRacesParent::OnRaceTime()
{
    if (!CallRace(&mChildHasReply))
        fail("problem calling Race()");

    if (!mChildHasReply)
        fail("child should have got a reply already");

    mHasReply = true;

    MessageLoop::current()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &TestRPCRacesParent::Test2));
}

bool
TestRPCRacesParent::AnswerRace(bool* hasReply)
{
    if (mHasReply)
        fail("apparently the parent won the RPC race!");
    *hasReply = hasReply;
    return true;
}

void
TestRPCRacesParent::Test2()
{
    puts("  passed");
    puts("Test 2");

    mHasReply = false;
    mChildHasReply = false;

    if (!CallStackFrame())
        fail("can't set up a stack frame");

    puts("  passed");

    MessageLoop::current()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &TestRPCRacesParent::Test3));
}

bool
TestRPCRacesParent::AnswerStackFrame()
{
    if (!SendWakeup())
        fail("can't wake up the child");

    if (!CallRace(&mChildHasReply))
        fail("can't set up race condition");
    mHasReply = true;

    if (!mChildHasReply)
        fail("child should have got a reply already");

    return true;
}

void
TestRPCRacesParent::Test3()
{
    puts("Test 3");

    if (!CallStackFrame3())
        fail("can't set up a stack frame");

    puts("  passed");

    Close();
}

bool
TestRPCRacesParent::AnswerStackFrame3()
{
    if (!SendWakeup3())
        fail("can't wake up the child");

    if (!CallChild())
        fail("can't set up race condition");

    return true;
}

bool
TestRPCRacesParent::AnswerParent()
{
    mAnsweredParent = true;
    return true;
}

bool
TestRPCRacesParent::RecvGetAnsweredParent(bool* answeredParent)
{
    *answeredParent = mAnsweredParent;
    return true;
}



bool
TestRPCRacesChild::RecvStart()
{
    puts("Test 1");

    if (!SendStartRace())
        fail("problem sending StartRace()");

    bool dontcare;
    if (!CallRace(&dontcare))
        fail("problem calling Race()");

    mHasReply = true;
    return true;
}

bool
TestRPCRacesChild::AnswerRace(bool* hasReply)
{
    if (!mHasReply)
        fail("apparently the child lost the RPC race!");

    *hasReply = mHasReply;

    return true;
}

bool
TestRPCRacesChild::AnswerStackFrame()
{
    
    mHasReply = false;

    if (!CallStackFrame())
        fail("can't set up stack frame");

    if (!mHasReply)
        fail("should have had reply by now");

    return true;
}

bool
TestRPCRacesChild::RecvWakeup()
{
    bool dontcare;
    if (!CallRace(&dontcare))
        fail("can't set up race condition");

    mHasReply = true;
    return true;
}

bool
TestRPCRacesChild::AnswerStackFrame3()
{
    if (!CallStackFrame3())
        fail("can't set up stack frame");
    return true;
}

bool
TestRPCRacesChild::RecvWakeup3()
{
    if (!CallParent())
        fail("can't set up race condition");
    return true;
}

bool
TestRPCRacesChild::AnswerChild()
{
    bool parentAnsweredParent;
    
    
    
    if (!SendGetAnsweredParent(&parentAnsweredParent))
        fail("sending GetAnsweredParent");

    if (parentAnsweredParent)
        fail("parent was supposed to win the race!");

    return true;
}

} 
} 
