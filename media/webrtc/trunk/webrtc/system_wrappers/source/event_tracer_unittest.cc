









#include "webrtc/system_wrappers/interface/event_tracer.h"

#include "gtest/gtest.h"
#include "webrtc/system_wrappers/interface/static_instance.h"
#include "webrtc/system_wrappers/interface/trace_event.h"

namespace {

class TestStatistics {
 public:
  TestStatistics() : events_logged_(0) {
  }

  void Reset() {
    events_logged_ = 0;
  }

  void Increment() {
    ++events_logged_;
  }

  int Count() const { return events_logged_; }

  static TestStatistics* Get() {
    static TestStatistics* test_stats = NULL;
    if (!test_stats)
      test_stats = new TestStatistics();
    return test_stats;
  }

 private:
  int events_logged_;
};

static const unsigned char* GetCategoryEnabledHandler(const char* name) {
  return reinterpret_cast<const unsigned char*>("test");
}

static void AddTraceEventHandler(char phase,
                                 const unsigned char* category_enabled,
                                 const char* name,
                                 unsigned long long id,
                                 int num_args,
                                 const char** arg_names,
                                 const unsigned char* arg_types,
                                 const unsigned long long* arg_values,
                                 unsigned char flags) {
  TestStatistics::Get()->Increment();
}

}  

namespace webrtc {

TEST(EventTracerTest, EventTracerDisabled) {
  {
    TRACE_EVENT0("test", "EventTracerDisabled");
  }
  EXPECT_FALSE(TestStatistics::Get()->Count());
  TestStatistics::Get()->Reset();
}

TEST(EventTracerTest, ScopedTraceEvent) {
  SetupEventTracer(&GetCategoryEnabledHandler, &AddTraceEventHandler);
  {
    TRACE_EVENT0("test", "ScopedTraceEvent");
  }
  EXPECT_EQ(2, TestStatistics::Get()->Count());
  TestStatistics::Get()->Reset();
}

}  
