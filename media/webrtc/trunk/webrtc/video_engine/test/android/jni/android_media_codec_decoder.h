









#ifndef WEBRTC_VIDEO_ENGINE_TEST_ANDROID_JNI_ANDROID_MEDIA_CODEC_DECODER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_ANDROID_JNI_ANDROID_MEDIA_CODEC_DECODER_H_

#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"

namespace webrtc {

class AndroidMediaCodecDecoder : public VideoDecoder {
 public:
  AndroidMediaCodecDecoder(JavaVM* vm, jobject surface, jclass decoderClass);
  virtual ~AndroidMediaCodecDecoder();

  
  
  
  
  
  
  
  virtual int32_t InitDecode(
      const VideoCodec* codecSettings, int32_t numberOfCores);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t
  Decode(const EncodedImage& inputImage,
         bool missingFrames,
         const RTPFragmentationHeader* fragmentation,
         const CodecSpecificInfo* codecSpecificInfo = NULL,
         int64_t renderTimeMs = -1);

  
  
  
  
  
  
  virtual int32_t RegisterDecodeCompleteCallback(
      DecodedImageCallback* callback);

  
  
  
  virtual int32_t Release();

  
  
  
  virtual int32_t Reset();

  
  
  
  
  
  
  
  
  virtual int32_t SetCodecConfigParameters(
      const uint8_t* , int32_t ) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  
  
  
  virtual VideoDecoder* Copy() { return NULL; }

 private:
  void Initialize(JavaVM* vm, jobject surface, jclass decoderClass);

  JavaVM* vm_;
  jobject surface_;
  jobject mediaCodecDecoder_;
  jclass decoderClass_;
  JNIEnv* env_;
  jmethodID setEncodedImageID_;
  bool vm_attached_;
};

}  

#endif  
