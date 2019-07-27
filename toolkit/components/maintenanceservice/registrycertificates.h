



#ifndef _REGISTRYCERTIFICATES_H_
#define _REGISTRYCERTIFICATES_H_

#include "certificatecheck.h"

int DoesBinaryMatchAllowedCertificates(LPCWSTR basePathForUpdate,
                                       LPCWSTR filePath,
                                       BOOL allowFallbackKeySkip = TRUE);

#endif
