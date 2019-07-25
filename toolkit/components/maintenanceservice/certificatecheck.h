




































#ifndef _CERTIFICATECHECK_H_
#define _CERTIFICATECHECK_H_

#include <wincrypt.h>

struct CertificateCheckInfo
{
  LPCWSTR name;
  LPCWSTR issuer;
};

BOOL DoCertificateAttributesMatch(PCCERT_CONTEXT pCertContext, 
                                  CertificateCheckInfo &infoToMatch);
DWORD VerifyCertificateTrustForFile(LPCWSTR filePath);
DWORD CheckCertificateForPEFile(LPCWSTR filePath, 
                                CertificateCheckInfo &infoToMatch);

#endif
