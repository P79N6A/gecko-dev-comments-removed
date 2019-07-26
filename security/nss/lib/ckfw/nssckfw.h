



#ifndef NSSCKFW_H
#define NSSCKFW_H

#ifdef DEBUG
static const char NSSCKFW_CVS_ID[] = "@(#) $RCSfile: nssckfw.h,v $ $Revision: 1.6 $ $Date: 2012/04/25 14:49:28 $";
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
















NSS_EXTERN NSSCKMDInstance *
NSSCKFWInstance_GetMDInstance
(
  NSSCKFWInstance *fwInstance
);






NSS_EXTERN NSSArena *
NSSCKFWInstance_GetArena
(
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
);






NSS_EXTERN CK_BBOOL
NSSCKFWInstance_MayCreatePthreads
(
  NSSCKFWInstance *fwInstance
);






NSS_EXTERN NSSCKFWMutex *
NSSCKFWInstance_CreateMutex
(
  NSSCKFWInstance *fwInstance,
  NSSArena *arena,
  CK_RV *pError
);






NSS_EXTERN NSSUTF8 *
NSSCKFWInstance_GetConfigurationData
(
  NSSCKFWInstance *fwInstance
);






NSS_EXTERN CK_C_INITIALIZE_ARGS_PTR
NSSCKFWInstance_GetInitArgs
(
  NSSCKFWInstance *fwInstance
);















NSS_EXTERN NSSCKMDSlot *
NSSCKFWSlot_GetMDSlot
(
  NSSCKFWSlot *fwSlot
);






NSS_EXTERN NSSCKFWInstance *
NSSCKFWSlot_GetFWInstance
(
  NSSCKFWSlot *fwSlot
);






NSS_EXTERN NSSCKMDInstance *
NSSCKFWSlot_GetMDInstance
(
  NSSCKFWSlot *fwSlot
);
















NSS_EXTERN NSSCKMDToken *
NSSCKFWToken_GetMDToken
(
  NSSCKFWToken *fwToken
);






NSS_EXTERN NSSArena *
NSSCKFWToken_GetArena
(
  NSSCKFWToken *fwToken,
  CK_RV *pError
);






NSS_EXTERN NSSCKFWSlot *
NSSCKFWToken_GetFWSlot
(
  NSSCKFWToken *fwToken
);






NSS_EXTERN NSSCKMDSlot *
NSSCKFWToken_GetMDSlot
(
  NSSCKFWToken *fwToken
);






NSS_EXTERN CK_STATE
NSSCKFWToken_GetSessionState
(
  NSSCKFWToken *fwToken
);














NSS_EXTERN NSSCKMDMechanism *
NSSCKFWMechanism_GetMDMechanism
(
  NSSCKFWMechanism *fwMechanism
);






NSS_EXTERN NSSItem *
NSSCKFWMechanism_GetParameter
(
  NSSCKFWMechanism *fwMechanism
);


















NSS_EXTERN NSSCKMDSession *
NSSCKFWSession_GetMDSession
(
  NSSCKFWSession *fwSession
);






NSS_EXTERN NSSArena *
NSSCKFWSession_GetArena
(
  NSSCKFWSession *fwSession,
  CK_RV *pError
);






NSS_EXTERN CK_RV
NSSCKFWSession_CallNotification
(
  NSSCKFWSession *fwSession,
  CK_NOTIFICATION event
);






NSS_EXTERN CK_BBOOL
NSSCKFWSession_IsRWSession
(
  NSSCKFWSession *fwSession
);






NSS_EXTERN CK_BBOOL
NSSCKFWSession_IsSO
(
  NSSCKFWSession *fwSession
);






NSS_EXTERN NSSCKFWCryptoOperation *
NSSCKFWSession_GetCurrentCryptoOperation
(
  NSSCKFWSession *fwSession,
  NSSCKFWCryptoOperationState state
);


















NSS_EXTERN NSSCKMDObject *
NSSCKFWObject_GetMDObject
(
  NSSCKFWObject *fwObject
);





NSS_EXTERN NSSArena *
NSSCKFWObject_GetArena
(
  NSSCKFWObject *fwObject,
  CK_RV *pError
);





NSS_EXTERN CK_BBOOL
NSSCKFWObject_IsTokenObject
(
  NSSCKFWObject *fwObject
);





NSS_EXTERN CK_ULONG
NSSCKFWObject_GetAttributeCount
(
  NSSCKFWObject *fwObject,
  CK_RV *pError
);





NSS_EXTERN CK_RV
NSSCKFWObject_GetAttributeTypes
(
  NSSCKFWObject *fwObject,
  CK_ATTRIBUTE_TYPE_PTR typeArray,
  CK_ULONG ulCount
);





NSS_EXTERN CK_ULONG
NSSCKFWObject_GetAttributeSize
(
  NSSCKFWObject *fwObject,
  CK_ATTRIBUTE_TYPE attribute,
  CK_RV *pError
);





NSS_EXTERN NSSItem *
NSSCKFWObject_GetAttribute
(
  NSSCKFWObject *fwObject,
  CK_ATTRIBUTE_TYPE attribute,
  NSSItem *itemOpt,
  NSSArena *arenaOpt,
  CK_RV *pError
);





NSS_EXTERN CK_ULONG
NSSCKFWObject_GetObjectSize
(
  NSSCKFWObject *fwObject,
  CK_RV *pError
);













NSS_EXTERN NSSCKMDFindObjects *
NSSCKFWFindObjects_GetMDFindObjects
(
  NSSCKFWFindObjects *
);















NSS_EXTERN CK_RV
NSSCKFWMutex_Destroy
(
  NSSCKFWMutex *mutex
);






NSS_EXTERN CK_RV
NSSCKFWMutex_Lock
(
  NSSCKFWMutex *mutex
);






NSS_EXTERN CK_RV
NSSCKFWMutex_Unlock
(
  NSSCKFWMutex *mutex
);

#endif 

