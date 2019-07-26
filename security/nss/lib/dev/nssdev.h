



#ifndef NSSDEV_H
#define NSSDEV_H

#ifdef DEBUG
static const char NSSDEV_CVS_ID[] = "@(#) $RCSfile: nssdev.h,v $ $Revision: 1.4 $ $Date: 2012/04/25 14:49:42 $";
#endif 






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
