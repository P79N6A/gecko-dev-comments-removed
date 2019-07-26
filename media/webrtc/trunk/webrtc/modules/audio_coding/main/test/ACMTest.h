









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_TEST_ACMTEST_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_TEST_ACMTEST_H_

class ACMTest {
 public:
  ACMTest() {}
  virtual ~ACMTest() = 0;
  virtual void Perform() = 0;
};

#endif  
