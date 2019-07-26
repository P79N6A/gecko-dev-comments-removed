









#ifndef WEBRTC_TEST_CHANNEL_TRANSPORT_UDP_SOCKET2_WINDOWS_H_
#define WEBRTC_TEST_CHANNEL_TRANSPORT_UDP_SOCKET2_WINDOWS_H_


#pragma warning(disable : 4995)


#include <Winsock2.h>
#include <Ntddndis.h>
#include <traffic.h>

#include "webrtc/system_wrappers/interface/atomic32.h"
#include "webrtc/system_wrappers/interface/condition_variable_wrapper.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/test/channel_transport/udp_socket2_manager_win.h"
#include "webrtc/test/channel_transport/udp_socket_wrapper.h"

namespace webrtc {
namespace test {

class UdpSocket2ManagerWindows;
class TrafficControlWindows;
struct PerIoContext;

class UdpSocket2Windows : public UdpSocketWrapper
{
public:
    UdpSocket2Windows(const int32_t id, UdpSocketManager* mgr,
                      bool ipV6Enable = false, bool disableGQOS = false);
    virtual ~UdpSocket2Windows();

    virtual int32_t ChangeUniqueId(const int32_t id);

    virtual bool ValidHandle();

    virtual bool SetCallback(CallbackObj, IncomingSocketCallback);

    virtual bool Bind(const SocketAddress& name);
    virtual bool SetSockopt(int32_t level, int32_t optname,
                            const int8_t* optval, int32_t optlen);

    virtual bool StartReceiving(const uint32_t receiveBuffers);
    virtual inline bool StartReceiving() {return StartReceiving(8);}
    virtual bool StopReceiving();

    virtual int32_t SendTo(const int8_t* buf, int32_t len,
                           const SocketAddress& to);

    virtual void CloseBlocking();

    virtual SOCKET GetFd() { return _socket;}
    virtual bool SetQos(int32_t serviceType, int32_t tokenRate,
                        int32_t bucketSize, int32_t peekBandwith,
                        int32_t minPolicedSize, int32_t maxSduSize,
                        const SocketAddress &stRemName,
                        int32_t overrideDSCP = 0);

    virtual int32_t SetTOS(const int32_t serviceType);
    virtual int32_t SetPCP(const int32_t pcp);

    virtual uint32_t ReceiveBuffers(){return _receiveBuffers.Value();}

protected:
    void IOCompleted(PerIoContext* pIOContext, uint32_t ioSize, uint32_t error);

    int32_t PostRecv();
    
    int32_t PostRecv(PerIoContext* pIoContext);

private:
    friend class UdpSocket2WorkerWindows;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    int32_t SetTrafficControl(int32_t dscp, int32_t pcp,
                              const struct sockaddr_in* name,
                              FLOWSPEC* send = NULL,
                              FLOWSPEC* recv = NULL);
    int32_t CreateFlowSpec(int32_t serviceType,
                           int32_t tokenRate,
                           int32_t bucketSize,
                           int32_t peekBandwith,
                           int32_t minPolicedSize,
                           int32_t maxSduSize, FLOWSPEC *f);

    int32_t _id;
    RWLockWrapper* _ptrCbRWLock;
    IncomingSocketCallback _incomingCb;
    CallbackObj _obj;
    bool _qos;

    SocketAddress _remoteAddr;
    SOCKET _socket;
    int32_t _iProtocol;
    UdpSocket2ManagerWindows* _mgr;

    CriticalSectionWrapper* _pCrit;
    Atomic32 _outstandingCalls;
    Atomic32 _outstandingCallComplete;
    volatile bool _terminate;
    volatile bool _addedToMgr;

    CriticalSectionWrapper* _ptrDeleteCrit;
    ConditionVariableWrapper* _ptrDeleteCond;
    bool _safeTodelete;

    RWLockWrapper* _ptrDestRWLock;
    bool _outstandingCallsDisabled;
    bool NewOutstandingCall();
    void OutstandingCallCompleted();
    void DisableNewOutstandingCalls();
    void WaitForOutstandingCalls();

    void RemoveSocketFromManager();

    
    
    
    RWLockWrapper* _ptrSocketRWLock;
    bool AquireSocket();
    void ReleaseSocket();
    bool InvalidateSocket();

    
    HANDLE _clientHandle;
    HANDLE _flowHandle;
    HANDLE _filterHandle;
    PTC_GEN_FLOW _flow;
    
    TrafficControlWindows* _gtc;
    
    int _pcp;

    Atomic32 _receiveBuffers;
};

}  
}  

#endif  
