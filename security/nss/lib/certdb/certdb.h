



































#ifndef _CERTDB_H_
#define _CERTDB_H_



#define CERTDB_VALID_PEER	(1<<0)
#define CERTDB_TRUSTED		(1<<1)
#define CERTDB_SEND_WARN	(1<<2)
#define CERTDB_VALID_CA		(1<<3)
#define CERTDB_TRUSTED_CA	(1<<4) /* trusted for issuing server certs */
#define CERTDB_NS_TRUSTED_CA	(1<<5)
#define CERTDB_USER		(1<<6)
#define CERTDB_TRUSTED_CLIENT_CA (1<<7) /* trusted for issuing client certs */
#define CERTDB_INVISIBLE_CA	(1<<8) /* don't show in UI */
#define CERTDB_GOVT_APPROVED_CA	(1<<9) /* can do strong crypto in export ver */


SEC_BEGIN_PROTOS

CERTSignedCrl *
SEC_FindCrlByKey(CERTCertDBHandle *handle, SECItem *crlKey, int type);

CERTSignedCrl *
SEC_FindCrlByName(CERTCertDBHandle *handle, SECItem *crlKey, int type);

CERTSignedCrl *
SEC_FindCrlByDERCert(CERTCertDBHandle *handle, SECItem *derCrl, int type);

PRBool
SEC_CertNicknameConflict(const char *nickname, SECItem *derSubject,
			 CERTCertDBHandle *handle);
CERTSignedCrl *
SEC_NewCrl(CERTCertDBHandle *handle, char *url, SECItem *derCrl, int type);

SECStatus
SEC_DeletePermCRL(CERTSignedCrl *crl);


SECStatus
SEC_LookupCrls(CERTCertDBHandle *handle, CERTCrlHeadNode **nodes, int type);

SECStatus 
SEC_DestroyCrl(CERTSignedCrl *crl);

CERTSignedCrl* SEC_DupCrl(CERTSignedCrl* acrl);

SECStatus
CERT_AddTempCertToPerm(CERTCertificate *cert, char *nickname,
		       CERTCertTrust *trust);

SECStatus SEC_DeletePermCertificate(CERTCertificate *cert);

PRBool
SEC_CrlIsNewer(CERTCrl *inNew, CERTCrl *old);

SECCertTimeValidity
SEC_CheckCrlTimes(CERTCrl *crl, PRTime t);

SEC_END_PROTOS

#endif 
