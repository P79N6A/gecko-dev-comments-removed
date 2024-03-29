






#ifndef _SOFTOKEN_H_
#define _SOFTOKEN_H_

#include "blapi.h"
#include "lowkeyti.h"
#include "softoknt.h"
#include "secoidt.h"

#include "pkcs11t.h"

SEC_BEGIN_PROTOS









extern SECStatus
RSA_HashSign(SECOidTag hashOid, NSSLOWKEYPrivateKey *key,
             unsigned char *sig, unsigned int *sigLen, unsigned int maxLen,
             const unsigned char *hash, unsigned int hashLen);

extern SECStatus
RSA_HashCheckSign(SECOidTag hashOid, NSSLOWKEYPublicKey *key,
                  const unsigned char *sig, unsigned int sigLen,
                  const unsigned char *hash, unsigned int hashLen);














extern unsigned char * CBC_PadBuffer(PLArenaPool *arena, unsigned char *inbuf,
                                     unsigned int inlen, unsigned int *outlen,
				     int blockSize);







extern CK_RV sftk_fipsPowerUpSelfTest( void ); 



	
unsigned long sftk_MapKeySize(CK_KEY_TYPE keyType);




extern PRBool sftk_audit_enabled;

extern void sftk_LogAuditMessage(NSSAuditSeverity severity, 
				 NSSAuditType, const char *msg);

extern void sftk_AuditCreateObject(CK_SESSION_HANDLE hSession,
			CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount,
			CK_OBJECT_HANDLE_PTR phObject, CK_RV rv);

extern void sftk_AuditCopyObject(CK_SESSION_HANDLE hSession,
			CK_OBJECT_HANDLE hObject,
			CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount,
			CK_OBJECT_HANDLE_PTR phNewObject, CK_RV rv);

extern void sftk_AuditDestroyObject(CK_SESSION_HANDLE hSession,
			CK_OBJECT_HANDLE hObject, CK_RV rv);

extern void sftk_AuditGetObjectSize(CK_SESSION_HANDLE hSession,
			CK_OBJECT_HANDLE hObject, CK_ULONG_PTR pulSize,
			CK_RV rv);

extern void sftk_AuditGetAttributeValue(CK_SESSION_HANDLE hSession,
			CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate,
			CK_ULONG ulCount, CK_RV rv);

extern void sftk_AuditSetAttributeValue(CK_SESSION_HANDLE hSession,
			CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate,
			CK_ULONG ulCount, CK_RV rv);

extern void sftk_AuditCryptInit(const char *opName,
			CK_SESSION_HANDLE hSession,
			CK_MECHANISM_PTR pMechanism,
			CK_OBJECT_HANDLE hKey, CK_RV rv);

extern void sftk_AuditGenerateKey(CK_SESSION_HANDLE hSession,
			CK_MECHANISM_PTR pMechanism,
			CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount,
			CK_OBJECT_HANDLE_PTR phKey, CK_RV rv);

extern void sftk_AuditGenerateKeyPair(CK_SESSION_HANDLE hSession,
			CK_MECHANISM_PTR pMechanism,
			CK_ATTRIBUTE_PTR pPublicKeyTemplate,
			CK_ULONG ulPublicKeyAttributeCount,
			CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
			CK_ULONG ulPrivateKeyAttributeCount,
			CK_OBJECT_HANDLE_PTR phPublicKey,
			CK_OBJECT_HANDLE_PTR phPrivateKey, CK_RV rv);

extern void sftk_AuditWrapKey(CK_SESSION_HANDLE hSession,
			CK_MECHANISM_PTR pMechanism,
			CK_OBJECT_HANDLE hWrappingKey, CK_OBJECT_HANDLE hKey,
			CK_BYTE_PTR pWrappedKey,
			CK_ULONG_PTR pulWrappedKeyLen, CK_RV rv);

extern void sftk_AuditUnwrapKey(CK_SESSION_HANDLE hSession,
			CK_MECHANISM_PTR pMechanism,
			CK_OBJECT_HANDLE hUnwrappingKey,
			CK_BYTE_PTR pWrappedKey, CK_ULONG ulWrappedKeyLen,
			CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount,
			CK_OBJECT_HANDLE_PTR phKey, CK_RV rv);

extern void sftk_AuditDeriveKey(CK_SESSION_HANDLE hSession,
			CK_MECHANISM_PTR pMechanism,
			CK_OBJECT_HANDLE hBaseKey,
			CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount,
			CK_OBJECT_HANDLE_PTR phKey, CK_RV rv);

extern void sftk_AuditDigestKey(CK_SESSION_HANDLE hSession,
			CK_OBJECT_HANDLE hKey, CK_RV rv);




extern PRBool sftk_fatalError;




#if defined(XP_UNIX) && !defined(NO_FORK_CHECK)

#ifdef DEBUG

#define FORK_ASSERT() \
    { \
        char* forkAssert = getenv("NSS_STRICT_NOFORK"); \
        if ( (!forkAssert) || (0 == strcmp(forkAssert, "1")) ) { \
            PORT_Assert(0); \
        } \
    }

#else

#define FORK_ASSERT()

#endif







#if !defined (CHECK_FORK_MIXED) && !defined(CHECK_FORK_PTHREAD) && \
    !defined (CHECK_FORK_GETPID)






#ifdef SOLARIS



#define CHECK_FORK_MIXED

#elif defined(LINUX)

#define CHECK_FORK_PTHREAD

#else





#define CHECK_FORK_GETPID

#endif

#endif

#if defined(CHECK_FORK_MIXED)

extern PRBool usePthread_atfork;
#include <unistd.h>
extern pid_t myPid;
extern PRBool forked;

#define PARENT_FORKED() (usePthread_atfork ? forked : (myPid && myPid != getpid()))

#elif defined(CHECK_FORK_PTHREAD)

extern PRBool forked;

#define PARENT_FORKED() forked

#elif defined(CHECK_FORK_GETPID)

#include <unistd.h>
extern pid_t myPid;

#define PARENT_FORKED() (myPid && myPid != getpid())
    
#endif

extern PRBool parentForkedAfterC_Initialize;
extern PRBool sftkForkCheckDisabled;

#define CHECK_FORK() \
    do { \
        if (!sftkForkCheckDisabled && PARENT_FORKED()) { \
            FORK_ASSERT(); \
            return CKR_DEVICE_ERROR; \
        } \
    } while (0)

#define SKIP_AFTER_FORK(x) if (!parentForkedAfterC_Initialize) x

#define ENABLE_FORK_CHECK() \
    { \
        char* doForkCheck = getenv("NSS_STRICT_NOFORK"); \
        if ( doForkCheck && !strcmp(doForkCheck, "DISABLED") ) { \
            sftkForkCheckDisabled = PR_TRUE; \
        } \
    }


#else



#define CHECK_FORK()
#define SKIP_AFTER_FORK(x) x
#define ENABLE_FORK_CHECK()

#ifndef NO_FORK_CHECK
#define NO_FORK_CHECK
#endif

#endif


SEC_END_PROTOS

#endif 
