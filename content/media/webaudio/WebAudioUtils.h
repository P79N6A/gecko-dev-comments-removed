





#ifndef WebAudioUtils_h_
#define WebAudioUtils_h_

#include <cmath>
#include "AudioParamTimeline.h"

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

  








  static void ConvertAudioParamToTicks(AudioParamTimeline& aParam,
                                       AudioNodeStream* aSource,
                                       AudioNodeStream* aDest);

  



  static float ConvertLinearToDecibels(float aLinearValue, float aMinDecibels)
  {
    return aLinearValue ? 20.0f * std::log10(aLinearValue) : aMinDecibels;
  }

  


  static float ConvertDecibelToLinear(float aDecibel)
  {
    return std::pow(10.0f, 0.05f * aDecibel);
  }
};

}
}

#endif

