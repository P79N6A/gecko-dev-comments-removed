









#ifndef ACM_TEST_SPATIAL_AUDIO_H
#define ACM_TEST_SPATIAL_AUDIO_H

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "ACMTest.h"
#include "Channel.h"
#include "PCMFile.h"
#include "audio_coding_module.h"
#include "utility.h"

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
