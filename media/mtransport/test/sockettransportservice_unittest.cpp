






#include <iostream>

#include "prio.h"

#include "nsCOMPtr.h"
#include "nsNetCID.h"
#include "nsXPCOM.h"
#include "nsXPCOMGlue.h"

#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsISocketTransportService.h"

#include "nsASocketHandler.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"

#include "mtransport_test_utils.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"
#include "gtest_utils.h"

MtransportTestUtils *test_utils;

namespace {
class SocketTransportServiceTest : public ::testing::Test {
 public:
  SocketTransportServiceTest() : received_(0),
                                 readpipe_(nullptr),
                                 writepipe_(nullptr),
                                 registered_(false) {
  }

  ~SocketTransportServiceTest() {
    if (readpipe_)
      PR_Close(readpipe_);
    if (writepipe_)
      PR_Close(writepipe_);
  }

  void SetUp();
  void RegisterHandler();
  void SendEvent();
  void SendPacket();

  void ReceivePacket() {
    ++received_;
  }

  void ReceiveEvent() {
    ++received_;
  }

  size_t Received() {
    return received_;
  }

 private:
  nsCOMPtr<nsISocketTransportService> stservice_;
  nsCOMPtr<nsIEventTarget> target_;
  size_t received_;
  PRFileDesc *readpipe_;
  PRFileDesc *writepipe_;
  bool registered_;
};



class EventReceived : public nsRunnable {
public:
  explicit EventReceived(SocketTransportServiceTest *test) :
      test_(test) {}

  NS_IMETHOD Run() {
    test_->ReceiveEvent();
    return NS_OK;
  }

  SocketTransportServiceTest *test_;
};



class RegisterEvent : public nsRunnable {
public:
  explicit RegisterEvent(SocketTransportServiceTest *test) :
      test_(test) {}

  NS_IMETHOD Run() {
    test_->RegisterHandler();
    return NS_OK;
  }

  SocketTransportServiceTest *test_;
};


class SocketHandler : public nsASocketHandler {
 public:
  explicit SocketHandler(SocketTransportServiceTest *test) : test_(test) {
  }

  void OnSocketReady(PRFileDesc *fd, int16_t outflags) override {
    unsigned char buf[1600];

    int32_t rv;
    rv = PR_Recv(fd, buf, sizeof(buf), 0, PR_INTERVAL_NO_WAIT);
    if (rv > 0) {
      std::cerr << "Read " << rv << " bytes" << std::endl;
      test_->ReceivePacket();
    }
  }

  void OnSocketDetached(PRFileDesc *fd) override {}

  void IsLocal(bool *aIsLocal) override {
    
    *aIsLocal = false;
  }

  virtual uint64_t ByteCountSent() override { return 0; }
  virtual uint64_t ByteCountReceived() override { return 0; }

  NS_DECL_ISUPPORTS

 protected:
  virtual ~SocketHandler() {}

 private:
  SocketTransportServiceTest *test_;
};

NS_IMPL_ISUPPORTS0(SocketHandler)

void SocketTransportServiceTest::SetUp() {
  
  nsresult rv;
  target_ = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  
  stservice_ = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  
  PRStatus status = PR_CreatePipe(&readpipe_, &writepipe_);
  ASSERT_EQ(status, PR_SUCCESS);

  
  
  
  rv = target_->Dispatch(new RegisterEvent(this), 0);
  ASSERT_TRUE(NS_SUCCEEDED(rv));
  ASSERT_TRUE_WAIT(registered_, 10000);

}

void SocketTransportServiceTest::RegisterHandler() {
  nsresult rv;

  rv = stservice_->AttachSocket(readpipe_, new SocketHandler(this));
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  registered_ = true;
}

void SocketTransportServiceTest::SendEvent() {
  nsresult rv;

  rv = target_->Dispatch(new EventReceived(this), 0);
  ASSERT_TRUE(NS_SUCCEEDED(rv));
  ASSERT_TRUE_WAIT(Received() == 1, 10000);
}

void SocketTransportServiceTest::SendPacket() {
  unsigned char buffer[1024];
  memset(buffer, 0, sizeof(buffer));

  int32_t status = PR_Write(writepipe_, buffer, sizeof(buffer));
  uint32_t size = status & 0xffff;
  ASSERT_EQ(sizeof(buffer), size);
}




TEST_F(SocketTransportServiceTest, SendEvent) {
  SendEvent();
}

TEST_F(SocketTransportServiceTest, SendPacket) {
  SendPacket();
}


}  


int main(int argc, char **argv) {
  test_utils = new MtransportTestUtils();

  
  ::testing::InitGoogleTest(&argc, argv);

  int rv = RUN_ALL_TESTS();
  delete test_utils;
  return rv;
}
