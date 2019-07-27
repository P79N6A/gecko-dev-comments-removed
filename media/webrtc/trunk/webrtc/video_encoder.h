









#ifndef WEBRTC_VIDEO_ENCODER_H_
#define WEBRTC_VIDEO_ENCODER_H_

#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/typedefs.h"
#include "webrtc/video_frame.h"

namespace webrtc {

class RTPFragmentationHeader;

struct CodecSpecificInfo;
struct VideoCodec;

class EncodedImageCallback {
 public:
  virtual ~EncodedImageCallback() {}

  
  
  virtual int32_t Encoded(
      EncodedImage& encoded_image,
      const CodecSpecificInfo* codec_specific_info = NULL,
      const RTPFragmentationHeader* fragmentation = NULL) = 0;
};

class VideoEncoder {
 public:
  enum EncoderType {
    kVp8,
    kVp9,
  };

  static VideoEncoder* Create(EncoderType codec_type);

  static VideoCodecVP8 GetDefaultVp8Settings();
  static VideoCodecVP9 GetDefaultVp9Settings();
  static VideoCodecH264 GetDefaultH264Settings();

  virtual ~VideoEncoder() {}

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t InitEncode(const VideoCodec* codec_settings,
                             int32_t number_of_cores,
                             uint32_t max_payload_size) = 0;

  
  
  
  
  
  
  virtual int32_t RegisterEncodeCompleteCallback(
      EncodedImageCallback* callback) = 0;

  
  
  virtual int32_t Release() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t Encode(const I420VideoFrame& frame,
                         const CodecSpecificInfo* codec_specific_info,
                         const std::vector<VideoFrameType>* frame_types) = 0;

  
  
  
  
  
  
  
  
  
  virtual int32_t SetChannelParameters(uint32_t packet_loss, int rtt) = 0;

  
  
  
  
  
  
  
  virtual int32_t SetRates(uint32_t bitrate, uint32_t framerate) = 0;

  virtual int32_t SetPeriodicKeyFrames(bool enable) { return -1; }
  virtual int32_t CodecConfigParameters(uint8_t* , int32_t ) {
    return -1;
  }
};

}  
#endif
