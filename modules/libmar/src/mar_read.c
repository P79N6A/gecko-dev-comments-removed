





































#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mar.h"
#include "mar_private.h"

#ifdef XP_WIN
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#define TABLESIZE 256

struct MarFile_ {
  FILE *fp;
  MarItem *item_table[TABLESIZE];
};


static PRUint32 mar_hash_name(const char *name) {
  PRUint32 val = 0;
  unsigned char* c;

  for (c = (unsigned char *) name; *c; ++c)
    val = val*37 + *c;

  return val % TABLESIZE;
}

static int mar_insert_item(MarFile *mar, const char *name, int namelen,
                           PRUint32 offset, PRUint32 length, PRUint32 flags) {
  MarItem *item, *root;
  PRUint32 hash;
  
  item = (MarItem *) malloc(sizeof(MarItem) + namelen);
  if (!item)
    return -1;
  item->next = NULL;
  item->offset = offset;
  item->length = length;
  item->flags = flags;
  memcpy(item->name, name, namelen + 1);

  hash = mar_hash_name(name);

  root = mar->item_table[hash];
  if (!root) {
    mar->item_table[hash] = item;
  } else {
    
    while (root->next)
      root = root->next;
    root->next = item;
  }
  return 0;
}

static int mar_consume_index(MarFile *mar, char **buf, const char *buf_end) {
  







  PRUint32 offset;
  PRUint32 length;
  PRUint32 flags;
  const char *name;
  int namelen;

  if ((buf_end - *buf) < (int)(3*sizeof(PRUint32) + 2))
    return -1;

  memcpy(&offset, *buf, sizeof(offset));
  *buf += sizeof(offset);

  memcpy(&length, *buf, sizeof(length));
  *buf += sizeof(length);

  memcpy(&flags, *buf, sizeof(flags));
  *buf += sizeof(flags);

  offset = ntohl(offset);
  length = ntohl(length);
  flags = ntohl(flags);

  name = *buf;
  
  while (**buf) {
    if (*buf == buf_end)
      return -1;
    ++(*buf);
  }
  namelen = (*buf - name);
  
  if (*buf == buf_end)
    return -1;
  ++(*buf);

  return mar_insert_item(mar, name, namelen, offset, length, flags);
}

static int mar_read_index(MarFile *mar) {
  char id[MAR_ID_SIZE], *buf, *bufptr, *bufend;
  PRUint32 offset_to_index, size_of_index;

  
  if (fread(id, MAR_ID_SIZE, 1, mar->fp) != 1)
    return -1;
  if (memcmp(id, MAR_ID, MAR_ID_SIZE) != 0)
    return -1;

  if (fread(&offset_to_index, sizeof(PRUint32), 1, mar->fp) != 1)
    return -1;
  offset_to_index = ntohl(offset_to_index);

  if (fseek(mar->fp, offset_to_index, SEEK_SET))
    return -1;
  if (fread(&size_of_index, sizeof(PRUint32), 1, mar->fp) != 1)
    return -1;
  size_of_index = ntohl(size_of_index);

  buf = (char *) malloc(size_of_index);
  if (!buf)
    return -1;
  if (fread(buf, size_of_index, 1, mar->fp) != 1) {
    free(buf);
    return -1;
  }

  bufptr = buf;
  bufend = buf + size_of_index;
  while (bufptr < bufend && mar_consume_index(mar, &bufptr, bufend) == 0);

  free(buf);
  return (bufptr == bufend) ? 0 : -1;
}

MarFile *mar_open(const char *path) {
  MarFile *mar;
  FILE *fp;

  fp = fopen(path, "rb");
  if (!fp)
    return NULL;

  mar = (MarFile *) malloc(sizeof(*mar));
  if (!mar) {
    fclose(fp);
    return NULL;
  }

  mar->fp = fp;
  memset(mar->item_table, 0, sizeof(mar->item_table));
  if (mar_read_index(mar)) {
    mar_close(mar);
    return NULL;
  }

  return mar;
}

void mar_close(MarFile *mar) {
  MarItem *item;
  int i;

  fclose(mar->fp);

  for (i = 0; i < TABLESIZE; ++i) {
    item = mar->item_table[i];
    while (item) {
      MarItem *temp = item;
      item = item->next;
      free(temp);
    }
  }

  free(mar);
}

const MarItem *mar_find_item(MarFile *mar, const char *name) {
  PRUint32 hash;
  const MarItem *item;

  hash = mar_hash_name(name);

  item = mar->item_table[hash];
  while (item && strcmp(item->name, name) != 0)
    item = item->next;

  return item;
}

int mar_enum_items(MarFile *mar, MarItemCallback callback, void *closure) {
  MarItem *item;
  int i;

  for (i = 0; i < TABLESIZE; ++i) {
    item = mar->item_table[i];
    while (item) {
      int rv = callback(mar, item, closure);
      if (rv)
        return rv;
      item = item->next;
    }
  }

  return 0;
}

int mar_read(MarFile *mar, const MarItem *item, int offset, char *buf,
             int bufsize) {
  int nr;

  if (offset == (int) item->length)
    return 0;
  if (offset > (int) item->length)
    return -1;

  nr = item->length - offset;
  if (nr > bufsize)
    nr = bufsize;

  if (fseek(mar->fp, item->offset + offset, SEEK_SET))
    return -1;

  return fread(buf, 1, nr, mar->fp);
}
