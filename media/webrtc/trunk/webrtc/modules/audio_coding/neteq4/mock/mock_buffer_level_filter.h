









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_BUFFER_LEVEL_FILTER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_BUFFER_LEVEL_FILTER_H_

#include "webrtc/modules/audio_coding/neteq4/buffer_level_filter.h"

#include "gmock/gmock.h"

namespace webrtc {

class MockBufferLevelFilter : public BufferLevelFilter {
 public:
  virtual ~MockBufferLevelFilter() { Die(); }
  MOCK_METHOD0(Die,
      void());
  MOCK_METHOD0(Reset,
      void());
  MOCK_METHOD3(Update,
      void(int buffer_size_packets, int time_stretched_samples,
           int packet_len_samples));
  MOCK_METHOD1(SetTargetBufferLevel,
      void(int target_buffer_level));
  MOCK_CONST_METHOD0(filtered_current_level,
      int());
};

}  
#endif  
