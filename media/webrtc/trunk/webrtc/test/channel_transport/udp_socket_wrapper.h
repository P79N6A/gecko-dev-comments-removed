









#ifndef WEBRTC_TEST_CHANNEL_TRANSPORT_UDP_SOCKET_WRAPPER_H_
#define WEBRTC_TEST_CHANNEL_TRANSPORT_UDP_SOCKET_WRAPPER_H_

#include "webrtc/test/channel_transport/udp_transport.h"

namespace webrtc {

class EventWrapper;

namespace test {

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
typedef void(*IncomingSocketCallback)(CallbackObj obj, const int8_t* buf,
                                      int32_t len, const SocketAddress* from);

class UdpSocketWrapper
{
public:
    static UdpSocketWrapper* CreateSocket(const int32_t id,
                                          UdpSocketManager* mgr,
                                          CallbackObj obj,
                                          IncomingSocketCallback cb,
                                          bool ipV6Enable = false,
                                          bool disableGQOS = false);

    
    virtual int32_t ChangeUniqueId(const int32_t id) = 0;

    
    
    virtual bool SetCallback(CallbackObj obj, IncomingSocketCallback cb) = 0;

    
    virtual bool Bind(const SocketAddress& name) = 0;

    
    virtual bool StartReceiving();
    virtual bool StartReceiving(const uint32_t );
    
    virtual bool StopReceiving();

    virtual bool ValidHandle() = 0;

    
    virtual bool SetSockopt(int32_t level, int32_t optname,
                            const int8_t* optval, int32_t optlen) = 0;

    
    virtual int32_t SetTOS(const int32_t serviceType) = 0;

    
    virtual int32_t SetPCP(const int32_t );

    
    virtual int32_t SendTo(const int8_t* buf, int32_t len,
                           const SocketAddress& to) = 0;

    virtual void SetEventToNull();

    
    virtual void CloseBlocking() {}

    
    virtual bool SetQos(int32_t serviceType, int32_t tokenRate,
                        int32_t bucketSize, int32_t peekBandwith,
                        int32_t minPolicedSize, int32_t maxSduSize,
                        const SocketAddress &stRemName,
                        int32_t overrideDSCP = 0) = 0;

    virtual uint32_t ReceiveBuffers();

protected:
    
    UdpSocketWrapper();
    
    virtual ~UdpSocketWrapper();

    bool _wantsIncoming;
    EventWrapper*  _deleteEvent;

private:
    static bool _initiated;
};

}  
}  

#endif  
