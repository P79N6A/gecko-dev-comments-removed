









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_TEST_TESTVADDTX_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_TEST_TESTVADDTX_H_

#include "webrtc/modules/audio_coding/main/test/ACMTest.h"
#include "webrtc/modules/audio_coding/main/test/Channel.h"
#include "webrtc/modules/audio_coding/main/test/PCMFile.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class Config;

typedef struct {
  bool statusDTX;
  bool statusVAD;
  ACMVADMode vadMode;
} VADDTXstruct;

class ActivityMonitor : public ACMVADCallback {
 public:
  ActivityMonitor();
  ~ActivityMonitor();
  int32_t InFrameType(int16_t frameType);
  void PrintStatistics();
  void ResetStatistics();
  void GetStatistics(uint32_t* getCounter);
 private:
  
  
  
  
  
  
  
  
  
  uint32_t _counter[6];
};

class TestVADDTX : public ACMTest {
 public:
  explicit TestVADDTX(const Config& config);
  ~TestVADDTX();

  void Perform();
 private:
  
  
  int16_t RegisterSendCodec(char side,
                            char* codecName,
                            int32_t samplingFreqHz = -1,
                            int32_t rateKhz = -1);
  void Run();
  void OpenOutFile(int16_t testNumber);
  void runTestCases();
  void runTestInternalDTX(int expected_result);
  void SetVAD(bool statusDTX, bool statusVAD, int16_t vadMode);
  VADDTXstruct GetVAD();
  int16_t VerifyTest();
  scoped_ptr<AudioCodingModule> _acmA;
  scoped_ptr<AudioCodingModule> _acmB;

  Channel* _channelA2B;

  PCMFile _inFileA;
  PCMFile _outFileB;

  ActivityMonitor _monitor;
  uint32_t _statCounter[6];

  VADDTXstruct _setStruct;
  VADDTXstruct _getStruct;
};

}  

#endif  
