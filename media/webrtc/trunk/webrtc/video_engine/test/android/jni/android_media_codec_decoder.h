









#ifndef WEBRTC_VIDEO_ENGINE_TEST_ANDROID_JNI_ANDROID_MEDIA_CODEC_DECODER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_ANDROID_JNI_ANDROID_MEDIA_CODEC_DECODER_H_

#include "modules/video_coding/codecs/interface/video_codec_interface.h"

namespace webrtc {

class AndroidMediaCodecDecoder : public VideoDecoder {
 public:
  AndroidMediaCodecDecoder(JavaVM* vm, jobject surface, jclass decoderClass);
  virtual ~AndroidMediaCodecDecoder() { }

  
  
  
  
  
  
  
  virtual WebRtc_Word32 InitDecode(
      const VideoCodec* codecSettings, WebRtc_Word32 numberOfCores);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32
  Decode(const EncodedImage& inputImage,
         bool missingFrames,
         const RTPFragmentationHeader* fragmentation,
         const CodecSpecificInfo* codecSpecificInfo = NULL,
         WebRtc_Word64 renderTimeMs = -1);

  
  
  
  
  
  
  virtual WebRtc_Word32 RegisterDecodeCompleteCallback(
      DecodedImageCallback* callback);

  
  
  
  virtual WebRtc_Word32 Release();

  
  
  
  virtual WebRtc_Word32 Reset();

  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetCodecConfigParameters(
      const WebRtc_UWord8* , WebRtc_Word32 ) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  
  
  
  virtual VideoDecoder* Copy() { return NULL; }

 private:
  DecodedImageCallback* decode_complete_callback_;
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
