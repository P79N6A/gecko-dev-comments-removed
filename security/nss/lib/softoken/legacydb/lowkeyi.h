




#ifndef _LOWKEYI_H_
#define _LOWKEYI_H_

#include "prtypes.h"
#include "seccomon.h"
#include "secoidt.h"
#include "pcertt.h"
#include "lowkeyti.h"
#include "sdb.h" 

SEC_BEGIN_PROTOS









extern void lg_prepare_low_rsa_priv_key_for_asn1(NSSLOWKEYPrivateKey *key);
extern void lg_prepare_low_pqg_params_for_asn1(PQGParams *params);
extern void lg_prepare_low_dsa_priv_key_for_asn1(NSSLOWKEYPrivateKey *key);
extern void lg_prepare_low_dh_priv_key_for_asn1(NSSLOWKEYPrivateKey *key);
#ifdef NSS_ENABLE_ECC
extern void lg_prepare_low_ec_priv_key_for_asn1(NSSLOWKEYPrivateKey *key);
extern void lg_prepare_low_ecparams_for_asn1(ECParams *params);
#endif 

typedef char * (* NSSLOWKEYDBNameFunc)(void *arg, int dbVersion);
    



extern NSSLOWKEYDBHandle *nsslowkey_OpenKeyDB(PRBool readOnly,
					   const char *domain,
					   const char *prefix,
					   NSSLOWKEYDBNameFunc namecb,
					   void *cbarg);




extern void nsslowkey_CloseKeyDB(NSSLOWKEYDBHandle *handle);




extern int nsslowkey_GetKeyDBVersion(NSSLOWKEYDBHandle *handle);




extern SECStatus nsslowkey_DeleteKey(NSSLOWKEYDBHandle *handle, 
				  const SECItem *pubkey);







extern SECStatus nsslowkey_StoreKeyByPublicKey(NSSLOWKEYDBHandle *handle, 
					    NSSLOWKEYPrivateKey *pk,
					    SECItem *pubKeyData,
					    char *nickname,
					    SDB *sdb);


extern PRBool nsslowkey_KeyForCertExists(NSSLOWKEYDBHandle *handle,
					 NSSLOWCERTCertificate *cert);

extern PRBool nsslowkey_KeyForIDExists(NSSLOWKEYDBHandle *handle, SECItem *id);






extern void lg_nsslowkey_DestroyPrivateKey(NSSLOWKEYPrivateKey *key);






extern void lg_nsslowkey_DestroyPublicKey(NSSLOWKEYPublicKey *key);





extern NSSLOWKEYPublicKey 
	*lg_nsslowkey_ConvertToPublicKey(NSSLOWKEYPrivateKey *privateKey);


SECStatus
nsslowkey_UpdateNickname(NSSLOWKEYDBHandle *handle,
                           NSSLOWKEYPrivateKey *privkey,
                           SECItem *pubKeyData,
                           char *nickname,
                           SDB *sdb);









extern SECStatus 
nsslowkey_StoreKeyByPublicKeyAlg(NSSLOWKEYDBHandle *handle, 
			      NSSLOWKEYPrivateKey *privkey, 
			      SECItem *pubKeyData,
			      char *nickname,
			      SDB *sdb,
                              PRBool update); 







extern NSSLOWKEYPrivateKey *
nsslowkey_FindKeyByPublicKey(NSSLOWKEYDBHandle *handle, SECItem *modulus, 
			  SDB *sdb);

extern char *
nsslowkey_FindKeyNicknameByPublicKey(NSSLOWKEYDBHandle *handle,
                                        SECItem *modulus, SDB *sdb);

#ifdef NSS_ENABLE_ECC




SECStatus LGEC_FillParams(PRArenaPool *arena, const SECItem *encodedParams, 
    ECParams *params);


SECStatus LGEC_CopyParams(PRArenaPool *arena, ECParams *dstParams,
	      const ECParams *srcParams);
#endif
SEC_END_PROTOS

#endif 
