









#ifndef WEBRTC_COMMON_AUDIO_WAV_HEADER_H_
#define WEBRTC_COMMON_AUDIO_WAV_HEADER_H_

#include <stddef.h>
#include <stdint.h>

namespace webrtc {

static const size_t kWavHeaderSize = 44;

enum WavFormat {
  kWavFormatPcm   = 1,  
  kWavFormatALaw  = 6,  
  kWavFormatMuLaw = 7,  
};


bool CheckWavParameters(int num_channels,
                        int sample_rate,
                        WavFormat format,
                        int bytes_per_sample,
                        uint32_t num_samples);





void WriteWavHeader(uint8_t* buf,
                    int num_channels,
                    int sample_rate,
                    WavFormat format,
                    int bytes_per_sample,
                    uint32_t num_samples);



bool ReadWavHeader(const uint8_t* buf,
                   int* num_channels,
                   int* sample_rate,
                   WavFormat* format,
                   int* bytes_per_sample,
                   uint32_t* num_samples);

}  

#endif  
