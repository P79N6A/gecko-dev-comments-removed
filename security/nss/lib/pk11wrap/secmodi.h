






































#ifndef _SECMODI_H_
#define _SECMODI_H_ 1
#include "pkcs11.h"
#include "nssilock.h"
#include "mcom_db.h"
#include "secoidt.h"
#include "secdert.h"
#include "certt.h"
#include "secmodt.h"
#include "keyt.h"

SEC_BEGIN_PROTOS


extern SECStatus SECMOD_DeletePermDB(SECMODModule *module);
extern SECStatus SECMOD_AddPermDB(SECMODModule *module);
extern SECStatus SECMOD_Shutdown(void);
void nss_DumpModuleLog(void);

extern int secmod_PrivateModuleCount;

extern void SECMOD_Init(void);
SECStatus secmod_ModuleInit(SECMODModule *mod, PRBool* alreadyLoaded);


extern SECStatus SECMOD_AddModuleToList(SECMODModule *newModule);
extern SECStatus SECMOD_AddModuleToDBOnlyList(SECMODModule *newModule);
extern SECStatus SECMOD_AddModuleToUnloadList(SECMODModule *newModule);
extern void SECMOD_RemoveList(SECMODModuleList **,SECMODModuleList *);
extern void SECMOD_AddList(SECMODModuleList *,SECMODModuleList *,SECMODListLock *);
extern SECMODListLock *SECMOD_NewListLock(void);
extern void SECMOD_DestroyListLock(SECMODListLock *);
extern void SECMOD_GetWriteLock(SECMODListLock *);
extern void SECMOD_ReleaseWriteLock(SECMODListLock *);


extern SECMODModule *SECMOD_FindModuleByID(SECMODModuleID);


extern SECMODModuleList *SECMOD_NewModuleListElement(void);
extern SECMODModuleList *SECMOD_DestroyModuleListElement(SECMODModuleList *);
extern void SECMOD_DestroyModuleList(SECMODModuleList *);
extern SECStatus SECMOD_AddModule(SECMODModule *newModule);

extern unsigned long SECMOD_InternaltoPubMechFlags(unsigned long internalFlags);
extern unsigned long SECMOD_InternaltoPubCipherFlags(unsigned long internalFlags);


SECStatus SECMOD_LoadPKCS11Module(SECMODModule *);
SECStatus SECMOD_UnloadModule(SECMODModule *);
void SECMOD_SetInternalModule(SECMODModule *);

void SECMOD_SlotDestroyModule(SECMODModule *module, PRBool fromSlot);
CK_RV pk11_notify(CK_SESSION_HANDLE session, CK_NOTIFICATION event,
                                                         CK_VOID_PTR pdata);
void pk11_SignedToUnsigned(CK_ATTRIBUTE *attrib);
CK_OBJECT_HANDLE pk11_FindObjectByTemplate(PK11SlotInfo *slot,
					CK_ATTRIBUTE *inTemplate,int tsize);
CK_OBJECT_HANDLE *pk11_FindObjectsByTemplate(PK11SlotInfo *slot,
			CK_ATTRIBUTE *inTemplate,int tsize, int *objCount);
SECStatus PK11_UpdateSlotAttribute(PK11SlotInfo *slot,
				 PK11DefaultArrayEntry *entry, PRBool add);

#define PK11_GETTAB(x) ((CK_FUNCTION_LIST_PTR)((x)->functionList))
#define PK11_SETATTRS(x,id,v,l) (x)->type = (id); \
		(x)->pValue=(v); (x)->ulValueLen = (l);
SECStatus PK11_CreateNewObject(PK11SlotInfo *slot, CK_SESSION_HANDLE session,
                               const CK_ATTRIBUTE *theTemplate, int count,
                                PRBool token, CK_OBJECT_HANDLE *objectID);

SECStatus pbe_PK11AlgidToParam(SECAlgorithmID *algid,SECItem *mech);
SECStatus PBE_PK11ParamToAlgid(SECOidTag algTag, SECItem *param, 
				PRArenaPool *arena, SECAlgorithmID *algId);

