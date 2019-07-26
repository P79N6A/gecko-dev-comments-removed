





#ifndef WebAudioUtils_h_
#define WebAudioUtils_h_

#include <cmath>
#include <limits>
#include "mozilla/TypeTraits.h"
#include "mozilla/FloatingPoint.h"
#include "MediaSegment.h"


typedef struct SpeexResamplerState_ SpeexResamplerState;

namespace mozilla {

class AudioNodeStream;

namespace dom {

class AudioParamTimeline;

namespace WebAudioUtils {
  
  
  
  const size_t MaxChannelCount = 32;
  
  
  const uint32_t MinSampleRate = 8000;
  const uint32_t MaxSampleRate = 192000;

  inline bool FuzzyEqual(float v1, float v2)
  {
    using namespace std;
    return fabsf(v1 - v2) < 1e-7f;
  }
  inline bool FuzzyEqual(double v1, double v2)
  {
    using namespace std;
    return fabs(v1 - v2) < 1e-7;
  }

  



  inline double ComputeSmoothingRate(double aDuration, double aSampleRate)
  {
    return 1.0 - std::exp(-1.0 / (aDuration * aSampleRate));
  }

  








  void ConvertAudioParamToTicks(AudioParamTimeline& aParam,
                                AudioNodeStream* aSource,
                                AudioNodeStream* aDest);

  



  inline float ConvertLinearToDecibels(float aLinearValue, float aMinDecibels)
  {
    return aLinearValue ? 20.0f * std::log10(aLinearValue) : aMinDecibels;
  }

  


  inline float ConvertDecibelsToLinear(float aDecibels)
  {
    return std::pow(10.0f, 0.05f * aDecibels);
  }

  


  inline float ConvertDecibelToLinear(float aDecibel)
  {
    return std::pow(10.0f, 0.05f * aDecibel);
  }

  inline void FixNaN(double& aDouble)
  {
    if (IsNaN(aDouble) || IsInfinite(aDouble)) {
      aDouble = 0.0;
    }
  }

  inline double DiscreteTimeConstantForSampleRate(double timeConstant, double sampleRate)
  {
    return 1.0 - std::exp(-1.0 / (sampleRate * timeConstant));
  }

  inline bool IsTimeValid(double aTime)
  {
    return aTime >= 0 &&  aTime <= (MEDIA_TIME_MAX >> MEDIA_TIME_FRAC_BITS);
  }

  





























































  template <typename IntType, typename FloatType>
  IntType TruncateFloatToInt(FloatType f)
  {
    using namespace std;

    static_assert(mozilla::IsIntegral<IntType>::value == true,
                  "IntType must be an integral type");
    static_assert(mozilla::IsFloatingPoint<FloatType>::value == true,
                  "FloatType must be a floating point type");

    if (f != f) {
      
      
      NS_RUNTIMEABORT("We should never see a NaN here");
    }

    if (f > FloatType(numeric_limits<IntType>::max())) {
      
      
      return numeric_limits<IntType>::max();
    }

    if (f < FloatType(numeric_limits<IntType>::min())) {
      
      
      return numeric_limits<IntType>::min();
    }

    
    return IntType(f);
  }

  void Shutdown();

  int
  SpeexResamplerProcess(SpeexResamplerState* aResampler,
                        uint32_t aChannel,
                        const float* aIn, uint32_t* aInLen,
                        float* aOut, uint32_t* aOutLen);

  int
  SpeexResamplerProcess(SpeexResamplerState* aResampler,
                        uint32_t aChannel,
                        const int16_t* aIn, uint32_t* aInLen,
                        float* aOut, uint32_t* aOutLen);

  int
  SpeexResamplerProcess(SpeexResamplerState* aResampler,
                        uint32_t aChannel,
                        const int16_t* aIn, uint32_t* aInLen,
                        int16_t* aOut, uint32_t* aOutLen);
  }

}
}

#endif

