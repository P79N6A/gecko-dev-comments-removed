




#ifndef GMPAudioDecoderProxy_h_
#define GMPAudioDecoderProxy_h_

#include "GMPCallbackBase.h"
#include "gmp-audio-codec.h"
#include "GMPAudioHost.h"
#include "nsTArray.h"
#include "mozilla/gmp/GMPTypes.h"

class GMPAudioDecoderProxyCallback : public GMPCallbackBase {
public:
  virtual ~GMPAudioDecoderProxyCallback() {}
  virtual void Decoded(const nsTArray<int16_t>& aPCM, uint64_t aTimeStamp) = 0;
  virtual void InputDataExhausted() = 0;
  virtual void DrainComplete() = 0;
  virtual void ResetComplete() = 0;
  virtual void Error(GMPErr aError) = 0;
};

class GMPAudioDecoderProxy {
public:
  virtual ~GMPAudioDecoderProxy() {}

  virtual nsresult InitDecode(GMPAudioCodecType aCodecType,
                              uint32_t aChannelCount,
                              uint32_t aBitsPerChannel,
                              uint32_t aSamplesPerSecond,
                              nsTArray<uint8_t>& aExtraData,
                              GMPAudioDecoderProxyCallback* aCallback) = 0;
  virtual nsresult Decode(mozilla::gmp::GMPAudioSamplesImpl& aSamples) = 0;
  virtual nsresult Reset() = 0;
  virtual nsresult Drain() = 0;
  
  
  virtual nsresult Close() = 0;
};

#endif 
