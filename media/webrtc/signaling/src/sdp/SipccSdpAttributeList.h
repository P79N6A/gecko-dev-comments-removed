





#ifndef _SIPCCSDPATTRIBUTELIST_H_
#define _SIPCCSDPATTRIBUTELIST_H_

#include "signaling/src/sdp/SdpAttributeList.h"

extern "C" {
#include "signaling/src/sdp/sipcc/sdp.h"
}

namespace mozilla
{

class SipccSdp;
class SipccSdpMediaSection;
class SdpErrorHolder;

class SipccSdpAttributeList : public SdpAttributeList
{
  friend class SipccSdpMediaSection;
  friend class SipccSdp;

public:
  
  using SdpAttributeList::HasAttribute;
  using SdpAttributeList::GetAttribute;

  virtual bool HasAttribute(AttributeType type,
                            bool sessionFallback) const override;
  virtual const SdpAttribute* GetAttribute(
      AttributeType type, bool sessionFallback) const override;
  virtual void SetAttribute(SdpAttribute* attr) override;
  virtual void RemoveAttribute(AttributeType type) override;
  virtual void Clear() override;

  virtual const SdpConnectionAttribute& GetConnection() const override;
  virtual const SdpFingerprintAttributeList& GetFingerprint() const
      override;
  virtual const SdpGroupAttributeList& GetGroup() const override;
  virtual const SdpOptionsAttribute& GetIceOptions() const override;
  virtual const SdpRtcpAttribute& GetRtcp() const override;
  virtual const SdpRemoteCandidatesAttribute& GetRemoteCandidates() const
      override;
  virtual const SdpSetupAttribute& GetSetup() const override;
  virtual const SdpSsrcAttributeList& GetSsrc() const override;
  virtual const SdpSsrcGroupAttributeList& GetSsrcGroup() const override;

  
  
  virtual const std::vector<std::string>& GetCandidate() const override;
  virtual const SdpExtmapAttributeList& GetExtmap() const override;
  virtual const SdpFmtpAttributeList& GetFmtp() const override;
  virtual const SdpImageattrAttributeList& GetImageattr() const override;
  virtual const SdpMsidAttributeList& GetMsid() const override;
  virtual const SdpRtcpFbAttributeList& GetRtcpFb() const override;
  virtual const SdpRtpmapAttributeList& GetRtpmap() const override;
  virtual const SdpSctpmapAttributeList& GetSctpmap() const override;

  
  
  virtual const std::string& GetIcePwd() const override;
  virtual const std::string& GetIceUfrag() const override;
  virtual const std::string& GetIdentity() const override;
  virtual const std::string& GetLabel() const override;
  virtual unsigned int GetMaxptime() const override;
  virtual const std::string& GetMid() const override;
  virtual const SdpMsidSemanticAttributeList& GetMsidSemantic()
    const override;
  virtual unsigned int GetPtime() const override;

  virtual SdpDirectionAttribute::Direction GetDirection() const override;

  virtual void Serialize(std::ostream&) const override;

  virtual ~SipccSdpAttributeList();

private:
  static const std::string kEmptyString;
  static const size_t kNumAttributeTypes = SdpAttribute::kLastAttribute + 1;

  
  
  explicit SipccSdpAttributeList(const SipccSdpAttributeList* sessionLevel);

  bool Load(sdp_t* sdp, uint16_t level, SdpErrorHolder& errorHolder);
  void LoadSimpleStrings(sdp_t* sdp, uint16_t level,
                         SdpErrorHolder& errorHolder);
  void LoadSimpleString(sdp_t* sdp, uint16_t level, sdp_attr_e attr,
                        AttributeType targetType, SdpErrorHolder& errorHolder);
  void LoadSimpleNumbers(sdp_t* sdp, uint16_t level,
                         SdpErrorHolder& errorHolder);
  void LoadSimpleNumber(sdp_t* sdp, uint16_t level, sdp_attr_e attr,
                        AttributeType targetType, SdpErrorHolder& errorHolder);
  void LoadFlags(sdp_t* sdp, uint16_t level);
  void LoadDirection(sdp_t* sdp, uint16_t level, SdpErrorHolder& errorHolder);
  bool LoadRtpmap(sdp_t* sdp, uint16_t level, SdpErrorHolder& errorHolder);
  bool LoadSctpmap(sdp_t* sdp, uint16_t level, SdpErrorHolder& errorHolder);
  void LoadIceAttributes(sdp_t* sdp, uint16_t level);
  bool LoadFingerprint(sdp_t* sdp, uint16_t level, SdpErrorHolder& errorHolder);
  void LoadCandidate(sdp_t* sdp, uint16_t level);
  void LoadSetup(sdp_t* sdp, uint16_t level);
  void LoadSsrc(sdp_t* sdp, uint16_t level);
  bool LoadGroups(sdp_t* sdp, uint16_t level, SdpErrorHolder& errorHolder);
  bool LoadMsidSemantics(sdp_t* sdp,
                         uint16_t level,
                         SdpErrorHolder& errorHolder);
  void LoadFmtp(sdp_t* sdp, uint16_t level);
  void LoadMsids(sdp_t* sdp, uint16_t level, SdpErrorHolder& errorHolder);
  void LoadExtmap(sdp_t* sdp, uint16_t level, SdpErrorHolder& errorHolder);
  void LoadRtcpFb(sdp_t* sdp, uint16_t level, SdpErrorHolder& errorHolder);
  static SdpRtpmapAttributeList::CodecType GetCodecType(rtp_ptype type);

  bool
  AtSessionLevel() const
  {
    return !mSessionLevel;
  }
  bool IsAllowedHere(SdpAttribute::AttributeType type) const;
  void WarnAboutMisplacedAttribute(SdpAttribute::AttributeType type,
                                   uint32_t lineNumber,
                                   SdpErrorHolder& errorHolder);

  const SipccSdpAttributeList* mSessionLevel;

  SdpAttribute* mAttributes[kNumAttributeTypes];

  SipccSdpAttributeList(const SipccSdpAttributeList& orig) = delete;
  SipccSdpAttributeList& operator=(const SipccSdpAttributeList& rhs) = delete;
};

} 

#endif
