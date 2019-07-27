





#ifndef _SDPATTRIBUTE_H_
#define _SDPATTRIBUTE_H_

#include <algorithm>
#include <vector>
#include <ostream>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <string>

#include "mozilla/UniquePtr.h"
#include "mozilla/Attributes.h"
#include "mozilla/Assertions.h"

#include "signaling/src/sdp/SdpEnum.h"

namespace mozilla
{




class SdpAttribute
{
public:
  enum AttributeType {
    kFirstAttribute = 0,
    kBundleOnlyAttribute = 0,
    kCandidateAttribute,
    kConnectionAttribute,
    kDirectionAttribute,
    kEndOfCandidatesAttribute,
    kExtmapAttribute,
    kFingerprintAttribute,
    kFmtpAttribute,
    kGroupAttribute,
    kIceLiteAttribute,
    kIceMismatchAttribute,
    kIceOptionsAttribute,
    kIcePwdAttribute,
    kIceUfragAttribute,
    kIdentityAttribute,
    kImageattrAttribute,
    kInactiveAttribute,
    kLabelAttribute,
    kMaxptimeAttribute,
    kMidAttribute,
    kMsidAttribute,
    kMsidSemanticAttribute,
    kPtimeAttribute,
    kRecvonlyAttribute,
    kRemoteCandidatesAttribute,
    kRtcpAttribute,
    kRtcpFbAttribute,
    kRtcpMuxAttribute,
    kRtcpRsizeAttribute,
    kRtpmapAttribute,
    kSctpmapAttribute,
    kSendonlyAttribute,
    kSendrecvAttribute,
    kSetupAttribute,
    kSsrcAttribute,
    kSsrcGroupAttribute,
    kLastAttribute = kSsrcGroupAttribute
  };

  explicit SdpAttribute(AttributeType type) : mType(type) {}
  virtual ~SdpAttribute() {}

  AttributeType
  GetType() const
  {
    return mType;
  }

  virtual void Serialize(std::ostream&) const = 0;

  static bool IsAllowedAtSessionLevel(AttributeType type);
  static bool IsAllowedAtMediaLevel(AttributeType type);
  static const std::string GetAttributeTypeString(AttributeType type);

protected:
  AttributeType mType;
};

inline std::ostream& operator<<(std::ostream& os, const SdpAttribute& attr)
{
  attr.Serialize(os);
  return os;
}

inline std::ostream& operator<<(std::ostream& os,
                                const SdpAttribute::AttributeType type)
{
  os << SdpAttribute::GetAttributeTypeString(type);
  return os;
}



































class SdpConnectionAttribute : public SdpAttribute
{
public:
  enum ConnValue { kNew, kExisting };

  explicit SdpConnectionAttribute(SdpConnectionAttribute::ConnValue value)
      : SdpAttribute(kConnectionAttribute), mValue(value)
  {
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  ConnValue mValue;
};

inline std::ostream& operator<<(std::ostream& os,
                                SdpConnectionAttribute::ConnValue c)
{
  switch (c) {
    case SdpConnectionAttribute::kNew:
      os << "new";
      break;
    case SdpConnectionAttribute::kExisting:
      os << "existing";
      break;
    default:
      MOZ_ASSERT(false);
      os << "?";
  }
  return os;
}




class SdpDirectionAttribute : public SdpAttribute
{
public:
  static const unsigned kSendFlag = 1;
  static const unsigned kRecvFlag = 1 << 1;

  enum Direction {
    kInactive = 0,
    kSendonly = kSendFlag,
    kRecvonly = kRecvFlag,
    kSendrecv = kSendFlag | kRecvFlag
  };

  explicit SdpDirectionAttribute(Direction value)
      : SdpAttribute(kDirectionAttribute), mValue(value)
  {
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  Direction mValue;
};

inline std::ostream& operator<<(std::ostream& os,
                                SdpDirectionAttribute::Direction d)
{
  switch (d) {
    case SdpDirectionAttribute::kSendonly:
      os << "sendonly";
      break;
    case SdpDirectionAttribute::kRecvonly:
      os << "recvonly";
      break;
    case SdpDirectionAttribute::kSendrecv:
      os << "sendrecv";
      break;
    case SdpDirectionAttribute::kInactive:
      os << "inactive";
      break;
    default:
      MOZ_ASSERT(false);
      os << "?";
  }
  return os;
}






















class SdpExtmapAttributeList : public SdpAttribute
{
public:
  SdpExtmapAttributeList() : SdpAttribute(kExtmapAttribute) {}

