







extern "C" {
#include "stun_msg.h" 
#include "stun_util.h"
#include "nr_api.h"
#include "async_wait.h"
#include "nr_socket.h"
#include "nr_socket_local.h"
#include "stun_hint.h"
#include "local_addr.h"
#include "registry.h"
}

#include "test_nr_socket.h"

#include "nsCOMPtr.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "nsAutoPtr.h"
#include "runnable_utils.h"
#include "mtransport_test_utils.h"

#include <vector>

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"
#include "gtest_utils.h"

namespace mozilla {

class TestNrSocketTest : public ::testing::Test {
 public:
  TestNrSocketTest() :
    wait_done_for_main_(false),
    sts_(),
    public_addrs_(),
    private_addrs_(),
    nats_() {
    
    nsresult rv;
    sts_ = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
    EXPECT_TRUE(NS_SUCCEEDED(rv)) << "Failed to get STS: " << (int)rv;
  }

  ~TestNrSocketTest() {
    sts_->Dispatch(WrapRunnable(this, &TestNrSocketTest::TearDown_s),
                   NS_DISPATCH_SYNC);
  }

  void TearDown_s() {
    public_addrs_.clear();
    private_addrs_.clear();
    nats_.clear();
    sts_ = nullptr;
  }

  nsRefPtr<TestNrSocket> CreateTestNrSocket_s(const char *ip_str,
                                              TestNat *nat) {
    
    
    nsRefPtr<TestNrSocket> sock(new TestNrSocket(nat ? nat : new TestNat));
    nr_transport_addr address;
    nr_ip4_str_port_to_transport_addr(ip_str, 0, IPPROTO_UDP, &address);
    int r = sock->create(&address);
    if (r) {
      return nullptr;
    }
    return sock;
  }

  void CreatePublicAddrs(size_t count, const char *ip_str = "127.0.0.1") {
    sts_->Dispatch(
        WrapRunnable(this,
                     &TestNrSocketTest::CreatePublicAddrs_s,
                     count,
                     ip_str),
        NS_DISPATCH_SYNC);
  }

  void CreatePublicAddrs_s(size_t count, const char* ip_str) {
    while (count--) {
      auto sock = CreateTestNrSocket_s(ip_str, nullptr);
      ASSERT_TRUE(sock) << "Failed to create socket";
      public_addrs_.push_back(sock);
    }
  }

  nsRefPtr<TestNat> CreatePrivateAddrs(size_t size,
                                       const char* ip_str = "127.0.0.1") {
    nsRefPtr<TestNat> result;
    sts_->Dispatch(
        WrapRunnableRet(&result,
                        this,
                        &TestNrSocketTest::CreatePrivateAddrs_s,
                        size,
                        ip_str),
        NS_DISPATCH_SYNC);
    return result;
  }

  nsRefPtr<TestNat> CreatePrivateAddrs_s(size_t count, const char* ip_str) {
    nsRefPtr<TestNat> nat(new TestNat);
    while (count--) {
      auto sock = CreateTestNrSocket_s(ip_str, nat);
      if (!sock) {
        EXPECT_TRUE(false) << "Failed to create socket";
        break;
      }
      private_addrs_.push_back(sock);
    }
    nat->enabled_ = true;
    nats_.push_back(nat);
    return nat;
  }

  bool CheckConnectivityVia(
      TestNrSocket *from,
      TestNrSocket *to,
      const nr_transport_addr &via,
      nr_transport_addr *sender_external_address = nullptr) {
    MOZ_ASSERT(from);

    if (!WaitForWriteable(from)) {
      return false;
    }

    int result = 0;
    sts_->Dispatch(WrapRunnableRet(&result,
                                   this,
                                   &TestNrSocketTest::SendData_s,
                                   from,
                                   via),
                   NS_DISPATCH_SYNC);
    if (result) {
      return false;
    }

    if (!WaitForReadable(to)) {
      return false;
    }

    nr_transport_addr dummy_outparam;
    if (!sender_external_address) {
      sender_external_address = &dummy_outparam;
    }

    MOZ_ASSERT(to);
    sts_->Dispatch(WrapRunnableRet(&result,
                                   this,
                                   &TestNrSocketTest::RecvData_s,
                                   to,
                                   sender_external_address),
                   NS_DISPATCH_SYNC);

    return !result;
  }

  bool CheckConnectivity(
      TestNrSocket *from,
      TestNrSocket *to,
      nr_transport_addr *sender_external_address = nullptr) {
    nr_transport_addr destination_address;
    int r = GetAddress(to, &destination_address);
    if (r) {
      return false;
    }

    return CheckConnectivityVia(from,
                                to,
                                destination_address,
                                sender_external_address);
  }

