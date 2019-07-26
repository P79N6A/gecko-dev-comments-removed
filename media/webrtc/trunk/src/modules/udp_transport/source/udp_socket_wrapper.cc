









#include "udp_socket_wrapper.h"

#include <stdlib.h>
#include <string.h>

#include "event_wrapper.h"
#include "trace.h"
#include "udp_socket_manager_wrapper.h"

#if defined(_WIN32)
    #include "udp_socket2_windows.h"
#else
    #include "udp_socket_posix.h"
#endif


namespace webrtc {
bool UdpSocketWrapper::_initiated = false;




#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif

UdpSocketWrapper::UdpSocketWrapper()
    : _wantsIncoming(false),
      _deleteEvent(NULL)
{
}

UdpSocketWrapper::~UdpSocketWrapper()
{
    if(_deleteEvent)
    {
      _deleteEvent->Set();
      _deleteEvent = NULL;
    }
}

void UdpSocketWrapper::SetEventToNull()
{
    if (_deleteEvent)
    {
        _deleteEvent = NULL;
    }
}

UdpSocketWrapper* UdpSocketWrapper::CreateSocket(const WebRtc_Word32 id,
                                                 UdpSocketManager* mgr,
                                                 CallbackObj obj,
                                                 IncomingSocketCallback cb,
                                                 bool ipV6Enable,
                                                 bool disableGQOS)

{
    WEBRTC_TRACE(kTraceMemory, kTraceTransport, id,
                 "UdpSocketWrapper::CreateSocket");

    UdpSocketWrapper* s = 0;

#ifdef _WIN32
    if (!_initiated)
    {
        WSADATA wsaData;
        WORD wVersionRequested = MAKEWORD( 2, 2 );
        WebRtc_Word32 err = WSAStartup( wVersionRequested, &wsaData);
        if (err != 0)
        {
            WEBRTC_TRACE(
                kTraceError,
                kTraceTransport,
                id,
                "UdpSocketWrapper::CreateSocket failed to initialize sockets\
 WSAStartup error:%d",
                err);
            return NULL;
        }

        _initiated = true;
    }

    s = new UdpSocket2Windows(id, mgr, ipV6Enable, disableGQOS);

#else
    if (!_initiated)
    {
        _initiated = true;
    }
    s = new UdpSocketPosix(id, mgr, ipV6Enable);
    if (s)
    {
        UdpSocketPosix* sl = static_cast<UdpSocketPosix*>(s);
        if (sl->GetFd() != INVALID_SOCKET && sl->GetFd() < FD_SETSIZE)
        {
            
        } else
        {
            WEBRTC_TRACE(
                kTraceError,
                kTraceTransport,
                id,
                "UdpSocketWrapper::CreateSocket failed to initialize socket");
            delete s;
            s = NULL;
        }
    }
#endif
    if (s)
    {
        s->_deleteEvent = NULL;
        if (!s->SetCallback(obj, cb))
        {
            WEBRTC_TRACE(
                kTraceError,
                kTraceTransport,
                id,
                "UdpSocketWrapper::CreateSocket failed to ser callback");
            return(NULL);
        }
    }
    return s;
}

bool UdpSocketWrapper::StartReceiving()
{
    _wantsIncoming = true;
    return true;
}

bool UdpSocketWrapper::StopReceiving()
{
    _wantsIncoming = false;
    return true;
}
} 
