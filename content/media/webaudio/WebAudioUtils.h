





#ifndef WebAudioUtils_h_
#define WebAudioUtils_h_

#include <cmath>
#include <limits>
#include "mozilla/TypeTraits.h"
#include "mozilla/Assertions.h"
#include "AudioParamTimeline.h"
#include "MediaSegment.h"

namespace mozilla {

class AudioNodeStream;

namespace dom {

struct WebAudioUtils {
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
                                                 MediaStream* aSource,
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

  



  static double StreamPositionToDestinationTime(TrackTicks aSourcePosition,
                                                AudioNodeStream* aSource,
                                                AudioNodeStream* aDestination);

  





























































  template <typename IntType, typename FloatType>
  static IntType TruncateFloatToInt(FloatType f)
  {
    using namespace std;

    MOZ_STATIC_ASSERT((mozilla::IsIntegral<IntType>::value == true),
                      "IntType must be an integral type");
    MOZ_STATIC_ASSERT((mozilla::IsFloatingPoint<FloatType>::value == true),
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
};

}
}

#endif

