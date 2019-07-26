









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_TEST_SPATIALAUDIO_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_TEST_SPATIALAUDIO_H_

#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/main/test/ACMTest.h"
#include "webrtc/modules/audio_coding/main/test/Channel.h"
#include "webrtc/modules/audio_coding/main/test/PCMFile.h"
#include "webrtc/modules/audio_coding/main/test/utility.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

#define MAX_FILE_NAME_LENGTH_BYTE 500

namespace webrtc {

class SpatialAudio : public ACMTest {
 public:
  SpatialAudio(int testMode);
  ~SpatialAudio();

  void Perform();
 private:
  int16_t Setup();
  void EncodeDecode(double leftPanning, double rightPanning);
  void EncodeDecode();

  scoped_ptr<AudioCodingModule> _acmLeft;
  scoped_ptr<AudioCodingModule> _acmRight;
  scoped_ptr<AudioCodingModule> _acmReceiver;
  Channel* _channel;
  PCMFile _inFile;
  PCMFile _outFile;
  int _testMode;
};

}  

#endif  
