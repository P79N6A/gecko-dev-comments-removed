









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TEST_BWE_TEST_LOGGING_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TEST_BWE_TEST_LOGGING_H_




#ifndef BWE_TEST_LOGGING_COMPILE_TIME_ENABLE
#define BWE_TEST_LOGGING_COMPILE_TIME_ENABLE 0
#endif  
































#if !(BWE_TEST_LOGGING_COMPILE_TIME_ENABLE)




#define BWE_TEST_LOGGING_GLOBAL_CONTEXT(name)



#define BWE_TEST_LOGGING_GLOBAL_ENABLE(enabled)



#define BWE_TEST_LOGGING_CONTEXT(name)




#define BWE_TEST_LOGGING_ENABLE(enabled)




#define BWE_TEST_LOGGING_TIME(time)






#define BWE_TEST_LOGGING_LOG1(name, format, _1)
#define BWE_TEST_LOGGING_LOG2(name, format, _1, _2)
#define BWE_TEST_LOGGING_LOG3(name, format, _1, _2, _3)
#define BWE_TEST_LOGGING_LOG4(name, format, _1, _2, _3, _4)
#define BWE_TEST_LOGGING_LOG5(name, format, _1, _2, _3, _4, _5)






#define BWE_TEST_LOGGING_PLOT(name, time, value)

#else  

#include <map>
#include <stack>
#include <string>

#include "webrtc/common_types.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

#define BWE_TEST_LOGGING_GLOBAL_CONTEXT(name) \
    do { \
      webrtc::testing::bwe::Logging::GetInstance()->SetGlobalContext(name); \
    } while (0);

#define BWE_TEST_LOGGING_GLOBAL_ENABLE(enabled) \
    do { \
      webrtc::testing::bwe::Logging::GetInstance()->SetGlobalEnable(enabled); \
    } while (0);

#define __BWE_TEST_LOGGING_CONTEXT_NAME(ctx, line) ctx ## line
#define __BWE_TEST_LOGGING_CONTEXT_DECLARE(ctx, line, name, time, enabled) \
    webrtc::testing::bwe::Logging::Context \
        __BWE_TEST_LOGGING_CONTEXT_NAME(ctx, line)(name, time, enabled)

#define BWE_TEST_LOGGING_CONTEXT(name) \
    __BWE_TEST_LOGGING_CONTEXT_DECLARE(__bwe_log_, __LINE__, name, -1, true)
#define BWE_TEST_LOGGING_ENABLE(enabled) \
    __BWE_TEST_LOGGING_CONTEXT_DECLARE(__bwe_log_, __LINE__, "", -1, \
                                       static_cast<bool>(enabled))
#define BWE_TEST_LOGGING_TIME(time) \
    __BWE_TEST_LOGGING_CONTEXT_DECLARE(__bwe_log_, __LINE__, "", \
                                       static_cast<int64_t>(time), true)

#define BWE_TEST_LOGGING_LOG1(name, format, _1) \
    do { \
      BWE_TEST_LOGGING_CONTEXT(name); \
      webrtc::testing::bwe::Logging::GetInstance()->Log(format, _1); \
    } while (0);
#define BWE_TEST_LOGGING_LOG2(name, format, _1, _2) \
    do { \
      BWE_TEST_LOGGING_CONTEXT(name); \
      webrtc::testing::bwe::Logging::GetInstance()->Log(format, _1, _2); \
    } while (0);
#define BWE_TEST_LOGGING_LOG3(name, format, _1, _2, _3) \
    do { \
      BWE_TEST_LOGGING_CONTEXT(name); \
      webrtc::testing::bwe::Logging::GetInstance()->Log(format, _1, _2, _3); \
    } while (0);
#define BWE_TEST_LOGGING_LOG4(name, format, _1, _2, _3, _4) \
    do { \
      BWE_TEST_LOGGING_CONTEXT(name); \
      webrtc::testing::bwe::Logging::GetInstance()->Log(format, _1, _2, _3, \
                                                        _4); \
    } while (0);
#define BWE_TEST_LOGGING_LOG5(name, format, _1, _2, _3, _4, _5) \
    do {\
      BWE_TEST_LOGGING_CONTEXT(name); \
      webrtc::testing::bwe::Logging::GetInstance()->Log(format, _1, _2, _3, \
                                                        _4, _5); \
    } while (0);

#define BWE_TEST_LOGGING_PLOT(name, time, value)\
    do { \
      __BWE_TEST_LOGGING_CONTEXT_DECLARE(__bwe_log_, __LINE__, name, \
                                         static_cast<int64_t>(time), true); \
      webrtc::testing::bwe::Logging::GetInstance()->Plot(value); \
    } while (0);

namespace webrtc {

class CriticalSectionWrapper;

namespace testing {
namespace bwe {

class Logging {
 public:
  class Context {
   public:
    Context(uint32_t name, int64_t timestamp_ms, bool enabled);
    Context(const std::string& name, int64_t timestamp_ms, bool enabled);
    Context(const char* name, int64_t timestamp_ms, bool enabled);
    ~Context();
   private:
    DISALLOW_IMPLICIT_CONSTRUCTORS(Context);
  };

  static Logging* GetInstance();

  void SetGlobalContext(uint32_t name);
  void SetGlobalContext(const std::string& name);
  void SetGlobalContext(const char* name);
  void SetGlobalEnable(bool enabled);

  void Log(const char format[], ...);
  void Plot(double value);

 private:
  struct State {
    State();
    State(const std::string& new_tag, int64_t timestamp_ms, bool enabled);
    void MergePrevious(const State& previous);

    std::string tag;
    int64_t timestamp_ms;
    bool enabled;
  };
  struct ThreadState {
    State global_state;
    std::stack<State> stack;
  };
  typedef std::map<uint32_t, ThreadState> ThreadMap;

  Logging();
  void PushState(const std::string& append_to_tag, int64_t timestamp_ms,
                 bool enabled);
  void PopState();

  static Logging g_Logging;
  scoped_ptr<CriticalSectionWrapper> crit_sect_;
  ThreadMap thread_map_;

  DISALLOW_COPY_AND_ASSIGN(Logging);
};
}  
}  
}  

#endif  
#endif  
