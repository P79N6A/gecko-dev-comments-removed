





#ifndef FFTBlock_h_
#define FFTBlock_h_

#ifdef BUILD_ARM_NEON
#include <cmath>
#include "mozilla/arm.h"
#include "dl/sp/api/omxSP.h"
#endif

#include "AlignedTArray.h"
#include "AudioNodeEngine.h"
#if defined(MOZ_LIBAV_FFT)
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avfft.h"
#ifdef __cplusplus
}
#endif
#else
#include "kiss_fft/kiss_fftr.h"
#endif

namespace mozilla {




class FFTBlock final
{
  union ComplexU {
#if !defined(MOZ_LIBAV_FFT)
    kiss_fft_cpx c;
#endif
    float f[2];
    struct {
      float r;
      float i;
    };
  };

public:
  explicit FFTBlock(uint32_t aFFTSize)
#if defined(MOZ_LIBAV_FFT)
    : mAvRDFT(nullptr)
    , mAvIRDFT(nullptr)
#else
    : mKissFFT(nullptr)
    , mKissIFFT(nullptr)
#ifdef BUILD_ARM_NEON
    , mOmxFFT(nullptr)
    , mOmxIFFT(nullptr)
#endif
#endif
  {
    MOZ_COUNT_CTOR(FFTBlock);
    SetFFTSize(aFFTSize);
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
#if defined(MOZ_LIBAV_FFT)
    AlignedTArray<FFTSample> complex(mFFTSize);
    PodCopy(complex.Elements(), aData, mFFTSize);
    av_rdft_calc(mAvRDFT, complex.Elements());
    PodCopy((FFTSample*)mOutputBuffer.Elements(), complex.Elements(), mFFTSize);
#else
#ifdef BUILD_ARM_NEON
    if (mozilla::supports_neon()) {
      omxSP_FFTFwd_RToCCS_F32_Sfs(aData, mOutputBuffer.Elements()->f, mOmxFFT);
    } else
#endif
    {
      kiss_fftr(mKissFFT, aData, &(mOutputBuffer.Elements()->c));
    }
#endif
  }
  
  
  void GetInverse(float* aDataOut)
  {
    GetInverseWithoutScaling(aDataOut);
    AudioBufferInPlaceScale(aDataOut, 1.0f / mFFTSize, mFFTSize);
  }
  
  
  
  void GetInverseWithoutScaling(float* aDataOut)
  {
    EnsureIFFT();
#if defined(MOZ_LIBAV_FFT)
    {
      PodCopy(aDataOut, (float*)mOutputBuffer.Elements(), mFFTSize);
      av_rdft_calc(mAvIRDFT, aDataOut);
      
      
      
      
      for (uint32_t i = 0; i < mFFTSize; ++i) {
        aDataOut[i] *= 2.0;
      }
    }
#else
#ifdef BUILD_ARM_NEON
    if (mozilla::supports_neon()) {
      omxSP_FFTInv_CCSToR_F32_Sfs(mOutputBuffer.Elements()->f, aDataOut, mOmxIFFT);
      
      
      AudioBufferInPlaceScale(aDataOut, mFFTSize, mFFTSize);
    } else
#endif
    {
      kiss_fftri(mKissIFFT, &(mOutputBuffer.Elements()->c), aDataOut);
    }
#endif
  }
  
  
  
  void PerformInverseFFT(float* aRealDataIn,
                         float *aImagDataIn,
                         float *aRealDataOut)
  {
    EnsureIFFT();
    const uint32_t inputSize = mFFTSize / 2 + 1;
#if defined(MOZ_LIBAV_FFT)
    AlignedTArray<FFTSample> inputBuffer(inputSize * 2);
    for (uint32_t i = 0; i < inputSize; ++i) {
      inputBuffer[2*i] = aRealDataIn[i];
      inputBuffer[(2*i)+1] = aImagDataIn[i];
    }
    av_rdft_calc(mAvIRDFT, inputBuffer.Elements());
    PodCopy(aRealDataOut, inputBuffer.Elements(), FFTSize());
    
    for (uint32_t i = 0; i < mFFTSize; ++i) {
      aRealDataOut[i] /= mFFTSize;
    }
#else
    AlignedTArray<ComplexU> inputBuffer(inputSize);
    for (uint32_t i = 0; i < inputSize; ++i) {
      inputBuffer[i].r = aRealDataIn[i];
      inputBuffer[i].i = aImagDataIn[i];
    }
#if defined(BUILD_ARM_NEON)
    if (mozilla::supports_neon()) {
      omxSP_FFTInv_CCSToR_F32_Sfs(inputBuffer.Elements()->f,
                                  aRealDataOut, mOmxIFFT);
    } else
#endif
    {
      kiss_fftri(mKissIFFT, &(inputBuffer.Elements()->c), aRealDataOut);
      for (uint32_t i = 0; i < mFFTSize; ++i) {
        aRealDataOut[i] /= mFFTSize;
      }
    }
#endif
  }

