









#include "webrtc/base/virtualsocketserver.h"

#include <errno.h>
#include <math.h>

#include <algorithm>
#include <map>
#include <vector>

#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/socketaddresspair.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/timeutils.h"

namespace rtc {
#if defined(WEBRTC_WIN)
const in_addr kInitialNextIPv4 = { {0x01, 0, 0, 0} };
#else

const in_addr kInitialNextIPv4 = { 0x01000000 };
#endif

const in6_addr kInitialNextIPv6 = { { {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2
    } } };

const uint16 kFirstEphemeralPort = 49152;
const uint16 kLastEphemeralPort = 65535;
const uint16 kEphemeralPortCount = kLastEphemeralPort - kFirstEphemeralPort + 1;
const uint32 kDefaultNetworkCapacity = 64 * 1024;
const uint32 kDefaultTcpBufferSize = 32 * 1024;

const uint32 UDP_HEADER_SIZE = 28;  
const uint32 TCP_HEADER_SIZE = 40;  
const uint32 TCP_MSS = 1400;  


const int NUM_SAMPLES = 1000;

enum {
  MSG_ID_PACKET,
  MSG_ID_CONNECT,
  MSG_ID_DISCONNECT,
};



class Packet : public MessageData {
 public:
  Packet(const char* data, size_t size, const SocketAddress& from)
        : size_(size), consumed_(0), from_(from) {
    ASSERT(NULL != data);
    data_ = new char[size_];
    memcpy(data_, data, size_);
  }

  virtual ~Packet() {
    delete[] data_;
  }

  const char* data() const { return data_ + consumed_; }
  size_t size() const { return size_ - consumed_; }
  const SocketAddress& from() const { return from_; }

  
  void Consume(size_t size) {
    ASSERT(size + consumed_ < size_);
    consumed_ += size;
  }

 private:
  char* data_;
  size_t size_, consumed_;
  SocketAddress from_;
};

struct MessageAddress : public MessageData {
  explicit MessageAddress(const SocketAddress& a) : addr(a) { }
  SocketAddress addr;
};



class VirtualSocket : public AsyncSocket, public MessageHandler {
 public:
  VirtualSocket(VirtualSocketServer* server, int family, int type, bool async)
      : server_(server), family_(family), type_(type), async_(async),
        state_(CS_CLOSED), error_(0), listen_queue_(NULL),
        write_enabled_(false),
        network_size_(0), recv_buffer_size_(0), bound_(false), was_any_(false) {
    ASSERT((type_ == SOCK_DGRAM) || (type_ == SOCK_STREAM));
    ASSERT(async_ || (type_ != SOCK_STREAM));  
  }

  virtual ~VirtualSocket() {
    Close();

    for (RecvBuffer::iterator it = recv_buffer_.begin();
         it != recv_buffer_.end(); ++it) {
      delete *it;
    }
  }

  virtual SocketAddress GetLocalAddress() const {
    return local_addr_;
  }

  virtual SocketAddress GetRemoteAddress() const {
    return remote_addr_;
  }

  
  void SetLocalAddress(const SocketAddress& addr) {
    local_addr_ = addr;
  }

  virtual int Bind(const SocketAddress& addr) {
    if (!local_addr_.IsNil()) {
      error_ = EINVAL;
      return -1;
    }
    local_addr_ = addr;
    int result = server_->Bind(this, &local_addr_);
    if (result != 0) {
      local_addr_.Clear();
      error_ = EADDRINUSE;
    } else {
      bound_ = true;
      was_any_ = addr.IsAnyIP();
    }
    return result;
  }

  virtual int Connect(const SocketAddress& addr) {
    return InitiateConnect(addr, true);
  }

