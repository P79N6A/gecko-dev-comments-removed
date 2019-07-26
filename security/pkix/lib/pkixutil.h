
















#ifndef mozilla_pkix__pkixutil_h
#define mozilla_pkix__pkixutil_h

#include "pkix/pkixtypes.h"
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
  
  
  
  enum ConstrainedNameOptions { ExcludeCN = 0, IncludeCN = 1 };

  
  BackCert(CERTCertificate* nssCert, BackCert* childCert,
           ConstrainedNameOptions cnOptions)
    : encodedBasicConstraints(nullptr)
    , encodedCertificatePolicies(nullptr)
    , encodedExtendedKeyUsage(nullptr)
    , encodedKeyUsage(nullptr)
    , encodedNameConstraints(nullptr)
    , childCert(childCert)
    , nssCert(nssCert)
    , constrainedNames(nullptr)
    , cnOptions(cnOptions)
  {
  }

  Result Init();

  const SECItem* encodedBasicConstraints;
  const SECItem* encodedCertificatePolicies;
  const SECItem* encodedExtendedKeyUsage;
  const SECItem* encodedKeyUsage;
  const SECItem* encodedNameConstraints;

  BackCert* const childCert;

  
  
  
  
  
   CERTCertificate* GetNSSCert() const { return nssCert; }

  
  
  
  
  Result GetConstrainedNames( const CERTGeneralName** result);

  
  
  Result PrependNSSCertToList(CERTCertList* results);

  PLArenaPool* GetArena();

private:
  CERTCertificate* nssCert;

  ScopedPLArenaPool arena;
  CERTGeneralName* constrainedNames;
  ConstrainedNameOptions cnOptions;

  BackCert(const BackCert&) ;
  void operator=(const BackCert&); ;
};

} } 

#endif
