






























#include "bspatch.h"
#include "errors.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>

#if defined(XP_WIN)
# include <io.h>
#else
# include <unistd.h>
#endif

#ifdef XP_WIN
# include <winsock2.h>
#else
# include <arpa/inet.h>
#endif

#ifndef SSIZE_MAX
# define SSIZE_MAX LONG_MAX
#endif

int
MBS_ReadHeader(FILE* file, MBSPatchHeader *header)
{
  size_t s = fread(header, 1, sizeof(MBSPatchHeader), file);
  if (s != sizeof(MBSPatchHeader))
    return READ_ERROR;

  header->slen      = ntohl(header->slen);
  header->scrc32    = ntohl(header->scrc32);
  header->dlen      = ntohl(header->dlen);
  header->cblen     = ntohl(header->cblen);
  header->difflen   = ntohl(header->difflen);
  header->extralen  = ntohl(header->extralen);

  struct stat hs;
  s = fstat(fileno(file), &hs);
  if (s)
    return READ_ERROR;

  if (memcmp(header->tag, "MBDIFF10", 8) != 0)
    return UNEXPECTED_ERROR;

  if (sizeof(MBSPatchHeader) +
      header->cblen +
      header->difflen +
      header->extralen != PRUint32(hs.st_size))
    return UNEXPECTED_ERROR;

  return OK;
}
         
int
MBS_ApplyPatch(const MBSPatchHeader *header, FILE* patchFile,
               unsigned char *fbuffer, FILE* file)
{
  unsigned char *fbufend = fbuffer + header->slen;

  unsigned char *buf = (unsigned char*) malloc(header->cblen +
                                               header->difflen +
                                               header->extralen);
  if (!buf)
    return MEM_ERROR;

  int rv = OK;

  size_t r = header->cblen + header->difflen + header->extralen;
  unsigned char *wb = buf;
  while (r) {
    size_t c = fread(wb, 1, (r > SSIZE_MAX) ? SSIZE_MAX : r, patchFile);
    if (c < 0) {
      rv = READ_ERROR;
      goto end;
    }

    r -= c;

    if (c == 0 && r) {
      rv = UNEXPECTED_ERROR;
      goto end;
    }
  }

  {
    MBSPatchTriple *ctrlsrc = (MBSPatchTriple*) buf;
    unsigned char *diffsrc = buf + header->cblen;
    unsigned char *extrasrc = diffsrc + header->difflen;

    MBSPatchTriple *ctrlend = (MBSPatchTriple*) diffsrc;
    unsigned char *diffend = extrasrc;
    unsigned char *extraend = extrasrc + header->extralen;

    do {
      ctrlsrc->x = ntohl(ctrlsrc->x);
      ctrlsrc->y = ntohl(ctrlsrc->y);
      ctrlsrc->z = ntohl(ctrlsrc->z);

#ifdef DEBUG_bsmedberg
      printf("Applying block:\n"
             " x: %u\n"
             " y: %u\n"
             " z: %i\n",
             ctrlsrc->x,
             ctrlsrc->y,
             ctrlsrc->z);
#endif

      

      if (fbuffer + ctrlsrc->x > fbufend ||
          diffsrc + ctrlsrc->x > diffend) {
        rv = UNEXPECTED_ERROR;
        goto end;
      }
      for (PRUint32 i = 0; i < ctrlsrc->x; ++i) {
        diffsrc[i] += fbuffer[i];
      }
      if ((PRUint32) fwrite(diffsrc, 1, ctrlsrc->x, file) != ctrlsrc->x) {
        rv = WRITE_ERROR;
        goto end;
      }
      fbuffer += ctrlsrc->x;
      diffsrc += ctrlsrc->x;

      

      if (extrasrc + ctrlsrc->y > extraend) {
        rv = UNEXPECTED_ERROR;
        goto end;
      }
      if ((PRUint32) fwrite(extrasrc, 1, ctrlsrc->y, file) != ctrlsrc->y) {
        rv = WRITE_ERROR;
        goto end;
      }
      extrasrc += ctrlsrc->y;

      

      if (fbuffer + ctrlsrc->z > fbufend) {
        rv = UNEXPECTED_ERROR;
        goto end;
      }
      fbuffer += ctrlsrc->z;

      

      ++ctrlsrc;
    } while (ctrlsrc < ctrlend);
  }

end:
  free(buf);
  return rv;
}
