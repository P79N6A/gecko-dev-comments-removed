









#ifndef WEBRTC_TEST_CHANNEL_TRANSPORT_UDP_TRANSPORT_H_
#define WEBRTC_TEST_CHANNEL_TRANSPORT_UDP_TRANSPORT_H_

#include "webrtc/common_types.h"
#include "webrtc/typedefs.h"








#define SS_MAXSIZE 128
#define SS_ALIGNSIZE (sizeof (uint64_t))
#define SS_PAD1SIZE  (SS_ALIGNSIZE - sizeof(int16_t))
#define SS_PAD2SIZE  (SS_MAXSIZE - (sizeof(int16_t) + SS_PAD1SIZE +\
                                    SS_ALIGNSIZE))


namespace webrtc {
namespace test {

struct SocketAddressIn {
  
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
  int8_t      sin_length;
  int8_t      sin_family;
#else
  int16_t     sin_family;
#endif
  uint16_t    sin_port;
  uint32_t    sin_addr;
  int8_t      sin_zero[8];
};

struct Version6InAddress {
  union {
    uint8_t     _s6_u8[16];
    uint32_t    _s6_u32[4];
    uint64_t    _s6_u64[2];
  } Version6AddressUnion;
};

struct SocketAddressInVersion6 {
  
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
  int8_t      sin_length;
  int8_t      sin_family;
#else
  int16_t     sin_family;
#endif
  
  uint16_t sin6_port;
  
  uint32_t sin6_flowinfo;
  
  struct Version6InAddress sin6_addr;
  
  uint32_t sin6_scope_id;
};

struct SocketAddressStorage {
  
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
  int8_t   sin_length;
  int8_t   sin_family;
#else
  int16_t  sin_family;
#endif
  int8_t   __ss_pad1[SS_PAD1SIZE];
  uint64_t __ss_align;
  int8_t   __ss_pad2[SS_PAD2SIZE];
};

struct SocketAddress {
  union {
    struct SocketAddressIn _sockaddr_in;
    struct SocketAddressInVersion6 _sockaddr_in6;
    struct SocketAddressStorage _sockaddr_storage;
  };
};


class UdpTransportData {
 public:
  virtual ~UdpTransportData()  {};

  virtual void IncomingRTPPacket(const int8_t* incomingRtpPacket,
                                 const int32_t rtpPacketLength,
                                 const char* fromIP,
                                 const uint16_t fromPort) = 0;

  virtual void IncomingRTCPPacket(const int8_t* incomingRtcpPacket,
                                  const int32_t rtcpPacketLength,
                                  const char* fromIP,
                                  const uint16_t fromPort) = 0;
};

class UdpTransport : public Transport {
 public:
    enum
    {
        kIpAddressVersion6Length = 64,
        kIpAddressVersion4Length = 16
    };
    enum ErrorCode
    {
        kNoSocketError            = 0,
        kFailedToBindPort         = 1,
        kIpAddressInvalid         = 2,
        kAddressInvalid           = 3,
        kSocketInvalid            = 4,
        kPortInvalid              = 5,
        kTosInvalid               = 6,
        kMulticastAddressInvalid  = 7,
        kQosError                 = 8,
        kSocketAlreadyInitialized = 9,
        kIpVersion6Error          = 10,
        FILTER_ERROR              = 11,
        kStartReceiveError        = 12,
        kStopReceiveError         = 13,
        kCannotFindLocalIp        = 14,
        kTosError                 = 16,
        kNotInitialized           = 17,
        kPcpError                 = 18
    };

    
    static UdpTransport* Create(const int32_t id, uint8_t& numSocketThreads);
    static void Destroy(UdpTransport* module);

    
    
    
    virtual int32_t InitializeSendSockets(const char* ipAddr,
                                          const uint16_t rtpPort,
                                          const uint16_t rtcpPort = 0) = 0;

    
    
    
    
    virtual int32_t InitializeReceiveSockets(
        UdpTransportData* const packetCallback,
        const uint16_t rtpPort,
        const char* ipAddr = NULL,
        const char* multicastIpAddr = NULL,
        const uint16_t rtcpPort = 0) = 0;

    
    
    
    virtual int32_t InitializeSourcePorts(const uint16_t rtpPort,
                                          const uint16_t rtcpPort = 0) = 0;

    
    
    
    virtual int32_t SourcePorts(uint16_t& rtpPort,
                                uint16_t& rtcpPort) const = 0;

    
    
    
    
