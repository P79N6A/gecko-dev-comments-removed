









#include <stdio.h>
#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/main/test/APITest.h"
#include "webrtc/modules/audio_coding/main/test/EncodeDecodeTest.h"
#include "webrtc/modules/audio_coding/main/test/iSACTest.h"
#include "webrtc/modules/audio_coding/main/test/opus_test.h"
#include "webrtc/modules/audio_coding/main/test/TestAllCodecs.h"
#include "webrtc/modules/audio_coding/main/test/TestFEC.h"
#include "webrtc/modules/audio_coding/main/test/TestStereo.h"
#include "webrtc/modules/audio_coding/main/test/TestVADDTX.h"
#include "webrtc/modules/audio_coding/main/test/TwoWayCommunication.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/gtest_disable.h"

using webrtc::AudioCodingModule;
using webrtc::Trace;



#define ACM_TEST_MODE 0

TEST(AudioCodingModuleTest, TestAllCodecs) {
  Trace::CreateTrace();
  Trace::SetTraceFile((webrtc::test::OutputPath() +
          "acm_allcodecs_trace.txt").c_str());
  webrtc::TestAllCodecs(ACM_TEST_MODE).Perform();
  Trace::ReturnTrace();
}

TEST(AudioCodingModuleTest, DISABLED_ON_ANDROID(TestEncodeDecode)) {
  Trace::CreateTrace();
  Trace::SetTraceFile((webrtc::test::OutputPath() +
      "acm_encodedecode_trace.txt").c_str());
  webrtc::EncodeDecodeTest(ACM_TEST_MODE).Perform();
  Trace::ReturnTrace();
}

TEST(AudioCodingModuleTest, DISABLED_ON_ANDROID(TestFEC)) {
  Trace::CreateTrace();
  Trace::SetTraceFile((webrtc::test::OutputPath() +
      "acm_fec_trace.txt").c_str());
  webrtc::TestFEC().Perform();
  Trace::ReturnTrace();
}

TEST(AudioCodingModuleTest, DISABLED_ON_ANDROID(TestIsac)) {
  Trace::CreateTrace();
  Trace::SetTraceFile((webrtc::test::OutputPath() +
      "acm_isac_trace.txt").c_str());
  webrtc::ISACTest(ACM_TEST_MODE).Perform();
  Trace::ReturnTrace();
}

TEST(AudioCodingModuleTest, DISABLED_ON_ANDROID(TwoWayCommunication)) {
  Trace::CreateTrace();
  Trace::SetTraceFile((webrtc::test::OutputPath() +
      "acm_twowaycom_trace.txt").c_str());
  webrtc::TwoWayCommunication(ACM_TEST_MODE).Perform();
  Trace::ReturnTrace();
}

TEST(AudioCodingModuleTest, DISABLED_ON_ANDROID(TestStereo)) {
  Trace::CreateTrace();
  Trace::SetTraceFile((webrtc::test::OutputPath() +
      "acm_stereo_trace.txt").c_str());
  webrtc::TestStereo(ACM_TEST_MODE).Perform();
  Trace::ReturnTrace();
}

TEST(AudioCodingModuleTest, DISABLED_ON_ANDROID(TestVADDTX)) {
  Trace::CreateTrace();
  Trace::SetTraceFile((webrtc::test::OutputPath() +
      "acm_vaddtx_trace.txt").c_str());
  webrtc::TestVADDTX().Perform();
  Trace::ReturnTrace();
}

TEST(AudioCodingModuleTest, TestOpus) {
  Trace::CreateTrace();
  Trace::SetTraceFile((webrtc::test::OutputPath() +
      "acm_opus_trace.txt").c_str());
  webrtc::OpusTest().Perform();
  Trace::ReturnTrace();
}



#ifdef ACM_TEST_FULL_API
  TEST(AudioCodingModuleTest, TestAPI) {
    Trace::CreateTrace();
    Trace::SetTraceFile((webrtc::test::OutputPath() +
        "acm_apitest_trace.txt").c_str());
    webrtc::APITest().Perform();
    Trace::ReturnTrace();
  }
#endif
