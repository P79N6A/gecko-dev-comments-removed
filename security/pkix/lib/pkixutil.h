























#ifndef mozilla_pkix__pkixutil_h
#define mozilla_pkix__pkixutil_h

#include "pkix/enumclass.h"
#include "pkix/pkixtypes.h"
#include "pkixder.h"
#include "prerror.h"
#include "seccomon.h"
#include "secerr.h"

namespace mozilla { namespace pkix {

enum Result
{
  Success = 0,
  FatalError = -1,      
                        
  RecoverableError = -2 
                        
                        
};





inline Result
Fail(Result result, PRErrorCode errorCode)
{
  PR_ASSERT(result != Success);
  PR_SetError(errorCode, 0);
  return result;
}

inline Result
MapSECStatus(SECStatus srv)
{
  if (srv == SECSuccess) {
    return Success;
  }

  PRErrorCode error = PORT_GetError();
  switch (error) {
    case SEC_ERROR_EXTENSION_NOT_FOUND:
      return RecoverableError;

    case PR_INVALID_STATE_ERROR:
    case SEC_ERROR_LIBRARY_FAILURE:
    case SEC_ERROR_NO_MEMORY:
      return FatalError;
  }

  
  return RecoverableError;
}










class BackCert
{
public:
  
  
  
  MOZILLA_PKIX_ENUM_CLASS IncludeCN { No = 0, Yes = 1 };

  
  BackCert(BackCert* childCert, IncludeCN includeCN)
    : encodedAuthorityInfoAccess(nullptr)
    , encodedBasicConstraints(nullptr)
    , encodedCertificatePolicies(nullptr)
    , encodedExtendedKeyUsage(nullptr)
    , encodedKeyUsage(nullptr)
    , encodedNameConstraints(nullptr)
    , encodedInhibitAnyPolicy(nullptr)
    , childCert(childCert)
    , constrainedNames(nullptr)
    , includeCN(includeCN)
  {
  }

  Result Init(const SECItem& certDER);

  const SECItem& GetDER() const { return nssCert->derCert; }
  const SECItem& GetIssuer() const { return nssCert->derIssuer; }
  const SECItem& GetSerialNumber() const { return nssCert->serialNumber; }
  const SECItem& GetSubject() const { return nssCert->derSubject; }
  const SECItem& GetSubjectPublicKeyInfo() const
  {
    return nssCert->derPublicKey;
  }

  Result VerifyOwnSignatureWithKey(TrustDomain& trustDomain,
                                   const SECItem& subjectPublicKeyInfo) const;

  der::Version version;

  const SECItem* encodedAuthorityInfoAccess;
  const SECItem* encodedBasicConstraints;
  const SECItem* encodedCertificatePolicies;
  const SECItem* encodedExtendedKeyUsage;
  const SECItem* encodedKeyUsage;
  const SECItem* encodedNameConstraints;
  const SECItem* encodedInhibitAnyPolicy;

  BackCert* const childCert;

  
  
  
  
  
   CERTCertificate* GetNSSCert() const { return nssCert.get(); }

  
  
  
  
  Result GetConstrainedNames( const CERTGeneralName** result);

  PLArenaPool* GetArena();

private:
  ScopedCERTCertificate nssCert;

  ScopedPLArenaPool arena;
  CERTGeneralName* constrainedNames;
  IncludeCN includeCN;

  BackCert(const BackCert&) ;
  void operator=(const BackCert&); ;
};

} } 

#endif
