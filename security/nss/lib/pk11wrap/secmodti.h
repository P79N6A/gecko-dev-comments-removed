







#ifndef  _SECMODTI_H_
#define  _SECMODTI_H_ 1
#include "prmon.h"
#include "prtypes.h"
#include "nssilckt.h"
#include "secmodt.h"
#include "pkcs11t.h"

#include "nssdevt.h"




typedef struct pk11TraverseSlotStr {
    SECStatus (*callback)(PK11SlotInfo *,CK_OBJECT_HANDLE, void *);
    void *callbackArg;
    CK_ATTRIBUTE *findTemplate;
    int templateCount;
} pk11TraverseSlot;



struct PK11SlotInfoStr {
    
    void *functionList;
    SECMODModule *module; 
    
    PRBool needTest;	
    PRBool isPerm;	
    PRBool isHW;	
    PRBool isInternal;  
    PRBool disabled;	
    PK11DisableReasons reason; 	
    PRBool readOnly;	
    PRBool needLogin;	


    PRBool hasRandom;   
    PRBool defRWSession; 


    PRBool isThreadSafe; 
    
    CK_FLAGS flags;      
    
    CK_SESSION_HANDLE session; 
    PZLock *sessionLock; 
    
    CK_SLOT_ID slotID;
    
    unsigned long defaultFlags;
    

    PRInt32 refCount;    
    PZLock *freeListLock;
    PK11SymKey *freeSymKeysWithSessionHead;
    PK11SymKey *freeSymKeysHead;
    int keyCount;
    int maxKeyCount;
    

    int askpw;		
    int timeout;	

    int authTransact;   

    int64 authTime;     
    int minPassword;	
    int maxPassword;	
    uint16 series;	


    uint16 flagSeries;	

    PRBool flagState;	

    uint16 wrapKey;	
    CK_MECHANISM_TYPE wrapMechanism;
			
    CK_OBJECT_HANDLE refKeys[1]; 
    CK_MECHANISM_TYPE *mechanismList; 

    int mechanismCount;
    
    CERTCertificate **cert_array;
    int array_size;
    int cert_count;
    char serial[16];
    

    char slot_name[65];
    char token_name[33];
    PRBool hasRootCerts;
    PRBool hasRootTrust;
    PRBool hasRSAInfo;
    CK_FLAGS RSAInfoFlags;
    PRBool protectedAuthPath;
    PRBool isActiveCard;
    PRIntervalTime lastLoginCheck;
    unsigned int lastState;
    
    NSSToken *nssToken;
    
    char mechanismBits[256];
};


struct PK11SymKeyStr {
    CK_MECHANISM_TYPE type;	
    CK_OBJECT_HANDLE  objectID; 
    PK11SlotInfo      *slot;    
    void	      *cx;	
    PK11SymKey	      *next;
    PRBool	      owner;
    SECItem	      data;	
    CK_SESSION_HANDLE session;
    PRBool	      sessionOwner;
    PRInt32	      refCount;	
    int		      size;	
    PK11Origin	      origin;	

    PK11SymKey        *parent;  
    uint16 series;		


    void *userData;		

    PK11FreeDataFunc freeFunc;	
};







struct PK11ContextStr {
    CK_ATTRIBUTE_TYPE	operation; 

    PK11SymKey  	*key;	   
    PK11SlotInfo	*slot;	   
    CK_SESSION_HANDLE	session;   
    PZLock		*sessionLock; 

    PRBool		ownSession;
    void 		*cx;	   
    void		*savedData;

    unsigned long	savedLength; 
    SECItem		*param;	    

    PRBool		init;	    
    CK_MECHANISM_TYPE	type;	    



    PRBool		fortezzaHack; 

};





struct PK11GenericObjectStr {
    PK11GenericObject *prev;
    PK11GenericObject *next;
    PK11SlotInfo *slot;
    CK_OBJECT_HANDLE objectID;
};


#define MAX_TEMPL_ATTRS 16 /* maximum attributes in template */


#define CKF_KEY_OPERATION_FLAGS 0x000e7b00UL


#endif 
