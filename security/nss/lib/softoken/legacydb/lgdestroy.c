





#include "pkcs11.h"
#include "lgdb.h"
#include "pcert.h"
#include "lowkeyi.h"




CK_RV
lg_DestroyObject(SDB *sdb, CK_OBJECT_HANDLE object_id)
{
    CK_RV crv = CKR_OK;
    SECStatus rv;
    NSSLOWCERTCertificate *cert;
    NSSLOWCERTCertTrust tmptrust;
    PRBool isKrl;
    NSSLOWKEYDBHandle *keyHandle;
    NSSLOWCERTCertDBHandle *certHandle;
    const SECItem *dbKey;

    object_id &= ~LG_TOKEN_MASK;
    dbKey = lg_lookupTokenKeyByHandle(sdb,object_id);
    if (dbKey == NULL) {
	return CKR_OBJECT_HANDLE_INVALID;
    }

    
    switch (object_id & LG_TOKEN_TYPE_MASK) {
    case LG_TOKEN_TYPE_PRIV:
    case LG_TOKEN_TYPE_KEY:
	

	keyHandle = lg_getKeyDB(sdb);
	if (!keyHandle) {
	    crv = CKR_TOKEN_WRITE_PROTECTED;
	    break;
	}
	rv = nsslowkey_DeleteKey(keyHandle, dbKey);
	if (rv != SECSuccess) {
	    crv = CKR_DEVICE_ERROR;
	}
	break;
    case LG_TOKEN_TYPE_PUB:
	break; 
    case LG_TOKEN_TYPE_CERT:
	certHandle = lg_getCertDB(sdb);
	if (!certHandle) {
	    crv = CKR_TOKEN_WRITE_PROTECTED;
	    break;
	}
	cert = nsslowcert_FindCertByKey(certHandle,dbKey);
	if (cert == NULL) {
	    crv = CKR_DEVICE_ERROR;
	    break;
	}
	rv = nsslowcert_DeletePermCertificate(cert);
	if (rv != SECSuccess) {
	    crv = CKR_DEVICE_ERROR;
	}
	nsslowcert_DestroyCertificate(cert);
	break;
    case LG_TOKEN_TYPE_CRL:
	certHandle = lg_getCertDB(sdb);
	if (!certHandle) {
	    crv = CKR_TOKEN_WRITE_PROTECTED;
	    break;
	}
	isKrl = (PRBool) (object_id == LG_TOKEN_KRL_HANDLE);
	rv = nsslowcert_DeletePermCRL(certHandle, dbKey, isKrl);
	if (rv == SECFailure) crv = CKR_DEVICE_ERROR;
	break;
    case LG_TOKEN_TYPE_TRUST:
	certHandle = lg_getCertDB(sdb);
	if (!certHandle) {
	    crv = CKR_TOKEN_WRITE_PROTECTED;
	    break;
	}
	cert = nsslowcert_FindCertByKey(certHandle, dbKey);
	if (cert == NULL) {
	    crv = CKR_DEVICE_ERROR;
	    break;
	}
	tmptrust = *cert->trust;
	tmptrust.sslFlags &= CERTDB_PRESERVE_TRUST_BITS;
	tmptrust.emailFlags &= CERTDB_PRESERVE_TRUST_BITS;
	tmptrust.objectSigningFlags &= CERTDB_PRESERVE_TRUST_BITS;
	tmptrust.sslFlags |= CERTDB_TRUSTED_UNKNOWN;
	tmptrust.emailFlags |= CERTDB_TRUSTED_UNKNOWN;
	tmptrust.objectSigningFlags |= CERTDB_TRUSTED_UNKNOWN;
	rv = nsslowcert_ChangeCertTrust(certHandle, cert, &tmptrust);
	if (rv != SECSuccess) crv = CKR_DEVICE_ERROR;
	nsslowcert_DestroyCertificate(cert);
	break;
    default:
	break;
    }
    lg_DBLock(sdb);
    lg_deleteTokenKeyByHandle(sdb,object_id);
    lg_DBUnlock(sdb);

    return crv;
}



