





































#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "bzlib.h"
#include "archivereader.h"
#include "errors.h"

#if defined(XP_UNIX)
# include <sys/types.h>
#elif defined(XP_WIN)
# include <io.h>
#endif

#ifdef WINCE
#include "updater_wince.h"
#endif

static int inbuf_size  = 262144;
static int outbuf_size = 262144;
static char *inbuf  = NULL;
static char *outbuf = NULL;

int
ArchiveReader::Open(const NS_tchar *path)
{
  if (mArchive)
    Close();

  if (!inbuf) {
    inbuf = (char *)malloc(inbuf_size);
    if (!inbuf) {
      
      inbuf_size = 1024;
      inbuf = (char *)malloc(inbuf_size);
      if (!inbuf)
        return MEM_ERROR;
    }
  }

  if (!outbuf) {
    outbuf = (char *)malloc(outbuf_size);
    if (!outbuf) {
      
      outbuf_size = 1024;
      outbuf = (char *)malloc(outbuf_size);
      if (!outbuf)
        return MEM_ERROR;
    }
  }

#ifdef XP_WIN
  mArchive = mar_wopen(path);
#else
  mArchive = mar_open(path);
#endif
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

  if (inbuf) {
    free(inbuf);
    inbuf = NULL;
  }

  if (outbuf) {
    free(outbuf);
    outbuf = NULL;
  }
}

int
ArchiveReader::ExtractFile(const char *name, const NS_tchar *dest)
{
  const MarItem *item = mar_find_item(mArchive, name);
  if (!item)
    return READ_ERROR;

#ifdef XP_WIN
  FILE* fp = _wfopen(dest, L"wb+");
#else
  int fd = creat(dest, item->flags);
  if (fd == -1)
    return WRITE_ERROR;

  FILE *fp = fdopen(fd, "wb");
#endif
  if (!fp)
    return WRITE_ERROR;

  int rv = ExtractItemToStream(item, fp);

  fclose(fp);
  return rv;
}

int
ArchiveReader::ExtractFileToStream(const char *name, FILE *fp)
{
  const MarItem *item = mar_find_item(mArchive, name);
  if (!item)
    return READ_ERROR;

  return ExtractItemToStream(item, fp);
}

int
ArchiveReader::ExtractItemToStream(const MarItem *item, FILE *fp)
{
  

  bz_stream strm;
  int offset, inlen, outlen, ret = OK;

  memset(&strm, 0, sizeof(strm));
  if (BZ2_bzDecompressInit(&strm, 0, 0) != BZ_OK)
    return UNEXPECTED_ERROR;

  offset = 0;
  for (;;) {
    if (!item->length) {
      ret = UNEXPECTED_ERROR;
      break;
    }

    if (offset < (int) item->length && strm.avail_in == 0) {
      inlen = mar_read(mArchive, item, offset, inbuf, inbuf_size);
      if (inlen <= 0)
        return READ_ERROR;
      offset += inlen;
      strm.next_in = inbuf;
      strm.avail_in = inlen;
    }

    strm.next_out = outbuf;
    strm.avail_out = outbuf_size;

    ret = BZ2_bzDecompress(&strm);
    if (ret != BZ_OK && ret != BZ_STREAM_END) {
      ret = UNEXPECTED_ERROR;
      break;
    }

    outlen = outbuf_size - strm.avail_out;
    if (outlen) {
      if (fwrite(outbuf, outlen, 1, fp) != 1) {
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
