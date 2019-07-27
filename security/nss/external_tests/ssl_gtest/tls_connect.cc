





#include "tls_connect.h"

#include <iostream>

#include "sslproto.h"
#include "gtest_utils.h"

extern std::string g_working_dir_path;

namespace nss_test {

static const std::string kTlsModesStreamArr[] = {"TLS"};
::testing::internal::ParamGenerator<std::string>
  TlsConnectTestBase::kTlsModesStream = ::testing::ValuesIn(kTlsModesStreamArr);
static const std::string kTlsModesAllArr[] = {"TLS", "DTLS"};
::testing::internal::ParamGenerator<std::string>
  TlsConnectTestBase::kTlsModesAll = ::testing::ValuesIn(kTlsModesAllArr);
static const uint16_t kTlsV10Arr[] = {SSL_LIBRARY_VERSION_TLS_1_0};
::testing::internal::ParamGenerator<uint16_t>
  TlsConnectTestBase::kTlsV10 = ::testing::ValuesIn(kTlsV10Arr);
static const uint16_t kTlsV11V12Arr[] = {SSL_LIBRARY_VERSION_TLS_1_1,
                                         SSL_LIBRARY_VERSION_TLS_1_2};
::testing::internal::ParamGenerator<uint16_t>
  TlsConnectTestBase::kTlsV11V12 = ::testing::ValuesIn(kTlsV11V12Arr);

static const uint16_t kTlsV12PlusArr[] = {SSL_LIBRARY_VERSION_TLS_1_2};
::testing::internal::ParamGenerator<uint16_t>
  TlsConnectTestBase::kTlsV12Plus = ::testing::ValuesIn(kTlsV12PlusArr);

static std::string VersionString(uint16_t version) {
  switch(version) {
  case 0:
    return "(no version)";
  case SSL_LIBRARY_VERSION_TLS_1_0:
    return "1.0";
  case SSL_LIBRARY_VERSION_TLS_1_1:
    return "1.1";
  case SSL_LIBRARY_VERSION_TLS_1_2:
    return "1.2";
  default:
    std::cerr << "Invalid version: " << version << std::endl;
    EXPECT_TRUE(false);
    return "";
  }
}

TlsConnectTestBase::TlsConnectTestBase(Mode mode, uint16_t version)
      : mode_(mode),
        client_(new TlsAgent("client", TlsAgent::CLIENT, mode_)),
        server_(new TlsAgent("server", TlsAgent::SERVER, mode_)),
        version_(version),
        session_ids_() {
  std::cerr << "Version: " << mode_ << " " << VersionString(version_) << std::endl;
}

TlsConnectTestBase::~TlsConnectTestBase() {
  delete client_;
  delete server_;
}

void TlsConnectTestBase::SetUp() {
  
  SSL_ConfigServerSessionIDCache(1024, 0, 0, g_working_dir_path.c_str());

  
  SSL3Statistics* stats = SSL_GetStatistics();
  memset(stats, 0, sizeof(*stats));

  Init();
}

void TlsConnectTestBase::TearDown() {
  client_ = nullptr;
  server_ = nullptr;

  SSL_ClearSessionCache();
  SSL_ShutdownServerSessionIDCache();
}

void TlsConnectTestBase::Init() {
  ASSERT_TRUE(client_->Init());
  ASSERT_TRUE(server_->Init());

  client_->SetPeer(server_);
  server_->SetPeer(client_);

  if (version_) {
    client_->SetVersionRange(version_, version_);
    server_->SetVersionRange(version_, version_);
  }
}

void TlsConnectTestBase::Reset() {
  delete client_;
  delete server_;

  client_ = new TlsAgent("client", TlsAgent::CLIENT, mode_);
  server_ = new TlsAgent("server", TlsAgent::SERVER, mode_);

  Init();
}

void TlsConnectTestBase::EnsureTlsSetup() {
  ASSERT_TRUE(client_->EnsureTlsSetup());
  ASSERT_TRUE(server_->EnsureTlsSetup());
}

void TlsConnectTestBase::Handshake() {
  server_->StartConnect();
  client_->StartConnect();
  client_->Handshake();
  server_->Handshake();

  ASSERT_TRUE_WAIT((client_->state() != TlsAgent::CONNECTING) &&
                   (server_->state() != TlsAgent::CONNECTING),
                   5000);

}

void TlsConnectTestBase::Connect() {
  Handshake();

  
  ASSERT_EQ(client_->version(), server_->version());
  ASSERT_EQ(std::min(client_->max_version(),
                     server_->max_version()),
            client_->version());

  ASSERT_EQ(TlsAgent::CONNECTED, client_->state());
  ASSERT_EQ(TlsAgent::CONNECTED, server_->state());

  int16_t cipher_suite1, cipher_suite2;
  bool ret = client_->cipher_suite(&cipher_suite1);
  ASSERT_TRUE(ret);
  ret = server_->cipher_suite(&cipher_suite2);
  ASSERT_TRUE(ret);
  ASSERT_EQ(cipher_suite1, cipher_suite2);

  std::cerr << "Connected with version " << client_->version()
            << " cipher suite " << client_->cipher_suite_name()
            << std::endl;

  
  std::vector<uint8_t> sid_c1 = client_->session_id();
  ASSERT_EQ(32U, sid_c1.size());
  std::vector<uint8_t> sid_s1 = server_->session_id();
  ASSERT_EQ(32U, sid_s1.size());
  ASSERT_EQ(sid_c1, sid_s1);
  session_ids_.push_back(sid_c1);
}

void TlsConnectTestBase::ConnectExpectFail() {
  Handshake();

  ASSERT_EQ(TlsAgent::ERROR, client_->state());
  ASSERT_EQ(TlsAgent::ERROR, server_->state());
}

void TlsConnectTestBase::EnableSomeECDHECiphers() {
  client_->EnableSomeECDHECiphers();
  server_->EnableSomeECDHECiphers();
}


void TlsConnectTestBase::ConfigureSessionCache(SessionResumptionMode client,
                                               SessionResumptionMode server) {
  client_->ConfigureSessionCache(client);
  server_->ConfigureSessionCache(server);
}

void TlsConnectTestBase::CheckResumption(SessionResumptionMode expected) {
  ASSERT_NE(RESUME_BOTH, expected);

  int resume_ct = expected ? 1 : 0;
  int stateless_ct = (expected & RESUME_TICKET) ? 1 : 0;

  SSL3Statistics* stats = SSL_GetStatistics();
  ASSERT_EQ(resume_ct, stats->hch_sid_cache_hits);
  ASSERT_EQ(resume_ct, stats->hsh_sid_cache_hits);

  ASSERT_EQ(stateless_ct, stats->hch_sid_stateless_resumes);
  ASSERT_EQ(stateless_ct, stats->hsh_sid_stateless_resumes);

  if (resume_ct) {
    
    ASSERT_GE(2U, session_ids_.size());
    ASSERT_EQ(session_ids_[session_ids_.size()-1],
              session_ids_[session_ids_.size()-2]);
  }
}

void TlsConnectTestBase::EnableAlpn() {
  
  
  
  
  static const uint8_t val[] = { 0x01, 0x62, 0x01, 0x61 };
  client_->EnableAlpn(val, sizeof(val));
  server_->EnableAlpn(val, sizeof(val));
}

void TlsConnectTestBase::EnableSrtp() {
  client_->EnableSrtp();
  server_->EnableSrtp();
}

void TlsConnectTestBase::CheckSrtp() {
  client_->CheckSrtp();
  server_->CheckSrtp();
}

TlsConnectGeneric::TlsConnectGeneric()
  : TlsConnectTestBase(TlsConnectTestBase::ToMode(std::get<0>(GetParam())),
                       std::get<1>(GetParam())) {}

} 
