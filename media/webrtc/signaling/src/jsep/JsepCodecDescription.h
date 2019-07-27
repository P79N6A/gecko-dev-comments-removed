



#ifndef _JSEPCODECDESCRIPTION_H_
#define _JSEPCODECDESCRIPTION_H_

#include <iostream>
#include <string>
#include "signaling/src/sdp/SdpMediaSection.h"

namespace mozilla {

#define JSEP_CODEC_CLONE(T)                                                    \
  virtual JsepCodecDescription* Clone() const MOZ_OVERRIDE                     \
  {                                                                            \
    return new T(*this);                                                       \
  }


struct JsepCodecDescription {
  JsepCodecDescription(mozilla::SdpMediaSection::MediaType type,
                       const std::string& defaultPt,
                       const std::string& name,
                       uint32_t clock,
                       uint32_t channels,
                       bool enabled)
      : mType(type),
        mDefaultPt(defaultPt),
        mName(name),
        mClock(clock),
        mChannels(channels),
        mEnabled(enabled)
  {
  }
  virtual ~JsepCodecDescription() {}

  virtual JsepCodecDescription* Clone() const = 0;
  virtual void AddFmtps(SdpFmtpAttributeList& fmtp) const = 0;
  virtual void AddRtcpFbs(SdpRtcpFbAttributeList& rtcpfb) const = 0;
  virtual bool LoadFmtps(const SdpFmtpAttributeList::Parameters& params) = 0;
  virtual bool LoadRtcpFbs(
      const SdpRtcpFbAttributeList::Feedback& feedback) = 0;

  static bool
  GetPtAsInt(const std::string& ptString, uint16_t* ptOutparam)
  {
    char* end;
    unsigned long pt = strtoul(ptString.c_str(), &end, 10);
    size_t length = static_cast<size_t>(end - ptString.c_str());
    if ((pt > UINT16_MAX) || (length != ptString.size())) {
      return false;
    }
    *ptOutparam = pt;
    return true;
  }

  bool
  GetPtAsInt(uint16_t* ptOutparam) const
  {
    return GetPtAsInt(mDefaultPt, ptOutparam);
  }

  virtual bool
  Matches(const std::string& fmt, const SdpMediaSection& remoteMsection) const
  {
    auto& attrs = remoteMsection.GetAttributeList();
    if (!attrs.HasAttribute(SdpAttribute::kRtpmapAttribute)) {
      return false;
    }

    const SdpRtpmapAttributeList& rtpmap = attrs.GetRtpmap();
    if (!rtpmap.HasEntry(fmt)) {
      return false;
    }

    const SdpRtpmapAttributeList::Rtpmap& entry = rtpmap.GetEntry(fmt);

    if (mType == remoteMsection.GetMediaType()
        && (mName == entry.name)
        && (mClock == entry.clock)
        && (mChannels == entry.channels)) {
      return ParametersMatch(FindParameters(entry.pt, remoteMsection));
    }
    return false;
  }

  virtual bool
  ParametersMatch(const SdpFmtpAttributeList::Parameters* fmtp) const
  {
    return true;
  }

  static const SdpFmtpAttributeList::Parameters*
  FindParameters(const std::string& pt,
                 const mozilla::SdpMediaSection& remoteMsection)
  {
    const SdpAttributeList& attrs = remoteMsection.GetAttributeList();

    if (attrs.HasAttribute(SdpAttribute::kFmtpAttribute)) {
      const SdpFmtpAttributeList& fmtps = attrs.GetFmtp();
      for (auto i = fmtps.mFmtps.begin(); i != fmtps.mFmtps.end(); ++i) {
        if (i->format == pt && i->parameters) {
          return i->parameters.get();
        }
      }
    }
    return nullptr;
  }

  virtual JsepCodecDescription*
  MakeNegotiatedCodec(const mozilla::SdpMediaSection& remoteMsection,
                      const std::string& pt,
                      bool sending) const
  {
    UniquePtr<JsepCodecDescription> negotiated(Clone());
    negotiated->mDefaultPt = pt;

    const SdpAttributeList& attrs = remoteMsection.GetAttributeList();

    if (sending) {
      auto* parameters = FindParameters(negotiated->mDefaultPt, remoteMsection);
      if (parameters) {
        if (!negotiated->LoadFmtps(*parameters)) {
          
          return nullptr;
        }
      }
    } else {
      
      if (attrs.HasAttribute(SdpAttribute::kRtcpFbAttribute)) {
        auto& rtcpfbs = attrs.GetRtcpFb().mFeedbacks;
        for (auto i = rtcpfbs.begin(); i != rtcpfbs.end(); ++i) {
          if (i->pt == negotiated->mDefaultPt || i->pt == "*") {
            if (!negotiated->LoadRtcpFbs(*i)) {
              
              return nullptr;
            }
          }
        }
      }
    }

    return negotiated.release();
  }

