





































#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mar.h"
#include "mar_private.h"

#ifdef XP_WIN
#include <io.h>
#include <direct.h>
#endif


static int mar_ensure_parent_dir(const char *path)
{
  char *slash = strrchr(path, '/');
  if (slash)
  {
    *slash = '\0';
    mar_ensure_parent_dir(path);
#ifdef XP_WIN
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
    *slash = '/';
  }
  return 0;
}

static int mar_test_callback(MarFile *mar, const MarItem *item, void *unused) {
  FILE *fp;
  char buf[BLOCKSIZE];
  int fd, len, offset = 0;

  if (mar_ensure_parent_dir(item->name))
    return -1;

#ifdef XP_WIN
  fd = _open(item->name, _O_BINARY|_O_CREAT|_O_TRUNC|_O_WRONLY, item->flags);
#else
  fd = creat(item->name, item->flags);
#endif
  if (fd == -1)
    return -1;

  fp = fdopen(fd, "wb");
  if (!fp)
    return -1;

  while ((len = mar_read(mar, item, offset, buf, sizeof(buf))) > 0) {
    if (fwrite(buf, len, 1, fp) != 1)
      break;
    offset += len;
  }

  fclose(fp);
  return len == 0 ? 0 : -1;
}

int mar_extract(const char *path) {
  MarFile *mar;
  int rv;

  mar = mar_open(path);
  if (!mar)
    return -1;

  rv = mar_enum_items(mar, mar_test_callback, NULL);

  mar_close(mar);
  return rv;
}
