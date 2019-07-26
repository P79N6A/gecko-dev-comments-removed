





#ifndef WebAudioUtils_h_
#define WebAudioUtils_h_

#include <cmath>

namespace mozilla {

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
};

}
}

#endif

