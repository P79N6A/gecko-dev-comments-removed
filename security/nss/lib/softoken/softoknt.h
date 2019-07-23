







































#ifndef _SOFTOKNT_H_
#define _SOFTOKNT_H_








typedef enum {
    RSA_BlockPrivate0 = 0,	
    RSA_BlockPrivate = 1,	
    RSA_BlockPublic = 2,	
    RSA_BlockOAEP = 3,		
				

    RSA_BlockRaw = 4,		
    RSA_BlockTotal
} RSA_BlockType;

#define NSS_SOFTOKEN_DEFAULT_CHUNKSIZE   2048




typedef enum {
    NSS_AUDIT_ERROR = 3,    
    NSS_AUDIT_WARNING = 2,  
    NSS_AUDIT_INFO = 1      
} NSSAuditSeverity;

typedef enum {
    NSS_AUDIT_ACCESS_KEY = 0,
    NSS_AUDIT_CHANGE_KEY,
    NSS_AUDIT_COPY_KEY,
    NSS_AUDIT_CRYPT,
    NSS_AUDIT_DERIVE_KEY,
    NSS_AUDIT_DESTROY_KEY,
    NSS_AUDIT_DIGEST_KEY,
    NSS_AUDIT_FIPS_STATE,
    NSS_AUDIT_GENERATE_KEY,
    NSS_AUDIT_INIT_PIN,
    NSS_AUDIT_INIT_TOKEN,
    NSS_AUDIT_LOAD_KEY,
    NSS_AUDIT_LOGIN,
    NSS_AUDIT_LOGOUT,
    NSS_AUDIT_SELF_TEST,
    NSS_AUDIT_SET_PIN,
    NSS_AUDIT_UNWRAP_KEY,
    NSS_AUDIT_WRAP_KEY
} NSSAuditType;

#endif 
