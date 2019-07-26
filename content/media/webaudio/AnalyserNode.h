





#ifndef AnalyserNode_h_
#define AnalyserNode_h_

#include "AudioNode.h"

namespace mozilla {
namespace dom {

class AudioContext;

class AnalyserNode : public AudioNode
{
public:
  explicit AnalyserNode(AudioContext* aContext);
  virtual ~AnalyserNode();

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  virtual bool SupportsMediaStreams() const MOZ_OVERRIDE
  {
    return true;
  }

  void GetFloatFrequencyData(Float32Array& aArray);
  void GetByteFrequencyData(Uint8Array& aArray);
  void GetByteTimeDomainData(Uint8Array& aArray);
  uint32_t FftSize() const
  {
    return mFFTSize;
  }
  void SetFftSize(uint32_t aValue, ErrorResult& aRv);
  uint32_t FrequencyBinCount() const
  {
    return FftSize() / 2;
  }
  double MinDecibels() const
  {
    return mMinDecibels;
  }
  void SetMinDecibels(double aValue, ErrorResult& aRv);
  double MaxDecibels() const
  {
    return mMaxDecibels;
  }
  void SetMaxDecibels(double aValue, ErrorResult& aRv);
  double SmoothingTimeConstant() const
  {
    return mSmoothingTimeConstant;
  }
  void SetSmoothingTimeConstant(double aValue, ErrorResult& aRv);

private:
  uint32_t mFFTSize;
  double mMinDecibels;
  double mMaxDecibels;
  double mSmoothingTimeConstant;
};

}
}

#endif

