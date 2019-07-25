












#include "udp_transport.h"
#include "gtest/gtest.h"


#include "udp_transport_impl.h"

TEST(UDPTransportTest, CreateTransport) {
  WebRtc_Word32 id = 0;
  WebRtc_UWord8 threads = 0;
  webrtc::UdpTransport* transport = webrtc::UdpTransport::Create(id, threads);
  webrtc::UdpTransport::Destroy(transport);
}


TEST(UDPTransportTest, ConstructorDoesNotCreateSocket) {
  WebRtc_Word32 id = 0;
  WebRtc_UWord8 threads = 0;
  webrtc::UdpTransportImpl::SocketMaker* null_maker = NULL;
  webrtc::UdpTransport* transport = new webrtc::UdpTransportImpl(id, threads,
                                                                 null_maker);
  webrtc::UdpTransport::Destroy(transport);
}
