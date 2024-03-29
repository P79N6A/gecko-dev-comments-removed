









#include "webrtc/modules/audio_coding/main/test/TestRedFec.h"

#include <assert.h>

#include "webrtc/common.h"
#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/audio_coding/main/test/utility.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {

TestRedFec::TestRedFec()
    : _acmA(AudioCodingModule::Create(0)),
      _acmB(AudioCodingModule::Create(1)),
      _channelA2B(NULL),
      _testCntr(0) {
}

TestRedFec::~TestRedFec() {
  if (_channelA2B != NULL) {
    delete _channelA2B;
    _channelA2B = NULL;
  }
}

void TestRedFec::Perform() {
  const std::string file_name = webrtc::test::ResourcePath(
      "audio_coding/testfile32kHz", "pcm");
  _inFileA.Open(file_name, 32000, "rb");

  ASSERT_EQ(0, _acmA->InitializeReceiver());
  ASSERT_EQ(0, _acmB->InitializeReceiver());

  uint8_t numEncoders = _acmA->NumberOfCodecs();
  CodecInst myCodecParam;
  for (uint8_t n = 0; n < numEncoders; n++) {
    EXPECT_EQ(0, _acmB->Codec(n, &myCodecParam));
    
    if (!strcmp(myCodecParam.plname, "opus")) {
      myCodecParam.channels = 1;
    }
    EXPECT_EQ(0, _acmB->RegisterReceiveCodec(myCodecParam));
  }

  
  _channelA2B = new Channel;
  _acmA->RegisterTransportCallback(_channelA2B);
  _channelA2B->RegisterReceiverACM(_acmB.get());

#ifndef WEBRTC_CODEC_G722
  EXPECT_TRUE(false);
  printf("G722 needs to be activated to run this test\n");
  return;
#endif
  char nameG722[] = "G722";
  EXPECT_EQ(0, RegisterSendCodec('A', nameG722, 16000));
  char nameCN[] = "CN";
  EXPECT_EQ(0, RegisterSendCodec('A', nameCN, 16000));
  char nameRED[] = "RED";
  EXPECT_EQ(0, RegisterSendCodec('A', nameRED));
  OpenOutFile(_testCntr);
  EXPECT_EQ(0, SetVAD(true, true, VADAggr));
  EXPECT_EQ(0, _acmA->SetREDStatus(false));
  EXPECT_FALSE(_acmA->REDStatus());
  Run();
  _outFileB.Close();

  EXPECT_EQ(0, _acmA->SetREDStatus(true));
  EXPECT_TRUE(_acmA->REDStatus());
  OpenOutFile(_testCntr);
  Run();
  _outFileB.Close();

  char nameISAC[] = "iSAC";
  RegisterSendCodec('A', nameISAC, 16000);
  OpenOutFile(_testCntr);
  EXPECT_EQ(0, SetVAD(true, true, VADVeryAggr));
  EXPECT_EQ(0, _acmA->SetREDStatus(false));
  EXPECT_FALSE(_acmA->REDStatus());
  Run();
  _outFileB.Close();

  EXPECT_EQ(0, _acmA->SetREDStatus(true));
  EXPECT_TRUE(_acmA->REDStatus());
  OpenOutFile(_testCntr);
  Run();
  _outFileB.Close();

  RegisterSendCodec('A', nameISAC, 32000);
  OpenOutFile(_testCntr);
  EXPECT_EQ(0, SetVAD(true, true, VADVeryAggr));
  EXPECT_EQ(0, _acmA->SetREDStatus(false));
  EXPECT_FALSE(_acmA->REDStatus());
  Run();
  _outFileB.Close();

  EXPECT_EQ(0, _acmA->SetREDStatus(true));
  EXPECT_TRUE(_acmA->REDStatus());
  OpenOutFile(_testCntr);
  Run();
  _outFileB.Close();

  RegisterSendCodec('A', nameISAC, 32000);
  OpenOutFile(_testCntr);
  EXPECT_EQ(0, SetVAD(false, false, VADNormal));
  EXPECT_EQ(0, _acmA->SetREDStatus(true));
  EXPECT_TRUE(_acmA->REDStatus());
  Run();

  RegisterSendCodec('A', nameISAC, 16000);
  EXPECT_TRUE(_acmA->REDStatus());
  Run();

  RegisterSendCodec('A', nameISAC, 32000);
  EXPECT_TRUE(_acmA->REDStatus());
  Run();

  RegisterSendCodec('A', nameISAC, 16000);
  EXPECT_TRUE(_acmA->REDStatus());
  Run();
  _outFileB.Close();

  _channelA2B->SetFECTestWithPacketLoss(true);

  EXPECT_EQ(0, RegisterSendCodec('A', nameG722));
  EXPECT_EQ(0, RegisterSendCodec('A', nameCN, 16000));
  OpenOutFile(_testCntr);
  EXPECT_EQ(0, SetVAD(true, true, VADAggr));
  EXPECT_EQ(0, _acmA->SetREDStatus(false));
  EXPECT_FALSE(_acmA->REDStatus());
  Run();
  _outFileB.Close();

  EXPECT_EQ(0, _acmA->SetREDStatus(true));
  EXPECT_TRUE(_acmA->REDStatus());
  OpenOutFile(_testCntr);
  Run();
  _outFileB.Close();

  RegisterSendCodec('A', nameISAC, 16000);
  OpenOutFile(_testCntr);
  EXPECT_EQ(0, SetVAD(true, true, VADVeryAggr));
  EXPECT_EQ(0, _acmA->SetREDStatus(false));
  EXPECT_FALSE(_acmA->REDStatus());
  Run();
  _outFileB.Close();

  EXPECT_EQ(0, _acmA->SetREDStatus(true));
  EXPECT_TRUE(_acmA->REDStatus());
  OpenOutFile(_testCntr);
  Run();
  _outFileB.Close();

  RegisterSendCodec('A', nameISAC, 32000);
  OpenOutFile(_testCntr);
  EXPECT_EQ(0, SetVAD(true, true, VADVeryAggr));
  EXPECT_EQ(0, _acmA->SetREDStatus(false));
  EXPECT_FALSE(_acmA->REDStatus());
  Run();
  _outFileB.Close();

  EXPECT_EQ(0, _acmA->SetREDStatus(true));
  EXPECT_TRUE(_acmA->REDStatus());
  OpenOutFile(_testCntr);
  Run();
  _outFileB.Close();

  RegisterSendCodec('A', nameISAC, 32000);
  OpenOutFile(_testCntr);
  EXPECT_EQ(0, SetVAD(false, false, VADNormal));
  EXPECT_EQ(0, _acmA->SetREDStatus(true));
  EXPECT_TRUE(_acmA->REDStatus());
  Run();

  RegisterSendCodec('A', nameISAC, 16000);
  EXPECT_TRUE(_acmA->REDStatus());
  Run();

  RegisterSendCodec('A', nameISAC, 32000);
  EXPECT_TRUE(_acmA->REDStatus());
  Run();

  RegisterSendCodec('A', nameISAC, 16000);
  EXPECT_TRUE(_acmA->REDStatus());
  Run();
  _outFileB.Close();

#ifndef WEBRTC_CODEC_OPUS
  EXPECT_TRUE(false);
  printf("Opus needs to be activated to run this test\n");
  return;
#endif

  char nameOpus[] = "opus";
  RegisterSendCodec('A', nameOpus, 48000);

  EXPECT_TRUE(_acmA->REDStatus());

  
  EXPECT_EQ(0, _acmA->SetPacketLossRate(25));

  
  EXPECT_EQ(-1, _acmA->SetCodecFEC(true));

  EXPECT_EQ(0, _acmA->SetREDStatus(false));
  EXPECT_EQ(0, _acmA->SetCodecFEC(true));

  
  EXPECT_EQ(-1, _acmA->SetREDStatus(true));

  EXPECT_TRUE(_acmA->CodecFEC());
  OpenOutFile(_testCntr);
  Run();

  
  RegisterSendCodec('A', nameISAC, 32000);
  EXPECT_EQ(0, SetVAD(false, false, VADNormal));

  
  EXPECT_FALSE(_acmA->CodecFEC());

  EXPECT_EQ(0, _acmA->SetREDStatus(true));
  EXPECT_TRUE(_acmA->REDStatus());
  Run();

  
  RegisterSendCodec('A', nameOpus, 48000);
  EXPECT_EQ(0, _acmA->SetCodecFEC(false));
  EXPECT_EQ(0, _acmA->SetREDStatus(false));
  Run();

  EXPECT_EQ(0, _acmA->SetCodecFEC(true));
  _outFileB.Close();

  
  RegisterSendCodec('A', nameG722, 16000);
  EXPECT_FALSE(_acmA->REDStatus());
  EXPECT_EQ(-1, _acmA->SetCodecFEC(true));
  EXPECT_FALSE(_acmA->CodecFEC());

  RegisterSendCodec('A', nameISAC, 16000);
  EXPECT_FALSE(_acmA->REDStatus());
  EXPECT_EQ(-1, _acmA->SetCodecFEC(true));
  EXPECT_FALSE(_acmA->CodecFEC());

  
  RegisterSendCodec('A', nameG722, 16000);
  EXPECT_FALSE(_acmA->REDStatus());
  EXPECT_EQ(0, _acmA->SetCodecFEC(false));
  EXPECT_FALSE(_acmA->CodecFEC());

  RegisterSendCodec('A', nameISAC, 16000);
  EXPECT_FALSE(_acmA->REDStatus());
  EXPECT_EQ(0, _acmA->SetCodecFEC(false));
  EXPECT_FALSE(_acmA->CodecFEC());
}

