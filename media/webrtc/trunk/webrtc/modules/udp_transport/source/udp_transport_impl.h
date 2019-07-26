









#ifndef WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_TRANSPORT_IMPL_H_
#define WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_TRANSPORT_IMPL_H_

#include "udp_transport.h"
#include "udp_socket_wrapper.h"

namespace webrtc {
class CriticalSectionWrapper;
class RWLockWrapper;
class UdpSocketManager;

class UdpTransportImpl : public UdpTransport
{
public:
    
    class SocketFactoryInterface {
    public:
        virtual ~SocketFactoryInterface() {}
        virtual UdpSocketWrapper* CreateSocket(const WebRtc_Word32 id,
                                               UdpSocketManager* mgr,
                                               CallbackObj obj,
                                               IncomingSocketCallback cb,
                                               bool ipV6Enable,
                                               bool disableGQOS) = 0;
    };

    
    
    
    UdpTransportImpl(const WebRtc_Word32 id,
                     SocketFactoryInterface* maker,
                     UdpSocketManager* socket_manager);
    virtual ~UdpTransportImpl();

    
    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);
    virtual WebRtc_Word32 TimeUntilNextProcess();
    virtual WebRtc_Word32 Process();

    
    virtual WebRtc_Word32 InitializeSendSockets(
        const char* ipAddr,
        const WebRtc_UWord16 rtpPort,
        const WebRtc_UWord16 rtcpPort = 0);
    virtual WebRtc_Word32 InitializeReceiveSockets(
        UdpTransportData* const packetCallback,
        const WebRtc_UWord16 rtpPort,
        const char* ipAddr = NULL,
        const char* multicastIpAddr = NULL,
        const WebRtc_UWord16 rtcpPort = 0);
    virtual WebRtc_Word32 InitializeSourcePorts(
        const WebRtc_UWord16 rtpPort,
        const WebRtc_UWord16 rtcpPort = 0);
    virtual WebRtc_Word32 SourcePorts(WebRtc_UWord16& rtpPort,
                                      WebRtc_UWord16& rtcpPort) const;
    virtual WebRtc_Word32 ReceiveSocketInformation(
        char ipAddr[kIpAddressVersion6Length],
        WebRtc_UWord16& rtpPort,
        WebRtc_UWord16& rtcpPort,
        char multicastIpAddr[kIpAddressVersion6Length]) const;
    virtual WebRtc_Word32 SendSocketInformation(
        char ipAddr[kIpAddressVersion6Length],
        WebRtc_UWord16& rtpPort,
        WebRtc_UWord16& rtcpPort) const;
    virtual WebRtc_Word32 RemoteSocketInformation(
        char ipAddr[kIpAddressVersion6Length],
        WebRtc_UWord16& rtpPort,
        WebRtc_UWord16& rtcpPort) const;
    virtual WebRtc_Word32 SetQoS(const bool QoS,
                                 const WebRtc_Word32 serviceType,
                                 const WebRtc_UWord32 maxBitrate = 0,
                                 const WebRtc_Word32 overrideDSCP = 0,
                                 const bool audio = false);
    virtual WebRtc_Word32 QoS(bool& QoS, WebRtc_Word32& serviceType,
                              WebRtc_Word32& overrideDSCP) const;
    virtual WebRtc_Word32 SetToS(const WebRtc_Word32 DSCP,
                                 const bool useSetSockOpt = false);
    virtual WebRtc_Word32 ToS(WebRtc_Word32& DSCP,
                              bool& useSetSockOpt) const;
    virtual WebRtc_Word32 SetPCP(const WebRtc_Word32 PCP);
    virtual WebRtc_Word32 PCP(WebRtc_Word32& PCP) const;
    virtual WebRtc_Word32 EnableIpV6();
    virtual bool IpV6Enabled() const;
    virtual WebRtc_Word32 SetFilterIP(
        const char filterIPAddress[kIpAddressVersion6Length]);
    virtual WebRtc_Word32 FilterIP(
        char filterIPAddress[kIpAddressVersion6Length]) const;
    virtual WebRtc_Word32 SetFilterPorts(const WebRtc_UWord16 rtpFilterPort,
                                         const WebRtc_UWord16 rtcpFilterPort);
    virtual WebRtc_Word32 FilterPorts(WebRtc_UWord16& rtpFilterPort,
                                      WebRtc_UWord16& rtcpFilterPort) const;
    virtual WebRtc_Word32 StartReceiving(
        const WebRtc_UWord32 numberOfSocketBuffers);
    virtual WebRtc_Word32 StopReceiving();
    virtual bool Receiving() const;
    virtual bool SendSocketsInitialized() const;
    virtual bool SourcePortsInitialized() const;
    virtual bool ReceiveSocketsInitialized() const;
    virtual WebRtc_Word32 SendRaw(const WebRtc_Word8* data,
                                  WebRtc_UWord32 length, WebRtc_Word32 isRTCP,
                                  WebRtc_UWord16 portnr = 0,
                                  const char* ip = NULL);
    virtual WebRtc_Word32 SendRTPPacketTo(const WebRtc_Word8 *data,
                                          WebRtc_UWord32 length,
                                          const SocketAddress& to);
    virtual WebRtc_Word32 SendRTCPPacketTo(const WebRtc_Word8 *data,
                                           WebRtc_UWord32 length,
                                           const SocketAddress& to);
    virtual WebRtc_Word32 SendRTPPacketTo(const WebRtc_Word8 *data,
                                          WebRtc_UWord32 length,
                                          WebRtc_UWord16 rtpPort);
    virtual WebRtc_Word32 SendRTCPPacketTo(const WebRtc_Word8 *data,
                                           WebRtc_UWord32 length,
                                           WebRtc_UWord16 rtcpPort);
    