  virtual void
  AddToMediaSection(SdpMediaSection& msection) const
  {
    if (mEnabled && msection.GetMediaType() == mType) {
      if (mType == SdpMediaSection::kApplication) {
        
        msection.AddDataChannel(mDefaultPt, mName, mChannels);
      } else {
        msection.AddCodec(mDefaultPt, mName, mClock, mChannels);
      }
      AddFmtpsToMSection(msection);
      AddRtcpFbsToMSection(msection);
    }
  }

  virtual void
  AddFmtpsToMSection(SdpMediaSection& msection) const
  {
    SdpAttributeList& attrs = msection.GetAttributeList();

    UniquePtr<SdpFmtpAttributeList> fmtps;

    if (attrs.HasAttribute(SdpAttribute::kFmtpAttribute)) {
      fmtps.reset(new SdpFmtpAttributeList(attrs.GetFmtp()));
    } else {
      fmtps.reset(new SdpFmtpAttributeList);
    }

    AddFmtps(*fmtps);

    if (!fmtps->mFmtps.empty()) {
      attrs.SetAttribute(fmtps.release());
    }
  }

  virtual void
  AddRtcpFbsToMSection(SdpMediaSection& msection) const
  {
    SdpAttributeList& attrs = msection.GetAttributeList();

    UniquePtr<SdpRtcpFbAttributeList> rtcpfbs;

    if (attrs.HasAttribute(SdpAttribute::kRtcpFbAttribute)) {
      rtcpfbs.reset(new SdpRtcpFbAttributeList(attrs.GetRtcpFb()));
    } else {
      rtcpfbs.reset(new SdpRtcpFbAttributeList);
    }

    AddRtcpFbs(*rtcpfbs);

    if (!rtcpfbs->mFeedbacks.empty()) {
      attrs.SetAttribute(rtcpfbs.release());
    }
  }

  mozilla::SdpMediaSection::MediaType mType;
  std::string mDefaultPt;
  std::string mName;
  uint32_t mClock;
  uint32_t mChannels;
  bool mEnabled;
};

struct JsepAudioCodecDescription : public JsepCodecDescription {
  JsepAudioCodecDescription(const std::string& defaultPt,
                            const std::string& name,
                            uint32_t clock,
                            uint32_t channels,
                            uint32_t packetSize,
                            uint32_t bitRate,
                            bool enabled = true)
      : JsepCodecDescription(mozilla::SdpMediaSection::kAudio, defaultPt, name,
                             clock, channels, enabled),
        mPacketSize(packetSize),
        mBitrate(bitRate)
  {
  }

  virtual void
  AddFmtps(SdpFmtpAttributeList& fmtp) const MOZ_OVERRIDE
  {
    
  }

  virtual void
  AddRtcpFbs(SdpRtcpFbAttributeList& rtcpfb) const MOZ_OVERRIDE
  {
    
  }

  virtual bool
  LoadFmtps(const SdpFmtpAttributeList::Parameters& params) MOZ_OVERRIDE
  {
    
    return true;
  }

  virtual bool
  LoadRtcpFbs(const SdpRtcpFbAttributeList::Feedback& feedback) MOZ_OVERRIDE
  {
    
    return true;
  }

  JSEP_CODEC_CLONE(JsepAudioCodecDescription)

  uint32_t mPacketSize;
  uint32_t mBitrate;
};

struct JsepVideoCodecDescription : public JsepCodecDescription {
  JsepVideoCodecDescription(const std::string& defaultPt,
                            const std::string& name,
                            uint32_t clock,
                            bool enabled = true)
      : JsepCodecDescription(mozilla::SdpMediaSection::kVideo, defaultPt, name,
                             clock, 0, enabled),
        mMaxFs(0),
        mMaxFr(0),
        mPacketizationMode(0),
        mMaxMbps(0),
        mMaxCpb(0),
        mMaxDpb(0),
        mMaxBr(0)
  {
  }

