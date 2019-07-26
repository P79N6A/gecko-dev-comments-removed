









#ifndef WEBRTC_TEST_CHANNEL_TRANSPORT_UDP_SOCKET_MANAGER_WRAPPER_H_
#define WEBRTC_TEST_CHANNEL_TRANSPORT_UDP_SOCKET_MANAGER_WRAPPER_H_

#include "webrtc/system_wrappers/interface/static_instance.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class UdpSocketWrapper;

class UdpSocketManager
{
public:
    static UdpSocketManager* Create(const int32_t id,
                                    uint8_t& numOfWorkThreads);
    static void Return();

    
    
    virtual bool Init(int32_t id, uint8_t& numOfWorkThreads) = 0;

    virtual int32_t ChangeUniqueId(const int32_t id) = 0;

    
    
    virtual bool Start() = 0;
    
    virtual bool Stop() = 0;

    virtual uint8_t WorkThreads() const;

    
    virtual bool AddSocket(UdpSocketWrapper* s) = 0;
    
    virtual bool RemoveSocket(UdpSocketWrapper* s) = 0;

protected:
    UdpSocketManager();
    virtual ~UdpSocketManager() {}

    uint8_t _numOfWorkThreads;

    
    static UdpSocketManager* CreateInstance();

private:
    
    
    friend UdpSocketManager* webrtc::GetStaticInstance<UdpSocketManager>(
        CountOperation count_operation);

    static UdpSocketManager* StaticInstance(
        CountOperation count_operation,
        const int32_t id,
        uint8_t& numOfWorkThreads);
};

}  
}  

#endif  
