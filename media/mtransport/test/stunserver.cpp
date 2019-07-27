














































































#include "logging.h"
#include "mozilla/Scoped.h"
#include "databuffer.h"

extern "C" {
#include "nr_api.h"
#include "async_wait.h"
#include "async_timer.h"
#include "nr_socket.h"
#include "nr_socket_local.h"
#include "transport_addr.h"
#include "addrs.h"
#include "local_addr.h"
#include "stun_util.h"
#include "registry.h"
#include "nr_socket_multi_tcp.h"
}

#include "stunserver.h"

#include <string>

MOZ_MTLOG_MODULE("stunserver");

namespace mozilla {



struct nr_socket_wrapped {
  nr_socket *sock_;
  nr_transport_addr addr_;
};

static int nr_socket_wrapped_destroy(void **objp) {
  if (!objp || !*objp)
    return 0;

  nr_socket_wrapped *wrapped = static_cast<nr_socket_wrapped *>(*objp);
  *objp = 0;

  delete wrapped;

  return 0;
}

static int nr_socket_wrapped_sendto(void *obj, const void *msg, size_t len, int flags,
    nr_transport_addr *addr) {
  nr_socket_wrapped *wrapped = static_cast<nr_socket_wrapped *>(obj);

  return nr_socket_sendto(wrapped->sock_, msg, len, flags, &wrapped->addr_);
}

static int nr_socket_wrapped_recvfrom(void *obj, void * restrict buf, size_t maxlen,
    size_t *len, int flags, nr_transport_addr *addr) {
  MOZ_CRASH();
}

static int nr_socket_wrapped_getfd(void *obj, NR_SOCKET *fd) {
  MOZ_CRASH();
}

static int nr_socket_wrapped_getaddr(void *obj, nr_transport_addr *addrp) {
  nr_socket_wrapped *wrapped = static_cast<nr_socket_wrapped *>(obj);

  return nr_socket_getaddr(wrapped->sock_, addrp);
}

static int nr_socket_wrapped_close(void *obj) {
  MOZ_CRASH();
}

static int nr_socket_wrapped_set_send_addr(nr_socket *sock, nr_transport_addr *addr) {
  nr_socket_wrapped *wrapped = static_cast<nr_socket_wrapped *>(sock->obj);

  return nr_transport_addr_copy(&wrapped->addr_, addr);
}

static nr_socket_vtbl nr_socket_wrapped_vtbl = {
  2,
  nr_socket_wrapped_destroy,
  nr_socket_wrapped_sendto,
  nr_socket_wrapped_recvfrom,
  nr_socket_wrapped_getfd,
  nr_socket_wrapped_getaddr,
  0,
  0,
  0,
  nr_socket_wrapped_close,
  0,
  0
};

int nr_socket_wrapped_create(nr_socket *inner, nr_socket **outp) {
  ScopedDeletePtr<nr_socket_wrapped> wrapped(new nr_socket_wrapped());

  wrapped->sock_ = inner;

  int r = nr_socket_create_int(wrapped.get(), &nr_socket_wrapped_vtbl, outp);
  if (r)
    return r;

  wrapped.forget();
  return 0;
}





TestStunServer* TestStunServer::instance;
TestStunTcpServer* TestStunTcpServer::instance;
uint16_t TestStunServer::instance_port = 3478;
uint16_t TestStunTcpServer::instance_port = 3478;

TestStunServer::~TestStunServer() {
  

  
  if (listen_sock_) {
    NR_SOCKET fd;
    if (!nr_socket_getfd(listen_sock_, &fd)) {
      NR_ASYNC_CANCEL(fd, NR_ASYNC_WAIT_READ);
    }
  }

  
  nr_stun_server_ctx_destroy(&stun_server_);
  nr_socket_destroy(&listen_sock_);
  nr_socket_destroy(&send_sock_);

  
  if (timer_handle_)
    NR_async_timer_cancel(timer_handle_);

  delete response_addr_;
}

int TestStunServer::SetInternalPort(nr_local_addr* addr, uint16_t port) {
  if (nr_transport_addr_set_port(&addr->addr, port)) {
    MOZ_MTLOG(ML_ERROR, "Couldn't set port");
    return R_INTERNAL;
  }

  if (nr_transport_addr_fmt_addr_string(&addr->addr)) {
    MOZ_MTLOG(ML_ERROR, "Couldn't re-set addr string");
    return R_INTERNAL;
  }

  return 0;
}

int TestStunServer::TryOpenListenSocket(nr_local_addr* addr, uint16_t port) {

  int r = SetInternalPort(addr, port);

  if (r)
    return r;

  if (nr_socket_local_create(nullptr, &addr->addr, &listen_sock_)) {
    MOZ_MTLOG(ML_ERROR, "Couldn't create listen socket");
    return R_ALREADY;
  }

  return 0;
}

int TestStunServer::Initialize() {
  nr_local_addr addrs[100];
  int addr_ct;
  int r;

  r = nr_stun_find_local_addresses(addrs, 100, &addr_ct);
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't retrieve addresses");
    return R_INTERNAL;
  }

