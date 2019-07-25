





#ifndef MAR_H__
#define MAR_H__


#include "prtypes.h"
#include "mozilla/StandardInteger.h"

#ifdef __cplusplus
extern "C" {
#endif

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
MarFile *mar_wopen(const PRUnichar *path);
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
int mar_verify_signatureW(MarFile *mar, 
                          const char *certData,
                          uint32_t sizeOfCertData);
#endif









int
mar_read_product_info_block(MarFile *mar, 
                            struct ProductInformationBlock *infoBlock);

#ifdef __cplusplus
}
#endif

#endif
