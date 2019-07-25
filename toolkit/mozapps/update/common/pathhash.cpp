




































#include <windows.h>
#include <wincrypt.h>
#include "pathhash.h"










static void
BinaryDataToHexString(const BYTE *hash, DWORD &hashSize, 
                      LPWSTR hexString)
{
  WCHAR *p = hexString;
  for (DWORD i = 0; i < hashSize; ++i) {
    wsprintfW(p, L"%.2x", hash[i]);
    p += 2;
  }
}










static BOOL
CalculateMD5(const char *data, DWORD dataSize, 
             BYTE **hash, DWORD &hashSize)
{
  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;

  if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 0)) {
    if (NTE_BAD_KEYSET != GetLastError()) {
      return FALSE;
    }
 
    
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 
                            CRYPT_NEWKEYSET)) {
      return FALSE;
    }
  }

  if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
    return FALSE;
  }

  if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(data), 
                    dataSize, 0)) {
    return FALSE;
  }

  DWORD dwCount = sizeof(DWORD);
  if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&hashSize, 
                        &dwCount, 0)) {
    return FALSE;
  }
  
  *hash = new BYTE[hashSize];
  ZeroMemory(*hash, hashSize);
  if (!CryptGetHashParam(hHash, HP_HASHVAL, *hash, &hashSize, 0)) {
    return FALSE;
  }

  if (hHash) {
    CryptDestroyHash(hHash);
  }

  if (hProv) {
    CryptReleaseContext(hProv,0);
  }

  return TRUE;
}









BOOL
CalculateRegistryPathFromFilePath(const LPCWSTR filePath, 
                                  LPWSTR registryPath)
{
  size_t filePathLen = wcslen(filePath); 
  if (!filePathLen) {
    return FALSE;
  }

  
  if (filePath[filePathLen -1] == L'\\' || 
      filePath[filePathLen - 1] == L'/') {
    filePathLen--;
  }

  
  
  
  
  WCHAR *lowercasePath = new WCHAR[filePathLen + 2];
  wcscpy(lowercasePath, filePath);
  _wcslwr(lowercasePath);

  BYTE *hash;
  DWORD hashSize = 0;
  if (!CalculateMD5(reinterpret_cast<const char*>(lowercasePath), 
                    filePathLen * 2, 
                    &hash, hashSize)) {
    delete[] lowercasePath;
    return FALSE;
  }
  delete[] lowercasePath;

  LPCWSTR baseRegPath = L"SOFTWARE\\Mozilla\\"
    L"MaintenanceService\\";
  wcsncpy(registryPath, baseRegPath, MAX_PATH);
  BinaryDataToHexString(hash, hashSize, 
                        registryPath + wcslen(baseRegPath));
  delete[] hash;
  return TRUE;
}
