









#include "cert.h"
#include "ssl.h"
#include "cryptohi.h"	
#include "keyhi.h"
#include "secder.h"
#include "secitem.h"
#include "sechash.h"

#include "sslimpl.h"
#include "sslproto.h"
#include "sslerr.h"
#include "prtime.h"
#include "prinrval.h"
#include "prerror.h"
#include "pratom.h"
#include "prthread.h"

#include "pk11func.h"
#include "secmod.h"
#ifndef NO_PKCS11_BYPASS
#include "blapi.h"
#endif

#include <stdio.h>
#ifdef NSS_ENABLE_ZLIB
#include "zlib.h"
#endif

#ifndef PK11_SETATTRS
#define PK11_SETATTRS(x,id,v,l) (x)->type = (id); \
		(x)->pValue=(v); (x)->ulValueLen = (l);
#endif

static SECStatus ssl3_AuthCertificate(sslSocket *ss);
static void      ssl3_CleanupPeerCerts(sslSocket *ss);
static PK11SymKey *ssl3_GenerateRSAPMS(sslSocket *ss, ssl3CipherSpec *spec,
                                       PK11SlotInfo * serverKeySlot);
static SECStatus ssl3_DeriveMasterSecret(sslSocket *ss, PK11SymKey *pms);
static SECStatus ssl3_DeriveConnectionKeysPKCS11(sslSocket *ss);
static SECStatus ssl3_HandshakeFailure(      sslSocket *ss);
static SECStatus ssl3_InitState(             sslSocket *ss);
static SECStatus ssl3_SendCertificate(       sslSocket *ss);
static SECStatus ssl3_SendCertificateStatus( sslSocket *ss);
static SECStatus ssl3_SendEmptyCertificate(  sslSocket *ss);
static SECStatus ssl3_SendCertificateRequest(sslSocket *ss);
static SECStatus ssl3_SendNextProto(         sslSocket *ss);
static SECStatus ssl3_SendFinished(          sslSocket *ss, PRInt32 flags);
static SECStatus ssl3_SendServerHello(       sslSocket *ss);
static SECStatus ssl3_SendServerHelloDone(   sslSocket *ss);
static SECStatus ssl3_SendServerKeyExchange( sslSocket *ss);
static SECStatus ssl3_UpdateHandshakeHashes( sslSocket *ss,
                                             const unsigned char *b,
                                             unsigned int l);
static SECStatus ssl3_FlushHandshakeMessages(sslSocket *ss, PRInt32 flags);
static int       ssl3_OIDToTLSHashAlgorithm(SECOidTag oid);

static SECStatus Null_Cipher(void *ctx, unsigned char *output, int *outputLen,
			     int maxOutputLen, const unsigned char *input,
			     int inputLen);
#ifndef NO_PKCS11_BYPASS
static SECStatus ssl3_AESGCMBypass(ssl3KeyMaterial *keys, PRBool doDecrypt,
				   unsigned char *out, int *outlen, int maxout,
				   const unsigned char *in, int inlen,
				   const unsigned char *additionalData,
				   int additionalDataLen);
#endif

#define MAX_SEND_BUF_LENGTH 32000 /* watch for 16-bit integer overflow */
#define MIN_SEND_BUF_LENGTH  4000









static ssl3CipherSuiteCfg cipherSuites[ssl_V3_SUITES_IMPLEMENTED] = {
   

#ifndef NSS_DISABLE_ECC
 { TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,   SSL_ALLOWED, PR_FALSE, PR_FALSE},
   


 { TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,    SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,    SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,      SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256, SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,   SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,      SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA,   SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA,     SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDHE_ECDSA_WITH_RC4_128_SHA,        SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDHE_RSA_WITH_RC4_128_SHA,          SSL_ALLOWED, PR_FALSE, PR_FALSE},
#endif 

 { TLS_DHE_RSA_WITH_AES_128_GCM_SHA256,     SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_DHE_RSA_WITH_AES_128_CBC_SHA,        SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_DHE_DSS_WITH_AES_128_CBC_SHA,        SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_DHE_RSA_WITH_AES_128_CBC_SHA256,     SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA,   SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA,   SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_DHE_RSA_WITH_AES_256_CBC_SHA,        SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_DHE_DSS_WITH_AES_256_CBC_SHA,        SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_DHE_RSA_WITH_AES_256_CBC_SHA256,     SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA,   SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA,   SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA,       SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA,       SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_DHE_DSS_WITH_RC4_128_SHA,            SSL_ALLOWED, PR_FALSE, PR_FALSE},

#ifndef NSS_DISABLE_ECC
 { TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA,     SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,       SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA,     SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDH_RSA_WITH_AES_256_CBC_SHA,       SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA,    SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA,      SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDH_ECDSA_WITH_RC4_128_SHA,         SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDH_RSA_WITH_RC4_128_SHA,           SSL_ALLOWED, PR_FALSE, PR_FALSE},
#endif 

 
 { TLS_RSA_WITH_AES_128_GCM_SHA256,         SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_RSA_WITH_AES_128_CBC_SHA,            SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_RSA_WITH_AES_128_CBC_SHA256,         SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_RSA_WITH_CAMELLIA_128_CBC_SHA,       SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_RSA_WITH_AES_256_CBC_SHA,            SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_RSA_WITH_AES_256_CBC_SHA256,         SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_RSA_WITH_CAMELLIA_256_CBC_SHA,       SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_RSA_WITH_SEED_CBC_SHA,               SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA,      SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_RSA_WITH_3DES_EDE_CBC_SHA,           SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_RSA_WITH_RC4_128_SHA,                SSL_ALLOWED, PR_TRUE,  PR_FALSE},
 { TLS_RSA_WITH_RC4_128_MD5,                SSL_ALLOWED, PR_TRUE,  PR_FALSE},

 
 { TLS_DHE_RSA_WITH_DES_CBC_SHA,            SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_DHE_DSS_WITH_DES_CBC_SHA,            SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { SSL_RSA_FIPS_WITH_DES_CBC_SHA,           SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_RSA_WITH_DES_CBC_SHA,                SSL_ALLOWED, PR_FALSE, PR_FALSE},

 
 { TLS_RSA_EXPORT1024_WITH_RC4_56_SHA,      SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA,     SSL_ALLOWED, PR_FALSE, PR_FALSE},

 
 { TLS_RSA_EXPORT_WITH_RC4_40_MD5,          SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5,      SSL_ALLOWED, PR_FALSE, PR_FALSE},

 
#ifndef NSS_DISABLE_ECC
 { TLS_ECDHE_ECDSA_WITH_NULL_SHA,           SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDHE_RSA_WITH_NULL_SHA,             SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDH_RSA_WITH_NULL_SHA,              SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_ECDH_ECDSA_WITH_NULL_SHA,            SSL_ALLOWED, PR_FALSE, PR_FALSE},
#endif 
 { TLS_RSA_WITH_NULL_SHA,                   SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_RSA_WITH_NULL_SHA256,                SSL_ALLOWED, PR_FALSE, PR_FALSE},
 { TLS_RSA_WITH_NULL_MD5,                   SSL_ALLOWED, PR_FALSE, PR_FALSE},
};



#ifdef DEBUG
void ssl3_CheckCipherSuiteOrderConsistency()
{
    unsigned int i;

    


    PORT_Assert(SSL_NumImplementedCiphers >= PR_ARRAY_SIZE(cipherSuites));

    for (i = 0; i < PR_ARRAY_SIZE(cipherSuites); ++i) {
        PORT_Assert(SSL_ImplementedCiphers[i] == cipherSuites[i].cipher_suite);
    }
}
#endif





static const  PRUint8 compressions [] = {
#ifdef NSS_ENABLE_ZLIB
    ssl_compression_deflate,
#endif
    ssl_compression_null
};

static const int compressionMethodsCount =
    sizeof(compressions) / sizeof(compressions[0]);



static PRBool
compressionEnabled(sslSocket *ss, SSLCompressionMethod compression)
{
    switch (compression) {
    case ssl_compression_null:
	return PR_TRUE;  
#ifdef NSS_ENABLE_ZLIB
    case ssl_compression_deflate:
        if (ss->version < SSL_LIBRARY_VERSION_TLS_1_3) {
            return ss->opt.enableDeflate;
        }
        return PR_FALSE;
#endif
    default:
	return PR_FALSE;
    }
}

static const  PRUint8 certificate_types [] = {
    ct_RSA_sign,
#ifndef NSS_DISABLE_ECC
    ct_ECDSA_sign,
#endif 
    ct_DSS_sign,
};







static const PRUint8 supported_signature_algorithms[] = {
    tls_hash_sha256, tls_sig_rsa,
#ifndef NSS_DISABLE_ECC
    tls_hash_sha256, tls_sig_ecdsa,
#endif
    tls_hash_sha256, tls_sig_dsa,
};

#define EXPORT_RSA_KEY_LENGTH 64	/* bytes */





CERTDistNames *ssl3_server_ca_list = NULL;
static SSL3Statistics ssl3stats;


static const ssl3BulkCipherDef bulk_cipher_defs[] = {
    
    
    
    
    
    
    
    {cipher_null,         calg_null,         0, 0, type_stream, 0, 0, 0, 0},
    {cipher_rc4,          calg_rc4,         16,16, type_stream, 0, 0, 0, 0},
    {cipher_rc4_40,       calg_rc4,         16, 5, type_stream, 0, 0, 0, 0},
    {cipher_rc4_56,       calg_rc4,         16, 7, type_stream, 0, 0, 0, 0},
    {cipher_rc2,          calg_rc2,         16,16, type_block,  8, 8, 0, 0},
    {cipher_rc2_40,       calg_rc2,         16, 5, type_block,  8, 8, 0, 0},
    {cipher_des,          calg_des,          8, 8, type_block,  8, 8, 0, 0},
    {cipher_3des,         calg_3des,        24,24, type_block,  8, 8, 0, 0},
    {cipher_des40,        calg_des,          8, 5, type_block,  8, 8, 0, 0},
    {cipher_idea,         calg_idea,        16,16, type_block,  8, 8, 0, 0},
    {cipher_aes_128,      calg_aes,         16,16, type_block, 16,16, 0, 0},
    {cipher_aes_256,      calg_aes,         32,32, type_block, 16,16, 0, 0},
    {cipher_camellia_128, calg_camellia,    16,16, type_block, 16,16, 0, 0},
    {cipher_camellia_256, calg_camellia,    32,32, type_block, 16,16, 0, 0},
    {cipher_seed,         calg_seed,        16,16, type_block, 16,16, 0, 0},
    {cipher_aes_128_gcm,  calg_aes_gcm,     16,16, type_aead,   4, 0,16, 8},
    {cipher_missing,      calg_null,         0, 0, type_stream, 0, 0, 0, 0},
};

static const ssl3KEADef kea_defs[] =
{ 
    
    {kea_null,           kt_null, sign_null,  PR_FALSE,   0, PR_FALSE, PR_FALSE},
    {kea_rsa,            kt_rsa,  sign_rsa,   PR_FALSE,   0, PR_FALSE, PR_FALSE},
    {kea_rsa_export,     kt_rsa,  sign_rsa,   PR_TRUE,  512, PR_FALSE, PR_TRUE},
    {kea_rsa_export_1024,kt_rsa,  sign_rsa,   PR_TRUE, 1024, PR_FALSE, PR_TRUE},
    {kea_dh_dss,         kt_dh,   sign_dsa,   PR_FALSE,   0, PR_FALSE, PR_FALSE},
    {kea_dh_dss_export,  kt_dh,   sign_dsa,   PR_TRUE,  512, PR_FALSE, PR_FALSE},
    {kea_dh_rsa,         kt_dh,   sign_rsa,   PR_FALSE,   0, PR_FALSE, PR_FALSE},
    {kea_dh_rsa_export,  kt_dh,   sign_rsa,   PR_TRUE,  512, PR_FALSE, PR_FALSE},
    {kea_dhe_dss,        kt_dh,   sign_dsa,   PR_FALSE,   0, PR_FALSE, PR_TRUE},
    {kea_dhe_dss_export, kt_dh,   sign_dsa,   PR_TRUE,  512, PR_FALSE, PR_TRUE},
    {kea_dhe_rsa,        kt_dh,   sign_rsa,   PR_FALSE,   0, PR_FALSE, PR_TRUE},
    {kea_dhe_rsa_export, kt_dh,   sign_rsa,   PR_TRUE,  512, PR_FALSE, PR_TRUE},
    {kea_dh_anon,        kt_dh,   sign_null,  PR_FALSE,   0, PR_FALSE, PR_TRUE},
    {kea_dh_anon_export, kt_dh,   sign_null,  PR_TRUE,  512, PR_FALSE, PR_TRUE},
    {kea_rsa_fips,       kt_rsa,  sign_rsa,   PR_FALSE,   0, PR_TRUE,  PR_FALSE},
#ifndef NSS_DISABLE_ECC
    {kea_ecdh_ecdsa,     kt_ecdh, sign_ecdsa, PR_FALSE,   0, PR_FALSE, PR_FALSE},
    {kea_ecdhe_ecdsa,    kt_ecdh, sign_ecdsa, PR_FALSE,   0, PR_FALSE, PR_TRUE},
    {kea_ecdh_rsa,       kt_ecdh, sign_rsa,   PR_FALSE,   0, PR_FALSE, PR_FALSE},
    {kea_ecdhe_rsa,      kt_ecdh, sign_rsa,   PR_FALSE,   0, PR_FALSE, PR_TRUE},
    {kea_ecdh_anon,      kt_ecdh, sign_null,  PR_FALSE,   0, PR_FALSE, PR_TRUE},
#endif 
};


static const ssl3CipherSuiteDef cipher_suite_defs[] = 
{


    {TLS_NULL_WITH_NULL_NULL,       cipher_null,   mac_null, kea_null},
    {TLS_RSA_WITH_NULL_MD5,         cipher_null,   mac_md5, kea_rsa},
    {TLS_RSA_WITH_NULL_SHA,         cipher_null,   mac_sha, kea_rsa},
    {TLS_RSA_WITH_NULL_SHA256,      cipher_null,   hmac_sha256, kea_rsa},
    {TLS_RSA_EXPORT_WITH_RC4_40_MD5,cipher_rc4_40, mac_md5, kea_rsa_export},
    {TLS_RSA_WITH_RC4_128_MD5,      cipher_rc4,    mac_md5, kea_rsa},
    {TLS_RSA_WITH_RC4_128_SHA,      cipher_rc4,    mac_sha, kea_rsa},
    {TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5,
                                    cipher_rc2_40, mac_md5, kea_rsa_export},
#if 0 
    {TLS_RSA_WITH_IDEA_CBC_SHA,     cipher_idea,   mac_sha, kea_rsa},
    {TLS_RSA_EXPORT_WITH_DES40_CBC_SHA,
                                    cipher_des40,  mac_sha, kea_rsa_export},
#endif
    {TLS_RSA_WITH_DES_CBC_SHA,      cipher_des,    mac_sha, kea_rsa},
    {TLS_RSA_WITH_3DES_EDE_CBC_SHA, cipher_3des,   mac_sha, kea_rsa},
    {TLS_DHE_DSS_WITH_DES_CBC_SHA,  cipher_des,    mac_sha, kea_dhe_dss},
    {TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA,
                                    cipher_3des,   mac_sha, kea_dhe_dss},
    {TLS_DHE_DSS_WITH_RC4_128_SHA,  cipher_rc4,    mac_sha, kea_dhe_dss},
#if 0 
    {TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA,
                                    cipher_des40,  mac_sha, kea_dh_dss_export},
    {TLS_DH_DSS_DES_CBC_SHA,        cipher_des,    mac_sha, kea_dh_dss},
    {TLS_DH_DSS_3DES_CBC_SHA,       cipher_3des,   mac_sha, kea_dh_dss},
    {TLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA,
                                    cipher_des40,  mac_sha, kea_dh_rsa_export},
    {TLS_DH_RSA_DES_CBC_SHA,        cipher_des,    mac_sha, kea_dh_rsa},
    {TLS_DH_RSA_3DES_CBC_SHA,       cipher_3des,   mac_sha, kea_dh_rsa},
    {TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA,
                                    cipher_des40,  mac_sha, kea_dh_dss_export},
    {TLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA,
                                    cipher_des40,  mac_sha, kea_dh_rsa_export},
#endif
    {TLS_DHE_RSA_WITH_DES_CBC_SHA,  cipher_des,    mac_sha, kea_dhe_rsa},
    {TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA,
                                    cipher_3des,   mac_sha, kea_dhe_rsa},
#if 0
    {SSL_DH_ANON_EXPORT_RC4_40_MD5, cipher_rc4_40, mac_md5, kea_dh_anon_export},
    {TLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA,
                                    cipher_des40,  mac_sha, kea_dh_anon_export},
    {TLS_DH_anon_WITH_DES_CBC_SHA,  cipher_des,    mac_sha, kea_dh_anon},
    {TLS_DH_anon_WITH_3DES_CBC_SHA, cipher_3des,   mac_sha, kea_dh_anon},
#endif



    {TLS_RSA_WITH_AES_128_CBC_SHA,     	cipher_aes_128, mac_sha, kea_rsa},
    {TLS_RSA_WITH_AES_128_CBC_SHA256,	cipher_aes_128, hmac_sha256, kea_rsa},
    {TLS_DHE_DSS_WITH_AES_128_CBC_SHA, 	cipher_aes_128, mac_sha, kea_dhe_dss},
    {TLS_DHE_RSA_WITH_AES_128_CBC_SHA, 	cipher_aes_128, mac_sha, kea_dhe_rsa},
    {TLS_DHE_RSA_WITH_AES_128_CBC_SHA256, cipher_aes_128, hmac_sha256, kea_dhe_rsa},
    {TLS_RSA_WITH_AES_256_CBC_SHA,     	cipher_aes_256, mac_sha, kea_rsa},
    {TLS_RSA_WITH_AES_256_CBC_SHA256,	cipher_aes_256, hmac_sha256, kea_rsa},
    {TLS_DHE_DSS_WITH_AES_256_CBC_SHA, 	cipher_aes_256, mac_sha, kea_dhe_dss},
    {TLS_DHE_RSA_WITH_AES_256_CBC_SHA, 	cipher_aes_256, mac_sha, kea_dhe_rsa},
    {TLS_DHE_RSA_WITH_AES_256_CBC_SHA256, cipher_aes_256, hmac_sha256, kea_dhe_rsa},
#if 0
    {TLS_DH_DSS_WITH_AES_128_CBC_SHA,  	cipher_aes_128, mac_sha, kea_dh_dss},
    {TLS_DH_RSA_WITH_AES_128_CBC_SHA,  	cipher_aes_128, mac_sha, kea_dh_rsa},
    {TLS_DH_anon_WITH_AES_128_CBC_SHA, 	cipher_aes_128, mac_sha, kea_dh_anon},
    {TLS_DH_DSS_WITH_AES_256_CBC_SHA,  	cipher_aes_256, mac_sha, kea_dh_dss},
    {TLS_DH_RSA_WITH_AES_256_CBC_SHA,  	cipher_aes_256, mac_sha, kea_dh_rsa},
    {TLS_DH_anon_WITH_AES_256_CBC_SHA, 	cipher_aes_256, mac_sha, kea_dh_anon},
#endif

    {TLS_RSA_WITH_SEED_CBC_SHA,	    cipher_seed,   mac_sha, kea_rsa},

    {TLS_RSA_WITH_CAMELLIA_128_CBC_SHA, cipher_camellia_128, mac_sha, kea_rsa},
    {TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA,
     cipher_camellia_128, mac_sha, kea_dhe_dss},
    {TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA,
     cipher_camellia_128, mac_sha, kea_dhe_rsa},
    {TLS_RSA_WITH_CAMELLIA_256_CBC_SHA,	cipher_camellia_256, mac_sha, kea_rsa},
    {TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA,
     cipher_camellia_256, mac_sha, kea_dhe_dss},
    {TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA,
     cipher_camellia_256, mac_sha, kea_dhe_rsa},

    {TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA,
                                    cipher_des,    mac_sha,kea_rsa_export_1024},
    {TLS_RSA_EXPORT1024_WITH_RC4_56_SHA,
                                    cipher_rc4_56, mac_sha,kea_rsa_export_1024},

    {SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA, cipher_3des, mac_sha, kea_rsa_fips},
    {SSL_RSA_FIPS_WITH_DES_CBC_SHA, cipher_des,    mac_sha, kea_rsa_fips},

    {TLS_DHE_RSA_WITH_AES_128_GCM_SHA256, cipher_aes_128_gcm, mac_aead, kea_dhe_rsa},
    {TLS_RSA_WITH_AES_128_GCM_SHA256, cipher_aes_128_gcm, mac_aead, kea_rsa},
    {TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256, cipher_aes_128_gcm, mac_aead, kea_ecdhe_rsa},
    {TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, cipher_aes_128_gcm, mac_aead, kea_ecdhe_ecdsa},

#ifndef NSS_DISABLE_ECC
    {TLS_ECDH_ECDSA_WITH_NULL_SHA,        cipher_null, mac_sha, kea_ecdh_ecdsa},
    {TLS_ECDH_ECDSA_WITH_RC4_128_SHA,      cipher_rc4, mac_sha, kea_ecdh_ecdsa},
    {TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA, cipher_3des, mac_sha, kea_ecdh_ecdsa},
    {TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA, cipher_aes_128, mac_sha, kea_ecdh_ecdsa},
    {TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA, cipher_aes_256, mac_sha, kea_ecdh_ecdsa},

    {TLS_ECDHE_ECDSA_WITH_NULL_SHA,        cipher_null, mac_sha, kea_ecdhe_ecdsa},
    {TLS_ECDHE_ECDSA_WITH_RC4_128_SHA,      cipher_rc4, mac_sha, kea_ecdhe_ecdsa},
    {TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA, cipher_3des, mac_sha, kea_ecdhe_ecdsa},
    {TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA, cipher_aes_128, mac_sha, kea_ecdhe_ecdsa},
    {TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256, cipher_aes_128, hmac_sha256, kea_ecdhe_ecdsa},
    {TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA, cipher_aes_256, mac_sha, kea_ecdhe_ecdsa},

    {TLS_ECDH_RSA_WITH_NULL_SHA,         cipher_null,    mac_sha, kea_ecdh_rsa},
    {TLS_ECDH_RSA_WITH_RC4_128_SHA,      cipher_rc4,     mac_sha, kea_ecdh_rsa},
    {TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA, cipher_3des,    mac_sha, kea_ecdh_rsa},
    {TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,  cipher_aes_128, mac_sha, kea_ecdh_rsa},
    {TLS_ECDH_RSA_WITH_AES_256_CBC_SHA,  cipher_aes_256, mac_sha, kea_ecdh_rsa},

    {TLS_ECDHE_RSA_WITH_NULL_SHA,         cipher_null,    mac_sha, kea_ecdhe_rsa},
    {TLS_ECDHE_RSA_WITH_RC4_128_SHA,      cipher_rc4,     mac_sha, kea_ecdhe_rsa},
    {TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA, cipher_3des,    mac_sha, kea_ecdhe_rsa},
    {TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,  cipher_aes_128, mac_sha, kea_ecdhe_rsa},
    {TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256, cipher_aes_128, hmac_sha256, kea_ecdhe_rsa},
    {TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,  cipher_aes_256, mac_sha, kea_ecdhe_rsa},

#if 0
    {TLS_ECDH_anon_WITH_NULL_SHA,         cipher_null,    mac_sha, kea_ecdh_anon},
    {TLS_ECDH_anon_WITH_RC4_128_SHA,      cipher_rc4,     mac_sha, kea_ecdh_anon},
    {TLS_ECDH_anon_WITH_3DES_EDE_CBC_SHA, cipher_3des,    mac_sha, kea_ecdh_anon},
    {TLS_ECDH_anon_WITH_AES_128_CBC_SHA,  cipher_aes_128, mac_sha, kea_ecdh_anon},
    {TLS_ECDH_anon_WITH_AES_256_CBC_SHA,  cipher_aes_256, mac_sha, kea_ecdh_anon},
#endif
#endif 
};

static const CK_MECHANISM_TYPE kea_alg_defs[] = {
    0x80000000L,
    CKM_RSA_PKCS,
    CKM_DH_PKCS_DERIVE,
    CKM_KEA_KEY_DERIVE,
    CKM_ECDH1_DERIVE
};

typedef struct SSLCipher2MechStr {
    SSLCipherAlgorithm  calg;
    CK_MECHANISM_TYPE   cmech;
} SSLCipher2Mech;


static const SSLCipher2Mech alg2Mech[] = {
    
    { calg_null     , (CK_MECHANISM_TYPE)0x80000000L	},
    { calg_rc4      , CKM_RC4				},
    { calg_rc2      , CKM_RC2_CBC			},
    { calg_des      , CKM_DES_CBC			},
    { calg_3des     , CKM_DES3_CBC			},
    { calg_idea     , CKM_IDEA_CBC			},
    { calg_fortezza , CKM_SKIPJACK_CBC64                },
    { calg_aes      , CKM_AES_CBC			},
    { calg_camellia , CKM_CAMELLIA_CBC			},
    { calg_seed     , CKM_SEED_CBC			},
    { calg_aes_gcm  , CKM_AES_GCM			},

};

#define mmech_invalid  (CK_MECHANISM_TYPE)0x80000000L
#define mmech_md5      CKM_SSL3_MD5_MAC
#define mmech_sha      CKM_SSL3_SHA1_MAC
#define mmech_md5_hmac CKM_MD5_HMAC
#define mmech_sha_hmac CKM_SHA_1_HMAC
#define mmech_sha256_hmac CKM_SHA256_HMAC

static const ssl3MACDef mac_defs[] = { 
    
    
    { mac_null, mmech_invalid,    0,  0          },
    { mac_md5,  mmech_md5,       48,  MD5_LENGTH },
    { mac_sha,  mmech_sha,       40,  SHA1_LENGTH},
    {hmac_md5,  mmech_md5_hmac,   0,  MD5_LENGTH },
    {hmac_sha,  mmech_sha_hmac,   0,  SHA1_LENGTH},
    {hmac_sha256, mmech_sha256_hmac, 0, SHA256_LENGTH},
    { mac_aead, mmech_invalid,    0,  0          },
};


const char * const ssl3_cipherName[] = {
    "NULL",
    "RC4",
    "RC4-40",
    "RC4-56",
    "RC2-CBC",
    "RC2-CBC-40",
    "DES-CBC",
    "3DES-EDE-CBC",
    "DES-CBC-40",
    "IDEA-CBC",
    "AES-128",
    "AES-256",
    "Camellia-128",
    "Camellia-256",
    "SEED-CBC",
    "AES-128-GCM",
    "missing"
};

#ifndef NSS_DISABLE_ECC











#define MAX_EC_WRAPPED_KEY_BUFLEN  504

typedef struct ECCWrappedKeyInfoStr {
    PRUint16 size;            
    PRUint16 encodedParamLen; 
    PRUint16 pubValueLen;     
    PRUint16 wrappedKeyLen;   
    PRUint8 var[MAX_EC_WRAPPED_KEY_BUFLEN]; 
    
} ECCWrappedKeyInfo;
#endif 

#if defined(TRACE)

static char *
ssl3_DecodeHandshakeType(int msgType)
{
    char * rv;
    static char line[40];

    switch(msgType) {
    case hello_request:	        rv = "hello_request (0)";               break;
    case client_hello:	        rv = "client_hello  (1)";               break;
    case server_hello:	        rv = "server_hello  (2)";               break;
    case hello_verify_request:  rv = "hello_verify_request (3)";        break;
    case certificate:	        rv = "certificate  (11)";               break;
    case server_key_exchange:	rv = "server_key_exchange (12)";        break;
    case certificate_request:	rv = "certificate_request (13)";        break;
    case server_hello_done:	rv = "server_hello_done   (14)";        break;
    case certificate_verify:	rv = "certificate_verify  (15)";        break;
    case client_key_exchange:	rv = "client_key_exchange (16)";        break;
    case finished:	        rv = "finished     (20)";               break;
    default:
        sprintf(line, "*UNKNOWN* handshake type! (%d)", msgType);
	rv = line;
    }
    return rv;
}

static char *
ssl3_DecodeContentType(int msgType)
{
    char * rv;
    static char line[40];

    switch(msgType) {
    case content_change_cipher_spec:
                                rv = "change_cipher_spec (20)";         break;
    case content_alert:	        rv = "alert      (21)";                 break;
    case content_handshake:	rv = "handshake  (22)";                 break;
    case content_application_data:
                                rv = "application_data (23)";           break;
    default:
        sprintf(line, "*UNKNOWN* record type! (%d)", msgType);
	rv = line;
    }
    return rv;
}

#endif

SSL3Statistics * 
SSL_GetStatistics(void)
{
    return &ssl3stats;
}

typedef struct tooLongStr {
#if defined(IS_LITTLE_ENDIAN)
    PRInt32 low;
    PRInt32 high;
#else
    PRInt32 high;
    PRInt32 low;
#endif
} tooLong;

void SSL_AtomicIncrementLong(long * x)
{
    if ((sizeof *x) == sizeof(PRInt32)) {
        PR_ATOMIC_INCREMENT((PRInt32 *)x);
    } else {
    	tooLong * tl = (tooLong *)x;
	if (PR_ATOMIC_INCREMENT(&tl->low) == 0)
	    PR_ATOMIC_INCREMENT(&tl->high);
    }
}

static PRBool
ssl3_CipherSuiteAllowedForVersionRange(
    ssl3CipherSuite cipherSuite,
    const SSLVersionRange *vrange)
{
    switch (cipherSuite) {
    



    case TLS_RSA_EXPORT_WITH_RC4_40_MD5:
    case TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5:
    







	return vrange->min <= SSL_LIBRARY_VERSION_TLS_1_0;

    case TLS_DHE_RSA_WITH_AES_256_CBC_SHA256:
    case TLS_RSA_WITH_AES_256_CBC_SHA256:
    case TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256:
    case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256:
    case TLS_DHE_RSA_WITH_AES_128_CBC_SHA256:
    case TLS_RSA_WITH_AES_128_CBC_SHA256:
    case TLS_RSA_WITH_AES_128_GCM_SHA256:
    case TLS_RSA_WITH_NULL_SHA256:
        return vrange->max == SSL_LIBRARY_VERSION_TLS_1_2;

    case TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256:
    case TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256:
    case TLS_DHE_RSA_WITH_AES_128_GCM_SHA256:
	return vrange->max >= SSL_LIBRARY_VERSION_TLS_1_2;

    

    case TLS_ECDH_ECDSA_WITH_NULL_SHA:
    case TLS_ECDH_ECDSA_WITH_RC4_128_SHA:
    case TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA:
    case TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA:
    case TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA:
    case TLS_ECDHE_ECDSA_WITH_NULL_SHA:
    case TLS_ECDHE_ECDSA_WITH_RC4_128_SHA:
    case TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA:
    case TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA:
    case TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA:
    case TLS_ECDH_RSA_WITH_NULL_SHA:
    case TLS_ECDH_RSA_WITH_RC4_128_SHA:
    case TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA:
    case TLS_ECDH_RSA_WITH_AES_128_CBC_SHA:
    case TLS_ECDH_RSA_WITH_AES_256_CBC_SHA:
    case TLS_ECDHE_RSA_WITH_NULL_SHA:
    case TLS_ECDHE_RSA_WITH_RC4_128_SHA:
    case TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA:
    case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA:
    case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA:
        return vrange->max >= SSL_LIBRARY_VERSION_TLS_1_0 &&
               vrange->min < SSL_LIBRARY_VERSION_TLS_1_3;

    default:
        return vrange->min < SSL_LIBRARY_VERSION_TLS_1_3;
    }
}



static const ssl3CipherSuiteDef *
ssl_LookupCipherSuiteDef(ssl3CipherSuite suite)
{
    int cipher_suite_def_len =
	sizeof(cipher_suite_defs) / sizeof(cipher_suite_defs[0]);
    int i;

    for (i = 0; i < cipher_suite_def_len; i++) {
	if (cipher_suite_defs[i].cipher_suite == suite)
	    return &cipher_suite_defs[i];
    }
    PORT_Assert(PR_FALSE);  
    PORT_SetError(SSL_ERROR_UNKNOWN_CIPHER_SUITE);
    return NULL;
}



static ssl3CipherSuiteCfg *
ssl_LookupCipherSuiteCfg(ssl3CipherSuite suite, ssl3CipherSuiteCfg *suites)
{
    int i;

    for (i = 0; i < ssl_V3_SUITES_IMPLEMENTED; i++) {
	if (suites[i].cipher_suite == suite)
	    return &suites[i];
    }
    
    PORT_SetError(SSL_ERROR_UNKNOWN_CIPHER_SUITE);
    return NULL;
}







int
ssl3_config_match_init(sslSocket *ss)
{
    ssl3CipherSuiteCfg *      suite;
    const ssl3CipherSuiteDef *cipher_def;
    SSLCipherAlgorithm        cipher_alg;
    CK_MECHANISM_TYPE         cipher_mech;
    SSL3KEAType               exchKeyType;
    int                       i;
    int                       numPresent		= 0;
    int                       numEnabled		= 0;
    PRBool                    isServer;
    sslServerCerts           *svrAuth;

    PORT_Assert(ss);
    if (!ss) {
    	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return 0;
    }
    if (SSL3_ALL_VERSIONS_DISABLED(&ss->vrange)) {
    	return 0;
    }
    isServer = (PRBool)(ss->sec.isServer != 0);

    for (i = 0; i < ssl_V3_SUITES_IMPLEMENTED; i++) {
	suite = &ss->cipherSuites[i];
	if (suite->enabled) {
	    ++numEnabled;
	    


	    cipher_def = ssl_LookupCipherSuiteDef(suite->cipher_suite);
	    if (!cipher_def) {
	    	suite->isPresent = PR_FALSE;
		continue;
	    }
	    cipher_alg = bulk_cipher_defs[cipher_def->bulk_cipher_alg].calg;
	    PORT_Assert(  alg2Mech[cipher_alg].calg == cipher_alg);
	    cipher_mech = alg2Mech[cipher_alg].cmech;
	    exchKeyType =
	    	    kea_defs[cipher_def->key_exchange_alg].exchKeyType;
#ifdef NSS_DISABLE_ECC
	    svrAuth = ss->serverCerts + exchKeyType;
#else
	    




	    switch (cipher_def->key_exchange_alg) {
	    case kea_ecdhe_rsa:
#if NSS_SERVER_DHE_IMPLEMENTED
	    





	    case kea_dhe_rsa:
#endif
		svrAuth = ss->serverCerts + kt_rsa;
		break;
	    case kea_ecdh_ecdsa:
	    case kea_ecdh_rsa:
	        






	    default:
		svrAuth = ss->serverCerts + exchKeyType;
		break;
	    }
#endif 

	    
	    suite->isPresent = (PRBool)
		(((exchKeyType == kt_null) ||
		   ((!isServer || (svrAuth->serverKeyPair &&
		                   svrAuth->SERVERKEY &&
				   svrAuth->serverCertChain)) &&
		    PK11_TokenExists(kea_alg_defs[exchKeyType]))) &&
		((cipher_alg == calg_null) || PK11_TokenExists(cipher_mech)));
	    if (suite->isPresent)
	    	++numPresent;
	}
    }
    PORT_Assert(numPresent > 0 || numEnabled == 0);
    if (numPresent <= 0) {
	PORT_SetError(SSL_ERROR_NO_CIPHERS_SUPPORTED);
    }
    return numPresent;
}










static PRBool
config_match(ssl3CipherSuiteCfg *suite, int policy, PRBool enabled,
	     const SSLVersionRange *vrange)
{
    PORT_Assert(policy != SSL_NOT_ALLOWED && enabled != PR_FALSE);
    if (policy == SSL_NOT_ALLOWED || !enabled)
    	return PR_FALSE;
    return (PRBool)(suite->enabled &&
                    suite->isPresent &&
	            suite->policy != SSL_NOT_ALLOWED &&
		    suite->policy <= policy &&
		    ssl3_CipherSuiteAllowedForVersionRange(
                        suite->cipher_suite, vrange));
}




static int
count_cipher_suites(sslSocket *ss, int policy, PRBool enabled)
{
    int i, count = 0;

    if (SSL3_ALL_VERSIONS_DISABLED(&ss->vrange)) {
	return 0;
    }
    for (i = 0; i < ssl_V3_SUITES_IMPLEMENTED; i++) {
	if (config_match(&ss->cipherSuites[i], policy, enabled, &ss->vrange))
	    count++;
    }
    if (count <= 0) {
	PORT_SetError(SSL_ERROR_SSL_DISABLED);
    }
    return count;
}





static SECStatus
Null_Cipher(void *ctx, unsigned char *output, int *outputLen, int maxOutputLen,
	    const unsigned char *input, int inputLen)
{
    if (inputLen > maxOutputLen) {
        *outputLen = 0;  
        PORT_SetError(SEC_ERROR_OUTPUT_LEN);
        return SECFailure;
    }
    *outputLen = inputLen;
    if (input != output)
	PORT_Memcpy(output, input, inputLen);
    return SECSuccess;
}












SECStatus
ssl3_NegotiateVersion(sslSocket *ss, SSL3ProtocolVersion peerVersion,
		      PRBool allowLargerPeerVersion)
{
    if (SSL3_ALL_VERSIONS_DISABLED(&ss->vrange)) {
	PORT_SetError(SSL_ERROR_SSL_DISABLED);
	return SECFailure;
    }

    if (peerVersion < ss->vrange.min ||
	(peerVersion > ss->vrange.max && !allowLargerPeerVersion)) {
	PORT_SetError(SSL_ERROR_UNSUPPORTED_VERSION);
	return SECFailure;
    }

    ss->version = PR_MIN(peerVersion, ss->vrange.max);
    PORT_Assert(ssl3_VersionIsSupported(ss->protocolVariant, ss->version));

    return SECSuccess;
}

static SECStatus
ssl3_GetNewRandom(SSL3Random *random)
{
    SECStatus rv;

    
    rv = PK11_GenerateRandom(random->rand, SSL3_RANDOM_LENGTH);
    if (rv != SECSuccess) {
	ssl_MapLowLevelError(SSL_ERROR_GENERATE_RANDOM_FAILURE);
    }
    return rv;
}


SECStatus
ssl3_SignHashes(SSL3Hashes *hash, SECKEYPrivateKey *key, SECItem *buf, 
                PRBool isTLS)
{
    SECStatus rv		= SECFailure;
    PRBool    doDerEncode       = PR_FALSE;
    int       signatureLen;
    SECItem   hashItem;

    buf->data    = NULL;

    switch (key->keyType) {
    case rsaKey:
	hashItem.data = hash->u.raw;
	hashItem.len = hash->len;
	break;
    case dsaKey:
	doDerEncode = isTLS;
	

	if (hash->hashAlg == SEC_OID_UNKNOWN) {
	    hashItem.data = hash->u.s.sha;
	    hashItem.len = sizeof(hash->u.s.sha);
	} else {
	    hashItem.data = hash->u.raw;
	    hashItem.len = hash->len;
	}
	break;
#ifndef NSS_DISABLE_ECC
    case ecKey:
	doDerEncode = PR_TRUE;
	

	if (hash->hashAlg == SEC_OID_UNKNOWN) {
	    hashItem.data = hash->u.s.sha;
	    hashItem.len = sizeof(hash->u.s.sha);
	} else {
	    hashItem.data = hash->u.raw;
	    hashItem.len = hash->len;
	}
	break;
#endif 
    default:
	PORT_SetError(SEC_ERROR_INVALID_KEY);
	goto done;
    }
    PRINT_BUF(60, (NULL, "hash(es) to be signed", hashItem.data, hashItem.len));

    if (hash->hashAlg == SEC_OID_UNKNOWN) {
	signatureLen = PK11_SignatureLen(key);
	if (signatureLen <= 0) {
	    PORT_SetError(SEC_ERROR_INVALID_KEY);
	    goto done;
	}

	buf->len  = (unsigned)signatureLen;
	buf->data = (unsigned char *)PORT_Alloc(signatureLen);
	if (!buf->data)
	    goto done;  

	rv = PK11_Sign(key, buf, &hashItem);
    } else {
	rv = SGN_Digest(key, hash->hashAlg, buf, &hashItem);
    }
    if (rv != SECSuccess) {
	ssl_MapLowLevelError(SSL_ERROR_SIGN_HASHES_FAILURE);
    } else if (doDerEncode) {
	SECItem   derSig	= {siBuffer, NULL, 0};

	
	rv = DSAU_EncodeDerSigWithLen(&derSig, buf, buf->len);
	if (rv == SECSuccess) {
	    PORT_Free(buf->data);	
	    *buf = derSig;		
	} else if (derSig.data) {
	    PORT_Free(derSig.data);
	}
    }

    PRINT_BUF(60, (NULL, "signed hashes", (unsigned char*)buf->data, buf->len));
done:
    if (rv != SECSuccess && buf->data) {
	PORT_Free(buf->data);
	buf->data = NULL;
    }
    return rv;
}


SECStatus
ssl3_VerifySignedHashes(SSL3Hashes *hash, CERTCertificate *cert, 
                        SECItem *buf, PRBool isTLS, void *pwArg)
{
    SECKEYPublicKey * key;
    SECItem *         signature	= NULL;
    SECStatus         rv;
    SECItem           hashItem;
    SECOidTag         encAlg;
    SECOidTag         hashAlg;


    PRINT_BUF(60, (NULL, "check signed hashes",
                  buf->data, buf->len));

    key = CERT_ExtractPublicKey(cert);
    if (key == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_EXTRACT_PUBLIC_KEY_FAILURE);
    	return SECFailure;
    }

    hashAlg = hash->hashAlg;
    switch (key->keyType) {
    case rsaKey:
	encAlg = SEC_OID_PKCS1_RSA_ENCRYPTION;
	hashItem.data = hash->u.raw;
	hashItem.len = hash->len;
	break;
    case dsaKey:
	encAlg = SEC_OID_ANSIX9_DSA_SIGNATURE;
	

	if (hash->hashAlg == SEC_OID_UNKNOWN) {
	    hashItem.data = hash->u.s.sha;
	    hashItem.len = sizeof(hash->u.s.sha);
	} else {
	    hashItem.data = hash->u.raw;
	    hashItem.len = hash->len;
	}
	
	if (isTLS || buf->len != SECKEY_SignatureLen(key)) {
	    signature = DSAU_DecodeDerSigToLen(buf, SECKEY_SignatureLen(key));
	    if (!signature) {
	    	PORT_SetError(SSL_ERROR_BAD_HANDSHAKE_HASH_VALUE);
		return SECFailure;
	    }
	    buf = signature;
	}
	break;

#ifndef NSS_DISABLE_ECC
    case ecKey:
	encAlg = SEC_OID_ANSIX962_EC_PUBLIC_KEY;
	





	if (hash->hashAlg == SEC_OID_UNKNOWN) {
	    hashAlg = SEC_OID_SHA1;
	    hashItem.data = hash->u.s.sha;
	    hashItem.len = sizeof(hash->u.s.sha);
	} else {
	    hashItem.data = hash->u.raw;
	    hashItem.len = hash->len;
	}
	break;
#endif 

    default:
    	SECKEY_DestroyPublicKey(key);
	PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
	return SECFailure;
    }

    PRINT_BUF(60, (NULL, "hash(es) to be verified",
                  hashItem.data, hashItem.len));

    if (hashAlg == SEC_OID_UNKNOWN || key->keyType == dsaKey) {
	




	rv = PK11_Verify(key, buf, &hashItem, pwArg);
    } else {
	rv = VFY_VerifyDigestDirect(&hashItem, key, buf, encAlg, hashAlg,
				    pwArg);
    }
    SECKEY_DestroyPublicKey(key);
    if (signature) {
    	SECITEM_FreeItem(signature, PR_TRUE);
    }
    if (rv != SECSuccess) {
	ssl_MapLowLevelError(SSL_ERROR_BAD_HANDSHAKE_HASH_VALUE);
    }
    return rv;
}










SECStatus
ssl3_ComputeCommonKeyHash(SECOidTag hashAlg,
			  PRUint8 * hashBuf, unsigned int bufLen,
			  SSL3Hashes *hashes, PRBool bypassPKCS11)
{
    SECStatus     rv 		= SECSuccess;

#ifndef NO_PKCS11_BYPASS
    if (bypassPKCS11) {
	if (hashAlg == SEC_OID_UNKNOWN) {
	    MD5_HashBuf (hashes->u.s.md5, hashBuf, bufLen);
	    SHA1_HashBuf(hashes->u.s.sha, hashBuf, bufLen);
	    hashes->len = MD5_LENGTH + SHA1_LENGTH;
	} else if (hashAlg == SEC_OID_SHA1) {
	    SHA1_HashBuf(hashes->u.raw, hashBuf, bufLen);
	    hashes->len = SHA1_LENGTH;
	} else if (hashAlg == SEC_OID_SHA256) {
	    SHA256_HashBuf(hashes->u.raw, hashBuf, bufLen);
	    hashes->len = SHA256_LENGTH;
	} else if (hashAlg == SEC_OID_SHA384) {
	    SHA384_HashBuf(hashes->u.raw, hashBuf, bufLen);
	    hashes->len = SHA384_LENGTH;
	} else if (hashAlg == SEC_OID_SHA512) {
	    SHA512_HashBuf(hashes->u.raw, hashBuf, bufLen);
	    hashes->len = SHA512_LENGTH;
	} else {
	    PORT_SetError(SSL_ERROR_UNSUPPORTED_HASH_ALGORITHM);
	    return SECFailure;
	}
    } else 
#endif
    {
	if (hashAlg == SEC_OID_UNKNOWN) {
	    rv = PK11_HashBuf(SEC_OID_MD5, hashes->u.s.md5, hashBuf, bufLen);
	    if (rv != SECSuccess) {
		ssl_MapLowLevelError(SSL_ERROR_MD5_DIGEST_FAILURE);
		rv = SECFailure;
		goto done;
	    }

	    rv = PK11_HashBuf(SEC_OID_SHA1, hashes->u.s.sha, hashBuf, bufLen);
	    if (rv != SECSuccess) {
		ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
		rv = SECFailure;
	    }
	    hashes->len = MD5_LENGTH + SHA1_LENGTH;
	} else {
	    hashes->len = HASH_ResultLenByOidTag(hashAlg);
	    if (hashes->len > sizeof(hashes->u.raw)) {
		ssl_MapLowLevelError(SSL_ERROR_UNSUPPORTED_HASH_ALGORITHM);
		rv = SECFailure;
		goto done;
	    }
	    rv = PK11_HashBuf(hashAlg, hashes->u.raw, hashBuf, bufLen);
	    if (rv != SECSuccess) {
		ssl_MapLowLevelError(SSL_ERROR_DIGEST_FAILURE);
		rv = SECFailure;
	    }
	}
    }
    hashes->hashAlg = hashAlg;

done:
    return rv;
}





static SECStatus
ssl3_ComputeExportRSAKeyHash(SECOidTag hashAlg,
			     SECItem modulus, SECItem publicExponent,
			     SSL3Random *client_rand, SSL3Random *server_rand,
			     SSL3Hashes *hashes, PRBool bypassPKCS11)
{
    PRUint8     * hashBuf;
    PRUint8     * pBuf;
    SECStatus     rv 		= SECSuccess;
    unsigned int  bufLen;
    PRUint8       buf[2*SSL3_RANDOM_LENGTH + 2 + 4096/8 + 2 + 4096/8];

    bufLen = 2*SSL3_RANDOM_LENGTH + 2 + modulus.len + 2 + publicExponent.len;
    if (bufLen <= sizeof buf) {
    	hashBuf = buf;
    } else {
    	hashBuf = PORT_Alloc(bufLen);
	if (!hashBuf) {
	    return SECFailure;
	}
    }

    memcpy(hashBuf, client_rand, SSL3_RANDOM_LENGTH); 
    	pBuf = hashBuf + SSL3_RANDOM_LENGTH;
    memcpy(pBuf, server_rand, SSL3_RANDOM_LENGTH);
    	pBuf += SSL3_RANDOM_LENGTH;
    pBuf[0]  = (PRUint8)(modulus.len >> 8);
    pBuf[1]  = (PRUint8)(modulus.len);
    	pBuf += 2;
    memcpy(pBuf, modulus.data, modulus.len);
    	pBuf += modulus.len;
    pBuf[0] = (PRUint8)(publicExponent.len >> 8);
    pBuf[1] = (PRUint8)(publicExponent.len);
    	pBuf += 2;
    memcpy(pBuf, publicExponent.data, publicExponent.len);
    	pBuf += publicExponent.len;
    PORT_Assert((unsigned int)(pBuf - hashBuf) == bufLen);

    rv = ssl3_ComputeCommonKeyHash(hashAlg, hashBuf, bufLen, hashes,
				   bypassPKCS11);

    PRINT_BUF(95, (NULL, "RSAkey hash: ", hashBuf, bufLen));
    if (hashAlg == SEC_OID_UNKNOWN) {
	PRINT_BUF(95, (NULL, "RSAkey hash: MD5 result",
		  hashes->u.s.md5, MD5_LENGTH));
	PRINT_BUF(95, (NULL, "RSAkey hash: SHA1 result",
		  hashes->u.s.sha, SHA1_LENGTH));
    } else {
	PRINT_BUF(95, (NULL, "RSAkey hash: result",
		  hashes->u.raw, hashes->len));
    }

    if (hashBuf != buf && hashBuf != NULL)
    	PORT_Free(hashBuf);
    return rv;
}



static SECStatus
ssl3_ComputeDHKeyHash(SECOidTag hashAlg,
		      SECItem dh_p, SECItem dh_g, SECItem dh_Ys,
		      SSL3Random *client_rand, SSL3Random *server_rand,
		      SSL3Hashes *hashes, PRBool bypassPKCS11)
{
    PRUint8     * hashBuf;
    PRUint8     * pBuf;
    SECStatus     rv 		= SECSuccess;
    unsigned int  bufLen;
    PRUint8       buf[2*SSL3_RANDOM_LENGTH + 2 + 4096/8 + 2 + 4096/8];

    bufLen = 2*SSL3_RANDOM_LENGTH + 2 + dh_p.len + 2 + dh_g.len + 2 + dh_Ys.len;
    if (bufLen <= sizeof buf) {
    	hashBuf = buf;
    } else {
    	hashBuf = PORT_Alloc(bufLen);
	if (!hashBuf) {
	    return SECFailure;
	}
    }

    memcpy(hashBuf, client_rand, SSL3_RANDOM_LENGTH); 
    	pBuf = hashBuf + SSL3_RANDOM_LENGTH;
    memcpy(pBuf, server_rand, SSL3_RANDOM_LENGTH);
    	pBuf += SSL3_RANDOM_LENGTH;
    pBuf[0]  = (PRUint8)(dh_p.len >> 8);
    pBuf[1]  = (PRUint8)(dh_p.len);
    	pBuf += 2;
    memcpy(pBuf, dh_p.data, dh_p.len);
    	pBuf += dh_p.len;
    pBuf[0] = (PRUint8)(dh_g.len >> 8);
    pBuf[1] = (PRUint8)(dh_g.len);
    	pBuf += 2;
    memcpy(pBuf, dh_g.data, dh_g.len);
    	pBuf += dh_g.len;
    pBuf[0] = (PRUint8)(dh_Ys.len >> 8);
    pBuf[1] = (PRUint8)(dh_Ys.len);
    	pBuf += 2;
    memcpy(pBuf, dh_Ys.data, dh_Ys.len);
    	pBuf += dh_Ys.len;
    PORT_Assert((unsigned int)(pBuf - hashBuf) == bufLen);

    rv = ssl3_ComputeCommonKeyHash(hashAlg, hashBuf, bufLen, hashes,
				   bypassPKCS11);

    PRINT_BUF(95, (NULL, "DHkey hash: ", hashBuf, bufLen));
    if (hashAlg == SEC_OID_UNKNOWN) {
	PRINT_BUF(95, (NULL, "DHkey hash: MD5 result",
		  hashes->u.s.md5, MD5_LENGTH));
	PRINT_BUF(95, (NULL, "DHkey hash: SHA1 result",
		  hashes->u.s.sha, SHA1_LENGTH));
    } else {
	PRINT_BUF(95, (NULL, "DHkey hash: result",
		  hashes->u.raw, hashes->len));
    }

    if (hashBuf != buf && hashBuf != NULL)
    	PORT_Free(hashBuf);
    return rv;
}

static void
ssl3_BumpSequenceNumber(SSL3SequenceNumber *num)
{
    num->low++;
    if (num->low == 0)
	num->high++;
}


static void
ssl3_CleanupKeyMaterial(ssl3KeyMaterial *mat)
{
    if (mat->write_key != NULL) {
	PK11_FreeSymKey(mat->write_key);
	mat->write_key = NULL;
    }
    if (mat->write_mac_key != NULL) {
	PK11_FreeSymKey(mat->write_mac_key);
	mat->write_mac_key = NULL;
    }
    if (mat->write_mac_context != NULL) {
	PK11_DestroyContext(mat->write_mac_context, PR_TRUE);
	mat->write_mac_context = NULL;
    }
}






void
ssl3_DestroyCipherSpec(ssl3CipherSpec *spec, PRBool freeSrvName)
{
    PRBool freeit = (PRBool)(!spec->bypassCiphers);

    if (spec->destroy) {
	spec->destroy(spec->encodeContext, freeit);
	spec->destroy(spec->decodeContext, freeit);
	spec->encodeContext = NULL; 
	spec->decodeContext = NULL;
    }
    if (spec->destroyCompressContext && spec->compressContext) {
	spec->destroyCompressContext(spec->compressContext, 1);
	spec->compressContext = NULL;
    }
    if (spec->destroyDecompressContext && spec->decompressContext) {
	spec->destroyDecompressContext(spec->decompressContext, 1);
	spec->decompressContext = NULL;
    }
    if (freeSrvName && spec->srvVirtName.data) {
        SECITEM_FreeItem(&spec->srvVirtName, PR_FALSE);
    }
    if (spec->master_secret != NULL) {
	PK11_FreeSymKey(spec->master_secret);
	spec->master_secret = NULL;
    }
    spec->msItem.data = NULL;
    spec->msItem.len  = 0;
    ssl3_CleanupKeyMaterial(&spec->client);
    ssl3_CleanupKeyMaterial(&spec->server);
    spec->bypassCiphers = PR_FALSE;
    spec->destroy=NULL;
    spec->destroyCompressContext = NULL;
    spec->destroyDecompressContext = NULL;
}







static SECStatus
ssl3_SetupPendingCipherSpec(sslSocket *ss)
{
    ssl3CipherSpec *          pwSpec;
    ssl3CipherSpec *          cwSpec;
    ssl3CipherSuite           suite     = ss->ssl3.hs.cipher_suite;
    SSL3MACAlgorithm          mac;
    SSL3BulkCipher            cipher;
    SSL3KeyExchangeAlgorithm  kea;
    const ssl3CipherSuiteDef *suite_def;
    PRBool                    isTLS;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    ssl_GetSpecWriteLock(ss);  

    pwSpec = ss->ssl3.pwSpec;
    PORT_Assert(pwSpec == ss->ssl3.prSpec);

    
    cwSpec = ss->ssl3.cwSpec;
    if (cwSpec->mac_def->mac == mac_null) {
	
	cwSpec->version = ss->version;
    }

    pwSpec->version  = ss->version;
    isTLS  = (PRBool)(pwSpec->version > SSL_LIBRARY_VERSION_3_0);

    SSL_TRC(3, ("%d: SSL3[%d]: Set XXX Pending Cipher Suite to 0x%04x",
		SSL_GETPID(), ss->fd, suite));

    suite_def = ssl_LookupCipherSuiteDef(suite);
    if (suite_def == NULL) {
	ssl_ReleaseSpecWriteLock(ss);
	return SECFailure;	
    }

    if (IS_DTLS(ss)) {
	
	PORT_Assert((suite_def->bulk_cipher_alg != cipher_rc4) &&
		    (suite_def->bulk_cipher_alg != cipher_rc4_40) &&
		    (suite_def->bulk_cipher_alg != cipher_rc4_56));
    }

    cipher = suite_def->bulk_cipher_alg;
    kea    = suite_def->key_exchange_alg;
    mac    = suite_def->mac_alg;
    if (mac <= ssl_mac_sha && mac != ssl_mac_null && isTLS)
	mac += 2;

    ss->ssl3.hs.suite_def = suite_def;
    ss->ssl3.hs.kea_def   = &kea_defs[kea];
    PORT_Assert(ss->ssl3.hs.kea_def->kea == kea);

    pwSpec->cipher_def   = &bulk_cipher_defs[cipher];
    PORT_Assert(pwSpec->cipher_def->cipher == cipher);

    pwSpec->mac_def = &mac_defs[mac];
    PORT_Assert(pwSpec->mac_def->mac == mac);

    ss->sec.keyBits       = pwSpec->cipher_def->key_size        * BPB;
    ss->sec.secretKeyBits = pwSpec->cipher_def->secret_key_size * BPB;
    ss->sec.cipherType    = cipher;

    pwSpec->encodeContext = NULL;
    pwSpec->decodeContext = NULL;

    pwSpec->mac_size = pwSpec->mac_def->mac_size;

    pwSpec->compression_method = ss->ssl3.hs.compression;
    pwSpec->compressContext = NULL;
    pwSpec->decompressContext = NULL;

    ssl_ReleaseSpecWriteLock(ss);  
    return SECSuccess;
}

#ifdef NSS_ENABLE_ZLIB
#define SSL3_DEFLATE_CONTEXT_SIZE sizeof(z_stream)

static SECStatus
ssl3_MapZlibError(int zlib_error)
{
    switch (zlib_error) {
    case Z_OK:
        return SECSuccess;
    default:
        return SECFailure;
    }
}

static SECStatus
ssl3_DeflateInit(void *void_context)
{
    z_stream *context = void_context;
    context->zalloc = NULL;
    context->zfree = NULL;
    context->opaque = NULL;

    return ssl3_MapZlibError(deflateInit(context, Z_DEFAULT_COMPRESSION));
}

static SECStatus
ssl3_InflateInit(void *void_context)
{
    z_stream *context = void_context;
    context->zalloc = NULL;
    context->zfree = NULL;
    context->opaque = NULL;
    context->next_in = NULL;
    context->avail_in = 0;

    return ssl3_MapZlibError(inflateInit(context));
}

static SECStatus
ssl3_DeflateCompress(void *void_context, unsigned char *out, int *out_len,
                     int maxout, const unsigned char *in, int inlen)
{
    z_stream *context = void_context;

    if (!inlen) {
        *out_len = 0;
        return SECSuccess;
    }

    context->next_in = (unsigned char*) in;
    context->avail_in = inlen;
    context->next_out = out;
    context->avail_out = maxout;
    if (deflate(context, Z_SYNC_FLUSH) != Z_OK) {
        return SECFailure;
    }
    if (context->avail_out == 0) {
        
        SSL_TRC(3, ("%d: SSL3[%d] Ran out of buffer while compressing",
                    SSL_GETPID()));
        return SECFailure;
    }

    *out_len = maxout - context->avail_out;
    return SECSuccess;
}

static SECStatus
ssl3_DeflateDecompress(void *void_context, unsigned char *out, int *out_len,
                       int maxout, const unsigned char *in, int inlen)
{
    z_stream *context = void_context;

    if (!inlen) {
        *out_len = 0;
        return SECSuccess;
    }

    context->next_in = (unsigned char*) in;
    context->avail_in = inlen;
    context->next_out = out;
    context->avail_out = maxout;
    if (inflate(context, Z_SYNC_FLUSH) != Z_OK) {
        PORT_SetError(SSL_ERROR_DECOMPRESSION_FAILURE);
        return SECFailure;
    }

    *out_len = maxout - context->avail_out;
    return SECSuccess;
}

static SECStatus
ssl3_DestroyCompressContext(void *void_context, PRBool unused)
{
    deflateEnd(void_context);
    PORT_Free(void_context);
    return SECSuccess;
}

static SECStatus
ssl3_DestroyDecompressContext(void *void_context, PRBool unused)
{
    inflateEnd(void_context);
    PORT_Free(void_context);
    return SECSuccess;
}

#endif 



static SECStatus
ssl3_InitCompressionContext(ssl3CipherSpec *pwSpec)
{
    
    switch (pwSpec->compression_method) {
    case ssl_compression_null:
	pwSpec->compressor = NULL;
	pwSpec->decompressor = NULL;
	pwSpec->compressContext = NULL;
	pwSpec->decompressContext = NULL;
	pwSpec->destroyCompressContext = NULL;
	pwSpec->destroyDecompressContext = NULL;
	break;
#ifdef NSS_ENABLE_ZLIB
    case ssl_compression_deflate:
	pwSpec->compressor = ssl3_DeflateCompress;
	pwSpec->decompressor = ssl3_DeflateDecompress;
	pwSpec->compressContext = PORT_Alloc(SSL3_DEFLATE_CONTEXT_SIZE);
	pwSpec->decompressContext = PORT_Alloc(SSL3_DEFLATE_CONTEXT_SIZE);
	pwSpec->destroyCompressContext = ssl3_DestroyCompressContext;
	pwSpec->destroyDecompressContext = ssl3_DestroyDecompressContext;
	ssl3_DeflateInit(pwSpec->compressContext);
	ssl3_InflateInit(pwSpec->decompressContext);
	break;
#endif 
    default:
	PORT_Assert(0);
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }

    return SECSuccess;
}

#ifndef NO_PKCS11_BYPASS





static SECStatus
ssl3_InitPendingContextsBypass(sslSocket *ss)
{
      ssl3CipherSpec  *  pwSpec;
      const ssl3BulkCipherDef *cipher_def;
      void *             serverContext = NULL;
      void *             clientContext = NULL;
      BLapiInitContextFunc initFn = (BLapiInitContextFunc)NULL;
      int                mode     = 0;
      unsigned int       optArg1  = 0;
      unsigned int       optArg2  = 0;
      PRBool             server_encrypts = ss->sec.isServer;
      SSLCipherAlgorithm calg;
      SECStatus          rv;

    PORT_Assert(ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));
    PORT_Assert(ss->opt.noLocks || ssl_HaveSpecWriteLock(ss));
    PORT_Assert(ss->ssl3.prSpec == ss->ssl3.pwSpec);

    pwSpec        = ss->ssl3.pwSpec;
    cipher_def    = pwSpec->cipher_def;

    calg = cipher_def->calg;

    if (calg == ssl_calg_aes_gcm) {
	pwSpec->encode = NULL;
	pwSpec->decode = NULL;
	pwSpec->destroy = NULL;
	pwSpec->encodeContext = NULL;
	pwSpec->decodeContext = NULL;
	pwSpec->aead = ssl3_AESGCMBypass;
	ssl3_InitCompressionContext(pwSpec);
	return SECSuccess;
    }

    serverContext = pwSpec->server.cipher_context;
    clientContext = pwSpec->client.cipher_context;

    switch (calg) {
    case ssl_calg_null:
	pwSpec->encode  = Null_Cipher;
	pwSpec->decode  = Null_Cipher;
        pwSpec->destroy = NULL;
	goto success;

    case ssl_calg_rc4:
      	initFn = (BLapiInitContextFunc)RC4_InitContext;
	pwSpec->encode  = (SSLCipher) RC4_Encrypt;
	pwSpec->decode  = (SSLCipher) RC4_Decrypt;
	pwSpec->destroy = (SSLDestroy) RC4_DestroyContext;
	break;
    case ssl_calg_rc2:
      	initFn = (BLapiInitContextFunc)RC2_InitContext;
	mode = NSS_RC2_CBC;
	optArg1 = cipher_def->key_size;
	pwSpec->encode  = (SSLCipher) RC2_Encrypt;
	pwSpec->decode  = (SSLCipher) RC2_Decrypt;
	pwSpec->destroy = (SSLDestroy) RC2_DestroyContext;
	break;
    case ssl_calg_des:
      	initFn = (BLapiInitContextFunc)DES_InitContext;
	mode = NSS_DES_CBC;
	optArg1 = server_encrypts;
	pwSpec->encode  = (SSLCipher) DES_Encrypt;
	pwSpec->decode  = (SSLCipher) DES_Decrypt;
	pwSpec->destroy = (SSLDestroy) DES_DestroyContext;
	break;
    case ssl_calg_3des:
      	initFn = (BLapiInitContextFunc)DES_InitContext;
	mode = NSS_DES_EDE3_CBC;
	optArg1 = server_encrypts;
	pwSpec->encode  = (SSLCipher) DES_Encrypt;
	pwSpec->decode  = (SSLCipher) DES_Decrypt;
	pwSpec->destroy = (SSLDestroy) DES_DestroyContext;
	break;
    case ssl_calg_aes:
      	initFn = (BLapiInitContextFunc)AES_InitContext;
	mode = NSS_AES_CBC;
	optArg1 = server_encrypts;
	optArg2 = AES_BLOCK_SIZE;
	pwSpec->encode  = (SSLCipher) AES_Encrypt;
	pwSpec->decode  = (SSLCipher) AES_Decrypt;
	pwSpec->destroy = (SSLDestroy) AES_DestroyContext;
	break;

    case ssl_calg_camellia:
      	initFn = (BLapiInitContextFunc)Camellia_InitContext;
	mode = NSS_CAMELLIA_CBC;
	optArg1 = server_encrypts;
	optArg2 = CAMELLIA_BLOCK_SIZE;
	pwSpec->encode  = (SSLCipher) Camellia_Encrypt;
	pwSpec->decode  = (SSLCipher) Camellia_Decrypt;
	pwSpec->destroy = (SSLDestroy) Camellia_DestroyContext;
	break;

    case ssl_calg_seed:
      	initFn = (BLapiInitContextFunc)SEED_InitContext;
	mode = NSS_SEED_CBC;
	optArg1 = server_encrypts;
	optArg2 = SEED_BLOCK_SIZE;
	pwSpec->encode  = (SSLCipher) SEED_Encrypt;
	pwSpec->decode  = (SSLCipher) SEED_Decrypt;
	pwSpec->destroy = (SSLDestroy) SEED_DestroyContext;
	break;

    case ssl_calg_idea:
    case ssl_calg_fortezza :
    default:
	PORT_Assert(0);
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	goto bail_out;
    }
    rv = (*initFn)(serverContext,
		   pwSpec->server.write_key_item.data,
		   pwSpec->server.write_key_item.len,
		   pwSpec->server.write_iv_item.data,
		   mode, optArg1, optArg2);
    if (rv != SECSuccess) {
	PORT_Assert(0);
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	goto bail_out;
    }

    switch (calg) {
    case ssl_calg_des:
    case ssl_calg_3des:
    case ssl_calg_aes:
    case ssl_calg_camellia:
    case ssl_calg_seed:
	


        optArg1 = !optArg1;
        break;
    
    case ssl_calg_null:
    case ssl_calg_rc4:
    case ssl_calg_rc2:
    case ssl_calg_idea:
    case ssl_calg_fortezza:
    case ssl_calg_aes_gcm:
        break;
    }

    rv = (*initFn)(clientContext,
		   pwSpec->client.write_key_item.data,
		   pwSpec->client.write_key_item.len,
		   pwSpec->client.write_iv_item.data,
		   mode, optArg1, optArg2);
    if (rv != SECSuccess) {
	PORT_Assert(0);
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	goto bail_out;
    }

    pwSpec->encodeContext = (ss->sec.isServer) ? serverContext : clientContext;
    pwSpec->decodeContext = (ss->sec.isServer) ? clientContext : serverContext;

    ssl3_InitCompressionContext(pwSpec);

success:
    return SECSuccess;

bail_out:
    return SECFailure;
}
#endif




static SECItem *
ssl3_ParamFromIV(CK_MECHANISM_TYPE mtype, SECItem *iv, CK_ULONG ulEffectiveBits)
{
    SECItem * param = PK11_ParamFromIV(mtype, iv);
    if (param && param->data  && param->len >= sizeof(CK_RC2_PARAMS)) {
	switch (mtype) {
	case CKM_RC2_KEY_GEN:
	case CKM_RC2_ECB:
	case CKM_RC2_CBC:
	case CKM_RC2_MAC:
	case CKM_RC2_MAC_GENERAL:
	case CKM_RC2_CBC_PAD:
	    *(CK_RC2_PARAMS *)param->data = ulEffectiveBits;
	default: break;
	}
    }
    return param;
}













static unsigned int
ssl3_BuildRecordPseudoHeader(unsigned char *out,
			     SSL3SequenceNumber seq_num,
			     SSL3ContentType type,
			     PRBool includesVersion,
			     SSL3ProtocolVersion version,
			     PRBool isDTLS,
			     int length)
{
    out[0] = (unsigned char)(seq_num.high >> 24);
    out[1] = (unsigned char)(seq_num.high >> 16);
    out[2] = (unsigned char)(seq_num.high >>  8);
    out[3] = (unsigned char)(seq_num.high >>  0);
    out[4] = (unsigned char)(seq_num.low  >> 24);
    out[5] = (unsigned char)(seq_num.low  >> 16);
    out[6] = (unsigned char)(seq_num.low  >>  8);
    out[7] = (unsigned char)(seq_num.low  >>  0);
    out[8] = type;

    
    if (!includesVersion) {
	out[9]  = MSB(length);
	out[10] = LSB(length);
	return 11;
    }

    
    if (isDTLS) {
	SSL3ProtocolVersion dtls_version;

	dtls_version = dtls_TLSVersionToDTLSVersion(version);
	out[9]  = MSB(dtls_version);
	out[10] = LSB(dtls_version);
    } else {
	out[9]  = MSB(version);
	out[10] = LSB(version);
    }
    out[11] = MSB(length);
    out[12] = LSB(length);
    return 13;
}

static SECStatus
ssl3_AESGCM(ssl3KeyMaterial *keys,
	    PRBool doDecrypt,
	    unsigned char *out,
	    int *outlen,
	    int maxout,
	    const unsigned char *in,
	    int inlen,
	    const unsigned char *additionalData,
	    int additionalDataLen)
{
    SECItem            param;
    SECStatus          rv = SECFailure;
    unsigned char      nonce[12];
    unsigned int       uOutLen;
    CK_GCM_PARAMS      gcmParams;

    static const int   tagSize = 16;
    static const int   explicitNonceLen = 8;

    

    memcpy(nonce, keys->write_iv, 4);
    if (doDecrypt) {
	memcpy(nonce + 4, in, explicitNonceLen);
	in += explicitNonceLen;
	inlen -= explicitNonceLen;
	*outlen = 0;
    } else {
	if (maxout < explicitNonceLen) {
	    PORT_SetError(SEC_ERROR_INPUT_LEN);
	    return SECFailure;
        }
	
	memcpy(nonce + 4, additionalData, explicitNonceLen);
	memcpy(out, additionalData, explicitNonceLen);
	out += explicitNonceLen;
	maxout -= explicitNonceLen;
	*outlen = explicitNonceLen;
    }

    param.type = siBuffer;
    param.data = (unsigned char *) &gcmParams;
    param.len = sizeof(gcmParams);
    gcmParams.pIv = nonce;
    gcmParams.ulIvLen = sizeof(nonce);
    gcmParams.pAAD = (unsigned char *)additionalData;  
    gcmParams.ulAADLen = additionalDataLen;
    gcmParams.ulTagBits = tagSize * 8;

    if (doDecrypt) {
	rv = PK11_Decrypt(keys->write_key, CKM_AES_GCM, &param, out, &uOutLen,
			  maxout, in, inlen);
    } else {
	rv = PK11_Encrypt(keys->write_key, CKM_AES_GCM, &param, out, &uOutLen,
			  maxout, in, inlen);
    }
    *outlen += (int) uOutLen;

    return rv;
}

#ifndef NO_PKCS11_BYPASS
static SECStatus
ssl3_AESGCMBypass(ssl3KeyMaterial *keys,
		  PRBool doDecrypt,
		  unsigned char *out,
		  int *outlen,
		  int maxout,
		  const unsigned char *in,
		  int inlen,
		  const unsigned char *additionalData,
		  int additionalDataLen)
{
    SECStatus          rv = SECFailure;
    unsigned char      nonce[12];
    unsigned int       uOutLen;
    AESContext        *cx;
    CK_GCM_PARAMS      gcmParams;

    static const int   tagSize = 16;
    static const int   explicitNonceLen = 8;

    

    PORT_Assert(keys->write_iv_item.len == 4);
    if (keys->write_iv_item.len != 4) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    memcpy(nonce, keys->write_iv_item.data, 4);
    if (doDecrypt) {
	memcpy(nonce + 4, in, explicitNonceLen);
	in += explicitNonceLen;
	inlen -= explicitNonceLen;
	*outlen = 0;
    } else {
	if (maxout < explicitNonceLen) {
	    PORT_SetError(SEC_ERROR_INPUT_LEN);
	    return SECFailure;
        }
	
	memcpy(nonce + 4, additionalData, explicitNonceLen);
	memcpy(out, additionalData, explicitNonceLen);
	out += explicitNonceLen;
	maxout -= explicitNonceLen;
	*outlen = explicitNonceLen;
    }

    gcmParams.pIv = nonce;
    gcmParams.ulIvLen = sizeof(nonce);
    gcmParams.pAAD = (unsigned char *)additionalData;  
    gcmParams.ulAADLen = additionalDataLen;
    gcmParams.ulTagBits = tagSize * 8;

    cx = (AESContext *)keys->cipher_context;
    rv = AES_InitContext(cx, keys->write_key_item.data,
			 keys->write_key_item.len,
			 (unsigned char *)&gcmParams, NSS_AES_GCM, !doDecrypt,
			 AES_BLOCK_SIZE);
    if (rv != SECSuccess) {
	return rv;
    }
    if (doDecrypt) {
	rv = AES_Decrypt(cx, out, &uOutLen, maxout, in, inlen);
    } else {
	rv = AES_Encrypt(cx, out, &uOutLen, maxout, in, inlen);
    }
    AES_DestroyContext(cx, PR_FALSE);
    *outlen += (int) uOutLen;

    return rv;
}
#endif





static SECStatus
ssl3_InitPendingContextsPKCS11(sslSocket *ss)
{
      ssl3CipherSpec  *  pwSpec;
      const ssl3BulkCipherDef *cipher_def;
      PK11Context *      serverContext = NULL;
      PK11Context *      clientContext = NULL;
      SECItem *          param;
      CK_MECHANISM_TYPE  mechanism;
      CK_MECHANISM_TYPE  mac_mech;
      CK_ULONG           macLength;
      CK_ULONG           effKeyBits;
      SECItem            iv;
      SECItem            mac_param;
      SSLCipherAlgorithm calg;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSpecWriteLock(ss));
    PORT_Assert(ss->ssl3.prSpec == ss->ssl3.pwSpec);

    pwSpec        = ss->ssl3.pwSpec;
    cipher_def    = pwSpec->cipher_def;
    macLength     = pwSpec->mac_size;
    calg          = cipher_def->calg;
    PORT_Assert(alg2Mech[calg].calg == calg);

    pwSpec->client.write_mac_context = NULL;
    pwSpec->server.write_mac_context = NULL;

    if (calg == calg_aes_gcm) {
	pwSpec->encode = NULL;
	pwSpec->decode = NULL;
	pwSpec->destroy = NULL;
	pwSpec->encodeContext = NULL;
	pwSpec->decodeContext = NULL;
	pwSpec->aead = ssl3_AESGCM;
	return SECSuccess;
    }

    




    mac_mech       = pwSpec->mac_def->mmech;
    mac_param.data = (unsigned char *)&macLength;
    mac_param.len  = sizeof(macLength);
    mac_param.type = 0;

    pwSpec->client.write_mac_context = PK11_CreateContextBySymKey(
	    mac_mech, CKA_SIGN, pwSpec->client.write_mac_key, &mac_param);
    if (pwSpec->client.write_mac_context == NULL)  {
	ssl_MapLowLevelError(SSL_ERROR_SYM_KEY_CONTEXT_FAILURE);
	goto fail;
    }
    pwSpec->server.write_mac_context = PK11_CreateContextBySymKey(
	    mac_mech, CKA_SIGN, pwSpec->server.write_mac_key, &mac_param);
    if (pwSpec->server.write_mac_context == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_SYM_KEY_CONTEXT_FAILURE);
	goto fail;
    }

    



    if (calg == calg_null) {
	pwSpec->encode  = Null_Cipher;
	pwSpec->decode  = Null_Cipher;
	pwSpec->destroy = NULL;
	return SECSuccess;
    }
    mechanism = alg2Mech[calg].cmech;
    effKeyBits = cipher_def->key_size * BPB;

    


    iv.data = pwSpec->server.write_iv;
    iv.len  = cipher_def->iv_size;
    param = ssl3_ParamFromIV(mechanism, &iv, effKeyBits);
    if (param == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_IV_PARAM_FAILURE);
    	goto fail;
    }
    serverContext = PK11_CreateContextBySymKey(mechanism,
				(ss->sec.isServer ? CKA_ENCRYPT : CKA_DECRYPT),
				pwSpec->server.write_key, param);
    iv.data = PK11_IVFromParam(mechanism, param, (int *)&iv.len);
    if (iv.data)
    	PORT_Memcpy(pwSpec->server.write_iv, iv.data, iv.len);
    SECITEM_FreeItem(param, PR_TRUE);
    if (serverContext == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_SYM_KEY_CONTEXT_FAILURE);
    	goto fail;
    }

    


    iv.data = pwSpec->client.write_iv;
    iv.len  = cipher_def->iv_size;

    param = ssl3_ParamFromIV(mechanism, &iv, effKeyBits);
    if (param == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_IV_PARAM_FAILURE);
    	goto fail;
    }
    clientContext = PK11_CreateContextBySymKey(mechanism,
				(ss->sec.isServer ? CKA_DECRYPT : CKA_ENCRYPT),
				pwSpec->client.write_key, param);
    iv.data = PK11_IVFromParam(mechanism, param, (int *)&iv.len);
    if (iv.data)
    	PORT_Memcpy(pwSpec->client.write_iv, iv.data, iv.len);
    SECITEM_FreeItem(param,PR_TRUE);
    if (clientContext == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_SYM_KEY_CONTEXT_FAILURE);
    	goto fail;
    }
    pwSpec->encode  = (SSLCipher) PK11_CipherOp;
    pwSpec->decode  = (SSLCipher) PK11_CipherOp;
    pwSpec->destroy = (SSLDestroy) PK11_DestroyContext;

    pwSpec->encodeContext = (ss->sec.isServer) ? serverContext : clientContext;
    pwSpec->decodeContext = (ss->sec.isServer) ? clientContext : serverContext;

    serverContext = NULL;
    clientContext = NULL;

    ssl3_InitCompressionContext(pwSpec);

    return SECSuccess;

fail:
    if (serverContext != NULL) PK11_DestroyContext(serverContext, PR_TRUE);
    if (clientContext != NULL) PK11_DestroyContext(clientContext, PR_TRUE);
    if (pwSpec->client.write_mac_context != NULL) {
    	PK11_DestroyContext(pwSpec->client.write_mac_context,PR_TRUE);
	pwSpec->client.write_mac_context = NULL;
    }
    if (pwSpec->server.write_mac_context != NULL) {
    	PK11_DestroyContext(pwSpec->server.write_mac_context,PR_TRUE);
	pwSpec->server.write_mac_context = NULL;
    }

    return SECFailure;
}













SECStatus
ssl3_InitPendingCipherSpec(sslSocket *ss, PK11SymKey *pms)
{
    ssl3CipherSpec  *  pwSpec;
    ssl3CipherSpec  *  cwSpec;
    SECStatus          rv;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    ssl_GetSpecWriteLock(ss);	

    PORT_Assert(ss->ssl3.prSpec == ss->ssl3.pwSpec);

    pwSpec        = ss->ssl3.pwSpec;
    cwSpec        = ss->ssl3.cwSpec;

    if (pms || (!pwSpec->msItem.len && !pwSpec->master_secret)) {
	rv = ssl3_DeriveMasterSecret(ss, pms);
	if (rv != SECSuccess) {
	    goto done;  
	}
    }
#ifndef NO_PKCS11_BYPASS
    if (ss->opt.bypassPKCS11 && pwSpec->msItem.len && pwSpec->msItem.data) {
	
	const ssl3KEADef * kea_def = ss->ssl3.hs.kea_def;
	PRBool             isTLS   = (PRBool)(kea_def->tls_keygen ||
                                (pwSpec->version > SSL_LIBRARY_VERSION_3_0));
	pwSpec->bypassCiphers = PR_TRUE;
	rv = ssl3_KeyAndMacDeriveBypass( pwSpec, 
			     (const unsigned char *)&ss->ssl3.hs.client_random,
			     (const unsigned char *)&ss->ssl3.hs.server_random,
			     isTLS, 
			     (PRBool)(kea_def->is_limited));
	if (rv == SECSuccess) {
	    rv = ssl3_InitPendingContextsBypass(ss);
	}
    } else
#endif
    if (pwSpec->master_secret) {
	rv = ssl3_DeriveConnectionKeysPKCS11(ss);
	if (rv == SECSuccess) {
	    rv = ssl3_InitPendingContextsPKCS11(ss);
	}
    } else {
	PORT_Assert(pwSpec->master_secret);
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	rv = SECFailure;
    }
    if (rv != SECSuccess) {
	goto done;
    }

    
    if (!IS_DTLS(ss)) {
	pwSpec->read_seq_num.high = pwSpec->write_seq_num.high = 0;
    } else {
	if (cwSpec->epoch == PR_UINT16_MAX) {
	    



	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    rv = SECFailure;
	    goto done;
	}
	
	pwSpec->epoch = cwSpec->epoch + 1;
	pwSpec->read_seq_num.high = pwSpec->write_seq_num.high =
	    pwSpec->epoch << 16;

	dtls_InitRecvdRecords(&pwSpec->recvdRecords);
    }
    pwSpec->read_seq_num.low = pwSpec->write_seq_num.low = 0;

done:
    ssl_ReleaseSpecWriteLock(ss);	
    if (rv != SECSuccess)
	ssl_MapLowLevelError(SSL_ERROR_SESSION_KEY_GEN_FAILURE);
    return rv;
}




static const unsigned char mac_pad_1 [60] = {
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36
};
static const unsigned char mac_pad_2 [60] = {
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c
};




static SECStatus
ssl3_ComputeRecordMAC(
    ssl3CipherSpec *   spec,
    PRBool             useServerMacKey,
    const unsigned char *header,
    unsigned int       headerLen,
    const SSL3Opaque * input,
    int                inputLength,
    unsigned char *    outbuf,
    unsigned int *     outLength)
{
    const ssl3MACDef * mac_def;
    SECStatus          rv;

    PRINT_BUF(95, (NULL, "frag hash1: header", header, headerLen));
    PRINT_BUF(95, (NULL, "frag hash1: input", input, inputLength));

    mac_def = spec->mac_def;
    if (mac_def->mac == mac_null) {
	*outLength = 0;
	return SECSuccess;
    }
#ifndef NO_PKCS11_BYPASS
    if (spec->bypassCiphers) {
	
	const SECHashObject *hashObj = NULL;
	unsigned int       pad_bytes = 0;
	PRUint64           write_mac_context[MAX_MAC_CONTEXT_LLONGS];

	switch (mac_def->mac) {
	case ssl_mac_null:
	    *outLength = 0;
	    return SECSuccess;
	case ssl_mac_md5:
	    pad_bytes = 48;
	    hashObj = HASH_GetRawHashObject(HASH_AlgMD5);
	    break;
	case ssl_mac_sha:
	    pad_bytes = 40;
	    hashObj = HASH_GetRawHashObject(HASH_AlgSHA1);
	    break;
	case ssl_hmac_md5: 
	    hashObj = HASH_GetRawHashObject(HASH_AlgMD5);
	    break;
	case ssl_hmac_sha: 
	    hashObj = HASH_GetRawHashObject(HASH_AlgSHA1);
	    break;
	case ssl_hmac_sha256: 
	    hashObj = HASH_GetRawHashObject(HASH_AlgSHA256);
	    break;
	default:
	    break;
	}
	if (!hashObj) {
	    PORT_Assert(0);
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return SECFailure;
	}

	if (spec->version <= SSL_LIBRARY_VERSION_3_0) {
	    unsigned int tempLen;
	    unsigned char temp[MAX_MAC_LENGTH];

	    
	    hashObj->begin(write_mac_context);
	    if (useServerMacKey)
		hashObj->update(write_mac_context, 
				spec->server.write_mac_key_item.data,
				spec->server.write_mac_key_item.len);
	    else
		hashObj->update(write_mac_context, 
				spec->client.write_mac_key_item.data,
				spec->client.write_mac_key_item.len);
	    hashObj->update(write_mac_context, mac_pad_1, pad_bytes);
	    hashObj->update(write_mac_context, header, headerLen);
	    hashObj->update(write_mac_context, input, inputLength);
	    hashObj->end(write_mac_context,    temp, &tempLen, sizeof temp);

	    
	    hashObj->begin(write_mac_context);
	    if (useServerMacKey)
		hashObj->update(write_mac_context, 
				spec->server.write_mac_key_item.data,
				spec->server.write_mac_key_item.len);
	    else
		hashObj->update(write_mac_context, 
				spec->client.write_mac_key_item.data,
				spec->client.write_mac_key_item.len);
	    hashObj->update(write_mac_context, mac_pad_2, pad_bytes);
	    hashObj->update(write_mac_context, temp, tempLen);
	    hashObj->end(write_mac_context, outbuf, outLength, spec->mac_size);
	    rv = SECSuccess;
	} else { 
#define cx ((HMACContext *)write_mac_context)
	    if (useServerMacKey) {
		rv = HMAC_Init(cx, hashObj, 
			       spec->server.write_mac_key_item.data,
			       spec->server.write_mac_key_item.len, PR_FALSE);
	    } else {
		rv = HMAC_Init(cx, hashObj, 
			       spec->client.write_mac_key_item.data,
			       spec->client.write_mac_key_item.len, PR_FALSE);
	    }
	    if (rv == SECSuccess) {
		HMAC_Begin(cx);
		HMAC_Update(cx, header, headerLen);
		HMAC_Update(cx, input, inputLength);
		rv = HMAC_Finish(cx, outbuf, outLength, spec->mac_size);
		HMAC_Destroy(cx, PR_FALSE);
	    }
#undef cx
	}
    } else
#endif
    {
	PK11Context *mac_context = 
	    (useServerMacKey ? spec->server.write_mac_context
	                     : spec->client.write_mac_context);
	rv  = PK11_DigestBegin(mac_context);
	rv |= PK11_DigestOp(mac_context, header, headerLen);
	rv |= PK11_DigestOp(mac_context, input, inputLength);
	rv |= PK11_DigestFinal(mac_context, outbuf, outLength, spec->mac_size);
    }

    PORT_Assert(rv != SECSuccess || *outLength == (unsigned)spec->mac_size);

    PRINT_BUF(95, (NULL, "frag hash2: result", outbuf, *outLength));

    if (rv != SECSuccess) {
    	rv = SECFailure;
	ssl_MapLowLevelError(SSL_ERROR_MAC_COMPUTATION_FAILURE);
    }
    return rv;
}







static SECStatus
ssl3_ComputeRecordMACConstantTime(
    ssl3CipherSpec *   spec,
    PRBool             useServerMacKey,
    const unsigned char *header,
    unsigned int       headerLen,
    const SSL3Opaque * input,
    int                inputLen,
    int                originalLen,
    unsigned char *    outbuf,
    unsigned int *     outLen)
{
    CK_MECHANISM_TYPE            macType;
    CK_NSS_MAC_CONSTANT_TIME_PARAMS params;
    SECItem                      param, inputItem, outputItem;
    SECStatus                    rv;
    PK11SymKey *                 key;

    PORT_Assert(inputLen >= spec->mac_size);
    PORT_Assert(originalLen >= inputLen);

    if (spec->bypassCiphers) {
	

	goto fallback;
    }

    if (spec->mac_def->mac == mac_null) {
	*outLen = 0;
	return SECSuccess;
    }

    macType = CKM_NSS_HMAC_CONSTANT_TIME;
    if (spec->version <= SSL_LIBRARY_VERSION_3_0) {
	macType = CKM_NSS_SSL3_MAC_CONSTANT_TIME;
    }

    params.macAlg = spec->mac_def->mmech;
    params.ulBodyTotalLen = originalLen;
    params.pHeader = (unsigned char *) header;  
    params.ulHeaderLen = headerLen;

    param.data = (unsigned char*) &params;
    param.len = sizeof(params);
    param.type = 0;

    inputItem.data = (unsigned char *) input;
    inputItem.len = inputLen;
    inputItem.type = 0;

    outputItem.data = outbuf;
    outputItem.len = *outLen;
    outputItem.type = 0;

    key = spec->server.write_mac_key;
    if (!useServerMacKey) {
	key = spec->client.write_mac_key;
    }

    rv = PK11_SignWithSymKey(key, macType, &param, &outputItem, &inputItem);
    if (rv != SECSuccess) {
	if (PORT_GetError() == SEC_ERROR_INVALID_ALGORITHM) {
	    goto fallback;
	}

	*outLen = 0;
	rv = SECFailure;
	ssl_MapLowLevelError(SSL_ERROR_MAC_COMPUTATION_FAILURE);
	return rv;
    }

    PORT_Assert(outputItem.len == (unsigned)spec->mac_size);
    *outLen = outputItem.len;

    return rv;

fallback:
    

    inputLen -= spec->mac_size;
    return ssl3_ComputeRecordMAC(spec, useServerMacKey, header, headerLen,
				 input, inputLen, outbuf, outLen);
}

static PRBool
ssl3_ClientAuthTokenPresent(sslSessionID *sid) {
    PK11SlotInfo *slot = NULL;
    PRBool isPresent = PR_TRUE;

    
    if (!sid || !sid->u.ssl3.clAuthValid) {
	return PR_TRUE;
    }

    
    slot = SECMOD_LookupSlot(sid->u.ssl3.clAuthModuleID,
	                     sid->u.ssl3.clAuthSlotID);
    if (slot == NULL ||
	!PK11_IsPresent(slot) ||
	sid->u.ssl3.clAuthSeries     != PK11_GetSlotSeries(slot) ||
	sid->u.ssl3.clAuthSlotID     != PK11_GetSlotID(slot)     ||
	sid->u.ssl3.clAuthModuleID   != PK11_GetModuleID(slot)   ||
	(PK11_NeedLogin(slot) && !PK11_IsLoggedIn(slot, NULL))) {
	isPresent = PR_FALSE;
    } 
    if (slot) {
	PK11_FreeSlot(slot);
    }
    return isPresent;
}


SECStatus
ssl3_CompressMACEncryptRecord(ssl3CipherSpec *   cwSpec,
		              PRBool             isServer,
			      PRBool             isDTLS,
			      PRBool             capRecordVersion,
                              SSL3ContentType    type,
		              const SSL3Opaque * pIn,
		              PRUint32           contentLen,
		              sslBuffer *        wrBuf)
{
    const ssl3BulkCipherDef * cipher_def;
    SECStatus                 rv;
    PRUint32                  macLen = 0;
    PRUint32                  fragLen;
    PRUint32  p1Len, p2Len, oddLen = 0;
    PRUint16                  headerLen;
    int                       ivLen = 0;
    int                       cipherBytes = 0;
    unsigned char             pseudoHeader[13];
    unsigned int              pseudoHeaderLen;

    cipher_def = cwSpec->cipher_def;
    headerLen = isDTLS ? DTLS_RECORD_HEADER_LENGTH : SSL3_RECORD_HEADER_LENGTH;

    if (cipher_def->type == type_block &&
	cwSpec->version >= SSL_LIBRARY_VERSION_TLS_1_1) {
	




	ivLen = cipher_def->iv_size;
	if (ivLen > wrBuf->space - headerLen) {
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return SECFailure;
	}
	rv = PK11_GenerateRandom(wrBuf->buf + headerLen, ivLen);
	if (rv != SECSuccess) {
	    ssl_MapLowLevelError(SSL_ERROR_GENERATE_RANDOM_FAILURE);
	    return rv;
	}
	rv = cwSpec->encode( cwSpec->encodeContext, 
	    wrBuf->buf + headerLen,
	    &cipherBytes,                       
	    ivLen,                              
	    wrBuf->buf + headerLen,
	    ivLen);                             
	if (rv != SECSuccess || cipherBytes != ivLen) {
	    PORT_SetError(SSL_ERROR_ENCRYPTION_FAILURE);
	    return SECFailure;
	}
    }

    if (cwSpec->compressor) {
	int outlen;
	rv = cwSpec->compressor(
	    cwSpec->compressContext,
	    wrBuf->buf + headerLen + ivLen, &outlen,
	    wrBuf->space - headerLen - ivLen, pIn, contentLen);
	if (rv != SECSuccess)
	    return rv;
	pIn = wrBuf->buf + headerLen + ivLen;
	contentLen = outlen;
    }

    pseudoHeaderLen = ssl3_BuildRecordPseudoHeader(
	pseudoHeader, cwSpec->write_seq_num, type,
	cwSpec->version >= SSL_LIBRARY_VERSION_TLS_1_0, cwSpec->version,
	isDTLS, contentLen);
    PORT_Assert(pseudoHeaderLen <= sizeof(pseudoHeader));
    if (cipher_def->type == type_aead) {
	const int nonceLen = cipher_def->explicit_nonce_size;
	const int tagLen = cipher_def->tag_size;

	if (headerLen + nonceLen + contentLen + tagLen > wrBuf->space) {
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return SECFailure;
	}

	cipherBytes = contentLen;
	rv = cwSpec->aead(
		isServer ? &cwSpec->server : &cwSpec->client,
		PR_FALSE,                                   
		wrBuf->buf + headerLen,                     
		&cipherBytes,                               
		wrBuf->space - headerLen,                   
		pIn, contentLen,                            
		pseudoHeader, pseudoHeaderLen);
	if (rv != SECSuccess) {
	    PORT_SetError(SSL_ERROR_ENCRYPTION_FAILURE);
	    return SECFailure;
	}
    } else {
	


	rv = ssl3_ComputeRecordMAC(cwSpec, isServer,
	    pseudoHeader, pseudoHeaderLen, pIn, contentLen,
	    wrBuf->buf + headerLen + ivLen + contentLen, &macLen);
	if (rv != SECSuccess) {
	    ssl_MapLowLevelError(SSL_ERROR_MAC_COMPUTATION_FAILURE);
	    return SECFailure;
	}
	p1Len   = contentLen;
	p2Len   = macLen;
	fragLen = contentLen + macLen;	
	PORT_Assert(fragLen <= MAX_FRAGMENT_LENGTH + 1024);

	



	if (cipher_def->type == type_block) {
	    unsigned char * pBuf;
	    int             padding_length;
	    int             i;

	    oddLen = contentLen % cipher_def->block_size;
	    
	    padding_length = cipher_def->block_size - 1 -
			    ((fragLen) & (cipher_def->block_size - 1));
	    fragLen += padding_length + 1;
	    PORT_Assert((fragLen % cipher_def->block_size) == 0);

	    
	    pBuf = &wrBuf->buf[headerLen + ivLen + fragLen - 1];
	    for (i = padding_length + 1; i > 0; --i) {
		*pBuf-- = padding_length;
	    }
	    
	    p2Len = fragLen - p1Len;
	}
	if (p1Len < 256) {
	    oddLen = p1Len;
	    p1Len = 0;
	} else {
	    p1Len -= oddLen;
	}
	if (oddLen) {
	    p2Len += oddLen;
	    PORT_Assert( (cipher_def->block_size < 2) || \
			 (p2Len % cipher_def->block_size) == 0);
	    memmove(wrBuf->buf + headerLen + ivLen + p1Len, pIn + p1Len,
		    oddLen);
	}
	if (p1Len > 0) {
	    int cipherBytesPart1 = -1;
	    rv = cwSpec->encode( cwSpec->encodeContext, 
		wrBuf->buf + headerLen + ivLen,         
		&cipherBytesPart1,                      
		p1Len,                                  
		pIn, p1Len);                      
	    PORT_Assert(rv == SECSuccess && cipherBytesPart1 == (int) p1Len);
	    if (rv != SECSuccess || cipherBytesPart1 != (int) p1Len) {
		PORT_SetError(SSL_ERROR_ENCRYPTION_FAILURE);
		return SECFailure;
	    }
	    cipherBytes += cipherBytesPart1;
	}
	if (p2Len > 0) {
	    int cipherBytesPart2 = -1;
	    rv = cwSpec->encode( cwSpec->encodeContext, 
		wrBuf->buf + headerLen + ivLen + p1Len,
		&cipherBytesPart2,          
		p2Len,                             
		wrBuf->buf + headerLen + ivLen + p1Len,
		p2Len);                            
	    PORT_Assert(rv == SECSuccess && cipherBytesPart2 == (int) p2Len);
	    if (rv != SECSuccess || cipherBytesPart2 != (int) p2Len) {
		PORT_SetError(SSL_ERROR_ENCRYPTION_FAILURE);
		return SECFailure;
	    }
	    cipherBytes += cipherBytesPart2;
	}
    }

    PORT_Assert(cipherBytes <= MAX_FRAGMENT_LENGTH + 1024);

    wrBuf->len    = cipherBytes + headerLen;
    wrBuf->buf[0] = type;
    if (isDTLS) {
	SSL3ProtocolVersion version;

	version = dtls_TLSVersionToDTLSVersion(cwSpec->version);
	wrBuf->buf[1] = MSB(version);
	wrBuf->buf[2] = LSB(version);
	wrBuf->buf[3] = (unsigned char)(cwSpec->write_seq_num.high >> 24);
	wrBuf->buf[4] = (unsigned char)(cwSpec->write_seq_num.high >> 16);
	wrBuf->buf[5] = (unsigned char)(cwSpec->write_seq_num.high >>  8);
	wrBuf->buf[6] = (unsigned char)(cwSpec->write_seq_num.high >>  0);
	wrBuf->buf[7] = (unsigned char)(cwSpec->write_seq_num.low  >> 24);
	wrBuf->buf[8] = (unsigned char)(cwSpec->write_seq_num.low  >> 16);
	wrBuf->buf[9] = (unsigned char)(cwSpec->write_seq_num.low  >>  8);
	wrBuf->buf[10] = (unsigned char)(cwSpec->write_seq_num.low >>  0);
	wrBuf->buf[11] = MSB(cipherBytes);
	wrBuf->buf[12] = LSB(cipherBytes);
    } else {
	SSL3ProtocolVersion version = cwSpec->version;

	if (capRecordVersion) {
	    version = PR_MIN(SSL_LIBRARY_VERSION_TLS_1_0, version);
	}
	wrBuf->buf[1] = MSB(version);
	wrBuf->buf[2] = LSB(version);
	wrBuf->buf[3] = MSB(cipherBytes);
	wrBuf->buf[4] = LSB(cipherBytes);
    }

    ssl3_BumpSequenceNumber(&cwSpec->write_seq_num);

    return SECSuccess;
}
































PRInt32
ssl3_SendRecord(   sslSocket *        ss,
                   DTLSEpoch          epoch, 
                   SSL3ContentType    type,
		   const SSL3Opaque * pIn,   
		   PRInt32            nIn,   
		   PRInt32            flags)
{
    sslBuffer      *          wrBuf 	  = &ss->sec.writeBuf;
    SECStatus                 rv;
    PRInt32                   totalSent   = 0;
    PRBool                    capRecordVersion;

    SSL_TRC(3, ("%d: SSL3[%d] SendRecord type: %s nIn=%d",
		SSL_GETPID(), ss->fd, ssl3_DecodeContentType(type),
		nIn));
    PRINT_BUF(50, (ss, "Send record (plain text)", pIn, nIn));

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss) );

    if (ss->ssl3.fatalAlertSent) {
        SSL_TRC(3, ("%d: SSL3[%d] Suppress write, fatal alert already sent",
                    SSL_GETPID(), ss->fd));
        return SECFailure;
    }

    capRecordVersion = ((flags & ssl_SEND_FLAG_CAP_RECORD_VERSION) != 0);

    if (capRecordVersion) {
	

	PORT_Assert(!IS_DTLS(ss));
	PORT_Assert(!ss->firstHsDone);
	PORT_Assert(type == content_handshake);
	PORT_Assert(ss->ssl3.hs.ws == wait_server_hello);
    }

    if (ss->ssl3.initialized == PR_FALSE) {
	



	PR_ASSERT(type == content_alert);
	rv = ssl3_InitState(ss);
	if (rv != SECSuccess) {
	    return SECFailure;	
    	}
    }

    
    if (!ssl3_ClientAuthTokenPresent(ss->sec.ci.sid)) {
	PORT_SetError(SSL_ERROR_TOKEN_INSERTION_REMOVAL);
	return SECFailure;
    }

    while (nIn > 0) {
	PRUint32  contentLen = PR_MIN(nIn, MAX_FRAGMENT_LENGTH);
	unsigned int spaceNeeded;
	unsigned int numRecords;

	ssl_GetSpecReadLock(ss);    

	if (nIn > 1 && ss->opt.cbcRandomIV &&
	    ss->ssl3.cwSpec->version < SSL_LIBRARY_VERSION_TLS_1_1 &&
	    type == content_application_data &&
	    ss->ssl3.cwSpec->cipher_def->type == type_block ) {
	    


	    numRecords = 2;
	} else {
	    numRecords = 1;
	}

	spaceNeeded = contentLen + (numRecords * SSL3_BUFFER_FUDGE);
	if (ss->ssl3.cwSpec->version >= SSL_LIBRARY_VERSION_TLS_1_1 &&
	    ss->ssl3.cwSpec->cipher_def->type == type_block) {
	    spaceNeeded += ss->ssl3.cwSpec->cipher_def->iv_size;
	}
	if (spaceNeeded > wrBuf->space) {
	    rv = sslBuffer_Grow(wrBuf, spaceNeeded);
	    if (rv != SECSuccess) {
		SSL_DBG(("%d: SSL3[%d]: SendRecord, tried to get %d bytes",
			 SSL_GETPID(), ss->fd, spaceNeeded));
		goto spec_locked_loser; 
	    }
	}

	if (numRecords == 2) {
	    sslBuffer secondRecord;

	    rv = ssl3_CompressMACEncryptRecord(ss->ssl3.cwSpec,
					       ss->sec.isServer, IS_DTLS(ss),
					       capRecordVersion, type, pIn,
					       1, wrBuf);
	    if (rv != SECSuccess)
	        goto spec_locked_loser;

	    PRINT_BUF(50, (ss, "send (encrypted) record data [1/2]:",
	                   wrBuf->buf, wrBuf->len));

	    secondRecord.buf = wrBuf->buf + wrBuf->len;
	    secondRecord.len = 0;
	    secondRecord.space = wrBuf->space - wrBuf->len;

	    rv = ssl3_CompressMACEncryptRecord(ss->ssl3.cwSpec,
	                                       ss->sec.isServer, IS_DTLS(ss),
					       capRecordVersion, type,
					       pIn + 1, contentLen - 1,
	                                       &secondRecord);
	    if (rv == SECSuccess) {
	        PRINT_BUF(50, (ss, "send (encrypted) record data [2/2]:",
	                       secondRecord.buf, secondRecord.len));
	        wrBuf->len += secondRecord.len;
	    }
	} else {
	    if (!IS_DTLS(ss)) {
		rv = ssl3_CompressMACEncryptRecord(ss->ssl3.cwSpec,
						   ss->sec.isServer,
						   IS_DTLS(ss),
						   capRecordVersion,
						   type, pIn,
						   contentLen, wrBuf);
	    } else {
		rv = dtls_CompressMACEncryptRecord(ss, epoch,
						   !!(flags & ssl_SEND_FLAG_USE_EPOCH),
						   type, pIn,
						   contentLen, wrBuf);
	    }

	    if (rv == SECSuccess) {
	        PRINT_BUF(50, (ss, "send (encrypted) record data:",
	                       wrBuf->buf, wrBuf->len));
	    }
	}

spec_locked_loser:
	ssl_ReleaseSpecReadLock(ss); 

	if (rv != SECSuccess)
	    return SECFailure;

	pIn += contentLen;
	nIn -= contentLen;
	PORT_Assert( nIn >= 0 );

	



	if ((ss->pendingBuf.len > 0) ||
	    (flags & ssl_SEND_FLAG_FORCE_INTO_BUFFER)) {

	    rv = ssl_SaveWriteData(ss, wrBuf->buf, wrBuf->len);
	    if (rv != SECSuccess) {
		
		return SECFailure;
	    }
	    wrBuf->len = 0;	

	    if (!(flags & ssl_SEND_FLAG_FORCE_INTO_BUFFER)) {
		PRInt32   sent;
		ss->handshakeBegun = 1;
		sent = ssl_SendSavedWriteData(ss);
		if (sent < 0 && PR_GetError() != PR_WOULD_BLOCK_ERROR) {
		    ssl_MapLowLevelError(SSL_ERROR_SOCKET_WRITE_FAILURE);
		    return SECFailure;
		}
		if (ss->pendingBuf.len) {
		    flags |= ssl_SEND_FLAG_FORCE_INTO_BUFFER;
		}
	    }
	} else if (wrBuf->len > 0) {
	    PRInt32   sent;
	    ss->handshakeBegun = 1;
	    sent = ssl_DefSend(ss, wrBuf->buf, wrBuf->len,
			       flags & ~ssl_SEND_FLAG_MASK);
	    if (sent < 0) {
		if (PR_GetError() != PR_WOULD_BLOCK_ERROR) {
		    ssl_MapLowLevelError(SSL_ERROR_SOCKET_WRITE_FAILURE);
		    return SECFailure;
		}
		
		sent = 0;
	    }
	    wrBuf->len -= sent;
	    if (wrBuf->len) {
		if (IS_DTLS(ss)) {
		    
		    PR_SetError(PR_WOULD_BLOCK_ERROR, 0);
		    return SECFailure;
		}
		


		rv = ssl_SaveWriteData(ss, wrBuf->buf + sent, wrBuf->len);
		if (rv != SECSuccess) {
		    
		    return SECFailure;
		}
	    }
	}
	totalSent += contentLen;
    }
    return totalSent;
}

#define SSL3_PENDING_HIGH_WATER 1024




int
ssl3_SendApplicationData(sslSocket *ss, const unsigned char *in,
			 PRInt32 len, PRInt32 flags)
{
    PRInt32   totalSent	= 0;
    PRInt32   discarded = 0;

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss) );
    
    PORT_Assert(!(flags & (ssl_SEND_FLAG_USE_EPOCH |
			   ssl_SEND_FLAG_NO_RETRANSMIT)));
    if (len < 0 || !in) {
	PORT_SetError(PR_INVALID_ARGUMENT_ERROR);
	return SECFailure;
    }

    if (ss->pendingBuf.len > SSL3_PENDING_HIGH_WATER &&
        !ssl_SocketIsBlocking(ss)) {
	PORT_Assert(!ssl_SocketIsBlocking(ss));
	PORT_SetError(PR_WOULD_BLOCK_ERROR);
	return SECFailure;
    }

    if (ss->appDataBuffered && len) {
	PORT_Assert (in[0] == (unsigned char)(ss->appDataBuffered));
	if (in[0] != (unsigned char)(ss->appDataBuffered)) {
	    PORT_SetError(PR_INVALID_ARGUMENT_ERROR);
	    return SECFailure;
	}
    	in++;
	len--;
	discarded = 1;
    }
    while (len > totalSent) {
	PRInt32   sent, toSend;

	if (totalSent > 0) {
	    





	    ssl_ReleaseXmitBufLock(ss);
	    PR_Sleep(PR_INTERVAL_NO_WAIT);	
	    ssl_GetXmitBufLock(ss);
	}
	toSend = PR_MIN(len - totalSent, MAX_FRAGMENT_LENGTH);
	



	sent = ssl3_SendRecord(ss, 0, content_application_data,
	                       in + totalSent, toSend, flags);
	if (sent < 0) {
	    if (totalSent > 0 && PR_GetError() == PR_WOULD_BLOCK_ERROR) {
		PORT_Assert(ss->lastWriteBlocked);
	    	break;
	    }
	    return SECFailure; 
	}
	totalSent += sent;
	if (ss->pendingBuf.len) {
	    
	    PORT_Assert(!ssl_SocketIsBlocking(ss));
	    PORT_Assert(ss->lastWriteBlocked);
	    break;	
	}
    }
    if (ss->pendingBuf.len) {
	
	PORT_Assert(!ssl_SocketIsBlocking(ss));
	if (totalSent > 0) {
	    ss->appDataBuffered = 0x100 | in[totalSent - 1];
	}

	totalSent = totalSent + discarded - 1;
	if (totalSent <= 0) {
	    PORT_SetError(PR_WOULD_BLOCK_ERROR);
	    totalSent = SECFailure;
	}
	return totalSent;
    } 
    ss->appDataBuffered = 0;
    return totalSent + discarded;
}















static SECStatus
ssl3_FlushHandshake(sslSocket *ss, PRInt32 flags)
{
    if (IS_DTLS(ss)) {
        return dtls_FlushHandshakeMessages(ss, flags);
    } else {
        return ssl3_FlushHandshakeMessages(ss, flags);
    }
}







static SECStatus
ssl3_FlushHandshakeMessages(sslSocket *ss, PRInt32 flags)
{
    static const PRInt32 allowedFlags = ssl_SEND_FLAG_FORCE_INTO_BUFFER |
                                        ssl_SEND_FLAG_CAP_RECORD_VERSION;
    PRInt32 rv = SECSuccess;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss) );

    if (!ss->sec.ci.sendBuf.buf || !ss->sec.ci.sendBuf.len)
	return rv;

    
    PORT_Assert(!(flags & ~allowedFlags));
    if ((flags & ~allowedFlags) != 0) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	rv = SECFailure;
    } else {
	rv = ssl3_SendRecord(ss, 0, content_handshake, ss->sec.ci.sendBuf.buf,
			     ss->sec.ci.sendBuf.len, flags);
    }
    if (rv < 0) { 
    	int err = PORT_GetError();
	PORT_Assert(err != PR_WOULD_BLOCK_ERROR);
	if (err == PR_WOULD_BLOCK_ERROR) {
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	}
    } else if (rv < ss->sec.ci.sendBuf.len) {
    	
	PORT_Assert(rv >= ss->sec.ci.sendBuf.len);
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	rv = SECFailure;
    } else {
	rv = SECSuccess;
    }

    
    ss->sec.ci.sendBuf.len = 0;
    return rv;
}







static SECStatus
ssl3_HandleNoCertificate(sslSocket *ss)
{
    if (ss->sec.peerCert != NULL) {
	if (ss->sec.peerKey != NULL) {
	    SECKEY_DestroyPublicKey(ss->sec.peerKey);
	    ss->sec.peerKey = NULL;
	}
	CERT_DestroyCertificate(ss->sec.peerCert);
	ss->sec.peerCert = NULL;
    }
    ssl3_CleanupPeerCerts(ss);

    






    if ((ss->opt.requireCertificate == SSL_REQUIRE_ALWAYS) ||
	(!ss->firstHsDone && 
	 (ss->opt.requireCertificate == SSL_REQUIRE_FIRST_HANDSHAKE))) {
	PRFileDesc * lower;

	if (ss->sec.uncache)
            ss->sec.uncache(ss->sec.ci.sid);
	SSL3_SendAlert(ss, alert_fatal, bad_certificate);

	lower = ss->fd->lower;
#ifdef _WIN32
	lower->methods->shutdown(lower, PR_SHUTDOWN_SEND);
#else
	lower->methods->shutdown(lower, PR_SHUTDOWN_BOTH);
#endif
	PORT_SetError(SSL_ERROR_NO_CERTIFICATE);
	return SECFailure;
    }
    return SECSuccess;
}



























SECStatus
SSL3_SendAlert(sslSocket *ss, SSL3AlertLevel level, SSL3AlertDescription desc)
{
    PRUint8 	bytes[2];
    SECStatus	rv;

    SSL_TRC(3, ("%d: SSL3[%d]: send alert record, level=%d desc=%d",
		SSL_GETPID(), ss->fd, level, desc));

    bytes[0] = level;
    bytes[1] = desc;

    ssl_GetSSL3HandshakeLock(ss);
    if (level == alert_fatal) {
	if (!ss->opt.noCache && ss->sec.ci.sid && ss->sec.uncache) {
	    ss->sec.uncache(ss->sec.ci.sid);
	}
    }
    ssl_GetXmitBufLock(ss);
    rv = ssl3_FlushHandshake(ss, ssl_SEND_FLAG_FORCE_INTO_BUFFER);
    if (rv == SECSuccess) {
	PRInt32 sent;
	sent = ssl3_SendRecord(ss, 0, content_alert, bytes, 2, 
			       desc == no_certificate 
			       ? ssl_SEND_FLAG_FORCE_INTO_BUFFER : 0);
	rv = (sent >= 0) ? SECSuccess : (SECStatus)sent;
    }
    if (level == alert_fatal) {
        ss->ssl3.fatalAlertSent = PR_TRUE;
    }
    ssl_ReleaseXmitBufLock(ss);
    ssl_ReleaseSSL3HandshakeLock(ss);
    return rv;	
}




static SECStatus
ssl3_IllegalParameter(sslSocket *ss)
{
    (void)SSL3_SendAlert(ss, alert_fatal, illegal_parameter);
    PORT_SetError(ss->sec.isServer ? SSL_ERROR_BAD_CLIENT
                                   : SSL_ERROR_BAD_SERVER );
    return SECFailure;
}




static SECStatus
ssl3_HandshakeFailure(sslSocket *ss)
{
    (void)SSL3_SendAlert(ss, alert_fatal, handshake_failure);
    PORT_SetError( ss->sec.isServer ? SSL_ERROR_BAD_CLIENT
                                    : SSL_ERROR_BAD_SERVER );
    return SECFailure;
}

static void
ssl3_SendAlertForCertError(sslSocket * ss, PRErrorCode errCode)
{
    SSL3AlertDescription desc	= bad_certificate;
    PRBool isTLS = ss->version >= SSL_LIBRARY_VERSION_3_1_TLS;

    switch (errCode) {
    case SEC_ERROR_LIBRARY_FAILURE:     desc = unsupported_certificate; break;
    case SEC_ERROR_EXPIRED_CERTIFICATE: desc = certificate_expired;     break;
    case SEC_ERROR_REVOKED_CERTIFICATE: desc = certificate_revoked;     break;
    case SEC_ERROR_INADEQUATE_KEY_USAGE:
    case SEC_ERROR_INADEQUATE_CERT_TYPE:
		                        desc = certificate_unknown;     break;
    case SEC_ERROR_UNTRUSTED_CERT:
		    desc = isTLS ? access_denied : certificate_unknown; break;
    case SEC_ERROR_UNKNOWN_ISSUER:      
    case SEC_ERROR_UNTRUSTED_ISSUER:    
		    desc = isTLS ? unknown_ca : certificate_unknown; break;
    case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
		    desc = isTLS ? unknown_ca : certificate_expired; break;

    case SEC_ERROR_CERT_NOT_IN_NAME_SPACE:
    case SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID:
    case SEC_ERROR_CA_CERT_INVALID:
    case SEC_ERROR_BAD_SIGNATURE:
    default:                            desc = bad_certificate;     break;
    }
    SSL_DBG(("%d: SSL3[%d]: peer certificate is no good: error=%d",
	     SSL_GETPID(), ss->fd, errCode));

    (void) SSL3_SendAlert(ss, alert_fatal, desc);
}





SECStatus
ssl3_DecodeError(sslSocket *ss)
{
    (void)SSL3_SendAlert(ss, alert_fatal, 
		  ss->version > SSL_LIBRARY_VERSION_3_0 ? decode_error 
							: illegal_parameter);
    PORT_SetError( ss->sec.isServer ? SSL_ERROR_BAD_CLIENT
                                    : SSL_ERROR_BAD_SERVER );
    return SECFailure;
}




static SECStatus
ssl3_HandleAlert(sslSocket *ss, sslBuffer *buf)
{
    SSL3AlertLevel       level;
    SSL3AlertDescription desc;
    int                  error;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    SSL_TRC(3, ("%d: SSL3[%d]: handle alert record", SSL_GETPID(), ss->fd));

    if (buf->len != 2) {
	(void)ssl3_DecodeError(ss);
	PORT_SetError(SSL_ERROR_RX_MALFORMED_ALERT);
	return SECFailure;
    }
    level = (SSL3AlertLevel)buf->buf[0];
    desc  = (SSL3AlertDescription)buf->buf[1];
    buf->len = 0;
    SSL_TRC(5, ("%d: SSL3[%d] received alert, level = %d, description = %d",
        SSL_GETPID(), ss->fd, level, desc));

    switch (desc) {
    case close_notify:		ss->recvdCloseNotify = 1;
		        	error = SSL_ERROR_CLOSE_NOTIFY_ALERT;     break;
    case unexpected_message: 	error = SSL_ERROR_HANDSHAKE_UNEXPECTED_ALERT;
									  break;
    case bad_record_mac: 	error = SSL_ERROR_BAD_MAC_ALERT; 	  break;
    case decryption_failed_RESERVED:
                                error = SSL_ERROR_DECRYPTION_FAILED_ALERT; 
    									  break;
    case record_overflow: 	error = SSL_ERROR_RECORD_OVERFLOW_ALERT;  break;
    case decompression_failure: error = SSL_ERROR_DECOMPRESSION_FAILURE_ALERT;
									  break;
    case handshake_failure: 	error = SSL_ERROR_HANDSHAKE_FAILURE_ALERT;
			        					  break;
    case no_certificate: 	error = SSL_ERROR_NO_CERTIFICATE;	  break;
    case bad_certificate: 	error = SSL_ERROR_BAD_CERT_ALERT; 	  break;
    case unsupported_certificate:error = SSL_ERROR_UNSUPPORTED_CERT_ALERT;break;
    case certificate_revoked: 	error = SSL_ERROR_REVOKED_CERT_ALERT; 	  break;
    case certificate_expired: 	error = SSL_ERROR_EXPIRED_CERT_ALERT; 	  break;
    case certificate_unknown: 	error = SSL_ERROR_CERTIFICATE_UNKNOWN_ALERT;
			        					  break;
    case illegal_parameter: 	error = SSL_ERROR_ILLEGAL_PARAMETER_ALERT;break;
    case inappropriate_fallback:
        error = SSL_ERROR_INAPPROPRIATE_FALLBACK_ALERT;
        break;

    
    case unknown_ca: 		error = SSL_ERROR_UNKNOWN_CA_ALERT;       break;
    case access_denied: 	error = SSL_ERROR_ACCESS_DENIED_ALERT;    break;
    case decode_error: 		error = SSL_ERROR_DECODE_ERROR_ALERT;     break;
    case decrypt_error: 	error = SSL_ERROR_DECRYPT_ERROR_ALERT;    break;
    case export_restriction: 	error = SSL_ERROR_EXPORT_RESTRICTION_ALERT; 
    									  break;
    case protocol_version: 	error = SSL_ERROR_PROTOCOL_VERSION_ALERT; break;
    case insufficient_security: error = SSL_ERROR_INSUFFICIENT_SECURITY_ALERT; 
    									  break;
    case internal_error: 	error = SSL_ERROR_INTERNAL_ERROR_ALERT;   break;
    case user_canceled: 	error = SSL_ERROR_USER_CANCELED_ALERT;    break;
    case no_renegotiation: 	error = SSL_ERROR_NO_RENEGOTIATION_ALERT; break;

    
    case unsupported_extension: 
			error = SSL_ERROR_UNSUPPORTED_EXTENSION_ALERT;    break;
    case certificate_unobtainable: 
			error = SSL_ERROR_CERTIFICATE_UNOBTAINABLE_ALERT; break;
    case unrecognized_name: 
			error = SSL_ERROR_UNRECOGNIZED_NAME_ALERT;        break;
    case bad_certificate_status_response: 
			error = SSL_ERROR_BAD_CERT_STATUS_RESPONSE_ALERT; break;
    case bad_certificate_hash_value: 
			error = SSL_ERROR_BAD_CERT_HASH_VALUE_ALERT;      break;
    default: 		error = SSL_ERROR_RX_UNKNOWN_ALERT;               break;
    }
    if (level == alert_fatal) {
	if (!ss->opt.noCache) {
	    if (ss->sec.uncache)
                ss->sec.uncache(ss->sec.ci.sid);
	}
	if ((ss->ssl3.hs.ws == wait_server_hello) &&
	    (desc == handshake_failure)) {
	    


	    error = SSL_ERROR_NO_CYPHER_OVERLAP;
	}
	PORT_SetError(error);
	return SECFailure;
    }
    if ((desc == no_certificate) && (ss->ssl3.hs.ws == wait_client_cert)) {
    	
	SECStatus rv;

	PORT_Assert(ss->sec.isServer);
	ss->ssl3.hs.ws = wait_client_key;
	rv = ssl3_HandleNoCertificate(ss);
	return rv;
    }
    return SECSuccess;
}











static SECStatus
ssl3_SendChangeCipherSpecs(sslSocket *ss)
{
    PRUint8           change = change_cipher_spec_choice;
    ssl3CipherSpec *  pwSpec;
    SECStatus         rv;
    PRInt32           sent;

    SSL_TRC(3, ("%d: SSL3[%d]: send change_cipher_spec record",
		SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    rv = ssl3_FlushHandshake(ss, ssl_SEND_FLAG_FORCE_INTO_BUFFER);
    if (rv != SECSuccess) {
	return rv;	
    }
    if (!IS_DTLS(ss)) {
	sent = ssl3_SendRecord(ss, 0, content_change_cipher_spec, &change, 1,
			       ssl_SEND_FLAG_FORCE_INTO_BUFFER);
	if (sent < 0) {
	    return (SECStatus)sent;	
	}
    } else {
	rv = dtls_QueueMessage(ss, content_change_cipher_spec, &change, 1);
	if (rv != SECSuccess) {
	    return rv;
	}
    }

    
    ssl_GetSpecWriteLock(ss);	
    pwSpec                     = ss->ssl3.pwSpec;

    ss->ssl3.pwSpec = ss->ssl3.cwSpec;
    ss->ssl3.cwSpec = pwSpec;

    SSL_TRC(3, ("%d: SSL3[%d] Set Current Write Cipher Suite to Pending",
		SSL_GETPID(), ss->fd ));

    
    


    if (ss->ssl3.prSpec == ss->ssl3.pwSpec) {
	if (!IS_DTLS(ss)) {
	    ssl3_DestroyCipherSpec(ss->ssl3.pwSpec, PR_FALSE);
	} else {
	    

	    ss->ssl3.hs.rtTimeoutMs = DTLS_FINISHED_TIMER_MS;
	    dtls_StartTimer(ss, dtls_FinishedTimerCb);
	}
    }
    ssl_ReleaseSpecWriteLock(ss); 

    return SECSuccess;
}







static SECStatus
ssl3_HandleChangeCipherSpecs(sslSocket *ss, sslBuffer *buf)
{
    ssl3CipherSpec *           prSpec;
    SSL3WaitState              ws      = ss->ssl3.hs.ws;
    SSL3ChangeCipherSpecChoice change;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    SSL_TRC(3, ("%d: SSL3[%d]: handle change_cipher_spec record",
		SSL_GETPID(), ss->fd));

    if (ws != wait_change_cipher) {
	if (IS_DTLS(ss)) {
	    
	    SSL_TRC(3, ("%d: SSL3[%d]: discard out of order "
			"DTLS change_cipher_spec",
			SSL_GETPID(), ss->fd));
	    buf->len = 0;
	    return SECSuccess;
	}
	(void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	PORT_SetError(SSL_ERROR_RX_UNEXPECTED_CHANGE_CIPHER);
	return SECFailure;
    }

    if(buf->len != 1) {
	(void)ssl3_DecodeError(ss);
	PORT_SetError(SSL_ERROR_RX_MALFORMED_CHANGE_CIPHER);
	return SECFailure;
    }
    change = (SSL3ChangeCipherSpecChoice)buf->buf[0];
    if (change != change_cipher_spec_choice) {
	
	(void)ssl3_IllegalParameter(ss);
	PORT_SetError(SSL_ERROR_RX_MALFORMED_CHANGE_CIPHER);
	return SECFailure;
    }
    buf->len = 0;

    
    ssl_GetSpecWriteLock(ss);   
    prSpec                    = ss->ssl3.prSpec;

    ss->ssl3.prSpec  = ss->ssl3.crSpec;
    ss->ssl3.crSpec  = prSpec;
    ss->ssl3.hs.ws   = wait_finished;

    SSL_TRC(3, ("%d: SSL3[%d] Set Current Read Cipher Suite to Pending",
		SSL_GETPID(), ss->fd ));

    


    if (ss->ssl3.prSpec == ss->ssl3.pwSpec) {
    	ssl3_DestroyCipherSpec(ss->ssl3.prSpec, PR_FALSE);
    }
    ssl_ReleaseSpecWriteLock(ss);   
    return SECSuccess;
}






static SECStatus
ssl3_DeriveMasterSecret(sslSocket *ss, PK11SymKey *pms)
{
    ssl3CipherSpec *  pwSpec = ss->ssl3.pwSpec;
    const ssl3KEADef *kea_def= ss->ssl3.hs.kea_def;
    unsigned char *   cr     = (unsigned char *)&ss->ssl3.hs.client_random;
    unsigned char *   sr     = (unsigned char *)&ss->ssl3.hs.server_random;
    PRBool            isTLS  = (PRBool)(kea_def->tls_keygen ||
                                (pwSpec->version > SSL_LIBRARY_VERSION_3_0));
    PRBool            isTLS12=
	    (PRBool)(isTLS && pwSpec->version >= SSL_LIBRARY_VERSION_TLS_1_2);
    




    PRBool    isDH = (PRBool) ((ss->ssl3.hs.kea_def->exchKeyType == kt_dh) ||
	                       (ss->ssl3.hs.kea_def->exchKeyType == kt_ecdh));
    SECStatus         rv = SECFailure;
    CK_MECHANISM_TYPE master_derive;
    CK_MECHANISM_TYPE key_derive;
    SECItem           params;
    CK_FLAGS          keyFlags;
    CK_VERSION        pms_version;
    CK_SSL3_MASTER_KEY_DERIVE_PARAMS master_params;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSpecWriteLock(ss));
    PORT_Assert(ss->ssl3.prSpec == ss->ssl3.pwSpec);
    if (isTLS12) {
	if(isDH) master_derive = CKM_NSS_TLS_MASTER_KEY_DERIVE_DH_SHA256;
	else master_derive = CKM_NSS_TLS_MASTER_KEY_DERIVE_SHA256;
	key_derive    = CKM_NSS_TLS_KEY_AND_MAC_DERIVE_SHA256;
	keyFlags      = CKF_SIGN | CKF_VERIFY;
    } else if (isTLS) {
	if(isDH) master_derive = CKM_TLS_MASTER_KEY_DERIVE_DH;
	else master_derive = CKM_TLS_MASTER_KEY_DERIVE;
	key_derive    = CKM_TLS_KEY_AND_MAC_DERIVE;
	keyFlags      = CKF_SIGN | CKF_VERIFY;
    } else {
	if (isDH) master_derive = CKM_SSL3_MASTER_KEY_DERIVE_DH;
	else master_derive = CKM_SSL3_MASTER_KEY_DERIVE;
	key_derive    = CKM_SSL3_KEY_AND_MAC_DERIVE;
	keyFlags      = 0;
    }

    if (pms || !pwSpec->master_secret) {
	if (isDH) {
	    master_params.pVersion                     = NULL;
	} else {
	    master_params.pVersion                     = &pms_version;
	}
	master_params.RandomInfo.pClientRandom     = cr;
	master_params.RandomInfo.ulClientRandomLen = SSL3_RANDOM_LENGTH;
	master_params.RandomInfo.pServerRandom     = sr;
	master_params.RandomInfo.ulServerRandomLen = SSL3_RANDOM_LENGTH;

	params.data = (unsigned char *) &master_params;
	params.len  = sizeof master_params;
    }

    if (pms != NULL) {
#if defined(TRACE)
	if (ssl_trace >= 100) {
	    SECStatus extractRV = PK11_ExtractKeyValue(pms);
	    if (extractRV == SECSuccess) {
		SECItem * keyData = PK11_GetKeyData(pms);
		if (keyData && keyData->data && keyData->len) {
		    ssl_PrintBuf(ss, "Pre-Master Secret", 
				 keyData->data, keyData->len);
		}
	    }
	}
#endif
	pwSpec->master_secret = PK11_DeriveWithFlags(pms, master_derive, 
				&params, key_derive, CKA_DERIVE, 0, keyFlags);
	if (!isDH && pwSpec->master_secret && ss->opt.detectRollBack) {
	    SSL3ProtocolVersion client_version;
	    client_version = pms_version.major << 8 | pms_version.minor;

	    if (IS_DTLS(ss)) {
		client_version = dtls_DTLSVersionToTLSVersion(client_version);
	    }

	    if (client_version != ss->clientHelloVersion) {
		
		PK11_FreeSymKey(pwSpec->master_secret);
	    	pwSpec->master_secret = NULL;
	    }
	}
	if (pwSpec->master_secret == NULL) {
	    
	    PK11SlotInfo * slot = PK11_GetSlotFromKey((PK11SymKey *)pms);
	    PK11SymKey *   fpms = ssl3_GenerateRSAPMS(ss, pwSpec, slot);

	    PK11_FreeSlot(slot);
	    if (fpms != NULL) {
		pwSpec->master_secret = PK11_DeriveWithFlags(fpms, 
					master_derive, &params, key_derive, 
					CKA_DERIVE, 0, keyFlags);
		PK11_FreeSymKey(fpms);
	    }
	}
    }
    if (pwSpec->master_secret == NULL) {
	
	PK11SlotInfo *  slot = PK11_GetInternalSlot();
	PK11SymKey *    fpms = ssl3_GenerateRSAPMS(ss, pwSpec, slot);

	PK11_FreeSlot(slot);
	if (fpms != NULL) {
	    pwSpec->master_secret = PK11_DeriveWithFlags(fpms, 
					master_derive, &params, key_derive, 
					CKA_DERIVE, 0, keyFlags);
	    if (pwSpec->master_secret == NULL) {
	    	pwSpec->master_secret = fpms; 
		fpms = NULL;
	    }
	}
	if (fpms) {
	    PK11_FreeSymKey(fpms);
    	}
    }
    if (pwSpec->master_secret == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_SESSION_KEY_GEN_FAILURE);
	return rv;
    }
#ifndef NO_PKCS11_BYPASS
    if (ss->opt.bypassPKCS11) {
	SECItem * keydata;
	



	rv = PK11_ExtractKeyValue(pwSpec->master_secret);
	if (rv != SECSuccess) {
	    return rv;
	} 
	


	keydata = PK11_GetKeyData(pwSpec->master_secret);
	if (keydata && keydata->len <= sizeof pwSpec->raw_master_secret) {
	    memcpy(pwSpec->raw_master_secret, keydata->data, keydata->len);
	    pwSpec->msItem.data = pwSpec->raw_master_secret;
	    pwSpec->msItem.len  = keydata->len;
	} else {
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return SECFailure;
	}
    }
#endif
    return SECSuccess;
}

















static SECStatus
ssl3_DeriveConnectionKeysPKCS11(sslSocket *ss)
{
    ssl3CipherSpec *         pwSpec     = ss->ssl3.pwSpec;
    const ssl3KEADef *       kea_def    = ss->ssl3.hs.kea_def;
    unsigned char *   cr     = (unsigned char *)&ss->ssl3.hs.client_random;
    unsigned char *   sr     = (unsigned char *)&ss->ssl3.hs.server_random;
    PRBool            isTLS  = (PRBool)(kea_def->tls_keygen ||
                                (pwSpec->version > SSL_LIBRARY_VERSION_3_0));
    PRBool            isTLS12=
	    (PRBool)(isTLS && pwSpec->version >= SSL_LIBRARY_VERSION_TLS_1_2);
    
    const ssl3BulkCipherDef *cipher_def = pwSpec->cipher_def;
    PK11SlotInfo *         slot   = NULL;
    PK11SymKey *           symKey = NULL;
    void *                 pwArg  = ss->pkcs11PinArg;
    int                    keySize;
    CK_SSL3_KEY_MAT_PARAMS key_material_params;
    CK_SSL3_KEY_MAT_OUT    returnedKeys;
    CK_MECHANISM_TYPE      key_derive;
    CK_MECHANISM_TYPE      bulk_mechanism;
    SSLCipherAlgorithm     calg;
    SECItem                params;
    PRBool         skipKeysAndIVs = (PRBool)(cipher_def->calg == calg_null);

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSpecWriteLock(ss));
    PORT_Assert(ss->ssl3.prSpec == ss->ssl3.pwSpec);

    if (!pwSpec->master_secret) {
	PORT_SetError(SSL_ERROR_SESSION_KEY_GEN_FAILURE);
	return SECFailure;
    }
    


    key_material_params.ulMacSizeInBits = pwSpec->mac_size           * BPB;
    key_material_params.ulKeySizeInBits = cipher_def->secret_key_size* BPB;
    key_material_params.ulIVSizeInBits  = cipher_def->iv_size        * BPB;
    if (cipher_def->type == type_block &&
	pwSpec->version >= SSL_LIBRARY_VERSION_TLS_1_1) {
	
	key_material_params.ulIVSizeInBits = 0;
	memset(pwSpec->client.write_iv, 0, cipher_def->iv_size);
	memset(pwSpec->server.write_iv, 0, cipher_def->iv_size);
    }

    key_material_params.bIsExport = (CK_BBOOL)(kea_def->is_limited);

    key_material_params.RandomInfo.pClientRandom     = cr;
    key_material_params.RandomInfo.ulClientRandomLen = SSL3_RANDOM_LENGTH;
    key_material_params.RandomInfo.pServerRandom     = sr;
    key_material_params.RandomInfo.ulServerRandomLen = SSL3_RANDOM_LENGTH;
    key_material_params.pReturnedKeyMaterial         = &returnedKeys;

    returnedKeys.pIVClient = pwSpec->client.write_iv;
    returnedKeys.pIVServer = pwSpec->server.write_iv;
    keySize                = cipher_def->key_size;

    if (skipKeysAndIVs) {
	keySize                             = 0;
        key_material_params.ulKeySizeInBits = 0;
        key_material_params.ulIVSizeInBits  = 0;
    	returnedKeys.pIVClient              = NULL;
    	returnedKeys.pIVServer              = NULL;
    }

    calg = cipher_def->calg;
    PORT_Assert(     alg2Mech[calg].calg == calg);
    bulk_mechanism = alg2Mech[calg].cmech;

    params.data    = (unsigned char *)&key_material_params;
    params.len     = sizeof(key_material_params);

    if (isTLS12) {
	key_derive    = CKM_NSS_TLS_KEY_AND_MAC_DERIVE_SHA256;
    } else if (isTLS) {
	key_derive    = CKM_TLS_KEY_AND_MAC_DERIVE;
    } else {
	key_derive    = CKM_SSL3_KEY_AND_MAC_DERIVE;
    }

    

    symKey = PK11_Derive(pwSpec->master_secret, key_derive, &params,
                         bulk_mechanism, CKA_ENCRYPT, keySize);
    if (!symKey) {
	ssl_MapLowLevelError(SSL_ERROR_SESSION_KEY_GEN_FAILURE);
	return SECFailure;
    }
    



    slot  = PK11_GetSlotFromKey(symKey);

    PK11_FreeSlot(slot); 
    pwSpec->client.write_mac_key =
    	PK11_SymKeyFromHandle(slot, symKey, PK11_OriginDerive,
	    CKM_SSL3_SHA1_MAC, returnedKeys.hClientMacSecret, PR_TRUE, pwArg);
    if (pwSpec->client.write_mac_key == NULL ) {
	goto loser;	
    }
    pwSpec->server.write_mac_key =
    	PK11_SymKeyFromHandle(slot, symKey, PK11_OriginDerive,
	    CKM_SSL3_SHA1_MAC, returnedKeys.hServerMacSecret, PR_TRUE, pwArg);
    if (pwSpec->server.write_mac_key == NULL ) {
	goto loser;	
    }
    if (!skipKeysAndIVs) {
	pwSpec->client.write_key =
		PK11_SymKeyFromHandle(slot, symKey, PK11_OriginDerive,
		     bulk_mechanism, returnedKeys.hClientKey, PR_TRUE, pwArg);
	if (pwSpec->client.write_key == NULL ) {
	    goto loser;	
	}
	pwSpec->server.write_key =
		PK11_SymKeyFromHandle(slot, symKey, PK11_OriginDerive,
		     bulk_mechanism, returnedKeys.hServerKey, PR_TRUE, pwArg);
	if (pwSpec->server.write_key == NULL ) {
	    goto loser;	
	}
    }
    PK11_FreeSymKey(symKey);
    return SECSuccess;


loser:
    if (symKey) PK11_FreeSymKey(symKey);
    ssl_MapLowLevelError(SSL_ERROR_SESSION_KEY_GEN_FAILURE);
    return SECFailure;
}



static SECStatus
ssl3_InitHandshakeHashes(sslSocket *ss)
{
    SSL_TRC(30,("%d: SSL3[%d]: start handshake hashes", SSL_GETPID(), ss->fd));

    PORT_Assert(ss->ssl3.hs.hashType == handshake_hash_unknown);
#ifndef NO_PKCS11_BYPASS
    if (ss->opt.bypassPKCS11) {
	PORT_Assert(!ss->ssl3.hs.sha_obj && !ss->ssl3.hs.sha_clone);
	if (ss->version >= SSL_LIBRARY_VERSION_TLS_1_2) {
	    

	    ss->ssl3.hs.sha_obj = HASH_GetRawHashObject(HASH_AlgSHA256);
	    if (!ss->ssl3.hs.sha_obj) {
		ssl_MapLowLevelError(SSL_ERROR_DIGEST_FAILURE);
		return SECFailure;
	    }
	    ss->ssl3.hs.sha_clone = (void (*)(void *, void *))SHA256_Clone;
	    ss->ssl3.hs.hashType = handshake_hash_single;
	    ss->ssl3.hs.sha_obj->begin(ss->ssl3.hs.sha_cx);
	} else {
	    ss->ssl3.hs.hashType = handshake_hash_combo;
	    MD5_Begin((MD5Context *)ss->ssl3.hs.md5_cx);
	    SHA1_Begin((SHA1Context *)ss->ssl3.hs.sha_cx);
	}
    } else
#endif
    {
	PORT_Assert(!ss->ssl3.hs.md5 && !ss->ssl3.hs.sha);
	




	if (ss->version >= SSL_LIBRARY_VERSION_TLS_1_2) {
	    

	    ss->ssl3.hs.sha = PK11_CreateDigestContext(SEC_OID_SHA256);
	    if (ss->ssl3.hs.sha == NULL) {
		ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
		return SECFailure;
	    }
	    ss->ssl3.hs.hashType = handshake_hash_single;

	    if (PK11_DigestBegin(ss->ssl3.hs.sha) != SECSuccess) {
		ssl_MapLowLevelError(SSL_ERROR_DIGEST_FAILURE);
		return SECFailure;
	    }

	    








	    if (!ss->sec.isServer) {
		ss->ssl3.hs.backupHash = PK11_CreateDigestContext(SEC_OID_SHA1);
		if (ss->ssl3.hs.backupHash == NULL) {
		    ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
		    return SECFailure;
		}

		if (PK11_DigestBegin(ss->ssl3.hs.backupHash) != SECSuccess) {
		    ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
		    return SECFailure;
		}
	    }
	} else {
	    

	    ss->ssl3.hs.md5 = PK11_CreateDigestContext(SEC_OID_MD5);
	    if (ss->ssl3.hs.md5 == NULL) {
		ssl_MapLowLevelError(SSL_ERROR_MD5_DIGEST_FAILURE);
		return SECFailure;
	    }
	    ss->ssl3.hs.sha = PK11_CreateDigestContext(SEC_OID_SHA1);
	    if (ss->ssl3.hs.sha == NULL) {
		PK11_DestroyContext(ss->ssl3.hs.md5, PR_TRUE);
		ss->ssl3.hs.md5 = NULL;
		ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
		return SECFailure;
	    }
	    ss->ssl3.hs.hashType = handshake_hash_combo;

	    if (PK11_DigestBegin(ss->ssl3.hs.md5) != SECSuccess) {
		ssl_MapLowLevelError(SSL_ERROR_MD5_DIGEST_FAILURE);
		return SECFailure;
	    }
	    if (PK11_DigestBegin(ss->ssl3.hs.sha) != SECSuccess) {
		ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
		return SECFailure;
	    }
	}
    }

    if (ss->ssl3.hs.messages.len > 0) {
	if (ssl3_UpdateHandshakeHashes(ss, ss->ssl3.hs.messages.buf,
				       ss->ssl3.hs.messages.len) !=
	    SECSuccess) {
	    return SECFailure;
	}
	PORT_Free(ss->ssl3.hs.messages.buf);
	ss->ssl3.hs.messages.buf = NULL;
	ss->ssl3.hs.messages.len = 0;
	ss->ssl3.hs.messages.space = 0;
    }

    return SECSuccess;
}

static SECStatus 
ssl3_RestartHandshakeHashes(sslSocket *ss)
{
    SECStatus rv = SECSuccess;

    SSL_TRC(30,("%d: SSL3[%d]: reset handshake hashes",
	    SSL_GETPID(), ss->fd ));
    ss->ssl3.hs.hashType = handshake_hash_unknown;
    ss->ssl3.hs.messages.len = 0;
#ifndef NO_PKCS11_BYPASS
    ss->ssl3.hs.sha_obj = NULL;
    ss->ssl3.hs.sha_clone = NULL;
#endif
    if (ss->ssl3.hs.md5) {
	PK11_DestroyContext(ss->ssl3.hs.md5,PR_TRUE);
	ss->ssl3.hs.md5 = NULL;
    }
    if (ss->ssl3.hs.sha) {
	PK11_DestroyContext(ss->ssl3.hs.sha,PR_TRUE);
	ss->ssl3.hs.sha = NULL;
    }
    return rv;
}











static SECStatus
ssl3_UpdateHandshakeHashes(sslSocket *ss, const unsigned char *b,
			   unsigned int l)
{
    SECStatus  rv = SECSuccess;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    

    if (ss->ssl3.hs.hashType == handshake_hash_unknown) {
	return sslBuffer_Append(&ss->ssl3.hs.messages, b, l);
    }

    PRINT_BUF(90, (NULL, "handshake hash input:", b, l));

#ifndef NO_PKCS11_BYPASS
    if (ss->opt.bypassPKCS11) {
	if (ss->ssl3.hs.hashType == handshake_hash_single) {
	    ss->ssl3.hs.sha_obj->update(ss->ssl3.hs.sha_cx, b, l);
	} else {
	    MD5_Update((MD5Context *)ss->ssl3.hs.md5_cx, b, l);
	    SHA1_Update((SHA1Context *)ss->ssl3.hs.sha_cx, b, l);
	}
	return rv;
    }
#endif
    if (ss->ssl3.hs.hashType == handshake_hash_single) {
	rv = PK11_DigestOp(ss->ssl3.hs.sha, b, l);
	if (rv != SECSuccess) {
	    ssl_MapLowLevelError(SSL_ERROR_DIGEST_FAILURE);
	    return rv;
	}
	if (ss->ssl3.hs.backupHash) {
	    rv = PK11_DigestOp(ss->ssl3.hs.backupHash, b, l);
	    if (rv != SECSuccess) {
		ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
		return rv;
	    }
	}
    } else {
	rv = PK11_DigestOp(ss->ssl3.hs.md5, b, l);
	if (rv != SECSuccess) {
	    ssl_MapLowLevelError(SSL_ERROR_MD5_DIGEST_FAILURE);
	    return rv;
	}
	rv = PK11_DigestOp(ss->ssl3.hs.sha, b, l);
	if (rv != SECSuccess) {
	    ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
	    return rv;
	}
    }
    return rv;
}






SECStatus
ssl3_AppendHandshake(sslSocket *ss, const void *void_src, PRInt32 bytes)
{
    unsigned char *  src  = (unsigned char *)void_src;
    int              room = ss->sec.ci.sendBuf.space - ss->sec.ci.sendBuf.len;
    SECStatus        rv;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) ); 

    if (!bytes)
    	return SECSuccess;
    if (ss->sec.ci.sendBuf.space < MAX_SEND_BUF_LENGTH && room < bytes) {
	rv = sslBuffer_Grow(&ss->sec.ci.sendBuf, PR_MAX(MIN_SEND_BUF_LENGTH,
		 PR_MIN(MAX_SEND_BUF_LENGTH, ss->sec.ci.sendBuf.len + bytes)));
	if (rv != SECSuccess)
	    return rv;	
	room = ss->sec.ci.sendBuf.space - ss->sec.ci.sendBuf.len;
    }

    PRINT_BUF(60, (ss, "Append to Handshake", (unsigned char*)void_src, bytes));
    rv = ssl3_UpdateHandshakeHashes(ss, src, bytes);
    if (rv != SECSuccess)
	return rv;	

    while (bytes > room) {
	if (room > 0)
	    PORT_Memcpy(ss->sec.ci.sendBuf.buf + ss->sec.ci.sendBuf.len, src, 
	                room);
	ss->sec.ci.sendBuf.len += room;
	rv = ssl3_FlushHandshake(ss, ssl_SEND_FLAG_FORCE_INTO_BUFFER);
	if (rv != SECSuccess) {
	    return rv;	
	}
	bytes -= room;
	src += room;
	room = ss->sec.ci.sendBuf.space;
	PORT_Assert(ss->sec.ci.sendBuf.len == 0);
    }
    PORT_Memcpy(ss->sec.ci.sendBuf.buf + ss->sec.ci.sendBuf.len, src, bytes);
    ss->sec.ci.sendBuf.len += bytes;
    return SECSuccess;
}

SECStatus
ssl3_AppendHandshakeNumber(sslSocket *ss, PRInt32 num, PRInt32 lenSize)
{
    SECStatus rv;
    PRUint8   b[4];
    PRUint8 * p = b;

    switch (lenSize) {
      case 4:
	*p++ = (num >> 24) & 0xff;
      case 3:
	*p++ = (num >> 16) & 0xff;
      case 2:
	*p++ = (num >> 8) & 0xff;
      case 1:
	*p = num & 0xff;
    }
    SSL_TRC(60, ("%d: number:", SSL_GETPID()));
    rv = ssl3_AppendHandshake(ss, &b[0], lenSize);
    return rv;	
}

SECStatus
ssl3_AppendHandshakeVariable(
    sslSocket *ss, const SSL3Opaque *src, PRInt32 bytes, PRInt32 lenSize)
{
    SECStatus rv;

    PORT_Assert((bytes < (1<<8) && lenSize == 1) ||
	      (bytes < (1L<<16) && lenSize == 2) ||
	      (bytes < (1L<<24) && lenSize == 3));

    SSL_TRC(60,("%d: append variable:", SSL_GETPID()));
    rv = ssl3_AppendHandshakeNumber(ss, bytes, lenSize);
    if (rv != SECSuccess) {
	return rv;	
    }
    SSL_TRC(60, ("data:"));
    rv = ssl3_AppendHandshake(ss, src, bytes);
    return rv;	
}

SECStatus
ssl3_AppendHandshakeHeader(sslSocket *ss, SSL3HandshakeType t, PRUint32 length)
{
    SECStatus rv;

    



    if (IS_DTLS(ss)) {
	rv = dtls_StageHandshakeMessage(ss);
	if (rv != SECSuccess) {
	    return rv;
	}
    }

    SSL_TRC(30,("%d: SSL3[%d]: append handshake header: type %s",
    	SSL_GETPID(), ss->fd, ssl3_DecodeHandshakeType(t)));

    rv = ssl3_AppendHandshakeNumber(ss, t, 1);
    if (rv != SECSuccess) {
    	return rv;	
    }
    rv = ssl3_AppendHandshakeNumber(ss, length, 3);
    if (rv != SECSuccess) {
    	return rv;	
    }

    if (IS_DTLS(ss)) {
	

	rv = ssl3_AppendHandshakeNumber(ss, ss->ssl3.hs.sendMessageSeq, 2);
	if (rv != SECSuccess) {
	    return rv;	
	}
	ss->ssl3.hs.sendMessageSeq++;

	
	rv = ssl3_AppendHandshakeNumber(ss, 0, 3);
	if (rv != SECSuccess) {
	    return rv;	
	}

	
	rv = ssl3_AppendHandshakeNumber(ss, length, 3);
	if (rv != SECSuccess) {
	    return rv;	
	}
    }

    return rv;		
}



SECStatus
ssl3_AppendSignatureAndHashAlgorithm(
	sslSocket *ss, const SSL3SignatureAndHashAlgorithm* sigAndHash)
{
    unsigned char serialized[2];

    serialized[0] = ssl3_OIDToTLSHashAlgorithm(sigAndHash->hashAlg);
    if (serialized[0] == 0) {
	PORT_SetError(SSL_ERROR_UNSUPPORTED_HASH_ALGORITHM);
	return SECFailure;
    }

    serialized[1] = sigAndHash->sigAlg;

    return ssl3_AppendHandshake(ss, serialized, sizeof(serialized));
}
















SECStatus
ssl3_ConsumeHandshake(sslSocket *ss, void *v, PRInt32 bytes, SSL3Opaque **b,
		      PRUint32 *length)
{
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    if ((PRUint32)bytes > *length) {
	return ssl3_DecodeError(ss);
    }
    PORT_Memcpy(v, *b, bytes);
    PRINT_BUF(60, (ss, "consume bytes:", *b, bytes));
    *b      += bytes;
    *length -= bytes;
    return SECSuccess;
}












PRInt32
ssl3_ConsumeHandshakeNumber(sslSocket *ss, PRInt32 bytes, SSL3Opaque **b,
			    PRUint32 *length)
{
    PRUint8  *buf = *b;
    int       i;
    PRInt32   num = 0;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( bytes <= sizeof num);

    if ((PRUint32)bytes > *length) {
	return ssl3_DecodeError(ss);
    }
    PRINT_BUF(60, (ss, "consume bytes:", *b, bytes));

    for (i = 0; i < bytes; i++)
	num = (num << 8) + buf[i];
    *b      += bytes;
    *length -= bytes;
    return num;
}















SECStatus
ssl3_ConsumeHandshakeVariable(sslSocket *ss, SECItem *i, PRInt32 bytes,
			      SSL3Opaque **b, PRUint32 *length)
{
    PRInt32   count;

    PORT_Assert(bytes <= 3);
    i->len  = 0;
    i->data = NULL;
    count = ssl3_ConsumeHandshakeNumber(ss, bytes, b, length);
    if (count < 0) { 		
    	return SECFailure;
    }
    if (count > 0) {
	if ((PRUint32)count > *length) {
	    return ssl3_DecodeError(ss);
	}
	i->data = *b;
	i->len  = count;
	*b      += count;
	*length -= count;
    }
    return SECSuccess;
}



static const struct {
    int tlsHash;
    SECOidTag oid;
} tlsHashOIDMap[] = {
    { tls_hash_md5, SEC_OID_MD5 },
    { tls_hash_sha1, SEC_OID_SHA1 },
    { tls_hash_sha224, SEC_OID_SHA224 },
    { tls_hash_sha256, SEC_OID_SHA256 },
    { tls_hash_sha384, SEC_OID_SHA384 },
    { tls_hash_sha512, SEC_OID_SHA512 }
};





SECOidTag
ssl3_TLSHashAlgorithmToOID(int hashFunc)
{
    unsigned int i;

    for (i = 0; i < PR_ARRAY_SIZE(tlsHashOIDMap); i++) {
	if (hashFunc == tlsHashOIDMap[i].tlsHash) {
	    return tlsHashOIDMap[i].oid;
	}
    }
    return SEC_OID_UNKNOWN;
}





static int
ssl3_OIDToTLSHashAlgorithm(SECOidTag oid)
{
    unsigned int i;

    for (i = 0; i < PR_ARRAY_SIZE(tlsHashOIDMap); i++) {
	if (oid == tlsHashOIDMap[i].oid) {
	    return tlsHashOIDMap[i].tlsHash;
	}
    }
    return 0;
}



static SECStatus
ssl3_TLSSignatureAlgorithmForKeyType(KeyType keyType,
				     TLSSignatureAlgorithm *out)
{
    switch (keyType) {
    case rsaKey:
	*out = tls_sig_rsa;
	return SECSuccess;
    case dsaKey:
	*out = tls_sig_dsa;
	return SECSuccess;
    case ecKey:
	*out = tls_sig_ecdsa;
	return SECSuccess;
    default:
	PORT_SetError(SEC_ERROR_INVALID_KEY);
	return SECFailure;
    }
}



static SECStatus
ssl3_TLSSignatureAlgorithmForCertificate(CERTCertificate *cert,
					 TLSSignatureAlgorithm *out)
{
    SECKEYPublicKey *key;
    KeyType keyType;

    key = CERT_ExtractPublicKey(cert);
    if (key == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_EXTRACT_PUBLIC_KEY_FAILURE);
    	return SECFailure;
    }

    keyType = key->keyType;
    SECKEY_DestroyPublicKey(key);
    return ssl3_TLSSignatureAlgorithmForKeyType(keyType, out);
}





SECStatus
ssl3_CheckSignatureAndHashAlgorithmConsistency(
	const SSL3SignatureAndHashAlgorithm *sigAndHash, CERTCertificate* cert)
{
    SECStatus rv;
    TLSSignatureAlgorithm sigAlg;

    rv = ssl3_TLSSignatureAlgorithmForCertificate(cert, &sigAlg);
    if (rv != SECSuccess) {
	return rv;
    }
    if (sigAlg != sigAndHash->sigAlg) {
	PORT_SetError(SSL_ERROR_INCORRECT_SIGNATURE_ALGORITHM);
	return SECFailure;
    }
    return SECSuccess;
}






SECStatus
ssl3_ConsumeSignatureAndHashAlgorithm(sslSocket *ss,
				      SSL3Opaque **b,
				      PRUint32 *length,
				      SSL3SignatureAndHashAlgorithm *out)
{
    unsigned char bytes[2];
    SECStatus rv;

    rv = ssl3_ConsumeHandshake(ss, bytes, sizeof(bytes), b, length);
    if (rv != SECSuccess) {
	return rv;
    }

    out->hashAlg = ssl3_TLSHashAlgorithmToOID(bytes[0]);
    if (out->hashAlg == SEC_OID_UNKNOWN) {
	PORT_SetError(SSL_ERROR_UNSUPPORTED_HASH_ALGORITHM);
	return SECFailure;
    }

    out->sigAlg = bytes[1];
    return SECSuccess;
}














static SECStatus
ssl3_ComputeHandshakeHashes(sslSocket *     ss,
                            ssl3CipherSpec *spec,   
			    SSL3Hashes *    hashes, 
			    PRUint32        sender)
{
    SECStatus     rv        = SECSuccess;
    PRBool        isTLS     = (PRBool)(spec->version > SSL_LIBRARY_VERSION_3_0);
    unsigned int  outLength;
    SSL3Opaque    md5_inner[MAX_MAC_LENGTH];
    SSL3Opaque    sha_inner[MAX_MAC_LENGTH];

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    hashes->hashAlg = SEC_OID_UNKNOWN;

#ifndef NO_PKCS11_BYPASS
    if (ss->opt.bypassPKCS11 &&
	ss->ssl3.hs.hashType == handshake_hash_single) {
	
	PRUint64      sha_cx[MAX_MAC_CONTEXT_LLONGS];

	if (!spec->msItem.data) {
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_HANDSHAKE);
	    return SECFailure;
	}

	ss->ssl3.hs.sha_clone(sha_cx, ss->ssl3.hs.sha_cx);
	ss->ssl3.hs.sha_obj->end(sha_cx, hashes->u.raw, &hashes->len,
				 sizeof(hashes->u.raw));

	PRINT_BUF(60, (NULL, "SHA-256: result", hashes->u.raw, hashes->len));

	

	hashes->hashAlg = SEC_OID_SHA256;
	rv = SECSuccess;
    } else if (ss->opt.bypassPKCS11) {
	
	PRUint64      md5_cx[MAX_MAC_CONTEXT_LLONGS];
	PRUint64      sha_cx[MAX_MAC_CONTEXT_LLONGS];

#define md5cx ((MD5Context *)md5_cx)
#define shacx ((SHA1Context *)sha_cx)

	if (!spec->msItem.data) {
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_HANDSHAKE);
	    return SECFailure;
	}

	MD5_Clone (md5cx,  (MD5Context *)ss->ssl3.hs.md5_cx);
	SHA1_Clone(shacx, (SHA1Context *)ss->ssl3.hs.sha_cx);

	if (!isTLS) {
	    
	    unsigned char s[4];

	    s[0] = (unsigned char)(sender >> 24);
	    s[1] = (unsigned char)(sender >> 16);
	    s[2] = (unsigned char)(sender >> 8);
	    s[3] = (unsigned char)sender;

	    if (sender != 0) {
		MD5_Update(md5cx, s, 4);
		PRINT_BUF(95, (NULL, "MD5 inner: sender", s, 4));
	    }

	    PRINT_BUF(95, (NULL, "MD5 inner: MAC Pad 1", mac_pad_1, 
			    mac_defs[mac_md5].pad_size));

	    MD5_Update(md5cx, spec->msItem.data, spec->msItem.len);
	    MD5_Update(md5cx, mac_pad_1, mac_defs[mac_md5].pad_size);
	    MD5_End(md5cx, md5_inner, &outLength, MD5_LENGTH);

	    PRINT_BUF(95, (NULL, "MD5 inner: result", md5_inner, outLength));

	    if (sender != 0) {
		SHA1_Update(shacx, s, 4);
		PRINT_BUF(95, (NULL, "SHA inner: sender", s, 4));
	    }

	    PRINT_BUF(95, (NULL, "SHA inner: MAC Pad 1", mac_pad_1, 
			    mac_defs[mac_sha].pad_size));

	    SHA1_Update(shacx, spec->msItem.data, spec->msItem.len);
	    SHA1_Update(shacx, mac_pad_1, mac_defs[mac_sha].pad_size);
	    SHA1_End(shacx, sha_inner, &outLength, SHA1_LENGTH);

	    PRINT_BUF(95, (NULL, "SHA inner: result", sha_inner, outLength));
	    PRINT_BUF(95, (NULL, "MD5 outer: MAC Pad 2", mac_pad_2, 
			    mac_defs[mac_md5].pad_size));
	    PRINT_BUF(95, (NULL, "MD5 outer: MD5 inner", md5_inner, MD5_LENGTH));

	    MD5_Begin(md5cx);
	    MD5_Update(md5cx, spec->msItem.data, spec->msItem.len);
	    MD5_Update(md5cx, mac_pad_2, mac_defs[mac_md5].pad_size);
	    MD5_Update(md5cx, md5_inner, MD5_LENGTH);
	}
	MD5_End(md5cx, hashes->u.s.md5, &outLength, MD5_LENGTH);

	PRINT_BUF(60, (NULL, "MD5 outer: result", hashes->u.s.md5, MD5_LENGTH));

	if (!isTLS) {
	    PRINT_BUF(95, (NULL, "SHA outer: MAC Pad 2", mac_pad_2, 
			    mac_defs[mac_sha].pad_size));
	    PRINT_BUF(95, (NULL, "SHA outer: SHA inner", sha_inner, SHA1_LENGTH));

	    SHA1_Begin(shacx);
	    SHA1_Update(shacx, spec->msItem.data, spec->msItem.len);
	    SHA1_Update(shacx, mac_pad_2, mac_defs[mac_sha].pad_size);
	    SHA1_Update(shacx, sha_inner, SHA1_LENGTH);
	}
	SHA1_End(shacx, hashes->u.s.sha, &outLength, SHA1_LENGTH);

	PRINT_BUF(60, (NULL, "SHA outer: result", hashes->u.s.sha, SHA1_LENGTH));

	hashes->len = MD5_LENGTH + SHA1_LENGTH;
	rv = SECSuccess;
#undef md5cx
#undef shacx
    } else 
#endif
    if (ss->ssl3.hs.hashType == handshake_hash_single) {
	
	PK11Context *h;
	unsigned int  stateLen;
	unsigned char stackBuf[1024];
	unsigned char *stateBuf = NULL;

	if (!spec->master_secret) {
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_HANDSHAKE);
	    return SECFailure;
	}

	h = ss->ssl3.hs.sha;
	stateBuf = PK11_SaveContextAlloc(h, stackBuf,
					 sizeof(stackBuf), &stateLen);
	if (stateBuf == NULL) {
	    ssl_MapLowLevelError(SSL_ERROR_DIGEST_FAILURE);
	    goto tls12_loser;
	}
	rv |= PK11_DigestFinal(h, hashes->u.raw, &hashes->len,
			       sizeof(hashes->u.raw));
	if (rv != SECSuccess) {
	    ssl_MapLowLevelError(SSL_ERROR_DIGEST_FAILURE);
	    rv = SECFailure;
	    goto tls12_loser;
	}
	

	hashes->hashAlg = SEC_OID_SHA256;
	rv = SECSuccess;

tls12_loser:
	if (stateBuf) {
	    if (PK11_RestoreContext(h, stateBuf, stateLen) != SECSuccess) {
		ssl_MapLowLevelError(SSL_ERROR_DIGEST_FAILURE);
		rv = SECFailure;
	    }
	    if (stateBuf != stackBuf) {
		PORT_ZFree(stateBuf, stateLen);
	    }
	}
    } else {
	
	PK11Context * md5;
	PK11Context * sha       = NULL;
	unsigned char *md5StateBuf = NULL;
	unsigned char *shaStateBuf = NULL;
	unsigned int  md5StateLen, shaStateLen;
	unsigned char md5StackBuf[256];
	unsigned char shaStackBuf[512];

	if (!spec->master_secret) {
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_HANDSHAKE);
	    return SECFailure;
	}

	md5StateBuf = PK11_SaveContextAlloc(ss->ssl3.hs.md5, md5StackBuf,
					    sizeof md5StackBuf, &md5StateLen);
	if (md5StateBuf == NULL) {
	    ssl_MapLowLevelError(SSL_ERROR_MD5_DIGEST_FAILURE);
	    goto loser;
	}
	md5 = ss->ssl3.hs.md5;

	shaStateBuf = PK11_SaveContextAlloc(ss->ssl3.hs.sha, shaStackBuf,
					    sizeof shaStackBuf, &shaStateLen);
	if (shaStateBuf == NULL) {
	    ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
	    goto loser;
	}
	sha = ss->ssl3.hs.sha;

	if (!isTLS) {
	    
	    unsigned char s[4];

	    s[0] = (unsigned char)(sender >> 24);
	    s[1] = (unsigned char)(sender >> 16);
	    s[2] = (unsigned char)(sender >> 8);
	    s[3] = (unsigned char)sender;

	    if (sender != 0) {
		rv |= PK11_DigestOp(md5, s, 4);
		PRINT_BUF(95, (NULL, "MD5 inner: sender", s, 4));
	    }

	    PRINT_BUF(95, (NULL, "MD5 inner: MAC Pad 1", mac_pad_1, 
			  mac_defs[mac_md5].pad_size));

	    rv |= PK11_DigestKey(md5,spec->master_secret);
	    rv |= PK11_DigestOp(md5, mac_pad_1, mac_defs[mac_md5].pad_size);
	    rv |= PK11_DigestFinal(md5, md5_inner, &outLength, MD5_LENGTH);
	    PORT_Assert(rv != SECSuccess || outLength == MD5_LENGTH);
	    if (rv != SECSuccess) {
		ssl_MapLowLevelError(SSL_ERROR_MD5_DIGEST_FAILURE);
		rv = SECFailure;
		goto loser;
	    }

	    PRINT_BUF(95, (NULL, "MD5 inner: result", md5_inner, outLength));

	    if (sender != 0) {
		rv |= PK11_DigestOp(sha, s, 4);
		PRINT_BUF(95, (NULL, "SHA inner: sender", s, 4));
	    }

	    PRINT_BUF(95, (NULL, "SHA inner: MAC Pad 1", mac_pad_1, 
			  mac_defs[mac_sha].pad_size));

	    rv |= PK11_DigestKey(sha, spec->master_secret);
	    rv |= PK11_DigestOp(sha, mac_pad_1, mac_defs[mac_sha].pad_size);
	    rv |= PK11_DigestFinal(sha, sha_inner, &outLength, SHA1_LENGTH);
	    PORT_Assert(rv != SECSuccess || outLength == SHA1_LENGTH);
	    if (rv != SECSuccess) {
		ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
		rv = SECFailure;
		goto loser;
	    }

	    PRINT_BUF(95, (NULL, "SHA inner: result", sha_inner, outLength));

	    PRINT_BUF(95, (NULL, "MD5 outer: MAC Pad 2", mac_pad_2, 
			  mac_defs[mac_md5].pad_size));
	    PRINT_BUF(95, (NULL, "MD5 outer: MD5 inner", md5_inner, MD5_LENGTH));

	    rv |= PK11_DigestBegin(md5);
	    rv |= PK11_DigestKey(md5, spec->master_secret);
	    rv |= PK11_DigestOp(md5, mac_pad_2, mac_defs[mac_md5].pad_size);
	    rv |= PK11_DigestOp(md5, md5_inner, MD5_LENGTH);
	}
	rv |= PK11_DigestFinal(md5, hashes->u.s.md5, &outLength, MD5_LENGTH);
	PORT_Assert(rv != SECSuccess || outLength == MD5_LENGTH);
	if (rv != SECSuccess) {
	    ssl_MapLowLevelError(SSL_ERROR_MD5_DIGEST_FAILURE);
	    rv = SECFailure;
	    goto loser;
	}

	PRINT_BUF(60, (NULL, "MD5 outer: result", hashes->u.s.md5, MD5_LENGTH));

	if (!isTLS) {
	    PRINT_BUF(95, (NULL, "SHA outer: MAC Pad 2", mac_pad_2, 
			  mac_defs[mac_sha].pad_size));
	    PRINT_BUF(95, (NULL, "SHA outer: SHA inner", sha_inner, SHA1_LENGTH));

	    rv |= PK11_DigestBegin(sha);
	    rv |= PK11_DigestKey(sha,spec->master_secret);
	    rv |= PK11_DigestOp(sha, mac_pad_2, mac_defs[mac_sha].pad_size);
	    rv |= PK11_DigestOp(sha, sha_inner, SHA1_LENGTH);
	}
	rv |= PK11_DigestFinal(sha, hashes->u.s.sha, &outLength, SHA1_LENGTH);
	PORT_Assert(rv != SECSuccess || outLength == SHA1_LENGTH);
	if (rv != SECSuccess) {
	    ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
	    rv = SECFailure;
	    goto loser;
	}

	PRINT_BUF(60, (NULL, "SHA outer: result", hashes->u.s.sha, SHA1_LENGTH));

	hashes->len = MD5_LENGTH + SHA1_LENGTH;
	rv = SECSuccess;

    loser:
	if (md5StateBuf) {
	    if (PK11_RestoreContext(ss->ssl3.hs.md5, md5StateBuf, md5StateLen)
		 != SECSuccess) 
	    {
		ssl_MapLowLevelError(SSL_ERROR_MD5_DIGEST_FAILURE);
		rv = SECFailure;
	    }
	    if (md5StateBuf != md5StackBuf) {
		PORT_ZFree(md5StateBuf, md5StateLen);
	    }
	}
	if (shaStateBuf) {
	    if (PK11_RestoreContext(ss->ssl3.hs.sha, shaStateBuf, shaStateLen)
		 != SECSuccess) 
	    {
		ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
		rv = SECFailure;
	    }
	    if (shaStateBuf != shaStackBuf) {
		PORT_ZFree(shaStateBuf, shaStateLen);
	    }
	}
    }
    return rv;
}

static SECStatus
ssl3_ComputeBackupHandshakeHashes(sslSocket * ss,
				  SSL3Hashes * hashes) 
{
    SECStatus rv = SECSuccess;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( !ss->sec.isServer );
    PORT_Assert( ss->ssl3.hs.hashType == handshake_hash_single );

    rv = PK11_DigestFinal(ss->ssl3.hs.backupHash, hashes->u.raw, &hashes->len,
			  sizeof(hashes->u.raw));
    if (rv != SECSuccess) {
	ssl_MapLowLevelError(SSL_ERROR_SHA_DIGEST_FAILURE);
	rv = SECFailure;
	goto loser;
    }
    hashes->hashAlg = SEC_OID_SHA1;

loser:
    PK11_DestroyContext(ss->ssl3.hs.backupHash, PR_TRUE);
    ss->ssl3.hs.backupHash = NULL;
    return rv;
}







SECStatus
ssl3_StartHandshakeHash(sslSocket *ss, unsigned char * buf, int length)
{
    SECStatus rv;

    ssl_GetSSL3HandshakeLock(ss);  

    rv = ssl3_InitState(ss);
    if (rv != SECSuccess) {
	goto done;		
    }
    rv = ssl3_RestartHandshakeHashes(ss);
    if (rv != SECSuccess) {
	goto done;
    }

    PORT_Memset(&ss->ssl3.hs.client_random, 0, SSL3_RANDOM_LENGTH);
    PORT_Memcpy(
	&ss->ssl3.hs.client_random.rand[SSL3_RANDOM_LENGTH - SSL_CHALLENGE_BYTES],
	&ss->sec.ci.clientChallenge,
	SSL_CHALLENGE_BYTES);

    rv = ssl3_UpdateHandshakeHashes(ss, buf, length);
    

done:
    ssl_ReleaseSSL3HandshakeLock(ss);  
    return rv;
}











SECStatus
ssl3_SendClientHello(sslSocket *ss, PRBool resending)
{
    sslSessionID *   sid;
    ssl3CipherSpec * cwSpec;
    SECStatus        rv;
    int              i;
    int              length;
    int              num_suites;
    int              actual_count = 0;
    PRBool           isTLS = PR_FALSE;
    PRBool           requestingResume = PR_FALSE, fallbackSCSV = PR_FALSE;
    PRInt32          total_exten_len = 0;
    unsigned         paddingExtensionLen;
    unsigned         numCompressionMethods;
    PRInt32          flags;

    SSL_TRC(3, ("%d: SSL3[%d]: send client_hello handshake", SSL_GETPID(),
		ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss) );

    rv = ssl3_InitState(ss);
    if (rv != SECSuccess) {
	return rv;		
    }
    ss->ssl3.hs.sendingSCSV = PR_FALSE; 
    PORT_Assert(IS_DTLS(ss) || !resending);

    SECITEM_FreeItem(&ss->ssl3.hs.newSessionTicket.ticket, PR_FALSE);
    ss->ssl3.hs.receivedNewSessionTicket = PR_FALSE;

    


    PORT_Memset(&ss->xtnData, 0, sizeof(TLSExtensionData));

    rv = ssl3_RestartHandshakeHashes(ss);
    if (rv != SECSuccess) {
	return rv;
    }

    



    if (ss->firstHsDone) {
	if (SSL3_ALL_VERSIONS_DISABLED(&ss->vrange)) {
	    PORT_SetError(SSL_ERROR_SSL_DISABLED);
	    return SECFailure;
	}

	if (ss->clientHelloVersion < ss->vrange.min ||
	    ss->clientHelloVersion > ss->vrange.max) {
	    PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	    return SECFailure;
	}
    }

    




    sid = (ss->opt.noCache) ? NULL
	    : ssl_LookupSID(&ss->sec.ci.peer, ss->sec.ci.port, ss->peerID, ss->url);

    




    if (sid) {
	PRBool sidOK = PR_TRUE;
	if (sid->u.ssl3.keys.msIsWrapped) {
	    
	    PK11SlotInfo *slot = NULL;
	    if (sid->u.ssl3.masterValid && !ss->opt.bypassPKCS11) {
		slot = SECMOD_LookupSlot(sid->u.ssl3.masterModuleID,
					 sid->u.ssl3.masterSlotID);
	    }
	    if (slot == NULL) {
	       sidOK = PR_FALSE;
	    } else {
		PK11SymKey *wrapKey = NULL;
		if (!PK11_IsPresent(slot) ||
		    ((wrapKey = PK11_GetWrapKey(slot, 
						sid->u.ssl3.masterWrapIndex,
						sid->u.ssl3.masterWrapMech,
						sid->u.ssl3.masterWrapSeries,
						ss->pkcs11PinArg)) == NULL) ) {
		    sidOK = PR_FALSE;
		}
		if (wrapKey) PK11_FreeSymKey(wrapKey);
		PK11_FreeSlot(slot);
		slot = NULL;
	    }
	}
	



	if (sidOK && !ssl3_ClientAuthTokenPresent(sid)) {
	    sidOK = PR_FALSE;
	}

	if (sidOK) {
            
	    if (ss->firstHsDone) {
		













		if (sid->version >= ss->vrange.min &&
		    sid->version <= ss->clientHelloVersion) {
		    ss->version = ss->clientHelloVersion;
		} else {
		    sidOK = PR_FALSE;
		}
	    } else {
                





		if (sid->version < ss->vrange.min || 
                    sid->version > ss->vrange.max) {
		    sidOK = PR_FALSE;
		} else {
	            rv = ssl3_NegotiateVersion(ss, SSL_LIBRARY_VERSION_MAX_SUPPORTED,
                                               PR_TRUE);
	            if (rv != SECSuccess) {
                        return rv;	
                    }
	        }
	    }
	}

	if (!sidOK) {
	    SSL_AtomicIncrementLong(& ssl3stats.sch_sid_cache_not_ok );
	    if (ss->sec.uncache)
                (*ss->sec.uncache)(sid);
	    ssl_FreeSID(sid);
	    sid = NULL;
	}
    }

    if (sid) {
	requestingResume = PR_TRUE;
	SSL_AtomicIncrementLong(& ssl3stats.sch_sid_cache_hits );

	PRINT_BUF(4, (ss, "client, found session-id:", sid->u.ssl3.sessionID,
		      sid->u.ssl3.sessionIDLength));

	ss->ssl3.policy = sid->u.ssl3.policy;
    } else {
	SSL_AtomicIncrementLong(& ssl3stats.sch_sid_cache_misses );

	







	if (ss->firstHsDone) {
	    ss->version = ss->clientHelloVersion;
	} else {
	    rv = ssl3_NegotiateVersion(ss, SSL_LIBRARY_VERSION_MAX_SUPPORTED,
				       PR_TRUE);
	    if (rv != SECSuccess)
		return rv;	
	}

	sid = ssl3_NewSessionID(ss, PR_FALSE);
	if (!sid) {
	    return SECFailure;	
        }
    }

    isTLS = (ss->version > SSL_LIBRARY_VERSION_3_0);
    ssl_GetSpecWriteLock(ss);
    cwSpec = ss->ssl3.cwSpec;
    if (cwSpec->mac_def->mac == mac_null) {
	
	cwSpec->version = ss->version;
    }
    ssl_ReleaseSpecWriteLock(ss);

    if (ss->sec.ci.sid != NULL) {
	ssl_FreeSID(ss->sec.ci.sid);	
    }
    ss->sec.ci.sid = sid;

    ss->sec.send = ssl3_SendApplicationData;

    
    if (SSL3_ALL_VERSIONS_DISABLED(&ss->vrange)) {
	PR_NOT_REACHED("No versions of SSL 3.0 or later are enabled");
	PORT_SetError(SSL_ERROR_SSL_DISABLED);
    	return SECFailure;
    }

    
    num_suites = ssl3_config_match_init(ss);
    if (!num_suites)
    	return SECFailure;	

    


    if (!ss->firstHsDone && !isTLS) {
	


	ss->ssl3.hs.sendingSCSV = PR_TRUE;
    }

    






    if (sid->u.ssl3.lock) {
        PR_RWLock_Rlock(sid->u.ssl3.lock);
    }

    if (isTLS || (ss->firstHsDone && ss->peerRequestedProtection)) {
	PRUint32 maxBytes = 65535; 
	PRInt32  extLen;

	extLen = ssl3_CallHelloExtensionSenders(ss, PR_FALSE, maxBytes, NULL);
	if (extLen < 0) {
	    if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	    return SECFailure;
	}
	total_exten_len += extLen;

	if (total_exten_len > 0)
	    total_exten_len += 2;
    }

#ifndef NSS_DISABLE_ECC
    if (!total_exten_len || !isTLS) {
	
    	ssl3_DisableECCSuites(ss, NULL); 
    }
#endif 

    if (IS_DTLS(ss)) {
	ssl3_DisableNonDTLSSuites(ss);
    }

    
    num_suites = count_cipher_suites(ss, ss->ssl3.policy, PR_TRUE);
    if (!num_suites) {
    	if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
    	return SECFailure;	
    }

    fallbackSCSV = ss->opt.enableFallbackSCSV && (!requestingResume ||
						  ss->version < sid->version);
    
    if (ss->ssl3.hs.sendingSCSV) {
	++num_suites;
    }
    if (fallbackSCSV) {
	++num_suites;
    }

    
    numCompressionMethods = 0;
    for (i = 0; i < compressionMethodsCount; i++) {
	if (compressionEnabled(ss, compressions[i]))
	    numCompressionMethods++;
    }

    length = sizeof(SSL3ProtocolVersion) + SSL3_RANDOM_LENGTH +
	1 + ((sid == NULL) ? 0 : sid->u.ssl3.sessionIDLength) +
	2 + num_suites*sizeof(ssl3CipherSuite) +
	1 + numCompressionMethods + total_exten_len;
    if (IS_DTLS(ss)) {
	length += 1 + ss->ssl3.hs.cookieLen;
    }

    





    if (!IS_DTLS(ss) && isTLS && !ss->firstHsDone) {
        paddingExtensionLen = ssl3_CalculatePaddingExtensionLength(length);
        total_exten_len += paddingExtensionLen;
        length += paddingExtensionLen;
    } else {
        paddingExtensionLen = 0;
    }

    rv = ssl3_AppendHandshakeHeader(ss, client_hello, length);
    if (rv != SECSuccess) {
	if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	return rv;	
    }

    if (ss->firstHsDone) {
	

	PORT_Assert(ss->version == ss->clientHelloVersion);
    }
    ss->clientHelloVersion = ss->version;
    if (IS_DTLS(ss)) {
	PRUint16 version;

	version = dtls_TLSVersionToDTLSVersion(ss->clientHelloVersion);
	rv = ssl3_AppendHandshakeNumber(ss, version, 2);
    } else {
	rv = ssl3_AppendHandshakeNumber(ss, ss->clientHelloVersion, 2);
    }
    if (rv != SECSuccess) {
	if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	return rv;	
    }

    if (!resending) { 
	rv = ssl3_GetNewRandom(&ss->ssl3.hs.client_random);
	if (rv != SECSuccess) {
	    if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	    return rv;	
	}
    }
    rv = ssl3_AppendHandshake(ss, &ss->ssl3.hs.client_random,
                              SSL3_RANDOM_LENGTH);
    if (rv != SECSuccess) {
	if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	return rv;	
    }

    if (sid)
	rv = ssl3_AppendHandshakeVariable(
	    ss, sid->u.ssl3.sessionID, sid->u.ssl3.sessionIDLength, 1);
    else
	rv = ssl3_AppendHandshakeNumber(ss, 0, 1);
    if (rv != SECSuccess) {
	if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	return rv;	
    }

    if (IS_DTLS(ss)) {
	rv = ssl3_AppendHandshakeVariable(
	    ss, ss->ssl3.hs.cookie, ss->ssl3.hs.cookieLen, 1);
	if (rv != SECSuccess) {
	    if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	    return rv;	
	}
    }

    rv = ssl3_AppendHandshakeNumber(ss, num_suites*sizeof(ssl3CipherSuite), 2);
    if (rv != SECSuccess) {
	if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	return rv;	
    }

    if (ss->ssl3.hs.sendingSCSV) {
	
	rv = ssl3_AppendHandshakeNumber(ss, TLS_EMPTY_RENEGOTIATION_INFO_SCSV,
					sizeof(ssl3CipherSuite));
	if (rv != SECSuccess) {
	    if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	    return rv;	
	}
	actual_count++;
    }
    if (fallbackSCSV) {
	rv = ssl3_AppendHandshakeNumber(ss, TLS_FALLBACK_SCSV,
					sizeof(ssl3CipherSuite));
	if (rv != SECSuccess) {
	    if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	    return rv;	
	}
	actual_count++;
    }
    for (i = 0; i < ssl_V3_SUITES_IMPLEMENTED; i++) {
	ssl3CipherSuiteCfg *suite = &ss->cipherSuites[i];
	if (config_match(suite, ss->ssl3.policy, PR_TRUE, &ss->vrange)) {
	    actual_count++;
	    if (actual_count > num_suites) {
		if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
		
		PORT_SetError(SSL_ERROR_TOKEN_INSERTION_REMOVAL);
		return SECFailure;
	    }
	    rv = ssl3_AppendHandshakeNumber(ss, suite->cipher_suite,
					    sizeof(ssl3CipherSuite));
	    if (rv != SECSuccess) {
		if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
		return rv;	
	    }
	}
    }

    


    if (actual_count != num_suites) {
	
	if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	PORT_SetError(SSL_ERROR_TOKEN_INSERTION_REMOVAL);
	return SECFailure;
    }

    rv = ssl3_AppendHandshakeNumber(ss, numCompressionMethods, 1);
    if (rv != SECSuccess) {
	if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	return rv;	
    }
    for (i = 0; i < compressionMethodsCount; i++) {
	if (!compressionEnabled(ss, compressions[i]))
	    continue;
	rv = ssl3_AppendHandshakeNumber(ss, compressions[i], 1);
	if (rv != SECSuccess) {
	    if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	    return rv;	
	}
    }

    if (total_exten_len) {
	PRUint32 maxBytes = total_exten_len - 2;
	PRInt32  extLen;

	rv = ssl3_AppendHandshakeNumber(ss, maxBytes, 2);
	if (rv != SECSuccess) {
	    if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	    return rv;	
	}

	extLen = ssl3_CallHelloExtensionSenders(ss, PR_TRUE, maxBytes, NULL);
	if (extLen < 0) {
	    if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	    return SECFailure;
	}
	maxBytes -= extLen;

	extLen = ssl3_AppendPaddingExtension(ss, paddingExtensionLen, maxBytes);
	if (extLen < 0) {
	    if (sid->u.ssl3.lock) { PR_RWLock_Unlock(sid->u.ssl3.lock); }
	    return SECFailure;
	}
	maxBytes -= extLen;

	PORT_Assert(!maxBytes);
    } 

    if (sid->u.ssl3.lock) {
        PR_RWLock_Unlock(sid->u.ssl3.lock);
    }

    if (ss->xtnData.sentSessionTicketInClientHello) {
        SSL_AtomicIncrementLong(&ssl3stats.sch_sid_stateless_resumes);
    }

    if (ss->ssl3.hs.sendingSCSV) {
	
	TLSExtensionData *xtnData = &ss->xtnData;
	xtnData->advertised[xtnData->numAdvertised++] = 
	    ssl_renegotiation_info_xtn;
    }

    flags = 0;
    if (!ss->firstHsDone && !IS_DTLS(ss)) {
	flags |= ssl_SEND_FLAG_CAP_RECORD_VERSION;
    }
    rv = ssl3_FlushHandshake(ss, flags);
    if (rv != SECSuccess) {
	return rv;	
    }

    ss->ssl3.hs.ws = wait_server_hello;
    return rv;
}






static SECStatus
ssl3_HandleHelloRequest(sslSocket *ss)
{
    sslSessionID *sid = ss->sec.ci.sid;
    SECStatus     rv;

    SSL_TRC(3, ("%d: SSL3[%d]: handle hello_request handshake",
		SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    if (ss->ssl3.hs.ws == wait_server_hello)
	return SECSuccess;
    if (ss->ssl3.hs.ws != idle_handshake || ss->sec.isServer) {
	(void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	PORT_SetError(SSL_ERROR_RX_UNEXPECTED_HELLO_REQUEST);
	return SECFailure;
    }
    if (ss->opt.enableRenegotiation == SSL_RENEGOTIATE_NEVER) {
	ssl_GetXmitBufLock(ss);
	rv = SSL3_SendAlert(ss, alert_warning, no_renegotiation);
	ssl_ReleaseXmitBufLock(ss);
	PORT_SetError(SSL_ERROR_RENEGOTIATION_NOT_ALLOWED);
	return SECFailure;
    }

    if (sid) {
	if (ss->sec.uncache)
            ss->sec.uncache(sid);
	ssl_FreeSID(sid);
	ss->sec.ci.sid = NULL;
    }

    if (IS_DTLS(ss)) {
	dtls_RehandshakeCleanup(ss);
    }

    ssl_GetXmitBufLock(ss);
    rv = ssl3_SendClientHello(ss, PR_FALSE);
    ssl_ReleaseXmitBufLock(ss);

    return rv;
}

#define UNKNOWN_WRAP_MECHANISM 0x7fffffff

static const CK_MECHANISM_TYPE wrapMechanismList[SSL_NUM_WRAP_MECHS] = {
    CKM_DES3_ECB,
    CKM_CAST5_ECB,
    CKM_DES_ECB,
    CKM_KEY_WRAP_LYNKS,
    CKM_IDEA_ECB,
    CKM_CAST3_ECB,
    CKM_CAST_ECB,
    CKM_RC5_ECB,
    CKM_RC2_ECB,
    CKM_CDMF_ECB,
    CKM_SKIPJACK_WRAP,
    CKM_SKIPJACK_CBC64,
    CKM_AES_ECB,
    CKM_CAMELLIA_ECB,
    CKM_SEED_ECB,
    UNKNOWN_WRAP_MECHANISM
};

static int
ssl_FindIndexByWrapMechanism(CK_MECHANISM_TYPE mech)
{
    const CK_MECHANISM_TYPE *pMech = wrapMechanismList;

    while (mech != *pMech && *pMech != UNKNOWN_WRAP_MECHANISM) {
    	++pMech;
    }
    return (*pMech == UNKNOWN_WRAP_MECHANISM) ? -1
                                              : (pMech - wrapMechanismList);
}

static PK11SymKey *
ssl_UnwrapSymWrappingKey(
	SSLWrappedSymWrappingKey *pWswk,
	SECKEYPrivateKey *        svrPrivKey,
	SSL3KEAType               exchKeyType,
	CK_MECHANISM_TYPE         masterWrapMech,
	void *                    pwArg)
{
    PK11SymKey *             unwrappedWrappingKey  = NULL;
    SECItem                  wrappedKey;
#ifndef NSS_DISABLE_ECC
    PK11SymKey *             Ks;
    SECKEYPublicKey          pubWrapKey;
    ECCWrappedKeyInfo        *ecWrapped;
#endif 

    
    PORT_Assert(pWswk->symWrapMechanism == masterWrapMech);
    PORT_Assert(pWswk->exchKeyType      == exchKeyType);
    if (pWswk->symWrapMechanism != masterWrapMech ||
	pWswk->exchKeyType      != exchKeyType) {
	goto loser;
    }
    wrappedKey.type = siBuffer;
    wrappedKey.data = pWswk->wrappedSymmetricWrappingkey;
    wrappedKey.len  = pWswk->wrappedSymKeyLen;
    PORT_Assert(wrappedKey.len <= sizeof pWswk->wrappedSymmetricWrappingkey);

    switch (exchKeyType) {

    case kt_rsa:
	unwrappedWrappingKey =
	    PK11_PubUnwrapSymKey(svrPrivKey, &wrappedKey,
				 masterWrapMech, CKA_UNWRAP, 0);
	break;

#ifndef NSS_DISABLE_ECC
    case kt_ecdh:
        










        ecWrapped = (ECCWrappedKeyInfo *) pWswk->wrappedSymmetricWrappingkey;

        PORT_Assert(ecWrapped->encodedParamLen + ecWrapped->pubValueLen + 
            ecWrapped->wrappedKeyLen <= MAX_EC_WRAPPED_KEY_BUFLEN);

        if (ecWrapped->encodedParamLen + ecWrapped->pubValueLen + 
            ecWrapped->wrappedKeyLen > MAX_EC_WRAPPED_KEY_BUFLEN) {
            PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
            goto loser;
        }

        pubWrapKey.keyType = ecKey;
        pubWrapKey.u.ec.size = ecWrapped->size;
        pubWrapKey.u.ec.DEREncodedParams.len = ecWrapped->encodedParamLen;
        pubWrapKey.u.ec.DEREncodedParams.data = ecWrapped->var;
        pubWrapKey.u.ec.publicValue.len = ecWrapped->pubValueLen;
        pubWrapKey.u.ec.publicValue.data = ecWrapped->var + 
            ecWrapped->encodedParamLen;

        wrappedKey.len  = ecWrapped->wrappedKeyLen;
        wrappedKey.data = ecWrapped->var + ecWrapped->encodedParamLen + 
            ecWrapped->pubValueLen;
        
        
        Ks = PK11_PubDeriveWithKDF(svrPrivKey, &pubWrapKey, PR_FALSE, NULL,
				   NULL, CKM_ECDH1_DERIVE, masterWrapMech, 
				   CKA_DERIVE, 0, CKD_NULL, NULL, NULL);
        if (Ks == NULL) {
            goto loser;
        }

        
        unwrappedWrappingKey = PK11_UnwrapSymKey(Ks, masterWrapMech, NULL, 
						 &wrappedKey, masterWrapMech, 
						 CKA_UNWRAP, 0);
        PK11_FreeSymKey(Ks);
        
        break;
#endif

    default:
	
	SET_ERROR_CODE
	goto loser;
    }
loser:
    return unwrappedWrappingKey;
}







typedef struct {
    PK11SymKey *      symWrapKey[kt_kea_size];
} ssl3SymWrapKey;

static PZLock *          symWrapKeysLock = NULL;
static ssl3SymWrapKey    symWrapKeys[SSL_NUM_WRAP_MECHS];

SECStatus ssl_FreeSymWrapKeysLock(void)
{
    if (symWrapKeysLock) {
        PZ_DestroyLock(symWrapKeysLock);
        symWrapKeysLock = NULL;
        return SECSuccess;
    }
    PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
    return SECFailure;
}

SECStatus
SSL3_ShutdownServerCache(void)
{
    int             i, j;

    if (!symWrapKeysLock)
    	return SECSuccess;	
    PZ_Lock(symWrapKeysLock);
    
    for (i = 0; i < SSL_NUM_WRAP_MECHS; ++i) {
    	for (j = 0; j < kt_kea_size; ++j) {
	    PK11SymKey **   pSymWrapKey;
	    pSymWrapKey = &symWrapKeys[i].symWrapKey[j];
	    if (*pSymWrapKey) {
		PK11_FreeSymKey(*pSymWrapKey);
	    	*pSymWrapKey = NULL;
	    }
	}
    }

    PZ_Unlock(symWrapKeysLock);
    ssl_FreeSessionCacheLocks();
    return SECSuccess;
}

SECStatus ssl_InitSymWrapKeysLock(void)
{
    symWrapKeysLock = PZ_NewLock(nssILockOther);
    return symWrapKeysLock ? SECSuccess : SECFailure;
}






static PK11SymKey *
getWrappingKey( sslSocket *       ss,
		PK11SlotInfo *    masterSecretSlot,
		SSL3KEAType       exchKeyType,
                CK_MECHANISM_TYPE masterWrapMech,
	        void *            pwArg)
{
    SECKEYPrivateKey *       svrPrivKey;
    SECKEYPublicKey *        svrPubKey             = NULL;
    PK11SymKey *             unwrappedWrappingKey  = NULL;
    PK11SymKey **            pSymWrapKey;
    CK_MECHANISM_TYPE        asymWrapMechanism = CKM_INVALID_MECHANISM;
    int                      length;
    int                      symWrapMechIndex;
    SECStatus                rv;
    SECItem                  wrappedKey;
    SSLWrappedSymWrappingKey wswk;
#ifndef NSS_DISABLE_ECC
    PK11SymKey *      Ks = NULL;
    SECKEYPublicKey   *pubWrapKey = NULL;
    SECKEYPrivateKey  *privWrapKey = NULL;
    ECCWrappedKeyInfo *ecWrapped;
#endif 

    svrPrivKey  = ss->serverCerts[exchKeyType].SERVERKEY;
    PORT_Assert(svrPrivKey != NULL);
    if (!svrPrivKey) {
    	return NULL;	
    }

    symWrapMechIndex = ssl_FindIndexByWrapMechanism(masterWrapMech);
    PORT_Assert(symWrapMechIndex >= 0);
    if (symWrapMechIndex < 0)
    	return NULL;	

    pSymWrapKey = &symWrapKeys[symWrapMechIndex].symWrapKey[exchKeyType];

    ssl_InitSessionCacheLocks(PR_TRUE);

    PZ_Lock(symWrapKeysLock);

    unwrappedWrappingKey = *pSymWrapKey;
    if (unwrappedWrappingKey != NULL) {
	if (PK11_VerifyKeyOK(unwrappedWrappingKey)) {
	    unwrappedWrappingKey = PK11_ReferenceSymKey(unwrappedWrappingKey);
	    goto done;
	}
	
	PK11_FreeSymKey(unwrappedWrappingKey);
	*pSymWrapKey = unwrappedWrappingKey = NULL;
    }

    
    
    if (ssl_GetWrappingKey(symWrapMechIndex, exchKeyType, &wswk)) {
    	
	unwrappedWrappingKey =
	    ssl_UnwrapSymWrappingKey(&wswk, svrPrivKey, exchKeyType,
                                     masterWrapMech, pwArg);
	if (unwrappedWrappingKey) {
	    goto install;
	}
    }

    if (!masterSecretSlot) 	
    	goto loser;

    length = PK11_GetBestKeyLength(masterSecretSlot, masterWrapMech);
    


    unwrappedWrappingKey = PK11_KeyGen(masterSecretSlot, masterWrapMech, NULL,
                                       length, pwArg);
    if (!unwrappedWrappingKey) {
    	goto loser;
    }

    


    PORT_Memset(&wswk, 0, sizeof wswk);	

    if (ss->serverCerts[exchKeyType].serverKeyPair) {
	svrPubKey = ss->serverCerts[exchKeyType].serverKeyPair->pubKey;
    }
    if (svrPubKey == NULL) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	goto loser;
    }
    wrappedKey.type = siBuffer;
    wrappedKey.len  = SECKEY_PublicKeyStrength(svrPubKey);
    wrappedKey.data = wswk.wrappedSymmetricWrappingkey;

    PORT_Assert(wrappedKey.len <= sizeof wswk.wrappedSymmetricWrappingkey);
    if (wrappedKey.len > sizeof wswk.wrappedSymmetricWrappingkey)
    	goto loser;

    
    switch (exchKeyType) {
    case kt_rsa:
	asymWrapMechanism = CKM_RSA_PKCS;
	rv = PK11_PubWrapSymKey(asymWrapMechanism, svrPubKey,
	                        unwrappedWrappingKey, &wrappedKey);
	break;

#ifndef NSS_DISABLE_ECC
    case kt_ecdh:
	










	PORT_Assert(svrPubKey->keyType == ecKey);
	if (svrPubKey->keyType != ecKey) {
	    
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    rv = SECFailure;
	    goto ec_cleanup;
	}

	privWrapKey = SECKEY_CreateECPrivateKey(
	    &svrPubKey->u.ec.DEREncodedParams, &pubWrapKey, NULL);
	if ((privWrapKey == NULL) || (pubWrapKey == NULL)) {
	    rv = SECFailure;
	    goto ec_cleanup;
	}
	
	
	if (pubWrapKey->u.ec.size == 0) {
	    pubWrapKey->u.ec.size = SECKEY_PublicKeyStrengthInBits(svrPubKey);
	}

	PORT_Assert(pubWrapKey->u.ec.DEREncodedParams.len + 
	    pubWrapKey->u.ec.publicValue.len < MAX_EC_WRAPPED_KEY_BUFLEN);
	if (pubWrapKey->u.ec.DEREncodedParams.len + 
	    pubWrapKey->u.ec.publicValue.len >= MAX_EC_WRAPPED_KEY_BUFLEN) {
	    PORT_SetError(SEC_ERROR_INVALID_KEY);
	    rv = SECFailure;
	    goto ec_cleanup;
	}

	
	Ks = PK11_PubDeriveWithKDF(svrPrivKey, pubWrapKey, PR_FALSE, NULL,
				   NULL, CKM_ECDH1_DERIVE, masterWrapMech, 
				   CKA_DERIVE, 0, CKD_NULL, NULL, NULL);
	if (Ks == NULL) {
	    rv = SECFailure;
	    goto ec_cleanup;
	}

	ecWrapped = (ECCWrappedKeyInfo *) (wswk.wrappedSymmetricWrappingkey);
	ecWrapped->size = pubWrapKey->u.ec.size;
	ecWrapped->encodedParamLen = pubWrapKey->u.ec.DEREncodedParams.len;
	PORT_Memcpy(ecWrapped->var, pubWrapKey->u.ec.DEREncodedParams.data, 
	    pubWrapKey->u.ec.DEREncodedParams.len);

	ecWrapped->pubValueLen = pubWrapKey->u.ec.publicValue.len;
	PORT_Memcpy(ecWrapped->var + ecWrapped->encodedParamLen, 
		    pubWrapKey->u.ec.publicValue.data, 
		    pubWrapKey->u.ec.publicValue.len);

	wrappedKey.len = MAX_EC_WRAPPED_KEY_BUFLEN - 
	    (ecWrapped->encodedParamLen + ecWrapped->pubValueLen);
	wrappedKey.data = ecWrapped->var + ecWrapped->encodedParamLen +
	    ecWrapped->pubValueLen;

	
	rv = PK11_WrapSymKey(masterWrapMech, NULL, Ks,
			     unwrappedWrappingKey, &wrappedKey);

	if (rv != SECSuccess) {
	    goto ec_cleanup;
	}

	


	ecWrapped->wrappedKeyLen = wrappedKey.len;

ec_cleanup:
	if (privWrapKey) SECKEY_DestroyPrivateKey(privWrapKey);
	if (pubWrapKey) SECKEY_DestroyPublicKey(pubWrapKey);
	if (Ks) PK11_FreeSymKey(Ks);
	asymWrapMechanism = masterWrapMech;
	break;
#endif 

    default:
	rv = SECFailure;
	break;
    }

    if (rv != SECSuccess) {
	ssl_MapLowLevelError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
	goto loser;
    }

    PORT_Assert(asymWrapMechanism != CKM_INVALID_MECHANISM);

    wswk.symWrapMechanism  = masterWrapMech;
    wswk.symWrapMechIndex  = symWrapMechIndex;
    wswk.asymWrapMechanism = asymWrapMechanism;
    wswk.exchKeyType       = exchKeyType;
    wswk.wrappedSymKeyLen  = wrappedKey.len;

    
    



    if (ssl_SetWrappingKey(&wswk)) {
    	



    	PK11_FreeSymKey(unwrappedWrappingKey);

	unwrappedWrappingKey =
	    ssl_UnwrapSymWrappingKey(&wswk, svrPrivKey, exchKeyType,
                                     masterWrapMech, pwArg);
    }

install:
    if (unwrappedWrappingKey) {
	*pSymWrapKey = PK11_ReferenceSymKey(unwrappedWrappingKey);
    }

loser:
done:
    PZ_Unlock(symWrapKeysLock);
    return unwrappedWrappingKey;
}



static void
hexEncode(char *out, const unsigned char *in, unsigned int length)
{
    static const char hextable[] = "0123456789abcdef";
    unsigned int i;

    for (i = 0; i < length; i++) {
	*(out++) = hextable[in[i] >> 4];
	*(out++) = hextable[in[i] & 15];
    }
}



static SECStatus
sendRSAClientKeyExchange(sslSocket * ss, SECKEYPublicKey * svrPubKey)
{
    PK11SymKey *	pms 		= NULL;
    SECStatus           rv    		= SECFailure;
    SECItem 		enc_pms 	= {siBuffer, NULL, 0};
    PRBool              isTLS;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));

    
    ssl_GetSpecWriteLock(ss);
    isTLS = (PRBool)(ss->ssl3.pwSpec->version > SSL_LIBRARY_VERSION_3_0);

    pms = ssl3_GenerateRSAPMS(ss, ss->ssl3.pwSpec, NULL);
    ssl_ReleaseSpecWriteLock(ss);
    if (pms == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
	goto loser;
    }

    
    enc_pms.len  = SECKEY_PublicKeyStrength(svrPubKey);
    enc_pms.data = (unsigned char*)PORT_Alloc(enc_pms.len);
    if (enc_pms.data == NULL) {
	goto loser;	
    }

    
    rv = PK11_PubWrapSymKey(CKM_RSA_PKCS, svrPubKey, pms, &enc_pms);
    if (rv != SECSuccess) {
	ssl_MapLowLevelError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
	goto loser;
    }

    if (ssl_keylog_iob) {
	SECStatus extractRV = PK11_ExtractKeyValue(pms);
	if (extractRV == SECSuccess) {
	    SECItem * keyData = PK11_GetKeyData(pms);
	    if (keyData && keyData->data && keyData->len) {
#ifdef TRACE
		if (ssl_trace >= 100) {
		    ssl_PrintBuf(ss, "Pre-Master Secret",
				 keyData->data, keyData->len);
		}
#endif
		if (ssl_keylog_iob && enc_pms.len >= 8 && keyData->len == 48) {
		    

		    


		    char buf[4 + 8*2 + 1 + 48*2 + 1];

		    strcpy(buf, "RSA ");
		    hexEncode(buf + 4, enc_pms.data, 8);
		    buf[20] = ' ';
		    hexEncode(buf + 21, keyData->data, 48);
		    buf[sizeof(buf) - 1] = '\n';

		    fwrite(buf, sizeof(buf), 1, ssl_keylog_iob);
		    fflush(ssl_keylog_iob);
		}
	    }
	}
    }

    rv = ssl3_InitPendingCipherSpec(ss,  pms);
    PK11_FreeSymKey(pms); pms = NULL;

    if (rv != SECSuccess) {
	ssl_MapLowLevelError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
	goto loser;
    }

    rv = ssl3_AppendHandshakeHeader(ss, client_key_exchange, 
				    isTLS ? enc_pms.len + 2 : enc_pms.len);
    if (rv != SECSuccess) {
	goto loser;	
    }
    if (isTLS) {
    	rv = ssl3_AppendHandshakeVariable(ss, enc_pms.data, enc_pms.len, 2);
    } else {
	rv = ssl3_AppendHandshake(ss, enc_pms.data, enc_pms.len);
    }
    if (rv != SECSuccess) {
	goto loser;	
    }

    rv = SECSuccess;

loser:
    if (enc_pms.data != NULL) {
	PORT_Free(enc_pms.data);
    }
    if (pms != NULL) {
    	PK11_FreeSymKey(pms);
    }
    return rv;
}



static SECStatus
sendDHClientKeyExchange(sslSocket * ss, SECKEYPublicKey * svrPubKey)
{
    PK11SymKey *	pms 		= NULL;
    SECStatus           rv    		= SECFailure;
    PRBool              isTLS;
    CK_MECHANISM_TYPE	target;

    SECKEYDHParams	dhParam;		
    SECKEYPublicKey	*pubKey = NULL;		
    SECKEYPrivateKey	*privKey = NULL;	

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));

    isTLS = (PRBool)(ss->ssl3.pwSpec->version > SSL_LIBRARY_VERSION_3_0);

    

    if (svrPubKey->keyType != dhKey) {
	PORT_SetError(SEC_ERROR_BAD_KEY);
	goto loser;
    }
    dhParam.prime.data = svrPubKey->u.dh.prime.data;
    dhParam.prime.len = svrPubKey->u.dh.prime.len;
    dhParam.base.data = svrPubKey->u.dh.base.data;
    dhParam.base.len = svrPubKey->u.dh.base.len;

    
    privKey = SECKEY_CreateDHPrivateKey(&dhParam, &pubKey, NULL);
    if (!privKey || !pubKey) {
	    ssl_MapLowLevelError(SEC_ERROR_KEYGEN_FAIL);
	    rv = SECFailure;
	    goto loser;
    }
    PRINT_BUF(50, (ss, "DH public value:",
					pubKey->u.dh.publicValue.data,
					pubKey->u.dh.publicValue.len));

    if (isTLS) target = CKM_TLS_MASTER_KEY_DERIVE_DH;
    else target = CKM_SSL3_MASTER_KEY_DERIVE_DH;

    

    pms = PK11_PubDerive(privKey, svrPubKey, PR_FALSE, NULL, NULL,
			    CKM_DH_PKCS_DERIVE, target, CKA_DERIVE, 0, NULL);

    if (pms == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
	goto loser;
    }

    SECKEY_DestroyPrivateKey(privKey);
    privKey = NULL;

    rv = ssl3_InitPendingCipherSpec(ss,  pms);
    PK11_FreeSymKey(pms); pms = NULL;

    if (rv != SECSuccess) {
	ssl_MapLowLevelError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
	goto loser;
    }

    rv = ssl3_AppendHandshakeHeader(ss, client_key_exchange, 
					pubKey->u.dh.publicValue.len + 2);
    if (rv != SECSuccess) {
	goto loser;	
    }
    rv = ssl3_AppendHandshakeVariable(ss, 
					pubKey->u.dh.publicValue.data,
					pubKey->u.dh.publicValue.len, 2);
    SECKEY_DestroyPublicKey(pubKey);
    pubKey = NULL;

    if (rv != SECSuccess) {
	goto loser;	
    }

    rv = SECSuccess;


loser:

    if(pms) PK11_FreeSymKey(pms);
    if(privKey) SECKEY_DestroyPrivateKey(privKey);
    if(pubKey) SECKEY_DestroyPublicKey(pubKey);
    return rv;
}






static SECStatus
ssl3_SendClientKeyExchange(sslSocket *ss)
{
    SECKEYPublicKey *	serverKey 	= NULL;
    SECStatus 		rv 		= SECFailure;
    PRBool              isTLS;

    SSL_TRC(3, ("%d: SSL3[%d]: send client_key_exchange handshake",
		SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    if (ss->sec.peerKey == NULL) {
	serverKey = CERT_ExtractPublicKey(ss->sec.peerCert);
	if (serverKey == NULL) {
	    ssl_MapLowLevelError(SSL_ERROR_EXTRACT_PUBLIC_KEY_FAILURE);
	    return SECFailure;
	}
    } else {
	serverKey = ss->sec.peerKey;
	ss->sec.peerKey = NULL; 
    }

    isTLS = (PRBool)(ss->ssl3.pwSpec->version > SSL_LIBRARY_VERSION_3_0);
    
    if (ss->ssl3.hs.kea_def->is_limited) {
	int keyLen = SECKEY_PublicKeyStrength(serverKey);	

	if (keyLen * BPB > ss->ssl3.hs.kea_def->key_size_limit) {
	    if (isTLS)
		(void)SSL3_SendAlert(ss, alert_fatal, export_restriction);
	    else
		(void)ssl3_HandshakeFailure(ss);
	    PORT_SetError(SSL_ERROR_PUB_KEY_SIZE_LIMIT_EXCEEDED);
	    goto loser;
	}
    }

    ss->sec.keaType    = ss->ssl3.hs.kea_def->exchKeyType;
    ss->sec.keaKeyBits = SECKEY_PublicKeyStrengthInBits(serverKey);

    switch (ss->ssl3.hs.kea_def->exchKeyType) {
    case kt_rsa:
	rv = sendRSAClientKeyExchange(ss, serverKey);
	break;

    case kt_dh:
	rv = sendDHClientKeyExchange(ss, serverKey);
	break;

#ifndef NSS_DISABLE_ECC
    case kt_ecdh:
	rv = ssl3_SendECDHClientKeyExchange(ss, serverKey);
	break;
#endif 

    default:
	
	SEND_ALERT
	PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
	break;
    }

    SSL_TRC(3, ("%d: SSL3[%d]: DONE sending client_key_exchange",
		SSL_GETPID(), ss->fd));

loser:
    if (serverKey) 
    	SECKEY_DestroyPublicKey(serverKey);
    return rv;	
}


static SECStatus
ssl3_SendCertificateVerify(sslSocket *ss)
{
    SECStatus     rv		= SECFailure;
    PRBool        isTLS;
    PRBool        isTLS12;
    SECItem       buf           = {siBuffer, NULL, 0};
    SSL3Hashes    hashes;
    KeyType       keyType;
    unsigned int  len;
    SSL3SignatureAndHashAlgorithm sigAndHash;

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    SSL_TRC(3, ("%d: SSL3[%d]: send certificate_verify handshake",
		SSL_GETPID(), ss->fd));

    ssl_GetSpecReadLock(ss);
    if (ss->ssl3.hs.hashType == handshake_hash_single &&
	ss->ssl3.hs.backupHash) {
	rv = ssl3_ComputeBackupHandshakeHashes(ss, &hashes);
	PORT_Assert(!ss->ssl3.hs.backupHash);
    } else {
	rv = ssl3_ComputeHandshakeHashes(ss, ss->ssl3.pwSpec, &hashes, 0);
    }
    ssl_ReleaseSpecReadLock(ss);
    if (rv != SECSuccess) {
	goto done;	
    }

    isTLS = (PRBool)(ss->ssl3.pwSpec->version > SSL_LIBRARY_VERSION_3_0);
    isTLS12 = (PRBool)(ss->ssl3.pwSpec->version >= SSL_LIBRARY_VERSION_TLS_1_2);
    keyType = ss->ssl3.clientPrivateKey->keyType;
    rv = ssl3_SignHashes(&hashes, ss->ssl3.clientPrivateKey, &buf, isTLS);
    if (rv == SECSuccess) {
	PK11SlotInfo * slot;
	sslSessionID * sid   = ss->sec.ci.sid;

    	



	slot = PK11_GetSlotFromPrivateKey(ss->ssl3.clientPrivateKey);
	sid->u.ssl3.clAuthSeries     = PK11_GetSlotSeries(slot);
	sid->u.ssl3.clAuthSlotID     = PK11_GetSlotID(slot);
	sid->u.ssl3.clAuthModuleID   = PK11_GetModuleID(slot);
	sid->u.ssl3.clAuthValid      = PR_TRUE;
	PK11_FreeSlot(slot);
    }
    SECKEY_DestroyPrivateKey(ss->ssl3.clientPrivateKey);
    ss->ssl3.clientPrivateKey = NULL;
    if (rv != SECSuccess) {
	goto done;	
    }

    len = buf.len + 2 + (isTLS12 ? 2 : 0);

    rv = ssl3_AppendHandshakeHeader(ss, certificate_verify, len);
    if (rv != SECSuccess) {
	goto done;	
    }
    if (isTLS12) {
	rv = ssl3_TLSSignatureAlgorithmForKeyType(keyType,
						  &sigAndHash.sigAlg);
	if (rv != SECSuccess) {
	    goto done;
	}
	sigAndHash.hashAlg = hashes.hashAlg;

	rv = ssl3_AppendSignatureAndHashAlgorithm(ss, &sigAndHash);
	if (rv != SECSuccess) {
	    goto done; 	
	}
    }
    rv = ssl3_AppendHandshakeVariable(ss, buf.data, buf.len, 2);
    if (rv != SECSuccess) {
	goto done;	
    }

done:
    if (buf.data)
	PORT_Free(buf.data);
    return rv;
}





static SECStatus
ssl3_HandleServerHello(sslSocket *ss, SSL3Opaque *b, PRUint32 length)
{
    sslSessionID *sid		= ss->sec.ci.sid;
    PRInt32       temp;		
    PRBool        suite_found   = PR_FALSE;
    int           i;
    int           errCode	= SSL_ERROR_RX_MALFORMED_SERVER_HELLO;
    SECStatus     rv;
    SECItem       sidBytes 	= {siBuffer, NULL, 0};
    PRBool        sid_match;
    PRBool        isTLS		= PR_FALSE;
    SSL3AlertDescription desc   = illegal_parameter;
    SSL3ProtocolVersion version;

    SSL_TRC(3, ("%d: SSL3[%d]: handle server_hello handshake",
    	SSL_GETPID(), ss->fd));
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( ss->ssl3.initialized );

    if (ss->ssl3.hs.ws != wait_server_hello) {
        errCode = SSL_ERROR_RX_UNEXPECTED_SERVER_HELLO;
	desc    = unexpected_message;
	goto alert_loser;
    }

    
    if (ss->ssl3.clientCertChain != NULL) {
       CERT_DestroyCertificateList(ss->ssl3.clientCertChain);
       ss->ssl3.clientCertChain = NULL;
    }
    if (ss->ssl3.clientCertificate != NULL) {
       CERT_DestroyCertificate(ss->ssl3.clientCertificate);
       ss->ssl3.clientCertificate = NULL;
    }
    if (ss->ssl3.clientPrivateKey != NULL) {
       SECKEY_DestroyPrivateKey(ss->ssl3.clientPrivateKey);
       ss->ssl3.clientPrivateKey = NULL;
    }

    temp = ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
    if (temp < 0) {
    	goto loser; 	
    }
    version = (SSL3ProtocolVersion)temp;

    if (IS_DTLS(ss)) {
	









	version = dtls_DTLSVersionToTLSVersion(version);
	if (version == 0) {  
            goto alert_loser;
	}
    }

    rv = ssl3_NegotiateVersion(ss, version, PR_FALSE);
    if (rv != SECSuccess) {
    	desc = (version > SSL_LIBRARY_VERSION_3_0) ? protocol_version 
						   : handshake_failure;
	errCode = SSL_ERROR_UNSUPPORTED_VERSION;
	goto alert_loser;
    }
    isTLS = (ss->version > SSL_LIBRARY_VERSION_3_0);

    rv = ssl3_InitHandshakeHashes(ss);
    if (rv != SECSuccess) {
	desc = internal_error;
	errCode = PORT_GetError();
	goto alert_loser;
    }

    rv = ssl3_ConsumeHandshake(
	ss, &ss->ssl3.hs.server_random, SSL3_RANDOM_LENGTH, &b, &length);
    if (rv != SECSuccess) {
    	goto loser; 	
    }

    rv = ssl3_ConsumeHandshakeVariable(ss, &sidBytes, 1, &b, &length);
    if (rv != SECSuccess) {
    	goto loser; 	
    }
    if (sidBytes.len > SSL3_SESSIONID_BYTES) {
	if (isTLS)
	    desc = decode_error;
	goto alert_loser;	
    }

    
    temp = ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
    if (temp < 0) {
    	goto loser; 	
    }
    ssl3_config_match_init(ss);
    for (i = 0; i < ssl_V3_SUITES_IMPLEMENTED; i++) {
	ssl3CipherSuiteCfg *suite = &ss->cipherSuites[i];
	if (temp == suite->cipher_suite) {
	    SSLVersionRange vrange = {ss->version, ss->version};
	    if (!config_match(suite, ss->ssl3.policy, PR_TRUE, &vrange)) {
		


		if (!ssl3_CipherSuiteAllowedForVersionRange(temp, &vrange)) {
		    desc    = handshake_failure;
		    errCode = SSL_ERROR_CIPHER_DISALLOWED_FOR_VERSION;
		    goto alert_loser;
		}

		break;	
	    }
	
	    suite_found = PR_TRUE;
	    break;	
	}
    }
    if (!suite_found) {
    	desc    = handshake_failure;
	errCode = SSL_ERROR_NO_CYPHER_OVERLAP;
	goto alert_loser;
    }
    ss->ssl3.hs.cipher_suite = (ssl3CipherSuite)temp;
    ss->ssl3.hs.suite_def    = ssl_LookupCipherSuiteDef((ssl3CipherSuite)temp);
    PORT_Assert(ss->ssl3.hs.suite_def);
    if (!ss->ssl3.hs.suite_def) {
    	PORT_SetError(errCode = SEC_ERROR_LIBRARY_FAILURE);
	goto loser;	
    }

    
    temp = ssl3_ConsumeHandshakeNumber(ss, 1, &b, &length);
    if (temp < 0) {
    	goto loser; 	
    }
    suite_found = PR_FALSE;
    for (i = 0; i < compressionMethodsCount; i++) {
	if (temp == compressions[i]) {
	    if (!compressionEnabled(ss, compressions[i])) {
		break;	
	    }
	    suite_found = PR_TRUE;
	    break;	
    	}
    }
    if (!suite_found) {
    	desc    = handshake_failure;
	errCode = SSL_ERROR_NO_COMPRESSION_OVERLAP;
	goto alert_loser;
    }
    ss->ssl3.hs.compression = (SSLCompressionMethod)temp;

    








    if (length != 0) {
	SECItem extensions;
	rv = ssl3_ConsumeHandshakeVariable(ss, &extensions, 2, &b, &length);
	if (rv != SECSuccess || length != 0) {
	    if (isTLS)
		goto alert_loser;
	} else {
	    rv = ssl3_HandleHelloExtensions(ss, &extensions.data,
					    &extensions.len);
	    if (rv != SECSuccess)
		goto alert_loser;
	}
    }
    if ((ss->opt.requireSafeNegotiation || 
         (ss->firstHsDone && (ss->peerRequestedProtection ||
	 ss->opt.enableRenegotiation == SSL_RENEGOTIATE_REQUIRES_XTN))) &&
	!ssl3_ExtensionNegotiated(ss, ssl_renegotiation_info_xtn)) {
	desc = handshake_failure;
	errCode = ss->firstHsDone ? SSL_ERROR_RENEGOTIATION_NOT_ALLOWED
	                          : SSL_ERROR_UNSAFE_NEGOTIATION;
	goto alert_loser;
    }

    
    desc    = handshake_failure;

    

    rv = ssl3_SetupPendingCipherSpec(ss);
    if (rv != SECSuccess) {
	goto alert_loser;	
    }

    




    sid_match = (PRBool)(sidBytes.len > 0 &&
	sidBytes.len == sid->u.ssl3.sessionIDLength &&
	!PORT_Memcmp(sid->u.ssl3.sessionID, sidBytes.data, sidBytes.len));

    if (sid_match &&
	sid->version == ss->version &&
	sid->u.ssl3.cipherSuite == ss->ssl3.hs.cipher_suite) do {
	ssl3CipherSpec *pwSpec = ss->ssl3.pwSpec;

	SECItem       wrappedMS;   

	ss->sec.authAlgorithm = sid->authAlgorithm;
	ss->sec.authKeyBits   = sid->authKeyBits;
	ss->sec.keaType       = sid->keaType;
	ss->sec.keaKeyBits    = sid->keaKeyBits;

	




	if (sid->u.ssl3.keys.msIsWrapped) {
	    PK11SlotInfo *slot;
	    PK11SymKey *  wrapKey;     
	    CK_FLAGS      keyFlags      = 0;

#ifndef NO_PKCS11_BYPASS
	    if (ss->opt.bypassPKCS11) {
		


		break;  
	    }
#endif
	    
	    slot = SECMOD_LookupSlot(sid->u.ssl3.masterModuleID,
				     sid->u.ssl3.masterSlotID);
	    if (slot == NULL) {
		break;		
	    }
	    if (!PK11_IsPresent(slot)) {
		PK11_FreeSlot(slot);
		break;		
	    }
	    wrapKey = PK11_GetWrapKey(slot, sid->u.ssl3.masterWrapIndex,
				      sid->u.ssl3.masterWrapMech,
				      sid->u.ssl3.masterWrapSeries,
				      ss->pkcs11PinArg);
	    PK11_FreeSlot(slot);
	    if (wrapKey == NULL) {
		break;		
	    }

	    if (ss->version > SSL_LIBRARY_VERSION_3_0) {	
		keyFlags = CKF_SIGN | CKF_VERIFY;
	    }

	    wrappedMS.data = sid->u.ssl3.keys.wrapped_master_secret;
	    wrappedMS.len  = sid->u.ssl3.keys.wrapped_master_secret_len;
	    pwSpec->master_secret =
		PK11_UnwrapSymKeyWithFlags(wrapKey, sid->u.ssl3.masterWrapMech, 
			    NULL, &wrappedMS, CKM_SSL3_MASTER_KEY_DERIVE,
			    CKA_DERIVE, sizeof(SSL3MasterSecret), keyFlags);
	    errCode = PORT_GetError();
	    PK11_FreeSymKey(wrapKey);
	    if (pwSpec->master_secret == NULL) {
		break;	
	    }
#ifndef NO_PKCS11_BYPASS
	} else if (ss->opt.bypassPKCS11) {
	    
	    wrappedMS.data = sid->u.ssl3.keys.wrapped_master_secret;
	    wrappedMS.len  = sid->u.ssl3.keys.wrapped_master_secret_len;
	    memcpy(pwSpec->raw_master_secret, wrappedMS.data, wrappedMS.len);
	    pwSpec->msItem.data = pwSpec->raw_master_secret;
	    pwSpec->msItem.len  = wrappedMS.len;
#endif
	} else {
	    
	    
	    PK11SlotInfo *slot = PK11_GetInternalSlot();
	    wrappedMS.data = sid->u.ssl3.keys.wrapped_master_secret;
	    wrappedMS.len  = sid->u.ssl3.keys.wrapped_master_secret_len;
	    pwSpec->master_secret =  
		PK11_ImportSymKey(slot, CKM_SSL3_MASTER_KEY_DERIVE, 
				  PK11_OriginUnwrap, CKA_ENCRYPT, 
				  &wrappedMS, NULL);
	    PK11_FreeSlot(slot);
	    if (pwSpec->master_secret == NULL) {
		break; 
	    }
	}

	
	SSL_AtomicIncrementLong(& ssl3stats.hsh_sid_cache_hits );

	
	if (ss->xtnData.sentSessionTicketInClientHello)
	    SSL_AtomicIncrementLong(& ssl3stats.hsh_sid_stateless_resumes );

	if (ssl3_ExtensionNegotiated(ss, ssl_session_ticket_xtn))
	    ss->ssl3.hs.ws = wait_new_session_ticket;
	else
	    ss->ssl3.hs.ws = wait_change_cipher;

	ss->ssl3.hs.isResuming = PR_TRUE;

	
	if (sid->peerCert != NULL) {
	    ss->sec.peerCert = CERT_DupCertificate(sid->peerCert);
	}

	
	rv = ssl3_InitPendingCipherSpec(ss,  NULL);
	if (rv != SECSuccess) {
	    goto alert_loser;	
	}
	return SECSuccess;
    } while (0);

    if (sid_match)
	SSL_AtomicIncrementLong(& ssl3stats.hsh_sid_cache_not_ok );
    else
	SSL_AtomicIncrementLong(& ssl3stats.hsh_sid_cache_misses );

    
    sid->u.ssl3.keys.resumable = PR_FALSE;
    if (ss->sec.uncache)
        (*ss->sec.uncache)(sid);
    ssl_FreeSID(sid);

    
    ss->sec.ci.sid = sid = ssl3_NewSessionID(ss, PR_FALSE);
    if (sid == NULL) {
	goto alert_loser;	
    }

    sid->version = ss->version;
    sid->u.ssl3.sessionIDLength = sidBytes.len;
    PORT_Memcpy(sid->u.ssl3.sessionID, sidBytes.data, sidBytes.len);

    ss->ssl3.hs.isResuming = PR_FALSE;
    if (ss->ssl3.hs.kea_def->signKeyType != sign_null) {
        

        ss->ssl3.hs.ws = wait_server_cert;
    } else if (ss->ssl3.hs.kea_def->ephemeral) {
        
        ss->ssl3.hs.ws = wait_server_key;
    } else {
        ss->ssl3.hs.ws = wait_cert_request;
    }
    return SECSuccess;

alert_loser:
    (void)SSL3_SendAlert(ss, alert_fatal, desc);

loser:
    errCode = ssl_MapLowLevelError(errCode);
    return SECFailure;
}



static PRBool
ssl3_BigIntGreaterThanOne(const SECItem* mpint) {
    unsigned char firstNonZeroByte = 0;
    unsigned int i;

    for (i = 0; i < mpint->len; i++) {
	if (mpint->data[i]) {
	    firstNonZeroByte = mpint->data[i];
	    break;
	}
    }

    if (firstNonZeroByte == 0)
	return PR_FALSE;
    if (firstNonZeroByte > 1)
	return PR_TRUE;

    

    return (i < mpint->len - 1);
}





static SECStatus
ssl3_HandleServerKeyExchange(sslSocket *ss, SSL3Opaque *b, PRUint32 length)
{
    PLArenaPool *    arena     = NULL;
    SECKEYPublicKey *peerKey   = NULL;
    PRBool           isTLS, isTLS12;
    SECStatus        rv;
    int              errCode   = SSL_ERROR_RX_MALFORMED_SERVER_KEY_EXCH;
    SSL3AlertDescription desc  = illegal_parameter;
    SSL3Hashes       hashes;
    SECItem          signature = {siBuffer, NULL, 0};
    SSL3SignatureAndHashAlgorithm sigAndHash;

    sigAndHash.hashAlg = SEC_OID_UNKNOWN;

    SSL_TRC(3, ("%d: SSL3[%d]: handle server_key_exchange handshake",
		SSL_GETPID(), ss->fd));
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    if (ss->ssl3.hs.ws != wait_server_key) {
        errCode = SSL_ERROR_RX_UNEXPECTED_SERVER_KEY_EXCH;
        desc = unexpected_message;
        goto alert_loser;
    }

    isTLS = (PRBool)(ss->ssl3.prSpec->version > SSL_LIBRARY_VERSION_3_0);
    isTLS12 = (PRBool)(ss->ssl3.prSpec->version >= SSL_LIBRARY_VERSION_TLS_1_2);

    switch (ss->ssl3.hs.kea_def->exchKeyType) {

    case kt_rsa: {
	SECItem          modulus   = {siBuffer, NULL, 0};
	SECItem          exponent  = {siBuffer, NULL, 0};

    	rv = ssl3_ConsumeHandshakeVariable(ss, &modulus, 2, &b, &length);
    	if (rv != SECSuccess) {
	    goto loser;		
	}
    	rv = ssl3_ConsumeHandshakeVariable(ss, &exponent, 2, &b, &length);
    	if (rv != SECSuccess) {
	    goto loser;		
	}
	if (isTLS12) {
	    rv = ssl3_ConsumeSignatureAndHashAlgorithm(ss, &b, &length,
						       &sigAndHash);
	    if (rv != SECSuccess) {
		goto loser;	
	    }
	    rv = ssl3_CheckSignatureAndHashAlgorithmConsistency(
		    &sigAndHash, ss->sec.peerCert);
	    if (rv != SECSuccess) {
		goto loser;
	    }
	}
    	rv = ssl3_ConsumeHandshakeVariable(ss, &signature, 2, &b, &length);
    	if (rv != SECSuccess) {
	    goto loser;		
	}
    	if (length != 0) {
	    if (isTLS)
		desc = decode_error;
	    goto alert_loser;		
	}

	
	
    	desc = isTLS ? decrypt_error : handshake_failure;

    	


	rv = ssl3_ComputeExportRSAKeyHash(sigAndHash.hashAlg, modulus, exponent,
					  &ss->ssl3.hs.client_random,
					  &ss->ssl3.hs.server_random, 
					  &hashes, ss->opt.bypassPKCS11);
        if (rv != SECSuccess) {
	    errCode =
	    	ssl_MapLowLevelError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	    goto alert_loser;
	}
        rv = ssl3_VerifySignedHashes(&hashes, ss->sec.peerCert, &signature,
				    isTLS, ss->pkcs11PinArg);
	if (rv != SECSuccess)  {
	    errCode =
	    	ssl_MapLowLevelError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	    goto alert_loser;
	}

	




    	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if (arena == NULL) {
	    goto no_memory;
	}

    	peerKey = PORT_ArenaZNew(arena, SECKEYPublicKey);
    	if (peerKey == NULL) {
            PORT_FreeArena(arena, PR_FALSE);
	    goto no_memory;
	}

	peerKey->arena              = arena;
	peerKey->keyType            = rsaKey;
	peerKey->pkcs11Slot         = NULL;
	peerKey->pkcs11ID           = CK_INVALID_HANDLE;
	if (SECITEM_CopyItem(arena, &peerKey->u.rsa.modulus,        &modulus) ||
	    SECITEM_CopyItem(arena, &peerKey->u.rsa.publicExponent, &exponent))
	{
            PORT_FreeArena(arena, PR_FALSE);
	    goto no_memory;
        }
    	ss->sec.peerKey = peerKey;
    	ss->ssl3.hs.ws = wait_cert_request;
    	return SECSuccess;
    }

    case kt_dh: {
	SECItem          dh_p      = {siBuffer, NULL, 0};
	SECItem          dh_g      = {siBuffer, NULL, 0};
	SECItem          dh_Ys     = {siBuffer, NULL, 0};

    	rv = ssl3_ConsumeHandshakeVariable(ss, &dh_p, 2, &b, &length);
    	if (rv != SECSuccess) {
	    goto loser;		
	}
	if (dh_p.len < 512/8) {
	    errCode = SSL_ERROR_WEAK_SERVER_EPHEMERAL_DH_KEY;
	    goto alert_loser;
	}
    	rv = ssl3_ConsumeHandshakeVariable(ss, &dh_g, 2, &b, &length);
    	if (rv != SECSuccess) {
	    goto loser;		
	}
	if (dh_g.len > dh_p.len || !ssl3_BigIntGreaterThanOne(&dh_g))
	    goto alert_loser;
    	rv = ssl3_ConsumeHandshakeVariable(ss, &dh_Ys, 2, &b, &length);
    	if (rv != SECSuccess) {
	    goto loser;		
	}
	if (dh_Ys.len > dh_p.len || !ssl3_BigIntGreaterThanOne(&dh_Ys))
	    goto alert_loser;
	if (isTLS12) {
	    rv = ssl3_ConsumeSignatureAndHashAlgorithm(ss, &b, &length,
						       &sigAndHash);
	    if (rv != SECSuccess) {
		goto loser;	
	    }
	    rv = ssl3_CheckSignatureAndHashAlgorithmConsistency(
		    &sigAndHash, ss->sec.peerCert);
	    if (rv != SECSuccess) {
		goto loser;
	    }
	}
    	rv = ssl3_ConsumeHandshakeVariable(ss, &signature, 2, &b, &length);
    	if (rv != SECSuccess) {
	    goto loser;		
	}
    	if (length != 0) {
	    if (isTLS)
		desc = decode_error;
	    goto alert_loser;		
	}

	PRINT_BUF(60, (NULL, "Server DH p", dh_p.data, dh_p.len));
	PRINT_BUF(60, (NULL, "Server DH g", dh_g.data, dh_g.len));
	PRINT_BUF(60, (NULL, "Server DH Ys", dh_Ys.data, dh_Ys.len));

	
	
    	desc = isTLS ? decrypt_error : handshake_failure;

    	


	rv = ssl3_ComputeDHKeyHash(sigAndHash.hashAlg, dh_p, dh_g, dh_Ys,
					  &ss->ssl3.hs.client_random,
					  &ss->ssl3.hs.server_random, 
					  &hashes, ss->opt.bypassPKCS11);
        if (rv != SECSuccess) {
	    errCode =
	    	ssl_MapLowLevelError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	    goto alert_loser;
	}
        rv = ssl3_VerifySignedHashes(&hashes, ss->sec.peerCert, &signature,
				    isTLS, ss->pkcs11PinArg);
	if (rv != SECSuccess)  {
	    errCode =
	    	ssl_MapLowLevelError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	    goto alert_loser;
	}

	




    	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if (arena == NULL) {
	    goto no_memory;
	}

    	ss->sec.peerKey = peerKey = PORT_ArenaZNew(arena, SECKEYPublicKey);
    	if (peerKey == NULL) {
	    goto no_memory;
	}

	peerKey->arena              = arena;
	peerKey->keyType            = dhKey;
	peerKey->pkcs11Slot         = NULL;
	peerKey->pkcs11ID           = CK_INVALID_HANDLE;

	if (SECITEM_CopyItem(arena, &peerKey->u.dh.prime,        &dh_p) ||
	    SECITEM_CopyItem(arena, &peerKey->u.dh.base,         &dh_g) ||
	    SECITEM_CopyItem(arena, &peerKey->u.dh.publicValue,  &dh_Ys))
	{
            PORT_FreeArena(arena, PR_FALSE);
	    goto no_memory;
        }
    	ss->sec.peerKey = peerKey;
    	ss->ssl3.hs.ws = wait_cert_request;
    	return SECSuccess;
    }

#ifndef NSS_DISABLE_ECC
    case kt_ecdh:
	rv = ssl3_HandleECDHServerKeyExchange(ss, b, length);
	return rv;
#endif 

    default:
    	desc    = handshake_failure;
	errCode = SEC_ERROR_UNSUPPORTED_KEYALG;
	break;		
    }

alert_loser:
    (void)SSL3_SendAlert(ss, alert_fatal, desc);
loser:
    PORT_SetError( errCode );
    return SECFailure;

no_memory:	
    ssl_MapLowLevelError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
    return SECFailure;
}





static SECStatus
ssl3_ExtractClientKeyInfo(sslSocket *ss,
			  TLSSignatureAlgorithm *sigAlg,
			  PRBool *preferSha1)
{
    SECStatus rv = SECSuccess;
    SECKEYPublicKey *pubk;

    pubk = CERT_ExtractPublicKey(ss->ssl3.clientCertificate);
    if (pubk == NULL) {
	rv = SECFailure;
	goto done;
    }

    rv = ssl3_TLSSignatureAlgorithmForKeyType(pubk->keyType, sigAlg);
    if (rv != SECSuccess) {
	goto done;
    }

    





    if (pubk->keyType == rsaKey || pubk->keyType == dsaKey) {
	*preferSha1 = SECKEY_PublicKeyStrength(pubk) <= 128;
    } else {
	*preferSha1 = PR_FALSE;
    }

done:
    if (pubk)
	SECKEY_DestroyPublicKey(pubk);
    return rv;
}





static void
ssl3_DestroyBackupHandshakeHashIfNotNeeded(sslSocket *ss,
					   const SECItem *algorithms)
{
    SECStatus rv;
    TLSSignatureAlgorithm sigAlg;
    PRBool preferSha1;
    PRBool supportsSha1 = PR_FALSE;
    PRBool supportsSha256 = PR_FALSE;
    PRBool needBackupHash = PR_FALSE;
    unsigned int i;

#ifndef NO_PKCS11_BYPASS
    
    if (ss->opt.bypassPKCS11) {
	PORT_Assert(!ss->ssl3.hs.backupHash);
	return;
    }
#endif
    PORT_Assert(ss->ssl3.hs.backupHash);

    
    rv = ssl3_ExtractClientKeyInfo(ss, &sigAlg, &preferSha1);
    if (rv != SECSuccess) {
	goto done;
    }

    
    for (i = 0; i < algorithms->len; i += 2) {
	if (algorithms->data[i+1] == sigAlg) {
	    if (algorithms->data[i] == tls_hash_sha1) {
		supportsSha1 = PR_TRUE;
	    } else if (algorithms->data[i] == tls_hash_sha256) {
		supportsSha256 = PR_TRUE;
	    }
	}
    }

    

    if (supportsSha1 && (preferSha1 || !supportsSha256)) {
	needBackupHash = PR_TRUE;
    }

done:
    if (!needBackupHash) {
	PK11_DestroyContext(ss->ssl3.hs.backupHash, PR_TRUE);
	ss->ssl3.hs.backupHash = NULL;
    }
}

typedef struct dnameNode {
    struct dnameNode *next;
    SECItem           name;
} dnameNode;





static SECStatus
ssl3_HandleCertificateRequest(sslSocket *ss, SSL3Opaque *b, PRUint32 length)
{
    PLArenaPool *        arena       = NULL;
    dnameNode *          node;
    PRInt32              remaining;
    PRBool               isTLS       = PR_FALSE;
    PRBool               isTLS12     = PR_FALSE;
    int                  i;
    int                  errCode     = SSL_ERROR_RX_MALFORMED_CERT_REQUEST;
    int                  nnames      = 0;
    SECStatus            rv;
    SSL3AlertDescription desc        = illegal_parameter;
    SECItem              cert_types  = {siBuffer, NULL, 0};
    SECItem              algorithms  = {siBuffer, NULL, 0};
    CERTDistNames        ca_list;

    SSL_TRC(3, ("%d: SSL3[%d]: handle certificate_request handshake",
		SSL_GETPID(), ss->fd));
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    if (ss->ssl3.hs.ws != wait_cert_request) {
        desc = unexpected_message;
        errCode = SSL_ERROR_RX_UNEXPECTED_CERT_REQUEST;
        goto alert_loser;
    }

    PORT_Assert(ss->ssl3.clientCertChain == NULL);
    PORT_Assert(ss->ssl3.clientCertificate == NULL);
    PORT_Assert(ss->ssl3.clientPrivateKey == NULL);

    isTLS = (PRBool)(ss->ssl3.prSpec->version > SSL_LIBRARY_VERSION_3_0);
    isTLS12 = (PRBool)(ss->ssl3.prSpec->version >= SSL_LIBRARY_VERSION_TLS_1_2);
    rv = ssl3_ConsumeHandshakeVariable(ss, &cert_types, 1, &b, &length);
    if (rv != SECSuccess)
    	goto loser;		

    if (isTLS12) {
	rv = ssl3_ConsumeHandshakeVariable(ss, &algorithms, 2, &b, &length);
	if (rv != SECSuccess)
	    goto loser;		
	



	if (algorithms.len == 0 || (algorithms.len & 1) != 0)
	    goto alert_loser;
    }

    arena = ca_list.arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
    	goto no_mem;

    remaining = ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
    if (remaining < 0)
    	goto loser;	 	

    if ((PRUint32)remaining > length)
	goto alert_loser;

    ca_list.head = node = PORT_ArenaZNew(arena, dnameNode);
    if (node == NULL)
    	goto no_mem;

    while (remaining > 0) {
	PRInt32 len;

	if (remaining < 2)
	    goto alert_loser;	

	node->name.len = len = ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
	if (len <= 0)
	    goto loser;		

	remaining -= 2;
	if (remaining < len)
	    goto alert_loser;	

	node->name.data = b;
	b         += len;
	length    -= len;
	remaining -= len;
	nnames++;
	if (remaining <= 0)
	    break;		

	node->next = PORT_ArenaZNew(arena, dnameNode);
	node = node->next;
	if (node == NULL)
	    goto no_mem;
    }

    ca_list.nnames = nnames;
    ca_list.names  = PORT_ArenaNewArray(arena, SECItem, nnames);
    if (nnames > 0 && ca_list.names == NULL)
        goto no_mem;

    for(i = 0, node = (dnameNode*)ca_list.head;
	i < nnames;
	i++, node = node->next) {
	ca_list.names[i] = node->name;
    }

    if (length != 0)
        goto alert_loser;   	

    desc = no_certificate;
    ss->ssl3.hs.ws = wait_hello_done;

    if (ss->getClientAuthData != NULL) {
	
	rv = (SECStatus)(*ss->getClientAuthData)(ss->getClientAuthDataArg,
						 ss->fd, &ca_list,
						 &ss->ssl3.clientCertificate,
						 &ss->ssl3.clientPrivateKey);
    } else {
	rv = SECFailure; 
    }
    switch (rv) {
    case SECWouldBlock:	
	ssl3_SetAlwaysBlock(ss);
	break;	

    case SECSuccess:
        
        if ((!ss->ssl3.clientCertificate) || (!ss->ssl3.clientPrivateKey)) {
            
            if (ss->ssl3.clientCertificate) {
                
                CERT_DestroyCertificate(ss->ssl3.clientCertificate);
                ss->ssl3.clientCertificate = NULL;
            }
            if (ss->ssl3.clientPrivateKey) {
                
                SECKEY_DestroyPrivateKey(ss->ssl3.clientPrivateKey);
                ss->ssl3.clientPrivateKey = NULL;
            }
            goto send_no_certificate;
        }
	


	ss->ssl3.clientCertChain = CERT_CertChainFromCert(
					ss->ssl3.clientCertificate,
					certUsageSSLClient, PR_FALSE);
	if (ss->ssl3.clientCertChain == NULL) {
	    CERT_DestroyCertificate(ss->ssl3.clientCertificate);
	    ss->ssl3.clientCertificate = NULL;
	    SECKEY_DestroyPrivateKey(ss->ssl3.clientPrivateKey);
	    ss->ssl3.clientPrivateKey = NULL;
	    goto send_no_certificate;
	}
	if (ss->ssl3.hs.hashType == handshake_hash_single) {
	    ssl3_DestroyBackupHandshakeHashIfNotNeeded(ss, &algorithms);
	}
	break;	

    case SECFailure:
    default:
send_no_certificate:
	if (isTLS) {
	    ss->ssl3.sendEmptyCert = PR_TRUE;
	} else {
	    (void)SSL3_SendAlert(ss, alert_warning, no_certificate);
	}
	rv = SECSuccess;
	break;
    }
    goto done;

no_mem:
    rv = SECFailure;
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    goto done;

alert_loser:
    if (isTLS && desc == illegal_parameter)
    	desc = decode_error;
    (void)SSL3_SendAlert(ss, alert_fatal, desc);
loser:
    PORT_SetError(errCode);
    rv = SECFailure;
done:
    if (arena != NULL)
    	PORT_FreeArena(arena, PR_FALSE);
    return rv;
}

static SECStatus
ssl3_CheckFalseStart(sslSocket *ss)
{
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( !ss->ssl3.hs.authCertificatePending );
    PORT_Assert( !ss->ssl3.hs.canFalseStart );

    if (!ss->canFalseStartCallback) {
	SSL_TRC(3, ("%d: SSL[%d]: no false start callback so no false start",
		    SSL_GETPID(), ss->fd));
    } else {
	PRBool maybeFalseStart;
	SECStatus rv;

	



        ssl_GetSpecReadLock(ss);
        maybeFalseStart = ss->ssl3.cwSpec->cipher_def->secret_key_size >= 10;
        ssl_ReleaseSpecReadLock(ss);

	if (!maybeFalseStart) {
	    SSL_TRC(3, ("%d: SSL[%d]: no false start due to weak cipher",
			SSL_GETPID(), ss->fd));
	} else {
	    rv = (ss->canFalseStartCallback)(ss->fd,
					     ss->canFalseStartCallbackData,
					     &ss->ssl3.hs.canFalseStart);
	    if (rv == SECSuccess) {
		SSL_TRC(3, ("%d: SSL[%d]: false start callback returned %s",
			    SSL_GETPID(), ss->fd,
			    ss->ssl3.hs.canFalseStart ? "TRUE" : "FALSE"));
	    } else {
		SSL_TRC(3, ("%d: SSL[%d]: false start callback failed (%s)",
			    SSL_GETPID(), ss->fd,
			    PR_ErrorToName(PR_GetError())));
	    }
	    return rv;
	}
    }

    ss->ssl3.hs.canFalseStart = PR_FALSE;
    return SECSuccess;
}

PRBool
ssl3_WaitingForStartOfServerSecondRound(sslSocket *ss)
{
    PRBool result;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    switch (ss->ssl3.hs.ws) {
    case wait_new_session_ticket:
        result = PR_TRUE;
        break;
    case wait_change_cipher:
        result = !ssl3_ExtensionNegotiated(ss, ssl_session_ticket_xtn);
        break;
    default:
        result = PR_FALSE;
        break;
    }

    return result;
}

static SECStatus ssl3_SendClientSecondRound(sslSocket *ss);





static SECStatus
ssl3_HandleServerHelloDone(sslSocket *ss)
{
    SECStatus     rv;
    SSL3WaitState ws          = ss->ssl3.hs.ws;

    SSL_TRC(3, ("%d: SSL3[%d]: handle server_hello_done handshake",
		SSL_GETPID(), ss->fd));
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    
    if (ws != wait_hello_done  &&
	ws != wait_cert_request) {
	SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	PORT_SetError(SSL_ERROR_RX_UNEXPECTED_HELLO_DONE);
	return SECFailure;
    }

    rv = ssl3_SendClientSecondRound(ss);

    return rv;
}





static SECStatus
ssl3_SendClientSecondRound(sslSocket *ss)
{
    SECStatus rv;
    PRBool sendClientCert;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    sendClientCert = !ss->ssl3.sendEmptyCert &&
		     ss->ssl3.clientCertChain  != NULL &&
		     ss->ssl3.clientPrivateKey != NULL;

    if (!sendClientCert &&
	ss->ssl3.hs.hashType == handshake_hash_single &&
	ss->ssl3.hs.backupHash) {
	
	PK11_DestroyContext(ss->ssl3.hs.backupHash, PR_TRUE);
	ss->ssl3.hs.backupHash = NULL;
    }

    























    if (ss->ssl3.hs.restartTarget) {
	PR_NOT_REACHED("unexpected ss->ssl3.hs.restartTarget");
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    if (ss->ssl3.hs.authCertificatePending &&
	(sendClientCert || ss->ssl3.sendEmptyCert || ss->firstHsDone)) {
	SSL_TRC(3, ("%d: SSL3[%p]: deferring ssl3_SendClientSecondRound because"
		    " certificate authentication is still pending.",
		    SSL_GETPID(), ss->fd));
	ss->ssl3.hs.restartTarget = ssl3_SendClientSecondRound;
	return SECWouldBlock;
    }

    ssl_GetXmitBufLock(ss);		

    if (ss->ssl3.sendEmptyCert) {
	ss->ssl3.sendEmptyCert = PR_FALSE;
	rv = ssl3_SendEmptyCertificate(ss);
	
	if (rv != SECSuccess) {
	    goto loser;	
    	}
    } else if (sendClientCert) {
	rv = ssl3_SendCertificate(ss);
	if (rv != SECSuccess) {
	    goto loser;	
    	}
    }

    rv = ssl3_SendClientKeyExchange(ss);
    if (rv != SECSuccess) {
    	goto loser;	
    }

    if (sendClientCert) {
	rv = ssl3_SendCertificateVerify(ss);
	if (rv != SECSuccess) {
	    goto loser;	
        }
    }

    rv = ssl3_SendChangeCipherSpecs(ss);
    if (rv != SECSuccess) {
	goto loser;	
    }

    





    ss->enoughFirstHsDone = PR_TRUE;

    if (!ss->firstHsDone) {
	


	rv = ssl3_SendNextProto(ss);
	if (rv != SECSuccess) {
	    goto loser;	
	}

	if (ss->opt.enableFalseStart) {
	    if (!ss->ssl3.hs.authCertificatePending) {
		







		ssl_ReleaseXmitBufLock(ss);
		rv = ssl3_CheckFalseStart(ss);
		ssl_GetXmitBufLock(ss);
		if (rv != SECSuccess) {
		    goto loser;
		}
	    } else {
		




		SSL_TRC(3, ("%d: SSL3[%p]: deferring false start check because"
			    " certificate authentication is still pending.",
			    SSL_GETPID(), ss->fd));
	    }
	}
    }

    rv = ssl3_SendFinished(ss, 0);
    if (rv != SECSuccess) {
	goto loser;	
    }

    ssl_ReleaseXmitBufLock(ss);		

    if (ssl3_ExtensionNegotiated(ss, ssl_session_ticket_xtn))
	ss->ssl3.hs.ws = wait_new_session_ticket;
    else
	ss->ssl3.hs.ws = wait_change_cipher;

    PORT_Assert(ssl3_WaitingForStartOfServerSecondRound(ss));

    return SECSuccess;

loser:
    ssl_ReleaseXmitBufLock(ss);
    return rv;
}




static SECStatus
ssl3_SendHelloRequest(sslSocket *ss)
{
    SECStatus rv;

    SSL_TRC(3, ("%d: SSL3[%d]: send hello_request handshake", SSL_GETPID(),
		ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss) );

    rv = ssl3_AppendHandshakeHeader(ss, hello_request, 0);
    if (rv != SECSuccess) {
	return rv;	
    }
    rv = ssl3_FlushHandshake(ss, 0);
    if (rv != SECSuccess) {
	return rv;	
    }
    ss->ssl3.hs.ws = wait_client_hello;
    return SECSuccess;
}





static SECComparison
ssl3_ServerNameCompare(const SECItem *name1, const SECItem *name2)
{
    if (!name1 != !name2) {
        return SECLessThan;
    }
    if (!name1) {
        return SECEqual;
    }
    if (name1->type != name2->type) {
        return SECLessThan;
    }
    return SECITEM_CompareItem(name1, name2);
}








sslSessionID *
ssl3_NewSessionID(sslSocket *ss, PRBool is_server)
{
    sslSessionID *sid;

    sid = PORT_ZNew(sslSessionID);
    if (sid == NULL)
    	return sid;

    if (is_server) {
        const SECItem *  srvName;
        SECStatus        rv = SECSuccess;

        ssl_GetSpecReadLock(ss);	
        srvName = &ss->ssl3.prSpec->srvVirtName;
        if (srvName->len && srvName->data) {
            rv = SECITEM_CopyItem(NULL, &sid->u.ssl3.srvName, srvName);
        }
        ssl_ReleaseSpecReadLock(ss); 
        if (rv != SECSuccess) {
            PORT_Free(sid);
            return NULL;
        }
    }
    sid->peerID		= (ss->peerID == NULL) ? NULL : PORT_Strdup(ss->peerID);
    sid->urlSvrName	= (ss->url    == NULL) ? NULL : PORT_Strdup(ss->url);
    sid->addr           = ss->sec.ci.peer;
    sid->port           = ss->sec.ci.port;
    sid->references     = 1;
    sid->cached         = never_cached;
    sid->version        = ss->version;

    sid->u.ssl3.keys.resumable = PR_TRUE;
    sid->u.ssl3.policy         = SSL_ALLOWED;
    sid->u.ssl3.clientWriteKey = NULL;
    sid->u.ssl3.serverWriteKey = NULL;

    if (is_server) {
	SECStatus rv;
	int       pid = SSL_GETPID();

	sid->u.ssl3.sessionIDLength = SSL3_SESSIONID_BYTES;
	sid->u.ssl3.sessionID[0]    = (pid >> 8) & 0xff;
	sid->u.ssl3.sessionID[1]    =  pid       & 0xff;
	rv = PK11_GenerateRandom(sid->u.ssl3.sessionID + 2,
	                         SSL3_SESSIONID_BYTES -2);
	if (rv != SECSuccess) {
	    ssl_FreeSID(sid);
	    ssl_MapLowLevelError(SSL_ERROR_GENERATE_RANDOM_FAILURE);
	    return NULL;
    	}
    }
    return sid;
}


static SECStatus
ssl3_SendServerHelloSequence(sslSocket *ss)
{
    const ssl3KEADef *kea_def;
    SECStatus         rv;

    SSL_TRC(3, ("%d: SSL3[%d]: begin send server_hello sequence",
		SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss) );

    rv = ssl3_SendServerHello(ss);
    if (rv != SECSuccess) {
	return rv;	
    }
    rv = ssl3_SendCertificate(ss);
    if (rv != SECSuccess) {
	return rv;	
    }
    rv = ssl3_SendCertificateStatus(ss);
    if (rv != SECSuccess) {
	return rv;	
    }
    


    kea_def = ss->ssl3.hs.kea_def;
    ss->ssl3.hs.usedStepDownKey = PR_FALSE;

    if (kea_def->is_limited && kea_def->exchKeyType == kt_rsa) {
	
	int keyLen;  

	keyLen = PK11_GetPrivateModulusLen(
			    ss->serverCerts[kea_def->exchKeyType].SERVERKEY);

	if (keyLen > 0 &&
	    keyLen * BPB <= kea_def->key_size_limit ) {
	    
	    
	} else if (ss->stepDownKeyPair != NULL) {
	    ss->ssl3.hs.usedStepDownKey = PR_TRUE;
	    rv = ssl3_SendServerKeyExchange(ss);
	    if (rv != SECSuccess) {
		return rv;	
	    }
	} else {
#ifndef HACKED_EXPORT_SERVER
	    PORT_SetError(SSL_ERROR_PUB_KEY_SIZE_LIMIT_EXCEEDED);
	    return rv;
#endif
	}
    } else if (kea_def->ephemeral) {
        rv = ssl3_SendServerKeyExchange(ss);
        if (rv != SECSuccess) {
            return rv;	
        }
    }

    if (ss->opt.requestCertificate) {
	rv = ssl3_SendCertificateRequest(ss);
	if (rv != SECSuccess) {
	    return rv;		
	}
    }
    rv = ssl3_SendServerHelloDone(ss);
    if (rv != SECSuccess) {
	return rv;		
    }

    ss->ssl3.hs.ws = (ss->opt.requestCertificate) ? wait_client_cert
                                               : wait_client_key;
    return SECSuccess;
}


static const PRUint8 emptyRIext[5] = {0xff, 0x01, 0x00, 0x01, 0x00};





static SECStatus
ssl3_HandleClientHello(sslSocket *ss, SSL3Opaque *b, PRUint32 length)
{
    sslSessionID *      sid      = NULL;
    PRInt32		tmp;
    unsigned int        i;
    int                 j;
    SECStatus           rv;
    int                 errCode  = SSL_ERROR_RX_MALFORMED_CLIENT_HELLO;
    SSL3AlertDescription desc    = illegal_parameter;
    SSL3AlertLevel      level    = alert_fatal;
    SSL3ProtocolVersion version;
    SECItem             sidBytes = {siBuffer, NULL, 0};
    SECItem             cookieBytes = {siBuffer, NULL, 0};
    SECItem             suites   = {siBuffer, NULL, 0};
    SECItem             comps    = {siBuffer, NULL, 0};
    PRBool              haveSpecWriteLock = PR_FALSE;
    PRBool              haveXmitBufLock   = PR_FALSE;

    SSL_TRC(3, ("%d: SSL3[%d]: handle client_hello handshake",
    	SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));
    PORT_Assert( ss->ssl3.initialized );

    if (!ss->sec.isServer ||
        (ss->ssl3.hs.ws != wait_client_hello &&
         ss->ssl3.hs.ws != idle_handshake)) {
        desc = unexpected_message;
        errCode = SSL_ERROR_RX_UNEXPECTED_CLIENT_HELLO;
        goto alert_loser;
    }
    if (ss->ssl3.hs.ws == idle_handshake &&
        ss->opt.enableRenegotiation == SSL_RENEGOTIATE_NEVER) {
        desc = no_renegotiation;
        level = alert_warning;
        errCode = SSL_ERROR_RENEGOTIATION_NOT_ALLOWED;
        goto alert_loser;
    }

    
    rv = ssl_GetPeerInfo(ss);
    if (rv != SECSuccess) {
	return rv;		
    }

    








    if (IS_DTLS(ss)) {
	ss->nextHandshake     = 0;
	ss->securityHandshake = 0;
    }

    


    PORT_Memset(&ss->xtnData, 0, sizeof(TLSExtensionData));
    ss->statelessResume = PR_FALSE;

    if (IS_DTLS(ss)) {
	dtls_RehandshakeCleanup(ss);
    }

    tmp = ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
    if (tmp < 0)
	goto loser;		

    
    if (IS_DTLS(ss)) {
	ss->clientHelloVersion = version =
	    dtls_DTLSVersionToTLSVersion((SSL3ProtocolVersion)tmp);
    } else {
	ss->clientHelloVersion = version = (SSL3ProtocolVersion)tmp;
    }

    rv = ssl3_NegotiateVersion(ss, version, PR_TRUE);
    if (rv != SECSuccess) {
    	desc = (version > SSL_LIBRARY_VERSION_3_0) ? protocol_version 
	                                           : handshake_failure;
	errCode = SSL_ERROR_UNSUPPORTED_VERSION;
	goto alert_loser;
    }

    rv = ssl3_InitHandshakeHashes(ss);
    if (rv != SECSuccess) {
	desc = internal_error;
	errCode = PORT_GetError();
	goto alert_loser;
    }

    
    rv = ssl3_ConsumeHandshake(
	ss, &ss->ssl3.hs.client_random, SSL3_RANDOM_LENGTH, &b, &length);
    if (rv != SECSuccess) {
	goto loser;		
    }

    
    rv = ssl3_ConsumeHandshakeVariable(ss, &sidBytes, 1, &b, &length);
    if (rv != SECSuccess) {
	goto loser;		
    }

    
    if (IS_DTLS(ss)) {
	rv = ssl3_ConsumeHandshakeVariable(ss, &cookieBytes, 1, &b, &length);
	if (rv != SECSuccess) {
	    goto loser;		
	}
    }

    
    rv = ssl3_ConsumeHandshakeVariable(ss, &suites, 2, &b, &length);
    if (rv != SECSuccess) {
	goto loser;		
    }

    

    if (ss->vrange.max > ss->clientHelloVersion) {
	for (i = 0; i + 1 < suites.len; i += 2) {
	    PRUint16 suite_i = (suites.data[i] << 8) | suites.data[i + 1];
	    if (suite_i != TLS_FALLBACK_SCSV)
		continue;
	    desc = inappropriate_fallback;
	    errCode = SSL_ERROR_INAPPROPRIATE_FALLBACK_ALERT;
	    goto alert_loser;
	}
    }

    
    rv = ssl3_ConsumeHandshakeVariable(ss, &comps, 1, &b, &length);
    if (rv != SECSuccess) {
	goto loser;		
    }

    
    if (ss->version >= SSL_LIBRARY_VERSION_TLS_1_3) {
        if (comps.len != 1 || comps.data[0] != ssl_compression_null) {
            goto loser;
        }
    }
    desc = handshake_failure;

    






    if (length) {
	
	PRInt32 extension_length;
	extension_length = ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
	if (extension_length < 0) {
	    goto loser;				
	}
	if (extension_length != length) {
	    ssl3_DecodeError(ss);		
	    goto loser;
	}
	rv = ssl3_HandleHelloExtensions(ss, &b, &length);
	if (rv != SECSuccess) {
	    goto loser;		
	}
    }
    if (!ssl3_ExtensionNegotiated(ss, ssl_renegotiation_info_xtn)) {
    	



	for (i = 0; i + 1 < suites.len; i += 2) {
	    PRUint16 suite_i = (suites.data[i] << 8) | suites.data[i + 1];
	    if (suite_i == TLS_EMPTY_RENEGOTIATION_INFO_SCSV) {
		SSL3Opaque * b2 = (SSL3Opaque *)emptyRIext;
		PRUint32     L2 = sizeof emptyRIext;
		(void)ssl3_HandleHelloExtensions(ss, &b2, &L2);
	    	break;
	    }
	}
    }
    if (ss->firstHsDone &&
        (ss->opt.enableRenegotiation == SSL_RENEGOTIATE_REQUIRES_XTN ||
        ss->opt.enableRenegotiation == SSL_RENEGOTIATE_TRANSITIONAL) && 
	!ssl3_ExtensionNegotiated(ss, ssl_renegotiation_info_xtn)) {
	desc    = no_renegotiation;
	level   = alert_warning;
	errCode = SSL_ERROR_RENEGOTIATION_NOT_ALLOWED;
	goto alert_loser;
    }
    if ((ss->opt.requireSafeNegotiation || 
         (ss->firstHsDone && ss->peerRequestedProtection)) &&
	!ssl3_ExtensionNegotiated(ss, ssl_renegotiation_info_xtn)) {
	desc = handshake_failure;
	errCode = SSL_ERROR_UNSAFE_NEGOTIATION;
    	goto alert_loser;
    }

    




    if (!ssl3_ExtensionNegotiated(ss, ssl_session_ticket_xtn) ||
	ss->xtnData.emptySessionTicket) {
	if (sidBytes.len > 0 && !ss->opt.noCache) {
	    SSL_TRC(7, ("%d: SSL3[%d]: server, lookup client session-id for 0x%08x%08x%08x%08x",
			SSL_GETPID(), ss->fd, ss->sec.ci.peer.pr_s6_addr32[0],
			ss->sec.ci.peer.pr_s6_addr32[1], 
			ss->sec.ci.peer.pr_s6_addr32[2],
			ss->sec.ci.peer.pr_s6_addr32[3]));
	    if (ssl_sid_lookup) {
		sid = (*ssl_sid_lookup)(&ss->sec.ci.peer, sidBytes.data, 
					sidBytes.len, ss->dbHandle);
	    } else {
		errCode = SSL_ERROR_SERVER_CACHE_NOT_CONFIGURED;
		goto loser;
	    }
	}
    } else if (ss->statelessResume) {
	


	sid = ss->sec.ci.sid;
	PORT_Assert(sid != NULL);  

	if (sidBytes.len > 0 && sidBytes.len <= SSL3_SESSIONID_BYTES) {
	    sid->u.ssl3.sessionIDLength = sidBytes.len;
	    PORT_Memcpy(sid->u.ssl3.sessionID, sidBytes.data,
		sidBytes.len);
	    sid->u.ssl3.sessionIDLength = sidBytes.len;
	} else {
	    sid->u.ssl3.sessionIDLength = 0;
	}
	ss->sec.ci.sid = NULL;
    }

    









    if (ssl3_ExtensionNegotiated(ss, ssl_session_ticket_xtn) && sid == NULL) {
	ssl3_RegisterServerHelloExtensionSender(ss,
	    ssl_session_ticket_xtn, ssl3_SendSessionTicketXtn);
    }

    if (sid != NULL) {
	





	if ((sid->peerCert == NULL) && ss->opt.requestCertificate &&
	    ((ss->opt.requireCertificate == SSL_REQUIRE_ALWAYS) ||
	     (ss->opt.requireCertificate == SSL_REQUIRE_NO_ERROR) ||
	     ((ss->opt.requireCertificate == SSL_REQUIRE_FIRST_HANDSHAKE) 
	      && !ss->firstHsDone))) {

	    SSL_AtomicIncrementLong(& ssl3stats.hch_sid_cache_not_ok );
	    if (ss->sec.uncache)
                ss->sec.uncache(sid);
	    ssl_FreeSID(sid);
	    sid = NULL;
	}
    }

#ifndef NSS_DISABLE_ECC
    
    ssl3_FilterECCipherSuitesByServerCerts(ss);
#endif

    if (IS_DTLS(ss)) {
	ssl3_DisableNonDTLSSuites(ss);
    }

#ifdef PARANOID
    
    j = ssl3_config_match_init(ss);
    if (j <= 0) {		
    	errCode = PORT_GetError();	
	goto alert_loser;
    }
#endif

    



    if (sid) do {
	ssl3CipherSuiteCfg *suite;
#ifdef PARANOID
	SSLVersionRange vrange = {ss->version, ss->version};
#endif

	
	if (!compressionEnabled(ss, sid->u.ssl3.compression))
	    break;

	
	for (i = 0; i < comps.len; i++) {
	    if (comps.data[i] == sid->u.ssl3.compression)
		break;
	}
	if (i == comps.len)
	    break;

	suite = ss->cipherSuites;
	
	for (j = ssl_V3_SUITES_IMPLEMENTED; j > 0; --j, ++suite) {
	    if (suite->cipher_suite == sid->u.ssl3.cipherSuite)
		break;
	}
	PORT_Assert(j > 0);
	if (j <= 0)
	    break;
#ifdef PARANOID
	




	if (!config_match(suite, ss->ssl3.policy, PR_TRUE, &vrange))
	    break;
#else
	if (!suite->enabled)
	    break;
#endif
	
	for (i = 0; i + 1 < suites.len; i += 2) {
	    PRUint16 suite_i = (suites.data[i] << 8) | suites.data[i + 1];
	    if (suite_i == suite->cipher_suite) {
		ss->ssl3.hs.cipher_suite = suite->cipher_suite;
		ss->ssl3.hs.suite_def =
		    ssl_LookupCipherSuiteDef(ss->ssl3.hs.cipher_suite);

		
		ss->ssl3.hs.compression = sid->u.ssl3.compression;
		goto compression_found;
	    }
	}
    } while (0);

    

#ifndef PARANOID
    
    j = ssl3_config_match_init(ss);
    if (j <= 0) {		
    	errCode = PORT_GetError();	
	goto alert_loser;
    }
#endif

    













    for (j = 0; j < ssl_V3_SUITES_IMPLEMENTED; j++) {
	ssl3CipherSuiteCfg *suite = &ss->cipherSuites[j];
	SSLVersionRange vrange = {ss->version, ss->version};
	if (!config_match(suite, ss->ssl3.policy, PR_TRUE, &vrange)) {
	    continue;
	}
	for (i = 0; i + 1 < suites.len; i += 2) {
	    PRUint16 suite_i = (suites.data[i] << 8) | suites.data[i + 1];
	    if (suite_i == suite->cipher_suite) {
		ss->ssl3.hs.cipher_suite = suite->cipher_suite;
		ss->ssl3.hs.suite_def =
		    ssl_LookupCipherSuiteDef(ss->ssl3.hs.cipher_suite);
		goto suite_found;
	    }
	}
    }
    errCode = SSL_ERROR_NO_CYPHER_OVERLAP;
    goto alert_loser;

suite_found:
    
    for (i = 0; i < comps.len; i++) {
	if (!compressionEnabled(ss, comps.data[i]))
	    continue;
	for (j = 0; j < compressionMethodsCount; j++) {
	    if (comps.data[i] == compressions[j]) {
		ss->ssl3.hs.compression = 
					(SSLCompressionMethod)compressions[j];
		goto compression_found;
	    }
	}
    }
    errCode = SSL_ERROR_NO_COMPRESSION_OVERLAP;
    				
    goto alert_loser;

compression_found:
    suites.data = NULL;
    comps.data = NULL;

    ss->sec.send = ssl3_SendApplicationData;

    



    if (sid != NULL) do {
	ssl3CipherSpec *pwSpec;
	SECItem         wrappedMS;  	

	if (sid->version != ss->version  ||
	    sid->u.ssl3.cipherSuite != ss->ssl3.hs.cipher_suite ||
	    sid->u.ssl3.compression != ss->ssl3.hs.compression) {
	    break;	
	}

	if (ss->sec.ci.sid) {
	    if (ss->sec.uncache)
                ss->sec.uncache(ss->sec.ci.sid);
	    PORT_Assert(ss->sec.ci.sid != sid);  
	    if (ss->sec.ci.sid != sid) {
		ssl_FreeSID(ss->sec.ci.sid);
	    }
	    ss->sec.ci.sid = NULL;
	}
	

	ssl_GetSpecWriteLock(ss);  haveSpecWriteLock = PR_TRUE;
	pwSpec = ss->ssl3.pwSpec;
	if (sid->u.ssl3.keys.msIsWrapped) {
	    PK11SymKey *    wrapKey; 	
	    CK_FLAGS        keyFlags      = 0;
#ifndef NO_PKCS11_BYPASS
	    if (ss->opt.bypassPKCS11) {
		


		break;  
	    }
#endif

	    wrapKey = getWrappingKey(ss, NULL, sid->u.ssl3.exchKeyType,
				     sid->u.ssl3.masterWrapMech, 
				     ss->pkcs11PinArg);
	    if (!wrapKey) {
		
		break;
	    }

	    if (ss->version > SSL_LIBRARY_VERSION_3_0) {	
		keyFlags = CKF_SIGN | CKF_VERIFY;
	    }

	    wrappedMS.data = sid->u.ssl3.keys.wrapped_master_secret;
	    wrappedMS.len  = sid->u.ssl3.keys.wrapped_master_secret_len;

	    
	    pwSpec->master_secret =
		PK11_UnwrapSymKeyWithFlags(wrapKey, sid->u.ssl3.masterWrapMech, 
			    NULL, &wrappedMS, CKM_SSL3_MASTER_KEY_DERIVE,
			    CKA_DERIVE, sizeof(SSL3MasterSecret), keyFlags);
	    PK11_FreeSymKey(wrapKey);
	    if (pwSpec->master_secret == NULL) {
		break;	
	    }
#ifndef NO_PKCS11_BYPASS
	} else if (ss->opt.bypassPKCS11) {
	    wrappedMS.data = sid->u.ssl3.keys.wrapped_master_secret;
	    wrappedMS.len  = sid->u.ssl3.keys.wrapped_master_secret_len;
	    memcpy(pwSpec->raw_master_secret, wrappedMS.data, wrappedMS.len);
	    pwSpec->msItem.data = pwSpec->raw_master_secret;
	    pwSpec->msItem.len  = wrappedMS.len;
#endif
	} else {
	    
	    
	    PK11SlotInfo * slot;
	    wrappedMS.data = sid->u.ssl3.keys.wrapped_master_secret;
	    wrappedMS.len  = sid->u.ssl3.keys.wrapped_master_secret_len;
	    slot = PK11_GetInternalSlot();
	    pwSpec->master_secret =  
		PK11_ImportSymKey(slot, CKM_SSL3_MASTER_KEY_DERIVE, 
				  PK11_OriginUnwrap, CKA_ENCRYPT, &wrappedMS, 
				  NULL);
	    PK11_FreeSlot(slot);
	    if (pwSpec->master_secret == NULL) {
		break;	
	    }
	}
	ss->sec.ci.sid = sid;
	if (sid->peerCert != NULL) {
	    ss->sec.peerCert = CERT_DupCertificate(sid->peerCert);
	}

	




	SSL_AtomicIncrementLong(& ssl3stats.hch_sid_cache_hits );
	if (ss->statelessResume)
	    SSL_AtomicIncrementLong(& ssl3stats.hch_sid_stateless_resumes );
	ss->ssl3.hs.isResuming = PR_TRUE;

        ss->sec.authAlgorithm = sid->authAlgorithm;
	ss->sec.authKeyBits   = sid->authKeyBits;
	ss->sec.keaType       = sid->keaType;
	ss->sec.keaKeyBits    = sid->keaKeyBits;

	




	ss->sec.localCert     = 
		CERT_DupCertificate(ss->serverCerts[sid->keaType].serverCert);

        
        if (sid != NULL &&
            sid->version > SSL_LIBRARY_VERSION_3_0 &&
            sid->u.ssl3.srvName.len && sid->u.ssl3.srvName.data) {
            
            SECItem *sidName = &sid->u.ssl3.srvName;
            SECItem *pwsName = &ss->ssl3.pwSpec->srvVirtName;
            if (pwsName->data) {
                SECITEM_FreeItem(pwsName, PR_FALSE);
            }
            rv = SECITEM_CopyItem(NULL, pwsName, sidName);
            if (rv != SECSuccess) {
                errCode = PORT_GetError();
                desc = internal_error;
                goto alert_loser;
            }
        }

        
        if (ssl3_ExtensionNegotiated(ss, ssl_server_name_xtn) &&
            ss->xtnData.sniNameArr) {
            PORT_Free(ss->xtnData.sniNameArr);
            ss->xtnData.sniNameArr = NULL;
            ss->xtnData.sniNameArrSize = 0;
        }

	ssl_GetXmitBufLock(ss); haveXmitBufLock = PR_TRUE;

	rv = ssl3_SendServerHello(ss);
	if (rv != SECSuccess) {
	    errCode = PORT_GetError();
	    goto loser;
	}

	if (haveSpecWriteLock) {
	    ssl_ReleaseSpecWriteLock(ss);
	    haveSpecWriteLock = PR_FALSE;
	}

	
	rv = ssl3_InitPendingCipherSpec(ss,  NULL);
	if (rv != SECSuccess) {
	    errCode = PORT_GetError();
	    goto loser;
	}

	rv = ssl3_SendChangeCipherSpecs(ss);
	if (rv != SECSuccess) {
	    errCode = PORT_GetError();
	    goto loser;
	}
	rv = ssl3_SendFinished(ss, 0);
	ss->ssl3.hs.ws = wait_change_cipher;
	if (rv != SECSuccess) {
	    errCode = PORT_GetError();
	    goto loser;
	}

	if (haveXmitBufLock) {
	    ssl_ReleaseXmitBufLock(ss);
	    haveXmitBufLock = PR_FALSE;
	}

        return SECSuccess;
    } while (0);

    if (haveSpecWriteLock) {
	ssl_ReleaseSpecWriteLock(ss);
	haveSpecWriteLock = PR_FALSE;
    }

    if (sid) { 	
	SSL_AtomicIncrementLong(& ssl3stats.hch_sid_cache_not_ok );
	if (ss->sec.uncache)
            ss->sec.uncache(sid);
	ssl_FreeSID(sid);
	sid = NULL;
    }
    SSL_AtomicIncrementLong(& ssl3stats.hch_sid_cache_misses );

    if (ssl3_ExtensionNegotiated(ss, ssl_server_name_xtn)) {
        int ret = 0;
        if (ss->sniSocketConfig) do { 
            ret = SSL_SNI_SEND_ALERT;
            
            if (ss->xtnData.sniNameArrSize) {
                
                ret = (SECStatus)(*ss->sniSocketConfig)(ss->fd,
                                         ss->xtnData.sniNameArr,
                                      ss->xtnData.sniNameArrSize,
                                          ss->sniSocketConfigArg);
            }
            if (ret <= SSL_SNI_SEND_ALERT) {
                

                errCode = SSL_ERROR_UNRECOGNIZED_NAME_ALERT;
                desc = unrecognized_name;
                break;
            } else if (ret == SSL_SNI_CURRENT_CONFIG_IS_USED) {
                SECStatus       rv = SECSuccess;
                SECItem *       cwsName, *pwsName;

                ssl_GetSpecWriteLock(ss);  
                pwsName = &ss->ssl3.pwSpec->srvVirtName;
                cwsName = &ss->ssl3.cwSpec->srvVirtName;
#ifndef SSL_SNI_ALLOW_NAME_CHANGE_2HS
                
                if (ss->firstHsDone) {
                    if (ssl3_ServerNameCompare(pwsName, cwsName)) {
                        ssl_ReleaseSpecWriteLock(ss);  
                        errCode = SSL_ERROR_UNRECOGNIZED_NAME_ALERT;
                        desc = handshake_failure;
                        ret = SSL_SNI_SEND_ALERT;
                        break;
                    }
                }
#endif
                if (pwsName->data) {
                    SECITEM_FreeItem(pwsName, PR_FALSE);
                }
                if (cwsName->data) {
                    rv = SECITEM_CopyItem(NULL, pwsName, cwsName);
                }
                ssl_ReleaseSpecWriteLock(ss);  
                if (rv != SECSuccess) {
                    errCode = SSL_ERROR_INTERNAL_ERROR_ALERT;
                    desc = internal_error;
                    ret = SSL_SNI_SEND_ALERT;
                    break;
                }
            } else if (ret < ss->xtnData.sniNameArrSize) {
                

                SECStatus       rv;
                SECItem *       name = &ss->xtnData.sniNameArr[ret];
                int             configedCiphers;
                SECItem *       pwsName;

                
                
                ssl_GetSpecWriteLock(ss);  
#ifndef SSL_SNI_ALLOW_NAME_CHANGE_2HS
                
                if (ss->firstHsDone) {
                    SECItem *cwsName = &ss->ssl3.cwSpec->srvVirtName;
                    if (ssl3_ServerNameCompare(name, cwsName)) {
                        ssl_ReleaseSpecWriteLock(ss);  
                        errCode = SSL_ERROR_UNRECOGNIZED_NAME_ALERT;
                        desc = handshake_failure;
                        ret = SSL_SNI_SEND_ALERT;
                        break;
                    }
                }
#endif
                pwsName = &ss->ssl3.pwSpec->srvVirtName;
                if (pwsName->data) {
                    SECITEM_FreeItem(pwsName, PR_FALSE);
                }
                rv = SECITEM_CopyItem(NULL, pwsName, name);
                ssl_ReleaseSpecWriteLock(ss);  
                if (rv != SECSuccess) {
                    errCode = SSL_ERROR_INTERNAL_ERROR_ALERT;
                    desc = internal_error;
                    ret = SSL_SNI_SEND_ALERT;
                    break;
                }
                configedCiphers = ssl3_config_match_init(ss);
                if (configedCiphers <= 0) {
                    
                    errCode = PORT_GetError();
                    desc = handshake_failure;
                    ret = SSL_SNI_SEND_ALERT;
                    break;
                }
                


                ssl3_RegisterServerHelloExtensionSender(ss, ssl_server_name_xtn,
                                                        ssl3_SendServerNameXtn);
            } else {
                
                PORT_Assert(ret < ss->xtnData.sniNameArrSize);
                errCode = SSL_ERROR_INTERNAL_ERROR_ALERT;
                desc = internal_error;
                ret = SSL_SNI_SEND_ALERT;
                break;
            }
        } while (0);
        



        if (ss->xtnData.sniNameArr) {
            PORT_Free(ss->xtnData.sniNameArr);
            ss->xtnData.sniNameArr = NULL;
            ss->xtnData.sniNameArrSize = 0;
        }
        if (ret <= SSL_SNI_SEND_ALERT) {
            
            goto alert_loser;
        }
    }
#ifndef SSL_SNI_ALLOW_NAME_CHANGE_2HS
    else if (ss->firstHsDone) {
        

        PRBool passed = PR_TRUE;
        ssl_GetSpecReadLock(ss);  
        if (ss->ssl3.cwSpec->srvVirtName.data) {
            passed = PR_FALSE;
        }
        ssl_ReleaseSpecReadLock(ss);  
        if (!passed) {
            errCode = SSL_ERROR_UNRECOGNIZED_NAME_ALERT;
            desc = handshake_failure;
            goto alert_loser;
        }
    }
#endif

    sid = ssl3_NewSessionID(ss, PR_TRUE);
    if (sid == NULL) {
	errCode = PORT_GetError();
	goto loser;	
    }
    ss->sec.ci.sid = sid;

    ss->ssl3.hs.isResuming = PR_FALSE;
    ssl_GetXmitBufLock(ss);
    rv = ssl3_SendServerHelloSequence(ss);
    ssl_ReleaseXmitBufLock(ss);
    if (rv != SECSuccess) {
	errCode = PORT_GetError();
	goto loser;
    }

    if (haveXmitBufLock) {
	ssl_ReleaseXmitBufLock(ss);
	haveXmitBufLock = PR_FALSE;
    }

    return SECSuccess;

alert_loser:
    if (haveSpecWriteLock) {
	ssl_ReleaseSpecWriteLock(ss);
	haveSpecWriteLock = PR_FALSE;
    }
    (void)SSL3_SendAlert(ss, level, desc);
    
loser:
    if (haveSpecWriteLock) {
	ssl_ReleaseSpecWriteLock(ss);
	haveSpecWriteLock = PR_FALSE;
    }

    if (haveXmitBufLock) {
	ssl_ReleaseXmitBufLock(ss);
	haveXmitBufLock = PR_FALSE;
    }

    PORT_SetError(errCode);
    return SECFailure;
}






SECStatus
ssl3_HandleV2ClientHello(sslSocket *ss, unsigned char *buffer, int length)
{
    sslSessionID *      sid 		= NULL;
    unsigned char *     suites;
    unsigned char *     random;
    SSL3ProtocolVersion version;
    SECStatus           rv;
    int                 i;
    int                 j;
    int                 sid_length;
    int                 suite_length;
    int                 rand_length;
    int                 errCode  = SSL_ERROR_RX_MALFORMED_CLIENT_HELLO;
    SSL3AlertDescription desc    = handshake_failure;

    SSL_TRC(3, ("%d: SSL3[%d]: handle v2 client_hello", SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );

    ssl_GetSSL3HandshakeLock(ss);

    PORT_Memset(&ss->xtnData, 0, sizeof(TLSExtensionData));

    rv = ssl3_InitState(ss);
    if (rv != SECSuccess) {
	ssl_ReleaseSSL3HandshakeLock(ss);
	return rv;		
    }
    rv = ssl3_RestartHandshakeHashes(ss);
    if (rv != SECSuccess) {
	ssl_ReleaseSSL3HandshakeLock(ss);
	return rv;
    }

    if (ss->ssl3.hs.ws != wait_client_hello) {
	desc    = unexpected_message;
	errCode = SSL_ERROR_RX_UNEXPECTED_CLIENT_HELLO;
	goto loser;	
    }

    version      = (buffer[1] << 8) | buffer[2];
    suite_length = (buffer[3] << 8) | buffer[4];
    sid_length   = (buffer[5] << 8) | buffer[6];
    rand_length  = (buffer[7] << 8) | buffer[8];
    ss->clientHelloVersion = version;

    rv = ssl3_NegotiateVersion(ss, version, PR_TRUE);
    if (rv != SECSuccess) {
	
	desc = (version > SSL_LIBRARY_VERSION_3_0) ? protocol_version
	                                           : handshake_failure;
	errCode = SSL_ERROR_UNSUPPORTED_VERSION;
	goto alert_loser;
    }

    rv = ssl3_InitHandshakeHashes(ss);
    if (rv != SECSuccess) {
	desc = internal_error;
	errCode = PORT_GetError();
	goto alert_loser;
    }

    
    if (length !=
        SSL_HL_CLIENT_HELLO_HBYTES + suite_length + sid_length + rand_length) {
	SSL_DBG(("%d: SSL3[%d]: bad v2 client hello message, len=%d should=%d",
		 SSL_GETPID(), ss->fd, length,
		 SSL_HL_CLIENT_HELLO_HBYTES + suite_length + sid_length +
		 rand_length));
	goto loser;		
    }

    suites = buffer + SSL_HL_CLIENT_HELLO_HBYTES;
    random = suites + suite_length + sid_length;

    if (rand_length < SSL_MIN_CHALLENGE_BYTES ||
	rand_length > SSL_MAX_CHALLENGE_BYTES) {
	goto loser;		
    }

    PORT_Assert(SSL_MAX_CHALLENGE_BYTES == SSL3_RANDOM_LENGTH);

    PORT_Memset(&ss->ssl3.hs.client_random, 0, SSL3_RANDOM_LENGTH);
    PORT_Memcpy(
	&ss->ssl3.hs.client_random.rand[SSL3_RANDOM_LENGTH - rand_length],
	random, rand_length);

    PRINT_BUF(60, (ss, "client random:", &ss->ssl3.hs.client_random.rand[0],
		   SSL3_RANDOM_LENGTH));
#ifndef NSS_DISABLE_ECC
    
    ssl3_FilterECCipherSuitesByServerCerts(ss);
#endif
    i = ssl3_config_match_init(ss);
    if (i <= 0) {
    	errCode = PORT_GetError();	
	goto alert_loser;
    }

    






    for (j = 0; j < ssl_V3_SUITES_IMPLEMENTED; j++) {
	ssl3CipherSuiteCfg *suite = &ss->cipherSuites[j];
	SSLVersionRange vrange = {ss->version, ss->version};
	if (!config_match(suite, ss->ssl3.policy, PR_TRUE, &vrange)) {
	    continue;
	}
	for (i = 0; i+2 < suite_length; i += 3) {
	    PRUint32 suite_i = (suites[i] << 16)|(suites[i+1] << 8)|suites[i+2];
	    if (suite_i == suite->cipher_suite) {
		ss->ssl3.hs.cipher_suite = suite->cipher_suite;
		ss->ssl3.hs.suite_def =
		    ssl_LookupCipherSuiteDef(ss->ssl3.hs.cipher_suite);
		goto suite_found;
	    }
	}
    }
    errCode = SSL_ERROR_NO_CYPHER_OVERLAP;
    goto alert_loser;

suite_found:

    


    for (i = 0; i+2 < suite_length; i += 3) {
	PRUint32 suite_i = (suites[i] << 16) | (suites[i+1] << 8) | suites[i+2];
	if (suite_i == TLS_EMPTY_RENEGOTIATION_INFO_SCSV) {
	    SSL3Opaque * b2 = (SSL3Opaque *)emptyRIext;
	    PRUint32     L2 = sizeof emptyRIext;
	    (void)ssl3_HandleHelloExtensions(ss, &b2, &L2);
	    break;
	}
    }

    if (ss->opt.requireSafeNegotiation &&
	!ssl3_ExtensionNegotiated(ss, ssl_renegotiation_info_xtn)) {
	desc = handshake_failure;
	errCode = SSL_ERROR_UNSAFE_NEGOTIATION;
    	goto alert_loser;
    }

    ss->ssl3.hs.compression = ssl_compression_null;
    ss->sec.send            = ssl3_SendApplicationData;

    
    SSL_AtomicIncrementLong(& ssl3stats.hch_sid_cache_misses );
    sid = ssl3_NewSessionID(ss, PR_TRUE);
    if (sid == NULL) {
    	errCode = PORT_GetError();
	goto loser;	
    }
    ss->sec.ci.sid = sid;
    

    
    rv = ssl3_UpdateHandshakeHashes(ss, buffer, length);
    if (rv != SECSuccess) {
    	errCode = PORT_GetError();
	goto loser;
    }

    ssl_GetXmitBufLock(ss);
    rv = ssl3_SendServerHelloSequence(ss);
    ssl_ReleaseXmitBufLock(ss);
    if (rv != SECSuccess) {
    	errCode = PORT_GetError();
	goto loser;
    }

    






    ssl_ReleaseSSL3HandshakeLock(ss);
    return SECSuccess;

alert_loser:
    SSL3_SendAlert(ss, alert_fatal, desc);
loser:
    ssl_ReleaseSSL3HandshakeLock(ss);
    PORT_SetError(errCode);
    return SECFailure;
}







static SECStatus
ssl3_SendServerHello(sslSocket *ss)
{
    sslSessionID *sid;
    SECStatus     rv;
    PRUint32      maxBytes = 65535;
    PRUint32      length;
    PRInt32       extensions_len = 0;
    SSL3ProtocolVersion version;

    SSL_TRC(3, ("%d: SSL3[%d]: send server_hello handshake", SSL_GETPID(),
		ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    if (!IS_DTLS(ss)) {
	PORT_Assert(MSB(ss->version) == MSB(SSL_LIBRARY_VERSION_3_0));

	if (MSB(ss->version) != MSB(SSL_LIBRARY_VERSION_3_0)) {
	    PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	    return SECFailure;
	}
    } else {
	PORT_Assert(MSB(ss->version) == MSB(SSL_LIBRARY_VERSION_DTLS_1_0));

	if (MSB(ss->version) != MSB(SSL_LIBRARY_VERSION_DTLS_1_0)) {
	    PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	    return SECFailure;
	}
    }

    sid = ss->sec.ci.sid;

    extensions_len = ssl3_CallHelloExtensionSenders(ss, PR_FALSE, maxBytes,
					       &ss->xtnData.serverSenders[0]);
    if (extensions_len > 0)
    	extensions_len += 2; 

    length = sizeof(SSL3ProtocolVersion) + SSL3_RANDOM_LENGTH + 1 +
             ((sid == NULL) ? 0: sid->u.ssl3.sessionIDLength) +
	     sizeof(ssl3CipherSuite) + 1 + extensions_len;
    rv = ssl3_AppendHandshakeHeader(ss, server_hello, length);
    if (rv != SECSuccess) {
	return rv;	
    }

    if (IS_DTLS(ss)) {
	version = dtls_TLSVersionToDTLSVersion(ss->version);
    } else {
	version = ss->version;
    }

    rv = ssl3_AppendHandshakeNumber(ss, version, 2);
    if (rv != SECSuccess) {
	return rv;	
    }
    rv = ssl3_GetNewRandom(&ss->ssl3.hs.server_random);
    if (rv != SECSuccess) {
	ssl_MapLowLevelError(SSL_ERROR_GENERATE_RANDOM_FAILURE);
	return rv;
    }
    rv = ssl3_AppendHandshake(
	ss, &ss->ssl3.hs.server_random, SSL3_RANDOM_LENGTH);
    if (rv != SECSuccess) {
	return rv;	
    }

    if (sid)
	rv = ssl3_AppendHandshakeVariable(
	    ss, sid->u.ssl3.sessionID, sid->u.ssl3.sessionIDLength, 1);
    else
	rv = ssl3_AppendHandshakeNumber(ss, 0, 1);
    if (rv != SECSuccess) {
	return rv;	
    }

    rv = ssl3_AppendHandshakeNumber(ss, ss->ssl3.hs.cipher_suite, 2);
    if (rv != SECSuccess) {
	return rv;	
    }
    rv = ssl3_AppendHandshakeNumber(ss, ss->ssl3.hs.compression, 1);
    if (rv != SECSuccess) {
	return rv;	
    }
    if (extensions_len) {
	PRInt32 sent_len;

    	extensions_len -= 2;
	rv = ssl3_AppendHandshakeNumber(ss, extensions_len, 2);
	if (rv != SECSuccess) 
	    return rv;	
	sent_len = ssl3_CallHelloExtensionSenders(ss, PR_TRUE, extensions_len,
					   &ss->xtnData.serverSenders[0]);
        PORT_Assert(sent_len == extensions_len);
	if (sent_len != extensions_len) {
	    if (sent_len >= 0)
	    	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return SECFailure;
	}
    }
    rv = ssl3_SetupPendingCipherSpec(ss);
    if (rv != SECSuccess) {
	return rv;	
    }

    return SECSuccess;
}






static SECStatus
ssl3_PickSignatureHashAlgorithm(sslSocket *ss,
				SSL3SignatureAndHashAlgorithm* out)
{
    TLSSignatureAlgorithm sigAlg;
    unsigned int i, j;
    

    static const SECOidTag hashPreference[] = {
        SEC_OID_SHA256,
        SEC_OID_SHA384,
        SEC_OID_SHA512,
        SEC_OID_SHA1,
    };

    switch (ss->ssl3.hs.kea_def->kea) {
    case kea_rsa:
    case kea_rsa_export:
    case kea_rsa_export_1024:
    case kea_dh_rsa:
    case kea_dh_rsa_export:
    case kea_dhe_rsa:
    case kea_dhe_rsa_export:
    case kea_rsa_fips:
    case kea_ecdh_rsa:
    case kea_ecdhe_rsa:
	sigAlg = tls_sig_rsa;
	break;
    case kea_dh_dss:
    case kea_dh_dss_export:
    case kea_dhe_dss:
    case kea_dhe_dss_export:
	sigAlg = tls_sig_dsa;
	break;
    case kea_ecdh_ecdsa:
    case kea_ecdhe_ecdsa:
	sigAlg = tls_sig_ecdsa;
	break;
    default:
	PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
	return SECFailure;
    }
    out->sigAlg = sigAlg;

    if (ss->version <= SSL_LIBRARY_VERSION_TLS_1_1) {
	

	out->hashAlg = SEC_OID_UNKNOWN;
	return SECSuccess;
    }

    if (ss->ssl3.hs.numClientSigAndHash == 0) {
	


	out->hashAlg = SEC_OID_SHA1;
	return SECSuccess;
    }

    for (i = 0; i < PR_ARRAY_SIZE(hashPreference); i++) {
	for (j = 0; j < ss->ssl3.hs.numClientSigAndHash; j++) {
	    const SSL3SignatureAndHashAlgorithm* sh =
		&ss->ssl3.hs.clientSigAndHash[j];
	    if (sh->sigAlg == sigAlg && sh->hashAlg == hashPreference[i]) {
		out->hashAlg = sh->hashAlg;
		return SECSuccess;
	    }
	}
    }

    PORT_SetError(SSL_ERROR_UNSUPPORTED_HASH_ALGORITHM);
    return SECFailure;
}


static SECStatus
ssl3_SendServerKeyExchange(sslSocket *ss)
{
    const ssl3KEADef * kea_def     = ss->ssl3.hs.kea_def;
    SECStatus          rv          = SECFailure;
    int                length;
    PRBool             isTLS;
    SECItem            signed_hash = {siBuffer, NULL, 0};
    SSL3Hashes         hashes;
    SECKEYPublicKey *  sdPub;	
    SSL3SignatureAndHashAlgorithm sigAndHash;

    SSL_TRC(3, ("%d: SSL3[%d]: send server_key_exchange handshake",
		SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    if (ssl3_PickSignatureHashAlgorithm(ss, &sigAndHash) != SECSuccess) {
	return SECFailure;
    }

    switch (kea_def->exchKeyType) {
    case kt_rsa:
	
	sdPub = ss->stepDownKeyPair->pubKey;
	PORT_Assert(sdPub != NULL);
	if (!sdPub) {
	    PORT_SetError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	    return SECFailure;
	}
	rv = ssl3_ComputeExportRSAKeyHash(sigAndHash.hashAlg,
					  sdPub->u.rsa.modulus,
					  sdPub->u.rsa.publicExponent,
	                                  &ss->ssl3.hs.client_random,
	                                  &ss->ssl3.hs.server_random,
					  &hashes, ss->opt.bypassPKCS11);
        if (rv != SECSuccess) {
	    ssl_MapLowLevelError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	    return rv;
	}

	isTLS = (PRBool)(ss->ssl3.pwSpec->version > SSL_LIBRARY_VERSION_3_0);
	rv = ssl3_SignHashes(&hashes, ss->serverCerts[kt_rsa].SERVERKEY, 
	                     &signed_hash, isTLS);
        if (rv != SECSuccess) {
	    goto loser;		
	}
	if (signed_hash.data == NULL) {
	    
	    PORT_SetError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	    goto loser;
	}
	length = 2 + sdPub->u.rsa.modulus.len +
	         2 + sdPub->u.rsa.publicExponent.len +
	         2 + signed_hash.len;

	rv = ssl3_AppendHandshakeHeader(ss, server_key_exchange, length);
	if (rv != SECSuccess) {
	    goto loser; 	
	}

	rv = ssl3_AppendHandshakeVariable(ss, sdPub->u.rsa.modulus.data,
					  sdPub->u.rsa.modulus.len, 2);
	if (rv != SECSuccess) {
	    goto loser; 	
	}

	rv = ssl3_AppendHandshakeVariable(
				ss, sdPub->u.rsa.publicExponent.data,
				sdPub->u.rsa.publicExponent.len, 2);
	if (rv != SECSuccess) {
	    goto loser; 	
	}

	if (ss->ssl3.pwSpec->version >= SSL_LIBRARY_VERSION_TLS_1_2) {
	    rv = ssl3_AppendSignatureAndHashAlgorithm(ss, &sigAndHash);
	    if (rv != SECSuccess) {
		goto loser; 	
	    }
	}

	rv = ssl3_AppendHandshakeVariable(ss, signed_hash.data,
	                                  signed_hash.len, 2);
	if (rv != SECSuccess) {
	    goto loser; 	
	}
	PORT_Free(signed_hash.data);
	return SECSuccess;

#ifndef NSS_DISABLE_ECC
    case kt_ecdh: {
	rv = ssl3_SendECDHServerKeyExchange(ss, &sigAndHash);
	return rv;
    }
#endif 

    case kt_dh:
    case kt_null:
    default:
	PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
	break;
    }
loser:
    if (signed_hash.data != NULL) 
    	PORT_Free(signed_hash.data);
    return SECFailure;
}


static SECStatus
ssl3_SendCertificateRequest(sslSocket *ss)
{
    PRBool         isTLS12;
    SECItem *      name;
    CERTDistNames *ca_list;
    const PRUint8 *certTypes;
    const PRUint8 *sigAlgs;
    SECItem *      names	= NULL;
    SECStatus      rv;
    int            length;
    int            i;
    int            calen	= 0;
    int            nnames	= 0;
    int            certTypesLength;
    int            sigAlgsLength;

    SSL_TRC(3, ("%d: SSL3[%d]: send certificate_request handshake",
		SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    isTLS12 = (PRBool)(ss->ssl3.pwSpec->version >= SSL_LIBRARY_VERSION_TLS_1_2);

    
    ca_list = ss->ssl3.ca_list;
    if (!ca_list) {
	ca_list = ssl3_server_ca_list;
    }

    if (ca_list != NULL) {
	names = ca_list->names;
	nnames = ca_list->nnames;
    }

    for (i = 0, name = names; i < nnames; i++, name++) {
	calen += 2 + name->len;
    }

    certTypes       = certificate_types;
    certTypesLength = sizeof certificate_types;
    sigAlgs         = supported_signature_algorithms;
    sigAlgsLength   = sizeof supported_signature_algorithms;

    length = 1 + certTypesLength + 2 + calen;
    if (isTLS12) {
	length += 2 + sigAlgsLength;
    }

    rv = ssl3_AppendHandshakeHeader(ss, certificate_request, length);
    if (rv != SECSuccess) {
	return rv; 		
    }
    rv = ssl3_AppendHandshakeVariable(ss, certTypes, certTypesLength, 1);
    if (rv != SECSuccess) {
	return rv; 		
    }
    if (isTLS12) {
	rv = ssl3_AppendHandshakeVariable(ss, sigAlgs, sigAlgsLength, 2);
	if (rv != SECSuccess) {
	    return rv; 		
	}
    }
    rv = ssl3_AppendHandshakeNumber(ss, calen, 2);
    if (rv != SECSuccess) {
	return rv; 		
    }
    for (i = 0, name = names; i < nnames; i++, name++) {
	rv = ssl3_AppendHandshakeVariable(ss, name->data, name->len, 2);
	if (rv != SECSuccess) {
	    return rv; 		
	}
    }

    return SECSuccess;
}

static SECStatus
ssl3_SendServerHelloDone(sslSocket *ss)
{
    SECStatus rv;

    SSL_TRC(3, ("%d: SSL3[%d]: send server_hello_done handshake",
		SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    rv = ssl3_AppendHandshakeHeader(ss, server_hello_done, 0);
    if (rv != SECSuccess) {
	return rv; 		
    }
    rv = ssl3_FlushHandshake(ss, 0);
    if (rv != SECSuccess) {
	return rv;	
    }
    return SECSuccess;
}





static SECStatus
ssl3_HandleCertificateVerify(sslSocket *ss, SSL3Opaque *b, PRUint32 length,
			     SSL3Hashes *hashes)
{
    SECItem              signed_hash = {siBuffer, NULL, 0};
    SECStatus            rv;
    int                  errCode     = SSL_ERROR_RX_MALFORMED_CERT_VERIFY;
    SSL3AlertDescription desc        = handshake_failure;
    PRBool               isTLS, isTLS12;
    SSL3SignatureAndHashAlgorithm sigAndHash;

    SSL_TRC(3, ("%d: SSL3[%d]: handle certificate_verify handshake",
		SSL_GETPID(), ss->fd));
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    isTLS = (PRBool)(ss->ssl3.prSpec->version > SSL_LIBRARY_VERSION_3_0);
    isTLS12 = (PRBool)(ss->ssl3.prSpec->version >= SSL_LIBRARY_VERSION_TLS_1_2);

    if (ss->ssl3.hs.ws != wait_cert_verify) {
	desc    = unexpected_message;
	errCode = SSL_ERROR_RX_UNEXPECTED_CERT_VERIFY;
	goto alert_loser;
    }

    if (isTLS12) {
	rv = ssl3_ConsumeSignatureAndHashAlgorithm(ss, &b, &length,
						   &sigAndHash);
	if (rv != SECSuccess) {
	    goto loser;	
	}
	rv = ssl3_CheckSignatureAndHashAlgorithmConsistency(
		&sigAndHash, ss->sec.peerCert);
	if (rv != SECSuccess) {
	    errCode = PORT_GetError();
	    desc = decrypt_error;
	    goto alert_loser;
	}

	

	if (sigAndHash.hashAlg != hashes->hashAlg) {
	    errCode = SSL_ERROR_UNSUPPORTED_HASH_ALGORITHM;
	    desc = decrypt_error;
	    goto alert_loser;
	}
    }

    rv = ssl3_ConsumeHandshakeVariable(ss, &signed_hash, 2, &b, &length);
    if (rv != SECSuccess) {
	goto loser;		
    }

    
    rv = ssl3_VerifySignedHashes(hashes, ss->sec.peerCert, &signed_hash,
				 isTLS, ss->pkcs11PinArg);
    if (rv != SECSuccess) {
    	errCode = PORT_GetError();
	desc = isTLS ? decrypt_error : handshake_failure;
	goto alert_loser;
    }

    signed_hash.data = NULL;

    if (length != 0) {
	desc    = isTLS ? decode_error : illegal_parameter;
	goto alert_loser;	
    }
    ss->ssl3.hs.ws = wait_change_cipher;
    return SECSuccess;

alert_loser:
    SSL3_SendAlert(ss, alert_fatal, desc);
loser:
    PORT_SetError(errCode);
    return SECFailure;
}












static PK11SymKey *
ssl3_GenerateRSAPMS(sslSocket *ss, ssl3CipherSpec *spec,
                    PK11SlotInfo * serverKeySlot)
{
    PK11SymKey *      pms		= NULL;
    PK11SlotInfo *    slot		= serverKeySlot;
    void *	      pwArg 		= ss->pkcs11PinArg;
    SECItem           param;
    CK_VERSION 	      version;
    CK_MECHANISM_TYPE mechanism_array[3];

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    if (slot == NULL) {
	SSLCipherAlgorithm calg;
	



	PORT_Assert( ss->opt.noLocks || ssl_HaveSpecWriteLock(ss));
	PORT_Assert(ss->ssl3.prSpec == ss->ssl3.pwSpec);

        calg = spec->cipher_def->calg;
	PORT_Assert(alg2Mech[calg].calg == calg);

	
	mechanism_array[0] = CKM_SSL3_PRE_MASTER_KEY_GEN;
	mechanism_array[1] = CKM_RSA_PKCS;
	mechanism_array[2] = alg2Mech[calg].cmech;

	slot = PK11_GetBestSlotMultiple(mechanism_array, 3, pwArg);
	if (slot == NULL) {
	   
	    slot = PK11_GetBestSlotMultiple(mechanism_array, 2, pwArg);
	    if (slot == NULL) {
		PORT_SetError(SSL_ERROR_TOKEN_SLOT_NOT_FOUND);
		return pms;	
	    }
	}
    }

    
    if (IS_DTLS(ss)) {
	SSL3ProtocolVersion temp;

	temp = dtls_TLSVersionToDTLSVersion(ss->clientHelloVersion);
	version.major = MSB(temp);
	version.minor = LSB(temp);
    } else {
	version.major = MSB(ss->clientHelloVersion);
	version.minor = LSB(ss->clientHelloVersion);
    }

    param.data = (unsigned char *)&version;
    param.len  = sizeof version;

    pms = PK11_KeyGen(slot, CKM_SSL3_PRE_MASTER_KEY_GEN, &param, 0, pwArg);
    if (!serverKeySlot)
	PK11_FreeSlot(slot);
    if (pms == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
    }
    return pms;
}













static SECStatus
ssl3_HandleRSAClientKeyExchange(sslSocket *ss,
                                SSL3Opaque *b,
				PRUint32 length,
				SECKEYPrivateKey *serverKey)
{
    PK11SymKey *      pms;
#ifndef NO_PKCS11_BYPASS
    unsigned char *   cr     = (unsigned char *)&ss->ssl3.hs.client_random;
    unsigned char *   sr     = (unsigned char *)&ss->ssl3.hs.server_random;
    ssl3CipherSpec *  pwSpec = ss->ssl3.pwSpec;
    unsigned int      outLen = 0;
#endif
    PRBool            isTLS  = PR_FALSE;
    SECStatus         rv;
    SECItem           enc_pms;
    unsigned char     rsaPmsBuf[SSL3_RSA_PMS_LENGTH];
    SECItem           pmsItem = {siBuffer, NULL, 0};

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( ss->ssl3.prSpec == ss->ssl3.pwSpec );

    enc_pms.data = b;
    enc_pms.len  = length;
    pmsItem.data = rsaPmsBuf;
    pmsItem.len  = sizeof rsaPmsBuf;

    if (ss->ssl3.prSpec->version > SSL_LIBRARY_VERSION_3_0) { 
	PRInt32 kLen;
	kLen = ssl3_ConsumeHandshakeNumber(ss, 2, &enc_pms.data, &enc_pms.len);
	if (kLen < 0) {
	    PORT_SetError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
	    return SECFailure;
	}
	if ((unsigned)kLen < enc_pms.len) {
	    enc_pms.len = kLen;
	}
	isTLS = PR_TRUE;
    } else {
	isTLS = (PRBool)(ss->ssl3.hs.kea_def->tls_keygen != 0);
    }

#ifndef NO_PKCS11_BYPASS
    if (ss->opt.bypassPKCS11) {
	







	rv = PK11_PrivDecryptPKCS1(serverKey, rsaPmsBuf, &outLen, 
				   sizeof rsaPmsBuf, enc_pms.data, enc_pms.len);
	if (rv != SECSuccess) {
	    
	    goto double_bypass;
	} else if (ss->opt.detectRollBack) {
	    SSL3ProtocolVersion client_version = 
					 (rsaPmsBuf[0] << 8) | rsaPmsBuf[1];

	    if (IS_DTLS(ss)) {
		client_version = dtls_DTLSVersionToTLSVersion(client_version);
	    }

	    if (client_version != ss->clientHelloVersion) {
		
		rv = PK11_GenerateRandom(rsaPmsBuf, sizeof rsaPmsBuf);
	    }
	}
	
	rv = ssl3_MasterKeyDeriveBypass(pwSpec, cr, sr, &pmsItem, isTLS, 
					PR_TRUE);
	if (rv != SECSuccess) {
	    pwSpec->msItem.data = pwSpec->raw_master_secret;
	    pwSpec->msItem.len  = SSL3_MASTER_SECRET_LENGTH;
	    PK11_GenerateRandom(pwSpec->msItem.data, pwSpec->msItem.len);
	}
	rv = ssl3_InitPendingCipherSpec(ss,  NULL);
    } else 
#endif
    {
#ifndef NO_PKCS11_BYPASS
double_bypass:
#endif
	





	pms = PK11_PubUnwrapSymKey(serverKey, &enc_pms,
				   CKM_SSL3_MASTER_KEY_DERIVE, CKA_DERIVE, 0);
	if (pms != NULL) {
	    PRINT_BUF(60, (ss, "decrypted premaster secret:",
			   PK11_GetKeyData(pms)->data,
			   PK11_GetKeyData(pms)->len));
	} else {
	    
	    PK11SlotInfo *  slot   = PK11_GetSlotFromPrivateKey(serverKey);

	    ssl_GetSpecWriteLock(ss);
	    pms = ssl3_GenerateRSAPMS(ss, ss->ssl3.prSpec, slot);
	    ssl_ReleaseSpecWriteLock(ss);
	    PK11_FreeSlot(slot);
	}

	if (pms == NULL) {
	    
	    ssl_MapLowLevelError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
	    return SECFailure;
	}

	
	rv = ssl3_InitPendingCipherSpec(ss,  pms);
	PK11_FreeSymKey(pms);
    }

    if (rv != SECSuccess) {
	SEND_ALERT
	return SECFailure; 
    }
    return SECSuccess;
}






static SECStatus
ssl3_HandleClientKeyExchange(sslSocket *ss, SSL3Opaque *b, PRUint32 length)
{
    SECKEYPrivateKey *serverKey         = NULL;
    SECStatus         rv;
    const ssl3KEADef *kea_def;
    ssl3KeyPair     *serverKeyPair      = NULL;
#ifndef NSS_DISABLE_ECC
    SECKEYPublicKey *serverPubKey       = NULL;
#endif 

    SSL_TRC(3, ("%d: SSL3[%d]: handle client_key_exchange handshake",
		SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    if (ss->ssl3.hs.ws != wait_client_key) {
	SSL3_SendAlert(ss, alert_fatal, unexpected_message);
    	PORT_SetError(SSL_ERROR_RX_UNEXPECTED_CLIENT_KEY_EXCH);
	return SECFailure;
    }

    kea_def   = ss->ssl3.hs.kea_def;

    if (ss->ssl3.hs.usedStepDownKey) {
	 PORT_Assert(kea_def->is_limited 
		 && kea_def->exchKeyType == kt_rsa 
		 && ss->stepDownKeyPair != NULL);
	 if (!kea_def->is_limited  ||
	      kea_def->exchKeyType != kt_rsa ||
	      ss->stepDownKeyPair == NULL) {
	 	
		goto skip;
	 }
    	serverKeyPair = ss->stepDownKeyPair;
	ss->sec.keaKeyBits = EXPORT_RSA_KEY_LENGTH * BPB;
    } else 
skip:
#ifndef NSS_DISABLE_ECC
    





    if ((kea_def->kea == kea_ecdhe_rsa) ||
               (kea_def->kea == kea_ecdhe_ecdsa)) {
	if (ss->ephemeralECDHKeyPair != NULL) {
	   serverKeyPair = ss->ephemeralECDHKeyPair;
	   if (serverKeyPair->pubKey) {
		ss->sec.keaKeyBits = 
		    SECKEY_PublicKeyStrengthInBits(serverKeyPair->pubKey);
	   }
	}
    } else 
#endif
    {
	sslServerCerts * sc = ss->serverCerts + kea_def->exchKeyType;
	serverKeyPair = sc->serverKeyPair;
	ss->sec.keaKeyBits = sc->serverKeyBits;
    }

    if (serverKeyPair) {
	serverKey = serverKeyPair->privKey;
    }

    if (serverKey == NULL) {
    	SEND_ALERT
	PORT_SetError(SSL_ERROR_NO_SERVER_KEY_FOR_ALG);
	return SECFailure;
    }

    ss->sec.keaType    = kea_def->exchKeyType;

    switch (kea_def->exchKeyType) {
    case kt_rsa:
	rv = ssl3_HandleRSAClientKeyExchange(ss, b, length, serverKey);
	if (rv != SECSuccess) {
	    SEND_ALERT
	    return SECFailure;	
	}
	break;


#ifndef NSS_DISABLE_ECC
    case kt_ecdh:
	





	if (serverKeyPair) {
	    serverPubKey = serverKeyPair->pubKey;
        }
	if (serverPubKey == NULL) {
	    
	    PORT_SetError(SSL_ERROR_EXTRACT_PUBLIC_KEY_FAILURE);
	    return SECFailure;
	}
	rv = ssl3_HandleECDHClientKeyExchange(ss, b, length, 
					      serverPubKey, serverKey);
	if (ss->ephemeralECDHKeyPair) {
	    ssl3_FreeKeyPair(ss->ephemeralECDHKeyPair);
	    ss->ephemeralECDHKeyPair = NULL;
	}
	if (rv != SECSuccess) {
	    return SECFailure;	
	}
	break;
#endif 

    default:
	(void) ssl3_HandshakeFailure(ss);
	PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
	return SECFailure;
    }
    ss->ssl3.hs.ws = ss->sec.peerCert ? wait_cert_verify : wait_change_cipher;
    return SECSuccess;

}


static SECStatus
ssl3_SendEmptyCertificate(sslSocket *ss)
{
    SECStatus            rv;

    rv = ssl3_AppendHandshakeHeader(ss, certificate, 3);
    if (rv == SECSuccess) {
	rv = ssl3_AppendHandshakeNumber(ss, 0, 3);
    }
    return rv;	
}

SECStatus
ssl3_HandleNewSessionTicket(sslSocket *ss, SSL3Opaque *b, PRUint32 length)
{
    SECStatus rv;
    SECItem ticketData;

    SSL_TRC(3, ("%d: SSL3[%d]: handle session_ticket handshake",
		SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    PORT_Assert(!ss->ssl3.hs.newSessionTicket.ticket.data);
    PORT_Assert(!ss->ssl3.hs.receivedNewSessionTicket);

    if (ss->ssl3.hs.ws != wait_new_session_ticket) {
	SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	PORT_SetError(SSL_ERROR_RX_UNEXPECTED_NEW_SESSION_TICKET);
	return SECFailure;
    }

    



    ss->ssl3.hs.newSessionTicket.received_timestamp = ssl_Time();
    if (length < 4) {
	(void)SSL3_SendAlert(ss, alert_fatal, decode_error);
	PORT_SetError(SSL_ERROR_RX_MALFORMED_NEW_SESSION_TICKET);
	return SECFailure;
    }
    ss->ssl3.hs.newSessionTicket.ticket_lifetime_hint =
	(PRUint32)ssl3_ConsumeHandshakeNumber(ss, 4, &b, &length);

    rv = ssl3_ConsumeHandshakeVariable(ss, &ticketData, 2, &b, &length);
    if (rv != SECSuccess || length != 0) {
	(void)SSL3_SendAlert(ss, alert_fatal, decode_error);
	PORT_SetError(SSL_ERROR_RX_MALFORMED_NEW_SESSION_TICKET);
	return SECFailure;  
    }
    

    if (ticketData.len != 0) {
	rv = SECITEM_CopyItem(NULL, &ss->ssl3.hs.newSessionTicket.ticket,
			      &ticketData);
	if (rv != SECSuccess) {
	    return rv;
	}
	ss->ssl3.hs.receivedNewSessionTicket = PR_TRUE;
    }

    ss->ssl3.hs.ws = wait_change_cipher;
    return SECSuccess;
}

#ifdef NISCC_TEST
static PRInt32 connNum = 0;

static SECStatus 
get_fake_cert(SECItem *pCertItem, int *pIndex)
{
    PRFileDesc *cf;
    char *      testdir;
    char *      startat;
    char *      stopat;
    const char *extension;
    int         fileNum;
    PRInt32     numBytes   = 0;
    PRStatus    prStatus;
    PRFileInfo  info;
    char        cfn[100];

    pCertItem->data = 0;
    if ((testdir = PR_GetEnv("NISCC_TEST")) == NULL) {
	return SECSuccess;
    }
    *pIndex   = (NULL != strstr(testdir, "root"));
    extension = (strstr(testdir, "simple") ? "" : ".der");
    fileNum     = PR_ATOMIC_INCREMENT(&connNum) - 1;
    if ((startat = PR_GetEnv("START_AT")) != NULL) {
	fileNum += atoi(startat);
    }
    if ((stopat = PR_GetEnv("STOP_AT")) != NULL && 
	fileNum >= atoi(stopat)) {
	*pIndex = -1;
	return SECSuccess;
    }
    sprintf(cfn, "%s/%08d%s", testdir, fileNum, extension);
    cf = PR_Open(cfn, PR_RDONLY, 0);
    if (!cf) {
	goto loser;
    }
    prStatus = PR_GetOpenFileInfo(cf, &info);
    if (prStatus != PR_SUCCESS) {
	PR_Close(cf);
	goto loser;
    }
    pCertItem = SECITEM_AllocItem(NULL, pCertItem, info.size);
    if (pCertItem) {
	numBytes = PR_Read(cf, pCertItem->data, info.size);
    }
    PR_Close(cf);
    if (numBytes != info.size) {
	SECITEM_FreeItem(pCertItem, PR_FALSE);
	PORT_SetError(SEC_ERROR_IO);
	goto loser;
    }
    fprintf(stderr, "using %s\n", cfn);
    return SECSuccess;

loser:
    fprintf(stderr, "failed to use %s\n", cfn);
    *pIndex = -1;
    return SECFailure;
}
#endif





static SECStatus
ssl3_SendCertificate(sslSocket *ss)
{
    SECStatus            rv;
    CERTCertificateList *certChain;
    int                  len 		= 0;
    int                  i;
    SSL3KEAType          certIndex;
#ifdef NISCC_TEST
    SECItem              fakeCert;
    int                  ndex           = -1;
#endif

    SSL_TRC(3, ("%d: SSL3[%d]: send certificate handshake",
		SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    if (ss->sec.localCert)
    	CERT_DestroyCertificate(ss->sec.localCert);
    if (ss->sec.isServer) {
	sslServerCerts * sc = NULL;

	







	if ((ss->ssl3.hs.kea_def->kea == kea_ecdhe_rsa) ||
	    (ss->ssl3.hs.kea_def->kea == kea_dhe_rsa)) {
	    certIndex = kt_rsa;
	} else {
	    certIndex = ss->ssl3.hs.kea_def->exchKeyType;
	}
	sc                    = ss->serverCerts + certIndex;
	certChain             = sc->serverCertChain;
	ss->sec.authKeyBits   = sc->serverKeyBits;
	ss->sec.authAlgorithm = ss->ssl3.hs.kea_def->signKeyType;
	ss->sec.localCert     = CERT_DupCertificate(sc->serverCert);
    } else {
	certChain          = ss->ssl3.clientCertChain;
	ss->sec.localCert = CERT_DupCertificate(ss->ssl3.clientCertificate);
    }

#ifdef NISCC_TEST
    rv = get_fake_cert(&fakeCert, &ndex);
#endif

    if (certChain) {
	for (i = 0; i < certChain->len; i++) {
#ifdef NISCC_TEST
	    if (fakeCert.len > 0 && i == ndex) {
		len += fakeCert.len + 3;
	    } else {
		len += certChain->certs[i].len + 3;
	    }
#else
	    len += certChain->certs[i].len + 3;
#endif
	}
    }

    rv = ssl3_AppendHandshakeHeader(ss, certificate, len + 3);
    if (rv != SECSuccess) {
	return rv; 		
    }
    rv = ssl3_AppendHandshakeNumber(ss, len, 3);
    if (rv != SECSuccess) {
	return rv; 		
    }
    if (certChain) {
        for (i = 0; i < certChain->len; i++) {
#ifdef NISCC_TEST
            if (fakeCert.len > 0 && i == ndex) {
                rv = ssl3_AppendHandshakeVariable(ss, fakeCert.data,
                                                  fakeCert.len, 3);
                SECITEM_FreeItem(&fakeCert, PR_FALSE);
            } else {
                rv = ssl3_AppendHandshakeVariable(ss, certChain->certs[i].data,
                                                  certChain->certs[i].len, 3);
            }
#else
            rv = ssl3_AppendHandshakeVariable(ss, certChain->certs[i].data,
                                              certChain->certs[i].len, 3);
#endif
            if (rv != SECSuccess) {
                return rv; 		
            }
        }
    }

    return SECSuccess;
}





static SECStatus
ssl3_SendCertificateStatus(sslSocket *ss)
{
    SECStatus rv;
    int len = 0;
    SECItemArray *statusToSend = NULL;
    SSL3KEAType certIndex;

    SSL_TRC(3, ("%d: SSL3[%d]: send certificate status handshake",
		SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));
    PORT_Assert( ss->sec.isServer);

    if (!ssl3_ExtensionNegotiated(ss, ssl_cert_status_xtn))
	return SECSuccess;

    
    if ((ss->ssl3.hs.kea_def->kea == kea_ecdhe_rsa) ||
	(ss->ssl3.hs.kea_def->kea == kea_dhe_rsa)) {
	certIndex = kt_rsa;
    } else {
	certIndex = ss->ssl3.hs.kea_def->exchKeyType;
    }
    if (ss->certStatusArray[certIndex] && ss->certStatusArray[certIndex]->len) {
	statusToSend = ss->certStatusArray[certIndex];
    }
    if (!statusToSend)
	return SECSuccess;

    
    len = 1 + statusToSend->items[0].len + 3;

    rv = ssl3_AppendHandshakeHeader(ss, certificate_status, len);
    if (rv != SECSuccess) {
	return rv; 		
    }
    rv = ssl3_AppendHandshakeNumber(ss, 1 , 1);
    if (rv != SECSuccess)
	return rv; 		

    rv = ssl3_AppendHandshakeVariable(ss,
				      statusToSend->items[0].data,
				      statusToSend->items[0].len,
				      3);
    if (rv != SECSuccess)
	return rv; 		

    return SECSuccess;
}




static void
ssl3_CleanupPeerCerts(sslSocket *ss)
{
    PLArenaPool * arena = ss->ssl3.peerCertArena;
    ssl3CertNode *certs = (ssl3CertNode *)ss->ssl3.peerCertChain;

    for (; certs; certs = certs->next) {
	CERT_DestroyCertificate(certs->cert);
    }
    if (arena) PORT_FreeArena(arena, PR_FALSE);
    ss->ssl3.peerCertArena = NULL;
    ss->ssl3.peerCertChain = NULL;
}







static SECStatus
ssl3_HandleCertificateStatus(sslSocket *ss, SSL3Opaque *b, PRUint32 length)
{
    PRInt32 status, len;

    if (ss->ssl3.hs.ws != wait_certificate_status) {
        (void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
        PORT_SetError(SSL_ERROR_RX_UNEXPECTED_CERT_STATUS);
        return SECFailure;
    }

    PORT_Assert(!ss->sec.isServer);

    
    status = ssl3_ConsumeHandshakeNumber(ss, 1, &b, &length);
    if (status != 1 ) {
       goto format_loser;
    }

    len = ssl3_ConsumeHandshakeNumber(ss, 3, &b, &length);
    if (len != length) {
       goto format_loser;
    }

#define MAX_CERTSTATUS_LEN 0x1ffff   /* 128k - 1 */
    if (length > MAX_CERTSTATUS_LEN)
       goto format_loser;
#undef MAX_CERTSTATUS_LEN

    
    SECITEM_AllocArray(NULL, &ss->sec.ci.sid->peerCertStatus, 1);
    if (!ss->sec.ci.sid->peerCertStatus.items)
       return SECFailure;

    ss->sec.ci.sid->peerCertStatus.items[0].data = PORT_Alloc(length);

    if (!ss->sec.ci.sid->peerCertStatus.items[0].data) {
        SECITEM_FreeArray(&ss->sec.ci.sid->peerCertStatus, PR_FALSE);
        return SECFailure;
    }

    PORT_Memcpy(ss->sec.ci.sid->peerCertStatus.items[0].data, b, length);
    ss->sec.ci.sid->peerCertStatus.items[0].len = length;
    ss->sec.ci.sid->peerCertStatus.items[0].type = siBuffer;

    return ssl3_AuthCertificate(ss);

format_loser:
    return ssl3_DecodeError(ss);
}





static SECStatus
ssl3_HandleCertificate(sslSocket *ss, SSL3Opaque *b, PRUint32 length)
{
    ssl3CertNode *   c;
    ssl3CertNode *   lastCert 	= NULL;
    PRInt32          remaining  = 0;
    PRInt32          size;
    SECStatus        rv;
    PRBool           isServer	= (PRBool)(!!ss->sec.isServer);
    PRBool           isTLS;
    SSL3AlertDescription desc;
    int              errCode    = SSL_ERROR_RX_MALFORMED_CERTIFICATE;
    SECItem          certItem;

    SSL_TRC(3, ("%d: SSL3[%d]: handle certificate handshake",
		SSL_GETPID(), ss->fd));
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    if ((isServer && ss->ssl3.hs.ws != wait_client_cert) ||
        (!isServer && ss->ssl3.hs.ws != wait_server_cert)) {
        desc = unexpected_message;
        errCode = SSL_ERROR_RX_UNEXPECTED_CERTIFICATE;
        goto alert_loser;
    }

    if (ss->sec.peerCert != NULL) {
	if (ss->sec.peerKey) {
	    SECKEY_DestroyPublicKey(ss->sec.peerKey);
	    ss->sec.peerKey = NULL;
	}
	CERT_DestroyCertificate(ss->sec.peerCert);
	ss->sec.peerCert = NULL;
    }

    ssl3_CleanupPeerCerts(ss);
    isTLS = (PRBool)(ss->ssl3.prSpec->version > SSL_LIBRARY_VERSION_3_0);

    



    if (length) {
	remaining = ssl3_ConsumeHandshakeNumber(ss, 3, &b, &length);
	if (remaining < 0)
	    goto loser;	
	if ((PRUint32)remaining > length)
	    goto decode_loser;
    }

    if (!remaining) {
	if (!(isTLS && isServer)) {
	    desc = bad_certificate;
	    goto alert_loser;
	}
    	
    	
	rv = ssl3_HandleNoCertificate(ss);
	if (rv != SECSuccess) {
	    errCode = PORT_GetError();
	    goto loser;
	}
       ss->ssl3.hs.ws = wait_client_key;
       return SECSuccess;
    }

    ss->ssl3.peerCertArena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (ss->ssl3.peerCertArena == NULL) {
	goto loser;	
    }

    
    remaining -= 3;
    if (remaining < 0)
	goto decode_loser;

    size = ssl3_ConsumeHandshakeNumber(ss, 3, &b, &length);
    if (size <= 0)
	goto loser;	

    if (remaining < size)
	goto decode_loser;

    certItem.data = b;
    certItem.len = size;
    b      += size;
    length -= size;
    remaining -= size;

    ss->sec.peerCert = CERT_NewTempCertificate(ss->dbHandle, &certItem, NULL,
                                            PR_FALSE, PR_TRUE);
    if (ss->sec.peerCert == NULL) {
	


	goto ambiguous_err;
    }

    
    while (remaining > 0) {
	remaining -= 3;
	if (remaining < 0)
	    goto decode_loser;

	size = ssl3_ConsumeHandshakeNumber(ss, 3, &b, &length);
	if (size <= 0)
	    goto loser;	

	if (remaining < size)
	    goto decode_loser;

	certItem.data = b;
	certItem.len = size;
	b      += size;
	length -= size;
	remaining -= size;

	c = PORT_ArenaNew(ss->ssl3.peerCertArena, ssl3CertNode);
	if (c == NULL) {
	    goto loser;	
	}

	c->cert = CERT_NewTempCertificate(ss->dbHandle, &certItem, NULL,
	                                  PR_FALSE, PR_TRUE);
	if (c->cert == NULL) {
	    goto ambiguous_err;
	}

	c->next = NULL;
	if (lastCert) {
	    lastCert->next = c;
	} else {
	    ss->ssl3.peerCertChain = c;
	}
	lastCert = c;
    }

    if (remaining != 0)
        goto decode_loser;

    SECKEY_UpdateCertPQG(ss->sec.peerCert);

    if (!isServer && ssl3_ExtensionNegotiated(ss, ssl_cert_status_xtn)) {
       ss->ssl3.hs.ws = wait_certificate_status;
       rv = SECSuccess;
    } else {
       rv = ssl3_AuthCertificate(ss); 
    }

    return rv;

ambiguous_err:
    errCode = PORT_GetError();
    switch (errCode) {
    case PR_OUT_OF_MEMORY_ERROR:
    case SEC_ERROR_BAD_DATABASE:
    case SEC_ERROR_NO_MEMORY:
       if (isTLS) {
           desc = internal_error;
           goto alert_loser;
       }
       goto loser;
    }
    ssl3_SendAlertForCertError(ss, errCode);
    goto loser;

decode_loser:
    desc = isTLS ? decode_error : bad_certificate;

alert_loser:
    (void)SSL3_SendAlert(ss, alert_fatal, desc);

loser:
    (void)ssl_MapLowLevelError(errCode);
    return SECFailure;
}

static SECStatus
ssl3_AuthCertificate(sslSocket *ss)
{
    SECStatus        rv;
    PRBool           isServer   = (PRBool)(!!ss->sec.isServer);
    int              errCode;

    ss->ssl3.hs.authCertificatePending = PR_FALSE;

    


    rv = (SECStatus)(*ss->authCertificate)(ss->authCertificateArg, ss->fd,
					   PR_TRUE, isServer);
    if (rv) {
	errCode = PORT_GetError();
	if (rv != SECWouldBlock) {
	    if (ss->handleBadCert) {
		rv = (*ss->handleBadCert)(ss->badCertArg, ss->fd);
	    }
	}

	if (rv == SECWouldBlock) {
	    if (ss->sec.isServer) {
		errCode = SSL_ERROR_FEATURE_NOT_SUPPORTED_FOR_SERVERS;
		rv = SECFailure;
		goto loser;
	    }

	    ss->ssl3.hs.authCertificatePending = PR_TRUE;
	    rv = SECSuccess;
	}

	if (rv != SECSuccess) {
	    ssl3_SendAlertForCertError(ss, errCode);
	    goto loser;
	}
    }

    ss->sec.ci.sid->peerCert = CERT_DupCertificate(ss->sec.peerCert);

    if (!ss->sec.isServer) {
        CERTCertificate *cert = ss->sec.peerCert;

	



	SECKEYPublicKey * pubKey  = CERT_ExtractPublicKey(cert);
	ss->sec.authAlgorithm = ss->ssl3.hs.kea_def->signKeyType;
	ss->sec.keaType       = ss->ssl3.hs.kea_def->exchKeyType;
	if (pubKey) {
	    ss->sec.keaKeyBits = ss->sec.authKeyBits =
		SECKEY_PublicKeyStrengthInBits(pubKey);
#ifndef NSS_DISABLE_ECC
	    if (ss->sec.keaType == kt_ecdh) {
		






		if (ss->ssl3.hs.kea_def->kea == kea_ecdh_ecdsa) {
		    ss->sec.authKeyBits = 
			cert->signatureWrap.signature.data[3]*8;
		    if (cert->signatureWrap.signature.data[4] == 0x00)
			    ss->sec.authKeyBits -= 8;
		    



		} else if (ss->ssl3.hs.kea_def->kea == kea_ecdh_rsa) {
		    ss->sec.authKeyBits = cert->signatureWrap.signature.len;
		    



		}
	    }
#endif 
	    SECKEY_DestroyPublicKey(pubKey); 
	    pubKey = NULL;
    	}

        if (ss->ssl3.hs.kea_def->ephemeral) {
            ss->ssl3.hs.ws = wait_server_key; 
        } else {
            ss->ssl3.hs.ws = wait_cert_request; 
        }
    } else {
	ss->ssl3.hs.ws = wait_client_key;
    }

    PORT_Assert(rv == SECSuccess);
    if (rv != SECSuccess) {
	errCode = SEC_ERROR_LIBRARY_FAILURE;
	rv = SECFailure;
	goto loser;
    }

    return rv;

loser:
    (void)ssl_MapLowLevelError(errCode);
    return SECFailure;
}

static SECStatus ssl3_FinishHandshake(sslSocket *ss);

static SECStatus
ssl3_AlwaysFail(sslSocket * ss)
{
    PORT_SetError(PR_INVALID_STATE_ERROR);
    return SECFailure;
}



SECStatus
ssl3_AuthCertificateComplete(sslSocket *ss, PRErrorCode error)
{
    SECStatus rv;

    PORT_Assert(ss->opt.noLocks || ssl_Have1stHandshakeLock(ss));

    if (ss->sec.isServer) {
	PORT_SetError(SSL_ERROR_FEATURE_NOT_SUPPORTED_FOR_SERVERS);
	return SECFailure;
    }

    ssl_GetRecvBufLock(ss);
    ssl_GetSSL3HandshakeLock(ss);

    if (!ss->ssl3.hs.authCertificatePending) {
	PORT_SetError(PR_INVALID_STATE_ERROR);
	rv = SECFailure;
	goto done;
    }

    ss->ssl3.hs.authCertificatePending = PR_FALSE;

    if (error != 0) {
	ss->ssl3.hs.restartTarget = ssl3_AlwaysFail;
	ssl3_SendAlertForCertError(ss, error);
	rv = SECSuccess;
    } else if (ss->ssl3.hs.restartTarget != NULL) {
	sslRestartTarget target = ss->ssl3.hs.restartTarget;
	ss->ssl3.hs.restartTarget = NULL;

	if (target == ssl3_FinishHandshake) {
	    SSL_TRC(3,("%d: SSL3[%p]: certificate authentication lost the race"
		       " with peer's finished message", SSL_GETPID(), ss->fd));
	}

	rv = target(ss);
	



	if (rv == SECWouldBlock) {
	    rv = SECSuccess;
	}
    } else {
	SSL_TRC(3, ("%d: SSL3[%p]: certificate authentication won the race with"
        	    " peer's finished message", SSL_GETPID(), ss->fd));

	PORT_Assert(!ss->ssl3.hs.isResuming);
	PORT_Assert(ss->ssl3.hs.ws != idle_handshake);

	if (ss->opt.enableFalseStart &&
	    !ss->firstHsDone &&
	    !ss->ssl3.hs.isResuming &&
	    ssl3_WaitingForStartOfServerSecondRound(ss)) {
	    



	    rv = ssl3_CheckFalseStart(ss);
	} else {
	    rv = SECSuccess;
	}
    }

done:
    ssl_ReleaseSSL3HandshakeLock(ss);
    ssl_ReleaseRecvBufLock(ss);

    return rv;
}

static SECStatus
ssl3_ComputeTLSFinished(ssl3CipherSpec *spec,
			PRBool          isServer,
                const   SSL3Hashes   *  hashes,
                        TLSFinished  *  tlsFinished)
{
    const char * label;
    unsigned int len;
    SECStatus    rv;

    label = isServer ? "server finished" : "client finished";
    len   = 15;

    rv = ssl3_TLSPRFWithMasterSecret(spec, label, len, hashes->u.raw,
	hashes->len, tlsFinished->verify_data,
	sizeof tlsFinished->verify_data);

    return rv;
}





SECStatus
ssl3_TLSPRFWithMasterSecret(ssl3CipherSpec *spec, const char *label,
    unsigned int labelLen, const unsigned char *val, unsigned int valLen,
    unsigned char *out, unsigned int outLen)
{
    SECStatus rv = SECSuccess;

    if (spec->master_secret && !spec->bypassCiphers) {
	SECItem param = {siBuffer, NULL, 0};
	CK_MECHANISM_TYPE mech = CKM_TLS_PRF_GENERAL;
	PK11Context *prf_context;
	unsigned int retLen;

	if (spec->version >= SSL_LIBRARY_VERSION_TLS_1_2) {
	    mech = CKM_NSS_TLS_PRF_GENERAL_SHA256;
	}
	prf_context = PK11_CreateContextBySymKey(mech, CKA_SIGN,
						 spec->master_secret, &param);
	if (!prf_context)
	    return SECFailure;

	rv  = PK11_DigestBegin(prf_context);
	rv |= PK11_DigestOp(prf_context, (unsigned char *) label, labelLen);
	rv |= PK11_DigestOp(prf_context, val, valLen);
	rv |= PK11_DigestFinal(prf_context, out, &retLen, outLen);
	PORT_Assert(rv != SECSuccess || retLen == outLen);

	PK11_DestroyContext(prf_context, PR_TRUE);
    } else {
	
#ifdef NO_PKCS11_BYPASS
	PORT_Assert(spec->master_secret);
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	rv = SECFailure;
#else
	SECItem inData  = { siBuffer, };
	SECItem outData = { siBuffer, };
	PRBool isFIPS   = PR_FALSE;

	inData.data  = (unsigned char *) val;
	inData.len   = valLen;
	outData.data = out;
	outData.len  = outLen;
	if (spec->version >= SSL_LIBRARY_VERSION_TLS_1_2) {
	    rv = TLS_P_hash(HASH_AlgSHA256, &spec->msItem, label, &inData,
			    &outData, isFIPS);
	} else {
	    rv = TLS_PRF(&spec->msItem, label, &inData, &outData, isFIPS);
	}
	PORT_Assert(rv != SECSuccess || outData.len == outLen);
#endif
    }
    return rv;
}




static SECStatus
ssl3_SendNextProto(sslSocket *ss)
{
    SECStatus rv;
    int padding_len;
    static const unsigned char padding[32] = {0};

    if (ss->ssl3.nextProto.len == 0 ||
	ss->ssl3.nextProtoState == SSL_NEXT_PROTO_SELECTED) {
	return SECSuccess;
    }

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    padding_len = 32 - ((ss->ssl3.nextProto.len + 2) % 32);

    rv = ssl3_AppendHandshakeHeader(ss, next_proto, ss->ssl3.nextProto.len +
						    2 + padding_len);
    if (rv != SECSuccess) {
	return rv;	
    }
    rv = ssl3_AppendHandshakeVariable(ss, ss->ssl3.nextProto.data,
				      ss->ssl3.nextProto.len, 1);
    if (rv != SECSuccess) {
	return rv;	
    }
    rv = ssl3_AppendHandshakeVariable(ss, padding, padding_len, 1);
    if (rv != SECSuccess) {
	return rv;	
    }
    return rv;
}





static void
ssl3_RecordKeyLog(sslSocket *ss)
{
    SECStatus rv;
    SECItem *keyData;
    char buf[14  +
	     SSL3_RANDOM_LENGTH*2  +
	     1  +
	     48*2  +
             1 ];
    unsigned int j;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    if (!ssl_keylog_iob)
	return;

    rv = PK11_ExtractKeyValue(ss->ssl3.cwSpec->master_secret);
    if (rv != SECSuccess)
	return;

    ssl_GetSpecReadLock(ss);

    
    keyData = PK11_GetKeyData(ss->ssl3.cwSpec->master_secret);
    if (!keyData || !keyData->data || keyData->len != 48) {
	ssl_ReleaseSpecReadLock(ss);
	return;
    }

    

    



    memcpy(buf, "CLIENT_RANDOM ", 14);
    j = 14;
    hexEncode(buf + j, ss->ssl3.hs.client_random.rand, SSL3_RANDOM_LENGTH);
    j += SSL3_RANDOM_LENGTH*2;
    buf[j++] = ' ';
    hexEncode(buf + j, keyData->data, 48);
    j += 48*2;
    buf[j++] = '\n';

    PORT_Assert(j == sizeof(buf));

    ssl_ReleaseSpecReadLock(ss);

    if (fwrite(buf, sizeof(buf), 1, ssl_keylog_iob) != 1)
        return;
    fflush(ssl_keylog_iob);
    return;
}





static SECStatus
ssl3_SendFinished(sslSocket *ss, PRInt32 flags)
{
    ssl3CipherSpec *cwSpec;
    PRBool          isTLS;
    PRBool          isServer = ss->sec.isServer;
    SECStatus       rv;
    SSL3Sender      sender = isServer ? sender_server : sender_client;
    SSL3Hashes      hashes;
    TLSFinished     tlsFinished;

    SSL_TRC(3, ("%d: SSL3[%d]: send finished handshake", SSL_GETPID(), ss->fd));

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    ssl_GetSpecReadLock(ss);
    cwSpec = ss->ssl3.cwSpec;
    isTLS = (PRBool)(cwSpec->version > SSL_LIBRARY_VERSION_3_0);
    rv = ssl3_ComputeHandshakeHashes(ss, cwSpec, &hashes, sender);
    if (isTLS && rv == SECSuccess) {
	rv = ssl3_ComputeTLSFinished(cwSpec, isServer, &hashes, &tlsFinished);
    }
    ssl_ReleaseSpecReadLock(ss);
    if (rv != SECSuccess) {
	goto fail;	
    }

    if (isTLS) {
	if (isServer)
	    ss->ssl3.hs.finishedMsgs.tFinished[1] = tlsFinished;
	else
	    ss->ssl3.hs.finishedMsgs.tFinished[0] = tlsFinished;
	ss->ssl3.hs.finishedBytes = sizeof tlsFinished;
	rv = ssl3_AppendHandshakeHeader(ss, finished, sizeof tlsFinished);
	if (rv != SECSuccess) 
	    goto fail; 		
	rv = ssl3_AppendHandshake(ss, &tlsFinished, sizeof tlsFinished);
	if (rv != SECSuccess) 
	    goto fail; 		
    } else {
	if (isServer)
	    ss->ssl3.hs.finishedMsgs.sFinished[1] = hashes.u.s;
	else
	    ss->ssl3.hs.finishedMsgs.sFinished[0] = hashes.u.s;
	PORT_Assert(hashes.len == sizeof hashes.u.s);
	ss->ssl3.hs.finishedBytes = sizeof hashes.u.s;
	rv = ssl3_AppendHandshakeHeader(ss, finished, sizeof hashes.u.s);
	if (rv != SECSuccess) 
	    goto fail; 		
	rv = ssl3_AppendHandshake(ss, &hashes.u.s, sizeof hashes.u.s);
	if (rv != SECSuccess) 
	    goto fail; 		
    }
    rv = ssl3_FlushHandshake(ss, flags);
    if (rv != SECSuccess) {
	goto fail;	
    }

    ssl3_RecordKeyLog(ss);

    return SECSuccess;

fail:
    return rv;
}




SECStatus
ssl3_CacheWrappedMasterSecret(sslSocket *ss, sslSessionID *sid,
    ssl3CipherSpec *spec, SSL3KEAType effectiveExchKeyType)
{
    PK11SymKey *      wrappingKey  = NULL;
    PK11SlotInfo *    symKeySlot;
    void *            pwArg        = ss->pkcs11PinArg;
    SECStatus         rv           = SECFailure;
    PRBool            isServer     = ss->sec.isServer;
    CK_MECHANISM_TYPE mechanism    = CKM_INVALID_MECHANISM;
    symKeySlot = PK11_GetSlotFromKey(spec->master_secret);
    if (!isServer) {
	int  wrapKeyIndex;
	int  incarnation;

	
	sid->u.ssl3.masterWrapIndex  = wrapKeyIndex =
				       PK11_GetCurrentWrapIndex(symKeySlot);
	PORT_Assert(wrapKeyIndex == 0);	

	sid->u.ssl3.masterWrapSeries = incarnation =
				       PK11_GetSlotSeries(symKeySlot);
	sid->u.ssl3.masterSlotID   = PK11_GetSlotID(symKeySlot);
	sid->u.ssl3.masterModuleID = PK11_GetModuleID(symKeySlot);
	sid->u.ssl3.masterValid    = PR_TRUE;
	

	wrappingKey = PK11_GetWrapKey(symKeySlot, wrapKeyIndex,
				      CKM_INVALID_MECHANISM, incarnation,
				      pwArg);
	if (wrappingKey) {
	    mechanism = PK11_GetMechanism(wrappingKey); 
	} else {
	    int keyLength;
	    




	    mechanism = PK11_GetBestWrapMechanism(symKeySlot);
	    keyLength = PK11_GetBestKeyLength(symKeySlot, mechanism);
	    


	    wrappingKey = PK11_KeyGen(symKeySlot, mechanism, NULL,
				      keyLength, pwArg);
	    if (wrappingKey) {
		PK11_SetWrapKey(symKeySlot, wrapKeyIndex, wrappingKey);
	    }
	}
    } else {
	
	mechanism = PK11_GetBestWrapMechanism(symKeySlot);
	if (mechanism != CKM_INVALID_MECHANISM) {
	    wrappingKey =
		getWrappingKey(ss, symKeySlot, effectiveExchKeyType,
			       mechanism, pwArg);
	    if (wrappingKey) {
		mechanism = PK11_GetMechanism(wrappingKey); 
	    }
	}
    }

    sid->u.ssl3.masterWrapMech = mechanism;
    PK11_FreeSlot(symKeySlot);

    if (wrappingKey) {
	SECItem wmsItem;

	wmsItem.data = sid->u.ssl3.keys.wrapped_master_secret;
	wmsItem.len  = sizeof sid->u.ssl3.keys.wrapped_master_secret;
	rv = PK11_WrapSymKey(mechanism, NULL, wrappingKey,
			     spec->master_secret, &wmsItem);
	
	sid->u.ssl3.keys.wrapped_master_secret_len = wmsItem.len;
	PK11_FreeSymKey(wrappingKey);
    }
    return rv;
}





static SECStatus
ssl3_HandleFinished(sslSocket *ss, SSL3Opaque *b, PRUint32 length,
		    const SSL3Hashes *hashes)
{
    sslSessionID *    sid	   = ss->sec.ci.sid;
    SECStatus         rv           = SECSuccess;
    PRBool            isServer     = ss->sec.isServer;
    PRBool            isTLS;
    SSL3KEAType       effectiveExchKeyType;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    SSL_TRC(3, ("%d: SSL3[%d]: handle finished handshake",
    	SSL_GETPID(), ss->fd));

    if (ss->ssl3.hs.ws != wait_finished) {
	SSL3_SendAlert(ss, alert_fatal, unexpected_message);
    	PORT_SetError(SSL_ERROR_RX_UNEXPECTED_FINISHED);
	return SECFailure;
    }

    isTLS = (PRBool)(ss->ssl3.crSpec->version > SSL_LIBRARY_VERSION_3_0);
    if (isTLS) {
	TLSFinished tlsFinished;

	if (length != sizeof tlsFinished) {
	    (void)SSL3_SendAlert(ss, alert_fatal, decode_error);
	    PORT_SetError(SSL_ERROR_RX_MALFORMED_FINISHED);
	    return SECFailure;
	}
	rv = ssl3_ComputeTLSFinished(ss->ssl3.crSpec, !isServer, 
	                             hashes, &tlsFinished);
	if (!isServer)
	    ss->ssl3.hs.finishedMsgs.tFinished[1] = tlsFinished;
	else
	    ss->ssl3.hs.finishedMsgs.tFinished[0] = tlsFinished;
	ss->ssl3.hs.finishedBytes = sizeof tlsFinished;
	if (rv != SECSuccess ||
	    0 != NSS_SecureMemcmp(&tlsFinished, b, length)) {
	    (void)SSL3_SendAlert(ss, alert_fatal, decrypt_error);
	    PORT_SetError(SSL_ERROR_BAD_HANDSHAKE_HASH_VALUE);
	    return SECFailure;
	}
    } else {
	if (length != sizeof(SSL3Finished)) {
	    (void)ssl3_IllegalParameter(ss);
	    PORT_SetError(SSL_ERROR_RX_MALFORMED_FINISHED);
	    return SECFailure;
	}

	if (!isServer)
	    ss->ssl3.hs.finishedMsgs.sFinished[1] = hashes->u.s;
	else
	    ss->ssl3.hs.finishedMsgs.sFinished[0] = hashes->u.s;
	PORT_Assert(hashes->len == sizeof hashes->u.s);
	ss->ssl3.hs.finishedBytes = sizeof hashes->u.s;
	if (0 != NSS_SecureMemcmp(&hashes->u.s, b, length)) {
	    (void)ssl3_HandshakeFailure(ss);
	    PORT_SetError(SSL_ERROR_BAD_HANDSHAKE_HASH_VALUE);
	    return SECFailure;
	}
    }

    ssl_GetXmitBufLock(ss);	

    if ((isServer && !ss->ssl3.hs.isResuming) ||
	(!isServer && ss->ssl3.hs.isResuming)) {
	PRInt32 flags = 0;

	





	if (isServer && !ss->ssl3.hs.isResuming &&
	    ssl3_ExtensionNegotiated(ss, ssl_session_ticket_xtn)) {
	    




	    rv = ssl3_SendNewSessionTicket(ss);
	    if (rv != SECSuccess) {
		goto xmit_loser;
	    }
	}

	rv = ssl3_SendChangeCipherSpecs(ss);
	if (rv != SECSuccess) {
	    goto xmit_loser;	
	}
	




	if (ss->writerThread == PR_GetCurrentThread()) {
	    flags = ssl_SEND_FLAG_FORCE_INTO_BUFFER;
	}

	if (!isServer && !ss->firstHsDone) {
	    rv = ssl3_SendNextProto(ss);
	    if (rv != SECSuccess) {
		goto xmit_loser; 
	    }
	}

	if (IS_DTLS(ss)) {
	    flags |= ssl_SEND_FLAG_NO_RETRANSMIT;
	}

	rv = ssl3_SendFinished(ss, flags);
	if (rv != SECSuccess) {
	    goto xmit_loser;	
	}
    }

xmit_loser:
    ssl_ReleaseXmitBufLock(ss);	
    if (rv != SECSuccess) {
        return rv;
    }

    if (ss->ssl3.hs.kea_def->kea == kea_ecdhe_rsa) {
	effectiveExchKeyType = kt_rsa;
    } else {
	effectiveExchKeyType = ss->ssl3.hs.kea_def->exchKeyType;
    }

    if (sid->cached == never_cached && !ss->opt.noCache && ss->sec.cache) {
	
	sid->u.ssl3.cipherSuite = ss->ssl3.hs.cipher_suite;
	sid->u.ssl3.compression = ss->ssl3.hs.compression;
	sid->u.ssl3.policy      = ss->ssl3.policy;
#ifndef NSS_DISABLE_ECC
	sid->u.ssl3.negotiatedECCurves = ss->ssl3.hs.negotiatedECCurves;
#endif
	sid->u.ssl3.exchKeyType = effectiveExchKeyType;
	sid->version            = ss->version;
	sid->authAlgorithm      = ss->sec.authAlgorithm;
	sid->authKeyBits        = ss->sec.authKeyBits;
	sid->keaType            = ss->sec.keaType;
	sid->keaKeyBits         = ss->sec.keaKeyBits;
	sid->lastAccessTime     = sid->creationTime = ssl_Time();
	sid->expirationTime     = sid->creationTime + ssl3_sid_timeout;
	sid->localCert          = CERT_DupCertificate(ss->sec.localCert);

	ssl_GetSpecReadLock(ss);	

	
	if (ss->ssl3.crSpec->msItem.len && ss->ssl3.crSpec->msItem.data) {
	    sid->u.ssl3.keys.wrapped_master_secret_len = 
			    ss->ssl3.crSpec->msItem.len;
	    memcpy(sid->u.ssl3.keys.wrapped_master_secret, 
		   ss->ssl3.crSpec->msItem.data, ss->ssl3.crSpec->msItem.len);
	    sid->u.ssl3.masterValid    = PR_TRUE;
	    sid->u.ssl3.keys.msIsWrapped = PR_FALSE;
	    rv = SECSuccess;
	} else {
	    rv = ssl3_CacheWrappedMasterSecret(ss, ss->sec.ci.sid,
					       ss->ssl3.crSpec,
					       effectiveExchKeyType);
	    sid->u.ssl3.keys.msIsWrapped = PR_TRUE;
	}
	ssl_ReleaseSpecReadLock(ss);  

	


	ss->ssl3.hs.cacheSID = rv == SECSuccess;
    }

    if (ss->ssl3.hs.authCertificatePending) {
	if (ss->ssl3.hs.restartTarget) {
	    PR_NOT_REACHED("ssl3_HandleFinished: unexpected restartTarget");
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return SECFailure;
	}

	ss->ssl3.hs.restartTarget = ssl3_FinishHandshake;
	return SECWouldBlock;
    }

    rv = ssl3_FinishHandshake(ss);
    return rv;
}




SECStatus
ssl3_FinishHandshake(sslSocket * ss)
{
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( ss->ssl3.hs.restartTarget == NULL );

    
    ss->handshake           = NULL;

    










    if (ss->ssl3.hs.receivedNewSessionTicket) {
	PORT_Assert(!ss->sec.isServer);
	ssl3_SetSIDSessionTicket(ss->sec.ci.sid, &ss->ssl3.hs.newSessionTicket);
	
	PORT_Assert(!ss->ssl3.hs.newSessionTicket.ticket.data);
        ss->ssl3.hs.receivedNewSessionTicket = PR_FALSE;
    }

    if (ss->ssl3.hs.cacheSID) {
	PORT_Assert(ss->sec.ci.sid->cached == never_cached);
	(*ss->sec.cache)(ss->sec.ci.sid);
	ss->ssl3.hs.cacheSID = PR_FALSE;
    }

    ss->ssl3.hs.canFalseStart = PR_FALSE; 
    ss->ssl3.hs.ws = idle_handshake;

    ssl_FinishHandshake(ss);

    return SECSuccess;
}





SECStatus
ssl3_HandleHandshakeMessage(sslSocket *ss, SSL3Opaque *b, PRUint32 length)
{
    SECStatus         rv 	= SECSuccess;
    SSL3HandshakeType type 	= ss->ssl3.hs.msg_type;
    SSL3Hashes        hashes;	
    PRUint8           hdr[4];
    PRUint8           dtlsData[8];

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    



    ssl_GetSpecReadLock(ss);	
    if((type == finished) || (type == certificate_verify)) {
	SSL3Sender      sender = (SSL3Sender)0;
	ssl3CipherSpec *rSpec  = ss->ssl3.prSpec;

	if (type == finished) {
	    sender = ss->sec.isServer ? sender_client : sender_server;
	    rSpec  = ss->ssl3.crSpec;
	}
	rv = ssl3_ComputeHandshakeHashes(ss, rSpec, &hashes, sender);
    }
    ssl_ReleaseSpecReadLock(ss); 
    if (rv != SECSuccess) {
	return rv;	
    }
    SSL_TRC(30,("%d: SSL3[%d]: handle handshake message: %s", SSL_GETPID(),
		ss->fd, ssl3_DecodeHandshakeType(ss->ssl3.hs.msg_type)));

    hdr[0] = (PRUint8)ss->ssl3.hs.msg_type;
    hdr[1] = (PRUint8)(length >> 16);
    hdr[2] = (PRUint8)(length >>  8);
    hdr[3] = (PRUint8)(length      );

    
    if (ss->ssl3.hs.msg_type == client_hello) {
	rv = ssl3_RestartHandshakeHashes(ss);
	if (rv != SECSuccess) {
	    return rv;
	}
    }
    

    if ((ss->ssl3.hs.msg_type != hello_request) &&
	(ss->ssl3.hs.msg_type != hello_verify_request)) {
	rv = ssl3_UpdateHandshakeHashes(ss, (unsigned char*) hdr, 4);
	if (rv != SECSuccess) return rv;	

	
	if (IS_DTLS(ss)) {
	    
	    dtlsData[0] = MSB(ss->ssl3.hs.recvMessageSeq);
	    dtlsData[1] = LSB(ss->ssl3.hs.recvMessageSeq);

	    
	    dtlsData[2] = 0;
	    dtlsData[3] = 0;
	    dtlsData[4] = 0;

	    
	    dtlsData[5] = (PRUint8)(length >> 16);
	    dtlsData[6] = (PRUint8)(length >>  8);
	    dtlsData[7] = (PRUint8)(length      );

	    rv = ssl3_UpdateHandshakeHashes(ss, (unsigned char*) dtlsData,
					    sizeof(dtlsData));
	    if (rv != SECSuccess) return rv;	
	}

	
	rv = ssl3_UpdateHandshakeHashes(ss, b, length);
	if (rv != SECSuccess) return rv;	
    }

    PORT_SetError(0);	

    if (ss->ssl3.hs.ws == wait_certificate_status &&
        ss->ssl3.hs.msg_type != certificate_status) {
        






        rv = ssl3_AuthCertificate(ss); 
        PORT_Assert(rv != SECWouldBlock);
        if (rv != SECSuccess) {
            return rv;
        }
    }

    switch (ss->ssl3.hs.msg_type) {
    case hello_request:
	if (length != 0) {
	    (void)ssl3_DecodeError(ss);
	    PORT_SetError(SSL_ERROR_RX_MALFORMED_HELLO_REQUEST);
	    return SECFailure;
	}
	if (ss->sec.isServer) {
	    (void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_HELLO_REQUEST);
	    return SECFailure;
	}
	rv = ssl3_HandleHelloRequest(ss);
	break;
    case client_hello:
	if (!ss->sec.isServer) {
	    (void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_CLIENT_HELLO);
	    return SECFailure;
	}
	rv = ssl3_HandleClientHello(ss, b, length);
	break;
    case server_hello:
	if (ss->sec.isServer) {
	    (void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_SERVER_HELLO);
	    return SECFailure;
	}
	rv = ssl3_HandleServerHello(ss, b, length);
	break;
    case hello_verify_request:
	if (!IS_DTLS(ss) || ss->sec.isServer) {
	    (void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_HELLO_VERIFY_REQUEST);
	    return SECFailure;
	}
	rv = dtls_HandleHelloVerifyRequest(ss, b, length);
	break;
    case certificate:
	rv = ssl3_HandleCertificate(ss, b, length);
	break;
    case certificate_status:
	rv = ssl3_HandleCertificateStatus(ss, b, length);
	break;
    case server_key_exchange:
	if (ss->sec.isServer) {
	    (void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_SERVER_KEY_EXCH);
	    return SECFailure;
	}
	rv = ssl3_HandleServerKeyExchange(ss, b, length);
	break;
    case certificate_request:
	if (ss->sec.isServer) {
	    (void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_CERT_REQUEST);
	    return SECFailure;
	}
	rv = ssl3_HandleCertificateRequest(ss, b, length);
	break;
    case server_hello_done:
	if (length != 0) {
	    (void)ssl3_DecodeError(ss);
	    PORT_SetError(SSL_ERROR_RX_MALFORMED_HELLO_DONE);
	    return SECFailure;
	}
	if (ss->sec.isServer) {
	    (void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_HELLO_DONE);
	    return SECFailure;
	}
	rv = ssl3_HandleServerHelloDone(ss);
	break;
    case certificate_verify:
	if (!ss->sec.isServer) {
	    (void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_CERT_VERIFY);
	    return SECFailure;
	}
	rv = ssl3_HandleCertificateVerify(ss, b, length, &hashes);
	break;
    case client_key_exchange:
	if (!ss->sec.isServer) {
	    (void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_CLIENT_KEY_EXCH);
	    return SECFailure;
	}
	rv = ssl3_HandleClientKeyExchange(ss, b, length);
	break;
    case new_session_ticket:
	if (ss->sec.isServer) {
	    (void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	    PORT_SetError(SSL_ERROR_RX_UNEXPECTED_NEW_SESSION_TICKET);
	    return SECFailure;
	}
	rv = ssl3_HandleNewSessionTicket(ss, b, length);
	break;
    case finished:
        rv = ssl3_HandleFinished(ss, b, length, &hashes);
	break;
    default:
	(void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	PORT_SetError(SSL_ERROR_RX_UNKNOWN_HANDSHAKE);
	rv = SECFailure;
    }

    if (IS_DTLS(ss) && (rv != SECFailure)) {
	
	ss->ssl3.hs.recvMessageSeq++;
    }

    return rv;
}





static SECStatus
ssl3_HandleHandshake(sslSocket *ss, sslBuffer *origBuf)
{
    







    sslBuffer *buf = &ss->ssl3.hs.msgState; 
    SECStatus rv;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    if (buf->buf == NULL) {
	*buf = *origBuf;
    }
    while (buf->len > 0) {
	if (ss->ssl3.hs.header_bytes < 4) {
	    PRUint8 t;
	    t = *(buf->buf++);
	    buf->len--;
	    if (ss->ssl3.hs.header_bytes++ == 0)
		ss->ssl3.hs.msg_type = (SSL3HandshakeType)t;
	    else
		ss->ssl3.hs.msg_len = (ss->ssl3.hs.msg_len << 8) + t;
	    if (ss->ssl3.hs.header_bytes < 4)
	    	continue;

#define MAX_HANDSHAKE_MSG_LEN 0x1ffff	/* 128k - 1 */
	    if (ss->ssl3.hs.msg_len > MAX_HANDSHAKE_MSG_LEN) {
		(void)ssl3_DecodeError(ss);
		PORT_SetError(SSL_ERROR_RX_RECORD_TOO_LONG);
		return SECFailure;
	    }
#undef MAX_HANDSHAKE_MSG_LEN

	    


	    if (ss->ssl3.hs.msg_len > 0) 
	    	continue;
	}

	




	if (ss->ssl3.hs.msg_body.len == 0 && buf->len >= ss->ssl3.hs.msg_len) {
	    
	    rv = ssl3_HandleHandshakeMessage(ss, buf->buf, ss->ssl3.hs.msg_len);
	    if (rv == SECFailure) {
		



		return rv;
	    }
	    buf->buf += ss->ssl3.hs.msg_len;
	    buf->len -= ss->ssl3.hs.msg_len;
	    ss->ssl3.hs.msg_len = 0;
	    ss->ssl3.hs.header_bytes = 0;
	    if (rv != SECSuccess) { 
		return rv;
	    }
	} else {
	    
	    unsigned int bytes;

	    PORT_Assert(ss->ssl3.hs.msg_body.len < ss->ssl3.hs.msg_len);
	    bytes = PR_MIN(buf->len, ss->ssl3.hs.msg_len - ss->ssl3.hs.msg_body.len);

	    
	    rv = sslBuffer_Grow(&ss->ssl3.hs.msg_body, ss->ssl3.hs.msg_len);
	    if (rv != SECSuccess) {
		
		return SECFailure;
	    }

	    PORT_Memcpy(ss->ssl3.hs.msg_body.buf + ss->ssl3.hs.msg_body.len,
		        buf->buf, bytes);
	    ss->ssl3.hs.msg_body.len += bytes;
	    buf->buf += bytes;
	    buf->len -= bytes;

	    PORT_Assert(ss->ssl3.hs.msg_body.len <= ss->ssl3.hs.msg_len);

	    
	    if (ss->ssl3.hs.msg_body.len == ss->ssl3.hs.msg_len) {
		rv = ssl3_HandleHandshakeMessage(
		    ss, ss->ssl3.hs.msg_body.buf, ss->ssl3.hs.msg_len);
		if (rv == SECFailure) {
		    



		    return rv;
		}
		ss->ssl3.hs.msg_body.len = 0;
		ss->ssl3.hs.msg_len = 0;
		ss->ssl3.hs.header_bytes = 0;
		if (rv != SECSuccess) { 
		    return rv;
		}
	    } else {
		PORT_Assert(buf->len == 0);
		break;
	    }
	}
    }	

    origBuf->len = 0;	
    buf->buf = NULL;	
    return SECSuccess;
}





#define DUPLICATE_MSB_TO_ALL(x) ( (unsigned)( (int)(x) >> (sizeof(int)*8-1) ) )
#define DUPLICATE_MSB_TO_ALL_8(x) ((unsigned char)(DUPLICATE_MSB_TO_ALL(x)))



static unsigned int
SECStatusToMask(SECStatus rv)
{
    unsigned int good;
    

    good = rv ^ SECSuccess;
    good--;
    return DUPLICATE_MSB_TO_ALL(good);
}


static unsigned char
ssl_ConstantTimeGE(unsigned int a, unsigned int b)
{
    a -= b;
    return DUPLICATE_MSB_TO_ALL(~a);
}


static unsigned char
ssl_ConstantTimeEQ8(unsigned char a, unsigned char b)
{
    unsigned int c = a ^ b;
    c--;
    return DUPLICATE_MSB_TO_ALL_8(c);
}

static SECStatus
ssl_RemoveSSLv3CBCPadding(sslBuffer *plaintext,
			  unsigned int blockSize,
			  unsigned int macSize)
{
    unsigned int paddingLength, good, t;
    const unsigned int overhead = 1  + macSize;

    

    if (overhead > plaintext->len) {
	return SECFailure;
    }

    paddingLength = plaintext->buf[plaintext->len-1];
    
    t = plaintext->len;
    t -= paddingLength+overhead;
    
    good = DUPLICATE_MSB_TO_ALL(~t);
    
    t = blockSize - (paddingLength+1);
    good &= DUPLICATE_MSB_TO_ALL(~t);
    plaintext->len -= good & (paddingLength+1);
    return (good & SECSuccess) | (~good & SECFailure);
}

static SECStatus
ssl_RemoveTLSCBCPadding(sslBuffer *plaintext, unsigned int macSize)
{
    unsigned int paddingLength, good, t, toCheck, i;
    const unsigned int overhead = 1  + macSize;

    

    if (overhead > plaintext->len) {
	return SECFailure;
    }

    paddingLength = plaintext->buf[plaintext->len-1];
    t = plaintext->len;
    t -= paddingLength+overhead;
    
    good = DUPLICATE_MSB_TO_ALL(~t);

    








    toCheck = 255; 
    if (toCheck > plaintext->len-1) {
	toCheck = plaintext->len-1;
    }

    for (i = 0; i < toCheck; i++) {
	unsigned int t = paddingLength - i;
	

	unsigned char mask = DUPLICATE_MSB_TO_ALL(~t);
	unsigned char b = plaintext->buf[plaintext->len-1-i];
	

	good &= ~(mask&(paddingLength ^ b));
    }

    



    good &= good >> 4;
    good &= good >> 2;
    good &= good >> 1;
    good <<= sizeof(good)*8-1;
    good = DUPLICATE_MSB_TO_ALL(good);

    plaintext->len -= good & (paddingLength+1);
    return (good & SECSuccess) | (~good & SECFailure);
}






static void
ssl_CBCExtractMAC(sslBuffer *plaintext,
		  unsigned int originalLength,
		  SSL3Opaque* out,
		  unsigned int macSize)
{
    unsigned char rotatedMac[MAX_MAC_LENGTH];
    

    unsigned macEnd = plaintext->len;
    unsigned macStart = macEnd - macSize;
    

    unsigned scanStart = 0;
    unsigned i, j, divSpoiler;
    unsigned char rotateOffset;

    if (originalLength > macSize + 255 + 1)
	scanStart = originalLength - (macSize + 255 + 1);

    






    divSpoiler = macSize >> 1;
    divSpoiler <<= (sizeof(divSpoiler)-1)*8;
    rotateOffset = (divSpoiler + macStart - scanStart) % macSize;

    memset(rotatedMac, 0, macSize);
    for (i = scanStart; i < originalLength;) {
	for (j = 0; j < macSize && i < originalLength; i++, j++) {
	    unsigned char macStarted = ssl_ConstantTimeGE(i, macStart);
	    unsigned char macEnded = ssl_ConstantTimeGE(i, macEnd);
	    unsigned char b = 0;
	    b = plaintext->buf[i];
	    rotatedMac[j] |= b & macStarted & ~macEnded;
	}
    }

    

    memset(out, 0, macSize);
    for (i = 0; i < macSize; i++) {
	unsigned char offset =
	    (divSpoiler + macSize - rotateOffset + i) % macSize;
	for (j = 0; j < macSize; j++) {
	    out[j] |= rotatedMac[i] & ssl_ConstantTimeEQ8(j, offset);
	}
    }
}























SECStatus
ssl3_HandleRecord(sslSocket *ss, SSL3Ciphertext *cText, sslBuffer *databuf)
{
    const ssl3BulkCipherDef *cipher_def;
    ssl3CipherSpec *     crSpec;
    SECStatus            rv;
    unsigned int         hashBytes = MAX_MAC_LENGTH + 1;
    PRBool               isTLS;
    SSL3ContentType      rType;
    SSL3Opaque           hash[MAX_MAC_LENGTH];
    SSL3Opaque           givenHashBuf[MAX_MAC_LENGTH];
    SSL3Opaque          *givenHash;
    sslBuffer           *plaintext;
    sslBuffer            temp_buf;
    PRUint64             dtls_seq_num;
    unsigned int         ivLen = 0;
    unsigned int         originalLen = 0;
    unsigned int         good;
    unsigned int         minLength;
    unsigned char        header[13];
    unsigned int         headerLen;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );

    if (!ss->ssl3.initialized) {
	ssl_GetSSL3HandshakeLock(ss);
	rv = ssl3_InitState(ss);
	ssl_ReleaseSSL3HandshakeLock(ss);
	if (rv != SECSuccess) {
	    return rv;		
    	}
    }

    
    if (!ssl3_ClientAuthTokenPresent(ss->sec.ci.sid)) {
	PORT_SetError(SSL_ERROR_TOKEN_INSERTION_REMOVAL);
	return SECFailure;
    }

    



    if (cText == NULL) {
	SSL_DBG(("%d: SSL3[%d]: HandleRecord, resuming handshake",
		 SSL_GETPID(), ss->fd));
	rType = content_handshake;
	goto process_it;
    }

    ssl_GetSpecReadLock(ss); 

    crSpec = ss->ssl3.crSpec;
    cipher_def = crSpec->cipher_def;

    







    if (IS_DTLS(ss)) {
	DTLSEpoch epoch = (cText->seq_num.high >> 16) & 0xffff;
	
	if (crSpec->epoch != epoch) {
	    ssl_ReleaseSpecReadLock(ss);
	    SSL_DBG(("%d: SSL3[%d]: HandleRecord, received packet "
		     "from irrelevant epoch %d", SSL_GETPID(), ss->fd, epoch));
	    
            databuf->len = 0; 
	    return SECSuccess;
	}

	dtls_seq_num = (((PRUint64)(cText->seq_num.high & 0xffff)) << 32) |
			((PRUint64)cText->seq_num.low);

	if (dtls_RecordGetRecvd(&crSpec->recvdRecords, dtls_seq_num) != 0) {
	    ssl_ReleaseSpecReadLock(ss);
	    SSL_DBG(("%d: SSL3[%d]: HandleRecord, rejecting "
		     "potentially replayed packet", SSL_GETPID(), ss->fd));
	    
            databuf->len = 0; 
	    return SECSuccess;
	}
    }

    good = ~0U;
    minLength = crSpec->mac_size;
    if (cipher_def->type == type_block) {
	
	minLength++;
	if (crSpec->version >= SSL_LIBRARY_VERSION_TLS_1_1) {
	    
	    minLength += cipher_def->iv_size;
	}
    } else if (cipher_def->type == type_aead) {
	minLength = cipher_def->explicit_nonce_size + cipher_def->tag_size;
    }

    

    if (cText->buf->len < minLength) {
	goto decrypt_loser;
    }

    if (cipher_def->type == type_block &&
	crSpec->version >= SSL_LIBRARY_VERSION_TLS_1_1) {
	





	SSL3Opaque iv[MAX_IV_LENGTH];
	int decoded;

	ivLen = cipher_def->iv_size;
	if (ivLen < 8 || ivLen > sizeof(iv)) {
	    ssl_ReleaseSpecReadLock(ss);
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return SECFailure;
	}

	PRINT_BUF(80, (ss, "IV (ciphertext):", cText->buf->buf, ivLen));

	



	rv = crSpec->decode(crSpec->decodeContext, iv, &decoded,
			    sizeof(iv), cText->buf->buf, ivLen);

	good &= SECStatusToMask(rv);
    }

    

    if (crSpec->decompressor) {
	temp_buf.buf = NULL;
	temp_buf.space = 0;
	plaintext = &temp_buf;
    } else {
	plaintext = databuf;
    }

    plaintext->len = 0; 
    if (plaintext->space < MAX_FRAGMENT_LENGTH) {
	rv = sslBuffer_Grow(plaintext, MAX_FRAGMENT_LENGTH + 2048);
	if (rv != SECSuccess) {
	    ssl_ReleaseSpecReadLock(ss);
	    SSL_DBG(("%d: SSL3[%d]: HandleRecord, tried to get %d bytes",
		     SSL_GETPID(), ss->fd, MAX_FRAGMENT_LENGTH + 2048));
	    
	    
	    return SECFailure;
	}
    }

    PRINT_BUF(80, (ss, "ciphertext:", cText->buf->buf + ivLen,
				      cText->buf->len - ivLen));

    isTLS = (PRBool)(crSpec->version > SSL_LIBRARY_VERSION_3_0);

    if (isTLS && cText->buf->len - ivLen > (MAX_FRAGMENT_LENGTH + 2048)) {
	ssl_ReleaseSpecReadLock(ss);
	SSL3_SendAlert(ss, alert_fatal, record_overflow);
	PORT_SetError(SSL_ERROR_RX_RECORD_TOO_LONG);
	return SECFailure;
    }

    rType = cText->type;
    if (cipher_def->type == type_aead) {
	



	unsigned int decryptedLen =
	    cText->buf->len - cipher_def->explicit_nonce_size -
	    cipher_def->tag_size;
	headerLen = ssl3_BuildRecordPseudoHeader(
	    header, IS_DTLS(ss) ? cText->seq_num : crSpec->read_seq_num,
	    rType, isTLS, cText->version, IS_DTLS(ss), decryptedLen);
	PORT_Assert(headerLen <= sizeof(header));
	rv = crSpec->aead(
		ss->sec.isServer ? &crSpec->client : &crSpec->server,
		PR_TRUE,                          
		plaintext->buf,                   
		(int*) &plaintext->len,           
		plaintext->space,                 
		cText->buf->buf,                  
		cText->buf->len,                  
		header, headerLen);
	if (rv != SECSuccess) {
	    good = 0;
	}
    } else {
	if (cipher_def->type == type_block &&
	    ((cText->buf->len - ivLen) % cipher_def->block_size) != 0) {
	    goto decrypt_loser;
	}

	
	rv = crSpec->decode(
	    crSpec->decodeContext, plaintext->buf, (int *)&plaintext->len,
	    plaintext->space, cText->buf->buf + ivLen, cText->buf->len - ivLen);
	if (rv != SECSuccess) {
	    goto decrypt_loser;
	}

	PRINT_BUF(80, (ss, "cleartext:", plaintext->buf, plaintext->len));

	originalLen = plaintext->len;

	
	if (cipher_def->type == type_block) {
	    const unsigned int blockSize = cipher_def->block_size;
	    const unsigned int macSize = crSpec->mac_size;

	    if (!isTLS) {
		good &= SECStatusToMask(ssl_RemoveSSLv3CBCPadding(
			    plaintext, blockSize, macSize));
	    } else {
		good &= SECStatusToMask(ssl_RemoveTLSCBCPadding(
			    plaintext, macSize));
	    }
	}

	
	headerLen = ssl3_BuildRecordPseudoHeader(
	    header, IS_DTLS(ss) ? cText->seq_num : crSpec->read_seq_num,
	    rType, isTLS, cText->version, IS_DTLS(ss),
	    plaintext->len - crSpec->mac_size);
	PORT_Assert(headerLen <= sizeof(header));
	if (cipher_def->type == type_block) {
	    rv = ssl3_ComputeRecordMACConstantTime(
		crSpec, (PRBool)(!ss->sec.isServer), header, headerLen,
		plaintext->buf, plaintext->len, originalLen,
		hash, &hashBytes);

	    ssl_CBCExtractMAC(plaintext, originalLen, givenHashBuf,
			      crSpec->mac_size);
	    givenHash = givenHashBuf;

	    



	    plaintext->len -= crSpec->mac_size;
	} else {
	    
	    plaintext->len -= crSpec->mac_size;

	    rv = ssl3_ComputeRecordMAC(
		crSpec, (PRBool)(!ss->sec.isServer), header, headerLen,
		plaintext->buf, plaintext->len, hash, &hashBytes);

	    

	    givenHash = plaintext->buf + plaintext->len;
	}

	good &= SECStatusToMask(rv);

	if (hashBytes != (unsigned)crSpec->mac_size ||
	    NSS_SecureMemcmp(givenHash, hash, crSpec->mac_size) != 0) {
	    
	    good = 0;
	}
    }

    if (good == 0) {
decrypt_loser:
	
	ssl_ReleaseSpecReadLock(ss);

	SSL_DBG(("%d: SSL3[%d]: decryption failed", SSL_GETPID(), ss->fd));

	if (!IS_DTLS(ss)) {
	    SSL3_SendAlert(ss, alert_fatal, bad_record_mac);
	    
	    PORT_SetError(SSL_ERROR_BAD_MAC_READ);
	    return SECFailure;
	} else {
	    
            databuf->len = 0; 
	    return SECSuccess;
	}
    }

    if (!IS_DTLS(ss)) {
	ssl3_BumpSequenceNumber(&crSpec->read_seq_num);
    } else {
	dtls_RecordSetRecvd(&crSpec->recvdRecords, dtls_seq_num);
    }

    ssl_ReleaseSpecReadLock(ss); 

    



    


    if (crSpec->decompressor) {
	if (databuf->space < plaintext->len + SSL3_COMPRESSION_MAX_EXPANSION) {
	    rv = sslBuffer_Grow(
	        databuf, plaintext->len + SSL3_COMPRESSION_MAX_EXPANSION);
	    if (rv != SECSuccess) {
		SSL_DBG(("%d: SSL3[%d]: HandleRecord, tried to get %d bytes",
			 SSL_GETPID(), ss->fd,
			 plaintext->len + SSL3_COMPRESSION_MAX_EXPANSION));
		
		
		PORT_Free(plaintext->buf);
		return SECFailure;
	    }
	}

	rv = crSpec->decompressor(crSpec->decompressContext,
				  databuf->buf,
				  (int*) &databuf->len,
				  databuf->space,
				  plaintext->buf,
				  plaintext->len);

	if (rv != SECSuccess) {
	    int err = ssl_MapLowLevelError(SSL_ERROR_DECOMPRESSION_FAILURE);
	    SSL3_SendAlert(ss, alert_fatal,
			   isTLS ? decompression_failure : bad_record_mac);

	    









	    if (plaintext->len >= 4) {
		unsigned int len = ((unsigned int) plaintext->buf[1] << 16) |
		                   ((unsigned int) plaintext->buf[2] << 8) |
		                   (unsigned int) plaintext->buf[3];
		if (len == plaintext->len - 4) {
		    
		    err = SSL_ERROR_RX_UNEXPECTED_UNCOMPRESSED_RECORD;
		}
	    }

	    PORT_Free(plaintext->buf);
	    PORT_SetError(err);
	    return SECFailure;
	}

	PORT_Free(plaintext->buf);
    }

    


    if (isTLS && databuf->len > (MAX_FRAGMENT_LENGTH + 1024)) {
	SSL3_SendAlert(ss, alert_fatal, record_overflow);
	PORT_SetError(SSL_ERROR_RX_RECORD_TOO_LONG);
	return SECFailure;
    }

    


    if (rType == content_application_data) {
	if (ss->firstHsDone)
	    return SECSuccess;
	(void)SSL3_SendAlert(ss, alert_fatal, unexpected_message);
	PORT_SetError(SSL_ERROR_RX_UNEXPECTED_APPLICATION_DATA);
	return SECFailure;
    }

    

process_it:
    


    ssl_GetSSL3HandshakeLock(ss);

    


    switch (rType) {
    case content_change_cipher_spec:
	rv = ssl3_HandleChangeCipherSpecs(ss, databuf);
	break;
    case content_alert:
	rv = ssl3_HandleAlert(ss, databuf);
	break;
    case content_handshake:
	if (!IS_DTLS(ss)) {
	    rv = ssl3_HandleHandshake(ss, databuf);
	} else {
	    rv = dtls_HandleHandshake(ss, databuf);
	}
	break;
    


    default:
	SSL_DBG(("%d: SSL3[%d]: bogus content type=%d",
		 SSL_GETPID(), ss->fd, cText->type));
	
	PORT_SetError(SSL_ERROR_RX_UNKNOWN_RECORD_TYPE);
	rv = SECFailure;
	break;
    }

    ssl_ReleaseSSL3HandshakeLock(ss);
    return rv;
}







static void
ssl3_InitCipherSpec(sslSocket *ss, ssl3CipherSpec *spec)
{
    spec->cipher_def               = &bulk_cipher_defs[cipher_null];
    PORT_Assert(spec->cipher_def->cipher == cipher_null);
    spec->mac_def                  = &mac_defs[mac_null];
    PORT_Assert(spec->mac_def->mac == mac_null);
    spec->encode                   = Null_Cipher;
    spec->decode                   = Null_Cipher;
    spec->destroy                  = NULL;
    spec->compressor               = NULL;
    spec->decompressor             = NULL;
    spec->destroyCompressContext   = NULL;
    spec->destroyDecompressContext = NULL;
    spec->mac_size                 = 0;
    spec->master_secret            = NULL;
    spec->bypassCiphers            = PR_FALSE;

    spec->msItem.data              = NULL;
    spec->msItem.len               = 0;

    spec->client.write_key         = NULL;
    spec->client.write_mac_key     = NULL;
    spec->client.write_mac_context = NULL;

    spec->server.write_key         = NULL;
    spec->server.write_mac_key     = NULL;
    spec->server.write_mac_context = NULL;

    spec->write_seq_num.high       = 0;
    spec->write_seq_num.low        = 0;

    spec->read_seq_num.high        = 0;
    spec->read_seq_num.low         = 0;

    spec->epoch                    = 0;
    dtls_InitRecvdRecords(&spec->recvdRecords);

    spec->version                  = ss->vrange.max;
}











static SECStatus
ssl3_InitState(sslSocket *ss)
{
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    if (ss->ssl3.initialized)
    	return SECSuccess;	

    ss->ssl3.policy = SSL_ALLOWED;

    ssl_GetSpecWriteLock(ss);
    ss->ssl3.crSpec = ss->ssl3.cwSpec = &ss->ssl3.specs[0];
    ss->ssl3.prSpec = ss->ssl3.pwSpec = &ss->ssl3.specs[1];
    ss->ssl3.hs.sendingSCSV = PR_FALSE;
    ssl3_InitCipherSpec(ss, ss->ssl3.crSpec);
    ssl3_InitCipherSpec(ss, ss->ssl3.prSpec);

    ss->ssl3.hs.ws = (ss->sec.isServer) ? wait_client_hello : wait_server_hello;
#ifndef NSS_DISABLE_ECC
    ss->ssl3.hs.negotiatedECCurves = ssl3_GetSupportedECCurveMask(ss);
#endif
    ssl_ReleaseSpecWriteLock(ss);

    PORT_Memset(&ss->xtnData, 0, sizeof(TLSExtensionData));

    if (IS_DTLS(ss)) {
	ss->ssl3.hs.sendMessageSeq = 0;
	ss->ssl3.hs.recvMessageSeq = 0;
	ss->ssl3.hs.rtTimeoutMs = INITIAL_DTLS_TIMEOUT_MS;
	ss->ssl3.hs.rtRetries = 0;
	ss->ssl3.hs.recvdHighWater = -1;
	PR_INIT_CLIST(&ss->ssl3.hs.lastMessageFlight);
	dtls_SetMTU(ss, 0); 
    }

    PORT_Assert(!ss->ssl3.hs.messages.buf && !ss->ssl3.hs.messages.space);
    ss->ssl3.hs.messages.buf = NULL;
    ss->ssl3.hs.messages.space = 0;

    ss->ssl3.hs.receivedNewSessionTicket = PR_FALSE;
    PORT_Memset(&ss->ssl3.hs.newSessionTicket, 0,
		sizeof(ss->ssl3.hs.newSessionTicket));

    ss->ssl3.initialized = PR_TRUE;
    return SECSuccess;
}





ssl3KeyPair *
ssl3_NewKeyPair( SECKEYPrivateKey * privKey, SECKEYPublicKey * pubKey)
{
    ssl3KeyPair * pair;

    if (!privKey || !pubKey) {
	PORT_SetError(PR_INVALID_ARGUMENT_ERROR);
    	return NULL;
    }
    pair = PORT_ZNew(ssl3KeyPair);
    if (!pair)
    	return NULL;			
    pair->refCount = 1;
    pair->privKey  = privKey;
    pair->pubKey   = pubKey;
    return pair;			
}

ssl3KeyPair *
ssl3_GetKeyPairRef(ssl3KeyPair * keyPair)
{
    PR_ATOMIC_INCREMENT(&keyPair->refCount);
    return keyPair;
}

void
ssl3_FreeKeyPair(ssl3KeyPair * keyPair)
{
    PRInt32 newCount =  PR_ATOMIC_DECREMENT(&keyPair->refCount);
    if (!newCount) {
	if (keyPair->privKey)
	    SECKEY_DestroyPrivateKey(keyPair->privKey);
	if (keyPair->pubKey)
	    SECKEY_DestroyPublicKey( keyPair->pubKey);
    	PORT_Free(keyPair);
    }
}







SECStatus
ssl3_CreateRSAStepDownKeys(sslSocket *ss)
{
    SECStatus             rv  	 = SECSuccess;
    SECKEYPrivateKey *    privKey;		
    SECKEYPublicKey *     pubKey;		

    if (ss->stepDownKeyPair)
	ssl3_FreeKeyPair(ss->stepDownKeyPair);
    ss->stepDownKeyPair = NULL;
#ifndef HACKED_EXPORT_SERVER
    
    if (PK11_GetPrivateModulusLen(ss->serverCerts[kt_rsa].SERVERKEY) >
                                                     EXPORT_RSA_KEY_LENGTH) {
	
	privKey = SECKEY_CreateRSAPrivateKey(EXPORT_RSA_KEY_LENGTH * BPB,
					     &pubKey, NULL);
    	if (!privKey || !pubKey ||
	    !(ss->stepDownKeyPair = ssl3_NewKeyPair(privKey, pubKey))) {
	    ssl_MapLowLevelError(SEC_ERROR_KEYGEN_FAIL);
	    rv = SECFailure;
	}
    }
#endif
    return rv;
}



SECStatus
ssl3_SetPolicy(ssl3CipherSuite which, int policy)
{
    ssl3CipherSuiteCfg *suite;

    suite = ssl_LookupCipherSuiteCfg(which, cipherSuites);
    if (suite == NULL) {
	return SECFailure; 
    }
    suite->policy = policy;

    return SECSuccess;
}

SECStatus
ssl3_GetPolicy(ssl3CipherSuite which, PRInt32 *oPolicy)
{
    ssl3CipherSuiteCfg *suite;
    PRInt32             policy;
    SECStatus           rv;

    suite = ssl_LookupCipherSuiteCfg(which, cipherSuites);
    if (suite) {
    	policy = suite->policy;
	rv     = SECSuccess;
    } else {
    	policy = SSL_NOT_ALLOWED;
	rv     = SECFailure;	
    }
    *oPolicy = policy;
    return rv;
}


SECStatus
ssl3_CipherPrefSetDefault(ssl3CipherSuite which, PRBool enabled)
{
    ssl3CipherSuiteCfg *suite;

    suite = ssl_LookupCipherSuiteCfg(which, cipherSuites);
    if (suite == NULL) {
	return SECFailure; 
    }
    suite->enabled = enabled;
    return SECSuccess;
}


SECStatus
ssl3_CipherPrefGetDefault(ssl3CipherSuite which, PRBool *enabled)
{
    ssl3CipherSuiteCfg *suite;
    PRBool              pref;
    SECStatus           rv;

    suite = ssl_LookupCipherSuiteCfg(which, cipherSuites);
    if (suite) {
    	pref   = suite->enabled;
	rv     = SECSuccess;
    } else {
    	pref   = SSL_NOT_ALLOWED;
	rv     = SECFailure;	
    }
    *enabled = pref;
    return rv;
}

SECStatus
ssl3_CipherPrefSet(sslSocket *ss, ssl3CipherSuite which, PRBool enabled)
{
    ssl3CipherSuiteCfg *suite;

    suite = ssl_LookupCipherSuiteCfg(which, ss->cipherSuites);
    if (suite == NULL) {
	return SECFailure; 
    }
    suite->enabled = enabled;
    return SECSuccess;
}

SECStatus
ssl3_CipherPrefGet(sslSocket *ss, ssl3CipherSuite which, PRBool *enabled)
{
    ssl3CipherSuiteCfg *suite;
    PRBool              pref;
    SECStatus           rv;

    suite = ssl_LookupCipherSuiteCfg(which, ss->cipherSuites);
    if (suite) {
    	pref   = suite->enabled;
	rv     = SECSuccess;
    } else {
    	pref   = SSL_NOT_ALLOWED;
	rv     = SECFailure;	
    }
    *enabled = pref;
    return rv;
}


void
ssl3_InitSocketPolicy(sslSocket *ss)
{
    PORT_Memcpy(ss->cipherSuites, cipherSuites, sizeof cipherSuites);
}




SECStatus
ssl3_ConstructV2CipherSpecsHack(sslSocket *ss, unsigned char *cs, int *size)
{
    int i, count = 0;

    PORT_Assert(ss != 0);
    if (!ss) {
	PORT_SetError(PR_INVALID_ARGUMENT_ERROR);
	return SECFailure;
    }
    if (SSL3_ALL_VERSIONS_DISABLED(&ss->vrange)) {
    	*size = 0;
	return SECSuccess;
    }
    if (cs == NULL) {
	*size = count_cipher_suites(ss, SSL_ALLOWED, PR_TRUE);
	return SECSuccess;
    }

    
    for (i = 0; i < ssl_V3_SUITES_IMPLEMENTED; i++) {
	ssl3CipherSuiteCfg *suite = &ss->cipherSuites[i];
	if (config_match(suite, SSL_ALLOWED, PR_TRUE, &ss->vrange)) {
	    if (cs != NULL) {
		*cs++ = 0x00;
		*cs++ = (suite->cipher_suite >> 8) & 0xFF;
		*cs++ =  suite->cipher_suite       & 0xFF;
	    }
	    count++;
	}
    }
    *size = count;
    return SECSuccess;
}









SECStatus
ssl3_RedoHandshake(sslSocket *ss, PRBool flushCache)
{
    sslSessionID *   sid = ss->sec.ci.sid;
    SECStatus        rv;

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    if (!ss->firstHsDone ||
        ((ss->version >= SSL_LIBRARY_VERSION_3_0) &&
	 ss->ssl3.initialized && 
	 (ss->ssl3.hs.ws != idle_handshake))) {
	PORT_SetError(SSL_ERROR_HANDSHAKE_NOT_COMPLETED);
	return SECFailure;
    }

    if (IS_DTLS(ss)) {
	dtls_RehandshakeCleanup(ss);
    }

    if (ss->opt.enableRenegotiation == SSL_RENEGOTIATE_NEVER) {
	PORT_SetError(SSL_ERROR_RENEGOTIATION_NOT_ALLOWED);
	return SECFailure;
    }
    if (sid && flushCache) {
        if (ss->sec.uncache)
            ss->sec.uncache(sid); 
	ssl_FreeSID(sid);	
	ss->sec.ci.sid = NULL;
    }

    ssl_GetXmitBufLock(ss);	

    
    rv = (ss->sec.isServer) ? ssl3_SendHelloRequest(ss)
                            : ssl3_SendClientHello(ss, PR_FALSE);

    ssl_ReleaseXmitBufLock(ss);	
    return rv;
}


void
ssl3_DestroySSL3Info(sslSocket *ss)
{

    if (ss->ssl3.clientCertificate != NULL)
	CERT_DestroyCertificate(ss->ssl3.clientCertificate);

    if (ss->ssl3.clientPrivateKey != NULL)
	SECKEY_DestroyPrivateKey(ss->ssl3.clientPrivateKey);

    if (ss->ssl3.peerCertArena != NULL)
	ssl3_CleanupPeerCerts(ss);

    if (ss->ssl3.clientCertChain != NULL) {
       CERT_DestroyCertificateList(ss->ssl3.clientCertChain);
       ss->ssl3.clientCertChain = NULL;
    }

    
#ifndef NO_PKCS11_BYPASS
    if (ss->opt.bypassPKCS11) {
	if (ss->ssl3.hs.hashType == handshake_hash_combo) {
	    SHA1_DestroyContext((SHA1Context *)ss->ssl3.hs.sha_cx, PR_FALSE);
	    MD5_DestroyContext((MD5Context *)ss->ssl3.hs.md5_cx, PR_FALSE);
	} else if (ss->ssl3.hs.hashType == handshake_hash_single) {
	    ss->ssl3.hs.sha_obj->destroy(ss->ssl3.hs.sha_cx, PR_FALSE);
	}
    } 
#endif
    if (ss->ssl3.hs.md5) {
	PK11_DestroyContext(ss->ssl3.hs.md5,PR_TRUE);
    }
    if (ss->ssl3.hs.sha) {
	PK11_DestroyContext(ss->ssl3.hs.sha,PR_TRUE);
    }
    if (ss->ssl3.hs.clientSigAndHash) {
	PORT_Free(ss->ssl3.hs.clientSigAndHash);
    }
    if (ss->ssl3.hs.messages.buf) {
    	PORT_Free(ss->ssl3.hs.messages.buf);
	ss->ssl3.hs.messages.buf = NULL;
	ss->ssl3.hs.messages.len = 0;
	ss->ssl3.hs.messages.space = 0;
    }

    
    PORT_Free(ss->ssl3.hs.msg_body.buf);

    SECITEM_FreeItem(&ss->ssl3.hs.newSessionTicket.ticket, PR_FALSE);

    
    ssl3_DestroyCipherSpec(&ss->ssl3.specs[0], PR_TRUE);
    ssl3_DestroyCipherSpec(&ss->ssl3.specs[1], PR_TRUE);

    
    if (IS_DTLS(ss)) {
	dtls_FreeHandshakeMessages(&ss->ssl3.hs.lastMessageFlight);
	if (ss->ssl3.hs.recvdFragments.buf) {
	    PORT_Free(ss->ssl3.hs.recvdFragments.buf);
	}
    }

    ss->ssl3.initialized = PR_FALSE;

    SECITEM_FreeItem(&ss->ssl3.nextProto, PR_FALSE);
}