    virtual int32_t ReceiveSocketInformation(
        char ipAddr[kIpAddressVersion6Length],
        uint16_t& rtpPort,
        uint16_t& rtcpPort,
        char multicastIpAddr[kIpAddressVersion6Length]) const = 0;

    
    
    
    virtual int32_t SendSocketInformation(char ipAddr[kIpAddressVersion6Length],
                                          uint16_t& rtpPort,
                                          uint16_t& rtcpPort) const = 0;

    
    
    virtual int32_t RemoteSocketInformation(
        char ipAddr[kIpAddressVersion6Length],
        uint16_t& rtpPort,
        uint16_t& rtcpPort) const = 0;

    
    
    
    
    
    virtual int32_t SetQoS(const bool QoS,
                           const int32_t serviceType,
                           const uint32_t maxBitrate = 0,
                           const int32_t overrideDSCP = 0,
                           const bool audio = false) = 0;

    
    
    
    virtual int32_t QoS(bool& QoS,
                        int32_t& serviceType,
                        int32_t& overrideDSCP) const = 0;

    
    virtual int32_t SetToS(const int32_t DSCP,
                           const bool useSetSockOpt = false) = 0;

    
    virtual int32_t ToS(int32_t& DSCP,
                        bool& useSetSockOpt) const = 0;

    
    
    
    virtual int32_t SetPCP(const int32_t PCP) = 0;

    
    virtual int32_t PCP(int32_t& PCP) const = 0;

    
    
    
    
    virtual int32_t EnableIpV6() = 0;

    
    virtual bool IpV6Enabled() const = 0;

    
    
    virtual int32_t SetFilterIP(
        const char filterIPAddress[kIpAddressVersion6Length]) = 0;

    
    virtual int32_t FilterIP(
        char filterIPAddress[kIpAddressVersion6Length]) const = 0;

    
    
    
    virtual int32_t SetFilterPorts(const uint16_t rtpFilterPort,
                                   const uint16_t rtcpFilterPort) = 0;

    
    
    virtual int32_t FilterPorts(uint16_t& rtpFilterPort,
                                uint16_t& rtcpFilterPort) const = 0;

    
    
    
    
    virtual int32_t StartReceiving(const uint32_t numberOfSocketBuffers) = 0;

    
    virtual int32_t StopReceiving() = 0;

    
    virtual bool Receiving() const = 0;

    
    virtual bool SendSocketsInitialized() const = 0;

    
    virtual bool SourcePortsInitialized() const = 0;

    
    virtual bool ReceiveSocketsInitialized() const = 0;

    
    
    
    
    virtual int32_t SendRaw(const int8_t* data,
                            uint32_t length,
                            int32_t isRTCP,
                            uint16_t portnr = 0,
                            const char* ip = NULL) = 0;

    
    virtual int32_t SendRTPPacketTo(const int8_t* data,
                                    uint32_t length,
                                    const SocketAddress& to) = 0;


    
    virtual int32_t SendRTCPPacketTo(const int8_t* data,
                                     uint32_t length,
                                     const SocketAddress& to) = 0;

    
    
    virtual int32_t SendRTPPacketTo(const int8_t* data,
                                    uint32_t length,
                                    uint16_t rtpPort) = 0;


    
    
    virtual int32_t SendRTCPPacketTo(const int8_t* data,
                                     uint32_t length,
                                     uint16_t rtcpPort) = 0;

    
    virtual int32_t SetSendIP(
        const char ipaddr[kIpAddressVersion6Length]) = 0;

    
    virtual int32_t SetSendPorts(const uint16_t rtpPort,
                                 const uint16_t rtcpPort = 0) = 0;

    
    virtual ErrorCode LastError() const = 0;

    
    
    static int32_t LocalHostAddress(uint32_t& localIP);

    
    
    static int32_t LocalHostAddressIPV6(char localIP[16]);

    
    static uint16_t Htons(uint16_t hostOrder);

    
    static uint32_t Htonl(uint32_t hostOrder);

    
    static uint32_t InetAddrIPV4(const char* ip);

    
    
    
    static int32_t InetPresentationToNumeric(int32_t af,
                                             const char* src,
                                             void* dst);

    
    
    
    
    static int32_t IPAddress(const SocketAddress& address,
                             char* ip,
                             uint32_t& ipSize,
                             uint16_t& sourcePort);

    
    
    
    
    
    
    
    virtual int32_t IPAddressCached(const SocketAddress& address,
                                    char* ip,
                                    uint32_t& ipSize,
                                    uint16_t& sourcePort) = 0;

    
    
    
    static bool IsIpAddressValid(const char* ipaddr, const bool ipV6);
};

}  
}  

#endif
