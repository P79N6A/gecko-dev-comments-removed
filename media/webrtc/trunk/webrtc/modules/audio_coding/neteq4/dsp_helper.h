









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DSP_HELPER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DSP_HELPER_H_

#include <string.h>  

#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {



class DspHelper {
 public:
  
  
  static const int16_t kDownsample8kHzTbl[3];
  static const int16_t kDownsample16kHzTbl[5];
  static const int16_t kDownsample32kHzTbl[7];
  static const int16_t kDownsample48kHzTbl[7];

  
  
  static const int kMuteFactorStart8kHz = 27307;
  static const int kMuteFactorIncrement8kHz = -5461;
  static const int kUnmuteFactorStart8kHz = 5461;
  static const int kUnmuteFactorIncrement8kHz = 5461;
  static const int kMuteFactorStart16kHz = 29789;
  static const int kMuteFactorIncrement16kHz = -2979;
  static const int kUnmuteFactorStart16kHz = 2979;
  static const int kUnmuteFactorIncrement16kHz = 2979;
  static const int kMuteFactorStart32kHz = 31208;
  static const int kMuteFactorIncrement32kHz = -1560;
  static const int kUnmuteFactorStart32kHz = 1560;
  static const int kUnmuteFactorIncrement32kHz = 1560;
  static const int kMuteFactorStart48kHz = 31711;
  static const int kMuteFactorIncrement48kHz = -1057;
  static const int kUnmuteFactorStart48kHz = 1057;
  static const int kUnmuteFactorIncrement48kHz = 1057;

  
  
  
  
  static int RampSignal(const int16_t* input,
                        size_t length,
                        int factor,
                        int increment,
                        int16_t* output);

  
  static int RampSignal(int16_t* signal,
                        size_t length,
                        int factor,
                        int increment);

  
  
  static int RampSignal(AudioMultiVector* signal,
                        size_t start_index,
                        size_t length,
                        int factor,
                        int increment);

  
  
  
  
  
  static void PeakDetection(int16_t* data, int data_length,
                            int num_peaks, int fs_mult,
                            int* peak_index, int16_t* peak_value);

  
  
  
  
  
  
  static void ParabolicFit(int16_t* signal_points, int fs_mult,
                           int* peak_index, int16_t* peak_value);

  
  
  
  
  static int MinDistortion(const int16_t* signal, int min_lag,
                           int max_lag, int length, int32_t* distortion_value);

  
  
  
  
  static void CrossFade(const int16_t* input1, const int16_t* input2,
                        size_t length, int16_t* mix_factor,
                        int16_t factor_decrement, int16_t* output);

  
  
  
  static void UnmuteSignal(const int16_t* input, size_t length, int16_t* factor,
                           int16_t increment, int16_t* output);

  
  
  static void MuteSignal(int16_t* signal, int16_t mute_slope, size_t length);

  
  
  
  
  
  static int DownsampleTo4kHz(const int16_t* input, size_t input_length,
                              int output_length, int input_rate_hz,
                              bool compensate_delay, int16_t* output);

 private:
  
  static const int16_t kParabolaCoefficients[17][3];

  DISALLOW_COPY_AND_ASSIGN(DspHelper);
};

}  
#endif  
