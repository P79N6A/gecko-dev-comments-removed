





#ifndef _SDPATTRIBUTELIST_H_
#define _SDPATTRIBUTELIST_H_

#include "mozilla/UniquePtr.h"
#include "mozilla/Attributes.h"

#include "signaling/src/sdp/SdpAttribute.h"

namespace mozilla
{

class SdpAttributeList
{
public:
  typedef SdpAttribute::AttributeType AttributeType;

  
  bool
  HasAttribute(AttributeType type) const
  {
    return HasAttribute(type, true);
  }

  const SdpAttribute*
  GetAttribute(AttributeType type) const
  {
    return GetAttribute(type, true);
  }

  virtual bool HasAttribute(AttributeType type, bool sessionFallback) const = 0;
  virtual const SdpAttribute* GetAttribute(AttributeType type,
                                           bool sessionFallback) const = 0;
  
  virtual void SetAttribute(SdpAttribute* attr) = 0;
  virtual void RemoveAttribute(AttributeType type) = 0;
  virtual void Clear() = 0;

  virtual const SdpConnectionAttribute& GetConnection() const = 0;
  virtual const SdpOptionsAttribute& GetIceOptions() const = 0;
  virtual const SdpRtcpAttribute& GetRtcp() const = 0;
  virtual const SdpRemoteCandidatesAttribute& GetRemoteCandidates() const = 0;
  virtual const SdpSetupAttribute& GetSetup() const = 0;

  
  
  virtual const std::vector<std::string>& GetCandidate() const = 0;
  virtual const SdpExtmapAttributeList& GetExtmap() const = 0;
  virtual const SdpFingerprintAttributeList& GetFingerprint() const = 0;
  virtual const SdpFmtpAttributeList& GetFmtp() const = 0;
  virtual const SdpGroupAttributeList& GetGroup() const = 0;
  virtual const SdpImageattrAttributeList& GetImageattr() const = 0;
  virtual const SdpMsidAttributeList& GetMsid() const = 0;
  virtual const SdpRtcpFbAttributeList& GetRtcpFb() const = 0;
  virtual const SdpRtpmapAttributeList& GetRtpmap() const = 0;
  virtual const SdpSctpmapAttributeList& GetSctpmap() const = 0;
  virtual const SdpSsrcAttributeList& GetSsrc() const = 0;
  virtual const SdpSsrcGroupAttributeList& GetSsrcGroup() const = 0;

  
  
  virtual const std::string& GetIcePwd() const = 0;
  virtual const std::string& GetIceUfrag() const = 0;
  virtual const std::string& GetIdentity() const = 0;
  virtual const std::string& GetLabel() const = 0;
  virtual unsigned int GetMaxptime() const = 0;
  virtual const std::string& GetMid() const = 0;
  virtual const std::string& GetMsidSemantic() const = 0;
  virtual unsigned int GetPtime() const = 0;

  
  virtual SdpDirectionAttribute::Direction GetDirection() const = 0;

  virtual void Serialize(std::ostream&) const = 0;
};

inline std::ostream& operator<<(std::ostream& os, const SdpAttributeList& al)
{
  al.Serialize(os);
  return os;
}

} 

#endif
