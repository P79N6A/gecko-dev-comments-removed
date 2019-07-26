




#ifndef _LOWKEYI_H_
#define _LOWKEYI_H_

#include "prtypes.h"
#include "seccomon.h"
#include "secoidt.h"
#include "lowkeyti.h"

SEC_BEGIN_PROTOS









extern void prepare_low_rsa_priv_key_for_asn1(NSSLOWKEYPrivateKey *key);
extern void prepare_low_pqg_params_for_asn1(PQGParams *params);
extern void prepare_low_dsa_priv_key_for_asn1(NSSLOWKEYPrivateKey *key);
extern void prepare_low_dsa_priv_key_export_for_asn1(NSSLOWKEYPrivateKey *key);
extern void prepare_low_dh_priv_key_for_asn1(NSSLOWKEYPrivateKey *key);
#ifdef NSS_ENABLE_ECC
extern void prepare_low_ec_priv_key_for_asn1(NSSLOWKEYPrivateKey *key);
extern void prepare_low_ecparams_for_asn1(ECParams *params);
#endif 






extern void nsslowkey_DestroyPrivateKey(NSSLOWKEYPrivateKey *key);






extern void nsslowkey_DestroyPublicKey(NSSLOWKEYPublicKey *key);




extern unsigned int nsslowkey_PublicModulusLen(NSSLOWKEYPublicKey *pubKey);





extern unsigned int nsslowkey_PrivateModulusLen(NSSLOWKEYPrivateKey *privKey);





extern NSSLOWKEYPublicKey 
		*nsslowkey_ConvertToPublicKey(NSSLOWKEYPrivateKey *privateKey);




extern NSSLOWKEYPrivateKey *
nsslowkey_CopyPrivateKey(NSSLOWKEYPrivateKey *privKey);


SEC_END_PROTOS

#endif 
