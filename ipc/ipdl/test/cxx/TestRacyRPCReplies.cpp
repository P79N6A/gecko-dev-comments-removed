#include "TestRacyRPCReplies.h"

#include "IPDLUnitTests.h"      

namespace mozilla {
namespace _ipdltest {




TestRacyRPCRepliesParent::TestRacyRPCRepliesParent()
{
    MOZ_COUNT_CTOR(TestRacyRPCRepliesParent);
}

TestRacyRPCRepliesParent::~TestRacyRPCRepliesParent()
{
    MOZ_COUNT_DTOR(TestRacyRPCRepliesParent);
}

void
TestRacyRPCRepliesParent::Main()
{
    int replyNum = -1;
    if (!CallR(&replyNum))
        fail("calling R()");

    if (1 != replyNum)
        fail("this should have been the first reply to R()");

    Close();
}

bool
TestRacyRPCRepliesParent::RecvA()
{
    int replyNum = -1;
    
    
    
    
    
    if (!CallR(&replyNum))
        fail("calling R()");

    if (2 != replyNum)
        fail("this should have been the second reply to R()");

    return true;
}




TestRacyRPCRepliesChild::TestRacyRPCRepliesChild() : mReplyNum(0)
{
    MOZ_COUNT_CTOR(TestRacyRPCRepliesChild);
}

TestRacyRPCRepliesChild::~TestRacyRPCRepliesChild()
{
    MOZ_COUNT_DTOR(TestRacyRPCRepliesChild);
}

bool
TestRacyRPCRepliesChild::AnswerR(int* replyNum)
{
    *replyNum = ++mReplyNum;

    if (1 == *replyNum)
        SendA();

    return true;
}


} 
} 