  virtual void
  AddFmtps(SdpFmtpAttributeList& fmtp) const MOZ_OVERRIDE
  {
    if (mName == "H264") {
      UniquePtr<SdpFmtpAttributeList::H264Parameters> params =
          MakeUnique<SdpFmtpAttributeList::H264Parameters>();

      params->packetization_mode = mPacketizationMode;
      
      params->level_asymmetry_allowed = true;
      params->profile_level_id = mProfileLevelId;
      params->max_mbps = mMaxMbps;
      params->max_fs = mMaxFs;
      params->max_cpb = mMaxCpb;
      params->max_dpb = mMaxDpb;
      params->max_br = mMaxBr;
      strncpy(params->sprop_parameter_sets,
              mSpropParameterSets.c_str(),
              sizeof(params->sprop_parameter_sets) - 1);
      fmtp.PushEntry(mDefaultPt, "", mozilla::Move(params));
    } else if (mName == "VP8") {
      UniquePtr<SdpFmtpAttributeList::VP8Parameters> params =
          MakeUnique<SdpFmtpAttributeList::VP8Parameters>();

      params->max_fs = mMaxFs;
      params->max_fr = mMaxFr;
      fmtp.PushEntry(mDefaultPt, "", mozilla::Move(params));
    }
  }

  virtual void
  AddRtcpFbs(SdpRtcpFbAttributeList& rtcpfb) const MOZ_OVERRIDE
  {
    
    rtcpfb.PushEntry(mDefaultPt, SdpRtcpFbAttributeList::kNack);
    rtcpfb.PushEntry(
        mDefaultPt, SdpRtcpFbAttributeList::kNack, SdpRtcpFbAttributeList::pli);
    rtcpfb.PushEntry(
        mDefaultPt, SdpRtcpFbAttributeList::kCcm, SdpRtcpFbAttributeList::fir);
  }

  virtual bool
  LoadFmtps(const SdpFmtpAttributeList::Parameters& params) MOZ_OVERRIDE
  {
    switch (params.codec_type) {
      case SdpRtpmapAttributeList::kH264:
        LoadH264Parameters(params);
        break;
      case SdpRtpmapAttributeList::kVP8:
        LoadVP8Parameters(params);
        break;
      case SdpRtpmapAttributeList::kVP9:
      case SdpRtpmapAttributeList::kiLBC:
      case SdpRtpmapAttributeList::kiSAC:
      case SdpRtpmapAttributeList::kOpus:
      case SdpRtpmapAttributeList::kG722:
      case SdpRtpmapAttributeList::kPCMU:
      case SdpRtpmapAttributeList::kPCMA:
      case SdpRtpmapAttributeList::kOtherCodec:
        MOZ_ASSERT(false, "Invalid codec type for video");
    }
    return true;
  }

  virtual bool
  LoadRtcpFbs(const SdpRtcpFbAttributeList::Feedback& feedback) MOZ_OVERRIDE
  {
    switch (feedback.type) {
      case SdpRtcpFbAttributeList::kAck:
        mAckFbTypes.push_back(feedback.parameter);
        break;
      case SdpRtcpFbAttributeList::kCcm:
        mCcmFbTypes.push_back(feedback.parameter);
        break;
      case SdpRtcpFbAttributeList::kNack:
        mNackFbTypes.push_back(feedback.parameter);
        break;
      case SdpRtcpFbAttributeList::kApp:
      case SdpRtcpFbAttributeList::kTrrInt:
        
        {}
    }
    return true;
  }

  enum Subprofile {
    kH264ConstrainedBaseline,
    kH264Baseline,
    kH264Main,
    kH264Extended,
    kH264High,
    kH264High10,
    kH264High42,
    kH264High44,
    kH264High10I,
    kH264High42I,
    kH264High44I,
    kH264CALVC44,
    kH264UnknownSubprofile
  };

  static Subprofile
  GetSubprofile(uint32_t profileLevelId)
  {
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if ((profileLevelId & 0xFF4F00) == 0x424000) {
      
      
      
      return kH264ConstrainedBaseline;
    }

    if ((profileLevelId & 0xFF8F00) == 0x4D8000) {
      
      
      
      return kH264ConstrainedBaseline;
    }

    if ((profileLevelId & 0xFFCF00) == 0x58C000) {
      
      
      
      return kH264ConstrainedBaseline;
    }

    if ((profileLevelId & 0xFF4F00) == 0x420000) {
      
      
      
      return kH264Baseline;
    }

    if ((profileLevelId & 0xFFCF00) == 0x588000) {
      
      
      
      return kH264Baseline;
    }

    if ((profileLevelId & 0xFFAF00) == 0x4D0000) {
      
      
      
      return kH264Main;
    }

    if ((profileLevelId & 0xFF0000) == 0x580000) {
      
      
      
      return kH264Extended;
    }

    if ((profileLevelId & 0xFFFF00) == 0x640000) {
      return kH264High;
    }

    if ((profileLevelId & 0xFFFF00) == 0x6E0000) {
      return kH264High10;
    }

    if ((profileLevelId & 0xFFFF00) == 0x7A0000) {
      return kH264High42;
    }

    if ((profileLevelId & 0xFFFF00) == 0xF40000) {
      return kH264High44;
    }

    if ((profileLevelId & 0xFFFF00) == 0x6E1000) {
      return kH264High10I;
    }

    if ((profileLevelId & 0xFFFF00) == 0x7A1000) {
      return kH264High42I;
    }

    if ((profileLevelId & 0xFFFF00) == 0xF41000) {
      return kH264High44I;
    }

    if ((profileLevelId & 0xFFFF00) == 0x2C1000) {
      return kH264CALVC44;
    }

    return kH264UnknownSubprofile;
  }

