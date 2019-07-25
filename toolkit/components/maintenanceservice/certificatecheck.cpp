




































#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <softpub.h>
#include <wintrust.h>

#include "certificatecheck.h"
#include "servicebase.h"

#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

static const int ENCODING = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;









DWORD
CheckCertificateForPEFile(LPCWSTR filePath, 
                          CertificateCheckInfo &infoToMatch)
{
  HCERTSTORE certStore = NULL;
  HCRYPTMSG cryptMsg = NULL; 
  PCCERT_CONTEXT certContext = NULL;
  PCMSG_SIGNER_INFO signerInfo = NULL;
  DWORD lastError = ERROR_SUCCESS;

  
  DWORD encoding, contentType, formatType;
  BOOL result = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                                  filePath, 
                                  CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                                  CERT_QUERY_CONTENT_FLAG_ALL, 
                                  0, &encoding, &contentType,
                                  &formatType, &certStore, &cryptMsg, NULL);
  if (!result) {
    lastError = GetLastError();
    LOG(("CryptQueryObject failed with %d\n", lastError));
    goto cleanup;
  }

  
  DWORD signerInfoSize;
  result = CryptMsgGetParam(cryptMsg, CMSG_SIGNER_INFO_PARAM, 0, 
                            NULL, &signerInfoSize);
  if (!result) {
    lastError = GetLastError();
    LOG(("CryptMsgGetParam failed with %d\n", lastError));
    goto cleanup;
  }

  
  signerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, signerInfoSize);
  if (!signerInfo) {
    lastError = GetLastError();
    LOG(("Unable to allocate memory for Signer Info.\n"));
    goto cleanup;
  }

  
  
  result = CryptMsgGetParam(cryptMsg, CMSG_SIGNER_INFO_PARAM, 0, 
                            (PVOID)signerInfo, &signerInfoSize);
  if (!result) {
    lastError = GetLastError();
    LOG(("CryptMsgGetParam failed with %d\n", lastError));
    goto cleanup;
  }

  
  CERT_INFO certInfo;     
  certInfo.Issuer = signerInfo->Issuer;
  certInfo.SerialNumber = signerInfo->SerialNumber;
  certContext = CertFindCertificateInStore(certStore, ENCODING, 0, 
                                           CERT_FIND_SUBJECT_CERT,
                                           (PVOID)&certInfo, NULL);
  if (!certContext) {
    lastError = GetLastError();
    LOG(("CertFindCertificateInStore failed with %d\n", lastError));
    goto cleanup;
  }

  if (!DoCertificateAttributesMatch(certContext, infoToMatch)) {
    lastError = ERROR_NOT_FOUND;
    LOG(("Certificate did not match issuer or name\n"));
    goto cleanup;
  }

cleanup:
  if (signerInfo) {
    LocalFree(signerInfo);
  }
  if (certContext) {
    CertFreeCertificateContext(certContext);
  }
  if (certStore) { 
    CertCloseStore(certStore, 0);
  }
  if (cryptMsg) { 
    CryptMsgClose(cryptMsg);
  }
  return lastError;
}








BOOL 
DoCertificateAttributesMatch(PCCERT_CONTEXT certContext, 
                             CertificateCheckInfo &infoToMatch)
{
  DWORD dwData;
  LPTSTR szName = NULL;

  if (infoToMatch.issuer) {
    
    dwData = CertGetNameString(certContext, 
                               CERT_NAME_SIMPLE_DISPLAY_TYPE,
                               CERT_NAME_ISSUER_FLAG, NULL,
                               NULL, 0);

    if (!dwData) {
      LOG(("CertGetNameString failed.\n"));
      return FALSE;
    }

    
    LPTSTR szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(WCHAR));
    if (!szName) {
      LOG(("Unable to allocate memory for issuer name.\n"));
      return FALSE;
    }

    
    if (!CertGetNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE,
                           CERT_NAME_ISSUER_FLAG, NULL, szName, dwData)) {
      LOG(("CertGetNameString failed.\n"));
      LocalFree(szName);
      return FALSE;
    }

    
    if (!infoToMatch.issuer ||
        wcscmp(szName, infoToMatch.issuer)) {
      LocalFree(szName);
      return FALSE;
    }

    LocalFree(szName);
    szName = NULL;
  }

  if (infoToMatch.name) {
    
    dwData = CertGetNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE,
                               0, NULL, NULL, 0);
    if (!dwData) {
      LOG(("CertGetNameString failed.\n"));
      return FALSE;
    }

    
    szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(WCHAR));
    if (!szName) {
      LOG(("Unable to allocate memory for subject name.\n"));
      return FALSE;
    }

    
    if (!(CertGetNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,
                            NULL, szName, dwData))) {
      LOG(("CertGetNameString failed.\n"));
      LocalFree(szName);
      return FALSE;
    }

    
    if (!infoToMatch.name || 
        wcscmp(szName, infoToMatch.name)) {
      LocalFree(szName);
      return FALSE;
    }

    
    LocalFree(szName);
  }

  
  return TRUE;
}







LPWSTR 
AllocateAndCopyWideString(LPCWSTR inputString)
{
  LPWSTR outputString = 
    (LPWSTR)LocalAlloc(LPTR, (wcslen(inputString) + 1) * sizeof(WCHAR));
  if (outputString) {
    lstrcpyW(outputString, inputString);
  }
  return outputString;
}







DWORD
VerifyCertificateTrustForFile(LPCWSTR filePath)
{
  
  WINTRUST_FILE_INFO fileToCheck;
  ZeroMemory(&fileToCheck, sizeof(fileToCheck));
  fileToCheck.cbStruct = sizeof(WINTRUST_FILE_INFO);
  fileToCheck.pcwszFilePath = filePath;

  
  WINTRUST_DATA trustData;
  ZeroMemory(&trustData, sizeof(trustData));
  trustData.cbStruct = sizeof(trustData);
  trustData.pPolicyCallbackData = NULL;
  trustData.pSIPClientData = NULL;
  trustData.dwUIChoice = WTD_UI_NONE;
  trustData.fdwRevocationChecks = WTD_REVOKE_NONE; 
  trustData.dwUnionChoice = WTD_CHOICE_FILE;
  trustData.dwStateAction = 0;
  trustData.hWVTStateData = NULL;
  trustData.pwszURLReference = NULL;
  
  trustData.dwUIContext = 0;
  trustData.pFile = &fileToCheck;

  GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
  
  LONG ret = WinVerifyTrust(NULL, &policyGUID, &trustData);
  if (ERROR_SUCCESS == ret) {
    
    
    LOG(("The file \"%ls\" is signed and the signature was verified.\n",
        filePath));
      return ERROR_SUCCESS;
  }

  DWORD lastError = GetLastError();
  LOG(("There was an error validating trust of the certificate for file"
       " \"%ls\". Returned: %d, Last error: %d\n", filePath, ret, lastError));
  return ret;
}
