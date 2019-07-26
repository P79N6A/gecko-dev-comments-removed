









#ifndef WEBRTC_MODULES_UDP_TRANSPORT_INTERFACE_UDP_TRANSPORT_H_
#define WEBRTC_MODULES_UDP_TRANSPORT_INTERFACE_UDP_TRANSPORT_H_

#include "common_types.h"
#include "module.h"
#include "typedefs.h"









#define SS_MAXSIZE 128
#define SS_ALIGNSIZE (sizeof (WebRtc_UWord64))
#define SS_PAD1SIZE  (SS_ALIGNSIZE - sizeof(WebRtc_Word16))
#define SS_PAD2SIZE  (SS_MAXSIZE - (sizeof(WebRtc_Word16) + SS_PAD1SIZE +\
                                    SS_ALIGNSIZE))


namespace webrtc {
struct SocketAddressIn
{
    
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    WebRtc_Word8      sin_length;
    WebRtc_Word8      sin_family;
#else
    WebRtc_Word16     sin_family;
#endif
    WebRtc_UWord16    sin_port;
    WebRtc_UWord32    sin_addr;
    WebRtc_Word8      sin_zero[8];
};

struct Version6InAddress
{
    union
    {
        WebRtc_UWord8     _s6_u8[16];
        WebRtc_UWord32    _s6_u32[4];
        WebRtc_UWord64    _s6_u64[2];
    } Version6AddressUnion;
};

struct SocketAddressInVersion6
{
    
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    WebRtc_Word8      sin_length;
    WebRtc_Word8      sin_family;
#else
    WebRtc_Word16     sin_family;
#endif
    
    WebRtc_UWord16 sin6_port;
    
    WebRtc_UWord32 sin6_flowinfo;
    
    struct Version6InAddress sin6_addr;
    
    WebRtc_UWord32 sin6_scope_id;
};

struct SocketAddressStorage
{
    
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    WebRtc_Word8   sin_length;
    WebRtc_Word8   sin_family;
#else
    WebRtc_Word16  sin_family;
#endif
    WebRtc_Word8   __ss_pad1[SS_PAD1SIZE];
    WebRtc_UWord64 __ss_align;
    WebRtc_Word8   __ss_pad2[SS_PAD2SIZE];
};

struct SocketAddress
{
    union
    {
        struct SocketAddressIn _sockaddr_in;
        struct SocketAddressInVersion6 _sockaddr_in6;
        struct SocketAddressStorage _sockaddr_storage;
    };
};


class UdpTransportData
{
public:
    virtual ~UdpTransportData()  {};

    virtual void IncomingRTPPacket(const WebRtc_Word8* incomingRtpPacket,
                                   const WebRtc_Word32 rtpPacketLength,
                                   const char* fromIP,
                                   const WebRtc_UWord16 fromPort) = 0;

    virtual void IncomingRTCPPacket(const WebRtc_Word8* incomingRtcpPacket,
                                    const WebRtc_Word32 rtcpPacketLength,
                                    const char* fromIP,
                                    const WebRtc_UWord16 fromPort) = 0;
};


class UdpTransport : public Module, public Transport
{
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

    
    static UdpTransport* Create(const WebRtc_Word32 id,
                                WebRtc_UWord8& numSocketThreads);
    static void Destroy(UdpTransport* module);

    
    
    
    virtual WebRtc_Word32 InitializeSendSockets(
        const char* ipAddr,
        const WebRtc_UWord16 rtpPort,
        const WebRtc_UWord16 rtcpPort = 0) = 0;

    
    
    
    
    virtual WebRtc_Word32 InitializeReceiveSockets(
        UdpTransportData* const packetCallback,
        const WebRtc_UWord16 rtpPort,
        const char* ipAddr = NULL,
        const char* multicastIpAddr = NULL,
        const WebRtc_UWord16 rtcpPort = 0) = 0;

    
    
    
    virtual WebRtc_Word32 InitializeSourcePorts(
        const WebRtc_UWord16 rtpPort,
        const WebRtc_UWord16 rtcpPort = 0) = 0;

    
    
    
    virtual WebRtc_Word32 SourcePorts(WebRtc_UWord16& rtpPort,
                                      WebRtc_UWord16& rtcpPort) const = 0;

    
    
    
    
    virtual WebRtc_Word32 ReceiveSocketInformation(
        char ipAddr[kIpAddressVersion6Length],
        WebRtc_UWord16& rtpPort,
        WebRtc_UWord16& rtcpPort,
        char multicastIpAddr[kIpAddressVersion6Length]) const = 0;

    
    
    
    virtual WebRtc_Word32 SendSocketInformation(
        char ipAddr[kIpAddressVersion6Length],
        WebRtc_UWord16& rtpPort,
        WebRtc_UWord16& rtcpPort) const = 0;

    
    
