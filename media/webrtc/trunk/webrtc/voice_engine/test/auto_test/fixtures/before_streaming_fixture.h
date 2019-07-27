









#ifndef SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_BEFORE_STREAMING_H_
#define SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_BEFORE_STREAMING_H_

#include <string>
#include "webrtc/voice_engine/test/auto_test/fixtures/after_initialization_fixture.h"
#include "webrtc/voice_engine/test/auto_test/resource_manager.h"





class BeforeStreamingFixture : public AfterInitializationFixture {
 public:
  BeforeStreamingFixture();
  virtual ~BeforeStreamingFixture();

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
