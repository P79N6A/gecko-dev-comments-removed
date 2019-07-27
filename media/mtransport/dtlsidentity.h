




#ifndef dtls_identity_h__
#define dtls_identity_h__

#include <string>

#include "m_cpp_utils.h"
#include "mozilla/RefPtr.h"
#include "nsISupportsImpl.h"
#include "ScopedNSSTypes.h"




namespace mozilla {

class DtlsIdentity {
 private:
  ~DtlsIdentity();

 public:
  
  static already_AddRefed<DtlsIdentity> Generate();

  
  
  
  
  CERTCertificate *cert() { return cert_; }
  SECKEYPrivateKey *privkey() { return privkey_; }

  nsresult ComputeFingerprint(const std::string algorithm,
                              unsigned char *digest,
                              std::size_t size,
                              std::size_t *digest_length);

  static nsresult ComputeFingerprint(const CERTCertificate *cert,
                                     const std::string algorithm,
                                     unsigned char *digest,
                                     std::size_t size,
                                     std::size_t *digest_length);
  static const std::string DEFAULT_HASH_ALGORITHM;
  enum {
    HASH_ALGORITHM_MAX_LENGTH = 64
  };

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DtlsIdentity)

private:
  DtlsIdentity(SECKEYPrivateKey *privkey, CERTCertificate *cert)
      : privkey_(privkey), cert_(cert) {}
  DISALLOW_COPY_ASSIGN(DtlsIdentity);

  ScopedSECKEYPrivateKey privkey_;
  CERTCertificate *cert_;  
                           
};
}  
#endif
