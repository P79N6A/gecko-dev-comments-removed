






























#ifndef bspatch_h__
#define bspatch_h__


#include "prtypes.h"

typedef struct MBSPatchHeader_ {
  
  char tag[8];
  
  
  PRUint32 slen;

  
  PRUint32 scrc32;

  
  PRUint32 dlen;

  
  PRUint32 cblen;

  
  PRUint32 difflen;

  
  PRUint32 extralen;

  
  
  
} MBSPatchHeader;







int MBS_ReadHeader(int fd, MBSPatchHeader *header);












int MBS_ApplyPatch(const MBSPatchHeader *header, int patchfd,
                   unsigned char *fbuffer, int filefd);

typedef struct MBSPatchTriple_ {
  PRUint32 x; 
  PRUint32 y; 
  PRInt32  z; 
} MBSPatchTriple;

#endif  
