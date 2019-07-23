





































#ifndef _PKCS11I_H_
#define _PKCS11I_H_ 1

#include "nssilock.h"
#include "seccomon.h"
#include "secoidt.h"
#include "lowkeyti.h"
#include "pkcs11t.h"

#include "sftkdbt.h"
















#define MAX_OBJS_ATTRS 45	/* number of attributes to preallocate in
				 * the object (must me the absolute max) */
#define ATTR_SPACE 50  		/* Maximum size of attribute data before extra
				 * data needs to be allocated. This is set to
				 * enough space to hold an SSL MASTER secret */

#define NSC_STRICT      PR_FALSE  /* forces the code to do strict template
				   * matching when doing C_FindObject on token
				   * objects. This will slow down search in
				   * NSS. */

#define NSC_CERT_BLOCK_SIZE     50
#define NSC_SEARCH_BLOCK_SIZE   5 
#define NSC_SLOT_LIST_BLOCK_SIZE 10

#define NSC_FIPS_MODULE 1
#define NSC_NON_FIPS_MODULE 0









#define SPACE_ATTRIBUTE_HASH_SIZE 32 
#define SPACE_SESSION_OBJECT_HASH_SIZE 32
#define SPACE_SESSION_HASH_SIZE 32
#define TIME_ATTRIBUTE_HASH_SIZE 32
#define TIME_SESSION_OBJECT_HASH_SIZE 1024
#define TIME_SESSION_HASH_SIZE 1024
#define MAX_OBJECT_LIST_SIZE 800  
				  

#define MAX_KEY_LEN 256 	  /* maximum symmetric key length in bytes */

#define MULTIACCESS "multiaccess:"

















#define LOG2_BUCKETS_PER_SESSION_LOCK 1
#define BUCKETS_PER_SESSION_LOCK (1 << (LOG2_BUCKETS_PER_SESSION_LOCK))



typedef struct SFTKAttributeStr SFTKAttribute;
typedef struct SFTKObjectListStr SFTKObjectList;
typedef struct SFTKObjectFreeListStr SFTKObjectFreeList;
typedef struct SFTKObjectListElementStr SFTKObjectListElement;
typedef struct SFTKObjectStr SFTKObject;
typedef struct SFTKSessionObjectStr SFTKSessionObject;
typedef struct SFTKTokenObjectStr SFTKTokenObject;
typedef struct SFTKSessionStr SFTKSession;
typedef struct SFTKSlotStr SFTKSlot;
typedef struct SFTKSessionContextStr SFTKSessionContext;
typedef struct SFTKSearchResultsStr SFTKSearchResults;
typedef struct SFTKHashVerifyInfoStr SFTKHashVerifyInfo;
typedef struct SFTKHashSignInfoStr SFTKHashSignInfo;
typedef struct SFTKSSLMACInfoStr SFTKSSLMACInfo;
typedef struct SFTKItemTemplateStr SFTKItemTemplate;


typedef void (*SFTKDestroy)(void *, PRBool);
typedef void (*SFTKBegin)(void *);
typedef SECStatus (*SFTKCipher)(void *,void *,unsigned int *,unsigned int,
					void *, unsigned int);
typedef SECStatus (*SFTKVerify)(void *,void *,unsigned int,void *,unsigned int);
typedef void (*SFTKHash)(void *,void *,unsigned int);
typedef void (*SFTKEnd)(void *,void *,unsigned int *,unsigned int);
typedef void (*SFTKFree)(void *);







typedef enum {
	SFTK_NEVER = 0,
	SFTK_ONCOPY = 1,
	SFTK_SENSITIVE = 2,
	SFTK_ALWAYS = 3
} SFTKModifyType;





typedef enum {
	SFTK_DestroyFailure,
	SFTK_Destroyed,
	SFTK_Busy
} SFTKFreeStatus;




