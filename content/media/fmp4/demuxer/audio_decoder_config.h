



#ifndef MEDIA_BASE_AUDIO_DECODER_CONFIG_H_
#define MEDIA_BASE_AUDIO_DECODER_CONFIG_H_

#include <vector>

#include "mp4_demuxer/basictypes.h"
#include "mp4_demuxer/channel_layout.h"

namespace mp4_demuxer {

enum AudioCodec {
  
  
  
  kUnknownAudioCodec = 0,
  kCodecAAC,
  kCodecMP3,
  kCodecPCM,
  kCodecVorbis,
  kCodecFLAC,
  kCodecAMR_NB,
  kCodecAMR_WB,
  kCodecPCM_MULAW,
  kCodecGSM_MS,
  kCodecPCM_S16BE,
  kCodecPCM_S24BE,
  kCodecOpus,
  
  
  
  

  
  kAudioCodecMax
};

enum SampleFormat {
  
  
  
  kUnknownSampleFormat = 0,
  kSampleFormatU8,         
  kSampleFormatS16,        
  kSampleFormatS32,        
  kSampleFormatF32,        
  kSampleFormatPlanarS16,  
  kSampleFormatPlanarF32,  

  
  kSampleFormatMax
};




class AudioDecoderConfig {
 public:
  
  
  AudioDecoderConfig();

  
  
  AudioDecoderConfig(AudioCodec codec, SampleFormat sample_format,
                     ChannelLayout channel_layout, int samples_per_second,
                     const uint8_t* extra_data, size_t extra_data_size,
                     bool is_encrypted);

  ~AudioDecoderConfig();

  
  void Initialize(AudioCodec codec, SampleFormat sample_format,
                  ChannelLayout channel_layout, int samples_per_second,
                  const uint8_t* extra_data, size_t extra_data_size,
                  bool is_encrypted);

  
  
  bool IsValidConfig() const;

  
  
  bool Matches(const AudioDecoderConfig& config) const;

  AudioCodec codec() const { return codec_; }
  int bits_per_channel() const { return bits_per_channel_; }
  ChannelLayout channel_layout() const { return channel_layout_; }
  int samples_per_second() const { return samples_per_second_; }
  SampleFormat sample_format() const { return sample_format_; }
  int bytes_per_frame() const { return bytes_per_frame_; }

  
  
  const uint8_t* extra_data() const {
    return extra_data_.empty() ? NULL : &extra_data_[0];
  }
  size_t extra_data_size() const { return extra_data_.size(); }

  
  
  
  bool is_encrypted() const { return is_encrypted_; }

  std::string AsHumanReadableString() const;

 private:
  AudioCodec codec_;
  SampleFormat sample_format_;
  int bits_per_channel_;
  ChannelLayout channel_layout_;
  int samples_per_second_;
  int bytes_per_frame_;
  std::vector<uint8_t> extra_data_;
  bool is_encrypted_;

  
  
  
};

}  

#endif  