int32_t TestRedFec::SetVAD(bool enableDTX, bool enableVAD, ACMVADMode vadMode) {
  return _acmA->SetVAD(enableDTX, enableVAD, vadMode);
}

int16_t TestRedFec::RegisterSendCodec(char side, char* codecName,
                                      int32_t samplingFreqHz) {
  std::cout << std::flush;
  AudioCodingModule* myACM;
  switch (side) {
    case 'A': {
      myACM = _acmA.get();
      break;
    }
    case 'B': {
      myACM = _acmB.get();
      break;
    }
    default:
      return -1;
  }

  if (myACM == NULL) {
    assert(false);
    return -1;
  }
  CodecInst myCodecParam;
  EXPECT_GT(AudioCodingModule::Codec(codecName, &myCodecParam,
                                     samplingFreqHz, 1), -1);
  EXPECT_GT(myACM->RegisterSendCodec(myCodecParam), -1);

  
  return 0;
}

void TestRedFec::Run() {
  AudioFrame audioFrame;

  uint16_t msecPassed = 0;
  uint32_t secPassed = 0;
  int32_t outFreqHzB = _outFileB.SamplingFrequency();

  while (!_inFileA.EndOfFile()) {
    EXPECT_GT(_inFileA.Read10MsData(audioFrame), 0);
    EXPECT_EQ(0, _acmA->Add10MsData(audioFrame));
    EXPECT_GT(_acmA->Process(), -1);
    EXPECT_EQ(0, _acmB->PlayoutData10Ms(outFreqHzB, &audioFrame));
    _outFileB.Write10MsData(audioFrame.data_, audioFrame.samples_per_channel_);
    msecPassed += 10;
    if (msecPassed >= 1000) {
      msecPassed = 0;
      secPassed++;
    }
    
    if (((secPassed % 5) == 4) && (msecPassed == 0) && (_testCntr > 14)) {
      EXPECT_EQ(0, _acmA->SetREDStatus(false));
    }
    if (((secPassed % 5) == 4) && (msecPassed >= 990) && (_testCntr > 14)) {
      EXPECT_EQ(0, _acmA->SetREDStatus(true));
    }
  }
  _inFileA.Rewind();
}

void TestRedFec::OpenOutFile(int16_t test_number) {
  std::string file_name;
  std::stringstream file_stream;
  file_stream << webrtc::test::OutputPath();
  file_stream << "TestRedFec_outFile_";
  file_stream << test_number << ".pcm";
  file_name = file_stream.str();
  _outFileB.Open(file_name, 16000, "wb");
}

}  
