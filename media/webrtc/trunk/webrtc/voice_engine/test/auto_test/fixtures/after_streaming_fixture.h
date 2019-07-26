









#ifndef SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_AFTER_STREAMING_H_
#define SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_AFTER_STREAMING_H_

#include "webrtc/voice_engine/test/auto_test/fixtures/after_initialization_fixture.h"
#include "webrtc/voice_engine/test/auto_test/resource_manager.h"





class AfterStreamingFixture : public AfterInitializationFixture {
 public:
  AfterStreamingFixture();
  virtual ~AfterStreamingFixture();

 protected:
  int             channel_;
  ResourceManager resource_manager_;
  std::string     fake_microphone_input_file_;

  
  void SwitchToManualMicrophone();

  
  void RestartFakeMicrophone();

  
  void PausePlaying();

  
  void ResumePlaying();

 private:
  void SetUpLocalPlayback();

  LoopBackTransport* transport_;
};


#endif  
