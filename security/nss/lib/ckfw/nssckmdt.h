



#ifndef NSSCKMDT_H
#define NSSCKMDT_H

#ifdef DEBUG
static const char NSSCKMDT_CVS_ID[] = "@(#) $RCSfile: nssckmdt.h,v $ $Revision: 1.7 $ $Date: 2012/04/25 14:49:28 $";
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

typedef struct NSSCKMDInstanceStr NSSCKMDInstance;
typedef struct NSSCKMDSlotStr NSSCKMDSlot;
typedef struct NSSCKMDTokenStr NSSCKMDToken;
typedef struct NSSCKMDSessionStr NSSCKMDSession;
typedef struct NSSCKMDCryptoOperationStr NSSCKMDCryptoOperation;
typedef struct NSSCKMDFindObjectsStr NSSCKMDFindObjects;
typedef struct NSSCKMDMechanismStr NSSCKMDMechanism;
typedef struct NSSCKMDObjectStr NSSCKMDObject;











typedef struct {
  PRBool needsFreeing;
  NSSItem* item;
} NSSCKFWItem ;













struct NSSCKMDInstanceStr {
  


  void *etc;

  





  CK_RV (PR_CALLBACK *Initialize)(
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance,
    NSSUTF8 *configurationData
  );

  





  void (PR_CALLBACK *Finalize)(
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance
  );

  




  CK_ULONG (PR_CALLBACK *GetNSlots)(
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  





  CK_VERSION (PR_CALLBACK *GetCryptokiVersion)(
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance
  );

  










  NSSUTF8 *(PR_CALLBACK *GetManufacturerID)(
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  










  NSSUTF8 *(PR_CALLBACK *GetLibraryDescription)(
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  




  CK_VERSION (PR_CALLBACK *GetLibraryVersion)(
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance
  );

  






  CK_BBOOL (PR_CALLBACK *ModuleHandlesSessionObjects)(
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance
  );

  






  CK_RV (PR_CALLBACK *GetSlots)(
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance,
    NSSCKMDSlot *slots[]
  );

  









  NSSCKMDSlot *(PR_CALLBACK *WaitForSlotEvent)(
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance,
    CK_BBOOL block,
    CK_RV *pError
  );

  





  void *null;
};














struct NSSCKMDSlotStr {
  


  void *etc;

  








  CK_RV (PR_CALLBACK *Initialize)(
    NSSCKMDSlot *mdSlot,
    NSSCKFWSlot *fwSlot,
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance
  );

  









  void (PR_CALLBACK *Destroy)(
    NSSCKMDSlot *mdSlot,
    NSSCKFWSlot *fwSlot,
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance
  );

  










  NSSUTF8 *(PR_CALLBACK *GetSlotDescription)(
    NSSCKMDSlot *mdSlot,
    NSSCKFWSlot *fwSlot,
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  










  NSSUTF8 *(PR_CALLBACK *GetManufacturerID)(
    NSSCKMDSlot *mdSlot,
    NSSCKFWSlot *fwSlot,
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  




  CK_BBOOL (PR_CALLBACK *GetTokenPresent)(
    NSSCKMDSlot *mdSlot,
    NSSCKFWSlot *fwSlot,
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance
  );

  




  CK_BBOOL (PR_CALLBACK *GetRemovableDevice)(
    NSSCKMDSlot *mdSlot,
    NSSCKFWSlot *fwSlot,
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance
  );

  




  CK_BBOOL (PR_CALLBACK *GetHardwareSlot)(
    NSSCKMDSlot *mdSlot,
    NSSCKFWSlot *fwSlot,
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance
  );

  




  CK_VERSION (PR_CALLBACK *GetHardwareVersion)(
    NSSCKMDSlot *mdSlot,
    NSSCKFWSlot *fwSlot,
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance
  );

  




  CK_VERSION (PR_CALLBACK *GetFirmwareVersion)(
    NSSCKMDSlot *mdSlot,
    NSSCKFWSlot *fwSlot,
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance
  );

  







  NSSCKMDToken *(PR_CALLBACK *GetToken)(
    NSSCKMDSlot *mdSlot,
    NSSCKFWSlot *fwSlot,
    NSSCKMDInstance *mdInstance,                                    
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  





  void *null;
};













struct NSSCKMDTokenStr {
  


  void *etc;

  








  CK_RV (PR_CALLBACK *Setup)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  







  void (PR_CALLBACK *Invalidate)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  





  CK_RV (PR_CALLBACK *InitToken)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSItem *pin,
    NSSUTF8 *label
  );

  










  NSSUTF8 *(PR_CALLBACK *GetLabel)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  










  NSSUTF8 *(PR_CALLBACK *GetManufacturerID)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  










  NSSUTF8 *(PR_CALLBACK *GetModel)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  










  NSSUTF8 *(PR_CALLBACK *GetSerialNumber)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  




  CK_BBOOL (PR_CALLBACK *GetHasRNG)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_BBOOL (PR_CALLBACK *GetIsWriteProtected)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_BBOOL (PR_CALLBACK *GetLoginRequired)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_BBOOL (PR_CALLBACK *GetUserPinInitialized)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  





  CK_BBOOL (PR_CALLBACK *GetRestoreKeyNotNeeded)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_BBOOL (PR_CALLBACK *GetHasClockOnToken)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_BBOOL (PR_CALLBACK *GetHasProtectedAuthenticationPath)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_BBOOL (PR_CALLBACK *GetSupportsDualCryptoOperations)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  





  CK_ULONG (PR_CALLBACK *GetMaxSessionCount)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  






  CK_ULONG (PR_CALLBACK *GetMaxRwSessionCount)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  





  CK_ULONG (PR_CALLBACK *GetMaxPinLen)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  





  CK_ULONG (PR_CALLBACK *GetMinPinLen)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  





  CK_ULONG (PR_CALLBACK *GetTotalPublicMemory)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  





  CK_ULONG (PR_CALLBACK *GetFreePublicMemory)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  





  CK_ULONG (PR_CALLBACK *GetTotalPrivateMemory)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  





  CK_ULONG (PR_CALLBACK *GetFreePrivateMemory)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_VERSION (PR_CALLBACK *GetHardwareVersion)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_VERSION (PR_CALLBACK *GetFirmwareVersion)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  






  CK_RV (PR_CALLBACK *GetUTCTime)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_CHAR utcTime[16]
  );

  







  NSSCKMDSession *(PR_CALLBACK *OpenSession)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKFWSession *fwSession,
    CK_BBOOL rw,
    CK_RV *pError
  );

  




  CK_ULONG (PR_CALLBACK *GetMechanismCount)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_RV (PR_CALLBACK *GetMechanismTypes)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_MECHANISM_TYPE types[]
  );

  





  NSSCKMDMechanism *(PR_CALLBACK *GetMechanism)(
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_MECHANISM_TYPE which,
    CK_RV *pError
  );

  





  void *null;
};













struct NSSCKMDSessionStr {
  


  void *etc;

  






  void (PR_CALLBACK *Close)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  



  CK_ULONG (PR_CALLBACK *GetDeviceError)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_RV (PR_CALLBACK *Login)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_USER_TYPE userType,
    NSSItem *pin,
    CK_STATE oldState,
    CK_STATE newState
  );

  




  CK_RV (PR_CALLBACK *Logout)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_STATE oldState,
    CK_STATE newState
  );

  







