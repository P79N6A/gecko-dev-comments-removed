



#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "registrycertificates.h"
#include "pathhash.h"
#include "nsWindowsHelpers.h"
#include "servicebase.h"
#include "updatehelper.h"
#include "errors.h"
#define MAX_KEY_LENGTH 255













BOOL
DoesBinaryMatchAllowedCertificates(LPCWSTR basePathForUpdate, LPCWSTR filePath,
                                   BOOL allowFallbackKeySkip)
{ 
  WCHAR maintenanceServiceKey[MAX_PATH + 1];
  if (!CalculateRegistryPathFromFilePath(basePathForUpdate, 
                                         maintenanceServiceKey)) {
    return SERVICE_UPDATER_SIGN_CALC_PATH;
  }

  
  
  
  
  
  
  HKEY baseKeyRaw;
  LONG retCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                               maintenanceServiceKey, 0, 
                               KEY_READ | KEY_WOW64_64KEY, &baseKeyRaw);
  if (retCode != ERROR_SUCCESS) {
    LOG_WARN(("Could not open key.  (%d)", retCode));
    
    
    
    retCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                            TEST_ONLY_FALLBACK_KEY_PATH, 0,
                            KEY_READ | KEY_WOW64_64KEY, &baseKeyRaw);
    if (retCode != ERROR_SUCCESS) {
      LOG_WARN(("Could not open fallback key.  (%d)", retCode));
      return SERVICE_UPDATER_SIGN_REG_OPEN;
    } else if (allowFallbackKeySkip) {
      LOG_WARN(("Fallback key present, skipping VerifyCertificateTrustForFile "
                "check and the certificate attribute registry matching "
                "check."));
      return TRUE;
    }
  }
  nsAutoRegKey baseKey(baseKeyRaw);

  
  DWORD subkeyCount = 0;
  retCode = RegQueryInfoKeyW(baseKey, nullptr, nullptr, nullptr, &subkeyCount,
                             nullptr, nullptr, nullptr, nullptr, nullptr,
                             nullptr, nullptr);
  if (retCode != ERROR_SUCCESS) {
    LOG_WARN(("Could not query info key.  (%d)", retCode));
    return SERVICE_UPDATER_SIGN_REG_QUERY;
  }

  
  for (DWORD i = 0; i < subkeyCount; i++) { 
    WCHAR subkeyBuffer[MAX_KEY_LENGTH];
    DWORD subkeyBufferCount = MAX_KEY_LENGTH;  
    retCode = RegEnumKeyExW(baseKey, i, subkeyBuffer, 
                            &subkeyBufferCount, nullptr, 
                            nullptr, nullptr, nullptr); 
    if (retCode != ERROR_SUCCESS) {
      LOG_WARN(("Could not enum certs.  (%d)", retCode));
      return SERVICE_UPDATER_SIGN_REG_ENUM;
    }

    
    HKEY subKeyRaw;
    retCode = RegOpenKeyExW(baseKey, 
                            subkeyBuffer, 
                            0, 
                            KEY_READ | KEY_WOW64_64KEY, 
                            &subKeyRaw);
    nsAutoRegKey subKey(subKeyRaw);
    if (retCode != ERROR_SUCCESS) {
      LOG_WARN(("Could not open subkey.  (%d)", retCode));
      continue; 
    }

    const int MAX_CHAR_COUNT = 256;
    DWORD valueBufSize = MAX_CHAR_COUNT * sizeof(WCHAR);
    WCHAR name[MAX_CHAR_COUNT] = { L'\0' };
    WCHAR issuer[MAX_CHAR_COUNT] = { L'\0' };

    
    retCode = RegQueryValueExW(subKey, L"name", 0, nullptr, 
                               (LPBYTE)name, &valueBufSize);
    if (retCode != ERROR_SUCCESS) {
      LOG_WARN(("Could not obtain name from registry.  (%d)", retCode));
      continue; 
    }

    
    valueBufSize = MAX_CHAR_COUNT * sizeof(WCHAR);
    retCode = RegQueryValueExW(subKey, L"issuer", 0, nullptr, 
                               (LPBYTE)issuer, &valueBufSize);
    if (retCode != ERROR_SUCCESS) {
      LOG_WARN(("Could not obtain issuer from registry.  (%d)", retCode));
      continue; 
    }

    CertificateCheckInfo allowedCertificate = {
      name, 
      issuer, 
    };

    retCode = CheckCertificateForPEFile(filePath, allowedCertificate);
    if (retCode != ERROR_SUCCESS) {
      LOG_WARN(("Error on certificate check.  (%d)", retCode));
      continue; 
    }

    retCode = VerifyCertificateTrustForFile(filePath);
    if (retCode != ERROR_SUCCESS) {
      LOG_WARN(("Error on certificate trust check.  (%d)", retCode));
      continue; 
    }

    
    return OK;
  }
  
  
  return SERVICE_UPDATER_SIGN_ERROR;
}
