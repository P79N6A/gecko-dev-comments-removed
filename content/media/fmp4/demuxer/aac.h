



#ifndef MEDIA_MP4_AAC_H_
#define MEDIA_MP4_AAC_H_

#include <vector>

#include "mp4_demuxer/basictypes.h"
#include "mp4_demuxer/channel_layout.h"

namespace mp4_demuxer {

class BitReader;





class AAC {
 public:
  AAC();
  ~AAC();

  
  
  
  
  bool Parse(const std::vector<uint8_t>& data);

  
  
  
  
  
  int GetOutputSamplesPerSecond(bool sbr_in_mimetype) const;

  
  
  
  
  
  ChannelLayout GetChannelLayout(bool sbr_in_mimetype) const;

  
  
  
  
  bool ConvertEsdsToADTS(std::vector<uint8_t>* buffer) const;

  
  static const size_t kADTSHeaderSize = 7;

  const std::vector<uint8_t>& AudioSpecificConfig() const;

 private:
  bool SkipDecoderGASpecificConfig(BitReader* bit_reader) const;
  bool SkipErrorSpecificConfig() const;
  bool SkipGASpecificConfig(BitReader* bit_reader) const;

  
  
  uint8_t profile_;
  uint8_t frequency_index_;
  uint8_t channel_config_;

  
  
  
  
  int frequency_;
  int extension_frequency_;
  ChannelLayout channel_layout_;

  std::vector<uint8_t> audio_specific_config_;
};

}  

#endif  