    virtual int SendPacket(int channel, const void* data, int length);
    virtual int SendRTCPPacket(int channel, const void* data, int length);

    
    virtual WebRtc_Word32 SetSendIP(const char* ipaddr);
    virtual WebRtc_Word32 SetSendPorts(const WebRtc_UWord16 rtpPort,
                                       const WebRtc_UWord16 rtcpPort = 0);

    virtual ErrorCode LastError() const;

    virtual WebRtc_Word32 IPAddressCached(const SocketAddress& address,
                                          char* ip,
                                          WebRtc_UWord32& ipSize,
                                          WebRtc_UWord16& sourcePort);

    WebRtc_Word32 Id() const {return _id;}
protected:
    
    
    static void IncomingRTPCallback(CallbackObj obj,
                                    const WebRtc_Word8* rtpPacket,
                                    WebRtc_Word32 rtpPacketLength,
                                    const SocketAddress* from);
    static void IncomingRTCPCallback(CallbackObj obj,
                                     const WebRtc_Word8* rtcpPacket,
                                     WebRtc_Word32 rtcpPacketLength,
                                     const SocketAddress* from);

    void CloseSendSockets();
    void CloseReceiveSockets();

    
    void BuildRemoteRTPAddr();
    
    void BuildRemoteRTCPAddr();

    void BuildSockaddrIn(WebRtc_UWord16 portnr, const char* ip,
                         SocketAddress& remoteAddr) const;

    ErrorCode BindLocalRTPSocket();
    ErrorCode BindLocalRTCPSocket();

    ErrorCode BindRTPSendSocket();
    ErrorCode BindRTCPSendSocket();

    void IncomingRTPFunction(const WebRtc_Word8* rtpPacket,
                             WebRtc_Word32 rtpPacketLength,
                             const SocketAddress* from);
    void IncomingRTCPFunction(const WebRtc_Word8* rtcpPacket,
                              WebRtc_Word32 rtcpPacketLength,
                              const SocketAddress* from);

    bool FilterIPAddress(const SocketAddress* fromAddress);

    bool SetSockOptUsed();

    WebRtc_Word32 EnableQoS(WebRtc_Word32 serviceType, bool audio,
                            WebRtc_UWord32 maxBitrate,
                            WebRtc_Word32 overrideDSCP);

    WebRtc_Word32 DisableQoS();

private:
    void GetCachedAddress(char* ip, WebRtc_UWord32& ipSize,
                          WebRtc_UWord16& sourcePort);

    WebRtc_Word32 _id;
    SocketFactoryInterface* _socket_creator;
    
    CriticalSectionWrapper* _crit;
    CriticalSectionWrapper* _critFilter;
    
    CriticalSectionWrapper* _critPacketCallback;
    UdpSocketManager* _mgr;
    ErrorCode _lastError;

    
    WebRtc_UWord16 _destPort;
    WebRtc_UWord16 _destPortRTCP;

    
    WebRtc_UWord16 _localPort;
    WebRtc_UWord16 _localPortRTCP;

    
    
    WebRtc_UWord16 _srcPort;
    WebRtc_UWord16 _srcPortRTCP;

    
    WebRtc_UWord16 _fromPort;
    WebRtc_UWord16 _fromPortRTCP;

    char _fromIP[kIpAddressVersion6Length];
    char _destIP[kIpAddressVersion6Length];
    char _localIP[kIpAddressVersion6Length];
    char _localMulticastIP[kIpAddressVersion6Length];

    UdpSocketWrapper* _ptrRtpSocket;
    UdpSocketWrapper* _ptrRtcpSocket;

    
    
    UdpSocketWrapper* _ptrSendRtpSocket;
    UdpSocketWrapper* _ptrSendRtcpSocket;

    SocketAddress _remoteRTPAddr;
    SocketAddress _remoteRTCPAddr;

    SocketAddress _localRTPAddr;
    SocketAddress _localRTCPAddr;

    WebRtc_Word32 _tos;
    bool _receiving;
    bool _useSetSockOpt;
    bool _qos;
    WebRtc_Word32 _pcp;
    bool _ipV6Enabled;
    WebRtc_Word32 _serviceType;
    WebRtc_Word32 _overrideDSCP;
    WebRtc_UWord32 _maxBitrate;

    
    RWLockWrapper* _cachLock;
    SocketAddress _previousAddress;
    char _previousIP[kIpAddressVersion6Length];
    WebRtc_UWord32 _previousIPSize;
    WebRtc_UWord16 _previousSourcePort;

    SocketAddress _filterIPAddress;
    WebRtc_UWord16 _rtpFilterPort;
    WebRtc_UWord16 _rtcpFilterPort;

    UdpTransportData* _packetCallback;
};
} 

#endif 
