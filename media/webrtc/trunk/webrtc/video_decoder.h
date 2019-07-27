









#ifndef WEBRTC_VIDEO_DECODER_H_
#define WEBRTC_VIDEO_DECODER_H_

#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/typedefs.h"
#include "webrtc/video_frame.h"

namespace webrtc {

class RTPFragmentationHeader;

struct CodecSpecificInfo;
struct VideoCodec;

class DecodedImageCallback {
 public:
  virtual ~DecodedImageCallback() {}

  virtual int32_t Decoded(I420VideoFrame& decodedImage) = 0;
  virtual int32_t ReceivedDecodedReferenceFrame(const uint64_t pictureId) {
    return -1;
  }

  virtual int32_t ReceivedDecodedFrame(const uint64_t pictureId) { return -1; }
};

class VideoDecoder {
 public:
  enum DecoderType {
    kVp8,
    kVp9
  };

  static VideoDecoder* Create(DecoderType codec_type);

  virtual ~VideoDecoder() {}

  virtual int32_t InitDecode(const VideoCodec* codecSettings,
                             int32_t numberOfCores) = 0;

  virtual int32_t Decode(const EncodedImage& inputImage,
                         bool missingFrames,
                         const RTPFragmentationHeader* fragmentation,
                         const CodecSpecificInfo* codecSpecificInfo = NULL,
                         int64_t renderTimeMs = -1) = 0;

  virtual int32_t RegisterDecodeCompleteCallback(
      DecodedImageCallback* callback) = 0;

  virtual int32_t Release() = 0;
  virtual int32_t Reset() = 0;

  virtual int32_t SetCodecConfigParameters(const uint8_t* ,
                                           int32_t ) {
    return -1;
  }

  virtual VideoDecoder* Copy() { return NULL; }
};

}  

#endif