PK11SymKey *pk11_TokenKeyGenWithFlagsAndKeyType(PK11SlotInfo *slot,
	CK_MECHANISM_TYPE type, SECItem *param, CK_KEY_TYPE keyType, 
	int keySize, SECItem *keyId, CK_FLAGS opFlags, 
	PK11AttrFlags attrFlags, void *wincx);

CK_MECHANISM_TYPE pk11_GetPBECryptoMechanism(SECAlgorithmID *algid,
                   SECItem **param, SECItem *pwd, PRBool faulty3DES);



extern void pk11sdr_Init(void);
extern void pk11sdr_Shutdown(void);





PRBool pk11_LoginStillRequired(PK11SlotInfo *slot, void *wincx);
CK_SESSION_HANDLE pk11_GetNewSession(PK11SlotInfo *slot, PRBool *owner);
void pk11_CloseSession(PK11SlotInfo *slot, CK_SESSION_HANDLE sess, PRBool own);
PK11SymKey *pk11_ForceSlot(PK11SymKey *symKey, CK_MECHANISM_TYPE type,
						CK_ATTRIBUTE_TYPE operation);

unsigned int pk11_OpFlagsToAttributes(CK_FLAGS flags, 
				CK_ATTRIBUTE *attrs, CK_BBOOL *ckTrue);

PRBool pk11_BadAttrFlags(PK11AttrFlags attrFlags);

unsigned int pk11_AttrFlagsToAttributes(PK11AttrFlags attrFlags,
		CK_ATTRIBUTE *attrs, CK_BBOOL *ckTrue, CK_BBOOL *ckFalse);
PRBool pk11_FindAttrInTemplate(CK_ATTRIBUTE *attr, unsigned int numAttrs,
					CK_ATTRIBUTE_TYPE target);

CK_MECHANISM_TYPE pk11_mapWrapKeyType(KeyType keyType);
PK11SymKey *pk11_KeyExchange(PK11SlotInfo *slot, CK_MECHANISM_TYPE type,
		CK_ATTRIBUTE_TYPE operation, CK_FLAGS flags, PRBool isPerm,
						PK11SymKey *symKey);

PRBool pk11_HandleTrustObject(PK11SlotInfo *slot, CERTCertificate *cert,
							 CERTCertTrust *trust);
CK_OBJECT_HANDLE pk11_FindPubKeyByAnyCert(CERTCertificate *cert,
					 PK11SlotInfo **slot, void *wincx);
SECStatus pk11_AuthenticateUnfriendly(PK11SlotInfo *slot, PRBool loadCerts,
							void *wincx);
int PK11_NumberObjectsFor(PK11SlotInfo *slot, CK_ATTRIBUTE *findTemplate,
						int templateCount);
SECItem *pk11_GetLowLevelKeyFromHandle(PK11SlotInfo *slot, 
						CK_OBJECT_HANDLE handle);
SECStatus PK11_TraverseSlot(PK11SlotInfo *slot, void *arg);
CK_OBJECT_HANDLE pk11_FindPrivateKeyFromCertID(PK11SlotInfo *slot, 
							SECItem *keyID);
SECKEYPrivateKey *PK11_MakePrivKey(PK11SlotInfo *slot, KeyType keyType, 
			PRBool isTemp, CK_OBJECT_HANDLE privID, void *wincx);
CERTCertificate *PK11_MakeCertFromHandle(PK11SlotInfo *slot,
			CK_OBJECT_HANDLE certID, CK_ATTRIBUTE *privateLabel);

SECItem *pk11_GenerateNewParamWithKeyLen(CK_MECHANISM_TYPE type, int keyLen);
SECItem *pk11_ParamFromIVWithLen(CK_MECHANISM_TYPE type, 
				 SECItem *iv, int keyLen);

SEC_END_PROTOS

#endif

