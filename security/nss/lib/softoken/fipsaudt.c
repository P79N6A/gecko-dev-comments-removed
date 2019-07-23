








































#include "prprf.h"
#include "softoken.h"














static void sftk_PrintReturnedObjectHandle(char *out, PRUint32 outlen,
    const char *argName, CK_OBJECT_HANDLE_PTR phObject, CK_RV rv)
{
    if ((rv == CKR_OK) && phObject) {
	PR_snprintf(out, outlen,
	    " *%s=0x%08lX", argName, (PRUint32)*phObject);
    } else {
	PORT_Assert(outlen != 0);
	out[0] = '\0';
    }
}





#define MECHANISM_BUFSIZE 64

static void sftk_PrintMechanism(char *out, PRUint32 outlen,
    CK_MECHANISM_PTR pMechanism)
{
    if (pMechanism) {
	




	PR_snprintf(out, outlen, "%p {mechanism=0x%08lX, ...}",
	    pMechanism, (PRUint32)pMechanism->mechanism);
    } else {
	PR_snprintf(out, outlen, "%p", pMechanism);
    }
}

void sftk_AuditCreateObject(CK_SESSION_HANDLE hSession,
    CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount,
    CK_OBJECT_HANDLE_PTR phObject, CK_RV rv)
{
    char msg[256];
    char shObject[32];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    sftk_PrintReturnedObjectHandle(shObject, sizeof shObject,
	"phObject", phObject, rv);
    PR_snprintf(msg, sizeof msg,
	"C_CreateObject(hSession=0x%08lX, pTemplate=%p, ulCount=%lu, "
	"phObject=%p)=0x%08lX%s",
	(PRUint32)hSession, pTemplate, (PRUint32)ulCount,
	phObject, (PRUint32)rv, shObject);
    sftk_LogAuditMessage(severity, msg);
}

void sftk_AuditCopyObject(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount,
    CK_OBJECT_HANDLE_PTR phNewObject, CK_RV rv)
{
    char msg[256];
    char shNewObject[32];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    sftk_PrintReturnedObjectHandle(shNewObject, sizeof shNewObject,
	"phNewObject", phNewObject, rv);
    PR_snprintf(msg, sizeof msg,
	"C_CopyObject(hSession=0x%08lX, hObject=0x%08lX, "
	"pTemplate=%p, ulCount=%lu, phNewObject=%p)=0x%08lX%s",
	(PRUint32)hSession, (PRUint32)hObject,
	pTemplate, (PRUint32)ulCount, phNewObject, (PRUint32)rv, shNewObject);
    sftk_LogAuditMessage(severity, msg);
}


void sftk_AuditDestroyObject(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject, CK_RV rv)
{
    char msg[256];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    PR_snprintf(msg, sizeof msg,
	"C_DestroyObject(hSession=0x%08lX, hObject=0x%08lX)=0x%08lX",
	(PRUint32)hSession, (PRUint32)hObject, (PRUint32)rv);
    sftk_LogAuditMessage(severity, msg);
}

void sftk_AuditGetObjectSize(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject, CK_ULONG_PTR pulSize, CK_RV rv)
{
    char msg[256];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    PR_snprintf(msg, sizeof msg,
	"C_GetObjectSize(hSession=0x%08lX, hObject=0x%08lX, "
	"pulSize=%p)=0x%08lX",
	(PRUint32)hSession, (PRUint32)hObject,
	pulSize, (PRUint32)rv);
    sftk_LogAuditMessage(severity, msg);
}

void sftk_AuditGetAttributeValue(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulCount, CK_RV rv)
{
    char msg[256];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    PR_snprintf(msg, sizeof msg,
	"C_GetAttributeValue(hSession=0x%08lX, hObject=0x%08lX, "
	"pTemplate=%p, ulCount=%lu)=0x%08lX",
	(PRUint32)hSession, (PRUint32)hObject,
	pTemplate, (PRUint32)ulCount, (PRUint32)rv);
    sftk_LogAuditMessage(severity, msg);
}

