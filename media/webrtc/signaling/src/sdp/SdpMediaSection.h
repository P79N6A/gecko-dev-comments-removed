





#ifndef _SDPMEDIASECTION_H_
#define _SDPMEDIASECTION_H_

#include "mozilla/Maybe.h"
#include "signaling/src/sdp/SdpEnum.h"
#include "signaling/src/sdp/SdpAttributeList.h"
#include <string>
#include <vector>

#include "signaling/src/sdp/SdpEnum.h"

namespace mozilla
{

class SdpAttributeList;

class SdpConnection;

class SdpMediaSection
{
public:
  enum MediaType { kAudio, kVideo, kText, kApplication, kMessage };

  enum Protocol {
    kRtpAvp,            
    kUdp,               
    kVat,               
    kRtp,               
    kUdptl,             
    kTcp,               
    kRtpAvpf,           
    kTcpRtpAvp,         
    kRtpSavp,           
    kTcpBfcp,           
    kTcpTlsBfcp,        
    kTcpTls,            
    kFluteUdp,          
    kTcpMsrp,           
    kTcpTlsMsrp,        
    kDccp,              
    kDccpRtpAvp,        
    kDccpRtpSavp,       
    kDccpRtpAvpf,       
    kDccpRtpSavpf,      
    kRtpSavpf,          
    kUdpTlsRtpSavp,     
    kTcpTlsRtpSavp,     
    kDccpTlsRtpSavp,    
    kUdpTlsRtpSavpf,    
    kTcpTlsRtpSavpf,    
    kDccpTlsRtpSavpf,   
    kUdpMbmsFecRtpAvp,  
    kUdpMbmsFecRtpSavp, 
    kUdpMbmsRepair,     
    kFecUdp,            
    kUdpFec,            
    kTcpMrcpv2,         
    kTcpTlsMrcpv2,      
    kPstn,              
    kUdpTlsUdptl,       
    kSctp,              
    kSctpDtls,          
    kDtlsSctp           
  };

  explicit SdpMediaSection(size_t level) : mLevel(level) {}

  virtual MediaType GetMediaType() const = 0;
  virtual unsigned int GetPort() const = 0;
  virtual void SetPort(unsigned int port) = 0;
  virtual unsigned int GetPortCount() const = 0;
  virtual Protocol GetProtocol() const = 0;
  virtual const SdpConnection& GetConnection() const = 0;
  virtual SdpConnection& GetConnection() = 0;
  virtual uint32_t GetBandwidth(const std::string& type) const = 0;
  virtual const std::vector<std::string>& GetFormats() const = 0;

  virtual const SdpAttributeList& GetAttributeList() const = 0;
  virtual SdpAttributeList& GetAttributeList() = 0;

  virtual SdpDirectionAttribute GetDirectionAttribute() const = 0;

  virtual void Serialize(std::ostream&) const = 0;

  virtual void AddCodec(const std::string& pt, const std::string& name,
                        uint32_t clockrate, uint16_t channels) = 0;
  virtual void ClearCodecs() = 0;

  virtual void AddDataChannel(const std::string& pt, const std::string& name,
                              uint16_t streams) = 0;

  size_t
  GetLevel() const
  {
    return mLevel;
  }

  inline bool
  IsReceiving() const
  {
    return GetDirectionAttribute().mValue & SdpDirectionAttribute::kRecvFlag;
  }

  inline bool
  IsSending() const
  {
    return GetDirectionAttribute().mValue & SdpDirectionAttribute::kSendFlag;
  }

  inline void
  SetReceiving(bool receiving)
  {
    auto direction = GetDirectionAttribute().mValue;
    if (direction & SdpDirectionAttribute::kSendFlag) {
      SetDirection(receiving ?
                   SdpDirectionAttribute::kSendrecv :
                   SdpDirectionAttribute::kSendonly);
    } else {
      SetDirection(receiving ?
                   SdpDirectionAttribute::kRecvonly :
                   SdpDirectionAttribute::kInactive);
    }
  }

  inline void
  SetSending(bool sending)
  {
    auto direction = GetDirectionAttribute().mValue;
    if (direction & SdpDirectionAttribute::kRecvFlag) {
      SetDirection(sending ?
                   SdpDirectionAttribute::kSendrecv :
                   SdpDirectionAttribute::kRecvonly);
    } else {
      SetDirection(sending ?
                   SdpDirectionAttribute::kSendonly :
                   SdpDirectionAttribute::kInactive);
    }
  }

