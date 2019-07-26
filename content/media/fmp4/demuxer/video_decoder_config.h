



#ifndef MEDIA_BASE_VIDEO_DECODER_CONFIG_H_
#define MEDIA_BASE_VIDEO_DECODER_CONFIG_H_

#include <string>
#include <vector>

#include "mp4_demuxer/basictypes.h"

namespace mp4_demuxer {

enum VideoCodec {
  
  
  
  kUnknownVideoCodec = 0,
  kCodecH264,
  kCodecVC1,
  kCodecMPEG2,
  kCodecMPEG4,
  kCodecTheora,
  kCodecVP8,
  kCodecVP9,
  
  
  
  

  kVideoCodecMax = kCodecVP9  
};



enum VideoCodecProfile {
  
  
  
  VIDEO_CODEC_PROFILE_UNKNOWN = -1,
  H264PROFILE_MIN = 0,
  H264PROFILE_BASELINE = H264PROFILE_MIN,
  H264PROFILE_MAIN = 1,
  H264PROFILE_EXTENDED = 2,
  H264PROFILE_HIGH = 3,
  H264PROFILE_HIGH10PROFILE = 4,
  H264PROFILE_HIGH422PROFILE = 5,
  H264PROFILE_HIGH444PREDICTIVEPROFILE = 6,
  H264PROFILE_SCALABLEBASELINE = 7,
  H264PROFILE_SCALABLEHIGH = 8,
  H264PROFILE_STEREOHIGH = 9,
  H264PROFILE_MULTIVIEWHIGH = 10,
  H264PROFILE_MAX = H264PROFILE_MULTIVIEWHIGH,
  VP8PROFILE_MIN = 11,
  VP8PROFILE_MAIN = VP8PROFILE_MIN,
  VP8PROFILE_MAX = VP8PROFILE_MAIN,
  VP9PROFILE_MIN = 12,
  VP9PROFILE_MAIN = VP9PROFILE_MIN,
  VP9PROFILE_MAX = VP9PROFILE_MAIN,
  VIDEO_CODEC_PROFILE_MAX = VP9PROFILE_MAX,
};




enum VideoFrameFormat { 
  INVALID = 0,  
  RGB32 = 4,  
  YV12 = 6,  
  YV16 = 7,  
  EMPTY = 9,  
  I420 = 11,  
  NATIVE_TEXTURE = 12,  
#if defined(GOOGLE_TV)
  HOLE = 13,  
#endif
  YV12A = 14,  
};

class VideoDecoderConfig {
 public:
  
  
  VideoDecoderConfig();

  
  
  VideoDecoderConfig(VideoCodec codec,
                     VideoCodecProfile profile,
                     VideoFrameFormat format,
                     const IntSize& coded_size,
                     const IntRect& visible_rect,
                     const IntSize& natural_size,
                     const uint8_t* extra_data, size_t extra_data_size,
                     bool is_encrypted);

  ~VideoDecoderConfig();

  
  void Initialize(VideoCodec codec,
                  VideoCodecProfile profile,
                  VideoFrameFormat format,
                  const IntSize& coded_size,
                  const IntRect& visible_rect,
                  const IntSize& natural_size,
                  const uint8_t* extra_data, size_t extra_data_size,
                  bool is_encrypted,
                  bool record_stats);

  
  
  bool IsValidConfig() const;

  
  
  bool Matches(const VideoDecoderConfig& config) const;

  
  
  std::string AsHumanReadableString() const;

  VideoCodec codec() const;
  VideoCodecProfile profile() const;

  
  VideoFrameFormat format() const;

  
  
  IntSize coded_size() const;

  
  IntRect visible_rect() const;

  
  
  IntSize natural_size() const;

  
  
  const uint8_t* extra_data() const;
  size_t extra_data_size() const;

  
  
  
  bool is_encrypted() const;

 private:
  VideoCodec codec_;
  VideoCodecProfile profile_;

  VideoFrameFormat format_;

  IntSize coded_size_;
  IntRect visible_rect_;
  IntSize natural_size_;

  std::vector<uint8_t> extra_data_;

  bool is_encrypted_;

  
  
  
};

}  

#endif  