    virtual WebRtc_Word32 RemoteSocketInformation(
        char ipAddr[kIpAddressVersion6Length],
        WebRtc_UWord16& rtpPort,
        WebRtc_UWord16& rtcpPort) const = 0;

    
    
    
    
    
    virtual WebRtc_Word32 SetQoS(const bool QoS,
                                 const WebRtc_Word32 serviceType,
                                 const WebRtc_UWord32 maxBitrate = 0,
                                 const WebRtc_Word32 overrideDSCP = 0,
                                 const bool audio = false) = 0;

    
    
    
    virtual WebRtc_Word32 QoS(bool& QoS,
                              WebRtc_Word32& serviceType,
                              WebRtc_Word32& overrideDSCP) const = 0;

    
    virtual WebRtc_Word32 SetToS(const WebRtc_Word32 DSCP,
                                 const bool useSetSockOpt = false) = 0;

    
    virtual WebRtc_Word32 ToS(WebRtc_Word32& DSCP,
                              bool& useSetSockOpt) const = 0;

    
    
    
    virtual WebRtc_Word32 SetPCP(const WebRtc_Word32 PCP) = 0;

    
    virtual WebRtc_Word32 PCP(WebRtc_Word32& PCP) const = 0;

    
    
    
    
    virtual WebRtc_Word32 EnableIpV6() = 0;

    
    virtual bool IpV6Enabled() const = 0;

    
    
    virtual WebRtc_Word32 SetFilterIP(
        const char filterIPAddress[kIpAddressVersion6Length]) = 0;

    
    virtual WebRtc_Word32 FilterIP(
        char filterIPAddress[kIpAddressVersion6Length]) const = 0;

    
    
    
    virtual WebRtc_Word32 SetFilterPorts(
        const WebRtc_UWord16 rtpFilterPort,
        const WebRtc_UWord16 rtcpFilterPort) = 0;

    
    
    virtual WebRtc_Word32 FilterPorts(WebRtc_UWord16& rtpFilterPort,
                                      WebRtc_UWord16& rtcpFilterPort) const = 0;

    
    
    
    
    virtual WebRtc_Word32 StartReceiving(
        const WebRtc_UWord32 numberOfSocketBuffers) = 0;

    
    virtual WebRtc_Word32 StopReceiving() = 0;

    
    virtual bool Receiving() const = 0;

    
    virtual bool SendSocketsInitialized() const = 0;

    
    virtual bool SourcePortsInitialized() const = 0;

    
    virtual bool ReceiveSocketsInitialized() const = 0;

    
    
    
    
    virtual WebRtc_Word32 SendRaw(const WebRtc_Word8* data,
                                  WebRtc_UWord32 length,
                                  WebRtc_Word32 isRTCP,
                                  WebRtc_UWord16 portnr = 0,
                                  const char* ip = NULL) = 0;

    
    virtual WebRtc_Word32 SendRTPPacketTo(const WebRtc_Word8* data,
                                          WebRtc_UWord32 length,
                                          const SocketAddress& to) = 0;


    
    virtual WebRtc_Word32 SendRTCPPacketTo(const WebRtc_Word8* data,
                                           WebRtc_UWord32 length,
                                           const SocketAddress& to) = 0;

    
    
    virtual WebRtc_Word32 SendRTPPacketTo(const WebRtc_Word8* data,
                                          WebRtc_UWord32 length,
                                          WebRtc_UWord16 rtpPort) = 0;


    
    
    virtual WebRtc_Word32 SendRTCPPacketTo(const WebRtc_Word8* data,
                                           WebRtc_UWord32 length,
                                           WebRtc_UWord16 rtcpPort) = 0;

    
    virtual WebRtc_Word32 SetSendIP(
        const char ipaddr[kIpAddressVersion6Length]) = 0;

    
    virtual WebRtc_Word32 SetSendPorts(const WebRtc_UWord16 rtpPort,
                                       const WebRtc_UWord16 rtcpPort = 0) = 0;

    
    virtual ErrorCode LastError() const = 0;

    
    
    static WebRtc_Word32 LocalHostAddress(WebRtc_UWord32& localIP);

    
    
    static WebRtc_Word32 LocalHostAddressIPV6(char localIP[16]);

    
    static WebRtc_UWord16 Htons(WebRtc_UWord16 hostOrder);

    
    static WebRtc_UWord32 Htonl(WebRtc_UWord32 hostOrder);

    
    static WebRtc_UWord32 InetAddrIPV4(const char* ip);

    
    
    
    static WebRtc_Word32 InetPresentationToNumeric(WebRtc_Word32 af,
                                                   const char* src,
                                                   void* dst);

    
    
    
    
    static WebRtc_Word32 IPAddress(const SocketAddress& address,
                                   char* ip,
                                   WebRtc_UWord32& ipSize,
                                   WebRtc_UWord16& sourcePort);

    
    
    
    
    
    
    
    virtual WebRtc_Word32 IPAddressCached(const SocketAddress& address,
                                          char* ip,
                                          WebRtc_UWord32& ipSize,
                                          WebRtc_UWord16& sourcePort) = 0;

    
    
    
    static bool IsIpAddressValid(const char* ipaddr, const bool ipV6);
};
} 

#endif