  int GetAddress(TestNrSocket *sock, nr_transport_addr_ *address) {
    MOZ_ASSERT(sock);
    MOZ_ASSERT(address);
    int r;
    sts_->Dispatch(WrapRunnableRet(&r,
                                   this,
                                   &TestNrSocketTest::GetAddress_s,
                                   sock,
                                   address),
                   NS_DISPATCH_SYNC);
    return r;
  }

  int GetAddress_s(TestNrSocket *sock, nr_transport_addr *address) {
    return sock->getaddr(address);
  }

  int SendData_s(TestNrSocket *from, const nr_transport_addr &to) {
    
    const char buf[] = "foobajooba";
    return from->sendto(buf, sizeof(buf), 0,
        
        const_cast<nr_transport_addr*>(&to));
  }

  int RecvData_s(TestNrSocket *to, nr_transport_addr *from) {
    
    const size_t bufSize = 1024;
    char buf[bufSize];
    size_t len;
    
    int r = to->recvfrom(buf, sizeof(buf), &len, 0, from);
    if (!r && (len == 0)) {
      r = R_INTERNAL;
    }
    return r;
  }

  bool WaitForSocketState(TestNrSocket *sock, int state) {
    MOZ_ASSERT(sock);
    sts_->Dispatch(WrapRunnable(this,
                                &TestNrSocketTest::WaitForSocketState_s,
                                sock,
                                state),
                   NS_DISPATCH_SYNC);

    bool res;
    WAIT_(wait_done_for_main_, 100, res);
    wait_done_for_main_ = false;

    if (!res) {
      sts_->Dispatch(WrapRunnable(this,
                                  &TestNrSocketTest::CancelWait_s,
                                  sock,
                                  state),
                     NS_DISPATCH_SYNC);
    }

    return res;
  }

  void WaitForSocketState_s(TestNrSocket *sock, int state) {
     NR_ASYNC_WAIT(sock, state, &WaitDone, this);
  }

  void CancelWait_s(TestNrSocket *sock, int state) {
     sock->cancel(state);
  }

  bool WaitForReadable(TestNrSocket *sock) {
    return WaitForSocketState(sock, NR_ASYNC_WAIT_READ);
  }

  bool WaitForWriteable(TestNrSocket *sock) {
    return WaitForSocketState(sock, NR_ASYNC_WAIT_WRITE);
  }

  static void WaitDone(void *sock, int how, void *test_fixture) {
    TestNrSocketTest *test = static_cast<TestNrSocketTest*>(test_fixture);
    test->wait_done_for_main_ = true;
  }

  
  Atomic<bool> wait_done_for_main_;

  nsCOMPtr<nsIEventTarget> sts_;
  std::vector<nsRefPtr<TestNrSocket>> public_addrs_;
  std::vector<nsRefPtr<TestNrSocket>> private_addrs_;
  std::vector<nsRefPtr<TestNat>> nats_;
};

} 

using mozilla::TestNrSocketTest;
using mozilla::TestNat;

TEST_F(TestNrSocketTest, PublicConnectivity) {
  CreatePublicAddrs(2);

  ASSERT_TRUE(CheckConnectivity(public_addrs_[0], public_addrs_[1]));
  ASSERT_TRUE(CheckConnectivity(public_addrs_[1], public_addrs_[0]));
  ASSERT_TRUE(CheckConnectivity(public_addrs_[0], public_addrs_[0]));
  ASSERT_TRUE(CheckConnectivity(public_addrs_[1], public_addrs_[1]));
}

TEST_F(TestNrSocketTest, PrivateConnectivity) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(2));
  nat->filtering_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat->mapping_type_ = TestNat::ENDPOINT_INDEPENDENT;

  ASSERT_TRUE(CheckConnectivity(private_addrs_[0], private_addrs_[1]));
  ASSERT_TRUE(CheckConnectivity(private_addrs_[1], private_addrs_[0]));
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0], private_addrs_[0]));
  ASSERT_TRUE(CheckConnectivity(private_addrs_[1], private_addrs_[1]));
}

TEST_F(TestNrSocketTest, NoConnectivityWithoutPinhole) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(1));
  nat->filtering_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat->mapping_type_ = TestNat::ENDPOINT_INDEPENDENT;
  CreatePublicAddrs(1);

  ASSERT_FALSE(CheckConnectivity(public_addrs_[0], private_addrs_[0]));
}

