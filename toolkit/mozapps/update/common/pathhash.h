




































#ifndef _PATHHASH_H_
#define _PATHHASH_H_









BOOL CalculateRegistryPathFromFilePath(const LPCWSTR filePath, 
                                       LPWSTR registryPath);







#define TEST_ONLY_FALLBACK_KEY_PATH \
  L"SOFTWARE\\Mozilla\\MaintenanceService\\3932ecacee736d366d6436db0f55bce4"

#endif