  CK_RV (PR_CALLBACK *InitPIN)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSItem *pin
  );

  







  CK_RV (PR_CALLBACK *SetPIN)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSItem *oldPin,
    NSSItem *newPin
  );

  






  CK_ULONG (PR_CALLBACK *GetOperationStateLen)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  







  CK_RV (PR_CALLBACK *GetOperationState)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSItem *buffer
  );

  








  CK_RV (PR_CALLBACK *SetOperationState)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSItem *state,
    NSSCKMDObject *mdEncryptionKey,
    NSSCKFWObject *fwEncryptionKey,
    NSSCKMDObject *mdAuthenticationKey,
    NSSCKFWObject *fwAuthenticationKey
  );

  












  NSSCKMDObject *(PR_CALLBACK *CreateObject)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSArena *handyArenaPointer,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulAttributeCount,
    CK_RV *pError
  );

  










  NSSCKMDObject *(PR_CALLBACK *CopyObject)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdOldObject,
    NSSCKFWObject *fwOldObject,
    NSSArena *handyArenaPointer,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulAttributeCount,
    CK_RV *pError
  );

  









  NSSCKMDFindObjects *(PR_CALLBACK *FindObjectsInit)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulAttributeCount,
    CK_RV *pError
  );

  




  CK_RV (PR_CALLBACK *SeedRandom)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSItem *seed
  );

  



  CK_RV (PR_CALLBACK *GetRandom)(
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSItem *buffer
  );

  





  void *null;
};