  struct Extmap {
    uint16_t entry;
    SdpDirectionAttribute::Direction direction;
    bool direction_specified;
    std::string extensionname;
    std::string extensionattributes;
  };

  void
  PushEntry(uint16_t entry, SdpDirectionAttribute::Direction direction,
            bool direction_specified, const std::string& extensionname,
            const std::string& extensionattributes = "")
  {
    Extmap value = { entry, direction, direction_specified, extensionname,
                     extensionattributes };
    mExtmaps.push_back(value);
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  std::vector<Extmap> mExtmaps;
};

















class SdpFingerprintAttributeList : public SdpAttribute
{
public:
  SdpFingerprintAttributeList() : SdpAttribute(kFingerprintAttribute) {}

  enum HashAlgorithm {
    kSha1,
    kSha224,
    kSha256,
    kSha384,
    kSha512,
    kMd5,
    kMd2,
    kUnknownAlgorithm
  };

  struct Fingerprint {
    HashAlgorithm hashFunc;
    std::vector<uint8_t> fingerprint;
  };

  
  
  void
  PushEntry(const std::string& algorithm_str,
            const std::vector<uint8_t>& fingerprint,
            bool enforcePlausible = true)
  {
    SdpFingerprintAttributeList::HashAlgorithm algorithm =
        SdpFingerprintAttributeList::kUnknownAlgorithm;

    if (algorithm_str == "sha-1") {
      algorithm = SdpFingerprintAttributeList::kSha1;
    } else if (algorithm_str == "sha-224") {
      algorithm = SdpFingerprintAttributeList::kSha224;
    } else if (algorithm_str == "sha-256") {
      algorithm = SdpFingerprintAttributeList::kSha256;
    } else if (algorithm_str == "sha-384") {
      algorithm = SdpFingerprintAttributeList::kSha384;
    } else if (algorithm_str == "sha-512") {
      algorithm = SdpFingerprintAttributeList::kSha512;
    } else if (algorithm_str == "md5") {
      algorithm = SdpFingerprintAttributeList::kMd5;
    } else if (algorithm_str == "md2") {
      algorithm = SdpFingerprintAttributeList::kMd2;
    }

    if ((algorithm == SdpFingerprintAttributeList::kUnknownAlgorithm) ||
        fingerprint.empty()) {
      if (enforcePlausible) {
        MOZ_ASSERT(false, "Unknown fingerprint algorithm");
      } else {
        return;
      }
    }

    PushEntry(algorithm, fingerprint);
  }

  void
  PushEntry(HashAlgorithm hashFunc, const std::vector<uint8_t>& fingerprint)
  {
    Fingerprint value = { hashFunc, fingerprint };
    mFingerprints.push_back(value);
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  std::vector<Fingerprint> mFingerprints;

  static std::string FormatFingerprint(const std::vector<uint8_t>& fp);
  static std::vector<uint8_t> ParseFingerprint(const std::string& str);
};

inline std::ostream& operator<<(std::ostream& os,
                                SdpFingerprintAttributeList::HashAlgorithm a)
{
  switch (a) {
    case SdpFingerprintAttributeList::kSha1:
      os << "sha-1";
      break;
    case SdpFingerprintAttributeList::kSha224:
      os << "sha-224";
      break;
    case SdpFingerprintAttributeList::kSha256:
      os << "sha-256";
      break;
    case SdpFingerprintAttributeList::kSha384:
      os << "sha-384";
      break;
    case SdpFingerprintAttributeList::kSha512:
      os << "sha-512";
      break;
    case SdpFingerprintAttributeList::kMd5:
      os << "md5";
      break;
    case SdpFingerprintAttributeList::kMd2:
      os << "md2";
      break;
    default:
      MOZ_ASSERT(false);
      os << "?";
  }
  return os;
}









class SdpGroupAttributeList : public SdpAttribute
{
public:
  SdpGroupAttributeList() : SdpAttribute(kGroupAttribute) {}

  enum Semantics {
    kLs,    
    kFid,   
    kSrf,   
    kAnat,  
    kFec,   
    kFecFr, 
    kCs,    
    kDdp,   
    kDup,   
    kBundle 
  };

  struct Group {
    Semantics semantics;
    std::vector<std::string> tags;
  };

