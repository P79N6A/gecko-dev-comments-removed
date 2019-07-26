



#include "mp4_demuxer/aac.h"

#include <algorithm>

#include "mp4_demuxer/bit_reader.h"

namespace mp4_demuxer {



static const int kFrequencyMap[] = {
  96000, 88200, 64000, 48000, 44100, 32000, 24000,
  22050, 16000, 12000, 11025, 8000, 7350
};

static ChannelLayout ConvertChannelConfigToLayout(uint8_t channel_config) {
  switch (channel_config) {
    case 1:
      return CHANNEL_LAYOUT_MONO;
    case 2:
      return CHANNEL_LAYOUT_STEREO;
    case 3:
      return CHANNEL_LAYOUT_SURROUND;
    case 4:
      return CHANNEL_LAYOUT_4_0;
    case 5:
      return CHANNEL_LAYOUT_5_0;
    case 6:
      return CHANNEL_LAYOUT_5_1;
    case 8:
      return CHANNEL_LAYOUT_7_1;
    default:
      break;
  }

  return CHANNEL_LAYOUT_UNSUPPORTED;
}

AAC::AAC()
    : profile_(0), frequency_index_(0), channel_config_(0), frequency_(0),
      extension_frequency_(0), channel_layout_(CHANNEL_LAYOUT_UNSUPPORTED) {
}

AAC::~AAC() {
}

bool AAC::Parse(const std::vector<uint8_t>& data) {
  if (data.empty())
    return false;

  BitReader reader(&data[0], data.size());
  uint8_t extension_type = 0;
  bool ps_present = false;
  uint8_t extension_frequency_index = 0xff;

  frequency_ = 0;
  extension_frequency_ = 0;

  
  

  
  RCHECK(reader.ReadBits(5, &profile_));
  RCHECK(reader.ReadBits(4, &frequency_index_));
  if (frequency_index_ == 0xf)
    RCHECK(reader.ReadBits(24, &frequency_));
  RCHECK(reader.ReadBits(4, &channel_config_));

  
  if (profile_ == 5 || profile_ == 29) {
    ps_present = (profile_ == 29);
    extension_type = 5;
    RCHECK(reader.ReadBits(4, &extension_frequency_index));
    if (extension_frequency_index == 0xf)
      RCHECK(reader.ReadBits(24, &extension_frequency_));
    RCHECK(reader.ReadBits(5, &profile_));
  }

  RCHECK(SkipDecoderGASpecificConfig(&reader));
  RCHECK(SkipErrorSpecificConfig());

  
  
  if (extension_type != 5 && reader.bits_available() >= 16) {
    uint16_t sync_extension_type;
    uint8_t sbr_present_flag;
    uint8_t ps_present_flag;

    if (reader.ReadBits(11, &sync_extension_type) &&
        sync_extension_type == 0x2b7) {
      if (reader.ReadBits(5, &extension_type) && extension_type == 5) {
        RCHECK(reader.ReadBits(1, &sbr_present_flag));

        if (sbr_present_flag) {
          RCHECK(reader.ReadBits(4, &extension_frequency_index));

          if (extension_frequency_index == 0xf)
            RCHECK(reader.ReadBits(24, &extension_frequency_));

          
          if (reader.bits_available() >= 12) {
            RCHECK(reader.ReadBits(11, &sync_extension_type));
            if (sync_extension_type == 0x548) {
              RCHECK(reader.ReadBits(1, &ps_present_flag));
              ps_present = ps_present_flag != 0;
            }
          }
        }
      }
    }
  }

  if (frequency_ == 0) {
    RCHECK(frequency_index_ < arraysize(kFrequencyMap));
    frequency_ = kFrequencyMap[frequency_index_];
  }

  if (extension_frequency_ == 0 && extension_frequency_index != 0xff) {
    RCHECK(extension_frequency_index < arraysize(kFrequencyMap));
    extension_frequency_ = kFrequencyMap[extension_frequency_index];
  }

  
  if (ps_present && channel_config_ == 1)
    channel_layout_ = CHANNEL_LAYOUT_STEREO;
  else
    channel_layout_ = ConvertChannelConfigToLayout(channel_config_);

  audio_specific_config_.insert(audio_specific_config_.begin(), data.begin(), data.end());

  return frequency_ != 0 && channel_layout_ != CHANNEL_LAYOUT_UNSUPPORTED &&
      profile_ >= 1 && profile_ <= 4 && frequency_index_ != 0xf &&
      channel_config_ <= 7;
}

const std::vector<uint8_t>& AAC::AudioSpecificConfig() const
{
  return audio_specific_config_;
}

int AAC::GetOutputSamplesPerSecond(bool sbr_in_mimetype) const {
  if (extension_frequency_ > 0)
    return extension_frequency_;

  if (!sbr_in_mimetype)
    return frequency_;

  
  
  
  
  DCHECK_GT(frequency_, 0);
  return std::min(2 * frequency_, 48000);
}

ChannelLayout AAC::GetChannelLayout(bool sbr_in_mimetype) const {
  
  
  
  if (sbr_in_mimetype && channel_config_ == 1)
    return CHANNEL_LAYOUT_STEREO;

  return channel_layout_;
}

bool AAC::ConvertEsdsToADTS(std::vector<uint8_t>* buffer) const {
  size_t size = buffer->size() + kADTSHeaderSize;

  DCHECK(profile_ >= 1 && profile_ <= 4 && frequency_index_ != 0xf &&
         channel_config_ <= 7);

  
  if (size >= (1 << 13))
    return false;

  std::vector<uint8_t>& adts = *buffer;

  adts.insert(buffer->begin(), kADTSHeaderSize, 0);
  adts[0] = 0xff;
  adts[1] = 0xf1;
  adts[2] = ((profile_ - 1) << 6) + (frequency_index_ << 2) +
      (channel_config_ >> 2);
  adts[3] = ((channel_config_ & 0x3) << 6) + (size >> 11);
  adts[4] = (size & 0x7ff) >> 3;
  adts[5] = ((size & 7) << 5) + 0x1f;
  adts[6] = 0xfc;

  return true;
}



bool AAC::SkipDecoderGASpecificConfig(BitReader* bit_reader) const {
  switch (profile_) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
    case 17:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
      return SkipGASpecificConfig(bit_reader);
    default:
      break;
  }

  return false;
}

bool AAC::SkipErrorSpecificConfig() const {
  switch (profile_) {
    case 17:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
      return false;
    default:
      break;
  }

  return true;
}



bool AAC::SkipGASpecificConfig(BitReader* bit_reader) const {
  uint8_t extension_flag = 0;
  uint8_t depends_on_core_coder;
  uint16_t dummy;

  RCHECK(bit_reader->ReadBits(1, &dummy));  
  RCHECK(bit_reader->ReadBits(1, &depends_on_core_coder));
  if (depends_on_core_coder == 1)
    RCHECK(bit_reader->ReadBits(14, &dummy));  

  RCHECK(bit_reader->ReadBits(1, &extension_flag));
  RCHECK(channel_config_ != 0);

  if (profile_ == 6 || profile_ == 20)
    RCHECK(bit_reader->ReadBits(3, &dummy));  

  if (extension_flag) {
    if (profile_ == 22) {
      RCHECK(bit_reader->ReadBits(5, &dummy));  
      RCHECK(bit_reader->ReadBits(11, &dummy));  
    }

    if (profile_ == 17 || profile_ == 19 || profile_ == 20 || profile_ == 23) {
      RCHECK(bit_reader->ReadBits(3, &dummy));  
    }

    RCHECK(bit_reader->ReadBits(1, &dummy));  
  }

  return true;
}

}  