struct SFTKAttributeStr {
    SFTKAttribute  	*next;
    SFTKAttribute  	*prev;
    PRBool		freeAttr;
    PRBool		freeData;
    
    CK_ATTRIBUTE_TYPE	handle;
    CK_ATTRIBUTE 	attrib;
    unsigned char space[ATTR_SPACE];
};





struct SFTKObjectListStr {
    SFTKObjectList *next;
    SFTKObjectList *prev;
    SFTKObject	   *parent;
};

struct SFTKObjectFreeListStr {
    SFTKObject	*head;
    PZLock	*lock;
    int		count;
};




struct SFTKObjectStr {
    SFTKObject *next;
    SFTKObject	*prev;
    CK_OBJECT_CLASS 	objclass;
    CK_OBJECT_HANDLE	handle;
    int 		refCount;
    PZLock 		*refLock;
    SFTKSlot	   	*slot;
    void 		*objectInfo;
    SFTKFree 		infoFree;
};

struct SFTKTokenObjectStr {
    SFTKObject  obj;
    SECItem	dbKey;
};

struct SFTKSessionObjectStr {
    SFTKObject	   obj;
    SFTKObjectList sessionList;
    PZLock		*attributeLock;
    SFTKSession   	*session;
    PRBool		wasDerived;
    int nextAttr;
    SFTKAttribute	attrList[MAX_OBJS_ATTRS];
    PRBool		optimizeSpace;
    unsigned int	hashSize;
    SFTKAttribute 	*head[1];
};




struct SFTKObjectListElementStr {
    SFTKObjectListElement	*next;
    SFTKObject 			*object;
};




struct SFTKSearchResultsStr {
    CK_OBJECT_HANDLE	*handles;
    int			size;
    int			index;
    int			array_size;
};





typedef enum {
    SFTK_ENCRYPT,
    SFTK_DECRYPT,
    SFTK_HASH,
    SFTK_SIGN,
    SFTK_SIGN_RECOVER,
    SFTK_VERIFY,
    SFTK_VERIFY_RECOVER
} SFTKContextType;


#define SFTK_MAX_BLOCK_SIZE 16

#define SFTK_MAX_MAC_LENGTH 64
#define SFTK_INVALID_MAC_SIZE 0xffffffff

struct SFTKSessionContextStr {
    SFTKContextType	type;
    PRBool		multi; 		
    PRBool		doPad; 		
    unsigned int	blockSize; 	
    unsigned int	padDataLength; 	
    unsigned char	padBuf[SFTK_MAX_BLOCK_SIZE];
    unsigned char	macBuf[SFTK_MAX_BLOCK_SIZE];
    CK_ULONG		macSize;	
    void		*cipherInfo;
    void		*hashInfo;
    unsigned int	cipherInfoLen;
    CK_MECHANISM_TYPE	currentMech;
    SFTKCipher		update;
    SFTKHash		hashUpdate;
    SFTKEnd		end;
    SFTKDestroy		destroy;
    SFTKDestroy		hashdestroy;
    SFTKVerify		verify;
    unsigned int	maxLen;
    SFTKObject		*key;
};




struct SFTKSessionStr {
    SFTKSession        *next;
    SFTKSession        *prev;
    CK_SESSION_HANDLE	handle;
    int			refCount;
    PZLock		*objectLock;
    int			objectIDCount;
    CK_SESSION_INFO	info;
    CK_NOTIFY		notify;
    CK_VOID_PTR		appData;
    SFTKSlot		*slot;
    SFTKSearchResults	*search;
    SFTKSessionContext	*enc_context;
    SFTKSessionContext	*hash_context;
    SFTKSessionContext	*sign_context;
    SFTKObjectList	*objects[1];
};



























