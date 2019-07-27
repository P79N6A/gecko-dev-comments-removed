









#ifndef WEBRTC_BASE_TESTCLIENT_H_
#define WEBRTC_BASE_TESTCLIENT_H_

#include <vector>
#include "webrtc/base/asyncudpsocket.h"
#include "webrtc/base/criticalsection.h"

namespace rtc {



class TestClient : public sigslot::has_slots<> {
 public:
  
  struct Packet {
    Packet(const SocketAddress& a, const char* b, size_t s);
    Packet(const Packet& p);
    virtual ~Packet();

    SocketAddress addr;
    char*  buf;
    size_t size;
  };

  
  
  explicit TestClient(AsyncPacketSocket* socket);
  ~TestClient();

  SocketAddress address() const { return socket_->GetLocalAddress(); }
  SocketAddress remote_address() const { return socket_->GetRemoteAddress(); }

  
  bool CheckConnState(AsyncPacketSocket::State state);

  
  bool CheckConnected() {
    return CheckConnState(AsyncPacketSocket::STATE_CONNECTED);
  }

  
  int Send(const char* buf, size_t size);

  
  int SendTo(const char* buf, size_t size, const SocketAddress& dest);

  
  
  
  Packet* NextPacket();

  
  
  bool CheckNextPacket(const char* buf, size_t len, SocketAddress* addr);

  
  bool CheckNoPacket();

  int GetError();
  int SetOption(Socket::Option opt, int value);

  bool ready_to_send() const;

 private:
  static const int kTimeout = 1000;
  
  Socket::ConnState GetState();
  
  void OnPacket(AsyncPacketSocket* socket, const char* buf, size_t len,
                const SocketAddress& remote_addr,
                const PacketTime& packet_time);
  void OnReadyToSend(AsyncPacketSocket* socket);

  CriticalSection crit_;
  AsyncPacketSocket* socket_;
  std::vector<Packet*>* packets_;
  bool ready_to_send_;
  DISALLOW_EVIL_CONSTRUCTORS(TestClient);
};

}  

#endif  
