
















#include "insanity/pkix.h"
#include "pkixcheck.h"
#include "pkixder.h"
#include "pkixutil.h"
#include "secder.h"

namespace insanity { namespace pkix {

Result
CheckTimes(const CERTCertificate* cert, PRTime time)
{
  PR_ASSERT(cert);

  SECCertTimeValidity validity = CERT_CheckCertValidTimes(cert, time, false);
  if (validity != secCertTimeValid) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }

  return Success;
}


Result
CheckBasicConstraints(const BackCert& cert,
                      EndEntityOrCA endEntityOrCA,
                      bool isTrustAnchor,
                      unsigned int subCACount)
{
  CERTBasicConstraints basicConstraints;
  if (cert.encodedBasicConstraints) {
    SECStatus rv = CERT_DecodeBasicConstraintValue(&basicConstraints,
                                                   cert.encodedBasicConstraints);
    if (rv != SECSuccess) {
      return MapSECStatus(rv);
    }
  } else {
    
    basicConstraints.isCA = false;
    basicConstraints.pathLenConstraint = 0;

    
    
    
    
    
    
    
    
    
    if (endEntityOrCA == MustBeCA && isTrustAnchor) {
      const CERTCertificate* nssCert = cert.GetNSSCert();

      der::Input versionDer;
      if (versionDer.Init(nssCert->version.data, nssCert->version.len)
            != der::Success) {
        return RecoverableError;
      }
      uint8_t version;
      if (der::OptionalVersion(versionDer, version) || der::End(versionDer)
            != der::Success) {
        return RecoverableError;
      }
      if (version == 1) {
        basicConstraints.isCA = true;
        basicConstraints.pathLenConstraint = CERT_UNLIMITED_PATH_CONSTRAINT;
      }
    }
  }

  if (endEntityOrCA == MustBeEndEntity) {
    

    if (basicConstraints.isCA) {
      
      
      
      
      
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_CERT_TYPE);
    }

    return Success;
  }

  PORT_Assert(endEntityOrCA == MustBeCA);

  
  if (!basicConstraints.isCA) {
    return Fail(RecoverableError, SEC_ERROR_CA_CERT_INVALID);
  }

  if (basicConstraints.pathLenConstraint >= 0) {
    if (subCACount >
           static_cast<unsigned int>(basicConstraints.pathLenConstraint)) {
      return Fail(RecoverableError, SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID);
    }
  }

  return Success;
}



Result
CheckExtensions(BackCert& cert,
                EndEntityOrCA endEntityOrCA,
                bool isTrustAnchor,
                unsigned int subCACount)
{
  
  

  PLArenaPool* arena = cert.GetArena();
  if (!arena) {
    return FatalError;
  }

  Result rv;

  
  
  
  
  
  

  
  
  
  rv = CheckBasicConstraints(cert, endEntityOrCA, isTrustAnchor, subCACount);
  if (rv != Success) {
    return rv;
  }

  
  
  
  
  

  return Success;
}

} } 
