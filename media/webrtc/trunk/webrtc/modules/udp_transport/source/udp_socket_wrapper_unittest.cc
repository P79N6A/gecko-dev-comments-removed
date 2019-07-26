





















#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "modules/udp_transport/source/udp_socket_wrapper.h"
#include "modules/udp_transport/source/udp_socket_manager_wrapper.h"

using ::testing::_;
using ::testing::Return;

namespace webrtc {

class MockSocketManager : public UdpSocketManager {
 public:
  MockSocketManager() {}
  
  void Destroy() {
    delete this;
  }
  MOCK_METHOD2(Init, bool(WebRtc_Word32, WebRtc_UWord8&));
  MOCK_METHOD1(ChangeUniqueId, WebRtc_Word32(const WebRtc_Word32));
  MOCK_METHOD0(Start, bool());
  MOCK_METHOD0(Stop, bool());
  MOCK_METHOD1(AddSocket, bool(webrtc::UdpSocketWrapper*));
  MOCK_METHOD1(RemoveSocket, bool(webrtc::UdpSocketWrapper*));
};



TEST(UdpSocketWrapper, CreateSocket) {
  WebRtc_Word32 id = 42;
  
  WebRtc_UWord8 threads = 1;
  UdpSocketManager* mgr = UdpSocketManager::Create(id, threads);
  UdpSocketWrapper* socket
       = UdpSocketWrapper::CreateSocket(id,
                                        mgr,
                                        NULL,  
                                        NULL,  
                                        false,  
                                        false);  
  socket->CloseBlocking();
  UdpSocketManager::Return();
}

}  
