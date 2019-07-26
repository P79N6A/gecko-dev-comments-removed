









#ifndef WEBRTC_TEST_CHANNEL_TRANSPORT_UDP_SOCKET_POSIX_H_
#define WEBRTC_TEST_CHANNEL_TRANSPORT_UDP_SOCKET_POSIX_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "webrtc/system_wrappers/interface/condition_variable_wrapper.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/test/channel_transport/udp_socket_wrapper.h"

namespace webrtc {
namespace test {

#define SOCKET_ERROR -1

class UdpSocketPosix : public UdpSocketWrapper
{
public:
    UdpSocketPosix(const int32_t id, UdpSocketManager* mgr,
                   bool ipV6Enable = false);

    virtual ~UdpSocketPosix();

    virtual int32_t ChangeUniqueId(const int32_t id);

    virtual bool SetCallback(CallbackObj obj, IncomingSocketCallback cb);

    virtual bool Bind(const SocketAddress& name);

    virtual bool SetSockopt(int32_t level, int32_t optname,
                            const int8_t* optval, int32_t optlen);

    virtual int32_t SetTOS(const int32_t serviceType);

    virtual int32_t SendTo(const int8_t* buf, int32_t len,
                           const SocketAddress& to);

    
    
    virtual void CloseBlocking();

    virtual SOCKET GetFd() {return _socket;}
    virtual int32_t GetError() {return _error;}

    virtual bool ValidHandle();

    virtual bool SetQos(int32_t ,
                        int32_t ,
                        int32_t ,
                        int32_t ,
                        int32_t ,
                        int32_t ,
                        const SocketAddress& ,
                        int32_t ) {return false;}

    bool CleanUp();
    void HasIncoming();
    bool WantsIncoming() {return _wantsIncoming;}
    void ReadyForDeletion();
private:
    friend class UdpSocketManagerPosix;

    int32_t _id;
    IncomingSocketCallback _incomingCb;
    CallbackObj _obj;
    int32_t _error;

    SOCKET _socket;
    UdpSocketManager* _mgr;
    ConditionVariableWrapper* _closeBlockingCompletedCond;
    ConditionVariableWrapper* _readyForDeletionCond;

    bool _closeBlockingActive;
    bool _closeBlockingCompleted;
    bool _readyForDeletion;

    CriticalSectionWrapper* _cs;
};

}  
}  

#endif  
