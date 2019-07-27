




#ifndef GMPAudioDecoderChild_h_
#define GMPAudioDecoderChild_h_

#include "mozilla/gmp/PGMPAudioDecoderChild.h"
#include "gmp-audio-decode.h"
#include "GMPAudioHost.h"

namespace mozilla {
namespace gmp {

class GMPChild;

class GMPAudioDecoderChild : public PGMPAudioDecoderChild,
                             public GMPAudioDecoderCallback
{
public:
  explicit GMPAudioDecoderChild(GMPChild* aPlugin);
  virtual ~GMPAudioDecoderChild();

  void Init(GMPAudioDecoder* aDecoder);
  GMPAudioHostImpl& Host();

  
  virtual void Decoded(GMPAudioSamples* aEncodedSamples) MOZ_OVERRIDE;
  virtual void InputDataExhausted() MOZ_OVERRIDE;
  virtual void DrainComplete() MOZ_OVERRIDE;
  virtual void ResetComplete() MOZ_OVERRIDE;
  virtual void Error(GMPErr aError) MOZ_OVERRIDE;

private:
  
  virtual bool RecvInitDecode(const GMPAudioCodecData& codecSettings) MOZ_OVERRIDE;
  virtual bool RecvDecode(const GMPAudioEncodedSampleData& input) MOZ_OVERRIDE;
  virtual bool RecvReset() MOZ_OVERRIDE;
  virtual bool RecvDrain() MOZ_OVERRIDE;
  virtual bool RecvDecodingComplete() MOZ_OVERRIDE;

  GMPChild* mPlugin;
  GMPAudioDecoder* mAudioDecoder;
  GMPAudioHostImpl mAudioHost;
};

} 
} 

#endif 
