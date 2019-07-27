




#ifndef GMPAudioDecoderChild_h_
#define GMPAudioDecoderChild_h_

#include "mozilla/gmp/PGMPAudioDecoderChild.h"
#include "gmp-audio-decode.h"
#include "GMPAudioHost.h"

namespace mozilla {
namespace gmp {

class GMPContentChild;

class GMPAudioDecoderChild : public PGMPAudioDecoderChild,
                             public GMPAudioDecoderCallback
{
public:
  explicit GMPAudioDecoderChild(GMPContentChild* aPlugin);
  virtual ~GMPAudioDecoderChild();

  void Init(GMPAudioDecoder* aDecoder);
  GMPAudioHostImpl& Host();

  
  virtual void Decoded(GMPAudioSamples* aEncodedSamples) override;
  virtual void InputDataExhausted() override;
  virtual void DrainComplete() override;
  virtual void ResetComplete() override;
  virtual void Error(GMPErr aError) override;

private:
  
  virtual bool RecvInitDecode(const GMPAudioCodecData& codecSettings) override;
  virtual bool RecvDecode(const GMPAudioEncodedSampleData& input) override;
  virtual bool RecvReset() override;
  virtual bool RecvDrain() override;
  virtual bool RecvDecodingComplete() override;

  GMPContentChild* mPlugin;
  GMPAudioDecoder* mAudioDecoder;
  GMPAudioHostImpl mAudioHost;
};

} 
} 

#endif 
