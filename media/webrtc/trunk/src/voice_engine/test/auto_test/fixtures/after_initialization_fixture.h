









#ifndef SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_TEST_BASE_AFTER_INIT_H_
#define SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_TEST_BASE_AFTER_INIT_H_

#include "before_initialization_fixture.h"
#include "scoped_ptr.h"

class TestErrorObserver;






class AfterInitializationFixture : public BeforeInitializationFixture {
 public:
  AfterInitializationFixture();
  virtual ~AfterInitializationFixture();
 protected:
  webrtc::scoped_ptr<TestErrorObserver> error_observer_;
};

#endif  
