









#include "ReleaseTest.h"
#include "ReceiverTests.h"
#include "TestMacros.h"
#include "MediaOptTest.h"
#include "CodecDataBaseTest.h"
#include "GenericCodecTest.h"




int ReleaseTest()
{
    printf("VCM RELEASE TESTS \n\n");
    
    

    printf("Testing receive side timing...\n");
    TEST(ReceiverTimingTests() == 0);
    
    printf("Testing jitter buffer...\n");
    TEST(JitterBufferTest() == 0);
    
    printf("Testing Codec Data Base...\n");
    TEST(CodecDBTest() == 0);
    
    printf("Testing Media Optimization....\n");
    TEST(VCMMediaOptTest(1) == 0); 

    
    
    printf("Testing Multi thread send-receive....\n");
    TEST(MTRxTxTest() == 0);
    printf("Verify by viewing output file MTRxTx_out.yuv \n");
    
    return 0;
}