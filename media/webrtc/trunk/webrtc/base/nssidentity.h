









#ifndef WEBRTC_BASE_NSSIDENTITY_H_
#define WEBRTC_BASE_NSSIDENTITY_H_

#include <string>

#include "cert.h"
#include "nspr.h"
#include "hasht.h"
#include "keythi.h"

#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/sslidentity.h"

namespace rtc {

class NSSKeyPair {
 public:
  NSSKeyPair(SECKEYPrivateKey* privkey, SECKEYPublicKey* pubkey) :
      privkey_(privkey), pubkey_(pubkey) {}
  ~NSSKeyPair();

  
  static NSSKeyPair* Generate();
  NSSKeyPair* GetReference();

  SECKEYPrivateKey* privkey() const { return privkey_; }
  SECKEYPublicKey * pubkey() const { return pubkey_; }

 private:
  SECKEYPrivateKey* privkey_;
  SECKEYPublicKey* pubkey_;

  DISALLOW_EVIL_CONSTRUCTORS(NSSKeyPair);
};


class NSSCertificate : public SSLCertificate {
 public:
  static NSSCertificate* FromPEMString(const std::string& pem_string);
  
  
  explicit NSSCertificate(CERTCertificate* cert);
  explicit NSSCertificate(CERTCertList* cert_list);
  virtual ~NSSCertificate() {
    if (certificate_)
      CERT_DestroyCertificate(certificate_);
  }

  virtual NSSCertificate* GetReference() const;

  virtual std::string ToPEMString() const;

  virtual void ToDER(Buffer* der_buffer) const;

  virtual bool GetSignatureDigestAlgorithm(std::string* algorithm) const;

  virtual bool ComputeDigest(const std::string& algorithm,
                             unsigned char* digest,
                             size_t size,
                             size_t* length) const;

  virtual bool GetChain(SSLCertChain** chain) const;

  CERTCertificate* certificate() { return certificate_; }

  
  
  
  static bool IsValidChain(const CERTCertList* cert_list);

  
  static bool GetDigestLength(const std::string& algorithm, size_t* length);

  
  bool Equals(const NSSCertificate* tocompare) const;

 private:
  NSSCertificate(CERTCertificate* cert, SSLCertChain* chain);
  static bool GetDigestObject(const std::string& algorithm,
                              const SECHashObject** hash_object);

  CERTCertificate* certificate_;
  scoped_ptr<SSLCertChain> chain_;

  DISALLOW_EVIL_CONSTRUCTORS(NSSCertificate);
};


class NSSIdentity : public SSLIdentity {
 public:
  static NSSIdentity* Generate(const std::string& common_name);
  static NSSIdentity* GenerateForTest(const SSLIdentityParams& params);
  static SSLIdentity* FromPEMStrings(const std::string& private_key,
                                     const std::string& certificate);
  virtual ~NSSIdentity() {
    LOG(LS_INFO) << "Destroying NSS identity";
  }

  virtual NSSIdentity* GetReference() const;
  virtual NSSCertificate& certificate() const;

  NSSKeyPair* keypair() const { return keypair_.get(); }

 private:
  NSSIdentity(NSSKeyPair* keypair, NSSCertificate* cert) :
      keypair_(keypair), certificate_(cert) {}

  static NSSIdentity* GenerateInternal(const SSLIdentityParams& params);

  rtc::scoped_ptr<NSSKeyPair> keypair_;
  rtc::scoped_ptr<NSSCertificate> certificate_;

  DISALLOW_EVIL_CONSTRUCTORS(NSSIdentity);
};

}  

#endif  
