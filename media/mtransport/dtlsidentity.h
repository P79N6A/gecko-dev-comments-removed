




#ifndef dtls_identity_h__
#define dtls_identity_h__

#include <string>

#include "m_cpp_utils.h"
#include "mozilla/RefPtr.h"
#include "nsISupportsImpl.h"
#include "sslt.h"
#include "ScopedNSSTypes.h"




namespace mozilla {

class DtlsIdentity final {
 public:
  
  DtlsIdentity(SECKEYPrivateKey *privkey,
               CERTCertificate *cert,
               SSLKEAType authType)
      : private_key_(privkey), cert_(cert), auth_type_(authType) {}

  
  
  static RefPtr<DtlsIdentity> Generate();

  
  
  CERTCertificate *cert() const { return cert_; }
  SECKEYPrivateKey *privkey() const { return private_key_; }
  
  
  
  
  SSLKEAType auth_type() const { return auth_type_; }

  nsresult ComputeFingerprint(const std::string algorithm,
                              uint8_t *digest,
                              size_t size,
                              size_t *digest_length) const;
  static nsresult ComputeFingerprint(const CERTCertificate *cert,
                                     const std::string algorithm,
                                     uint8_t *digest,
                                     size_t size,
                                     size_t *digest_length);

  static const std::string DEFAULT_HASH_ALGORITHM;
  enum {
    HASH_ALGORITHM_MAX_LENGTH = 64
  };

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DtlsIdentity)

 private:
  ~DtlsIdentity();
  DISALLOW_COPY_ASSIGN(DtlsIdentity);

  ScopedSECKEYPrivateKey private_key_;
  CERTCertificate *cert_;  
                           
  SSLKEAType auth_type_;
};
}  
#endif
