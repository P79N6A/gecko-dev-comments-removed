





































#include <string.h>
#include <fcntl.h>
#include "bzlib.h"
#include "archivereader.h"
#include "errors.h"

#if defined(XP_UNIX)
# include <sys/types.h>
#elif defined(XP_WIN)
# include <io.h>
#endif

int
ArchiveReader::Open(const char *path)
{
  if (mArchive)
    Close();

  mArchive = mar_open(path);
  if (!mArchive)
    return READ_ERROR;

  return OK;
}

void
ArchiveReader::Close()
{
  if (mArchive) {
    mar_close(mArchive);
    mArchive = NULL;
  }
}

int
ArchiveReader::ExtractFile(const char *name, const char *dest)
{
  const MarItem *item = mar_find_item(mArchive, name);
  if (!item)
    return READ_ERROR;

#ifdef XP_WIN
  int fd = _open(dest, _O_BINARY|_O_CREAT|_O_TRUNC|_O_WRONLY, item->flags);
#else
  int fd = creat(dest, item->flags);
#endif
  if (fd == -1)
    return WRITE_ERROR;

  FILE *fp = fdopen(fd, "wb");
  if (!fp)
    return WRITE_ERROR;

  int rv = ExtractItemToStream(item, fp);

  fclose(fp);
  return rv;
}

int
ArchiveReader::ExtractItemToStream(const MarItem *item, FILE *fp)
{
  

  char inbuf[BUFSIZ], outbuf[BUFSIZ];
  bz_stream strm;
  int offset, inlen, ret = OK;

  memset(&strm, 0, sizeof(strm));
  if (BZ2_bzDecompressInit(&strm, 0, 0) != BZ_OK)
    return UNEXPECTED_ERROR;

  offset = 0;
  for (;;) {
    if (offset < (int) item->length && strm.avail_in == 0) {
      inlen = mar_read(mArchive, item, offset, inbuf, BUFSIZ);
      if (inlen <= 0)
        return READ_ERROR;
      offset += inlen;
      strm.next_in = inbuf;
      strm.avail_in = inlen;
    }

    strm.next_out = outbuf;
    strm.avail_out = BUFSIZ;

    ret = BZ2_bzDecompress(&strm);
    if (ret != BZ_OK && ret != BZ_STREAM_END) {
      ret = UNEXPECTED_ERROR;
      break;
    }

    if (strm.avail_out < BUFSIZ) {
      if (fwrite(outbuf, BUFSIZ - strm.avail_out, 1, fp) != 1) {
        ret = WRITE_ERROR;
        break;
      }
    }

    if (ret == BZ_STREAM_END) {
      ret = OK;
      break;
    }
  }

  BZ2_bzDecompressEnd(&strm);
  return ret;
}
