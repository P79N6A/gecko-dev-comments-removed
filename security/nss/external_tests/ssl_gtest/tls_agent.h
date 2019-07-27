





#ifndef tls_agent_h_
#define tls_agent_h_

#include "prio.h"
#include "ssl.h"

#include <iostream>

#include "test_io.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"

namespace nss_test {

#define LOG(msg) std::cerr << name_ << ": " << msg << std::endl

enum SessionResumptionMode {
  RESUME_NONE = 0,
  RESUME_SESSIONID = 1,
  RESUME_TICKET = 2,
  RESUME_BOTH = RESUME_SESSIONID | RESUME_TICKET
};

class TlsAgent : public PollTarget {
 public:
  enum Role { CLIENT, SERVER };
  enum State { INIT, CONNECTING, CONNECTED, ERROR };

  TlsAgent(const std::string& name, Role role, Mode mode)
      : name_(name),
        mode_(mode),
        pr_fd_(nullptr),
        adapter_(nullptr),
        ssl_fd_(nullptr),
        role_(role),
        state_(INIT) {
      memset(&info_, 0, sizeof(info_));
      memset(&csinfo_, 0, sizeof(csinfo_));
      SECStatus rv = SSL_VersionRangeGetDefault(mode_ == STREAM ?
                                                ssl_variant_stream : ssl_variant_datagram,
                                                &vrange_);
      EXPECT_EQ(SECSuccess, rv);
  }

  ~TlsAgent() {
    if (pr_fd_) {
      PR_Close(pr_fd_);
    }

    if (ssl_fd_) {
      PR_Close(ssl_fd_);
    }
  }

  bool Init() {
    pr_fd_ = DummyPrSocket::CreateFD(name_, mode_);
    if (!pr_fd_) return false;

    adapter_ = DummyPrSocket::GetAdapter(pr_fd_);
    if (!adapter_) return false;

    return true;
  }

  void SetPeer(TlsAgent* peer) { adapter_->SetPeer(peer->adapter_); }

  void SetPacketFilter(PacketFilter* filter) {
    adapter_->SetPacketFilter(filter);
  }


  void StartConnect();
  void CheckKEAType(SSLKEAType type) const;
  void CheckVersion(uint16_t version) const;

  void Handshake();
  void EnableSomeECDHECiphers();
  bool EnsureTlsSetup();

  void ConfigureSessionCache(SessionResumptionMode mode);
  void SetSessionTicketsEnabled(bool en);
  void SetSessionCacheEnabled(bool en);
  void SetVersionRange(uint16_t minver, uint16_t maxver);
  void EnableAlpn(const uint8_t* val, size_t len);
  void CheckAlpn(SSLNextProtoState expected_state,
                 const std::string& expected);
  void EnableSrtp();
  void CheckSrtp();

  State state() const { return state_; }

  const char* state_str() const { return state_str(state()); }

  const char* state_str(State state) const { return states[state]; }

  PRFileDesc* ssl_fd() { return ssl_fd_; }

  uint16_t min_version() const { return vrange_.min; }
  uint16_t max_version() const { return vrange_.max; }

  bool version(uint16_t* version) const {
    if (state_ != CONNECTED) return false;

    *version = info_.protocolVersion;

    return true;
  }

  uint16_t version() const {
    EXPECT_EQ(CONNECTED, state_);

    return info_.protocolVersion;
  }

  bool cipher_suite(int16_t* cipher_suite) const {
    if (state_ != CONNECTED) return false;

    *cipher_suite = info_.cipherSuite;
    return true;
  }

  std::string cipher_suite_name() const {
    if (state_ != CONNECTED) return "UNKNOWN";

    return csinfo_.cipherSuiteName;
  }

  std::vector<uint8_t> session_id() const {
    return std::vector<uint8_t>(info_.sessionID,
                                info_.sessionID + info_.sessionIDLength);
  }

 private:
  const static char* states[];

  void SetState(State state) {
    if (state_ == state) return;

    LOG("Changing state from " << state_str(state_) << " to "
                               << state_str(state));
    state_ = state;
  }

  
  static SECStatus AuthCertificateHook(void* arg, PRFileDesc* fd,
                                       PRBool checksig, PRBool isServer) {
    return SECSuccess;
  }

  static void ReadableCallback(PollTarget* self, Event event) {
    TlsAgent* agent = static_cast<TlsAgent*>(self);
    agent->ReadableCallback_int();
  }

  void ReadableCallback_int() {
    LOG("Readable");
    Handshake();
  }

  static PRInt32 SniHook(PRFileDesc *fd, const SECItem *srvNameArr,
                         PRUint32 srvNameArrSize,
                         void *arg) {
    return SSL_SNI_CURRENT_CONFIG_IS_USED;
  }

  const std::string name_;
  Mode mode_;
  PRFileDesc* pr_fd_;
  DummyPrSocket* adapter_;
  PRFileDesc* ssl_fd_;
  Role role_;
  State state_;
  SSLChannelInfo info_;
  SSLCipherSuiteInfo csinfo_;
  SSLVersionRange vrange_;
};

}  

#endif
