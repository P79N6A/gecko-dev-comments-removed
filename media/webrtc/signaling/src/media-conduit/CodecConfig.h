




#ifndef CODEC_CONFIG_H_
#define CODEC_CONFIG_H_

#include <string>
#include <vector>

namespace mozilla {




struct AudioCodecConfig
{
  



  int mType;
  std::string mName;
  int mFreq;
  int mPacSize;
  int mChannels;
  int mRate;

  


  explicit AudioCodecConfig(int type, std::string name,
                            int freq,int pacSize,
                            int channels, int rate)
                                                   : mType(type),
                                                     mName(name),
                                                     mFreq(freq),
                                                     mPacSize(pacSize),
                                                     mChannels(channels),
                                                     mRate(rate)

  {
  }
};






#define    MAX_SPROP_LEN    128


struct VideoCodecConfigH264
{
    char       sprop_parameter_sets[MAX_SPROP_LEN];
    int        packetization_mode;
    int        profile_level_id;
    int        max_mbps;
    int        max_fs;
    int        max_cpb;
    int        max_dpb;
    int        max_br;
    int        tias_bw;
};



class VideoCodecConfig
{
public:
  



  int mType; 
  std::string mName;

  std::vector<std::string> mAckFbTypes;
  std::vector<std::string> mNackFbTypes;
  std::vector<std::string> mCcmFbTypes;

  unsigned int mMaxFrameSize;
  unsigned int mMaxFrameRate;
  unsigned int mMaxMBPS;    
  unsigned int mMaxBitrate;
  
  std::string mSpropParameterSets;
  uint8_t mProfile;
  uint8_t mConstraints;
  uint8_t mLevel;
  uint8_t mPacketizationMode;
  

  VideoCodecConfig(int type,
                   std::string name,
                   unsigned int max_fs = 0,
                   unsigned int max_fr = 0,
                   const struct VideoCodecConfigH264 *h264 = nullptr) :
    mType(type),
    mName(name),
    mMaxFrameSize(max_fs), 
    mMaxFrameRate(max_fr),
    mMaxMBPS(0),
    mMaxBitrate(0),
    mProfile(0x42),
    mConstraints(0xE0),
    mLevel(0x0C),
    mPacketizationMode(1)
  {
    if (h264) {
      if (max_fs == 0 || (h264->max_fs != 0 && (unsigned int) h264->max_fs < max_fs)) {
        mMaxFrameSize = h264->max_fs;
      }
      mMaxMBPS = h264->max_mbps;
      mMaxBitrate = h264->max_br;
      mProfile = (h264->profile_level_id & 0x00FF0000) >> 16;
      mConstraints = (h264->profile_level_id & 0x0000FF00) >> 8;
      mLevel = (h264->profile_level_id & 0x000000FF);
      mPacketizationMode = h264->packetization_mode;
      mSpropParameterSets = h264->sprop_parameter_sets;
    }
  }

  
  
  bool RtcpFbAckIsSet(const std::string& type) const
  {
    for (auto i = mAckFbTypes.begin(); i != mAckFbTypes.end(); ++i) {
      if (*i == type) {
        return true;
      }
    }
    return false;
  }

  bool RtcpFbNackIsSet(const std::string& type) const
  {
    for (auto i = mNackFbTypes.begin(); i != mNackFbTypes.end(); ++i) {
      if (*i == type) {
        return true;
      }
    }
    return false;
  }

  bool RtcpFbCcmIsSet(const std::string& type) const
  {
    for (auto i = mCcmFbTypes.begin(); i != mCcmFbTypes.end(); ++i) {
      if (*i == type) {
        return true;
      }
    }
    return false;
  }
};
}
#endif
