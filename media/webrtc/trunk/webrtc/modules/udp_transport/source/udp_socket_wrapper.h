









#ifndef WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_SOCKET_WRAPPER_H_
#define WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_SOCKET_WRAPPER_H_

#include "udp_transport.h"

namespace webrtc {
class EventWrapper;
class UdpSocketManager;

#define SOCKET_ERROR_NO_QOS -1000

#ifndef _WIN32
typedef int SOCKET;
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET  (SOCKET)(~0)

#ifndef AF_INET
#define AF_INET 2
#endif

#endif

typedef void* CallbackObj;
typedef void(*IncomingSocketCallback)(CallbackObj obj, const WebRtc_Word8* buf,
                                      WebRtc_Word32 len,
                                      const SocketAddress* from);

class UdpSocketWrapper
{
public:
    static UdpSocketWrapper* CreateSocket(const WebRtc_Word32 id,
                                          UdpSocketManager* mgr,
                                          CallbackObj obj,
                                          IncomingSocketCallback cb,
                                          bool ipV6Enable = false,
                                          bool disableGQOS = false);

    
    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id) = 0;

    
    
    virtual bool SetCallback(CallbackObj obj, IncomingSocketCallback cb) = 0;

    
    virtual bool Bind(const SocketAddress& name) = 0;

    
    virtual bool StartReceiving();
    virtual inline bool StartReceiving(const WebRtc_UWord32 )
    {return StartReceiving();}
    
    virtual bool StopReceiving();

    virtual bool ValidHandle() = 0;

    
    virtual bool SetSockopt(WebRtc_Word32 level, WebRtc_Word32 optname,
                            const WebRtc_Word8* optval,
                            WebRtc_Word32 optlen) = 0;

    
    virtual WebRtc_Word32 SetTOS(const WebRtc_Word32 serviceType) = 0;

    
    virtual WebRtc_Word32 SetPCP(const WebRtc_Word32 ) {return -1;}

    
    virtual WebRtc_Word32 SendTo(const WebRtc_Word8* buf, WebRtc_Word32 len,
                                 const SocketAddress& to) = 0;

    virtual void SetEventToNull();

    
    virtual void CloseBlocking() {}

    
    virtual bool SetQos(WebRtc_Word32 serviceType, WebRtc_Word32 tokenRate,
                        WebRtc_Word32 bucketSize, WebRtc_Word32 peekBandwith,
                        WebRtc_Word32 minPolicedSize, WebRtc_Word32 maxSduSize,
                        const SocketAddress &stRemName,
                        WebRtc_Word32 overrideDSCP = 0) = 0;

    virtual WebRtc_UWord32 ReceiveBuffers() {return 0;};

protected:
    
    UdpSocketWrapper();
    
    virtual ~UdpSocketWrapper();

    bool _wantsIncoming;
    EventWrapper*  _deleteEvent;

private:
    static bool _initiated;
};
} 

#endif 
