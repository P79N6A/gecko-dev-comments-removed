









#ifndef WEBRTC_BASE_SSLSTREAMADAPTERHELPER_H_
#define WEBRTC_BASE_SSLSTREAMADAPTERHELPER_H_

#include <string>
#include <vector>

#include "webrtc/base/buffer.h"
#include "webrtc/base/stream.h"
#include "webrtc/base/sslidentity.h"
#include "webrtc/base/sslstreamadapter.h"

namespace rtc {




class SSLStreamAdapterHelper : public SSLStreamAdapter {
 public:
  explicit SSLStreamAdapterHelper(StreamInterface* stream)
      : SSLStreamAdapter(stream),
        state_(SSL_NONE),
        role_(SSL_CLIENT),
        ssl_error_code_(0),  
        ssl_mode_(SSL_MODE_TLS) {}


  
  virtual void SetIdentity(SSLIdentity* identity);
  virtual void SetServerRole(SSLRole role = SSL_SERVER);
  virtual void SetMode(SSLMode mode);

  virtual int StartSSLWithServer(const char* server_name);
  virtual int StartSSLWithPeer();

  virtual bool SetPeerCertificateDigest(const std::string& digest_alg,
                                        const unsigned char* digest_val,
                                        size_t digest_len);
  virtual bool GetPeerCertificate(SSLCertificate** cert) const;
  virtual StreamState GetState() const;
  virtual void Close();

 protected:
  
  
  
  
  

  
  int ContinueSSL();

  
  
  
  
  
  
  virtual void Error(const char* context, int err, bool signal);

  
  virtual int BeginSSL() = 0;
  virtual void Cleanup() = 0;
  virtual bool GetDigestLength(const std::string& algorithm,
                               size_t* length) = 0;

  enum SSLState {
    
    
    SSL_NONE,
    SSL_WAIT,  
    SSL_CONNECTING,  
    SSL_CONNECTED,  
    SSL_ERROR,  
    SSL_CLOSED  
  };

  
  enum { MSG_DTLS_TIMEOUT = MSG_MAX + 1 };

  SSLState state_;
  SSLRole role_;
  int ssl_error_code_;  

  
  scoped_ptr<SSLIdentity> identity_;
  
  
  std::string ssl_server_name_;
  
  scoped_ptr<SSLCertificate> peer_certificate_;

  
  Buffer peer_certificate_digest_value_;
  std::string peer_certificate_digest_algorithm_;

  
  SSLMode ssl_mode_;

 private:
  
  
  
  int StartSSL();
};

}  

#endif  
