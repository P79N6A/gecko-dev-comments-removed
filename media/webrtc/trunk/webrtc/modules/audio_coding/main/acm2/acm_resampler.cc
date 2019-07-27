









#include "webrtc/modules/audio_coding/main/acm2/acm_resampler.h"

#include <assert.h>
#include <string.h>

#include "webrtc/common_audio/resampler/include/resampler.h"
#include "webrtc/system_wrappers/interface/logging.h"

namespace webrtc {
namespace acm2 {

ACMResampler::ACMResampler() {
}

ACMResampler::~ACMResampler() {
}

int ACMResampler::Resample10Msec(const int16_t* in_audio,
                                 int in_freq_hz,
                                 int out_freq_hz,
                                 int num_audio_channels,
                                 int out_capacity_samples,
                                 int16_t* out_audio) {
  int in_length = in_freq_hz * num_audio_channels / 100;
  int out_length = out_freq_hz * num_audio_channels / 100;
  if (in_freq_hz == out_freq_hz) {
    if (out_capacity_samples < in_length) {
      assert(false);
      return -1;
    }
    memcpy(out_audio, in_audio, in_length * sizeof(int16_t));
    return in_length / num_audio_channels;
  }

  if (resampler_.InitializeIfNeeded(in_freq_hz, out_freq_hz,
                                    num_audio_channels) != 0) {
    LOG_FERR3(LS_ERROR, InitializeIfNeeded, in_freq_hz, out_freq_hz,
              num_audio_channels);
    return -1;
  }

  out_length =
      resampler_.Resample(in_audio, in_length, out_audio, out_capacity_samples);
  if (out_length == -1) {
    LOG_FERR4(LS_ERROR,
              Resample,
              in_audio,
              in_length,
              out_audio,
              out_capacity_samples);
    return -1;
  }

  return out_length / num_audio_channels;
}

}  
}  