  void Multiply(const FFTBlock& aFrame)
  {
    BufferComplexMultiply(mOutputBuffer.Elements()->f,
                          aFrame.mOutputBuffer.Elements()->f,
                          mOutputBuffer.Elements()->f,
                          mFFTSize / 2 + 1);
  }

  
  
  
  
  void PadAndMakeScaledDFT(const float* aData, size_t dataSize)
  {
    MOZ_ASSERT(dataSize <= FFTSize());
    AlignedTArray<float> paddedData;
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
#if defined(MOZ_LIBAV_FFT)
    amount += aMallocSizeOf(mAvRDFT);
    amount += aMallocSizeOf(mAvIRDFT);
#else
    amount += aMallocSizeOf(mKissFFT);
    amount += aMallocSizeOf(mKissIFFT);
#endif
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
#if defined(MOZ_LIBAV_FFT)
    if (!mAvRDFT) {
      mAvRDFT = av_rdft_init(log((double)mFFTSize)/M_LN2, DFT_R2C);
    }
#else
#ifdef BUILD_ARM_NEON
    if (mozilla::supports_neon()) {
      if (!mOmxFFT) {
        mOmxFFT = createOmxFFT(mFFTSize);
      }
    } else
#endif
    {
      if (!mKissFFT) {
        mKissFFT = kiss_fftr_alloc(mFFTSize, 0, nullptr, nullptr);
      }
    }
#endif
  }
  void EnsureIFFT()
  {
#if defined(MOZ_LIBAV_FFT)
    if (!mAvIRDFT) {
      mAvIRDFT = av_rdft_init(log((double)mFFTSize)/M_LN2, IDFT_C2R);
    }
#else
#ifdef BUILD_ARM_NEON
    if (mozilla::supports_neon()) {
      if (!mOmxIFFT) {
        mOmxIFFT = createOmxFFT(mFFTSize);
      }
    } else
#endif
    {
      if (!mKissIFFT) {
        mKissIFFT = kiss_fftr_alloc(mFFTSize, 1, nullptr, nullptr);
      }
    }
#endif
  }

#ifdef BUILD_ARM_NEON
  static OMXFFTSpec_R_F32* createOmxFFT(uint32_t aFFTSize)
  {
    MOZ_ASSERT((aFFTSize & (aFFTSize-1)) == 0);
    OMX_INT bufSize;
    OMX_INT order = log((double)aFFTSize)/M_LN2;
    MOZ_ASSERT(aFFTSize>>order == 1);
    OMXResult status = omxSP_FFTGetBufSize_R_F32(order, &bufSize);
    if (status == OMX_Sts_NoErr) {
      OMXFFTSpec_R_F32* context = static_cast<OMXFFTSpec_R_F32*>(malloc(bufSize));
      if (omxSP_FFTInit_R_F32(context, order) != OMX_Sts_NoErr) {
        return nullptr;
      }
      return context;
    }
    return nullptr;
  }
#endif

  void Clear()
  {
#if defined(MOZ_LIBAV_FFT)
    av_rdft_end(mAvRDFT);
    av_rdft_end(mAvIRDFT);
    mAvRDFT = mAvIRDFT = nullptr;
#else
#ifdef BUILD_ARM_NEON
    free(mOmxFFT);
    free(mOmxIFFT);
    mOmxFFT = mOmxIFFT = nullptr;
#endif
    free(mKissFFT);
    free(mKissIFFT);
    mKissFFT = mKissIFFT = nullptr;
#endif
  }
  void AddConstantGroupDelay(double sampleFrameDelay);
  void InterpolateFrequencyComponents(const FFTBlock& block0,
                                      const FFTBlock& block1, double interp);
#if defined(MOZ_LIBAV_FFT)
  RDFTContext *mAvRDFT;
  RDFTContext *mAvIRDFT;
#else
  kiss_fftr_cfg mKissFFT;
  kiss_fftr_cfg mKissIFFT;
#ifdef BUILD_ARM_NEON
  OMXFFTSpec_R_F32* mOmxFFT;
  OMXFFTSpec_R_F32* mOmxIFFT;
#endif
#endif
  AlignedTArray<ComplexU> mOutputBuffer;
  uint32_t mFFTSize;
};
}

#endif

