




#ifndef GMPVideoEncoderProxy_h_
#define GMPVideoEncoderProxy_h_

#include "nsTArray.h"
#include "gmp-video-encode.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"

#include "GMPCallbackBase.h"
#include "GMPUtils.h"

class GMPVideoEncoderCallbackProxy : public GMPCallbackBase {
public:
  virtual ~GMPVideoEncoderCallbackProxy() {}
  virtual void Encoded(GMPVideoEncodedFrame* aEncodedFrame,
                       const nsTArray<uint8_t>& aCodecSpecificInfo) = 0;
  virtual void Error(GMPErr aError) = 0;
};












class GMPVideoEncoderProxy {
public:
  virtual GMPErr InitEncode(const GMPVideoCodec& aCodecSettings,
                            const nsTArray<uint8_t>& aCodecSpecific,
                            GMPVideoEncoderCallbackProxy* aCallback,
                            int32_t aNumberOfCores,
                            uint32_t aMaxPayloadSize) = 0;
  virtual GMPErr Encode(mozilla::GMPUniquePtr<GMPVideoi420Frame> aInputFrame,
                        const nsTArray<uint8_t>& aCodecSpecificInfo,
                        const nsTArray<GMPVideoFrameType>& aFrameTypes) = 0;
  virtual GMPErr SetChannelParameters(uint32_t aPacketLoss, uint32_t aRTT) = 0;
  virtual GMPErr SetRates(uint32_t aNewBitRate, uint32_t aFrameRate) = 0;
  virtual GMPErr SetPeriodicKeyFrames(bool aEnable) = 0;
  virtual const uint32_t GetPluginId() const = 0;

  
  
  virtual void Close() = 0;
};

#endif 
