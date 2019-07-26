







#ifndef stunserver_h__
#define stunserver_h__

#include <map>
#include <string>
#include "prio.h"
#include "nsError.h"

typedef struct nr_stun_server_ctx_ nr_stun_server_ctx;
typedef struct nr_socket_ nr_socket;
typedef struct nr_local_addr_ nr_local_addr;

namespace mozilla {

class TestStunServer {
 public:
  
  
  static TestStunServer *GetInstance();
  static void ShutdownInstance();
  
  
  static void ConfigurePort(uint16_t port);
  static TestStunServer *Create();

  ~TestStunServer();

  void SetActive(bool active);
  void SetDelay(uint32_t delay_ms);
  void SetDropInitialPackets(uint32_t count);
  const std::string& addr() const { return listen_addr_; }
  uint16_t port() const { return listen_port_; }

  
  
  nsresult SetResponseAddr(nr_transport_addr *addr);
  nsresult SetResponseAddr(const std::string& addr, uint16_t port);

  void Reset();

 private:
  TestStunServer()
      : listen_sock_(nullptr),
        send_sock_(nullptr),
        stun_server_(nullptr),
        active_(true),
        delay_ms_(0),
        initial_ct_(0),
        response_addr_(nullptr),
        timer_handle_(nullptr),
        listen_port_(0) {}

  void Process(const uint8_t *msg, size_t len, nr_transport_addr *addr_in);
  int TryOpenListenSocket(nr_local_addr* addr, uint16_t port);

  static void readable_cb(NR_SOCKET sock, int how, void *cb_arg);
  static void process_cb(NR_SOCKET sock, int how, void *cb_arg);

  nr_socket *listen_sock_;
  nr_socket *send_sock_;
  nr_stun_server_ctx *stun_server_;
  bool active_;
  uint32_t delay_ms_;
  uint32_t initial_ct_;
  nr_transport_addr *response_addr_;
  void *timer_handle_;
  std::map<std::string, uint32_t> received_ct_;
  std::string listen_addr_;
  uint16_t listen_port_;

  static TestStunServer* instance;
  static uint16_t instance_port;
};

}  

#endif
