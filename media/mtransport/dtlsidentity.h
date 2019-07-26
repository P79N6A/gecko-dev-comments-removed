




#ifndef dtls_identity_h__
#define dtls_identity_h__

#include <string>

#include "m_cpp_utils.h"
#include "mozilla/RefPtr.h"
#include "nsISupportsImpl.h"
#include "ScopedNSSTypes.h"




namespace mozilla {

class DtlsIdentity {
  ~DtlsIdentity();

 public:
  
  static TemporaryRef<DtlsIdentity> Generate();

  
  
  
  
  CERTCertificate *cert() { return cert_; }
  SECKEYPrivateKey *privkey() { return privkey_; }

  std::string GetFormattedFingerprint(const std::string &algorithm = DEFAULT_HASH_ALGORITHM);

  nsresult ComputeFingerprint(const std::string algorithm,
                              unsigned char *digest,
                              std::size_t size,
                              std::size_t *digest_length);

  static nsresult ComputeFingerprint(const CERTCertificate *cert,
                                     const std::string algorithm,
                                     unsigned char *digest,
                                     std::size_t size,
                                     std::size_t *digest_length);

  static nsresult ParseFingerprint(const std::string fp,
                                   unsigned char *digest,
                                   size_t size, size_t *length);

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DtlsIdentity)

 private:
  DtlsIdentity(SECKEYPrivateKey *privkey, CERTCertificate *cert)
      : privkey_(privkey), cert_(cert) {}
  DISALLOW_COPY_ASSIGN(DtlsIdentity);

  static const std::string DEFAULT_HASH_ALGORITHM;
  static const size_t HASH_ALGORITHM_MAX_LENGTH;

  std::string FormatFingerprint(const unsigned char *digest,
                                std::size_t size);

  ScopedSECKEYPrivateKey privkey_;
  CERTCertificate *cert_;  
                           
};
}  
#endif
