









#include <stdio.h>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "APITest.h"
#include "audio_coding_module.h"
#include "EncodeDecodeTest.h"
#include "iSACTest.h"
#include "TestAllCodecs.h"
#include "TestFEC.h"
#include "TestStereo.h"
#include "testsupport/fileutils.h"
#include "TestVADDTX.h"
#include "trace.h"
#include "TwoWayCommunication.h"

using webrtc::AudioCodingModule;
using webrtc::Trace;




#define ACM_TEST_MODE 1


















#define ACM_AUTO_TEST










#ifdef ACM_AUTO_TEST
#undef ACM_TEST_MODE
#define ACM_TEST_MODE 0
#ifndef ACM_TEST_ALL_CODECS
#define ACM_TEST_ALL_CODECS
#endif
#endif

void PopulateTests(std::vector<ACMTest*>* tests) {
  Trace::CreateTrace();
  Trace::SetTraceFile((webrtc::test::OutputPath() + "acm_trace.txt").c_str());

  printf("The following tests will be executed:\n");
#ifdef ACM_AUTO_TEST
  printf("  ACM auto test\n");
  tests->push_back(new webrtc::EncodeDecodeTest(0));
  tests->push_back(new webrtc::TwoWayCommunication(0));
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




#ifdef ACM_TEST_ALL_CODECS
TEST(AudioCodingModuleTest, TestAllCodecs) {
  Trace::CreateTrace();
  Trace::SetTraceFile((webrtc::test::OutputPath() +
      "acm_allcodecs_trace.txt").c_str());
  webrtc::TestAllCodecs(ACM_TEST_MODE).Perform();
  Trace::ReturnTrace();
}
#endif

TEST(AudioCodingModuleTest, RunAllTests) {
  std::vector<ACMTest*> tests;
  PopulateTests(&tests);
  std::vector<ACMTest*>::iterator it;
  for (it = tests.begin(); it < tests.end(); it++) {
    (*it)->Perform();
    delete (*it);
  }

  Trace::ReturnTrace();
  printf("ACM test completed\n");
}