  void
  PushEntry(Semantics semantics, const std::vector<std::string>& tags)
  {
    Group value = { semantics, tags };
    mGroups.push_back(value);
  }

  void
  RemoveMid(const std::string& mid)
  {
    for (auto i = mGroups.begin(); i != mGroups.end();) {
      auto tag = std::find(i->tags.begin(), i->tags.end(), mid);
      if (tag != i->tags.end()) {
        i->tags.erase(tag);
      }

      if (i->tags.empty()) {
        i = mGroups.erase(i);
      } else {
        ++i;
      }
    }
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  std::vector<Group> mGroups;
};

inline std::ostream& operator<<(std::ostream& os,
                                SdpGroupAttributeList::Semantics s)
{
  switch (s) {
    case SdpGroupAttributeList::kLs:
      os << "LS";
      break;
    case SdpGroupAttributeList::kFid:
      os << "FID";
      break;
    case SdpGroupAttributeList::kSrf:
      os << "SRF";
      break;
    case SdpGroupAttributeList::kAnat:
      os << "ANAT";
      break;
    case SdpGroupAttributeList::kFec:
      os << "FEC";
      break;
    case SdpGroupAttributeList::kFecFr:
      os << "FEC-FR";
      break;
    case SdpGroupAttributeList::kCs:
      os << "CS";
      break;
    case SdpGroupAttributeList::kDdp:
      os << "DDP";
      break;
    case SdpGroupAttributeList::kDup:
      os << "DUP";
      break;
    case SdpGroupAttributeList::kBundle:
      os << "BUNDLE";
      break;
    default:
      MOZ_ASSERT(false);
      os << "?";
  }
  return os;
}















#if 0
class SdpIdentityAttribute : public SdpAttribute
{
public:
  explicit SdpIdentityAttribute(const std::string &assertion,
                                const std::vector<std::string> &extensions =
                                    std::vector<std::string>()) :
    SdpAttribute(kIdentityAttribute),
    mAssertion(assertion),
    mExtensions(extensions) {}

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  std::string mAssertion;
  std::vector<std::string> mExtensions;
}
#endif









































































class SdpImageattrAttributeList : public SdpAttribute
{
public:
  SdpImageattrAttributeList() : SdpAttribute(kImageattrAttribute) {}

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;
};







class SdpMsidAttributeList : public SdpAttribute
{
public:
  SdpMsidAttributeList() : SdpAttribute(kMsidAttribute) {}

  struct Msid {
    std::string identifier;
    std::string appdata;
  };

  void
  PushEntry(const std::string& identifier, const std::string& appdata = "")
  {
    Msid value = { identifier, appdata };
    mMsids.push_back(value);
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  std::vector<Msid> mMsids;
};







class SdpMsidSemanticAttributeList : public SdpAttribute
{
public:
  SdpMsidSemanticAttributeList() : SdpAttribute(kMsidSemanticAttribute) {}

  struct MsidSemantic
  {
    
    std::string semantic;
    std::vector<std::string> msids;
  };

  void
  PushEntry(const std::string& semantic, const std::vector<std::string>& msids)
  {
    MsidSemantic value = {semantic, msids};
    mMsidSemantics.push_back(value);
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  std::vector<MsidSemantic> mMsidSemantics;
};







class SdpRemoteCandidatesAttribute : public SdpAttribute
{
public:
  struct Candidate {
    std::string id;
    std::string address;
    uint16_t port;
  };

  explicit SdpRemoteCandidatesAttribute(
      const std::vector<Candidate>& candidates)
      : SdpAttribute(kRemoteCandidatesAttribute), mCandidates(candidates)
  {
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  std::vector<Candidate> mCandidates;
};






class SdpRtcpAttribute : public SdpAttribute
{
public:
  explicit SdpRtcpAttribute(uint16_t port,
                            sdp::NetType netType = sdp::kNetTypeNone,
                            sdp::AddrType addrType = sdp::kAddrTypeNone,
                            const std::string& address = "")
      : SdpAttribute(kRtcpAttribute),
        mPort(port),
        mNetType(netType),
        mAddrType(addrType),
        mAddress(address)
  {
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  uint16_t mPort;
  sdp::NetType mNetType;
  sdp::AddrType mAddrType;
  std::string mAddress;
};
































class SdpRtcpFbAttributeList : public SdpAttribute
{
public:
  SdpRtcpFbAttributeList() : SdpAttribute(kRtcpFbAttribute) {}

