





































#ifndef MAR_H__
#define MAR_H__


#ifndef WINCE
#include "prtypes.h"
#else
typedef int PRInt32;
typedef unsigned int PRUint32;
typedef wchar_t PRUnichar;
#endif

#ifdef __cplusplus
extern "C" {
#endif




typedef struct MarItem_ {
  struct MarItem_ *next;  
  PRUint32 offset;        
  PRUint32 length;        
  PRUint32 flags;         
  char name[1];           
} MarItem;

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










int mar_create(const char *dest, int numfiles, char **files);







int mar_extract(const char *path);

#ifdef __cplusplus
}
#endif

#endif
