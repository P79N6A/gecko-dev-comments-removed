




#ifndef CODEC_CONFIG_H_
#define CODEC_CONFIG_H_

#include <string>
#include "ccsdp_rtcp_fb.h"

namespace mozilla {

class LoadManager;




struct AudioCodecConfig
{
  



  int mType;
  std::string mName;
  int mFreq;
  int mPacSize;
  int mChannels;
  int mRate;
  LoadManager* mLoadManager;

  


  explicit AudioCodecConfig(int type, std::string name,
                            int freq,int pacSize,
                            int channels, int rate,
                            LoadManager* load_manager = nullptr)
                                                   : mType(type),
                                                     mName(name),
                                                     mFreq(freq),
                                                     mPacSize(pacSize),
                                                     mChannels(channels),
                                                     mRate(rate),
                                                     mLoadManager(load_manager)

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
  LoadManager* mLoadManager;

  VideoCodecConfig(int type,
                   std::string name,
                   int rtcpFbTypes,
                   LoadManager* load_manager = nullptr) :
                                     mType(type),
                                     mName(name),
                                     mRtcpFbTypes(rtcpFbTypes),
                                     mMaxFrameSize(0),
                                     mMaxFrameRate(0),
                                     mLoadManager(load_manager)
  {
    
    
    
    
    if (mName == "H264_P0")
      mName = "I420";
  }

  VideoCodecConfig(int type,
                   std::string name,
                   int rtcpFbTypes,
                   unsigned int max_fs,
                   unsigned int max_fr,
                   LoadManager* load_manager = nullptr) :
                                         mType(type),
                                         mName(name),
                                         mRtcpFbTypes(rtcpFbTypes),
                                         mMaxFrameSize(max_fs),
                                         mMaxFrameRate(max_fr),
                                         mLoadManager(load_manager)
  {
    
    
    
    
    if (mName == "H264_P0")
      mName = "I420";
  }


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
