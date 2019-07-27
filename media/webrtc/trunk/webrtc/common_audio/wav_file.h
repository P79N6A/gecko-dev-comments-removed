









#ifndef WEBRTC_COMMON_AUDIO_WAV_FILE_H_
#define WEBRTC_COMMON_AUDIO_WAV_FILE_H_

#ifdef __cplusplus

#include <stdint.h>
#include <cstddef>
#include <string>

namespace webrtc {



class WavWriter {
 public:
  
  WavWriter(const std::string& filename, int sample_rate, int num_channels);

  
  ~WavWriter();

  
  
  
  void WriteSamples(const float* samples, size_t num_samples);
  void WriteSamples(const int16_t* samples, size_t num_samples);

  int sample_rate() const { return sample_rate_; }
  int num_channels() const { return num_channels_; }
  uint32_t num_samples() const { return num_samples_; }

 private:
  void Close();
  const int sample_rate_;
  const int num_channels_;
  uint32_t num_samples_;  
  FILE* file_handle_;  
};


class WavReader {
 public:
  
  explicit WavReader(const std::string& filename);

  
  ~WavReader();

  
  
  size_t ReadSamples(size_t num_samples, float* samples);
  size_t ReadSamples(size_t num_samples, int16_t* samples);

  int sample_rate() const { return sample_rate_; }
  int num_channels() const { return num_channels_; }
  uint32_t num_samples() const { return num_samples_; }

 private:
  void Close();
  int sample_rate_;
  int num_channels_;
  uint32_t num_samples_;  
  FILE* file_handle_;  
};

}  

extern "C" {
#endif


typedef struct rtc_WavWriter rtc_WavWriter;
rtc_WavWriter* rtc_WavOpen(const char* filename,
                           int sample_rate,
                           int num_channels);
void rtc_WavClose(rtc_WavWriter* wf);
void rtc_WavWriteSamples(rtc_WavWriter* wf,
                         const float* samples,
                         size_t num_samples);
int rtc_WavSampleRate(const rtc_WavWriter* wf);
int rtc_WavNumChannels(const rtc_WavWriter* wf);
uint32_t rtc_WavNumSamples(const rtc_WavWriter* wf);

#ifdef __cplusplus
}  
#endif

#endif
