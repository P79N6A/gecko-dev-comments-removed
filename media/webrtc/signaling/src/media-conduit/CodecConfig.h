




#ifndef CODEC_CONFIG_H_
#define CODEC_CONFIG_H_

#include <string>
#include "ccsdp_rtcp_fb.h"

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






struct VideoCodecConfig
{
  



  int mType; 
  std::string mName;
  uint32_t mRtcpFbTypes;
  unsigned int mMaxFrameSize;
  unsigned int mMaxFrameRate;
  uint8_t mProfile;
  uint8_t mConstraints;
  uint8_t mLevel;
  uint8_t mPacketizationMode;
  

  VideoCodecConfig(int type,
                   std::string name,
                   int rtcpFbTypes,
                   uint8_t profile = 0x42,
                   uint8_t constraints = 0xC0,
                   uint8_t level = 30,
                   uint8_t packetization = 0) :
                                     mType(type),
                                     mName(name),
                                     mRtcpFbTypes(rtcpFbTypes),
                                     mMaxFrameSize(0),
                                     mMaxFrameRate(0),
                                     mProfile(profile),
                                     mConstraints(constraints),
                                     mLevel(level),
                                     mPacketizationMode(packetization) {}

  VideoCodecConfig(int type,
                   std::string name,
                   int rtcpFbTypes,
                   unsigned int max_fs,
                   unsigned int max_fr) :
                                         mType(type),
                                         mName(name),
                                         mRtcpFbTypes(rtcpFbTypes),
                                         mMaxFrameSize(max_fs),
                                         mMaxFrameRate(max_fr) {}

  bool RtcpFbIsSet(sdp_rtcp_fb_nack_type_e type) const
  {
    return mRtcpFbTypes & sdp_rtcp_fb_nack_to_bitmap(type);
  }

  bool RtcpFbIsSet(sdp_rtcp_fb_ack_type_e type) const
  {
    return mRtcpFbTypes & sdp_rtcp_fb_ack_to_bitmap(type);
  }

  bool RtcpFbIsSet(sdp_rtcp_fb_ccm_type_e type) const
  {
    return mRtcpFbTypes & sdp_rtcp_fb_ccm_to_bitmap(type);
  }
};
}
#endif
