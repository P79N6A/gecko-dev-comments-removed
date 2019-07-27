





#ifndef tls_connect_h_
#define tls_connect_h_

#include "sslt.h"

#include "tls_agent.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"

namespace nss_test {


class TlsConnectTestBase : public ::testing::Test {
 public:
  TlsConnectTestBase(Mode mode);
  virtual ~TlsConnectTestBase();

  void SetUp();
  void TearDown();

  
  void Init();
  
  void Reset();
  
  void EnsureTlsSetup();

  
  void Handshake();
  
  void Connect();
  
  void ConnectExpectFail();

  void EnableSomeECDHECiphers();
  void ConfigureSessionCache(SessionResumptionMode client,
                             SessionResumptionMode server);
  void CheckResumption(SessionResumptionMode expected);
  void EnableAlpn();
  void EnableSrtp();
  void CheckSrtp();

 protected:
  Mode mode_;
  TlsAgent* client_;
  TlsAgent* server_;
  std::vector<std::vector<uint8_t>> session_ids_;
};


class TlsConnectTest : public TlsConnectTestBase {
 public:
  TlsConnectTest() : TlsConnectTestBase(STREAM) {}
};


class DtlsConnectTest : public TlsConnectTestBase {
 public:
  DtlsConnectTest() : TlsConnectTestBase(DGRAM) {}
};



class TlsConnectGeneric : public TlsConnectTestBase,
                          public ::testing::WithParamInterface<std::string> {
 public:
  TlsConnectGeneric();
};

} 

#endif
