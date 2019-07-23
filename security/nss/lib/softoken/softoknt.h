







































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

#endif 
