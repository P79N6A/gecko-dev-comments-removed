









#ifndef WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_SOCKET_MANAGER_WINDOWS_H_
#define WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_SOCKET_MANAGER_WINDOWS_H_

#define FD_SETSIZE 1024

#include <winsock2.h>
#include <map>
#include <list>



#include "udp_socket_manager_wrapper.h"
#include "thread_wrapper.h"
#include "critical_section_wrapper.h"

namespace webrtc {
class UdpSocketWindows;

class UdpSocketManagerWindows : public UdpSocketManager
{
public:
    UdpSocketManagerWindows();
    virtual ~UdpSocketManagerWindows();

    virtual bool Init(WebRtc_Word32 id,
                      WebRtc_UWord8& numOfWorkThreads);

    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

    virtual bool Start();
    virtual bool Stop();

    virtual bool AddSocket(UdpSocketWrapper* s);
    virtual bool RemoveSocket(UdpSocketWrapper* s);

protected:
    static bool Run(ThreadObj obj);
    bool Process();

private:
    WebRtc_Word32 _id;
    ThreadWrapper* _thread;

    fd_set _readFds;
    fd_set _writeFds;
    fd_set _exceptFds;

    CriticalSectionWrapper* _critSectList;

    std::map<SOCKET, UdpSocketWindows*> _socketMap;
    std::list<UdpSocketWindows*> _addList;
    std::list<SOCKET> _removeList;

    static WebRtc_UWord32 _numOfActiveManagers;
};
} 

#endif 
