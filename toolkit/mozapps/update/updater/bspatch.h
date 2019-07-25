






























#ifndef bspatch_h__
#define bspatch_h__

#include "mozilla/StandardInteger.h"
#include <stdio.h>

typedef struct MBSPatchHeader_ {
  
  char tag[8];
  
  
  uint32_t slen;

  
  uint32_t scrc32;

  
  uint32_t dlen;

  
  uint32_t cblen;

  
  uint32_t difflen;

  
  uint32_t extralen;

  
  
  
} MBSPatchHeader;







int MBS_ReadHeader(FILE* file, MBSPatchHeader *header);












int MBS_ApplyPatch(const MBSPatchHeader *header, FILE* patchFile,
                   unsigned char *fbuffer, FILE* file);

typedef struct MBSPatchTriple_ {
  uint32_t x; 
  uint32_t y; 
  int32_t  z; 
} MBSPatchTriple;

#endif  
