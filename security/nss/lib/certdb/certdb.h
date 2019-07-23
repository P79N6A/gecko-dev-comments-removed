



































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
SEC_CertNicknameConflict(char *nickname, SECItem *derSubject,
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

#ifdef notdef






SECStatus SEC_AddPermCertificate(PCERTCertDBHandle *handle, SECItem *derCert,
				char *nickname, PCERTCertTrust *trust);

certDBEntryCert *
SEC_FindPermCertByKey(PCERTCertDBHandle *handle, SECItem *certKey);

certDBEntryCert
*SEC_FindPermCertByName(PCERTCertDBHandle *handle, SECItem *name);

SECStatus SEC_OpenPermCertDB(PCERTCertDBHandle *handle,
			     PRBool readOnly,
			     PCERTDBNameFunc namecb,
			     void *cbarg);


typedef SECStatus (PR_CALLBACK * PermCertCallback)(PCERTCertificate *cert,
                                                   SECItem *k, void *pdata);






SECStatus
PCERT_TraversePermCerts(PCERTCertDBHandle *handle,
		      PermCertCallback certfunc,
		      void *udata );

SECStatus
SEC_AddTempNickname(PCERTCertDBHandle *handle, char *nickname, SECItem *certKey);

SECStatus
SEC_DeleteTempNickname(PCERTCertDBHandle *handle, char *nickname);


PRBool
SEC_CertDBKeyConflict(SECItem *derCert, PCERTCertDBHandle *handle);

SECStatus
SEC_GetCrlTimes(PCERTCrl *dates, PRTime *notBefore, PRTime *notAfter);

PCERTSignedCrl *
SEC_AddPermCrlToTemp(PCERTCertDBHandle *handle, certDBEntryRevocation *entry);

SECStatus
SEC_DeleteTempCrl(PCERTSignedCrl *crl);


SECStatus
SEC_CheckKRL(PCERTCertDBHandle *handle,SECKEYLowPublicKey *key,
	     PCERTCertificate *rootCert, int64 t, void *wincx);

SECStatus
SEC_CheckCRL(PCERTCertDBHandle *handle,PCERTCertificate *cert,
	     PCERTCertificate *caCert, int64 t, void *wincx);

SECStatus
SEC_CrlReplaceUrl(PCERTSignedCrl *crl,char *url);




CERTCompareValidityStatus
CERT_CompareValidityTimes(CERTValidity* val_a, CERTValidity* val_b);

#endif

SEC_END_PROTOS

#endif 
