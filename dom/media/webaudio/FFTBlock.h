





#ifndef FFTBlock_h_
#define FFTBlock_h_

#include "nsTArray.h"
#include "AudioNodeEngine.h"
#include "kiss_fft/kiss_fftr.h"

namespace mozilla {




class FFTBlock final
{
public:
  explicit FFTBlock(uint32_t aFFTSize)
    : mFFT(nullptr)
    , mIFFT(nullptr)
    , mFFTSize(aFFTSize)
  {
    MOZ_COUNT_CTOR(FFTBlock);
    mOutputBuffer.SetLength(aFFTSize / 2 + 1);
    PodZero(mOutputBuffer.Elements(), aFFTSize / 2 + 1);
  }
  ~FFTBlock()
  {
    MOZ_COUNT_DTOR(FFTBlock);
    Clear();
  }

  
  
  static FFTBlock*
  CreateInterpolatedBlock(const FFTBlock& block0,
                          const FFTBlock& block1, double interp);

  
  void PerformFFT(const float* aData)
  {
    EnsureFFT();
    kiss_fftr(mFFT, aData, mOutputBuffer.Elements());
  }
  
  
  void GetInverse(float* aDataOut)
  {
    GetInverseWithoutScaling(aDataOut);
    AudioBufferInPlaceScale(aDataOut, 1.0f / mFFTSize, mFFTSize);
  }
  
  
  
  void GetInverseWithoutScaling(float* aDataOut)
  {
    EnsureIFFT();
    kiss_fftri(mIFFT, mOutputBuffer.Elements(), aDataOut);
  }
  
  
  
  void PerformInverseFFT(float* aRealDataIn,
                         float *aImagDataIn,
                         float *aRealDataOut)
  {
    EnsureIFFT();
    const uint32_t inputSize = mFFTSize / 2 + 1;
    nsTArray<kiss_fft_cpx> inputBuffer;
    inputBuffer.SetLength(inputSize);
    for (uint32_t i = 0; i < inputSize; ++i) {
      inputBuffer[i].r = aRealDataIn[i];
      inputBuffer[i].i = aImagDataIn[i];
    }
    kiss_fftri(mIFFT, inputBuffer.Elements(), aRealDataOut);
    for (uint32_t i = 0; i < mFFTSize; ++i) {
      aRealDataOut[i] /= mFFTSize;
    }
  }

  void Multiply(const FFTBlock& aFrame)
  {
    BufferComplexMultiply(reinterpret_cast<const float*>(mOutputBuffer.Elements()),
                          reinterpret_cast<const float*>(aFrame.mOutputBuffer.Elements()),
                          reinterpret_cast<float*>(mOutputBuffer.Elements()),
                          mFFTSize / 2 + 1);
  }

  
  
  
  
  void PadAndMakeScaledDFT(const float* aData, size_t dataSize)
  {
    MOZ_ASSERT(dataSize <= FFTSize());
    nsTArray<float> paddedData;
    paddedData.SetLength(FFTSize());
    AudioBufferCopyWithScale(aData, 1.0f / FFTSize(),
                             paddedData.Elements(), dataSize);
    PodZero(paddedData.Elements() + dataSize, mFFTSize - dataSize);
    PerformFFT(paddedData.Elements());
  }

  void SetFFTSize(uint32_t aSize)
  {
    mFFTSize = aSize;
    mOutputBuffer.SetLength(aSize / 2 + 1);
    PodZero(mOutputBuffer.Elements(), aSize / 2 + 1);
    Clear();
  }

  
  double ExtractAverageGroupDelay();

  uint32_t FFTSize() const
  {
    return mFFTSize;
  }
  float RealData(uint32_t aIndex) const
  {
    return mOutputBuffer[aIndex].r;
  }
  float ImagData(uint32_t aIndex) const
  {
    return mOutputBuffer[aIndex].i;
  }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    size_t amount = 0;
    amount += aMallocSizeOf(mFFT);
    amount += aMallocSizeOf(mIFFT);
    amount += mOutputBuffer.SizeOfExcludingThis(aMallocSizeOf);
    return amount;
  }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  FFTBlock(const FFTBlock& other) = delete;
  void operator=(const FFTBlock& other) = delete;

  void EnsureFFT()
  {
    if (!mFFT) {
      mFFT = kiss_fftr_alloc(mFFTSize, 0, nullptr, nullptr);
    }
  }
  void EnsureIFFT()
  {
    if (!mIFFT) {
      mIFFT = kiss_fftr_alloc(mFFTSize, 1, nullptr, nullptr);
    }
  }
  void Clear()
  {
    free(mFFT);
    free(mIFFT);
    mFFT = mIFFT = nullptr;
  }
  void AddConstantGroupDelay(double sampleFrameDelay);
  void InterpolateFrequencyComponents(const FFTBlock& block0,
                                      const FFTBlock& block1, double interp);

  kiss_fftr_cfg mFFT, mIFFT;
  nsTArray<kiss_fft_cpx> mOutputBuffer;
  uint32_t mFFTSize;
};

}

#endif

