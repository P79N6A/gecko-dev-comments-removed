









#include "cmslocal.h"
#include "smime.h"

#include "cert.h"
#include "key.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "prtime.h"
#include "secerr.h"


#if 0

















NSSCMSMessage *
NSS_SMIMEMessage_CreateEncrypted(CERTCertificate *scert,
			CERTCertificate **rcerts,
			CERTCertDBHandle *certdb,
			PK11PasswordFunc pwfn,
			void *pwfn_arg)
{
    NSSCMSMessage *cmsg;
    long cipher;
    SECOidTag encalg;
    int keysize;
    int mapi, rci;

    cipher = smime_choose_cipher (scert, rcerts);
    if (cipher < 0)
	return NULL;

    mapi = smime_mapi_by_cipher (cipher);
    if (mapi < 0)
	return NULL;

    







    encalg = smime_cipher_map[mapi].algtag;
    keysize = smime_keysize_by_cipher (cipher);
    if (keysize < 0)
	return NULL;

    cinfo = SEC_PKCS7CreateEnvelopedData (scert, certUsageEmailRecipient,
					  certdb, encalg, keysize,
					  pwfn, pwfn_arg);
    if (cinfo == NULL)
	return NULL;

    for (rci = 0; rcerts[rci] != NULL; rci++) {
	if (rcerts[rci] == scert)
	    continue;
	if (SEC_PKCS7AddRecipient (cinfo, rcerts[rci], certUsageEmailRecipient,
				   NULL) != SECSuccess) {
	    SEC_PKCS7DestroyContentInfo (cinfo);
	    return NULL;
	}
    }

    return cinfo;
}
































NSSCMSMessage *
NSS_SMIMEMessage_CreateSigned(CERTCertificate *scert,
		      CERTCertificate *ecert,
		      CERTCertDBHandle *certdb,
		      SECOidTag digestalgtag,
		      SECItem *digest,
		      PK11PasswordFunc pwfn,
		      void *pwfn_arg)
{
    NSSCMSMessage *cmsg;
    NSSCMSSignedData *sigd;
    NSSCMSSignerInfo *signerinfo;

    
    

    cmsg = NSS_CMSMessage_Create(NULL);
    if (cmsg == NULL)
	return NULL;

    sigd = NSS_CMSSignedData_Create(cmsg);
    if (sigd == NULL)
	goto loser;

    
    signerinfo = NSS_CMSSignerInfo_Create(cmsg, scert, digestalgtag);
    if (signerinfo == NULL)
	goto loser;

    
    if (NSS_CMSSignerInfo_AddSigningTime(signerinfo, PR_Now()) != SECSuccess)
	goto loser;
    
    
    if (NSS_SMIMESignerInfo_AddSMIMEProfile(signerinfo, scert) != SECSuccess)
	goto loser;

    
    if (NSS_CMSSignedData_AddSignerInfo(sigd, signerinfo) != SECSuccess)
	goto loser;

    
    
    
    if (NSS_CMSSignedData_AddCertChain(sigd, scert) != SECSuccess)
	goto loser;

    

    if ( ( ecert != NULL ) && ( ecert != scert ) ) {
	if (NSS_CMSSignedData_AddCertificate(sigd, ecert) != SECSuccess)
	    goto loser;
    }

    return cmsg;
loser:
    if (cmsg)
	NSS_CMSMessage_Destroy(cmsg);
    return NULL;
}
#endif