struct SFTKSlotStr {
    CK_SLOT_ID		slotID;			
    PZLock		*slotLock;		
    PZLock		**sessionLock;		
    unsigned int	numSessionLocks;	
    unsigned long	sessionLockMask;	
    PZLock		*objectLock;		
    PRLock		*pwCheckLock;		
    PRBool		present;		
    PRBool		hasTokens;		
    PRBool		isLoggedIn;		
    PRBool		ssoLoggedIn;		
    PRBool		needLogin;		
    PRBool		DB_loaded;		
    PRBool		readOnly;		
    PRBool		optimizeSpace;		
    SFTKDBHandle	*certDB;		
    SFTKDBHandle	*keyDB;			
    int			minimumPinLen;		
    PRInt32		sessionIDCount;		
                                        	
    int			sessionIDConflict; 	
                                            	
    int			sessionCount;           
    PRInt32             rwSessionCount;    	
                                          	
    int			sessionObjectHandleCount;
    int			index;			
    PLHashTable		*tokObjHashTable;	
    SFTKObject		**sessObjHashTable;	
    unsigned int	sessObjHashSize;	
    SFTKSession		**head;			
    unsigned int	sessHashSize;		
    char		tokDescription[33];	
    char		updateTokDescription[33]; 
    char		slotDescription[65];	
};




struct SFTKHashVerifyInfoStr {
    SECOidTag   	hashOid;
    NSSLOWKEYPublicKey	*key;
};

struct SFTKHashSignInfoStr {
    SECOidTag   	hashOid;
    NSSLOWKEYPrivateKey	*key;
};


struct SFTKSSLMACInfoStr {
    void 		*hashContext;
    SFTKBegin		begin;
    SFTKHash		update;
    SFTKEnd		end;
    CK_ULONG		macSize;
    int			padSize;
    unsigned char	key[MAX_KEY_LEN];
    unsigned int	keySize;
};




struct SFTKItemTemplateStr {
    CK_ATTRIBUTE_TYPE	type;
    SECItem		*item;
};


#define SFTK_SET_ITEM_TEMPLATE(templ, count, itemPtr, attr) \
   templ[count].type = attr; \
   templ[count].item = itemPtr

#define SFTK_MAX_ITEM_TEMPLATE 10




#define SFTK_SESSION_SLOT_MASK	0xff000000L




#define SFTK_TOKEN_MASK		0x80000000L
#define SFTK_TOKEN_MAGIC	0x80000000L
#define SFTK_TOKEN_TYPE_MASK	0x70000000L

#define SFTK_TOKEN_TYPE_PRIV	0x10000000L
#define SFTK_TOKEN_TYPE_PUB	0x20000000L
#define SFTK_TOKEN_TYPE_KEY	0x30000000L

#define SFTK_TOKEN_TYPE_TRUST	0x40000000L
#define SFTK_TOKEN_TYPE_CRL	0x50000000L
#define SFTK_TOKEN_TYPE_SMIME	0x60000000L
#define SFTK_TOKEN_TYPE_CERT	0x70000000L

#define SFTK_TOKEN_KRL_HANDLE	(SFTK_TOKEN_MAGIC|SFTK_TOKEN_TYPE_CRL|1)

#define SFTK_MAX_PIN	255

#define FIPS_MIN_PIN	7


#define NETSCAPE_SLOT_ID 1
#define PRIVATE_KEY_SLOT_ID 2
#define FIPS_SLOT_ID 3


#define sftk_SlotFromSession(sp) ((sp)->slot)
#define sftk_isToken(id) (((id) & SFTK_TOKEN_MASK) == SFTK_TOKEN_MAGIC)


#define SHMULTIPLIER 1791398085


#define sftk_hash(value,size) \
	((PRUint32)((value) * SHMULTIPLIER) & (size-1))
#define sftkqueue_add(element,id,head,hash_size) \
	{ int tmp = sftk_hash(id,hash_size); \
	(element)->next = (head)[tmp]; \
	(element)->prev = NULL; \
	if ((head)[tmp]) (head)[tmp]->prev = (element); \
	(head)[tmp] = (element); }