void sftk_AuditSetAttributeValue(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulCount, CK_RV rv)
{
    char msg[256];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    PR_snprintf(msg, sizeof msg,
	"C_SetAttributeValue(hSession=0x%08lX, hObject=0x%08lX, "
	"pTemplate=%p, ulCount=%lu)=0x%08lX",
	(PRUint32)hSession, (PRUint32)hObject,
	pTemplate, (PRUint32)ulCount, (PRUint32)rv);
    sftk_LogAuditMessage(severity, msg);
}

void sftk_AuditCryptInit(const char *opName, CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey, CK_RV rv)
{
    char msg[256];
    char mech[MECHANISM_BUFSIZE];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    sftk_PrintMechanism(mech, sizeof mech, pMechanism);
    PR_snprintf(msg, sizeof msg,
	"C_%sInit(hSession=0x%08lX, pMechanism=%s, "
	"hKey=0x%08lX)=0x%08lX",
	opName, (PRUint32)hSession, mech,
	(PRUint32)hKey, (PRUint32)rv);
    sftk_LogAuditMessage(severity, msg);
}

void sftk_AuditGenerateKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulCount, CK_OBJECT_HANDLE_PTR phKey, CK_RV rv)
{
    char msg[256];
    char mech[MECHANISM_BUFSIZE];
    char shKey[32];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    sftk_PrintMechanism(mech, sizeof mech, pMechanism);
    sftk_PrintReturnedObjectHandle(shKey, sizeof shKey, "phKey", phKey, rv);
    PR_snprintf(msg, sizeof msg,
	"C_GenerateKey(hSession=0x%08lX, pMechanism=%s, "
	"pTemplate=%p, ulCount=%lu, phKey=%p)=0x%08lX%s",
	(PRUint32)hSession, mech,
	pTemplate, (PRUint32)ulCount, phKey, (PRUint32)rv, shKey);
    sftk_LogAuditMessage(severity, msg);
}

void sftk_AuditGenerateKeyPair(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_ATTRIBUTE_PTR pPublicKeyTemplate,
    CK_ULONG ulPublicKeyAttributeCount, CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
    CK_ULONG ulPrivateKeyAttributeCount, CK_OBJECT_HANDLE_PTR phPublicKey,
    CK_OBJECT_HANDLE_PTR phPrivateKey, CK_RV rv)
{
    char msg[512];
    char mech[MECHANISM_BUFSIZE];
    char shPublicKey[32];
    char shPrivateKey[32];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    sftk_PrintMechanism(mech, sizeof mech, pMechanism);
    sftk_PrintReturnedObjectHandle(shPublicKey, sizeof shPublicKey,
	"phPublicKey", phPublicKey, rv);
    sftk_PrintReturnedObjectHandle(shPrivateKey, sizeof shPrivateKey,
	"phPrivateKey", phPrivateKey, rv);
    PR_snprintf(msg, sizeof msg,
	"C_GenerateKeyPair(hSession=0x%08lX, pMechanism=%s, "
	"pPublicKeyTemplate=%p, ulPublicKeyAttributeCount=%lu, "
	"pPrivateKeyTemplate=%p, ulPrivateKeyAttributeCount=%lu, "
	"phPublicKey=%p, phPrivateKey=%p)=0x%08lX%s%s",
	(PRUint32)hSession, mech,
	pPublicKeyTemplate, (PRUint32)ulPublicKeyAttributeCount,
	pPrivateKeyTemplate, (PRUint32)ulPrivateKeyAttributeCount,
	phPublicKey, phPrivateKey, (PRUint32)rv, shPublicKey, shPrivateKey);
    sftk_LogAuditMessage(severity, msg);
}

void sftk_AuditWrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hWrappingKey,
    CK_OBJECT_HANDLE hKey, CK_BYTE_PTR pWrappedKey,
    CK_ULONG_PTR pulWrappedKeyLen, CK_RV rv)
{
    char msg[256];
    char mech[MECHANISM_BUFSIZE];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    sftk_PrintMechanism(mech, sizeof mech, pMechanism);
    PR_snprintf(msg, sizeof msg,
	"C_WrapKey(hSession=0x%08lX, pMechanism=%s, hWrappingKey=0x%08lX, "
	"hKey=0x%08lX, pWrappedKey=%p, pulWrappedKeyLen=%p)=0x%08lX",
	(PRUint32)hSession, mech, (PRUint32)hWrappingKey,
	(PRUint32)hKey, pWrappedKey, pulWrappedKeyLen, (PRUint32)rv);
    sftk_LogAuditMessage(severity, msg);
}