  enum Type { kAck, kApp, kCcm, kNack, kTrrInt };

  static const char* pli;
  static const char* sli;
  static const char* rpsi;
  static const char* app;

  static const char* fir;
  static const char* tmmbr;
  static const char* tstr;
  static const char* vbcm;

  struct Feedback {
    std::string pt;
    Type type;
    std::string parameter;
    std::string extra;
  };

  void
  PushEntry(const std::string& pt, Type type, const std::string& parameter = "",
            const std::string& extra = "")
  {
    Feedback value = { pt, type, parameter, extra };
    mFeedbacks.push_back(value);
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  std::vector<Feedback> mFeedbacks;
};

inline std::ostream& operator<<(std::ostream& os,
                                SdpRtcpFbAttributeList::Type type)
{
  switch (type) {
    case SdpRtcpFbAttributeList::kAck:
      os << "ack";
      break;
    case SdpRtcpFbAttributeList::kApp:
      os << "app";
      break;
    case SdpRtcpFbAttributeList::kCcm:
      os << "ccm";
      break;
    case SdpRtcpFbAttributeList::kNack:
      os << "nack";
      break;
    case SdpRtcpFbAttributeList::kTrrInt:
      os << "trr-int";
      break;
    default:
      MOZ_ASSERT(false);
      os << "?";
  }
  return os;
}





class SdpRtpmapAttributeList : public SdpAttribute
{
public:
  SdpRtpmapAttributeList() : SdpAttribute(kRtpmapAttribute) {}

  
  enum CodecType {
    kOpus,
    kG722,
    kPCMU,
    kPCMA,
    kVP8,
    kVP9,
    kiLBC,
    kiSAC,
    kH264,
    kOtherCodec
  };

  struct Rtpmap {
    std::string pt;
    CodecType codec;
    std::string name;
    uint32_t clock;
    
    
    uint32_t channels;
  };

  void
  PushEntry(const std::string& pt, CodecType codec, const std::string& name,
            uint32_t clock, uint32_t channels = 0)
  {
    Rtpmap value = { pt, codec, name, clock, channels };
    mRtpmaps.push_back(value);
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  bool
  HasEntry(const std::string& pt) const
  {
    for (auto it = mRtpmaps.begin(); it != mRtpmaps.end(); ++it) {
      if (it->pt == pt) {
        return true;
      }
    }
    return false;
  }

  const Rtpmap&
  GetEntry(const std::string& pt) const
  {
    for (auto it = mRtpmaps.begin(); it != mRtpmaps.end(); ++it) {
      if (it->pt == pt) {
        return *it;
      }
    }
    MOZ_CRASH();
  }

  std::vector<Rtpmap> mRtpmaps;
};

inline std::ostream& operator<<(std::ostream& os,
                                SdpRtpmapAttributeList::CodecType c)
{
  switch (c) {
    case SdpRtpmapAttributeList::kOpus:
      os << "opus";
      break;
    case SdpRtpmapAttributeList::kG722:
      os << "G722";
      break;
    case SdpRtpmapAttributeList::kPCMU:
      os << "PCMU";
      break;
    case SdpRtpmapAttributeList::kPCMA:
      os << "PCMA";
      break;
    case SdpRtpmapAttributeList::kVP8:
      os << "VP8";
      break;
    case SdpRtpmapAttributeList::kVP9:
      os << "VP9";
      break;
    case SdpRtpmapAttributeList::kiLBC:
      os << "iLBC";
      break;
    case SdpRtpmapAttributeList::kiSAC:
      os << "iSAC";
      break;
    case SdpRtpmapAttributeList::kH264:
      os << "H264";
      break;
    default:
      MOZ_ASSERT(false);
      os << "?";
  }
  return os;
}






class SdpFmtpAttributeList : public SdpAttribute
{
public:
  SdpFmtpAttributeList() : SdpAttribute(kFmtpAttribute) {}

  
  class Parameters
  {
  public:
    explicit Parameters(SdpRtpmapAttributeList::CodecType aCodec)
        : codec_type(aCodec)
    {
    }

    virtual ~Parameters() {}
    virtual Parameters* Clone() const = 0;
    virtual void Serialize(std::ostream& os) const = 0;

    SdpRtpmapAttributeList::CodecType codec_type;
  };