  if (addr_ct < 1) {
    MOZ_MTLOG(ML_ERROR, "No local addresses");
    return R_INTERNAL;
  }

  int tries = 100;
  while (tries--) {
    
    r = TryOpenListenSocket(&addrs[0], instance_port);
    
    
    if (r != R_ALREADY) {
      break;
    }
    ++instance_port;
  }

  if (r) {
    return R_INTERNAL;
  }

  r = nr_socket_wrapped_create(listen_sock_, &send_sock_);
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't create send socket");
    return R_INTERNAL;
  }

  r = nr_stun_server_ctx_create(const_cast<char *>("Test STUN server"),
                                send_sock_,
                                &stun_server_);
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't create STUN server");
    return R_INTERNAL;
  }

  
  char addr_string[INET6_ADDRSTRLEN];
  r = nr_transport_addr_get_addrstring(&addrs[0].addr, addr_string,
                                       sizeof(addr_string));
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Failed to convert listen addr to a string representation");
    return R_INTERNAL;
  }

  listen_addr_ = addr_string;
  listen_port_ = instance_port;

  return 0;
}

TestStunServer* TestStunServer::Create() {
  NR_reg_init(NR_REG_MODE_LOCAL);

  ScopedDeletePtr<TestStunServer> server(new TestStunServer());

  if (server->Initialize())
    return nullptr;

  NR_SOCKET fd;
  int r = nr_socket_getfd(server->listen_sock_, &fd);
  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't get fd");
    return nullptr;
  }

  NR_ASYNC_WAIT(fd, NR_ASYNC_WAIT_READ, &TestStunServer::readable_cb, server.get());

  return server.forget();
}

void TestStunServer::ConfigurePort(uint16_t port) {
  instance_port = port;
}

TestStunServer* TestStunServer::GetInstance() {
  if (!instance)
    instance = Create();

  MOZ_ASSERT(instance);
  return instance;
}

void TestStunServer::ShutdownInstance() {
  delete instance;

  instance = nullptr;
}


struct DeferredStunOperation {
  DeferredStunOperation(TestStunServer *server,
                        const char *data, size_t len,
                        nr_transport_addr *addr) :
      server_(server),
      buffer_(reinterpret_cast<const uint8_t *>(data), len) {
    nr_transport_addr_copy(&addr_, addr);
  }

  TestStunServer *server_;
  DataBuffer buffer_;
  nr_transport_addr addr_;
};

void TestStunServer::Process(const uint8_t *msg, size_t len, nr_transport_addr *addr) {
  
  nr_socket_wrapped_set_send_addr(send_sock_, addr);
  nr_stun_server_process_request(stun_server_, send_sock_,
                                 const_cast<char *>(reinterpret_cast<const char *>(msg)),
                                 len,
                                 response_addr_ ?
                                 response_addr_ : addr,
                                 NR_STUN_AUTH_RULE_OPTIONAL);
}

void TestStunServer::process_cb(NR_SOCKET s, int how, void *cb_arg) {
  DeferredStunOperation *op = static_cast<DeferredStunOperation *>(cb_arg);
  op->server_->timer_handle_ = nullptr;
  op->server_->Process(op->buffer_.data(), op->buffer_.len(), &op->addr_);

  delete op;
}

