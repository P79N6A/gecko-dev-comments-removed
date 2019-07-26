





#ifndef WebAudioUtils_h_
#define WebAudioUtils_h_

#include <cmath>
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
    if (MOZ_DOUBLE_IS_NaN(aDouble) || MOZ_DOUBLE_IS_INFINITE(aDouble)) {
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
};

}
}

#endif

