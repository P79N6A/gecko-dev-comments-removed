









#ifndef API_TEST_H
#define API_TEST_H

#include "webrtc/modules/audio_coding/main/test/ACMTest.h"
#include "webrtc/modules/audio_coding/main/test/Channel.h"
#include "webrtc/modules/audio_coding/main/test/PCMFile.h"
#include "webrtc/modules/audio_coding/main/test/utility.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"

namespace webrtc {

enum APITESTAction {
  TEST_CHANGE_CODEC_ONLY = 0,
  DTX_TEST = 1
};

class APITest : public ACMTest {
 public:
  APITest();
  ~APITest();

  void Perform();
 private:
  int16_t SetUp();

  static bool PushAudioThreadA(void* obj);
  static bool PullAudioThreadA(void* obj);
  static bool ProcessThreadA(void* obj);
  static bool APIThreadA(void* obj);

  static bool PushAudioThreadB(void* obj);
  static bool PullAudioThreadB(void* obj);
  static bool ProcessThreadB(void* obj);
  static bool APIThreadB(void* obj);

  void CheckVADStatus(char side);

  
  void TestDelay(char side);

  
  void TestRegisteration(char side);

  
  
  void TestPlayout(char receiveSide);

  
  void TestSendVAD(char side);

  void CurrentCodec(char side);

  void ChangeCodec(char side);

  void Wait(uint32_t waitLengthMs);

  void RunTest(char thread);

  bool PushAudioRunA();
  bool PullAudioRunA();
  bool ProcessRunA();
  bool APIRunA();

  bool PullAudioRunB();
  bool PushAudioRunB();
  bool ProcessRunB();
  bool APIRunB();

  
  AudioCodingModule* _acmA;
  AudioCodingModule* _acmB;

  
  Channel* _channel_A2B;
  Channel* _channel_B2A;

  
  
  PCMFile _inFileA;
  PCMFile _outFileA;
  
  PCMFile _outFileB;
  PCMFile _inFileB;

  
  
  int32_t _outFreqHzA;
  
  int32_t _outFreqHzB;

  
  
  
  bool _writeToFile;
  
  
  EventWrapper* _pullEventA;      
  EventWrapper* _pushEventA;      
  EventWrapper* _processEventA;   
  EventWrapper* _apiEventA;       
  
  EventWrapper* _pullEventB;      
  EventWrapper* _pushEventB;      
  EventWrapper* _processEventB;   
  EventWrapper* _apiEventB;       

  
  uint8_t _codecCntrA;
  uint8_t _codecCntrB;

  
  bool _thereIsEncoderA;
  bool _thereIsEncoderB;
  bool _thereIsDecoderA;
  bool _thereIsDecoderB;

  bool _sendVADA;
  bool _sendDTXA;
  ACMVADMode _sendVADModeA;

  bool _sendVADB;
  bool _sendDTXB;
  ACMVADMode _sendVADModeB;

  int32_t _minDelayA;
  int32_t _minDelayB;
  bool _payloadUsed[32];

  AudioPlayoutMode _playoutModeA;
  AudioPlayoutMode _playoutModeB;

  bool _verbose;

  int _dotPositionA;
  int _dotMoveDirectionA;
  int _dotPositionB;
  int _dotMoveDirectionB;

  char _movingDot[41];

  DTMFDetector* _dtmfCallback;
  VADCallback* _vadCallbackA;
  VADCallback* _vadCallbackB;
  RWLockWrapper& _apiTestRWLock;
  bool _randomTest;
  int _testNumA;
  int _testNumB;
};

}  

#endif
