


#ifndef UTILPARS_T_H
#define UTILPARS_T_H  1
#include "pkcs11t.h"













#define NSSUTIL_HANDLE_STRING_ARG(param,target,value,command) \
    if (PORT_Strncasecmp(param,value,sizeof(value)-1) == 0) { \
        param += sizeof(value)-1; \
        if (target) PORT_Free(target); \
        target = NSSUTIL_ArgFetchValue(param,&next); \
        param += next; \
        command ;\
    } else

#define NSSUTIL_HANDLE_FINAL_ARG(param) \
    { param = NSSUTIL_ArgSkipParameter(param); } param = NSSUTIL_ArgStrip(param);

#define NSSUTIL_PATH_SEPARATOR "/"


#define NSSUTIL_DEFAULT_INTERNAL_INIT1  \
        "library= name=\"NSS Internal PKCS #11 Module\" parameters="
#define NSSUTIL_DEFAULT_INTERNAL_INIT2 \
        " NSS=\"Flags=internal,critical trustOrder=75 cipherOrder=100 slotParams=(1={"
#define NSSUTIL_DEFAULT_INTERNAL_INIT3 \
        " askpw=any timeout=30})\""
#define NSSUTIL_DEFAULT_SFTKN_FLAGS \
	"slotFlags=[RSA,DSA,DH,RC2,RC4,DES,RANDOM,SHA1,MD5,MD2,SSL,TLS,AES,Camellia,SEED,SHA256,SHA512]"

#define NSSUTIL_DEFAULT_CIPHER_ORDER 0
#define NSSUTIL_DEFAULT_TRUST_ORDER 50
#define NSSUTIL_ARG_ESCAPE '\\'





struct NSSUTILPreSlotInfoStr {
    CK_SLOT_ID slotID;  	
    unsigned long defaultFlags; 

    int askpw;			
    long timeout;		
    char hasRootCerts;		
    char hasRootTrust;		
    int  reserved0[2];
    void *reserved1[2];
};





typedef enum {
   NSS_DB_TYPE_NONE= 0,
   NSS_DB_TYPE_SQL,
   NSS_DB_TYPE_EXTERN,
   NSS_DB_TYPE_LEGACY,
   NSS_DB_TYPE_MULTIACCESS
} NSSDBType;

#endif 
