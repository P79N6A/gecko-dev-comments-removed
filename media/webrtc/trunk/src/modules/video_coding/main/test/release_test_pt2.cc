









#include "ReleaseTest.h"
#include "ReceiverTests.h"
#include "TestMacros.h"
#include "MediaOptTest.h"
#include "CodecDataBaseTest.h"
#include "GenericCodecTest.h"




int ReleaseTestPart2()
{
    printf("Verify that TICK_TIME_DEBUG and EVENT_DEBUG are uncommented");
    

    printf("Testing Generic Codecs...\n");
    TEST(VCMGenericCodecTest() == 0);
    printf("Verify by viewing output file GCTest_out.yuv \n");
    
    return 0;
}