void TestStunServer::readable_cb(NR_SOCKET s, int how, void *cb_arg) {
  TestStunServer* server = static_cast<TestStunServer*>(cb_arg);

  char message[4096];
  size_t message_len;
  nr_transport_addr addr;

  int r = nr_socket_recvfrom(server->listen_sock_, message, sizeof(message),
    &message_len, 0, &addr);

  if (r) {
    MOZ_MTLOG(ML_ERROR, "Couldn't read STUN message");
    return;
  }

  MOZ_MTLOG(ML_DEBUG, "Received data of length " << message_len);

  
  NR_ASYNC_WAIT(s, NR_ASYNC_WAIT_READ, &TestStunServer::readable_cb, server);


  
  std::string key(addr.as_string);

  if (server->received_ct_.count(key) == 0) {
    server->received_ct_[key] = 0;
  }

  ++server->received_ct_[key];

  if (!server->active_ || (server->received_ct_[key] <= server->initial_ct_)) {
    MOZ_MTLOG(ML_DEBUG, "Dropping message #"
              << server->received_ct_[key] << " from " << key);
    return;
  }

  if (server->delay_ms_) {
    NR_ASYNC_TIMER_SET(server->delay_ms_,
                       process_cb,
                       new DeferredStunOperation(
                           server,
                           message, message_len,
                           &addr),
                       &server->timer_handle_);
  } else {
    server->Process(reinterpret_cast<const uint8_t *>(message), message_len, &addr);
  }
}

void TestStunServer::SetActive(bool active) {
  active_ = active;
}

void TestStunServer::SetDelay(uint32_t delay_ms) {
  delay_ms_ = delay_ms;
}

void TestStunServer::SetDropInitialPackets(uint32_t count) {
  initial_ct_ = count;
}

nsresult TestStunServer::SetResponseAddr(nr_transport_addr *addr) {
  delete response_addr_;

  response_addr_ = new nr_transport_addr();

  int r = nr_transport_addr_copy(response_addr_, addr);
  if (r)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult TestStunServer::SetResponseAddr(const std::string& addr,
                                         uint16_t port) {
  nr_transport_addr addr2;

  int r = nr_ip4_str_port_to_transport_addr(addr.c_str(),
                                            port, IPPROTO_UDP,
                                            &addr2);
  if (r)
    return NS_ERROR_FAILURE;

  return SetResponseAddr(&addr2);
}

void TestStunServer::Reset() {
  delay_ms_ = 0;
  if (timer_handle_) {
    NR_async_timer_cancel(timer_handle_);
    timer_handle_ = nullptr;
  }
  delete response_addr_;
  response_addr_ = nullptr;
}




void TestStunTcpServer::ConfigurePort(uint16_t port) {
  instance_port = port;
}

TestStunTcpServer* TestStunTcpServer::GetInstance() {
  if (!instance)
    instance = Create();

  MOZ_ASSERT(instance);
  return instance;
}

void TestStunTcpServer::ShutdownInstance() {
  delete instance;

  instance = nullptr;
}

int TestStunTcpServer::TryOpenListenSocket(nr_local_addr* addr, uint16_t port) {

  addr->addr.protocol=IPPROTO_TCP;

  int r = SetInternalPort(addr, port);

  if (r)
    return r;

  if (ice_ctx_ == NULL)
    ice_ctx_ = NrIceCtx::Create("stun", true);

  
  
  
  if(nr_socket_multi_tcp_create(ice_ctx_->ctx(),
     &addr->addr, TCP_TYPE_PASSIVE, 0, 0, 2048,
     &listen_sock_)) {
     MOZ_MTLOG(ML_ERROR, "Couldn't create listen socket");
     return R_ALREADY;
  }

  if(nr_socket_listen(listen_sock_, 10)) {
    MOZ_MTLOG(ML_ERROR, "Couldn't listen on socket");
    return R_ALREADY;
  }

  return 0;
}

TestStunTcpServer* TestStunTcpServer::Create() {
  NR_reg_init(NR_REG_MODE_LOCAL);

  ScopedDeletePtr<TestStunTcpServer> server(new TestStunTcpServer());

  server->Initialize();

  nr_socket_multi_tcp_set_readable_cb(server->listen_sock_,
    &TestStunServer::readable_cb, server.get());

  return server.forget();
}

TestStunTcpServer::~TestStunTcpServer() {
  ice_ctx_ = nullptr;
  nr_socket_destroy(&listen_sock_);
}

}  