struct NSSCKMDFindObjectsStr {
  


  void *etc;

  





  void (PR_CALLBACK *Final)(
    NSSCKMDFindObjects *mdFindObjects,
    NSSCKFWFindObjects *fwFindObjects,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  





  NSSCKMDObject *(PR_CALLBACK *Next)(
    NSSCKMDFindObjects *mdFindObjects,
    NSSCKFWFindObjects *fwFindObjects,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSArena *arena,
    CK_RV *pError
  );

  





  void *null;
};














struct NSSCKMDCryptoOperationStr {
  


  void *etc;

  




  void (PR_CALLBACK *Destroy)(
    NSSCKMDCryptoOperation *mdCryptoOperation,
    NSSCKFWCryptoOperation *fwCryptoOperation,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );


  



  CK_ULONG (PR_CALLBACK *GetFinalLength)(
    NSSCKMDCryptoOperation *mdCryptoOperation,
    NSSCKFWCryptoOperation *fwCryptoOperation,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  



  CK_ULONG (PR_CALLBACK *GetOperationLength)(
    NSSCKMDCryptoOperation *mdCryptoOperation,
    NSSCKFWCryptoOperation *fwCryptoOperation,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    const NSSItem   *inputBuffer,
    CK_RV *pError
  );

  







  CK_RV(PR_CALLBACK *Final)(
    NSSCKMDCryptoOperation *mdCryptoOperation,
    NSSCKFWCryptoOperation *fwCryptoOperation,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSItem       *outputBuffer
  );


  







  CK_RV(PR_CALLBACK *Update)(
    NSSCKMDCryptoOperation *mdCryptoOperation,
    NSSCKFWCryptoOperation *fwCryptoOperation,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    const NSSItem   *inputBuffer,
    NSSItem   *outputBuffer
  );

  







  CK_RV(PR_CALLBACK *DigestUpdate)(
    NSSCKMDCryptoOperation *mdCryptoOperation,
    NSSCKFWCryptoOperation *fwCryptoOperation,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    const NSSItem   *inputBuffer
  );

  





  CK_RV(PR_CALLBACK *UpdateFinal)(
    NSSCKMDCryptoOperation *mdCryptoOperation,
    NSSCKFWCryptoOperation *fwCryptoOperation,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    const NSSItem   *inputBuffer,
    NSSItem   *outputBuffer
  );

  







  CK_RV(PR_CALLBACK *UpdateCombo)(
    NSSCKMDCryptoOperation *mdCryptoOperation,
    NSSCKFWCryptoOperation *fwCryptoOperation,
    NSSCKMDCryptoOperation *mdPeerCryptoOperation,
    NSSCKFWCryptoOperation *fwPeerCryptoOperation,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    const NSSItem   *inputBuffer,
    NSSItem   *outputBuffer
  );

  


  CK_RV(PR_CALLBACK *DigestKey)(
    NSSCKMDCryptoOperation *mdCryptoOperation,
    NSSCKFWCryptoOperation *fwCryptoOperation,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdKey,
    NSSCKFWObject *fwKey
  );

  





  void *null;
};






struct NSSCKMDMechanismStr {
  


  void *etc;

  




  void (PR_CALLBACK *Destroy)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );


  






  CK_ULONG (PR_CALLBACK *GetMinKeySize)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  






  CK_ULONG (PR_CALLBACK *GetMaxKeySize)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  




  CK_BBOOL (PR_CALLBACK *GetInHardware)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  







































  

  NSSCKMDCryptoOperation * (PR_CALLBACK *EncryptInit)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdKey,
    NSSCKFWObject *fwKey,
    CK_RV *pError
  );

  

