




































#ifndef MAR_CMDLINE_H__
#define MAR_CMDLINE_H__


#include "prtypes.h"

#ifdef __cplusplus
extern "C" {
#endif









int is_old_mar(const char *path, int *oldMar);
















int mar_verify_signature(const char *pathToMAR, 
                         const char *certData,
                         PRUint32 sizeOfCertData,
                         const char *certName);

#ifdef __cplusplus
}
#endif

#endif
