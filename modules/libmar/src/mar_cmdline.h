




































#ifndef MAR_CMDLINE_H__
#define MAR_CMDLINE_H__


#include "prtypes.h"

#ifdef __cplusplus
extern "C" {
#endif




















int get_mar_file_info(const char *path, 
                      int *hasSignatureBlock,
                      int *numSignatures,
                      int *hasAdditionalBlocks,
                      int *offsetAdditionalBlocks,
                      int *numAdditionalBlocks);
















int mar_verify_signature(const char *pathToMAR, 
                         const char *certData,
                         PRUint32 sizeOfCertData,
                         const char *certName);









int
read_product_info_block(char *path, 
                        struct ProductInformationBlock *infoBlock);










int
strip_signature_block(const char *src, const char * dest);

#ifdef __cplusplus
}
#endif

#endif
