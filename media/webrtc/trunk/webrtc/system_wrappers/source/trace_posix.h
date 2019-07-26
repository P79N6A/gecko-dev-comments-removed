









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_TRACE_POSIX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_TRACE_POSIX_H_

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/source/trace_impl.h"

namespace webrtc {

class TracePosix : public TraceImpl {
 public:
  TracePosix();
  virtual ~TracePosix();

  
  
  virtual int32_t AddTime(char* trace_message, const TraceLevel level) const
      OVERRIDE;

  virtual int32_t AddBuildInfo(char* trace_message) const OVERRIDE;
  virtual int32_t AddDateTimeInfo(char* trace_message) const OVERRIDE;

 private:
  volatile mutable uint32_t  prev_api_tick_count_;
  volatile mutable uint32_t  prev_tick_count_;

  CriticalSectionWrapper& crit_sect_;
};

}  

#endif  
