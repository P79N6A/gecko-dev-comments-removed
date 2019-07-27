















#ifndef WEBRTCGMPVIDEOCODEC_H_
#define WEBRTCGMPVIDEOCODEC_H_

#include <iostream>
#include <queue>

#include "nsThreadUtils.h"
#include "mozilla/Mutex.h"

#include "mozIGeckoMediaPluginService.h"
#include "MediaConduitInterface.h"
#include "AudioConduit.h"
#include "VideoConduit.h"
#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"

#include "gmp-video-host.h"
#include "GMPVideoDecoderProxy.h"
#include "GMPVideoEncoderProxy.h"

namespace mozilla {

class WebrtcGmpVideoEncoder : public WebrtcVideoEncoder,
                              public GMPVideoEncoderCallbackProxy
{
public:
  WebrtcGmpVideoEncoder();
  virtual ~WebrtcGmpVideoEncoder();

  
  virtual const uint64_t PluginID() MOZ_OVERRIDE
  {
    return mGMP ? mGMP->ParentID() : mCachedPluginId;
  }

  virtual void Terminated() MOZ_OVERRIDE;

  virtual int32_t InitEncode(const webrtc::VideoCodec* aCodecSettings,
                             int32_t aNumberOfCores,
                             uint32_t aMaxPayloadSize) MOZ_OVERRIDE;

  virtual int32_t Encode(const webrtc::I420VideoFrame& aInputImage,
                         const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
                         const std::vector<webrtc::VideoFrameType>* aFrameTypes) MOZ_OVERRIDE;

  virtual int32_t RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* aCallback) MOZ_OVERRIDE;

  virtual int32_t Release() MOZ_OVERRIDE;

  virtual int32_t SetChannelParameters(uint32_t aPacketLoss,
                                       int aRTT) MOZ_OVERRIDE;

  virtual int32_t SetRates(uint32_t aNewBitRate,
                           uint32_t aFrameRate) MOZ_OVERRIDE;

  
  virtual void Encoded(GMPVideoEncodedFrame* aEncodedFrame,
                       const nsTArray<uint8_t>& aCodecSpecificInfo) MOZ_OVERRIDE;

  virtual void Error(GMPErr aError) MOZ_OVERRIDE {
  }

private:
  virtual int32_t InitEncode_g(const webrtc::VideoCodec* aCodecSettings,
                               int32_t aNumberOfCores,
                               uint32_t aMaxPayloadSize);

  virtual int32_t Encode_g(const webrtc::I420VideoFrame* aInputImage,
                           const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
                           const std::vector<webrtc::VideoFrameType>* aFrameTypes);

  virtual int32_t SetRates_g(uint32_t aNewBitRate,
                             uint32_t aFrameRate);

  nsCOMPtr<mozIGeckoMediaPluginService> mMPS;
  nsCOMPtr<nsIThread> mGMPThread;
  GMPVideoEncoderProxy* mGMP;
  GMPVideoHost* mHost;
  GMPVideoCodec mCodecParams;
  uint32_t mMaxPayloadSize;
  webrtc::EncodedImageCallback* mCallback;
  uint64_t mCachedPluginId;
};


class WebrtcGmpVideoDecoder : public WebrtcVideoDecoder,
                              public GMPVideoDecoderCallbackProxy
{
public:
  WebrtcGmpVideoDecoder();
  virtual ~WebrtcGmpVideoDecoder();

  
  virtual const uint64_t PluginID() MOZ_OVERRIDE
  {
    return mGMP ? mGMP->ParentID() : mCachedPluginId;
  }

  virtual void Terminated() MOZ_OVERRIDE;

  virtual int32_t InitDecode(const webrtc::VideoCodec* aCodecSettings,
                             int32_t aNumberOfCores) MOZ_OVERRIDE;
  virtual int32_t Decode(const webrtc::EncodedImage& aInputImage,
                         bool aMissingFrames,
                         const webrtc::RTPFragmentationHeader* aFragmentation,
                         const webrtc::CodecSpecificInfo* aCodecSpecificInfo = nullptr,
                         int64_t aRenderTimeMs = -1) MOZ_OVERRIDE;
  virtual int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* aCallback) MOZ_OVERRIDE;

  virtual int32_t Release() MOZ_OVERRIDE;

  virtual int32_t Reset() MOZ_OVERRIDE;

  virtual void Decoded(GMPVideoi420Frame* aDecodedFrame) MOZ_OVERRIDE;

  virtual void ReceivedDecodedReferenceFrame(const uint64_t aPictureId) MOZ_OVERRIDE {
    MOZ_CRASH();
  }

  virtual void ReceivedDecodedFrame(const uint64_t aPictureId) MOZ_OVERRIDE {
    MOZ_CRASH();
  }

  virtual void InputDataExhausted() MOZ_OVERRIDE {
    MOZ_CRASH();
  }

  virtual void DrainComplete() MOZ_OVERRIDE {
    MOZ_CRASH();
  }

  virtual void ResetComplete() MOZ_OVERRIDE {
    MOZ_CRASH();
  }

  virtual void Error(GMPErr aError) MOZ_OVERRIDE {
  }

private:
  virtual int32_t InitDecode_g(const webrtc::VideoCodec* aCodecSettings,
                               int32_t aNumberOfCores);

  virtual int32_t Decode_g(const webrtc::EncodedImage& aInputImage,
                           bool aMissingFrames,
                           const webrtc::RTPFragmentationHeader* aFragmentation,
                           const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
                           int64_t aRenderTimeMs);

  nsCOMPtr<mozIGeckoMediaPluginService> mMPS;
  nsCOMPtr<nsIThread> mGMPThread;
  GMPVideoDecoderProxy* mGMP; 
  GMPVideoHost* mHost;
  webrtc::DecodedImageCallback* mCallback;
  uint64_t mCachedPluginId;
};

}

#endif
