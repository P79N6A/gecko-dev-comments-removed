









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_STATS_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_STATS_H_

#include <vector>

#include "webrtc/common_video/interface/video_image.h"

namespace webrtc {
namespace test {


struct FrameStatistic {
  FrameStatistic();

  bool encoding_successful;
  bool decoding_successful;
  int encode_return_code;
  int decode_return_code;
  int encode_time_in_us;
  int decode_time_in_us;
  int frame_number;
  
  int packets_dropped;
  int total_packets;

  
  
  int bit_rate_in_kbps;

  
  int encoded_frame_length_in_bytes;
  webrtc::VideoFrameType frame_type;
};



class Stats {
 public:
  typedef std::vector<FrameStatistic>::iterator FrameStatisticsIterator;

  Stats();
  virtual ~Stats();

  
  
  
  
  FrameStatistic& NewFrame(int frame_number);

  
  
  void PrintSummary();

  std::vector<FrameStatistic> stats_;
};

}  
}  

#endif  