  NSSCKMDCryptoOperation * (PR_CALLBACK *DecryptInit)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdKey,
    NSSCKFWObject *fwKey,
    CK_RV *pError
  );

  

  NSSCKMDCryptoOperation * (PR_CALLBACK *DigestInit)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );


  

  NSSCKMDCryptoOperation * (PR_CALLBACK *SignInit)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdKey,
    NSSCKFWObject *fwKey,
    CK_RV *pError
  );

  

  NSSCKMDCryptoOperation * (PR_CALLBACK *VerifyInit)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdKey,
    NSSCKFWObject *fwKey,
    CK_RV *pError
  );

  

  NSSCKMDCryptoOperation * (PR_CALLBACK *SignRecoverInit)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdKey,
    NSSCKFWObject *fwKey,
    CK_RV *pError
  );

  

  NSSCKMDCryptoOperation * (PR_CALLBACK *VerifyRecoverInit)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdKey,
    NSSCKFWObject *fwKey,
    CK_RV *pError
  );

  



  



  NSSCKMDObject *(PR_CALLBACK *GenerateKey)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulAttributeCount,
    CK_RV *pError
  );

  


  CK_RV (PR_CALLBACK *GenerateKeyPair)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_ATTRIBUTE_PTR pPublicKeyTemplate,
    CK_ULONG ulPublicKeyAttributeCount,
    CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
    CK_ULONG ulPrivateKeyAttributeCount,
    NSSCKMDObject **pPublicKey,
    NSSCKMDObject **pPrivateKey
  );

  


  CK_ULONG (PR_CALLBACK *GetWrapKeyLength)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdWrappingKey,
    NSSCKFWObject *fwWrappingKey,
    NSSCKMDObject *mdWrappedKey,
    NSSCKFWObject *fwWrappedKey,
    CK_RV *pError
  );

  


  CK_RV (PR_CALLBACK *WrapKey)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdWrappingKey,
    NSSCKFWObject *fwWrappingKey,
    NSSCKMDObject *mdKeyObject,
    NSSCKFWObject *fwKeyObject,
    NSSItem *wrappedKey
  );

  



  NSSCKMDObject *(PR_CALLBACK *UnwrapKey)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdWrappingKey,
    NSSCKFWObject *fwWrappingKey,
    NSSItem *wrappedKey,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulAttributeCount,
    CK_RV *pError
  );    
    
  



  NSSCKMDObject *(PR_CALLBACK *DeriveKey)(
    NSSCKMDMechanism *mdMechanism,
    NSSCKFWMechanism *fwMechanism,
    CK_MECHANISM_PTR  pMechanism,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    NSSCKMDObject *mdBaseKey,
    NSSCKFWObject *fwBaseKey,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulAttributeCount,
    CK_RV *pError
  );    

  





  void *null;
};












struct NSSCKMDObjectStr {
  


  void *etc;

  





  void (PR_CALLBACK *Finalize)(
    NSSCKMDObject *mdObject,
    NSSCKFWObject *fwObject,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_RV (PR_CALLBACK *Destroy)(
    NSSCKMDObject *mdObject,
    NSSCKFWObject *fwObject,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  







  CK_BBOOL (PR_CALLBACK *IsTokenObject)(
    NSSCKMDObject *mdObject,
    NSSCKFWObject *fwObject,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance
  );

  




  CK_ULONG (PR_CALLBACK *GetAttributeCount)(
    NSSCKMDObject *mdObject,
    NSSCKFWObject *fwObject,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  





  CK_RV (PR_CALLBACK *GetAttributeTypes)(
    NSSCKMDObject *mdObject,
    NSSCKFWObject *fwObject,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_ATTRIBUTE_TYPE_PTR typeArray,
    CK_ULONG ulCount
  );

  



  CK_ULONG (PR_CALLBACK *GetAttributeSize)(
    NSSCKMDObject *mdObject,
    NSSCKFWObject *fwObject,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_ATTRIBUTE_TYPE attribute,
    CK_RV *pError
  );

  






  NSSCKFWItem (PR_CALLBACK *GetAttribute)(
    NSSCKMDObject *mdObject,
    NSSCKFWObject *fwObject,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_ATTRIBUTE_TYPE attribute,
    CK_RV *pError
  );

  


  CK_RV (PR_CALLBACK *FreeAttribute)(
    NSSCKFWItem * item
  );

  



  CK_RV (PR_CALLBACK *SetAttribute)(
    NSSCKMDObject *mdObject,
    NSSCKFWObject *fwObject,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_ATTRIBUTE_TYPE attribute,
    NSSItem *value
  );

  









  CK_ULONG (PR_CALLBACK *GetObjectSize)(
    NSSCKMDObject *mdObject,
    NSSCKFWObject *fwObject,
    NSSCKMDSession *mdSession,
    NSSCKFWSession *fwSession,
    NSSCKMDToken *mdToken,
    NSSCKFWToken *fwToken,
    NSSCKMDInstance *mdInstance,
    NSSCKFWInstance *fwInstance,
    CK_RV *pError
  );

  





  void *null;
};


#endif 
