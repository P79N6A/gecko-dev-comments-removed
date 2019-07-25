









#ifndef WEBRTC_VOICE_ENGINE_VOE_STRESS_TEST_H
#define WEBRTC_VOICE_ENGINE_VOE_STRESS_TEST_H

namespace webrtc {
class ThreadWrapper;
}

namespace voetest {

using namespace webrtc;

class VoETestManager;

class VoEStressTest {
 public:
  VoEStressTest(VoETestManager& mgr) :
    _mgr(mgr), _ptrExtraApiThread(NULL) {
  }
  ~VoEStressTest() {
  }
  int DoTest();

 private:
  int MenuSelection();
  int StartStopTest();
  int CreateDeleteChannelsTest();
  int MultipleThreadsTest();

  static bool RunExtraApi(void* ptr);
  bool ProcessExtraApi();

  VoETestManager& _mgr;
  static const char* _key;

  ThreadWrapper* _ptrExtraApiThread;
};

} 

#endif 
