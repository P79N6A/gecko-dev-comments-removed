
















#include "udp_socket_wrapper.h"
#include "udp_socket_manager_wrapper.h"
#include "gtest/gtest.h"
#include "system_wrappers/interface/trace.h"

namespace webrtc {

TEST(UdpSocketManager, CreateCallsInitAndDoesNotLeakMemory) {
  WebRtc_Word32 id = 42;
  WebRtc_UWord8 threads = 1;
  UdpSocketManager* mgr = UdpSocketManager::Create(id, threads);
  
  EXPECT_FALSE(mgr->Init(id, threads))
      << "Init should return false since Create is supposed to call it.";
  UdpSocketManager::Return();
}



TEST(UdpSocketManager, AddAndRemoveSocketDoesNotLeakMemory) {
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
  
  
  EXPECT_EQ(true, mgr->RemoveSocket(socket));
  UdpSocketManager::Return();
}






TEST(UdpSocketManager, UnremovedSocketsGetCollectedAtManagerDeletion) {
#if defined(_WIN32)
  
#else
  WebRtc_Word32 id = 42;
  WebRtc_UWord8 threads = 1;
  UdpSocketManager* mgr = UdpSocketManager::Create(id, threads);
  UdpSocketWrapper* unused_socket
       = UdpSocketWrapper::CreateSocket(id,
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