  virtual int Close() {
    if (!local_addr_.IsNil() && bound_) {
      
      server_->Unbind(local_addr_, this);
      bound_ = false;
    }

    if (SOCK_STREAM == type_) {
      
      if (listen_queue_) {
        while (!listen_queue_->empty()) {
          SocketAddress addr = listen_queue_->front();

          
          server_->Disconnect(server_->LookupBinding(addr));
          listen_queue_->pop_front();
        }
        delete listen_queue_;
        listen_queue_ = NULL;
      }
      
      if (CS_CONNECTED == state_) {
        
        VirtualSocket* socket =
            server_->LookupConnection(local_addr_, remote_addr_);
        if (!socket) {
          
          
          
          
          socket = server_->LookupBinding(remote_addr_);
        }
        server_->Disconnect(socket);

        
        server_->RemoveConnection(remote_addr_, local_addr_);
        server_->RemoveConnection(local_addr_, remote_addr_);
      }
      
      MessageList msgs;
      if (server_->msg_queue_) {
        server_->msg_queue_->Clear(this, MSG_ID_CONNECT, &msgs);
      }
      for (MessageList::iterator it = msgs.begin(); it != msgs.end(); ++it) {
        ASSERT(NULL != it->pdata);
        MessageAddress* data = static_cast<MessageAddress*>(it->pdata);

        
        VirtualSocket* socket = server_->LookupConnection(local_addr_,
                                                          data->addr);
        if (socket) {
          
          
          
          server_->Disconnect(socket);
          server_->RemoveConnection(local_addr_, data->addr);
        } else {
          server_->Disconnect(server_->LookupBinding(data->addr));
        }
        delete data;
      }
      
      if (server_->msg_queue_) {
        server_->msg_queue_->Clear(this);
      }
    }

    state_ = CS_CLOSED;
    local_addr_.Clear();
    remote_addr_.Clear();
    return 0;
  }

  virtual int Send(const void *pv, size_t cb) {
    if (CS_CONNECTED != state_) {
      error_ = ENOTCONN;
      return -1;
    }
    if (SOCK_DGRAM == type_) {
      return SendUdp(pv, cb, remote_addr_);
    } else {
      return SendTcp(pv, cb);
    }
  }

  virtual int SendTo(const void *pv, size_t cb, const SocketAddress& addr) {
    if (SOCK_DGRAM == type_) {
      return SendUdp(pv, cb, addr);
    } else {
      if (CS_CONNECTED != state_) {
        error_ = ENOTCONN;
        return -1;
      }
      return SendTcp(pv, cb);
    }
  }

  virtual int Recv(void *pv, size_t cb) {
    SocketAddress addr;
    return RecvFrom(pv, cb, &addr);
  }

  virtual int RecvFrom(void *pv, size_t cb, SocketAddress *paddr) {
    
    if (recv_buffer_.empty()) {
      if (async_) {
        error_ = EAGAIN;
        return -1;
      }
      while (recv_buffer_.empty()) {
        Message msg;
        server_->msg_queue_->Get(&msg);
        server_->msg_queue_->Dispatch(&msg);
      }
    }

    
    Packet* packet = recv_buffer_.front();
    size_t data_read = _min(cb, packet->size());
    memcpy(pv, packet->data(), data_read);
    *paddr = packet->from();

    if (data_read < packet->size()) {
      packet->Consume(data_read);
    } else {
      recv_buffer_.pop_front();
      delete packet;
    }

    if (SOCK_STREAM == type_) {
      bool was_full = (recv_buffer_size_ == server_->recv_buffer_capacity_);
      recv_buffer_size_ -= data_read;
      if (was_full) {
        VirtualSocket* sender = server_->LookupBinding(remote_addr_);
        ASSERT(NULL != sender);
        server_->SendTcp(sender);
      }
    }

    return static_cast<int>(data_read);
  }

  virtual int Listen(int backlog) {
    ASSERT(SOCK_STREAM == type_);
    ASSERT(CS_CLOSED == state_);
    if (local_addr_.IsNil()) {
      error_ = EINVAL;
      return -1;
    }
    ASSERT(NULL == listen_queue_);
    listen_queue_ = new ListenQueue;
    state_ = CS_CONNECTING;
    return 0;
  }

  virtual VirtualSocket* Accept(SocketAddress *paddr) {
    if (NULL == listen_queue_) {
      error_ = EINVAL;
      return NULL;
    }
    while (!listen_queue_->empty()) {
      VirtualSocket* socket = new VirtualSocket(server_, AF_INET, type_,
                                                async_);

      
      socket->SetLocalAddress(local_addr_);
      
      socket->set_was_any(was_any_);
      SocketAddress remote_addr(listen_queue_->front());
      int result = socket->InitiateConnect(remote_addr, false);
      listen_queue_->pop_front();
      if (result != 0) {
        delete socket;
        continue;
      }
      socket->CompleteConnect(remote_addr, false);
      if (paddr) {
        *paddr = remote_addr;
      }
      return socket;
    }
    error_ = EWOULDBLOCK;
    return NULL;
  }