  virtual bool
  ParametersMatch(const SdpFmtpAttributeList::Parameters* fmtp) const
      MOZ_OVERRIDE
  {
    if (mName == "H264") {
      if (!fmtp) {
        
        
        
        return false;
      }

      auto* h264Params =
          static_cast<const SdpFmtpAttributeList::H264Parameters*>(fmtp);

      if (!h264Params->level_asymmetry_allowed) {
        if (GetSubprofile(h264Params->profile_level_id) !=
            GetSubprofile(mProfileLevelId)) {
          return false;
        }
      }

      if (h264Params->packetization_mode != mPacketizationMode) {
        return false;
      }
    }
    return true;
  }

  void
  LoadH264Parameters(const SdpFmtpAttributeList::Parameters& params)
  {
    const SdpFmtpAttributeList::H264Parameters& h264Params =
        static_cast<const SdpFmtpAttributeList::H264Parameters&>(params);

    mMaxFs = h264Params.max_fs;
    mProfileLevelId = h264Params.profile_level_id;
    mPacketizationMode = h264Params.packetization_mode;
    mMaxMbps = h264Params.max_mbps;
    mMaxCpb = h264Params.max_cpb;
    mMaxDpb = h264Params.max_dpb;
    mMaxBr = h264Params.max_br;
    mSpropParameterSets = h264Params.sprop_parameter_sets;
  }

  void
  LoadVP8Parameters(const SdpFmtpAttributeList::Parameters& params)
  {
    const SdpFmtpAttributeList::VP8Parameters& vp8Params =
        static_cast<const SdpFmtpAttributeList::VP8Parameters&>(params);

    mMaxFs = vp8Params.max_fs;
    mMaxFr = vp8Params.max_fr;
  }

  JSEP_CODEC_CLONE(JsepVideoCodecDescription)

  std::vector<std::string> mAckFbTypes;
  std::vector<std::string> mNackFbTypes;
  std::vector<std::string> mCcmFbTypes;

  uint32_t mMaxFs;

  
  uint32_t mProfileLevelId;
  uint32_t mMaxFr;
  uint32_t mPacketizationMode;
  uint32_t mMaxMbps;
  uint32_t mMaxCpb;
  uint32_t mMaxDpb;
  uint32_t mMaxBr;
  std::string mSpropParameterSets;
};

struct JsepApplicationCodecDescription : public JsepCodecDescription {
  JsepApplicationCodecDescription(const std::string& defaultPt,
                                  const std::string& name,
                                  uint16_t channels,
                                  bool enabled = true)
      : JsepCodecDescription(mozilla::SdpMediaSection::kApplication, defaultPt,
                             name, 0, channels, enabled)
  {
  }

  virtual void
  AddFmtps(SdpFmtpAttributeList& fmtp) const MOZ_OVERRIDE
  {
    
  }

  virtual void
  AddRtcpFbs(SdpRtcpFbAttributeList& rtcpfb) const MOZ_OVERRIDE
  {
    
  }

  virtual bool
  LoadFmtps(const SdpFmtpAttributeList::Parameters& params) MOZ_OVERRIDE
  {
    
    return true;
  }

  virtual bool
  LoadRtcpFbs(const SdpRtcpFbAttributeList::Feedback& feedback) MOZ_OVERRIDE
  {
    
    return true;
  }

  JSEP_CODEC_CLONE(JsepApplicationCodecDescription)

  
  virtual bool
  Matches(const std::string& fmt,
          const SdpMediaSection& remoteMsection) const MOZ_OVERRIDE
  {
    auto& attrs = remoteMsection.GetAttributeList();
    if (!attrs.HasAttribute(SdpAttribute::kSctpmapAttribute)) {
      return false;
    }

    const SdpSctpmapAttributeList& sctpmap = attrs.GetSctpmap();
    if (!sctpmap.HasEntry(fmt)) {
      return false;
    }

    const SdpSctpmapAttributeList::Sctpmap& entry = sctpmap.GetEntry(fmt);

    if (mType == remoteMsection.GetMediaType() && (mName == entry.name)) {
      return true;
    }
    return false;
  }
};

} 

#endif
