









#define _USE_MATH_DEFINES

#include "webrtc/common_audio/window_generator.h"

#include <cmath>
#include <complex>

#include "webrtc/base/checks.h"

using std::complex;

namespace {


complex<float> I0(complex<float> x) {
  complex<float> y = x / 3.75f;
  y *= y;
  return 1.0f + y * (
    3.5156229f + y * (
      3.0899424f + y * (
        1.2067492f + y * (
          0.2659732f + y * (
            0.360768e-1f + y * 0.45813e-2f)))));
}

}  

namespace webrtc {

void WindowGenerator::Hanning(int length, float* window) {
  CHECK_GT(length, 1);
  CHECK(window != nullptr);
  for (int i = 0; i < length; ++i) {
    window[i] = 0.5f * (1 - cosf(2 * static_cast<float>(M_PI) * i /
                                 (length - 1)));
  }
}

void WindowGenerator::KaiserBesselDerived(float alpha, int length,
                                          float* window) {
  CHECK_GT(length, 1);
  CHECK(window != nullptr);

  const int half = (length + 1) / 2;
  float sum = 0.0f;

  for (int i = 0; i <= half; ++i) {
    complex<float> r = (4.0f * i) / length - 1.0f;
    sum += I0(static_cast<float>(M_PI) * alpha * sqrt(1.0f - r * r)).real();
    window[i] = sum;
  }
  for (int i = length - 1; i >= half; --i) {
    window[length - i - 1] = sqrtf(window[length - i - 1] / sum);
    window[i] = window[length - i - 1];
  }
  if (length % 2 == 1) {
    window[half - 1] = sqrtf(window[half - 1] / sum);
  }
}

}  

