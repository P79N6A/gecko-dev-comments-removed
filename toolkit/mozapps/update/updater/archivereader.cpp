





#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "bzlib.h"
#include "archivereader.h"
#include "errors.h"
#ifdef XP_WIN
#include "nsAlgorithm.h" 
#include "updatehelper.h"
#endif



#include "primaryCert.h"
#include "secondaryCert.h"
#include "xpcshellCert.h"

#define UPDATER_NO_STRING_GLUE_STL
#include "nsVersionComparator.cpp"
#undef UPDATER_NO_STRING_GLUE_STL

#if defined(XP_UNIX)
# include <sys/types.h>
#elif defined(XP_WIN)
# include <io.h>
#endif

static int inbuf_size  = 262144;
static int outbuf_size = 262144;
static char *inbuf  = nullptr;
static char *outbuf = nullptr;









template<uint32_t SIZE>
int
VerifyLoadedCert(MarFile *archive, const uint8_t (&certData)[SIZE])
{
  const uint32_t size = SIZE;
  const uint8_t* const data = &certData[0];
  if (mar_verify_signatures(archive, &data, &size, 1)) {
    return CERT_VERIFY_ERROR;
  }

  return OK;
}









int
ArchiveReader::VerifySignature()
{
  if (!mArchive) {
    return ARCHIVE_NOT_OPEN;
  }

  
  
  int rv = OK;
#ifdef XP_WIN
  if (DoesFallbackKeyExist()) {
    rv = VerifyLoadedCert(mArchive, xpcshellCertData);
  } else
#endif
  {
    rv = VerifyLoadedCert(mArchive, primaryCertData);
    if (rv != OK) {
      rv = VerifyLoadedCert(mArchive, secondaryCertData);
    }
  }
  return rv;
}






















int
ArchiveReader::VerifyProductInformation(const char *MARChannelID, 
                                        const char *appVersion)
{
  if (!mArchive) {
    return ARCHIVE_NOT_OPEN;
  }

  ProductInformationBlock productInfoBlock;
  int rv = mar_read_product_info_block(mArchive, 
                                       &productInfoBlock);
  if (rv != OK) {
    return COULD_NOT_READ_PRODUCT_INFO_BLOCK_ERROR;
  }

  
  
  if (MARChannelID && strlen(MARChannelID)) {
    
    const char *delimiter = " ,\t";
    
    
    char channelCopy[512] = { 0 };
    strncpy(channelCopy, MARChannelID, sizeof(channelCopy) - 1);
    char *channel = strtok(channelCopy, delimiter);
    rv = MAR_CHANNEL_MISMATCH_ERROR;
    while(channel) {
      if (!strcmp(channel, productInfoBlock.MARChannelID)) {
        rv = OK;
        break;
      }
      channel = strtok(nullptr, delimiter);
    }
  }

  if (rv == OK) {
    








    int versionCompareResult = 
      mozilla::CompareVersions(appVersion, productInfoBlock.productVersion);
    if (1 == versionCompareResult) {
      rv = VERSION_DOWNGRADE_ERROR;
    }
  }

  free((void *)productInfoBlock.MARChannelID);
  free((void *)productInfoBlock.productVersion);
  return rv;
}

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
        return ARCHIVE_READER_MEM_ERROR;
    }
  }

  if (!outbuf) {
    outbuf = (char *)malloc(outbuf_size);
    if (!outbuf) {
      
      outbuf_size = 1024;
      outbuf = (char *)malloc(outbuf_size);
      if (!outbuf)
        return ARCHIVE_READER_MEM_ERROR;
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
    mArchive = nullptr;
  }

  if (inbuf) {
    free(inbuf);
    inbuf = nullptr;
  }

  if (outbuf) {
    free(outbuf);
    outbuf = nullptr;
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
    return UNEXPECTED_BZIP_ERROR;

  offset = 0;
  for (;;) {
    if (!item->length) {
      ret = UNEXPECTED_MAR_ERROR;
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
      ret = UNEXPECTED_BZIP_ERROR;
      break;
    }

    outlen = outbuf_size - strm.avail_out;
    if (outlen) {
      if (fwrite(outbuf, outlen, 1, fp) != 1) {
        ret = WRITE_ERROR_EXTRACT;
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
