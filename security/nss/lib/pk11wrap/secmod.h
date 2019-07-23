


































#ifndef _SECMOD_H_
#define _SEDMOD_H_
#include "seccomon.h"
#include "secmodt.h"
#include "prinrval.h"




#define PUBLIC_MECH_RSA_FLAG         0x00000001ul
#define PUBLIC_MECH_DSA_FLAG         0x00000002ul
#define PUBLIC_MECH_RC2_FLAG         0x00000004ul
#define PUBLIC_MECH_RC4_FLAG         0x00000008ul
#define PUBLIC_MECH_DES_FLAG         0x00000010ul
#define PUBLIC_MECH_DH_FLAG          0x00000020ul
#define PUBLIC_MECH_FORTEZZA_FLAG    0x00000040ul
#define PUBLIC_MECH_RC5_FLAG         0x00000080ul
#define PUBLIC_MECH_SHA1_FLAG        0x00000100ul
#define PUBLIC_MECH_MD5_FLAG         0x00000200ul
#define PUBLIC_MECH_MD2_FLAG         0x00000400ul
#define PUBLIC_MECH_SSL_FLAG         0x00000800ul
#define PUBLIC_MECH_TLS_FLAG         0x00001000ul
#define PUBLIC_MECH_AES_FLAG         0x00002000ul
#define PUBLIC_MECH_SHA256_FLAG      0x00004000ul
#define PUBLIC_MECH_SHA512_FLAG      0x00008000ul
#define PUBLIC_MECH_CAMELLIA_FLAG    0x00010000ul
#define PUBLIC_MECH_SEED_FLAG        0x00020000ul

#define PUBLIC_MECH_RANDOM_FLAG      0x08000000ul
#define PUBLIC_MECH_FRIENDLY_FLAG    0x10000000ul
#define PUBLIC_OWN_PW_DEFAULTS       0X20000000ul
#define PUBLIC_DISABLE_FLAG          0x40000000ul


#define PUBLIC_MECH_RESERVED_FLAGS   0x87FF0000ul




#define PUBLIC_CIPHER_FORTEZZA_FLAG  0x00000001ul


#define PUBLIC_CIPHER_RESERVED_FLAGS 0xFFFFFFFEul

SEC_BEGIN_PROTOS







extern SECMODModule *SECMOD_LoadModule(char *moduleSpec,SECMODModule *parent,
							PRBool recurse);

extern SECMODModule *SECMOD_LoadUserModule(char *moduleSpec,SECMODModule *parent,
							PRBool recurse);

SECStatus SECMOD_UnloadUserModule(SECMODModule *mod);

SECMODModule * SECMOD_CreateModule(const char *lib, const char *name,
					const char *param, const char *nss);



char **SECMOD_GetModuleSpecList(SECMODModule *module);
SECStatus SECMOD_FreeModuleSpecList(SECMODModule *module,char **moduleSpecList);

 


extern SECMODModuleList *SECMOD_GetDefaultModuleList(void); 

extern SECMODModuleList *SECMOD_GetDeadModuleList(void);

extern SECMODModuleList *SECMOD_GetDBModuleList(void);


extern SECMODListLock *SECMOD_GetDefaultModuleListLock(void);

extern SECStatus SECMOD_UpdateModule(SECMODModule *module);


extern void SECMOD_GetReadLock(SECMODListLock *);
extern void SECMOD_ReleaseReadLock(SECMODListLock *);


extern SECMODModule *SECMOD_FindModule(const char *name);
extern SECStatus SECMOD_DeleteModule(const char *name, int *type);
extern SECStatus SECMOD_DeleteModuleEx(const char * name, 
                                       SECMODModule *mod, 
                                       int *type, 
                                       PRBool permdb);
extern SECStatus SECMOD_DeleteInternalModule(const char *name);
extern PRBool SECMOD_CanDeleteInternalModule(void);
extern SECStatus SECMOD_AddNewModule(const char* moduleName, 
			      const char* dllPath,
                              unsigned long defaultMechanismFlags,
                              unsigned long cipherEnableFlags);
extern SECStatus SECMOD_AddNewModuleEx(const char* moduleName,
			      const char* dllPath,
                              unsigned long defaultMechanismFlags,
                              unsigned long cipherEnableFlags,
                              char* modparms,
                              char* nssparms);


extern SECMODModule *SECMOD_GetInternalModule(void);
extern SECMODModule *SECMOD_ReferenceModule(SECMODModule *module);
extern void SECMOD_DestroyModule(SECMODModule *module);
extern PK11SlotInfo *SECMOD_LookupSlot(SECMODModuleID module,
							unsigned long slotID);
extern PK11SlotInfo *SECMOD_FindSlot(SECMODModule *module,const char *name);



PRBool SECMOD_IsModulePresent( unsigned long int pubCipherEnableFlags );



extern unsigned long SECMOD_PubMechFlagstoInternal(unsigned long publicFlags);
extern unsigned long SECMOD_PubCipherFlagstoInternal(unsigned long publicFlags);

PRBool SECMOD_HasRemovableSlots(SECMODModule *mod);
PK11SlotInfo *SECMOD_WaitForAnyTokenEvent(SECMODModule *mod, 
				unsigned long flags, PRIntervalTime latency);






SECStatus SECMOD_CancelWait(SECMODModule *mod);








SECStatus SECMOD_UpdateSlotList(SECMODModule *mod);
SEC_END_PROTOS

#endif