  class H264Parameters : public Parameters
  {
  public:
    H264Parameters()
        : Parameters(SdpRtpmapAttributeList::kH264),
          packetization_mode(0),
          level_asymmetry_allowed(false),
          profile_level_id(0),
          max_mbps(0),
          max_fs(0),
          max_cpb(0),
          max_dpb(0),
          max_br(0)
    {
      memset(sprop_parameter_sets, 0, sizeof(sprop_parameter_sets));
    }

    virtual Parameters*
    Clone() const MOZ_OVERRIDE
    {
      return new H264Parameters(*this);
    }

    virtual void
    Serialize(std::ostream& os) const MOZ_OVERRIDE
    {
      
      
      os << "profile-level-id=" << std::hex << std::setfill('0') << std::setw(6)
         << profile_level_id << std::dec << std::setfill(' ');

      os << ";level-asymmetry-allowed=" << (level_asymmetry_allowed ? 1 : 0);

      if (strlen(sprop_parameter_sets)) {
        os << ";sprop-parameter-sets=" << sprop_parameter_sets;
      }

      if (packetization_mode != 0) {
        os << ";packetization-mode=" << packetization_mode;
      }

      if (max_mbps != 0) {
        os << ";max-mbps=" << max_mbps;
      }

      if (max_fs != 0) {
        os << ";max-fs=" << max_fs;
      }

      if (max_cpb != 0) {
        os << ";max-cpb=" << max_cpb;
      }

      if (max_dpb != 0) {
        os << ";max-dpb=" << max_dpb;
      }

      if (max_br != 0) {
        os << ";max-br=" << max_br;
      }
    }

    static const size_t max_sprop_len = 128;
    char sprop_parameter_sets[max_sprop_len];
    unsigned int packetization_mode;
    bool level_asymmetry_allowed;
    unsigned int profile_level_id;
    unsigned int max_mbps;
    unsigned int max_fs;
    unsigned int max_cpb;
    unsigned int max_dpb;
    unsigned int max_br;
  };

  
  class VP8Parameters : public Parameters
  {
  public:
    VP8Parameters()
        : Parameters(SdpRtpmapAttributeList::kVP8), max_fs(0), max_fr(0)
    {
    }

    virtual Parameters*
    Clone() const MOZ_OVERRIDE
    {
      return new VP8Parameters(*this);
    }

    virtual void
    Serialize(std::ostream& os) const MOZ_OVERRIDE
    {
      
      
      os << "max-fs=" << max_fs;
      os << ";max-fr=" << max_fr;
    }

    unsigned int max_fs;
    unsigned int max_fr;
  };

  class Fmtp
  {
  public:
    Fmtp(const std::string& aFormat, const std::string& aParametersString,
         UniquePtr<Parameters> aParameters)
        : format(aFormat),
          parameters_string(aParametersString),
          parameters(Move(aParameters))
    {
    }

    
    Fmtp(const Fmtp& orig) { *this = orig; }

    Fmtp& operator=(const Fmtp& rhs)
    {
      if (this != &rhs) {
        format = rhs.format;
        parameters_string = rhs.parameters_string;
        parameters.reset(rhs.parameters ? rhs.parameters->Clone() : nullptr);
      }
      return *this;
    }

    
    
    
    
    
    
    
    
    
    
    std::string format;
    std::string parameters_string;
    UniquePtr<Parameters> parameters;
  };

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  void
  PushEntry(const std::string& format, const std::string& parameters_string,
            UniquePtr<Parameters> parameters)
  {
    mFmtps.push_back(Fmtp(format, parameters_string, Move(parameters)));
  }

  std::vector<Fmtp> mFmtps;
};
















class SdpSctpmapAttributeList : public SdpAttribute
{
public:
  SdpSctpmapAttributeList() : SdpAttribute(kSctpmapAttribute) {}

  struct Sctpmap {
    std::string pt;
    std::string name;
    uint32_t streams;
  };

