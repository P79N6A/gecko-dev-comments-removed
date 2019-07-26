





















#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "webrtc/test/channel_transport/udp_socket_wrapper.h"
#include "webrtc/test/channel_transport/udp_socket_manager_wrapper.h"

using ::testing::_;
using ::testing::Return;

namespace webrtc {
namespace test {

class MockSocketManager : public UdpSocketManager {
 public:
  MockSocketManager() {}
  
  void Destroy() {
    delete this;
  }
  MOCK_METHOD2(Init, bool(int32_t, uint8_t&));
  MOCK_METHOD1(ChangeUniqueId, int32_t(const int32_t));
  MOCK_METHOD0(Start, bool());
  MOCK_METHOD0(Stop, bool());
  MOCK_METHOD1(AddSocket, bool(UdpSocketWrapper*));
  MOCK_METHOD1(RemoveSocket, bool(UdpSocketWrapper*));
};



TEST(UdpSocketWrapper, CreateSocket) {
  int32_t id = 42;
  
  uint8_t threads = 1;
  UdpSocketManager* mgr = UdpSocketManager::Create(id, threads);
  UdpSocketWrapper* socket =
      UdpSocketWrapper::CreateSocket(id,
                                     mgr,
                                     NULL,  
                                     NULL,  
                                     false,  
                                     false);  
  socket->CloseBlocking();
  UdpSocketManager::Return();
}

}  
}  