  virtual int GetError() const {
    return error_;
  }

  virtual void SetError(int error) {
    error_ = error;
  }

  virtual ConnState GetState() const {
    return state_;
  }

  virtual int GetOption(Option opt, int* value) {
    OptionsMap::const_iterator it = options_map_.find(opt);
    if (it == options_map_.end()) {
      return -1;
    }
    *value = it->second;
    return 0;  
  }

  virtual int SetOption(Option opt, int value) {
    options_map_[opt] = value;
    return 0;  
  }

  virtual int EstimateMTU(uint16* mtu) {
    if (CS_CONNECTED != state_)
      return ENOTCONN;
    else
      return 65536;
  }

  void OnMessage(Message *pmsg) {
    if (pmsg->message_id == MSG_ID_PACKET) {
      
      ASSERT(NULL != pmsg->pdata);
      Packet* packet = static_cast<Packet*>(pmsg->pdata);

      recv_buffer_.push_back(packet);

      if (async_) {
        SignalReadEvent(this);
      }
    } else if (pmsg->message_id == MSG_ID_CONNECT) {
      ASSERT(NULL != pmsg->pdata);
      MessageAddress* data = static_cast<MessageAddress*>(pmsg->pdata);
      if (listen_queue_ != NULL) {
        listen_queue_->push_back(data->addr);
        if (async_) {
          SignalReadEvent(this);
        }
      } else if ((SOCK_STREAM == type_) && (CS_CONNECTING == state_)) {
        CompleteConnect(data->addr, true);
      } else {
        LOG(LS_VERBOSE) << "Socket at " << local_addr_ << " is not listening";
        server_->Disconnect(server_->LookupBinding(data->addr));
      }
      delete data;
    } else if (pmsg->message_id == MSG_ID_DISCONNECT) {
      ASSERT(SOCK_STREAM == type_);
      if (CS_CLOSED != state_) {
        int error = (CS_CONNECTING == state_) ? ECONNREFUSED : 0;
        state_ = CS_CLOSED;
        remote_addr_.Clear();
        if (async_) {
          SignalCloseEvent(this, error);
        }
      }
    } else {
      ASSERT(false);
    }
  }

  bool was_any() { return was_any_; }
  void set_was_any(bool was_any) { was_any_ = was_any; }

 private:
  struct NetworkEntry {
    size_t size;
    uint32 done_time;
  };

  typedef std::deque<SocketAddress> ListenQueue;
  typedef std::deque<NetworkEntry> NetworkQueue;
  typedef std::vector<char> SendBuffer;
  typedef std::list<Packet*> RecvBuffer;
  typedef std::map<Option, int> OptionsMap;

  int InitiateConnect(const SocketAddress& addr, bool use_delay) {
    if (!remote_addr_.IsNil()) {
      error_ = (CS_CONNECTED == state_) ? EISCONN : EINPROGRESS;
      return -1;
    }
    if (local_addr_.IsNil()) {
      
      int result = 0;
      if (addr.ipaddr().family() == AF_INET) {
        result = Bind(SocketAddress("0.0.0.0", 0));
      } else if (addr.ipaddr().family() == AF_INET6) {
        result = Bind(SocketAddress("::", 0));
      }
      if (result != 0) {
        return result;
      }
    }
    if (type_ == SOCK_DGRAM) {
      remote_addr_ = addr;
      state_ = CS_CONNECTED;
    } else {
      int result = server_->Connect(this, addr, use_delay);
      if (result != 0) {
        error_ = EHOSTUNREACH;
        return -1;
      }
      state_ = CS_CONNECTING;
    }
    return 0;
  }

  void CompleteConnect(const SocketAddress& addr, bool notify) {
    ASSERT(CS_CONNECTING == state_);
    remote_addr_ = addr;
    state_ = CS_CONNECTED;
    server_->AddConnection(remote_addr_, local_addr_, this);
    if (async_ && notify) {
      SignalConnectEvent(this);
    }
  }

