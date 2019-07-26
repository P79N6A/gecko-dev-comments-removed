









#ifndef TEST_VAD_DTX_H
#define TEST_VAD_DTX_H

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "ACMTest.h"
#include "Channel.h"
#include "PCMFile.h"

namespace webrtc {

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
  TestVADDTX();
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
