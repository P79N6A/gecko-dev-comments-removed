








































#include <stdlib.h>
#include <iostream>

#include "sigslot.h"

#include "logging.h"
#include "nspr.h"
#include "nss.h"
#include "ssl.h"

#include "mozilla/Scoped.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"

#include "mtransport_test_utils.h"
#include "runnable_utils.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"
#include "gtest_utils.h"

#define USE_TURN


extern "C" {
#include "nr_api.h"
#include "registry.h"
#include "async_timer.h"
#include "r_crc32.h"
#include "ice_util.h"
#include "transport_addr.h"
#include "nr_crypto.h"
#include "nr_socket.h"
#include "nr_socket_local.h"
#include "nr_socket_buffered_stun.h"
#include "stun_client_ctx.h"
#include "turn_client_ctx.h"
}

#include "nricemediastream.h"
#include "nricectx.h"


MtransportTestUtils *test_utils;

using namespace mozilla;

std::string g_turn_server;
std::string g_turn_user;
std::string g_turn_password;

std::string kDummyTurnServer("192.0.2.1");  

class TurnClient : public ::testing::Test {
 public:
  TurnClient()
      : turn_server_(g_turn_server),
        real_socket_(nullptr),
        net_socket_(nullptr),
        buffered_socket_(nullptr),
        net_fd_(nullptr),
        turn_ctx_(nullptr),
        allocated_(false),
        received_(0),
        protocol_(IPPROTO_UDP) {
  }

  ~TurnClient() {
  }

  void SetTcp() {
    protocol_ = IPPROTO_TCP;
  }

  void Init_s() {
    int r;
    nr_transport_addr addr;
    r = nr_ip4_port_to_transport_addr(0, 0, protocol_, &addr);
    ASSERT_EQ(0, r);

    r = nr_socket_local_create(nullptr, &addr, &real_socket_);
    ASSERT_EQ(0, r);

    if (protocol_ == IPPROTO_TCP) {
      int r =
          nr_socket_buffered_stun_create(real_socket_, 100000, TURN_TCP_FRAMING,
                                         &buffered_socket_);
      ASSERT_EQ(0, r);
      net_socket_ = buffered_socket_;
    } else {
      net_socket_ = real_socket_;
    }

    r = nr_str_port_to_transport_addr(turn_server_.c_str(), 3478,
      protocol_, &addr);
    ASSERT_EQ(0, r);

    std::vector<unsigned char> password_vec(
        g_turn_password.begin(), g_turn_password.end());
    Data password;
    INIT_DATA(password, &password_vec[0], password_vec.size());
    r = nr_turn_client_ctx_create("test", net_socket_,
                                  g_turn_user.c_str(),
                                  &password,
                                  &addr, &turn_ctx_);
    ASSERT_EQ(0, r);

    r = nr_socket_getfd(net_socket_, &net_fd_);
    ASSERT_EQ(0, r);

    NR_ASYNC_WAIT(net_fd_, NR_ASYNC_WAIT_READ, socket_readable_cb,
        (void *)this);
  }

  void TearDown_s() {
    nr_turn_client_ctx_destroy(&turn_ctx_);
    if (net_fd_) {
      NR_ASYNC_CANCEL(net_fd_, NR_ASYNC_WAIT_READ);
    }

    nr_socket_destroy(&buffered_socket_);
  }

  void TearDown() {
    RUN_ON_THREAD(test_utils->sts_target(),
                  WrapRunnable(this, &TurnClient::TearDown_s),
                  NS_DISPATCH_SYNC);
  }

  void Allocate_s() {
    Init_s();
    ASSERT_TRUE(turn_ctx_);

    int r = nr_turn_client_allocate(turn_ctx_,
                                    allocate_success_cb,
                                    this);
    ASSERT_EQ(0, r);
  }

  void Allocate(bool expect_success=true) {
    RUN_ON_THREAD(test_utils->sts_target(),
                  WrapRunnable(this, &TurnClient::Allocate_s),
                  NS_DISPATCH_SYNC);

    if (expect_success) {
      ASSERT_TRUE_WAIT(allocated_, 5000);
    }
    else {
      PR_Sleep(10000);
      ASSERT_FALSE(allocated_);
    }
  }

  void Allocated() {
    if (turn_ctx_->state!=NR_TURN_CLIENT_STATE_ALLOCATED) {
      std::cerr << "Allocation failed" << std::endl;
      return;
    }
    allocated_ = true;

    int r;
    nr_transport_addr addr;

    r = nr_turn_client_get_relayed_address(turn_ctx_, &addr);
    ASSERT_EQ(0, r);

    relay_addr_ = addr.as_string;

    std::cerr << "Allocation succeeded with addr=" << relay_addr_ << std::endl;
  }