  int SendUdp(const void* pv, size_t cb, const SocketAddress& addr) {
    
    if (local_addr_.IsNil()) {
      local_addr_ = EmptySocketAddressWithFamily(addr.ipaddr().family());
      int result = server_->Bind(this, &local_addr_);
      if (result != 0) {
        local_addr_.Clear();
        error_ = EADDRINUSE;
        return result;
      }
    }

    
    return server_->SendUdp(this, static_cast<const char*>(pv), cb, addr);
  }

  int SendTcp(const void* pv, size_t cb) {
    size_t capacity = server_->send_buffer_capacity_ - send_buffer_.size();
    if (0 == capacity) {
      write_enabled_ = true;
      error_ = EWOULDBLOCK;
      return -1;
    }
    size_t consumed = _min(cb, capacity);
    const char* cpv = static_cast<const char*>(pv);
    send_buffer_.insert(send_buffer_.end(), cpv, cpv + consumed);
    server_->SendTcp(this);
    return static_cast<int>(consumed);
  }

  VirtualSocketServer* server_;
  int family_;
  int type_;
  bool async_;
  ConnState state_;
  int error_;
  SocketAddress local_addr_;
  SocketAddress remote_addr_;

  
  ListenQueue* listen_queue_;

  
  SendBuffer send_buffer_;
  bool write_enabled_;

  
  CriticalSection crit_;

  
  NetworkQueue network_;
  size_t network_size_;

  
  RecvBuffer recv_buffer_;
  
  size_t recv_buffer_size_;

  
  bool bound_;

  
  
  
  
  bool was_any_;

  
  OptionsMap options_map_;

