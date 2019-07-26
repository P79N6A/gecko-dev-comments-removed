









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_BUFFER_LEVEL_FILTER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_BUFFER_LEVEL_FILTER_H_

#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {

class BufferLevelFilter {
 public:
  BufferLevelFilter();
  virtual ~BufferLevelFilter() {}
  virtual void Reset();

  
  
  
  
  
  virtual void Update(int buffer_size_packets, int time_stretched_samples,
                      int packet_len_samples);

  
  
  
  virtual void SetTargetBufferLevel(int target_buffer_level);

  virtual int filtered_current_level() const { return filtered_current_level_; }

 private:
  int level_factor_;  
  int filtered_current_level_;  

  DISALLOW_COPY_AND_ASSIGN(BufferLevelFilter);
};

}  
#endif  
