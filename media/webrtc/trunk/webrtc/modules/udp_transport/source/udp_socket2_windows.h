









#ifndef WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_SOCKET2_WINDOWS_H_
#define WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_SOCKET2_WINDOWS_H_


#pragma warning(disable : 4995)


#include <Winsock2.h>
#include <Ntddndis.h>
#include <traffic.h>

#include "atomic32.h"
#include "condition_variable_wrapper.h"
#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "list_wrapper.h"
#include "rw_lock_wrapper.h"
#include "trace.h"
#include "udp_socket_wrapper.h"
#include "udp_socket2_manager_windows.h"

namespace webrtc {
class UdpSocket2ManagerWindows;
class TrafficControlWindows;
struct PerIoContext;

class UdpSocket2Windows : public UdpSocketWrapper
{
public:
    UdpSocket2Windows(const WebRtc_Word32 id, UdpSocketManager* mgr,
                      bool ipV6Enable = false, bool disableGQOS = false);
    virtual ~UdpSocket2Windows();

    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

    virtual bool ValidHandle();

    virtual bool SetCallback(CallbackObj, IncomingSocketCallback);

    virtual bool Bind(const SocketAddress& name);
    virtual bool SetSockopt(WebRtc_Word32 level, WebRtc_Word32 optname,
                            const WebRtc_Word8* optval, WebRtc_Word32 optlen);

    virtual bool StartReceiving(const WebRtc_UWord32 receiveBuffers);
    virtual inline bool StartReceiving() {return StartReceiving(8);}
    virtual bool StopReceiving();

    virtual WebRtc_Word32 SendTo(const WebRtc_Word8* buf, WebRtc_Word32 len,
                                 const SocketAddress& to);

    virtual void CloseBlocking();

    virtual SOCKET GetFd() { return _socket;}
    virtual bool SetQos(WebRtc_Word32 serviceType, WebRtc_Word32 tokenRate,
                        WebRtc_Word32 bucketSize, WebRtc_Word32 peekBandwith,
                        WebRtc_Word32 minPolicedSize, WebRtc_Word32 maxSduSize,
                        const SocketAddress &stRemName,
                        WebRtc_Word32 overrideDSCP = 0);

    virtual WebRtc_Word32 SetTOS(const WebRtc_Word32 serviceType);
    virtual WebRtc_Word32 SetPCP(const WebRtc_Word32 pcp);

    virtual WebRtc_UWord32 ReceiveBuffers(){return _receiveBuffers.Value();}

protected:
    void IOCompleted(PerIoContext* pIOContext, WebRtc_UWord32 ioSize,
                     WebRtc_UWord32 error);

    WebRtc_Word32 PostRecv();
    
    WebRtc_Word32 PostRecv(PerIoContext* pIoContext);

private:
    friend class UdpSocket2WorkerWindows;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 SetTrafficControl(WebRtc_Word32 dscp, WebRtc_Word32 pcp,
                                    const struct sockaddr_in* name,
                                    FLOWSPEC* send = NULL,
                                    FLOWSPEC* recv = NULL);
    WebRtc_Word32 CreateFlowSpec(WebRtc_Word32 serviceType,
                                 WebRtc_Word32 tokenRate,
                                 WebRtc_Word32 bucketSize,
                                 WebRtc_Word32 peekBandwith,
                                 WebRtc_Word32 minPolicedSize,
                                 WebRtc_Word32 maxSduSize, FLOWSPEC *f);

    WebRtc_Word32 _id;
    RWLockWrapper* _ptrCbRWLock;
    IncomingSocketCallback _incomingCb;
    CallbackObj _obj;
    bool _qos;

    SocketAddress _remoteAddr;
    SOCKET _socket;
    WebRtc_Word32 _iProtocol;
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
#endif 
