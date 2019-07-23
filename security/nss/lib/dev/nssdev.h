



































#ifndef NSSDEV_H
#define NSSDEV_H

#ifdef DEBUG
static const char NSSDEV_CVS_ID[] = "@(#) $RCSfile: nssdev.h,v $ $Revision: 1.3 $ $Date: 2005/01/20 02:25:47 $";
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
