



#ifndef NSSDEV_H
#define NSSDEV_H







#ifndef NSSDEVT_H
#include "nssdevt.h"
#endif 

PR_BEGIN_EXTERN_C







NSS_EXTERN NSSAlgorithmAndParameters *
NSSAlgorithmAndParameters_CreateSHA1Digest
(
  NSSArena *arenaOpt
);

NSS_EXTERN NSSAlgorithmAndParameters *
NSSAlgorithmAndParameters_CreateMD5Digest
(
  NSSArena *arenaOpt
);

PR_END_EXTERN_C

#endif 
