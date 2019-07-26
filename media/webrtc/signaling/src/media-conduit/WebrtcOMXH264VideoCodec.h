



#ifndef WEBRTC_GONK
#pragma error WebrtcOMXH264VideoCodec works only on B2G.
#endif

#ifndef WEBRTC_OMX_H264_CODEC_H_
#define WEBRTC_OMX_H264_CODEC_H_

#include "AudioConduit.h"
#include "VideoConduit.h"

namespace android {
  class OMXVideoEncoder;
}

namespace mozilla {

class WebrtcOMXDecoder;
class OMXOutputDrain;

class WebrtcOMXH264VideoEncoder : public WebrtcVideoEncoder
{
public:
  WebrtcOMXH264VideoEncoder();

  virtual ~WebrtcOMXH264VideoEncoder();

  
  virtual int32_t InitEncode(const webrtc::VideoCodec* aCodecSettings,
                             int32_t aNumOfCores,
                             uint32_t aMaxPayloadSize) MOZ_OVERRIDE;

  virtual int32_t Encode(const webrtc::I420VideoFrame& aInputImage,
                         const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
                         const std::vector<webrtc::VideoFrameType>* aFrameTypes) MOZ_OVERRIDE;

  virtual int32_t RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* aCallback) MOZ_OVERRIDE;

  virtual int32_t Release() MOZ_OVERRIDE;

  virtual int32_t SetChannelParameters(uint32_t aPacketLossRate,
                                       int aRoundTripTimeMs) MOZ_OVERRIDE;

  virtual int32_t SetRates(uint32_t aBitRate, uint32_t aFrameRate) MOZ_OVERRIDE;

private:
  RefPtr<android::OMXVideoEncoder> mOMX;
  webrtc::EncodedImageCallback* mCallback;
  RefPtr<OMXOutputDrain> mOutputDrain;
  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mFrameRate;
  bool mOMXConfigured;
  webrtc::EncodedImage mEncodedImage;
};

class WebrtcOMXH264VideoDecoder : public WebrtcVideoDecoder
{
public:
  WebrtcOMXH264VideoDecoder();

  virtual ~WebrtcOMXH264VideoDecoder();

  
  virtual int32_t InitDecode(const webrtc::VideoCodec* aCodecSettings,
                             int32_t aNumOfCores) MOZ_OVERRIDE;
  virtual int32_t Decode(const webrtc::EncodedImage& aInputImage,
                         bool aMissingFrames,
                         const webrtc::RTPFragmentationHeader* aFragmentation,
                         const webrtc::CodecSpecificInfo* aCodecSpecificInfo = nullptr,
                         int64_t aRenderTimeMs = -1) MOZ_OVERRIDE;
  virtual int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) MOZ_OVERRIDE;

  virtual int32_t Release() MOZ_OVERRIDE;

  virtual int32_t Reset() MOZ_OVERRIDE;

private:
  webrtc::DecodedImageCallback* mCallback;
  RefPtr<WebrtcOMXDecoder> mOMX;
};

}

#endif 
