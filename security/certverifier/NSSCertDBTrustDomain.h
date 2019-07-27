





#ifndef mozilla_psm__NSSCertDBTrustDomain_h
#define mozilla_psm__NSSCertDBTrustDomain_h

#include "pkix/pkixtypes.h"
#include "secmodt.h"
#include "CertVerifier.h"

namespace mozilla { namespace psm {

SECStatus InitializeNSS(const char* dir, bool readOnly);

void DisableMD5();

extern const char BUILTIN_ROOTS_MODULE_DEFAULT_NAME[];

void PORT_Free_string(char* str);







SECStatus LoadLoadableRoots( const char* dir,
                            const char* modNameUTF8);

void UnloadLoadableRoots(const char* modNameUTF8);


char* DefaultServerNicknameForCert(CERTCertificate* cert);

void SaveIntermediateCerts(const ScopedCERTCertList& certList);

class NSSCertDBTrustDomain : public mozilla::pkix::TrustDomain
{

public:
  enum OCSPFetching {
    NeverFetchOCSP = 0,
    FetchOCSPForDVSoftFail = 1,
    FetchOCSPForDVHardFail = 2,
    FetchOCSPForEV = 3,
    LocalOnlyOCSPForEV = 4,
  };
  NSSCertDBTrustDomain(SECTrustType certDBTrustType, OCSPFetching ocspFetching,
                       OCSPCache& ocspCache, void* pinArg,
                       CertVerifier::ocsp_get_config ocspGETConfig,
           CERTChainVerifyCallback* checkChainCallback = nullptr,
           ScopedCERTCertList* builtChain = nullptr);

  virtual SECStatus FindIssuer(const SECItem& encodedIssuerName,
                               IssuerChecker& checker, PRTime time);

  virtual SECStatus GetCertTrust(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                                 const mozilla::pkix::CertPolicyId& policy,
                                 const SECItem& candidateCertDER,
                          mozilla::pkix::TrustLevel* trustLevel);

  virtual SECStatus VerifySignedData(
                      const mozilla::pkix::SignedDataWithSignature& signedData,
                      const SECItem& subjectPublicKeyInfo);

  virtual SECStatus DigestBuf(const SECItem& item,  uint8_t* digestBuf,
                              size_t digestBufLen);

  virtual SECStatus CheckRevocation(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                                    const mozilla::pkix::CertID& certID,
                                    PRTime time,
                        const SECItem* stapledOCSPResponse,
                        const SECItem* aiaExtension);

  virtual SECStatus IsChainValid(const mozilla::pkix::DERArray& certChain);

private:
  enum EncodedResponseSource {
    ResponseIsFromNetwork = 1,
    ResponseWasStapled = 2
  };
  static const PRTime ServerFailureDelay = 5 * 60 * PR_USEC_PER_SEC;
  SECStatus VerifyAndMaybeCacheEncodedOCSPResponse(
    const mozilla::pkix::CertID& certID, PRTime time,
    uint16_t maxLifetimeInDays, const SECItem& encodedResponse,
    EncodedResponseSource responseSource,  bool& expired);

  const SECTrustType mCertDBTrustType;
  const OCSPFetching mOCSPFetching;
  OCSPCache& mOCSPCache; 
  void* mPinArg; 
  const CertVerifier::ocsp_get_config mOCSPGetConfig;
  CERTChainVerifyCallback* mCheckChainCallback; 
  ScopedCERTCertList* mBuiltChain; 
};

} } 

#endif
