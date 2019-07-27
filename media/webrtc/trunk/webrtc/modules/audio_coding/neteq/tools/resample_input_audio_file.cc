









#include "webrtc/modules/audio_coding/neteq/tools/resample_input_audio_file.h"

#include "webrtc/base/checks.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {
namespace test {

bool ResampleInputAudioFile::Read(size_t samples,
                                  int output_rate_hz,
                                  int16_t* destination) {
  const size_t samples_to_read = samples * file_rate_hz_ / output_rate_hz;
  CHECK_EQ(samples_to_read * output_rate_hz, samples * file_rate_hz_)
      << "Frame size and sample rates don't add up to an integer.";
  scoped_ptr<int16_t[]> temp_destination(new int16_t[samples_to_read]);
  if (!InputAudioFile::Read(samples_to_read, temp_destination.get()))
    return false;
  resampler_.ResetIfNeeded(
      file_rate_hz_, output_rate_hz, kResamplerSynchronous);
  int output_length = 0;
  CHECK_EQ(resampler_.Push(temp_destination.get(),
                           static_cast<int>(samples_to_read),
                           destination,
                           static_cast<int>(samples),
                           output_length),
           0);
  CHECK_EQ(static_cast<int>(samples), output_length);
  return true;
}

}  
}  
