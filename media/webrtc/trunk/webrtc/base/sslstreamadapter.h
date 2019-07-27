









#ifndef WEBRTC_BASE_SSLSTREAMADAPTER_H_
#define WEBRTC_BASE_SSLSTREAMADAPTER_H_

#include <string>
#include <vector>

#include "webrtc/base/stream.h"
#include "webrtc/base/sslidentity.h"

namespace rtc {
















enum SSLRole { SSL_CLIENT, SSL_SERVER };
enum SSLMode { SSL_MODE_TLS, SSL_MODE_DTLS };


enum { SSE_MSG_TRUNC = 0xff0001 };

class SSLStreamAdapter : public StreamAdapterInterface {
 public:
  
  
  
  static SSLStreamAdapter* Create(StreamInterface* stream);

  explicit SSLStreamAdapter(StreamInterface* stream)
      : StreamAdapterInterface(stream), ignore_bad_cert_(false),
        client_auth_enabled_(true) { }

  void set_ignore_bad_cert(bool ignore) { ignore_bad_cert_ = ignore; }
  bool ignore_bad_cert() const { return ignore_bad_cert_; }

  void set_client_auth_enabled(bool enabled) { client_auth_enabled_ = enabled; }
  bool client_auth_enabled() const { return client_auth_enabled_; }

  
  
  
  
  
  
  virtual void SetIdentity(SSLIdentity* identity) = 0;

  
  
  
  
  virtual void SetServerRole(SSLRole role = SSL_SERVER) = 0;

  
  virtual void SetMode(SSLMode mode) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  virtual int StartSSLWithServer(const char* server_name) = 0;

  
  
  
  
  
  
  
  virtual int StartSSLWithPeer() = 0;

  
  
  
  
  
  
  
  virtual bool SetPeerCertificateDigest(const std::string& digest_alg,
                                        const unsigned char* digest_val,
                                        size_t digest_len) = 0;

  
  
  
  virtual bool GetPeerCertificate(SSLCertificate** cert) const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  virtual bool ExportKeyingMaterial(const std::string& label,
                                    const uint8* context,
                                    size_t context_len,
                                    bool use_context,
                                    uint8* result,
                                    size_t result_len) {
    return false;  
  }


  
  virtual bool SetDtlsSrtpCiphers(const std::vector<std::string>& ciphers) {
    return false;
  }

  virtual bool GetDtlsSrtpCipher(std::string* cipher) {
    return false;
  }

  
  static bool HaveDtls();
  static bool HaveDtlsSrtp();
  static bool HaveExporter();

 private:
  
  
  
  bool ignore_bad_cert_;

  
  
  
  bool client_auth_enabled_;
};

}  

#endif  
