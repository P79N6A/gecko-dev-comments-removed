



#ifndef PKITM_H
#define PKITM_H

#ifdef DEBUG
static const char PKITM_CVS_ID[] = "@(#) $RCSfile: pkitm.h,v $ $Revision: 1.17 $ $Date: 2012/07/27 21:41:52 $";
#endif 







#ifndef BASET_H
#include "baset.h"
#endif 

#ifndef PKIT_H
#include "pkit.h"
#endif 

PR_BEGIN_EXTERN_C

typedef enum nssCertIDMatchEnum {
  nssCertIDMatch_Yes = 0,
  nssCertIDMatch_No = 1,
  nssCertIDMatch_Unknown = 2
} nssCertIDMatch;









struct nssDecodedCertStr {
    NSSCertificateType type;
    void *data;
    
    NSSItem *  (*getIdentifier)(nssDecodedCert *dc);
    
    void *     (*getIssuerIdentifier)(nssDecodedCert *dc);
    
    nssCertIDMatch (*matchIdentifier)(nssDecodedCert *dc, void *id);
    
    PRBool     (*isValidIssuer)(nssDecodedCert *dc);
    
    NSSUsage * (*getUsage)(nssDecodedCert *dc);
    
    PRBool     (*isValidAtTime)(nssDecodedCert *dc, NSSTime *time);
    
    PRBool     (*isNewerThan)(nssDecodedCert *dc, nssDecodedCert *cmpdc);
    
    PRBool     (*matchUsage)(nssDecodedCert *dc, const NSSUsage *usage);
    
    PRBool     (*isTrustedForUsage)(nssDecodedCert *dc,
                                    const NSSUsage *usage);
    
    NSSASCII7 *(*getEmailAddress)(nssDecodedCert *dc);
    
    PRStatus   (*getDERSerialNumber)(nssDecodedCert *dc,
                                     NSSDER *derSerial, NSSArena *arena);
};

struct NSSUsageStr {
    PRBool anyUsage;
    SECCertUsage nss3usage;
    PRBool nss3lookingForCA;
};

typedef struct nssPKIObjectCollectionStr nssPKIObjectCollection;

typedef struct
{
  union {
    PRStatus (*  cert)(NSSCertificate *c, void *arg);
    PRStatus (*   crl)(NSSCRL       *crl, void *arg);
    PRStatus (* pvkey)(NSSPrivateKey *vk, void *arg);
    PRStatus (* pbkey)(NSSPublicKey *bk, void *arg);
  } func;
  void *arg;
} nssPKIObjectCallback;

PR_END_EXTERN_C

#endif 
