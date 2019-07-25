




































#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "registrycertificates.h"
#include "pathhash.h"
#include "nsWindowsHelpers.h"
#include "servicebase.h"
#define MAX_KEY_LENGTH 255







BOOL
DoesBinaryMatchAllowedCertificates(LPCWSTR basePathForUpdate, LPCWSTR filePath)
{ 
  WCHAR maintenanceServiceKey[MAX_PATH + 1];
  if (!CalculateRegistryPathFromFilePath(basePathForUpdate, 
                                         maintenanceServiceKey)) {
    return FALSE;
  }

  
  
  
  
  
  
  HKEY baseKeyRaw;
  LONG retCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                               maintenanceServiceKey, 0, 
                               KEY_READ | KEY_WOW64_64KEY, &baseKeyRaw);
  if (retCode != ERROR_SUCCESS) {
    LOG(("Could not open key. (%d)\n", retCode));
    
    
    
    retCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                            TEST_ONLY_FALLBACK_KEY_PATH, 0,
                            KEY_READ | KEY_WOW64_64KEY, &baseKeyRaw);
    if (retCode != ERROR_SUCCESS) {
      LOG(("Could not open fallback key. (%d)\n", retCode));
      return FALSE;
    }
  }
  nsAutoRegKey baseKey(baseKeyRaw);

  
  DWORD subkeyCount = 0;
  retCode = RegQueryInfoKeyW(baseKey, NULL, NULL, NULL, &subkeyCount, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL);
  if (retCode != ERROR_SUCCESS) {
    LOG(("Could not query info key: %d\n", retCode));
    return FALSE;
  }

  
  for (DWORD i = 0; i < subkeyCount; i++) { 
    WCHAR subkeyBuffer[MAX_KEY_LENGTH];
    DWORD subkeyBufferCount = MAX_KEY_LENGTH;  
    retCode = RegEnumKeyExW(baseKey, i, subkeyBuffer, 
                            &subkeyBufferCount, NULL, 
                            NULL, NULL, NULL); 
    if (retCode != ERROR_SUCCESS) {
      LOG(("Could not enum Certs: %d\n", retCode));
      return FALSE;
    }

    
    HKEY subKeyRaw;
    retCode = RegOpenKeyExW(baseKey, 
                            subkeyBuffer, 
                            0, 
                            KEY_READ | KEY_WOW64_64KEY, 
                            &subKeyRaw);
    nsAutoRegKey subKey(subKeyRaw);
    if (retCode != ERROR_SUCCESS) {
      LOG(("Could not open subkey: %d\n", retCode));
      continue; 
    }

    const int MAX_CHAR_COUNT = 256;
    DWORD valueBufSize = MAX_CHAR_COUNT * sizeof(WCHAR);
    WCHAR name[MAX_CHAR_COUNT] = { L'\0' };
    WCHAR issuer[MAX_CHAR_COUNT] = { L'\0' };

    
    retCode = RegQueryValueExW(subKey, L"name", 0, NULL, 
                               (LPBYTE)name, &valueBufSize);
    if (retCode != ERROR_SUCCESS) {
      LOG(("Could not obtain name from registry: %d\n", retCode));
      continue; 
    }

    
    valueBufSize = MAX_CHAR_COUNT * sizeof(WCHAR);
    retCode = RegQueryValueExW(subKey, L"issuer", 0, NULL, 
                               (LPBYTE)issuer, &valueBufSize);
    if (retCode != ERROR_SUCCESS) {
      LOG(("Could not obtain issuer from registry: %d\n", retCode));
      continue; 
    }

    CertificateCheckInfo allowedCertificate = {
      name, 
      issuer, 
    };

    retCode = CheckCertificateForPEFile(filePath, allowedCertificate);
    if (retCode != ERROR_SUCCESS) {
      LOG(("Error on certificate check: %d\n", retCode));
      continue; 
    }

    retCode = VerifyCertificateTrustForFile(filePath);
    if (retCode != ERROR_SUCCESS) {
      LOG(("Error on certificate trust check: %d\n", retCode));
      continue; 
    }

    
    return TRUE; 
  }
  
  
  return FALSE;
}
