









#ifndef WEBRTC_BASE_FIREWALLSOCKETSERVER_H_
#define WEBRTC_BASE_FIREWALLSOCKETSERVER_H_

#include <vector>
#include "webrtc/base/socketserver.h"
#include "webrtc/base/criticalsection.h"

namespace rtc {

class FirewallManager;



enum FirewallProtocol { FP_UDP, FP_TCP, FP_ANY };
enum FirewallDirection { FD_IN, FD_OUT, FD_ANY };

class FirewallSocketServer : public SocketServer {
 public:
  FirewallSocketServer(SocketServer * server,
                       FirewallManager * manager = NULL,
                       bool should_delete_server = false);
  virtual ~FirewallSocketServer();

  SocketServer* socketserver() const { return server_; }
  void set_socketserver(SocketServer* server) {
    if (server_ && should_delete_server_) {
      delete server_;
      server_ = NULL;
      should_delete_server_ = false;
    }
    server_ = server;
  }

  
  void set_udp_sockets_enabled(bool enabled) { udp_sockets_enabled_ = enabled; }
  void set_tcp_sockets_enabled(bool enabled) { tcp_sockets_enabled_ = enabled; }
  bool tcp_listen_enabled() const { return tcp_listen_enabled_; }
  void set_tcp_listen_enabled(bool enabled) { tcp_listen_enabled_ = enabled; }

  
  void AddRule(bool allow, FirewallProtocol p = FP_ANY,
               FirewallDirection d = FD_ANY,
               const SocketAddress& addr = SocketAddress());
  void AddRule(bool allow, FirewallProtocol p,
               const SocketAddress& src, const SocketAddress& dst);
  void ClearRules();

  bool Check(FirewallProtocol p,
             const SocketAddress& src, const SocketAddress& dst);

  virtual Socket* CreateSocket(int type);
  virtual Socket* CreateSocket(int family, int type);

  virtual AsyncSocket* CreateAsyncSocket(int type);
  virtual AsyncSocket* CreateAsyncSocket(int family, int type);

  virtual void SetMessageQueue(MessageQueue* queue) {
    server_->SetMessageQueue(queue);
  }
  virtual bool Wait(int cms, bool process_io) {
    return server_->Wait(cms, process_io);
  }
  virtual void WakeUp() {
    return server_->WakeUp();
  }

  Socket * WrapSocket(Socket * sock, int type);
  AsyncSocket * WrapSocket(AsyncSocket * sock, int type);

 private:
  SocketServer * server_;
  FirewallManager * manager_;
  CriticalSection crit_;
  struct Rule {
    bool allow;
    FirewallProtocol p;
    FirewallDirection d;
    SocketAddress src;
    SocketAddress dst;
  };
  std::vector<Rule> rules_;
  bool should_delete_server_;
  bool udp_sockets_enabled_;
  bool tcp_sockets_enabled_;
  bool tcp_listen_enabled_;
};



class FirewallManager {
 public:
  FirewallManager();
  ~FirewallManager();

  void AddServer(FirewallSocketServer * server);
  void RemoveServer(FirewallSocketServer * server);

  void AddRule(bool allow, FirewallProtocol p = FP_ANY,
               FirewallDirection d = FD_ANY,
               const SocketAddress& addr = SocketAddress());
  void ClearRules();

 private:
  CriticalSection crit_;
  std::vector<FirewallSocketServer *> servers_;
};

}  

#endif  
