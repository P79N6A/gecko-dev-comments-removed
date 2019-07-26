









#include "webrtc/modules/audio_coding/neteq4/buffer_level_filter.h"

#include <algorithm>  

namespace webrtc {

BufferLevelFilter::BufferLevelFilter() {
  Reset();
}

void BufferLevelFilter::Reset() {
  filtered_current_level_ = 0;
  level_factor_ = 253;
}

void BufferLevelFilter::Update(int buffer_size_packets,
                               int time_stretched_samples,
                               int packet_len_samples) {
  
  
  
  
  
  filtered_current_level_ = ((level_factor_ * filtered_current_level_) >> 8) +
      ((256 - level_factor_) * buffer_size_packets);

  
  if (time_stretched_samples && packet_len_samples > 0) {
    
    
    
    
    filtered_current_level_ = std::max(0,
        filtered_current_level_ -
        (time_stretched_samples << 8) / packet_len_samples);
  }
}

void BufferLevelFilter::SetTargetBufferLevel(int target_buffer_level) {
  if (target_buffer_level <= 1) {
    level_factor_ = 251;
  } else if (target_buffer_level <= 3) {
    level_factor_ = 252;
  } else if (target_buffer_level <= 7) {
    level_factor_ = 253;
  } else {
    level_factor_ = 254;
  }
}
}  