  void Deallocate_s() {
    ASSERT_TRUE(turn_ctx_);

    std::cerr << "De-Allocating..." << std::endl;
    int r = nr_turn_client_deallocate(turn_ctx_);
    ASSERT_EQ(0, r);
  }

  void Deallocate() {
    RUN_ON_THREAD(test_utils->sts_target(),
                  WrapRunnable(this, &TurnClient::Deallocate_s),
                  NS_DISPATCH_SYNC);
  }

  void RequestPermission_s(const std::string& target) {
    nr_transport_addr addr;
    int r;

    
    ASSERT_EQ(0, target.compare(0, 4, "IP4:"));

    size_t offset = target.rfind(':');
    ASSERT_NE(std::string::npos, offset);

    std::string host = target.substr(4, offset - 4);
    std::string port = target.substr(offset + 1);

    r = nr_str_port_to_transport_addr(host.c_str(),
                                      atoi(port.c_str()),
                                      IPPROTO_UDP,
                                      &addr);
    ASSERT_EQ(0, r);

    r = nr_turn_client_ensure_perm(turn_ctx_, &addr);
    ASSERT_EQ(0, r);
  }

  void RequestPermission(const std::string& target) {
    RUN_ON_THREAD(test_utils->sts_target(),
                  WrapRunnable(this, &TurnClient::RequestPermission_s, target),
                  NS_DISPATCH_SYNC);

  }

  void Readable(NR_SOCKET s, int how, void *arg) {
    
    std::cerr << "Socket is readable" << std::endl;
    NR_ASYNC_WAIT(s, how, socket_readable_cb, arg);

    UCHAR buf[8192];
    size_t len_s;
    nr_transport_addr addr;

    int r = nr_socket_recvfrom(net_socket_, buf, sizeof(buf), &len_s, 0, &addr);
    if (r) {
      std::cerr << "Error reading from socket" << std::endl;
      return;
    }

    ASSERT_LT(len_s, (size_t)INT_MAX);
    int len = (int)len_s;

    if (nr_is_stun_response_message(buf, len)) {
      std::cerr << "STUN response" << std::endl;
      r = nr_turn_client_process_response(turn_ctx_, buf, len, &addr);

      if (r && r != R_REJECTED && r != R_RETRY) {
        std::cerr << "Error processing STUN: " << r << std::endl;
      }
    } else if (nr_is_stun_indication_message(buf, len)) {
      std::cerr << "STUN indication" << std::endl;

      
      unsigned char data[NR_STUN_MAX_MESSAGE_SIZE];
      size_t datal;
      nr_transport_addr remote_addr;

      r = nr_turn_client_parse_data_indication(turn_ctx_, &addr,
                                               buf, len,
                                               data, &datal, sizeof(data),
                                               &remote_addr);
      ASSERT_EQ(0, r);
      std::cerr << "Received " << datal << " bytes from "
                << remote_addr.as_string << std::endl;

      received_ += datal;

      for (size_t i=0; i < datal; i++) {
        ASSERT_EQ(i & 0xff, data[i]);
      }
    }
    else {
      if (nr_is_stun_message(buf, len)) {
        std::cerr << "STUN message of unexpected type" << std::endl;
      } else {
        std::cerr << "Not a STUN message" << std::endl;
      }
      return;
    }
  }

  void SendTo_s(const std::string& target, int expect_return) {
    nr_transport_addr addr;
    int r;

    
    ASSERT_EQ(0, target.compare(0, 4, "IP4:"));

    size_t offset = target.rfind(':');
    ASSERT_NE(std::string::npos, offset);

    std::string host = target.substr(4, offset - 4);
    std::string port = target.substr(offset + 1);

    r = nr_str_port_to_transport_addr(host.c_str(),
                                      atoi(port.c_str()),
                                      IPPROTO_UDP,
                                      &addr);
    ASSERT_EQ(0, r);

    unsigned char test[100];
    for (size_t i=0; i<sizeof(test); i++) {
      test[i] = i & 0xff;
    }

    std::cerr << "Sending test message to " << target << " ..." << std::endl;

    r = nr_turn_client_send_indication(turn_ctx_,
                                            test, sizeof(test), 0,
                                            &addr);
    if (expect_return >= 0) {
      ASSERT_EQ(expect_return, r);
    }
  }

  void SendTo(const std::string& target, int expect_return=0) {
    RUN_ON_THREAD(test_utils->sts_target(),
                  WrapRunnable(this, &TurnClient::SendTo_s, target,
                               expect_return),
                  NS_DISPATCH_SYNC);
  }

  int received() const { return received_; }

  static void socket_readable_cb(NR_SOCKET s, int how, void *arg) {
    static_cast<TurnClient *>(arg)->Readable(s, how, arg);
  }

