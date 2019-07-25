


































#ifndef _SEC_UTIL_H_
#define _SEC_UTIL_H_

#include "seccomon.h"
#include "secitem.h"
#include "prerror.h"
#include "base64.h"
#include "key.h"
#include "secpkcs7.h"
#include "secasn1.h"
#include "secder.h"
#include <stdio.h>

#define SEC_CT_PRIVATE_KEY		"private-key"
#define SEC_CT_PUBLIC_KEY		"public-key"
#define SEC_CT_CERTIFICATE		"certificate"
#define SEC_CT_CERTIFICATE_REQUEST	"certificate-request"
#define SEC_CT_PKCS7			"pkcs7"
#define SEC_CT_CRL			"crl"

#define NS_CERTREQ_HEADER "-----BEGIN NEW CERTIFICATE REQUEST-----"
#define NS_CERTREQ_TRAILER "-----END NEW CERTIFICATE REQUEST-----"

#define NS_CERT_HEADER "-----BEGIN CERTIFICATE-----"
#define NS_CERT_TRAILER "-----END CERTIFICATE-----"

#define NS_CRL_HEADER  "-----BEGIN CRL-----"
#define NS_CRL_TRAILER "-----END CRL-----"

#ifdef SECUTIL_NEW
typedef int (*SECU_PPFunc)(PRFileDesc *out, SECItem *item, 
                           char *msg, int level);
#else
typedef int (*SECU_PPFunc)(FILE *out, SECItem *item, char *msg, int level);
#endif

typedef struct {
    enum {
	PW_NONE = 0,
	PW_FROMFILE = 1,
	PW_PLAINTEXT = 2,
	PW_EXTERNAL = 3
    } source;
    char *data;
} secuPWData;







SECStatus SECU_ChangePW(PK11SlotInfo *slot, char *passwd, char *pwFile);








SECStatus SECU_ChangePW2(PK11SlotInfo *slot, char *oldPass, char *newPass,
                        char *oldPwFile, char *newPwFile);







extern PRBool SEC_CheckPassword(char *password);






extern PRBool SEC_BlindCheckPassword(char *password);






extern char *SEC_GetPassword(FILE *in, FILE *out, char *msg,
				      PRBool (*chkpw)(char *));

char *SECU_FilePasswd(PK11SlotInfo *slot, PRBool retry, void *arg);

char *SECU_GetPasswordString(void *arg, char *prompt);






extern SECStatus SEC_WriteDongleFile(int fd, char *pw);






extern char *SEC_ReadDongleFile(int fd);





char *SECU_AppendFilenameToDir(char *dir, char *filename);


extern char *SECU_DefaultSSLDir(void);







extern char *SECU_ConfigDirectory(const char* base);




extern int
SECU_GetClientAuthData(void *arg, PRFileDesc *fd,
		       struct CERTDistNamesStr *caNames,
		       struct CERTCertificateStr **pRetCert,
		       struct SECKEYPrivateKeyStr **pRetKey);


extern void SECU_PrintError(char *progName, char *msg, ...);


extern void SECU_PrintSystemError(char *progName, char *msg, ...);


extern const char * SECU_Strerror(PRErrorCode errNum);



extern void
SECU_printCertProblems(FILE *outfile, CERTCertDBHandle *handle, 
	CERTCertificate *cert, PRBool checksig, 
	SECCertificateUsage certUsage, void *pinArg, PRBool verbose);



extern void
SECU_printCertProblemsOnDate(FILE *outfile, CERTCertDBHandle *handle, 
	CERTCertificate *cert, PRBool checksig, SECCertificateUsage certUsage, 
	void *pinArg, PRBool verbose, PRTime datetime);


extern void
SECU_displayVerifyLog(FILE *outfile, CERTVerifyLog *log,
                      PRBool verbose);


extern SECStatus SECU_FileToItem(SECItem *dst, PRFileDesc *src);
extern SECStatus SECU_TextFileToItem(SECItem *dst, PRFileDesc *src);


extern SECStatus 
SECU_ReadDERFromFile(SECItem *der, PRFileDesc *inFile, PRBool ascii);


extern void SECU_Indent(FILE *out, int level);


extern void SECU_PrintInteger(FILE *out, SECItem *i, char *m, int level);


extern SECOidTag SECU_PrintObjectID(FILE *out, SECItem *oid, char *m, int level);


extern void SECU_PrintAlgorithmID(FILE *out, SECAlgorithmID *a, char *m,
				  int level);


extern void SECU_PrintAsHex(FILE *out, SECItem *i, const char *m, int level);


extern void SECU_PrintBuf(FILE *out, const char *msg, const void *vp, int len);






extern void SECU_PrintUTCTime(FILE *out, SECItem *t, char *m, int level);






extern void SECU_PrintGeneralizedTime(FILE *out, SECItem *t, char *m,
				      int level);






extern void SECU_PrintTimeChoice(FILE *out, SECItem *t, char *m, int level);


extern SECStatus SECU_PrintCertNickname(CERTCertListNode* cert, void *data);


extern SECStatus
SECU_PrintCertificateNames(CERTCertDBHandle *handle, PRFileDesc* out, 
                           PRBool sortByName, PRBool sortByTrust);


int SECU_CheckCertNameExists(CERTCertDBHandle *handle, char *nickname);


extern int SECU_PrintCertificateRequest(FILE *out, SECItem *der, char *m,
	int level);


extern int SECU_PrintCertificate(FILE *out, SECItem *der, char *m, int level);


