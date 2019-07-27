




#ifndef GMPAudioDecoderParent_h_
#define GMPAudioDecoderParent_h_

#include "mozilla/RefPtr.h"
#include "gmp-audio-decode.h"
#include "gmp-audio-codec.h"
#include "mozilla/gmp/PGMPAudioDecoderParent.h"
#include "GMPMessageUtils.h"
#include "GMPAudioDecoderProxy.h"

namespace mozilla {
namespace gmp {

class GMPParent;

class GMPAudioDecoderParent MOZ_FINAL : public GMPAudioDecoderProxy
                                      , public PGMPAudioDecoderParent
{
public:
  NS_INLINE_DECL_REFCOUNTING(GMPAudioDecoderParent)

  explicit GMPAudioDecoderParent(GMPParent *aPlugin);

  nsresult Shutdown();

  
  virtual nsresult InitDecode(GMPAudioCodecType aCodecType,
                              uint32_t aChannelCount,
                              uint32_t aBitsPerChannel,
                              uint32_t aSamplesPerSecond,
                              nsTArray<uint8_t>& aExtraData,
                              GMPAudioDecoderProxyCallback* aCallback) MOZ_OVERRIDE;
  virtual nsresult Decode(GMPAudioSamplesImpl& aInput) MOZ_OVERRIDE;
  virtual nsresult Reset() MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;
  virtual nsresult Close() MOZ_OVERRIDE;

private:
  ~GMPAudioDecoderParent();

  
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
  virtual bool RecvDecoded(const GMPAudioDecodedSampleData& aDecoded) MOZ_OVERRIDE;
  virtual bool RecvInputDataExhausted() MOZ_OVERRIDE;
  virtual bool RecvDrainComplete() MOZ_OVERRIDE;
  virtual bool RecvResetComplete() MOZ_OVERRIDE;
  virtual bool RecvError(const GMPErr& aError) MOZ_OVERRIDE;
  virtual bool Recv__delete__() MOZ_OVERRIDE;

  bool mIsOpen;
  nsRefPtr<GMPParent> mPlugin;
  GMPAudioDecoderProxyCallback* mCallback;
};

} 
} 

#endif 