  friend class VirtualSocketServer;
};

VirtualSocketServer::VirtualSocketServer(SocketServer* ss)
    : server_(ss), server_owned_(false), msg_queue_(NULL), stop_on_idle_(false),
      network_delay_(Time()), next_ipv4_(kInitialNextIPv4),
      next_ipv6_(kInitialNextIPv6), next_port_(kFirstEphemeralPort),
      bindings_(new AddressMap()), connections_(new ConnectionMap()),
      bandwidth_(0), network_capacity_(kDefaultNetworkCapacity),
      send_buffer_capacity_(kDefaultTcpBufferSize),
      recv_buffer_capacity_(kDefaultTcpBufferSize),
      delay_mean_(0), delay_stddev_(0), delay_samples_(NUM_SAMPLES),
      delay_dist_(NULL), drop_prob_(0.0) {
  if (!server_) {
    server_ = new PhysicalSocketServer();
    server_owned_ = true;
  }
  UpdateDelayDistribution();
}

VirtualSocketServer::~VirtualSocketServer() {
  delete bindings_;
  delete connections_;
  delete delay_dist_;
  if (server_owned_) {
    delete server_;
  }
}

IPAddress VirtualSocketServer::GetNextIP(int family) {
  if (family == AF_INET) {
    IPAddress next_ip(next_ipv4_);
    next_ipv4_.s_addr =
        HostToNetwork32(NetworkToHost32(next_ipv4_.s_addr) + 1);
    return next_ip;
  } else if (family == AF_INET6) {
    IPAddress next_ip(next_ipv6_);
    uint32* as_ints = reinterpret_cast<uint32*>(&next_ipv6_.s6_addr);
    as_ints[3] += 1;
    return next_ip;
  }
  return IPAddress();
}

uint16 VirtualSocketServer::GetNextPort() {
  uint16 port = next_port_;
  if (next_port_ < kLastEphemeralPort) {
    ++next_port_;
  } else {
    next_port_ = kFirstEphemeralPort;
  }
  return port;
}

Socket* VirtualSocketServer::CreateSocket(int type) {
  return CreateSocket(AF_INET, type);
}

Socket* VirtualSocketServer::CreateSocket(int family, int type) {
  return CreateSocketInternal(family, type);
}

AsyncSocket* VirtualSocketServer::CreateAsyncSocket(int type) {
  return CreateAsyncSocket(AF_INET, type);
}

AsyncSocket* VirtualSocketServer::CreateAsyncSocket(int family, int type) {
  return CreateSocketInternal(family, type);
}

VirtualSocket* VirtualSocketServer::CreateSocketInternal(int family, int type) {
  return new VirtualSocket(this, family, type, true);
}

void VirtualSocketServer::SetMessageQueue(MessageQueue* msg_queue) {
  msg_queue_ = msg_queue;
  if (msg_queue_) {
    msg_queue_->SignalQueueDestroyed.connect(this,
        &VirtualSocketServer::OnMessageQueueDestroyed);
  }
}

bool VirtualSocketServer::Wait(int cmsWait, bool process_io) {
  ASSERT(msg_queue_ == Thread::Current());
  if (stop_on_idle_ && Thread::Current()->empty()) {
    return false;
  }
  return socketserver()->Wait(cmsWait, process_io);
}

void VirtualSocketServer::WakeUp() {
  socketserver()->WakeUp();
}

bool VirtualSocketServer::ProcessMessagesUntilIdle() {
  ASSERT(msg_queue_ == Thread::Current());
  stop_on_idle_ = true;
  while (!msg_queue_->empty()) {
    Message msg;
    if (msg_queue_->Get(&msg, kForever)) {
      msg_queue_->Dispatch(&msg);
    }
  }
  stop_on_idle_ = false;
  return !msg_queue_->IsQuitting();
}

void VirtualSocketServer::SetNextPortForTesting(uint16 port) {
  next_port_ = port;
}

int VirtualSocketServer::Bind(VirtualSocket* socket,
                              const SocketAddress& addr) {
  ASSERT(NULL != socket);
  
  ASSERT(!IPIsUnspec(addr.ipaddr()));
  ASSERT(addr.port() != 0);

  
  SocketAddress normalized(addr.ipaddr().Normalized(), addr.port());

  AddressMap::value_type entry(normalized, socket);
  return bindings_->insert(entry).second ? 0 : -1;
}

int VirtualSocketServer::Bind(VirtualSocket* socket, SocketAddress* addr) {
  ASSERT(NULL != socket);

  if (IPIsAny(addr->ipaddr())) {
    addr->SetIP(GetNextIP(addr->ipaddr().family()));
  } else if (!IPIsUnspec(addr->ipaddr())) {
    addr->SetIP(addr->ipaddr().Normalized());
  } else {
    ASSERT(false);
  }

  if (addr->port() == 0) {
    for (int i = 0; i < kEphemeralPortCount; ++i) {
      addr->SetPort(GetNextPort());
      if (bindings_->find(*addr) == bindings_->end()) {
        break;
      }
    }
  }

  return Bind(socket, *addr);
}

VirtualSocket* VirtualSocketServer::LookupBinding(const SocketAddress& addr) {
  SocketAddress normalized(addr.ipaddr().Normalized(),
                           addr.port());
  AddressMap::iterator it = bindings_->find(normalized);
  return (bindings_->end() != it) ? it->second : NULL;
}

int VirtualSocketServer::Unbind(const SocketAddress& addr,
                                VirtualSocket* socket) {
  SocketAddress normalized(addr.ipaddr().Normalized(),
                           addr.port());
  ASSERT((*bindings_)[normalized] == socket);
  bindings_->erase(bindings_->find(normalized));
  return 0;
}

void VirtualSocketServer::AddConnection(const SocketAddress& local,
                                        const SocketAddress& remote,
                                        VirtualSocket* remote_socket) {
  
  
  SocketAddress local_normalized(local.ipaddr().Normalized(),
                                 local.port());
  SocketAddress remote_normalized(remote.ipaddr().Normalized(),
                                  remote.port());
  SocketAddressPair address_pair(local_normalized, remote_normalized);
  connections_->insert(std::pair<SocketAddressPair,
                       VirtualSocket*>(address_pair, remote_socket));
}

VirtualSocket* VirtualSocketServer::LookupConnection(
    const SocketAddress& local,
    const SocketAddress& remote) {
  SocketAddress local_normalized(local.ipaddr().Normalized(),
                                 local.port());
  SocketAddress remote_normalized(remote.ipaddr().Normalized(),
                                  remote.port());
  SocketAddressPair address_pair(local_normalized, remote_normalized);
  ConnectionMap::iterator it = connections_->find(address_pair);
  return (connections_->end() != it) ? it->second : NULL;
}

void VirtualSocketServer::RemoveConnection(const SocketAddress& local,
                                           const SocketAddress& remote) {
  SocketAddress local_normalized(local.ipaddr().Normalized(),
                                local.port());
  SocketAddress remote_normalized(remote.ipaddr().Normalized(),
                                 remote.port());
  SocketAddressPair address_pair(local_normalized, remote_normalized);
  connections_->erase(address_pair);
}

static double Random() {
  return static_cast<double>(rand()) / RAND_MAX;
}

int VirtualSocketServer::Connect(VirtualSocket* socket,
                                 const SocketAddress& remote_addr,
                                 bool use_delay) {
  uint32 delay = use_delay ? GetRandomTransitDelay() : 0;
  VirtualSocket* remote = LookupBinding(remote_addr);
  if (!CanInteractWith(socket, remote)) {
    LOG(LS_INFO) << "Address family mismatch between "
                 << socket->GetLocalAddress() << " and " << remote_addr;
    return -1;
  }
  if (remote != NULL) {
    SocketAddress addr = socket->GetLocalAddress();
    msg_queue_->PostDelayed(delay, remote, MSG_ID_CONNECT,
                            new MessageAddress(addr));
  } else {
    LOG(LS_INFO) << "No one listening at " << remote_addr;
    msg_queue_->PostDelayed(delay, socket, MSG_ID_DISCONNECT);
  }
  return 0;
}

bool VirtualSocketServer::Disconnect(VirtualSocket* socket) {
  if (socket) {
    
    msg_queue_->Post(socket, MSG_ID_DISCONNECT);
    return true;
  }
  return false;
}

int VirtualSocketServer::SendUdp(VirtualSocket* socket,
                                 const char* data, size_t data_size,
                                 const SocketAddress& remote_addr) {
  
  if (Random() < drop_prob_) {
    LOG(LS_VERBOSE) << "Dropping packet: bad luck";
    return static_cast<int>(data_size);
  }

  VirtualSocket* recipient = LookupBinding(remote_addr);
  if (!recipient) {
    
    scoped_ptr<VirtualSocket> dummy_socket(
        CreateSocketInternal(AF_INET, SOCK_DGRAM));
    dummy_socket->SetLocalAddress(remote_addr);
    if (!CanInteractWith(socket, dummy_socket.get())) {
      LOG(LS_VERBOSE) << "Incompatible address families: "
                      << socket->GetLocalAddress() << " and " << remote_addr;
      return -1;
    }
    LOG(LS_VERBOSE) << "No one listening at " << remote_addr;
    return static_cast<int>(data_size);
  }

  if (!CanInteractWith(socket, recipient)) {
    LOG(LS_VERBOSE) << "Incompatible address families: "
                    << socket->GetLocalAddress() << " and " << remote_addr;
    return -1;
  }

  CritScope cs(&socket->crit_);

  uint32 cur_time = Time();
  PurgeNetworkPackets(socket, cur_time);

  
  
  
  
  
  
  

  size_t packet_size = data_size + UDP_HEADER_SIZE;
  if (socket->network_size_ + packet_size > network_capacity_) {
    LOG(LS_VERBOSE) << "Dropping packet: network capacity exceeded";
    return static_cast<int>(data_size);
  }

  AddPacketToNetwork(socket, recipient, cur_time, data, data_size,
                     UDP_HEADER_SIZE, false);

  return static_cast<int>(data_size);
}

void VirtualSocketServer::SendTcp(VirtualSocket* socket) {
  
  
  
  
  

  
  VirtualSocket* recipient = LookupConnection(socket->local_addr_,
                                              socket->remote_addr_);
  if (!recipient) {
    LOG(LS_VERBOSE) << "Sending data to no one.";
    return;
  }

  CritScope cs(&socket->crit_);

  uint32 cur_time = Time();
  PurgeNetworkPackets(socket, cur_time);

  while (true) {
    size_t available = recv_buffer_capacity_ - recipient->recv_buffer_size_;
    size_t max_data_size = _min<size_t>(available, TCP_MSS - TCP_HEADER_SIZE);
    size_t data_size = _min(socket->send_buffer_.size(), max_data_size);
    if (0 == data_size)
      break;

    AddPacketToNetwork(socket, recipient, cur_time, &socket->send_buffer_[0],
                       data_size, TCP_HEADER_SIZE, true);
    recipient->recv_buffer_size_ += data_size;

    size_t new_buffer_size = socket->send_buffer_.size() - data_size;
    
    
    if (data_size < socket->send_buffer_.size()) {
      
      memmove(&socket->send_buffer_[0], &socket->send_buffer_[data_size],
              new_buffer_size);
    }
    socket->send_buffer_.resize(new_buffer_size);
  }

  if (socket->write_enabled_
      && (socket->send_buffer_.size() < send_buffer_capacity_)) {
    socket->write_enabled_ = false;
    socket->SignalWriteEvent(socket);
  }
}

void VirtualSocketServer::AddPacketToNetwork(VirtualSocket* sender,
                                             VirtualSocket* recipient,
                                             uint32 cur_time,
                                             const char* data,
                                             size_t data_size,
                                             size_t header_size,
                                             bool ordered) {
  VirtualSocket::NetworkEntry entry;
  entry.size = data_size + header_size;

  sender->network_size_ += entry.size;
  uint32 send_delay = SendDelay(static_cast<uint32>(sender->network_size_));
  entry.done_time = cur_time + send_delay;
  sender->network_.push_back(entry);

  
  uint32 transit_delay = GetRandomTransitDelay();

  
  Packet* p = new Packet(data, data_size, sender->local_addr_);
  uint32 ts = TimeAfter(send_delay + transit_delay);
  if (ordered) {
    
    
    
    ts = TimeMax(ts, network_delay_);
  }
  msg_queue_->PostAt(ts, recipient, MSG_ID_PACKET, p);
  network_delay_ = TimeMax(ts, network_delay_);
}

void VirtualSocketServer::PurgeNetworkPackets(VirtualSocket* socket,
                                              uint32 cur_time) {
  while (!socket->network_.empty() &&
         (socket->network_.front().done_time <= cur_time)) {
    ASSERT(socket->network_size_ >= socket->network_.front().size);
    socket->network_size_ -= socket->network_.front().size;
    socket->network_.pop_front();
  }
}

uint32 VirtualSocketServer::SendDelay(uint32 size) {
  if (bandwidth_ == 0)
    return 0;
  else
    return 1000 * size / bandwidth_;
}

#if 0
void PrintFunction(std::vector<std::pair<double, double> >* f) {
  return;
  double sum = 0;
  for (uint32 i = 0; i < f->size(); ++i) {
    std::cout << (*f)[i].first << '\t' << (*f)[i].second << std::endl;
    sum += (*f)[i].second;
  }
  if (!f->empty()) {
    const double mean = sum / f->size();
    double sum_sq_dev = 0;
    for (uint32 i = 0; i < f->size(); ++i) {
      double dev = (*f)[i].second - mean;
      sum_sq_dev += dev * dev;
    }
    std::cout << "Mean = " << mean << " StdDev = "
              << sqrt(sum_sq_dev / f->size()) << std::endl;
  }
}
#endif  

void VirtualSocketServer::UpdateDelayDistribution() {
  Function* dist = CreateDistribution(delay_mean_, delay_stddev_,
                                      delay_samples_);
  
  {
    CritScope cs(&delay_crit_);
    delete delay_dist_;
    delay_dist_ = dist;
  }
}

static double PI = 4 * atan(1.0);

static double Normal(double x, double mean, double stddev) {
  double a = (x - mean) * (x - mean) / (2 * stddev * stddev);
  return exp(-a) / (stddev * sqrt(2 * PI));
}

#if 0  
static double Pareto(double x, double min, double k) {
  if (x < min)
    return 0;
  else
    return k * std::pow(min, k) / std::pow(x, k+1);
}
#endif

VirtualSocketServer::Function* VirtualSocketServer::CreateDistribution(
    uint32 mean, uint32 stddev, uint32 samples) {
  Function* f = new Function();

  if (0 == stddev) {
    f->push_back(Point(mean, 1.0));
  } else {
    double start = 0;
    if (mean >= 4 * static_cast<double>(stddev))
      start = mean - 4 * static_cast<double>(stddev);
    double end = mean + 4 * static_cast<double>(stddev);

    for (uint32 i = 0; i < samples; i++) {
      double x = start + (end - start) * i / (samples - 1);
      double y = Normal(x, mean, stddev);
      f->push_back(Point(x, y));
    }
  }
  return Resample(Invert(Accumulate(f)), 0, 1, samples);
}

uint32 VirtualSocketServer::GetRandomTransitDelay() {
  size_t index = rand() % delay_dist_->size();
  double delay = (*delay_dist_)[index].second;
  
  return static_cast<uint32>(delay);
}

struct FunctionDomainCmp {
  bool operator()(const VirtualSocketServer::Point& p1,
                   const VirtualSocketServer::Point& p2) {
    return p1.first < p2.first;
  }
  bool operator()(double v1, const VirtualSocketServer::Point& p2) {
    return v1 < p2.first;
  }
  bool operator()(const VirtualSocketServer::Point& p1, double v2) {
    return p1.first < v2;
  }
};

VirtualSocketServer::Function* VirtualSocketServer::Accumulate(Function* f) {
  ASSERT(f->size() >= 1);
  double v = 0;
  for (Function::size_type i = 0; i < f->size() - 1; ++i) {
    double dx = (*f)[i + 1].first - (*f)[i].first;
    double avgy = ((*f)[i + 1].second + (*f)[i].second) / 2;
    (*f)[i].second = v;
    v = v + dx * avgy;
  }
  (*f)[f->size()-1].second = v;
  return f;
}

VirtualSocketServer::Function* VirtualSocketServer::Invert(Function* f) {
  for (Function::size_type i = 0; i < f->size(); ++i)
    std::swap((*f)[i].first, (*f)[i].second);

  std::sort(f->begin(), f->end(), FunctionDomainCmp());
  return f;
}

VirtualSocketServer::Function* VirtualSocketServer::Resample(
    Function* f, double x1, double x2, uint32 samples) {
  Function* g = new Function();

  for (size_t i = 0; i < samples; i++) {
    double x = x1 + (x2 - x1) * i / (samples - 1);
    double y = Evaluate(f, x);
    g->push_back(Point(x, y));
  }

  delete f;
  return g;
}

double VirtualSocketServer::Evaluate(Function* f, double x) {
  Function::iterator iter =
      std::lower_bound(f->begin(), f->end(), x, FunctionDomainCmp());
  if (iter == f->begin()) {
    return (*f)[0].second;
  } else if (iter == f->end()) {
    ASSERT(f->size() >= 1);
    return (*f)[f->size() - 1].second;
  } else if (iter->first == x) {
    return iter->second;
  } else {
    double x1 = (iter - 1)->first;
    double y1 = (iter - 1)->second;
    double x2 = iter->first;
    double y2 = iter->second;
    return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
  }
}

bool VirtualSocketServer::CanInteractWith(VirtualSocket* local,
                                          VirtualSocket* remote) {
  if (!local || !remote) {
    return false;
  }
  IPAddress local_ip = local->GetLocalAddress().ipaddr();
  IPAddress remote_ip = remote->GetLocalAddress().ipaddr();
  IPAddress local_normalized = local_ip.Normalized();
  IPAddress remote_normalized = remote_ip.Normalized();
  
  
  
  if (local_normalized.family() == remote_normalized.family()) {
    return true;
  }

  
  int remote_v6_only = 0;
  remote->GetOption(Socket::OPT_IPV6_V6ONLY, &remote_v6_only);
  if (local_ip.family() == AF_INET && !remote_v6_only && IPIsAny(remote_ip)) {
    return true;
  }
  
  int local_v6_only = 0;
  local->GetOption(Socket::OPT_IPV6_V6ONLY, &local_v6_only);
  if (remote_ip.family() == AF_INET && !local_v6_only && IPIsAny(local_ip)) {
    return true;
  }

  
  
  if (local_ip.family() == AF_INET6 && local->was_any()) {
    return true;
  }
  if (remote_ip.family() == AF_INET6 && remote->was_any()) {
    return true;
  }

  return false;
}

}  
