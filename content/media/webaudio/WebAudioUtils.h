





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
class MediaStream;

namespace dom {

class AudioParamTimeline;

struct WebAudioUtils {
  
  
  static const uint32_t MaxChannelCount = 32;

  static bool FuzzyEqual(float v1, float v2)
  {
    using namespace std;
    return fabsf(v1 - v2) < 1e-7f;
  }
  static bool FuzzyEqual(double v1, double v2)
  {
    using namespace std;
    return fabs(v1 - v2) < 1e-7;
  }

  



  static double ComputeSmoothingRate(double aDuration, double aSampleRate)
  {
    return 1.0 - std::exp(-1.0 / (aDuration * aSampleRate));
  }

  



  static TrackTicks
  ConvertDestinationStreamTimeToSourceStreamTime(double aTime,
                                                 AudioNodeStream* aSource,
                                                 MediaStream* aDestination);

  








  static void ConvertAudioParamToTicks(AudioParamTimeline& aParam,
                                       AudioNodeStream* aSource,
                                       AudioNodeStream* aDest);

  



  static float ConvertLinearToDecibels(float aLinearValue, float aMinDecibels)
  {
    return aLinearValue ? 20.0f * std::log10(aLinearValue) : aMinDecibels;
  }

  


  static float ConvertDecibelsToLinear(float aDecibels)
  {
    return std::pow(10.0f, 0.05f * aDecibels);
  }

  


  static float ConvertDecibelToLinear(float aDecibel)
  {
    return std::pow(10.0f, 0.05f * aDecibel);
  }

  static void FixNaN(double& aDouble)
  {
    if (IsNaN(aDouble) || IsInfinite(aDouble)) {
      aDouble = 0.0;
    }
  }

  static double DiscreteTimeConstantForSampleRate(double timeConstant, double sampleRate)
  {
    return 1.0 - std::exp(-1.0 / (sampleRate * timeConstant));
  }

  static bool IsTimeValid(double aTime)
  {
    return aTime >= 0 &&  aTime <= (MEDIA_TIME_MAX >> MEDIA_TIME_FRAC_BITS);
  }

  



  static double StreamPositionToDestinationTime(TrackTicks aSourcePosition,
                                                AudioNodeStream* aSource,
                                                AudioNodeStream* aDestination);

  





























































  template <typename IntType, typename FloatType>
  static IntType TruncateFloatToInt(FloatType f)
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

  static void Shutdown();

  static int
  SpeexResamplerProcess(SpeexResamplerState* aResampler,
                        uint32_t aChannel,
                        const float* aIn, uint32_t* aInLen,
                        float* aOut, uint32_t* aOutLen);

  static int
  SpeexResamplerProcess(SpeexResamplerState* aResampler,
                        uint32_t aChannel,
                        const int16_t* aIn, uint32_t* aInLen,
                        float* aOut, uint32_t* aOutLen);
};

}
}

#endif