#define sftkqueue_find(element,id,head,hash_size) \
	for( (element) = (head)[sftk_hash(id,hash_size)]; (element) != NULL; \
					 (element) = (element)->next) { \
	    if ((element)->handle == (id)) { break; } }
#define sftkqueue_is_queued(element,id,head,hash_size) \
	( ((element)->next) || ((element)->prev) || \
	 ((head)[sftk_hash(id,hash_size)] == (element)) )
#define sftkqueue_delete(element,id,head,hash_size) \
	if ((element)->next) (element)->next->prev = (element)->prev; \
	if ((element)->prev) (element)->prev->next = (element)->next; \
	   else (head)[sftk_hash(id,hash_size)] = ((element)->next); \
	(element)->next = NULL; \
	(element)->prev = NULL; \

#define sftkqueue_init_element(element) \
    (element)->prev = NULL;

#define sftkqueue_add2(element, id, index, head) \
    {                                            \
	(element)->next = (head)[index];         \
	if ((head)[index])                       \
	    (head)[index]->prev = (element);     \
	(head)[index] = (element);               \
    }

#define sftkqueue_find2(element, id, index, head) \
    for ( (element) = (head)[index];              \
          (element) != NULL;                      \
          (element) = (element)->next) {          \
	if ((element)->handle == (id)) { break; } \
    }

#define sftkqueue_delete2(element, id, index, head) \
	if ((element)->next) (element)->next->prev = (element)->prev; \
	if ((element)->prev) (element)->prev->next = (element)->next; \
	   else (head)[index] = ((element)->next);

#define sftkqueue_clear_deleted_element(element) \
	(element)->next = NULL; \
	(element)->prev = NULL; \



#ifdef NOSPREAD

#define SFTK_SESSION_LOCK(slot,handle) \
    ((slot)->sessionLock[((handle) >> LOG2_BUCKETS_PER_SESSION_LOCK) \
        & (slot)->sessionLockMask])
#else

#define SFTK_SESSION_LOCK(slot,handle) \
    ((slot)->sessionLock[(handle) & (slot)->sessionLockMask])
#endif


#define sftk_attr_expand(ap) (ap)->type,(ap)->pValue,(ap)->ulValueLen
#define sftk_item_expand(ip) (ip)->data,(ip)->len

typedef struct sftk_token_parametersStr {
    CK_SLOT_ID slotID;
    char *configdir;
    char *certPrefix;
    char *keyPrefix;
    char *updatedir;
    char *updCertPrefix;
    char *updKeyPrefix;
    char *updateID;
    char *tokdes;
    char *slotdes;
    char *updtokdes;
    int minPW; 
    PRBool readOnly;
    PRBool noCertDB;
    PRBool noKeyDB;
    PRBool forceOpen;
    PRBool pwRequired;
    PRBool optimizeSpace;
} sftk_token_parameters;

typedef struct sftk_parametersStr {
    char *configdir;
    char *updatedir;
    char *updateID;
    char *secmodName;
    char *man;
    char *libdes; 
    PRBool readOnly;
    PRBool noModDB;
    PRBool noCertDB;
    PRBool forceOpen;
    PRBool pwRequired;
    PRBool optimizeSpace;
    sftk_token_parameters *tokens;
    int token_count;
} sftk_parameters;



#ifdef macintosh
#define PATH_SEPARATOR ":"
#define SECMOD_DB "Security Modules"
#define CERT_DB_FMT "%sCertificates%s"
#define KEY_DB_FMT "%sKey Database%s"
#else
#define PATH_SEPARATOR "/"
#define SECMOD_DB "secmod.db"
#define CERT_DB_FMT "%scert%s.db"
#define KEY_DB_FMT "%skey%s.db"
#endif

SEC_BEGIN_PROTOS


