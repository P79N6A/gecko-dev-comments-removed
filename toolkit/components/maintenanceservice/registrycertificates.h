



#ifndef _REGISTRYCERTIFICATES_H_
#define _REGISTRYCERTIFICATES_H_

#include "certificatecheck.h"

BOOL DoesBinaryMatchAllowedCertificates(LPCWSTR basePathForUpdate,
                                        LPCWSTR filePath,
                                        BOOL allowFallbackKeySkip = TRUE);

#endif