  static void allocate_success_cb(NR_SOCKET s, int how, void *arg){
    static_cast<TurnClient *>(arg)->Allocated();
  }

 protected:
  std::string turn_server_;
  nr_socket *real_socket_;
  nr_socket *net_socket_;
  nr_socket *buffered_socket_;
  NR_SOCKET net_fd_;
  nr_turn_client_ctx *turn_ctx_;
  std::string relay_addr_;
  bool allocated_;
  int received_;
  int protocol_;
};

TEST_F(TurnClient, Allocate) {
  Allocate();
}

TEST_F(TurnClient, AllocateTcp) {
  SetTcp();
  Allocate();
}

TEST_F(TurnClient, AllocateAndHold) {
  Allocate();
  PR_Sleep(20000);
  ASSERT_TRUE(turn_ctx_->state == NR_TURN_CLIENT_STATE_ALLOCATED);
}

TEST_F(TurnClient, SendToSelf) {
  Allocate();
  SendTo(relay_addr_);
  ASSERT_TRUE_WAIT(received() == 100, 5000);
  SendTo(relay_addr_);
  ASSERT_TRUE_WAIT(received() == 200, 1000);
}


TEST_F(TurnClient, SendToSelfTcp) {
  SetTcp();
  Allocate();
  SendTo(relay_addr_);
  ASSERT_TRUE_WAIT(received() == 100, 5000);
  SendTo(relay_addr_);
  ASSERT_TRUE_WAIT(received() == 200, 1000);
}

TEST_F(TurnClient, PermissionDenied) {
  Allocate();
  RequestPermission(relay_addr_);
  PR_Sleep(1000);

  
  nr_turn_permission *perm;
  perm = STAILQ_FIRST(&turn_ctx_->permissions);
  ASSERT_TRUE(perm);
  while (perm) {
    perm->stun->last_error_code = 403;
    std::cerr << "Set 403's on permission" << std::endl;
    perm = STAILQ_NEXT(perm, entry);
  }

  SendTo(relay_addr_, R_NOT_PERMITTED);
  ASSERT_TRUE(received() == 0);

  
  
  
}

TEST_F(TurnClient, DeallocateReceiveFailure) {
  Allocate();
  SendTo(relay_addr_);
  ASSERT_TRUE_WAIT(received() == 100, 5000);
  Deallocate();
  turn_ctx_->state = NR_TURN_CLIENT_STATE_ALLOCATED;
  SendTo(relay_addr_);
  PR_Sleep(1000);
  ASSERT_TRUE(received() == 100);
}

TEST_F(TurnClient, DeallocateReceiveFailureTcp) {
  SetTcp();
  Allocate();
  SendTo(relay_addr_);
  ASSERT_TRUE_WAIT(received() == 100, 5000);
  Deallocate();
  turn_ctx_->state = NR_TURN_CLIENT_STATE_ALLOCATED;
  



  SendTo(relay_addr_, -1);
  PR_Sleep(1000);
  ASSERT_TRUE(received() == 100);
}

TEST_F(TurnClient, AllocateDummyServer) {
  turn_server_ = kDummyTurnServer;
  Allocate(false);
}

static std::string get_environment(const char *name) {
  char *value = getenv(name);

  if (!value)
    return "";

  return value;
}

int main(int argc, char **argv)
{
  g_turn_server = get_environment("TURN_SERVER_ADDRESS");
  g_turn_user = get_environment("TURN_SERVER_USER");
  g_turn_password = get_environment("TURN_SERVER_PASSWORD");

  if (g_turn_server.empty() ||
      g_turn_user.empty(),
      g_turn_password.empty()) {
    printf(
        "Set TURN_SERVER_ADDRESS, TURN_SERVER_USER, and TURN_SERVER_PASSWORD\n"
        "environment variables to run this test\n");
    return 0;
  }
  {
    nr_transport_addr addr;
    if (nr_str_port_to_transport_addr(g_turn_server.c_str(), 3478,
                                      IPPROTO_UDP, &addr)) {
      printf("Invalid TURN_SERVER_ADDRESS \"%s\". Only IP numbers supported.\n",
             g_turn_server.c_str());
      return 0;
    }
  }
  test_utils = new MtransportTestUtils();
  NSS_NoDB_Init(nullptr);
  NSS_SetDomesticPolicy();

  
  
  std::string dummy("dummy");
  RUN_ON_THREAD(test_utils->sts_target(),
                WrapRunnableNM(&NrIceCtx::Create,
                               dummy, false, false, false, false, false),
                NS_DISPATCH_SYNC);

  
  ::testing::InitGoogleTest(&argc, argv);

  int rv = RUN_ALL_TESTS();
  delete test_utils;
  return rv;
}