extern PRBool nsf_init;
extern CK_RV nsc_CommonInitialize(CK_VOID_PTR pReserved, PRBool isFIPS);
extern CK_RV nsc_CommonFinalize(CK_VOID_PTR pReserved, PRBool isFIPS);
extern PRBool sftk_ForkReset(CK_VOID_PTR pReserved, CK_RV* crv);
extern CK_RV nsc_CommonGetSlotList(CK_BBOOL tokPresent, 
	CK_SLOT_ID_PTR pSlotList, CK_ULONG_PTR pulCount, int moduleIndex);


extern CK_RV SFTK_SlotInit(char *configdir, char *updatedir, char *updateID,
			sftk_token_parameters *params, int moduleIndex);
extern CK_RV SFTK_SlotReInit(SFTKSlot *slot, char *configdir,
			char *updatedir, char *updateID,
			sftk_token_parameters *params, int moduleIndex);
extern CK_RV SFTK_DestroySlotData(SFTKSlot *slot);
extern CK_RV SFTK_ShutdownSlot(SFTKSlot *slot);
extern CK_RV sftk_CloseAllSessions(SFTKSlot *slot, PRBool logout);



extern SFTKAttribute *sftk_FindAttribute(SFTKObject *object,
					 CK_ATTRIBUTE_TYPE type);
extern void sftk_FreeAttribute(SFTKAttribute *attribute);
extern CK_RV sftk_AddAttributeType(SFTKObject *object, CK_ATTRIBUTE_TYPE type,
				   void *valPtr,
				  CK_ULONG length);
extern CK_RV sftk_Attribute2SecItem(PLArenaPool *arena, SECItem *item,
				    SFTKObject *object, CK_ATTRIBUTE_TYPE type);
extern CK_RV sftk_MultipleAttribute2SecItem(PLArenaPool *arena, 
		SFTKObject *object, SFTKItemTemplate *templ, int count);
extern unsigned int sftk_GetLengthInBits(unsigned char *buf,
							 unsigned int bufLen);
extern CK_RV sftk_ConstrainAttribute(SFTKObject *object, 
	CK_ATTRIBUTE_TYPE type, int minLength, int maxLength, int minMultiple);
extern PRBool sftk_hasAttribute(SFTKObject *object, CK_ATTRIBUTE_TYPE type);
extern PRBool sftk_isTrue(SFTKObject *object, CK_ATTRIBUTE_TYPE type);
extern void sftk_DeleteAttributeType(SFTKObject *object,
				     CK_ATTRIBUTE_TYPE type);
extern CK_RV sftk_Attribute2SecItem(PLArenaPool *arena, SECItem *item,
				    SFTKObject *object, CK_ATTRIBUTE_TYPE type);
extern CK_RV sftk_Attribute2SSecItem(PLArenaPool *arena, SECItem *item,
				     SFTKObject *object,
				     CK_ATTRIBUTE_TYPE type);
extern SFTKModifyType sftk_modifyType(CK_ATTRIBUTE_TYPE type,
				      CK_OBJECT_CLASS inClass);
extern PRBool sftk_isSensitive(CK_ATTRIBUTE_TYPE type, CK_OBJECT_CLASS inClass);
extern char *sftk_getString(SFTKObject *object, CK_ATTRIBUTE_TYPE type);
extern void sftk_nullAttribute(SFTKObject *object,CK_ATTRIBUTE_TYPE type);
extern CK_RV sftk_GetULongAttribute(SFTKObject *object, CK_ATTRIBUTE_TYPE type,
                                                         CK_ULONG *longData);
extern CK_RV sftk_forceAttribute(SFTKObject *object, CK_ATTRIBUTE_TYPE type,
				 void *value, unsigned int len);
extern CK_RV sftk_defaultAttribute(SFTKObject *object, CK_ATTRIBUTE_TYPE type,
				   void *value, unsigned int len);
extern unsigned int sftk_MapTrust(CK_TRUST trust, PRBool clientAuth);

