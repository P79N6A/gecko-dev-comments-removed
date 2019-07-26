
















#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/test/channel_transport/udp_socket_manager_wrapper.h"
#include "webrtc/test/channel_transport/udp_socket_wrapper.h"

namespace webrtc {
namespace test {

TEST(UdpSocketManager, CreateCallsInitAndDoesNotLeakMemory) {
  int32_t id = 42;
  uint8_t threads = 1;
  UdpSocketManager* mgr = UdpSocketManager::Create(id, threads);
  
  EXPECT_FALSE(mgr->Init(id, threads))
      << "Init should return false since Create is supposed to call it.";
  UdpSocketManager::Return();
}



TEST(UdpSocketManager, AddAndRemoveSocketDoesNotLeakMemory) {
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
  
  
  EXPECT_EQ(true, mgr->RemoveSocket(socket));
  UdpSocketManager::Return();
}






TEST(UdpSocketManager, UnremovedSocketsGetCollectedAtManagerDeletion) {
#if defined(_WIN32)
  
#else
  int32_t id = 42;
  uint8_t threads = 1;
  UdpSocketManager* mgr = UdpSocketManager::Create(id, threads);
  UdpSocketWrapper* unused_socket = UdpSocketWrapper::CreateSocket(
      id,
      mgr,
      NULL,  
      NULL,  
      false,  
      false);  
  
  
  
  unused_socket->SetEventToNull();
  unused_socket = NULL;
  UdpSocketManager::Return();
#endif
}

}  
}  
