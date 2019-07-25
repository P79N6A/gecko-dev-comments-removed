



































#ifndef NSSPKIT_H
#define NSSPKIT_H

#ifdef DEBUG
static const char NSSPKIT_CVS_ID[] = "@(#) $RCSfile: nsspkit.h,v $ $Revision: 1.8 $ $Date: 2010/05/21 00:02:48 $";
#endif 







#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

PR_BEGIN_EXTERN_C
















struct NSSCertificateStr;
typedef struct NSSCertificateStr NSSCertificate;



















typedef NSSCertificate NSSUserCertificate;









struct NSSPrivateKeyStr;
typedef struct NSSPrivateKeyStr NSSPrivateKey;






struct NSSPublicKeyStr;
typedef struct NSSPublicKeyStr NSSPublicKey;






struct NSSSymmetricKeyStr;
typedef struct NSSSymmetricKeyStr NSSSymmetricKey;





















struct NSSTrustDomainStr;
typedef struct NSSTrustDomainStr NSSTrustDomain;























typedef struct NSSCryptoContextStr NSSCryptoContext;











struct NSSOIDStr;  
typedef struct NSSOIDStr NSSOID;















struct NSSTimeStr;
typedef struct NSSTimeStr NSSTime;

struct NSSTrustStr;
typedef struct NSSTrustStr NSSTrust;

















struct NSSUsageStr;
typedef struct NSSUsageStr NSSUsage;







struct NSSPoliciesStr;
typedef struct NSSPoliciesStr NSSPolicies;








struct NSSAlgorithmAndParametersStr;
typedef struct NSSAlgorithmAndParametersStr NSSAlgorithmAndParameters;









typedef struct NSSCallbackStr NSSCallback;

struct NSSCallbackStr {
    
    PRStatus (* getInitPW)(NSSUTF8 *slotName, void *arg, 
                           NSSUTF8 **ssoPW, NSSUTF8 **userPW); 
    


    PRStatus (* getNewPW)(NSSUTF8 *slotName, PRUint32 *retries, void *arg,
                          NSSUTF8 **oldPW, NSSUTF8 **newPW); 
    
    PRStatus (* getPW)(NSSUTF8 *slotName, PRUint32 *retries, void *arg,
                       NSSUTF8 **password); 
    void *arg;
};



typedef PRUint32 NSSOperations;



#define NSSOperations_ENCRYPT           0x0001
#define NSSOperations_DECRYPT           0x0002
#define NSSOperations_WRAP              0x0004
#define NSSOperations_UNWRAP            0x0008
#define NSSOperations_SIGN              0x0010
#define NSSOperations_SIGN_RECOVER      0x0020
#define NSSOperations_VERIFY            0x0040
#define NSSOperations_VERIFY_RECOVER    0x0080

struct NSSPKIXCertificateStr;

PR_END_EXTERN_C

#endif 
