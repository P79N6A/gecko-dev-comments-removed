









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_TESTREDFEC_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_TESTREDFEC_H_

#include <string>
#include "webrtc/modules/audio_coding/main/test/ACMTest.h"
#include "webrtc/modules/audio_coding/main/test/Channel.h"
#include "webrtc/modules/audio_coding/main/test/PCMFile.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class Config;

class TestRedFec : public ACMTest {
 public:
  explicit TestRedFec();
  ~TestRedFec();

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
