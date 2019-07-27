











#ifndef WEBRTC_BASE_SSLIDENTITY_H_
#define WEBRTC_BASE_SSLIDENTITY_H_

#include <algorithm>
#include <string>
#include <vector>

#include "webrtc/base/buffer.h"
#include "webrtc/base/messagedigest.h"

namespace rtc {


class SSLCertChain;









class SSLCertificate {
 public:
  
  
  
  
  
  
  static SSLCertificate* FromPEMString(const std::string& pem_string);
  virtual ~SSLCertificate() {}

  
  
  
  virtual SSLCertificate* GetReference() const = 0;

  
  
  virtual bool GetChain(SSLCertChain** chain) const = 0;

  
  virtual std::string ToPEMString() const = 0;

  
  virtual void ToDER(Buffer* der_buffer) const = 0;

  
  
  virtual bool GetSignatureDigestAlgorithm(std::string* algorithm) const = 0;

  
  virtual bool ComputeDigest(const std::string& algorithm,
                             unsigned char* digest,
                             size_t size,
                             size_t* length) const = 0;
};




class SSLCertChain {
 public:
  
  
  explicit SSLCertChain(const std::vector<SSLCertificate*>& certs) {
    ASSERT(!certs.empty());
    certs_.resize(certs.size());
    std::transform(certs.begin(), certs.end(), certs_.begin(), DupCert);
  }
  explicit SSLCertChain(const SSLCertificate* cert) {
    certs_.push_back(cert->GetReference());
  }

  ~SSLCertChain() {
    std::for_each(certs_.begin(), certs_.end(), DeleteCert);
  }

  
  size_t GetSize() const { return certs_.size(); }

  
  const SSLCertificate& Get(size_t pos) const { return *(certs_[pos]); }

  
  
  SSLCertChain* Copy() const {
    return new SSLCertChain(certs_);
  }

 private:
  
  static SSLCertificate* DupCert(const SSLCertificate* cert) {
    return cert->GetReference();
  }

  
  static void DeleteCert(SSLCertificate* cert) { delete cert; }

  std::vector<SSLCertificate*> certs_;

  DISALLOW_COPY_AND_ASSIGN(SSLCertChain);
};





struct SSLIdentityParams {
  std::string common_name;
  int not_before;  
  int not_after;  
};




class SSLIdentity {
 public:
  
  
  
  
  
  static SSLIdentity* Generate(const std::string& common_name);

  
  static SSLIdentity* GenerateForTest(const SSLIdentityParams& params);

  
  static SSLIdentity* FromPEMStrings(const std::string& private_key,
                                     const std::string& certificate);

  virtual ~SSLIdentity() {}

  
  
  
  virtual SSLIdentity* GetReference() const = 0;

  
  virtual const SSLCertificate& certificate() const = 0;

  
  static bool PemToDer(const std::string& pem_type,
                       const std::string& pem_string,
                       std::string* der);
  static std::string DerToPem(const std::string& pem_type,
                              const unsigned char* data,
                              size_t length);
};

extern const char kPemTypeCertificate[];
extern const char kPemTypeRsaPrivateKey[];

}  

#endif  
