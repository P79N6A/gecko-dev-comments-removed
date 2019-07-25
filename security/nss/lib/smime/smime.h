










































#ifndef _SECMIME_H_
#define _SECMIME_H_ 1

#include "cms.h"



SEC_BEGIN_PROTOS




























extern SECStatus NSS_SMIMEUtil_EnableCipher(long which, int on);














extern SECStatus NSS_SMIMEUtils_AllowCipher(long which, int on);





extern PRBool NSS_SMIMEUtil_DecryptionAllowed(SECAlgorithmID *algid, PK11SymKey *key);



















extern PRBool NSS_SMIMEUtil_EncryptionPossible(void);







extern SECStatus NSS_SMIMEUtil_CreateSMIMECapabilities(PLArenaPool *poolp, SECItem *dest);




extern SECStatus NSS_SMIMEUtil_CreateSMIMEEncKeyPrefs(PLArenaPool *poolp, SECItem *dest, CERTCertificate *cert);




extern SECStatus NSS_SMIMEUtil_CreateMSSMIMEEncKeyPrefs(PLArenaPool *poolp, SECItem *dest, CERTCertificate *cert);





extern CERTCertificate *NSS_SMIMEUtil_GetCertFromEncryptionKeyPreference(CERTCertDBHandle *certdb, SECItem *DERekp);




extern SECStatus
NSS_SMIMEUtil_FindBulkAlgForRecipients(CERTCertificate **rcerts, SECOidTag *bulkalgtag, int *keysize);











extern PRBool NSSSMIME_VersionCheck(const char *importedVersion);




extern const char *NSSSMIME_GetVersion(void);


SEC_END_PROTOS

#endif 
