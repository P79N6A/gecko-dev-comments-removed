









#ifndef WEBRTC_BASE_OPENSSLSTREAMADAPTER_H__
#define WEBRTC_BASE_OPENSSLSTREAMADAPTER_H__

#include <string>
#include <vector>

#include "webrtc/base/buffer.h"
#include "webrtc/base/sslstreamadapter.h"
#include "webrtc/base/opensslidentity.h"

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct x509_store_ctx_st X509_STORE_CTX;

namespace rtc {




























class OpenSSLIdentity;



class OpenSSLStreamAdapter : public SSLStreamAdapter {
 public:
  explicit OpenSSLStreamAdapter(StreamInterface* stream);
  virtual ~OpenSSLStreamAdapter();

  virtual void SetIdentity(SSLIdentity* identity);

  
  virtual void SetServerRole(SSLRole role = SSL_SERVER);
  virtual bool SetPeerCertificateDigest(const std::string& digest_alg,
                                        const unsigned char* digest_val,
                                        size_t digest_len);

  virtual bool GetPeerCertificate(SSLCertificate** cert) const;

  virtual int StartSSLWithServer(const char* server_name);
  virtual int StartSSLWithPeer();
  virtual void SetMode(SSLMode mode);

  virtual StreamResult Read(void* data, size_t data_len,
                            size_t* read, int* error);
  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error);
  virtual void Close();
  virtual StreamState GetState() const;

  
  virtual bool ExportKeyingMaterial(const std::string& label,
                                    const uint8* context,
                                    size_t context_len,
                                    bool use_context,
                                    uint8* result,
                                    size_t result_len);


  
  virtual bool SetDtlsSrtpCiphers(const std::vector<std::string>& ciphers);
  virtual bool GetDtlsSrtpCipher(std::string* cipher);

  
  static bool HaveDtls();
  static bool HaveDtlsSrtp();
  static bool HaveExporter();

 protected:
  virtual void OnEvent(StreamInterface* stream, int events, int err);

 private:
  enum SSLState {
    
    
    SSL_NONE,
    SSL_WAIT,  
    SSL_CONNECTING,  
    SSL_CONNECTED,  
    SSL_ERROR,  
    SSL_CLOSED  
  };

  enum { MSG_TIMEOUT = MSG_MAX+1};

  
  
  
  

  
  
  
  int StartSSL();
  
  int BeginSSL();
  
  int ContinueSSL();

  
  
  
  
  
  
  void Error(const char* context, int err, bool signal);
  void Cleanup();

  
  virtual void OnMessage(Message* msg);

  
  void FlushInput(unsigned int left);

  
  SSL_CTX* SetupSSLContext();
  
  bool SSLPostConnectionCheck(SSL* ssl, const char* server_name,
                              const X509* peer_cert,
                              const std::string& peer_digest);
  
  
  
  
  static int SSLVerifyCallback(int ok, X509_STORE_CTX* store);

  SSLState state_;
  SSLRole role_;
  int ssl_error_code_;  
  
  
  bool ssl_read_needs_write_;
  bool ssl_write_needs_read_;

  SSL* ssl_;
  SSL_CTX* ssl_ctx_;

  
  scoped_ptr<OpenSSLIdentity> identity_;
  
  
  std::string ssl_server_name_;
  
  
  scoped_ptr<OpenSSLCertificate> peer_certificate_;
  
  
  Buffer peer_certificate_digest_value_;
  std::string peer_certificate_digest_algorithm_;

  
  bool custom_verification_succeeded_;

  
  std::string srtp_ciphers_;

  
  SSLMode ssl_mode_;
};



}  

#endif  