extern void SECU_PrintTrustFlags(FILE *out, CERTCertTrust *trust, char *m, 
                                 int level);


extern int SECU_PrintRSAPublicKey(FILE *out, SECItem *der, char *m, int level);

extern int SECU_PrintSubjectPublicKeyInfo(FILE *out, SECItem *der, char *m, 
                                          int level);

#ifdef HAVE_EPV_TEMPLATE

extern int SECU_PrintPrivateKey(FILE *out, SECItem *der, char *m, int level);
#endif


extern int SECU_PrintFingerprints(FILE *out, SECItem *derCert, char *m,
                                  int level);


extern int SECU_PrintPKCS7ContentInfo(FILE *out, SECItem *der, char *m, 
				      int level);


extern SECStatus SECU_PKCS11Init(PRBool readOnly);


extern int SECU_PrintSignedData(FILE *out, SECItem *der, const char *m, 
                                int level, SECU_PPFunc inner);


extern SECStatus SEC_PrintCertificateAndTrust(CERTCertificate *cert,
                                              const char *label,
                                              CERTCertTrust *trust);

extern int SECU_PrintCrl(FILE *out, SECItem *der, char *m, int level);

extern void
SECU_PrintCRLInfo(FILE *out, CERTCrl *crl, char *m, int level);

extern void SECU_PrintString(FILE *out, SECItem *si, char *m, int level);
extern void SECU_PrintAny(FILE *out, SECItem *i, char *m, int level);

extern void SECU_PrintPolicy(FILE *out, SECItem *value, char *msg, int level);
extern void SECU_PrintPrivKeyUsagePeriodExtension(FILE *out, SECItem *value,
                                 char *msg, int level);

extern void SECU_PrintExtensions(FILE *out, CERTCertExtension **extensions,
				 char *msg, int level);

extern void SECU_PrintName(FILE *out, CERTName *name, const char *msg,
                           int level);
extern void SECU_PrintRDN(FILE *out, CERTRDN *rdn, const char *msg, int level);

#ifdef SECU_GetPassword

extern SECKEYLowPublicKey *SECU_ConvHighToLow(SECKEYPublicKey *pubHighKey);
#endif

extern char *SECU_GetModulePassword(PK11SlotInfo *slot, PRBool retry, void *arg);

extern SECStatus DER_PrettyPrint(FILE *out, SECItem *it, PRBool raw);

extern char *SECU_SECModDBName(void);

extern void SECU_PrintPRandOSError(char *progName);

extern SECStatus SECU_RegisterDynamicOids(void);


extern SECOidTag SECU_StringToSignatureAlgTag(const char *alg);




extern SECStatus SECU_StoreCRL(PK11SlotInfo *slot, SECItem *derCrl,
                               PRFileDesc *outFile, PRBool ascii, char *url);














extern SECStatus SECU_DerSignDataCRL(PRArenaPool *arena, CERTSignedData *sd,
                                     unsigned char *buf, int len,
                                     SECKEYPrivateKey *pk, SECOidTag algID);

typedef enum  {
    noKeyFound = 1,
    noSignatureMatch = 2,
    failToEncode = 3,
    failToSign = 4,
    noMem = 5
} SignAndEncodeFuncExitStat;

extern SECStatus
SECU_SignAndEncodeCRL(CERTCertificate *issuer, CERTSignedCrl *signCrl,
                      SECOidTag hashAlgTag, SignAndEncodeFuncExitStat *resCode);

extern SECStatus
SECU_CopyCRL(PRArenaPool *destArena, CERTCrl *destCrl, CERTCrl *srcCrl);





CERTAuthKeyID *
SECU_FindCRLAuthKeyIDExten (PRArenaPool *arena, CERTSignedCrl *crl);




CERTCertificate *
SECU_FindCrlIssuer(CERTCertDBHandle *dbHandle, SECItem* subject,
                   CERTAuthKeyID* id, PRTime validTime);




typedef SECStatus (* EXTEN_EXT_VALUE_ENCODER) (PRArenaPool *extHandleArena,
                                               void *value, SECItem *encodedValue);


SECStatus 
SECU_EncodeAndAddExtensionValue(PRArenaPool *arena, void *extHandle, 
                                void *value, PRBool criticality, int extenType, 
                                EXTEN_EXT_VALUE_ENCODER EncodeValueFn);


void
SECU_SECItemToHex(const SECItem * item, char * dst);



SECStatus
SECU_SECItemHexStringToBinary(SECItem* srcdest);








typedef struct {
    char flag;
    PRBool needsArg;
    char *arg;
    PRBool activated;
    char *longform;
} secuCommandFlag;


typedef struct
{
    int numCommands;
    int numOptions;

    secuCommandFlag *commands;
    secuCommandFlag *options;
} secuCommand;


SECStatus 
SECU_ParseCommandLine(int argc, char **argv, char *progName,
		      const secuCommand *cmd);
char *
SECU_GetOptionArg(const secuCommand *cmd, int optionNum);








char *SECU_ErrorString(int16 err);


char *SECU_ErrorStringRaw(int16 err);

void printflags(char *trusts, unsigned int flags);

#if !defined(XP_UNIX) && !defined(XP_OS2)
extern int ffs(unsigned int i);
#endif



CERTCertificate*
SECU_FindCertByNicknameOrFilename(CERTCertDBHandle *handle,
                                  char *name, PRBool ascii,
                                  void *pwarg);
#include "secerr.h"
#include "sslerr.h"

#endif 
