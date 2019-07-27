



#ifndef MAR_CMDLINE_H__
#define MAR_CMDLINE_H__



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









int
read_product_info_block(char *path, 
                        struct ProductInformationBlock *infoBlock);










int
refresh_product_info_block(const char *path,
                           struct ProductInformationBlock *infoBlock);










int
strip_signature_block(const char *src, const char * dest);










int
extract_signature(const char *src, uint32_t sigIndex, const char * dest);











int
import_signature(const char *src,
                 uint32_t sigIndex,
                 const char * base64SigFile,
                 const char *dest);

#ifdef __cplusplus
}
#endif

#endif
