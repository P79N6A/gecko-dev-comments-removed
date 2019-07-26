





#ifndef AnalyserNode_h_
#define AnalyserNode_h_

#include "AudioNode.h"
#include "FFTBlock.h"

namespace mozilla {
namespace dom {

class AudioContext;

class AnalyserNode : public AudioNode
{
public:
  explicit AnalyserNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void GetFloatFrequencyData(const Float32Array& aArray);
  void GetByteFrequencyData(const Uint8Array& aArray);
  void GetByteTimeDomainData(const Uint8Array& aArray);
  uint32_t FftSize() const
  {
    return mAnalysisBlock.FFTSize();
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
  friend class AnalyserNodeEngine;
  void AppendChunk(const AudioChunk& aChunk);
  bool AllocateBuffer();
  bool FFTAnalysis();
  void ApplyBlackmanWindow(float* aBuffer, uint32_t aSize);

private:
  FFTBlock mAnalysisBlock;
  double mMinDecibels;
  double mMaxDecibels;
  double mSmoothingTimeConstant;
  uint32_t mWriteIndex;
  FallibleTArray<float> mBuffer;
  FallibleTArray<float> mOutputBuffer;
};

}
}

#endif

