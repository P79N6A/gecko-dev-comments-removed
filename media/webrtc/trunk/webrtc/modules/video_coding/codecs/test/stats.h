









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_STATS_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_STATS_H_

#include <vector>

#include "common_video/interface/video_image.h"

namespace webrtc {
namespace test {


struct FrameStatistic {
  FrameStatistic() :
      encoding_successful(false), decoding_successful(false),
      encode_return_code(0), decode_return_code(0),
      encode_time_in_us(0), decode_time_in_us(0),
      frame_number(0), packets_dropped(0), total_packets(0),
      bit_rate_in_kbps(0), encoded_frame_length_in_bytes(0),
      frame_type(kDeltaFrame) {
  };
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
