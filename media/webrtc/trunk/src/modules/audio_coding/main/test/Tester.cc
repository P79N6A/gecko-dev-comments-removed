









#include <stdio.h>
#include <string>
#include <vector>

#include "audio_coding_module.h"
#include "trace.h"

#include "APITest.h"
#include "EncodeDecodeTest.h"
#include "gtest/gtest.h"
#include "iSACTest.h"
#include "SpatialAudio.h"
#include "TestAllCodecs.h"
#include "TestFEC.h"
#include "TestStereo.h"
#include "TestVADDTX.h"
#include "TwoWayCommunication.h"
#include "testsupport/fileutils.h"

using webrtc::AudioCodingModule;
using webrtc::Trace;






#define ACM_AUTO_TEST

                                  









void PopulateTests(std::vector<ACMTest*>* tests)
{

     Trace::CreateTrace();
     std::string trace_file = webrtc::test::OutputPath() + "acm_trace.txt";
     Trace::SetTraceFile(trace_file.c_str());

     printf("The following tests will be executed:\n");
#ifdef ACM_AUTO_TEST
    printf("  ACM auto test\n");
    tests->push_back(new webrtc::EncodeDecodeTest(0));
    tests->push_back(new webrtc::TwoWayCommunication(0));
    tests->push_back(new webrtc::TestAllCodecs(0));
    tests->push_back(new webrtc::TestStereo(0));

    tests->push_back(new webrtc::TestVADDTX(0));
    tests->push_back(new webrtc::TestFEC(0));
    tests->push_back(new webrtc::ISACTest(0));
#endif
#ifdef ACM_TEST_ENC_DEC
    printf("  ACM encode-decode test\n");
    tests->push_back(new webrtc::EncodeDecodeTest(2));
#endif
#ifdef ACM_TEST_TWO_WAY
    printf("  ACM two-way communication test\n");
    tests->push_back(new webrtc::TwoWayCommunication(1));
#endif
#ifdef ACM_TEST_ALL_ENC_DEC
    printf("  ACM all codecs test\n");
    tests->push_back(new webrtc::TestAllCodecs(1));
#endif
#ifdef ACM_TEST_STEREO
    printf("  ACM stereo test\n");
    tests->push_back(new webrtc::TestStereo(1));
    
#endif
#ifdef ACM_TEST_VAD_DTX
    printf("  ACM VAD-DTX test\n");
    tests->push_back(new webrtc::TestVADDTX(1));
#endif
#ifdef ACM_TEST_FEC
    printf("  ACM FEC test\n");
    tests->push_back(new webrtc::TestFEC(1));
#endif
#ifdef ACM_TEST_CODEC_SPEC_API
    printf("  ACM codec API test\n");
    tests->push_back(new webrtc::ISACTest(1));
#endif
#ifdef ACM_TEST_FULL_API
    printf("  ACM full API test\n");
    tests->push_back(new webrtc::APITest());
#endif
    printf("\n");
}



TEST(AudioCodingModuleTest, RunAllTests)
{
    std::vector<ACMTest*> tests;
    PopulateTests(&tests);
    std::vector<ACMTest*>::iterator it;
    for (it=tests.begin() ; it < tests.end(); it++)
    {
        (*it)->Perform();
        delete (*it);
    }

    Trace::ReturnTrace();
    printf("ACM test completed\n");
}