void sftk_AuditUnwrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hUnwrappingKey,
    CK_BYTE_PTR pWrappedKey, CK_ULONG ulWrappedKeyLen,
    CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount,
    CK_OBJECT_HANDLE_PTR phKey, CK_RV rv)
{
    char msg[256];
    char mech[MECHANISM_BUFSIZE];
    char shKey[32];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    sftk_PrintMechanism(mech, sizeof mech, pMechanism);
    sftk_PrintReturnedObjectHandle(shKey, sizeof shKey, "phKey", phKey, rv);
    PR_snprintf(msg, sizeof msg,
	"C_UnwrapKey(hSession=0x%08lX, pMechanism=%s, "
	"hUnwrappingKey=0x%08lX, pWrappedKey=%p, ulWrappedKeyLen=%lu, "
	"pTemplate=%p, ulAttributeCount=%lu, phKey=%p)=0x%08lX%s",
	(PRUint32)hSession, mech,
	(PRUint32)hUnwrappingKey, pWrappedKey, (PRUint32)ulWrappedKeyLen,
	pTemplate, (PRUint32)ulAttributeCount, phKey, (PRUint32)rv, shKey);
    sftk_LogAuditMessage(severity, msg);
}

void sftk_AuditDeriveKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hBaseKey,
    CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount,
    CK_OBJECT_HANDLE_PTR phKey, CK_RV rv)
{
    char msg[512];
    char mech[MECHANISM_BUFSIZE];
    char shKey[32];
    char sTlsKeys[128];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    sftk_PrintMechanism(mech, sizeof mech, pMechanism);
    sftk_PrintReturnedObjectHandle(shKey, sizeof shKey, "phKey", phKey, rv);
    if ((rv == CKR_OK) &&
	(pMechanism->mechanism == CKM_TLS_KEY_AND_MAC_DERIVE)) {
	CK_SSL3_KEY_MAT_PARAMS *param =
	    (CK_SSL3_KEY_MAT_PARAMS *)pMechanism->pParameter;
	CK_SSL3_KEY_MAT_OUT *keymat = param->pReturnedKeyMaterial;
	PR_snprintf(sTlsKeys, sizeof sTlsKeys,
	    " hClientMacSecret=0x%08lX hServerMacSecret=0x%08lX"
	    " hClientKey=0x%08lX hServerKey=0x%08lX",
	    (PRUint32)keymat->hClientMacSecret,
	    (PRUint32)keymat->hServerMacSecret,
	    (PRUint32)keymat->hClientKey,
	    (PRUint32)keymat->hServerKey);
    } else {
	sTlsKeys[0] = '\0';
    }
    PR_snprintf(msg, sizeof msg,
	"C_DeriveKey(hSession=0x%08lX, pMechanism=%s, "
	"hBaseKey=0x%08lX, pTemplate=%p, ulAttributeCount=%lu, "
	"phKey=%p)=0x%08lX%s%s",
	(PRUint32)hSession, mech,
	(PRUint32)hBaseKey, pTemplate,(PRUint32)ulAttributeCount,
	phKey, (PRUint32)rv, shKey, sTlsKeys);
    sftk_LogAuditMessage(severity, msg);
}

void sftk_AuditDigestKey(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hKey, CK_RV rv)
{
    char msg[256];
    NSSAuditSeverity severity = (rv == CKR_OK) ?
	NSS_AUDIT_INFO : NSS_AUDIT_ERROR;

    PR_snprintf(msg, sizeof msg,
	"C_DigestKey(hSession=0x%08lX, hKey=0x%08lX)=0x%08lX",
	(PRUint32)hSession, (PRUint32)hKey, (PRUint32)rv);
    sftk_LogAuditMessage(severity, msg);
}
