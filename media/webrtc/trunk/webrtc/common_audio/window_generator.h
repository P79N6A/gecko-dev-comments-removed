









#ifndef WEBRTC_COMMON_AUDIO_WINDOW_GENERATOR_H_
#define WEBRTC_COMMON_AUDIO_WINDOW_GENERATOR_H_

#include "webrtc/base/constructormagic.h"

namespace webrtc {


class WindowGenerator {
 public:
  static void Hanning(int length, float* window);
  static void KaiserBesselDerived(float alpha, int length, float* window);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(WindowGenerator);
};

}  

#endif  

