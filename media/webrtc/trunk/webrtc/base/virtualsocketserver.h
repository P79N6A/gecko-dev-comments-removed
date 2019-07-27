









#ifndef WEBRTC_BASE_VIRTUALSOCKETSERVER_H_
#define WEBRTC_BASE_VIRTUALSOCKETSERVER_H_

#include <assert.h>

#include <deque>
#include <map>

#include "webrtc/base/messagequeue.h"
#include "webrtc/base/socketserver.h"

namespace rtc {

class VirtualSocket;
class SocketAddressPair;





class VirtualSocketServer : public SocketServer, public sigslot::has_slots<> {
 public:
  
  
  explicit VirtualSocketServer(SocketServer* ss);
  virtual ~VirtualSocketServer();

  SocketServer* socketserver() { return server_; }

  
  
  uint32 bandwidth() const { return bandwidth_; }
  void set_bandwidth(uint32 bandwidth) { bandwidth_ = bandwidth; }

  
  
  uint32 network_capacity() const { return network_capacity_; }
  void set_network_capacity(uint32 capacity) {
    network_capacity_ = capacity;
  }

  
  uint32 send_buffer_capacity() const { return send_buffer_capacity_; }
  void set_send_buffer_capacity(uint32 capacity) {
    send_buffer_capacity_ = capacity;
  }

  
  uint32 recv_buffer_capacity() const { return recv_buffer_capacity_; }
  void set_recv_buffer_capacity(uint32 capacity) {
    recv_buffer_capacity_ = capacity;
  }

  
  
  
  uint32 delay_mean() const { return delay_mean_; }
  uint32 delay_stddev() const { return delay_stddev_; }
  uint32 delay_samples() const { return delay_samples_; }
  void set_delay_mean(uint32 delay_mean) { delay_mean_ = delay_mean; }
  void set_delay_stddev(uint32 delay_stddev) {
    delay_stddev_ = delay_stddev;
  }
  void set_delay_samples(uint32 delay_samples) {
    delay_samples_ = delay_samples;
  }

  
  
  void UpdateDelayDistribution();

  
  
  double drop_probability() { return drop_prob_; }
  void set_drop_probability(double drop_prob) {
    assert((0 <= drop_prob) && (drop_prob <= 1));
    drop_prob_ = drop_prob;
  }

  
  virtual Socket* CreateSocket(int type);
  virtual Socket* CreateSocket(int family, int type);

  virtual AsyncSocket* CreateAsyncSocket(int type);
  virtual AsyncSocket* CreateAsyncSocket(int family, int type);

  
  virtual void SetMessageQueue(MessageQueue* queue);
  virtual bool Wait(int cms, bool process_io);
  virtual void WakeUp();

  typedef std::pair<double, double> Point;
  typedef std::vector<Point> Function;

  static Function* CreateDistribution(uint32 mean, uint32 stddev,
                                      uint32 samples);

  
  
  
  bool ProcessMessagesUntilIdle();

  
  void SetNextPortForTesting(uint16 port);

 protected:
  
  IPAddress GetNextIP(int family);
  uint16 GetNextPort();

  VirtualSocket* CreateSocketInternal(int family, int type);

  
  int Bind(VirtualSocket* socket, SocketAddress* addr);

  
  int Bind(VirtualSocket* socket, const SocketAddress& addr);

  
  VirtualSocket* LookupBinding(const SocketAddress& addr);

  int Unbind(const SocketAddress& addr, VirtualSocket* socket);

  
  void AddConnection(const SocketAddress& client,
                     const SocketAddress& server,
                     VirtualSocket* socket);

  
  VirtualSocket* LookupConnection(const SocketAddress& client,
                                  const SocketAddress& server);

  void RemoveConnection(const SocketAddress& client,
                        const SocketAddress& server);

  
  int Connect(VirtualSocket* socket, const SocketAddress& remote_addr,
              bool use_delay);

  
  bool Disconnect(VirtualSocket* socket);

  
  int SendUdp(VirtualSocket* socket, const char* data, size_t data_size,
              const SocketAddress& remote_addr);

  
  void SendTcp(VirtualSocket* socket);

  
  void AddPacketToNetwork(VirtualSocket* socket, VirtualSocket* recipient,
                          uint32 cur_time, const char* data, size_t data_size,
                          size_t header_size, bool ordered);

  
  void PurgeNetworkPackets(VirtualSocket* socket, uint32 cur_time);

  
  uint32 SendDelay(uint32 size);

  
  uint32 GetRandomTransitDelay();

  
  
  static Function* Accumulate(Function* f);
  static Function* Invert(Function* f);
  static Function* Resample(Function* f, double x1, double x2, uint32 samples);
  static double Evaluate(Function* f, double x);

  
  
  
  void OnMessageQueueDestroyed() { msg_queue_ = NULL; }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static bool CanInteractWith(VirtualSocket* local, VirtualSocket* remote);

 private:
  friend class VirtualSocket;

  typedef std::map<SocketAddress, VirtualSocket*> AddressMap;
  typedef std::map<SocketAddressPair, VirtualSocket*> ConnectionMap;

  SocketServer* server_;
  bool server_owned_;
  MessageQueue* msg_queue_;
  bool stop_on_idle_;
  uint32 network_delay_;
  in_addr next_ipv4_;
  in6_addr next_ipv6_;
  uint16 next_port_;
  AddressMap* bindings_;
  ConnectionMap* connections_;

  uint32 bandwidth_;
  uint32 network_capacity_;
  uint32 send_buffer_capacity_;
  uint32 recv_buffer_capacity_;
  uint32 delay_mean_;
  uint32 delay_stddev_;
  uint32 delay_samples_;
  Function* delay_dist_;
  CriticalSection delay_crit_;

  double drop_prob_;
  DISALLOW_EVIL_CONSTRUCTORS(VirtualSocketServer);
};

}  

#endif  
