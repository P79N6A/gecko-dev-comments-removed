




#ifndef GMPVideoDecoderProxy_h_
#define GMPVideoDecoderProxy_h_

#include "nsTArray.h"
#include "gmp-video-decode.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"

#include "GMPCallbackBase.h"
#include "GMPUtils.h"

class GMPVideoDecoderCallbackProxy : public GMPCallbackBase,
                                     public GMPVideoDecoderCallback
{
public:
  virtual ~GMPVideoDecoderCallbackProxy() {}
};












class GMPVideoDecoderProxy {
public:
  virtual nsresult InitDecode(const GMPVideoCodec& aCodecSettings,
                              const nsTArray<uint8_t>& aCodecSpecific,
                              GMPVideoDecoderCallbackProxy* aCallback,
                              int32_t aCoreCount) = 0;
  virtual nsresult Decode(mozilla::GMPUnique<GMPVideoEncodedFrame> aInputFrame,
                          bool aMissingFrames,
                          const nsTArray<uint8_t>& aCodecSpecificInfo,
                          int64_t aRenderTimeMs = -1) = 0;
  virtual nsresult Reset() = 0;
  virtual nsresult Drain() = 0;
  virtual const uint64_t ParentID() = 0;

  
  
  virtual void Close() = 0;

  virtual const nsCString& GetDisplayName() const = 0;
};

#endif
