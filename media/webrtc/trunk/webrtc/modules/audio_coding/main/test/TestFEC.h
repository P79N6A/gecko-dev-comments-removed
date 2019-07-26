









#ifndef TEST_FEC_H
#define TEST_FEC_H

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "ACMTest.h"
#include "Channel.h"
#include "PCMFile.h"

namespace webrtc {

class TestFEC : public ACMTest {
 public:
  TestFEC();
  ~TestFEC();

  void Perform();
 private:
  
  
  
  int16_t RegisterSendCodec(char side, char* codecName,
                            int32_t sampFreqHz = -1);
  void Run();
  void OpenOutFile(int16_t testNumber);
  int32_t SetVAD(bool enableDTX, bool enableVAD, ACMVADMode vadMode);
  scoped_ptr<AudioCodingModule> _acmA;
  scoped_ptr<AudioCodingModule> _acmB;

  Channel* _channelA2B;

  PCMFile _inFileA;
  PCMFile _outFileB;
  int16_t _testCntr;
};

}  

#endif
