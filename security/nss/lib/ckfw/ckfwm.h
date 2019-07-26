



#ifndef CKFWM_H
#define CKFWM_H

#ifdef DEBUG
static const char CKFWM_CVS_ID[] = "@(#) $RCSfile: ckfwm.h,v $ $Revision: 1.7 $ $Date: 2012/04/25 14:49:28 $";
#endif 







#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

#ifndef NSSCKT_H
#include "nssckt.h"
#endif 

#ifndef NSSCKFWT_H
#include "nssckfwt.h"
#endif 


















NSS_EXTERN nssCKFWHash *
nssCKFWHash_Create
(
  NSSCKFWInstance *fwInstance,
  NSSArena *arena,
  CK_RV *pError
);





NSS_EXTERN void
nssCKFWHash_Destroy
(
  nssCKFWHash *hash
);





NSS_EXTERN CK_RV
nssCKFWHash_Add
(
  nssCKFWHash *hash,
  const void *key,
  const void *value
);





NSS_EXTERN void
nssCKFWHash_Remove
(
  nssCKFWHash *hash,
  const void *it
);





NSS_EXTERN CK_ULONG
nssCKFWHash_Count
(
  nssCKFWHash *hash
);





NSS_EXTERN CK_BBOOL
nssCKFWHash_Exists
(
  nssCKFWHash *hash,
  const void *it
);





NSS_EXTERN void *
nssCKFWHash_Lookup
(
  nssCKFWHash *hash,
  const void *it
);





NSS_EXTERN void
nssCKFWHash_Iterate
(
  nssCKFWHash *hash,
  nssCKFWHashIterator fcn,
  void *closure
);

#endif 