extern SFTKObject *sftk_NewObject(SFTKSlot *slot);
extern CK_RV sftk_CopyObject(SFTKObject *destObject, SFTKObject *srcObject);
extern SFTKFreeStatus sftk_FreeObject(SFTKObject *object);
extern CK_RV sftk_DeleteObject(SFTKSession *session, SFTKObject *object);
extern void sftk_ReferenceObject(SFTKObject *object);
extern SFTKObject *sftk_ObjectFromHandle(CK_OBJECT_HANDLE handle,
					 SFTKSession *session);
extern void sftk_AddSlotObject(SFTKSlot *slot, SFTKObject *object);
extern void sftk_AddObject(SFTKSession *session, SFTKObject *object);


extern CK_RV SFTK_ClearTokenKeyHashTable(SFTKSlot *slot);

extern CK_RV sftk_searchObjectList(SFTKSearchResults *search,
				   SFTKObject **head, unsigned int size,
				   PZLock *lock, CK_ATTRIBUTE_PTR inTemplate,
				   int count, PRBool isLoggedIn);
extern SFTKObjectListElement *sftk_FreeObjectListElement(
					     SFTKObjectListElement *objectList);
extern void sftk_FreeObjectList(SFTKObjectListElement *objectList);
extern void sftk_FreeSearch(SFTKSearchResults *search);
extern CK_RV sftk_handleObject(SFTKObject *object, SFTKSession *session);

extern SFTKSlot *sftk_SlotFromID(CK_SLOT_ID slotID, PRBool all);
extern SFTKSlot *sftk_SlotFromSessionHandle(CK_SESSION_HANDLE handle);
extern SFTKSession *sftk_SessionFromHandle(CK_SESSION_HANDLE handle);
extern void sftk_FreeSession(SFTKSession *session);
extern SFTKSession *sftk_NewSession(CK_SLOT_ID slotID, CK_NOTIFY notify,
				    CK_VOID_PTR pApplication, CK_FLAGS flags);
extern void sftk_update_state(SFTKSlot *slot,SFTKSession *session);
extern void sftk_update_all_states(SFTKSlot *slot);
extern void sftk_FreeContext(SFTKSessionContext *context);
extern void sftk_InitFreeLists(void);
extern void sftk_CleanupFreeLists(void);

extern NSSLOWKEYPublicKey *sftk_GetPubKey(SFTKObject *object,
					  CK_KEY_TYPE key_type, CK_RV *crvp);
extern NSSLOWKEYPrivateKey *sftk_GetPrivKey(SFTKObject *object,
					    CK_KEY_TYPE key_type, CK_RV *crvp);
extern void sftk_FormatDESKey(unsigned char *key, int length);
extern PRBool sftk_CheckDESKey(unsigned char *key);
extern PRBool sftk_IsWeakKey(unsigned char *key,CK_KEY_TYPE key_type);


extern CK_RV sftk_MechAllowsOperation(CK_MECHANISM_TYPE type, CK_ATTRIBUTE_TYPE op);



NSSLOWKEYPrivateKey *sftk_FindKeyByPublicKey(SFTKSlot *slot, SECItem *dbKey);




SFTKSessionObject * sftk_narrowToSessionObject(SFTKObject *);
SFTKTokenObject * sftk_narrowToTokenObject(SFTKObject *);




void sftk_addHandle(SFTKSearchResults *search, CK_OBJECT_HANDLE handle);
PRBool sftk_poisonHandle(SFTKSlot *slot, SECItem *dbkey, 
						CK_OBJECT_HANDLE handle);
SFTKObject * sftk_NewTokenObject(SFTKSlot *slot, SECItem *dbKey, 
						CK_OBJECT_HANDLE handle);
SFTKTokenObject *sftk_convertSessionToToken(SFTKObject *so);





extern CK_RV
sftk_TLSPRFInit(SFTKSessionContext *context, 
		  SFTKObject *        key, 
		  CK_KEY_TYPE         key_type);

SEC_END_PROTOS

#endif 
