



#ifndef MAR_CMDLINE_H__
#define MAR_CMDLINE_H__


#include "prtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ProductInformationBlock;



















int get_mar_file_info(const char *path, 
                      int *hasSignatureBlock,
                      uint32_t *numSignatures,
                      int *hasAdditionalBlocks,
                      uint32_t *offsetAdditionalBlocks,
                      uint32_t *numAdditionalBlocks);
















int mar_verify_signature(const char *pathToMAR, 
                         const char *certData,
                         uint32_t sizeOfCertData,
                         const char *certName);









int
read_product_info_block(char *path, 
                        struct ProductInformationBlock *infoBlock);










int
refresh_product_info_block(const char *path,
                           struct ProductInformationBlock *infoBlock);










int
strip_signature_block(const char *src, const char * dest);

#ifdef __cplusplus
}
#endif

#endif