TEST_F(TestNrSocketTest, NoConnectivityBetweenSubnets) {
  nsRefPtr<TestNat> nat1(CreatePrivateAddrs(1));
  nat1->filtering_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat1->mapping_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nsRefPtr<TestNat> nat2(CreatePrivateAddrs(1));
  nat2->filtering_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat2->mapping_type_ = TestNat::ENDPOINT_INDEPENDENT;

  ASSERT_FALSE(CheckConnectivity(private_addrs_[0], private_addrs_[1]));
  ASSERT_FALSE(CheckConnectivity(private_addrs_[1], private_addrs_[0]));
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0], private_addrs_[0]));
  ASSERT_TRUE(CheckConnectivity(private_addrs_[1], private_addrs_[1]));
}

TEST_F(TestNrSocketTest, FullConeAcceptIngress) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(1));
  nat->filtering_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat->mapping_type_ = TestNat::ENDPOINT_INDEPENDENT;
  CreatePublicAddrs(2);

  nr_transport_addr sender_external_address;
  
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[0],
                                &sender_external_address));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[0],
                                   private_addrs_[0],
                                   sender_external_address));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[1],
                                   private_addrs_[0],
                                   sender_external_address));
}

TEST_F(TestNrSocketTest, FullConeOnePinhole) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(1));
  nat->filtering_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat->mapping_type_ = TestNat::ENDPOINT_INDEPENDENT;
  CreatePublicAddrs(2);

  nr_transport_addr sender_external_address;
  
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[0],
                                &sender_external_address));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[0],
                                   private_addrs_[0],
                                   sender_external_address));

  
  nr_transport_addr sender_external_address2;
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[1],
                                &sender_external_address2));
  ASSERT_FALSE(nr_transport_addr_cmp(&sender_external_address,
                                     &sender_external_address2,
                                     NR_TRANSPORT_ADDR_CMP_MODE_ALL))
    << "addr1: " << sender_external_address.as_string << " addr2: "
    << sender_external_address2.as_string;
}




TEST_F(TestNrSocketTest, DISABLED_AddressRestrictedCone) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(1));
  nat->filtering_type_ = TestNat::ADDRESS_DEPENDENT;
  nat->mapping_type_ = TestNat::ENDPOINT_INDEPENDENT;
  CreatePublicAddrs(2, "127.0.0.1");
  CreatePublicAddrs(1, "127.0.0.2");

  nr_transport_addr sender_external_address;
  
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[0],
                                &sender_external_address));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[0],
                                   private_addrs_[0],
                                   sender_external_address));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[1],
                                   private_addrs_[0],
                                   sender_external_address));

  
  
  
  
  
#ifndef __linux__
  
  ASSERT_FALSE(CheckConnectivityVia(public_addrs_[2],
                                   private_addrs_[0],
                                   sender_external_address));
#endif

  
  nr_transport_addr sender_external_address2;
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[1],
                                &sender_external_address2));
  ASSERT_FALSE(nr_transport_addr_cmp(&sender_external_address,
                                     &sender_external_address2,
                                     NR_TRANSPORT_ADDR_CMP_MODE_ALL))
    << "addr1: " << sender_external_address.as_string << " addr2: "
    << sender_external_address2.as_string;

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[1],
                                   private_addrs_[0],
                                   sender_external_address2));

  
  nr_transport_addr sender_external_address3;
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[2],
                                &sender_external_address3));
  ASSERT_FALSE(nr_transport_addr_cmp(&sender_external_address,
                                     &sender_external_address3,
                                     NR_TRANSPORT_ADDR_CMP_MODE_ALL))
    << "addr1: " << sender_external_address.as_string << " addr2: "
    << sender_external_address3.as_string;

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[2],
                                   private_addrs_[0],
                                   sender_external_address3));
}

TEST_F(TestNrSocketTest, RestrictedCone) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(1));
  nat->filtering_type_ = TestNat::PORT_DEPENDENT;
  nat->mapping_type_ = TestNat::ENDPOINT_INDEPENDENT;
  CreatePublicAddrs(2);

  nr_transport_addr sender_external_address;
  
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[0],
                                &sender_external_address));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[0],
                                   private_addrs_[0],
                                   sender_external_address));

  
  ASSERT_FALSE(CheckConnectivityVia(public_addrs_[1],
                                    private_addrs_[0],
                                    sender_external_address));

  
  nr_transport_addr sender_external_address2;
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[1],
                                &sender_external_address2));
  ASSERT_FALSE(nr_transport_addr_cmp(&sender_external_address,
                                     &sender_external_address2,
                                     NR_TRANSPORT_ADDR_CMP_MODE_ALL))
    << "addr1: " << sender_external_address.as_string << " addr2: "
    << sender_external_address2.as_string;

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[1],
                                   private_addrs_[0],
                                   sender_external_address2));
}

