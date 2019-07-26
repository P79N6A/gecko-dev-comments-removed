





#ifndef MAR_H__
#define MAR_H__

#include "mozilla/Assertions.h"
#include "mozilla/StandardInteger.h"

#ifdef __cplusplus
extern "C" {
#endif







#define MAX_SIGNATURES 8
MOZ_STATIC_ASSERT(MAX_SIGNATURES <= 9, "too many signatures");

struct ProductInformationBlock {
  const char *MARChannelID;
  const char *productVersion;
};




typedef struct MarItem_ {
  struct MarItem_ *next;  
  uint32_t offset;        
  uint32_t length;        
  uint32_t flags;         
  char name[1];           
} MarItem;

#define TABLESIZE 256

struct MarFile_ {
  FILE *fp;
  MarItem *item_table[TABLESIZE];
};

typedef struct MarFile_ MarFile;








typedef int (* MarItemCallback)(MarFile *mar, const MarItem *item, void *data);







MarFile *mar_open(const char *path);

#ifdef XP_WIN
MarFile *mar_wopen(const wchar_t *path);
#endif





void mar_close(MarFile *mar);







const MarItem *mar_find_item(MarFile *mar, const char *item);










int mar_enum_items(MarFile *mar, MarItemCallback callback, void *data);











int mar_read(MarFile *mar, const MarItem *item, int offset, char *buf,
             int bufsize);











int mar_create(const char *dest, 
               int numfiles, 
               char **files, 
               struct ProductInformationBlock *infoBlock);







int mar_extract(const char *path);





















#ifdef XP_WIN
int mar_verify_signaturesW(MarFile *mar,
                           const uint8_t * const *certData,
                           const uint32_t *certDataSizes,
                           uint32_t certCount);
#endif









int
mar_read_product_info_block(MarFile *mar, 
                            struct ProductInformationBlock *infoBlock);

#ifdef __cplusplus
}
#endif

#endif
