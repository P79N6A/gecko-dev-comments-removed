









#ifndef WEBRTC_BASE_OPENSSLIDENTITY_H_
#define WEBRTC_BASE_OPENSSLIDENTITY_H_

#include <openssl/evp.h>
#include <openssl/x509.h>

#include <string>

#include "webrtc/base/common.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/sslidentity.h"

typedef struct ssl_ctx_st SSL_CTX;

namespace rtc {



class OpenSSLKeyPair {
 public:
  explicit OpenSSLKeyPair(EVP_PKEY* pkey) : pkey_(pkey) {
    ASSERT(pkey_ != NULL);
  }

  static OpenSSLKeyPair* Generate();

  virtual ~OpenSSLKeyPair();

  virtual OpenSSLKeyPair* GetReference() {
    AddReference();
    return new OpenSSLKeyPair(pkey_);
  }

  EVP_PKEY* pkey() const { return pkey_; }

 private:
  void AddReference();

  EVP_PKEY* pkey_;

  DISALLOW_EVIL_CONSTRUCTORS(OpenSSLKeyPair);
};



class OpenSSLCertificate : public SSLCertificate {
 public:
  
  explicit OpenSSLCertificate(X509* x509) : x509_(x509) {
    AddReference();
  }

  static OpenSSLCertificate* Generate(OpenSSLKeyPair* key_pair,
                                      const SSLIdentityParams& params);
  static OpenSSLCertificate* FromPEMString(const std::string& pem_string);

  virtual ~OpenSSLCertificate();

  virtual OpenSSLCertificate* GetReference() const {
    return new OpenSSLCertificate(x509_);
  }

  X509* x509() const { return x509_; }

  virtual std::string ToPEMString() const;

  virtual void ToDER(Buffer* der_buffer) const;

  
  virtual bool ComputeDigest(const std::string& algorithm,
                             unsigned char* digest,
                             size_t size,
                             size_t* length) const;

  
  static bool ComputeDigest(const X509* x509,
                            const std::string& algorithm,
                            unsigned char* digest,
                            size_t size,
                            size_t* length);

  virtual bool GetSignatureDigestAlgorithm(std::string* algorithm) const;

  virtual bool GetChain(SSLCertChain** chain) const {
    
    
    
    return false;
  }

 private:
  void AddReference() const;

  X509* x509_;

  DISALLOW_EVIL_CONSTRUCTORS(OpenSSLCertificate);
};



class OpenSSLIdentity : public SSLIdentity {
 public:
  static OpenSSLIdentity* Generate(const std::string& common_name);
  static OpenSSLIdentity* GenerateForTest(const SSLIdentityParams& params);
  static SSLIdentity* FromPEMStrings(const std::string& private_key,
                                     const std::string& certificate);
  virtual ~OpenSSLIdentity() { }

  virtual const OpenSSLCertificate& certificate() const {
    return *certificate_;
  }

  virtual OpenSSLIdentity* GetReference() const {
    return new OpenSSLIdentity(key_pair_->GetReference(),
                               certificate_->GetReference());
  }

  
  bool ConfigureIdentity(SSL_CTX* ctx);

 private:
  OpenSSLIdentity(OpenSSLKeyPair* key_pair,
                  OpenSSLCertificate* certificate)
      : key_pair_(key_pair), certificate_(certificate) {
    ASSERT(key_pair != NULL);
    ASSERT(certificate != NULL);
  }

  static OpenSSLIdentity* GenerateInternal(const SSLIdentityParams& params);

  scoped_ptr<OpenSSLKeyPair> key_pair_;
  scoped_ptr<OpenSSLCertificate> certificate_;

  DISALLOW_EVIL_CONSTRUCTORS(OpenSSLIdentity);
};


}  

#endif  