TEST_F(TestNrSocketTest, PortDependentMappingFullCone) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(1));
  nat->filtering_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat->mapping_type_ = TestNat::PORT_DEPENDENT;
  CreatePublicAddrs(2);

  nr_transport_addr sender_external_address0;
  
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[0],
                                &sender_external_address0));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[0],
                                   private_addrs_[0],
                                   sender_external_address0));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[1],
                                   private_addrs_[0],
                                   sender_external_address0));

  
  nr_transport_addr sender_external_address1;
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[1],
                                &sender_external_address1));
  ASSERT_TRUE(nr_transport_addr_cmp(&sender_external_address0,
                                    &sender_external_address1,
                                    NR_TRANSPORT_ADDR_CMP_MODE_ALL))
    << "addr1: " << sender_external_address0.as_string << " addr2: "
    << sender_external_address1.as_string;

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[1],
                                   private_addrs_[0],
                                   sender_external_address1));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[0],
                                   private_addrs_[0],
                                   sender_external_address1));
}

TEST_F(TestNrSocketTest, Symmetric) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(1));
  nat->filtering_type_ = TestNat::PORT_DEPENDENT;
  nat->mapping_type_ = TestNat::PORT_DEPENDENT;
  CreatePublicAddrs(2);

  nr_transport_addr sender_external_address;
  
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[0],
                                &sender_external_address));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[0],
                                   private_addrs_[0],
                                   sender_external_address));

  
  ASSERT_FALSE(CheckConnectivityVia(public_addrs_[1],
                                    private_addrs_[0],
                                    sender_external_address));

  
  nr_transport_addr sender_external_address2;
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[1],
                                &sender_external_address2));
  ASSERT_TRUE(nr_transport_addr_cmp(&sender_external_address,
                                    &sender_external_address2,
                                    NR_TRANSPORT_ADDR_CMP_MODE_ALL));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[1],
                                   private_addrs_[0],
                                   sender_external_address2));
}

TEST_F(TestNrSocketTest, BlockUdp) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(2));
  nat->block_udp_ = true;
  CreatePublicAddrs(1);

  nr_transport_addr sender_external_address;
  ASSERT_FALSE(CheckConnectivity(private_addrs_[0],
                                 public_addrs_[0],
                                 &sender_external_address));

  
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                private_addrs_[1]));
  ASSERT_TRUE(CheckConnectivity(private_addrs_[1],
                                private_addrs_[0]));
}

TEST_F(TestNrSocketTest, DenyHairpinning) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(2));
  nat->filtering_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat->mapping_type_ = TestNat::ENDPOINT_INDEPENDENT;
  CreatePublicAddrs(1);

  nr_transport_addr sender_external_address;
  
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[0],
                                &sender_external_address));

  
  ASSERT_FALSE(CheckConnectivityVia(private_addrs_[1],
                                    private_addrs_[0],
                                    sender_external_address));
}

TEST_F(TestNrSocketTest, AllowHairpinning) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(2));
  nat->filtering_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat->mapping_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat->mapping_timeout_ = 30000;
  nat->allow_hairpinning_ = true;
  CreatePublicAddrs(1);

  nr_transport_addr sender_external_address;
  
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[0],
                                &sender_external_address));

  
  ASSERT_TRUE(CheckConnectivityVia(private_addrs_[1],
                                   private_addrs_[0],
                                   sender_external_address));
}

TEST_F(TestNrSocketTest, FullConeTimeout) {
  nsRefPtr<TestNat> nat(CreatePrivateAddrs(1));
  nat->filtering_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat->mapping_type_ = TestNat::ENDPOINT_INDEPENDENT;
  nat->mapping_timeout_ = 200;
  CreatePublicAddrs(2);

  nr_transport_addr sender_external_address;
  
  ASSERT_TRUE(CheckConnectivity(private_addrs_[0],
                                public_addrs_[0],
                                &sender_external_address));

  
  ASSERT_TRUE(CheckConnectivityVia(public_addrs_[0],
                                   private_addrs_[0],
                                   sender_external_address));

  PR_Sleep(201);

  
  ASSERT_FALSE(CheckConnectivityVia(public_addrs_[0],
                                    private_addrs_[0],
                                    sender_external_address));
}




int main(int argc, char **argv)
{
  
  MtransportTestUtils test_utils;

  NR_reg_init(NR_REG_MODE_LOCAL);

  
  ::testing::InitGoogleTest(&argc, argv);

  int rv = RUN_ALL_TESTS();

  return rv;
}
