









#ifndef WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_SOCKET_MANAGER_WRAPPER_H_
#define WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_SOCKET_MANAGER_WRAPPER_H_

#include "system_wrappers/interface/static_instance.h"
#include "typedefs.h"

namespace webrtc {

class UdpSocketWrapper;

class UdpSocketManager
{
public:
    static UdpSocketManager* Create(const WebRtc_Word32 id,
                                    WebRtc_UWord8& numOfWorkThreads);
    static void Return();

    
    
    virtual bool Init(WebRtc_Word32 id,
                      WebRtc_UWord8& numOfWorkThreads) = 0;

    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id) = 0;

    
    
    virtual bool Start() = 0;
    
    virtual bool Stop() = 0;

    virtual WebRtc_UWord8 WorkThreads() const;

    
    virtual bool AddSocket(UdpSocketWrapper* s) = 0;
    
    virtual bool RemoveSocket(UdpSocketWrapper* s) = 0;

protected:
    UdpSocketManager();
    virtual ~UdpSocketManager() {}

    WebRtc_UWord8 _numOfWorkThreads;

    
    static UdpSocketManager* CreateInstance();

private:
    
    
    friend UdpSocketManager*
    GetStaticInstance<UdpSocketManager>(CountOperation count_operation);

    static UdpSocketManager* StaticInstance(
        CountOperation count_operation,
        const WebRtc_Word32 id,
        WebRtc_UWord8& numOfWorkThreads);
};

} 

#endif 