  void
  PushEntry(const std::string& pt, const std::string& name,
            uint32_t streams = 0)
  {
    Sctpmap value = { pt, name, streams };
    mSctpmaps.push_back(value);
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  bool
  HasEntry(const std::string& pt) const
  {
    for (auto it = mSctpmaps.begin(); it != mSctpmaps.end(); ++it) {
      if (it->pt == pt) {
        return true;
      }
    }
    return false;
  }

  const Sctpmap&
  GetEntry(const std::string& pt) const
  {
    for (auto it = mSctpmaps.begin(); it != mSctpmaps.end(); ++it) {
      if (it->pt == pt) {
        return *it;
      }
    }
    MOZ_CRASH();
  }

  std::vector<Sctpmap> mSctpmaps;
};






class SdpSetupAttribute : public SdpAttribute
{
public:
  enum Role { kActive, kPassive, kActpass, kHoldconn };

  explicit SdpSetupAttribute(Role role)
      : SdpAttribute(kSetupAttribute), mRole(role)
  {
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  Role mRole;
};

inline std::ostream& operator<<(std::ostream& os, SdpSetupAttribute::Role r)
{
  switch (r) {
    case SdpSetupAttribute::kActive:
      os << "active";
      break;
    case SdpSetupAttribute::kPassive:
      os << "passive";
      break;
    case SdpSetupAttribute::kActpass:
      os << "actpass";
      break;
    case SdpSetupAttribute::kHoldconn:
      os << "holdconn";
      break;
    default:
      MOZ_ASSERT(false);
      os << "?";
  }
  return os;
}
















class SdpSsrcAttributeList : public SdpAttribute
{
public:
  SdpSsrcAttributeList() : SdpAttribute(kSsrcAttribute) {}

  struct Ssrc {
    uint32_t ssrc;
    std::string attribute;
  };

  void
  PushEntry(uint32_t ssrc, const std::string& attribute)
  {
    Ssrc value = { ssrc, attribute };
    mSsrcs.push_back(value);
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  std::vector<Ssrc> mSsrcs;
};









class SdpSsrcGroupAttributeList : public SdpAttribute
{
public:
  enum Semantics {
    kFec,   
    kFid,   
    kFecFr, 
    kDup    
  };

  struct SsrcGroup {
    Semantics semantics;
    std::vector<uint32_t> ssrcs;
  };

  SdpSsrcGroupAttributeList() : SdpAttribute(kSsrcGroupAttribute) {}

  void
  PushEntry(Semantics semantics, const std::vector<uint32_t>& ssrcs)
  {
    SsrcGroup value = { semantics, ssrcs };
    mSsrcGroups.push_back(value);
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  std::vector<SsrcGroup> mSsrcGroups;
};

inline std::ostream& operator<<(std::ostream& os,
                                SdpSsrcGroupAttributeList::Semantics s)
{
  switch (s) {
    case SdpSsrcGroupAttributeList::kFec:
      os << "FEC";
      break;
    case SdpSsrcGroupAttributeList::kFid:
      os << "FID";
      break;
    case SdpSsrcGroupAttributeList::kFecFr:
      os << "FEC-FR";
      break;
    case SdpSsrcGroupAttributeList::kDup:
      os << "DUP";
      break;
    default:
      MOZ_ASSERT(false);
      os << "?";
  }
  return os;
}


class SdpMultiStringAttribute : public SdpAttribute
{
public:
  explicit SdpMultiStringAttribute(AttributeType type) : SdpAttribute(type) {}

  void
  PushEntry(const std::string& entry)
  {
    mValues.push_back(entry);
  }

  virtual void Serialize(std::ostream& os) const;

  std::vector<std::string> mValues;
};




class SdpOptionsAttribute : public SdpAttribute
{
public:
  explicit SdpOptionsAttribute(AttributeType type) : SdpAttribute(type) {}

  void
  PushEntry(const std::string& entry)
  {
    mValues.push_back(entry);
  }

  void Load(const std::string& value);

  virtual void Serialize(std::ostream& os) const;

  std::vector<std::string> mValues;
};


class SdpFlagAttribute : public SdpAttribute
{
public:
  explicit SdpFlagAttribute(AttributeType type) : SdpAttribute(type) {}

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;
};


class SdpStringAttribute : public SdpAttribute
{
public:
  explicit SdpStringAttribute(AttributeType type, const std::string& value)
      : SdpAttribute(type), mValue(value)
  {
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  std::string mValue;
};


class SdpNumberAttribute : public SdpAttribute
{
public:
  explicit SdpNumberAttribute(AttributeType type, uint32_t value = 0)
      : SdpAttribute(type), mValue(value)
  {
  }

  virtual void Serialize(std::ostream& os) const MOZ_OVERRIDE;

  uint32_t mValue;
};

} 

#endif