  inline void SetDirection(SdpDirectionAttribute::Direction direction)
  {
    GetAttributeList().SetAttribute(new SdpDirectionAttribute(direction));
  }

private:
  size_t mLevel;
};

inline std::ostream& operator<<(std::ostream& os, const SdpMediaSection& ms)
{
  ms.Serialize(os);
  return os;
}

inline std::ostream& operator<<(std::ostream& os, SdpMediaSection::MediaType t)
{
  switch (t) {
    case SdpMediaSection::kAudio:
      return os << "audio";
    case SdpMediaSection::kVideo:
      return os << "video";
    case SdpMediaSection::kText:
      return os << "text";
    case SdpMediaSection::kApplication:
      return os << "application";
    case SdpMediaSection::kMessage:
      return os << "message";
  }
  MOZ_ASSERT(false, "Unknown MediaType");
  return os << "?";
}

inline std::ostream& operator<<(std::ostream& os, SdpMediaSection::Protocol p)
{
  switch (p) {
    case SdpMediaSection::kRtpAvp:
      return os << "RTP/AVP";
    case SdpMediaSection::kUdp:
      return os << "udp";
    case SdpMediaSection::kVat:
      return os << "vat";
    case SdpMediaSection::kRtp:
      return os << "rtp";
    case SdpMediaSection::kUdptl:
      return os << "udptl";
    case SdpMediaSection::kTcp:
      return os << "TCP";
    case SdpMediaSection::kRtpAvpf:
      return os << "RTP/AVPF";
    case SdpMediaSection::kTcpRtpAvp:
      return os << "TCP/RTP/AVP";
    case SdpMediaSection::kRtpSavp:
      return os << "RTP/SAVP";
    case SdpMediaSection::kTcpBfcp:
      return os << "TCP/BFCP";
    case SdpMediaSection::kTcpTlsBfcp:
      return os << "TCP/TLS/BFCP";
    case SdpMediaSection::kTcpTls:
      return os << "TCP/TLS";
    case SdpMediaSection::kFluteUdp:
      return os << "FLUTE/UDP";
    case SdpMediaSection::kTcpMsrp:
      return os << "TCP/MSRP";
    case SdpMediaSection::kTcpTlsMsrp:
      return os << "TCP/TLS/MSRP";
    case SdpMediaSection::kDccp:
      return os << "DCCP";
    case SdpMediaSection::kDccpRtpAvp:
      return os << "DCCP/RTP/AVP";
    case SdpMediaSection::kDccpRtpSavp:
      return os << "DCCP/RTP/SAVP";
    case SdpMediaSection::kDccpRtpAvpf:
      return os << "DCCP/RTP/AVPF";
    case SdpMediaSection::kDccpRtpSavpf:
      return os << "DCCP/RTP/SAVPF";
    case SdpMediaSection::kRtpSavpf:
      return os << "RTP/SAVPF";
    case SdpMediaSection::kUdpTlsRtpSavp:
      return os << "UDP/TLS/RTP/SAVP";
    case SdpMediaSection::kTcpTlsRtpSavp:
      return os << "TCP/TLS/RTP/SAVP";
    case SdpMediaSection::kDccpTlsRtpSavp:
      return os << "DCCP/TLS/RTP/SAVP";
    case SdpMediaSection::kUdpTlsRtpSavpf:
      return os << "UDP/TLS/RTP/SAVPF";
    case SdpMediaSection::kTcpTlsRtpSavpf:
      return os << "TCP/TLS/RTP/SAVPF";
    case SdpMediaSection::kDccpTlsRtpSavpf:
      return os << "DCCP/TLS/RTP/SAVPF";
    case SdpMediaSection::kUdpMbmsFecRtpAvp:
      return os << "UDP/MBMS-FEC/RTP/AVP";
    case SdpMediaSection::kUdpMbmsFecRtpSavp:
      return os << "UDP/MBMS-FEC/RTP/SAVP";
    case SdpMediaSection::kUdpMbmsRepair:
      return os << "UDP/MBMS-REPAIR";
    case SdpMediaSection::kFecUdp:
      return os << "FEC/UDP";
    case SdpMediaSection::kUdpFec:
      return os << "UDP/FEC";
    case SdpMediaSection::kTcpMrcpv2:
      return os << "TCP/MRCPv2";
    case SdpMediaSection::kTcpTlsMrcpv2:
      return os << "TCP/TLS/MRCPv2";
    case SdpMediaSection::kPstn:
      return os << "PSTN";
    case SdpMediaSection::kUdpTlsUdptl:
      return os << "UDP/TLS/UDPTL";
    case SdpMediaSection::kSctp:
      return os << "SCTP";
    case SdpMediaSection::kSctpDtls:
      return os << "SCTP/DTLS";
    case SdpMediaSection::kDtlsSctp:
      return os << "DTLS/SCTP";
  }
  MOZ_ASSERT(false, "Unknown Protocol");
  return os << "?";
}

class SdpConnection
{
public:
  SdpConnection(sdp::AddrType addrType, std::string addr, uint8_t ttl = 0,
                uint32_t count = 0)
      : mAddrType(addrType), mAddr(addr), mTtl(ttl), mCount(count)
  {
  }
  ~SdpConnection() {}

  sdp::AddrType
  GetAddrType() const
  {
    return mAddrType;
  }
  const std::string&
  GetAddress() const
  {
    return mAddr;
  }
  void
  SetAddress(const std::string& address)
  {
    mAddr = address;
  }
  uint8_t
  GetTtl() const
  {
    return mTtl;
  }
  uint32_t
  GetCount() const
  {
    return mCount;
  }

  void
  Serialize(std::ostream& os) const
  {
    sdp::NetType netType = sdp::kInternet;

    os << "c=" << netType << " " << mAddrType << " " << mAddr;

    if (mTtl) {
      os << "/" << static_cast<uint32_t>(mTtl);
      if (mCount) {
        os << "/" << mCount;
      }
    }
    os << "\r\n";
  }

private:
  sdp::AddrType mAddrType;
  std::string mAddr;
  uint8_t mTtl;    
  uint32_t mCount; 
};

inline std::ostream& operator<<(std::ostream& os, const SdpConnection& c)
{
  c.Serialize(os);
  return os;
}

} 

#endif
