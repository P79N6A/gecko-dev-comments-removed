





#ifndef tls_connect_h_
#define tls_connect_h_

#include <tuple>

#include "sslt.h"

#include "tls_agent.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"

namespace nss_test {


class TlsConnectTestBase : public ::testing::Test {
 public:
  static ::testing::internal::ParamGenerator<std::string> kTlsModesStream;
  static ::testing::internal::ParamGenerator<std::string> kTlsModesAll;
  static ::testing::internal::ParamGenerator<uint16_t> kTlsV10;
  static ::testing::internal::ParamGenerator<uint16_t> kTlsV11V12;
  static ::testing::internal::ParamGenerator<uint16_t> kTlsV12Plus;

  static inline Mode ToMode(const std::string& str) {
    return str == "TLS" ? STREAM : DGRAM;
  }

  TlsConnectTestBase(Mode mode, uint16_t version);
  virtual ~TlsConnectTestBase();

  void SetUp();
  void TearDown();

  
  void Init();
  
  void ResetRsa();
  
  
  void ResetEcdsa();
  
  void EnsureTlsSetup();

  
  void Handshake();
  
  void Connect();
  
  void ConnectExpectFail();

  void EnableSomeEcdheCiphers();
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
  uint16_t version_;
  std::vector<std::vector<uint8_t>> session_ids_;

 private:
  void Reset(const std::string& server_name, SSLKEAType kea);
};


class TlsConnectStream : public TlsConnectTestBase,
                         public ::testing::WithParamInterface<uint16_t> {
 public:
  TlsConnectStream() : TlsConnectTestBase(STREAM, GetParam()) {}
};


class TlsConnectDatagram : public TlsConnectTestBase,
                           public ::testing::WithParamInterface<uint16_t> {
 public:
  TlsConnectDatagram() : TlsConnectTestBase(DGRAM, GetParam()) {}
};




class TlsConnectGeneric
  : public TlsConnectTestBase,
    public ::testing::WithParamInterface<std::tuple<std::string, uint16_t>> {
 public:
  TlsConnectGeneric();
};

} 

#endif
