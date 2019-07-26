






#ifndef _BLAPI_H_
#define _BLAPI_H_

#include "blapit.h"
#include "hasht.h"
#include "alghmac.h"

SEC_BEGIN_PROTOS






extern SECStatus BL_Init(void);











extern RSAPrivateKey *RSA_NewKey(int         keySizeInBits,
				 SECItem *   publicExponent);





extern SECStatus RSA_PublicKeyOp(RSAPublicKey *   key,
				 unsigned char *  output,
				 const unsigned char *  input);





extern SECStatus RSA_PrivateKeyOp(RSAPrivateKey *  key,
				  unsigned char *  output,
				  const unsigned char *  input);






extern SECStatus RSA_PrivateKeyOpDoubleChecked(RSAPrivateKey *  key,
				               unsigned char *  output,
				               const unsigned char *  input);




extern SECStatus RSA_PrivateKeyCheck(RSAPrivateKey *key);










































extern SECStatus RSA_PopulatePrivateKey(RSAPrivateKey *key);











extern SECStatus
RSA_SignRaw(RSAPrivateKey       * key,
            unsigned char       * output,
            unsigned int        * outputLen,
            unsigned int          maxOutputLen,
            const unsigned char * input,
            unsigned int          inputLen);

extern SECStatus
RSA_CheckSignRaw(RSAPublicKey        * key,
                 const unsigned char * sig,
                 unsigned int          sigLen,
                 const unsigned char * hash,
                 unsigned int          hashLen);

extern SECStatus
RSA_CheckSignRecoverRaw(RSAPublicKey        * key,
                        unsigned char       * data,
                        unsigned int        * dataLen,
                        unsigned int          maxDataLen,
                        const unsigned char * sig,
                        unsigned int          sigLen);

extern SECStatus
RSA_EncryptRaw(RSAPublicKey        * key,
               unsigned char       * output,
               unsigned int        * outputLen,
               unsigned int          maxOutputLen,
               const unsigned char * input,
               unsigned int          inputLen);

extern SECStatus
RSA_DecryptRaw(RSAPrivateKey       * key,
               unsigned char       * output,
               unsigned int        * outputLen,
               unsigned int          maxOutputLen,
               const unsigned char * input,
               unsigned int          inputLen);












extern SECStatus
RSA_EncryptOAEP(RSAPublicKey        * key,
                HASH_HashType         hashAlg,
                HASH_HashType         maskHashAlg,
                const unsigned char * label,
                unsigned int          labelLen,
                const unsigned char * seed,
                unsigned int          seedLen,
                unsigned char       * output,
                unsigned int        * outputLen,
                unsigned int          maxOutputLen,
                const unsigned char * input,
                unsigned int          inputLen);

extern SECStatus
RSA_DecryptOAEP(RSAPrivateKey       * key,
                HASH_HashType         hashAlg,
                HASH_HashType         maskHashAlg,
                const unsigned char * label,
                unsigned int          labelLen,
                unsigned char       * output,
                unsigned int        * outputLen,
                unsigned int          maxOutputLen,
                const unsigned char * input,
                unsigned int          inputLen);




extern SECStatus
RSA_EncryptBlock(RSAPublicKey        * key,
                 unsigned char       * output,
                 unsigned int        * outputLen,
                 unsigned int          maxOutputLen,
                 const unsigned char * input,
                 unsigned int          inputLen);

extern SECStatus
RSA_DecryptBlock(RSAPrivateKey       * key,
                 unsigned char       * output,
                 unsigned int        * outputLen,
                 unsigned int          maxOutputLen,
                 const unsigned char * input,
                 unsigned int          inputLen);










extern SECStatus
RSA_SignPSS(RSAPrivateKey       * key,
            HASH_HashType         hashAlg,
            HASH_HashType         maskHashAlg,
            const unsigned char * salt,
            unsigned int          saltLen,
            unsigned char       * output,
            unsigned int        * outputLen,
            unsigned int          maxOutputLen,
            const unsigned char * input,
            unsigned int          inputLen);

extern SECStatus
RSA_CheckSignPSS(RSAPublicKey        * key,
                 HASH_HashType         hashAlg,
                 HASH_HashType         maskHashAlg,
                 unsigned int          saltLen,
                 const unsigned char * sig,
                 unsigned int          sigLen,
                 const unsigned char * hash,
                 unsigned int          hashLen);










extern SECStatus
RSA_Sign(RSAPrivateKey       * key,
         unsigned char       * output,
         unsigned int        * outputLen,
         unsigned int          maxOutputLen,
         const unsigned char * data,
         unsigned int          dataLen);

extern SECStatus
RSA_CheckSign(RSAPublicKey        * key,
              const unsigned char * sig,
              unsigned int          sigLen,
              const unsigned char * data,
              unsigned int          dataLen);

extern SECStatus
RSA_CheckSignRecover(RSAPublicKey        * key,
                     unsigned char       * output,
                     unsigned int        * outputLen,
                     unsigned int          maxOutputLen,
                     const unsigned char * sig,
                     unsigned int          sigLen);







extern SECStatus DSA_NewRandom(PLArenaPool * arena, const SECItem * q,
                               SECItem * random);







extern SECStatus DSA_NewKey(const PQGParams *     params, 
		            DSAPrivateKey **      privKey);







extern SECStatus DSA_SignDigest(DSAPrivateKey *   key,
				SECItem *         signature,
				const SECItem *   digest);





extern SECStatus DSA_VerifyDigest(DSAPublicKey *  key,
				  const SECItem * signature,
				  const SECItem * digest);


extern SECStatus DSA_NewKeyFromSeed(const PQGParams *params, 
                                    const unsigned char * seed,
                                    DSAPrivateKey **privKey);


extern SECStatus DSA_SignDigestWithSeed(DSAPrivateKey * key,
                                        SECItem *       signature,
                                        const SECItem * digest,
                                        const unsigned char * seed);








extern SECStatus DH_GenParam(int primeLen, DHParams ** params);





extern SECStatus DH_NewKey(DHParams *           params, 
                           DHPrivateKey **	privKey);



















extern SECStatus DH_Derive(SECItem *    publicValue, 
		           SECItem *    prime, 
			   SECItem *    privateValue, 
			   SECItem *    derivedSecret,
			   unsigned int outBytes);





extern SECStatus KEA_Derive(SECItem *prime, 
                            SECItem *public1, 
                            SECItem *public2, 
			    SECItem *private1, 
			    SECItem *private2,
			    SECItem *derivedSecret);





extern PRBool KEA_Verify(SECItem *Y, SECItem *prime, SECItem *subPrime);






















SECStatus
JPAKE_Sign(PLArenaPool * arena, const PQGParams * pqg, HASH_HashType hashType,
           const SECItem * signerID, const SECItem * x,
           const SECItem * testRandom, const SECItem * gxIn, SECItem * gxOut,
           SECItem * gv, SECItem * r);






SECStatus
JPAKE_Verify(PLArenaPool * arena, const PQGParams * pqg,
             HASH_HashType hashType, const SECItem * signerID,
             const SECItem * peerID, const SECItem * gx,
             const SECItem * gv, const SECItem * r);














SECStatus
JPAKE_Round2(PLArenaPool * arena, const SECItem * p, const SECItem  *q,
             const SECItem * gx1, const SECItem * gx3, const SECItem * gx4,
             SECItem * base, const SECItem * x2, const SECItem * s, SECItem * x2s);






SECStatus
JPAKE_Final(PLArenaPool * arena, const SECItem * p, const SECItem  *q,
            const SECItem * x2, const SECItem * gx4, const SECItem * x2s,
            const SECItem * B, SECItem * K);









extern SECStatus EC_NewKey(ECParams *          params, 
                           ECPrivateKey **     privKey);

extern SECStatus EC_NewKeyFromSeed(ECParams *  params, 
                           ECPrivateKey **     privKey,
                           const unsigned char* seed,
                           int                 seedlen);





extern SECStatus EC_ValidatePublicKey(ECParams * params, 
                           SECItem *           publicValue);














extern SECStatus ECDH_Derive(SECItem *       publicValue,
                             ECParams *      params,
                             SECItem *       privateValue,
                             PRBool          withCofactor,
                             SECItem *       derivedSecret);






extern SECStatus ECDSA_SignDigest(ECPrivateKey  *key, 
                                  SECItem       *signature, 
                                  const SECItem *digest);




extern SECStatus ECDSA_VerifyDigest(ECPublicKey   *key, 
                                    const SECItem *signature, 
                                    const SECItem *digest);


extern SECStatus ECDSA_SignDigestWithSeed(ECPrivateKey        *key,
                                          SECItem             *signature,
                                          const SECItem       *digest,
                                          const unsigned char *seed, 
                                          const int           seedlen);











extern RC4Context *RC4_CreateContext(const unsigned char *key, int len);

extern RC4Context *RC4_AllocateContext(void);
extern SECStatus   RC4_InitContext(RC4Context *cx, 
				   const unsigned char *key, 
				   unsigned int keylen,
				   const unsigned char *, 
				   int, 
				   unsigned int ,
				   unsigned int );






extern void RC4_DestroyContext(RC4Context *cx, PRBool freeit);












extern SECStatus RC4_Encrypt(RC4Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);












extern SECStatus RC4_Decrypt(RC4Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);


















extern RC2Context *RC2_CreateContext(const unsigned char *key, unsigned int len,
				     const unsigned char *iv, int mode, 
				     unsigned effectiveKeyLen);
extern RC2Context *RC2_AllocateContext(void);
extern SECStatus   RC2_InitContext(RC2Context *cx,
				   const unsigned char *key, 
				   unsigned int keylen,
				   const unsigned char *iv, 
				   int mode, 
				   unsigned int effectiveKeyLen,
				   unsigned int );






extern void RC2_DestroyContext(RC2Context *cx, PRBool freeit);












extern SECStatus RC2_Encrypt(RC2Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);












extern SECStatus RC2_Decrypt(RC2Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);
















extern RC5Context *RC5_CreateContext(const SECItem *key, unsigned int rounds,
                     unsigned int wordSize, const unsigned char *iv, int mode);
extern RC5Context *RC5_AllocateContext(void);
extern SECStatus   RC5_InitContext(RC5Context *cx, 
				   const unsigned char *key, 
				   unsigned int keylen,
				   const unsigned char *iv, 
				   int mode,
				   unsigned int rounds, 
				   unsigned int wordSize);






extern void RC5_DestroyContext(RC5Context *cx, PRBool freeit);












extern SECStatus RC5_Encrypt(RC5Context *cx, unsigned char *output,
                            unsigned int *outputLen, unsigned int maxOutputLen,
                            const unsigned char *input, unsigned int inputLen);













extern SECStatus RC5_Decrypt(RC5Context *cx, unsigned char *output,
                            unsigned int *outputLen, unsigned int maxOutputLen,
                            const unsigned char *input, unsigned int inputLen);




















extern DESContext *DES_CreateContext(const unsigned char *key, 
                                     const unsigned char *iv,
				     int mode, PRBool encrypt);
extern DESContext *DES_AllocateContext(void);
extern SECStatus   DES_InitContext(DESContext *cx,
				   const unsigned char *key, 
				   unsigned int keylen,
				   const unsigned char *iv, 
				   int mode,
				   unsigned int encrypt,
				   unsigned int );






extern void DES_DestroyContext(DESContext *cx, PRBool freeit);














extern SECStatus DES_Encrypt(DESContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);














extern SECStatus DES_Decrypt(DESContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);





extern SEEDContext *
SEED_CreateContext(const unsigned char *key, const unsigned char *iv, 
		   int mode, PRBool encrypt);
extern SEEDContext *SEED_AllocateContext(void);
extern SECStatus   SEED_InitContext(SEEDContext *cx, 
				    const unsigned char *key, 
				    unsigned int keylen, 
				    const unsigned char *iv, 
				    int mode, unsigned int encrypt, 
				    unsigned int );
extern void SEED_DestroyContext(SEEDContext *cx, PRBool freeit);
extern SECStatus 
SEED_Encrypt(SEEDContext *cx, unsigned char *output, 
	     unsigned int *outputLen, unsigned int maxOutputLen, 
	     const unsigned char *input, unsigned int inputLen);
extern SECStatus 
SEED_Decrypt(SEEDContext *cx, unsigned char *output, 
	     unsigned int *outputLen, unsigned int maxOutputLen, 
             const unsigned char *input, unsigned int inputLen);













extern AESContext *
AES_CreateContext(const unsigned char *key, const unsigned char *iv, 
                  int mode, int encrypt,
                  unsigned int keylen, unsigned int blocklen);
extern AESContext *AES_AllocateContext(void);
extern SECStatus   AES_InitContext(AESContext *cx,
				   const unsigned char *key, 
				   unsigned int keylen, 
				   const unsigned char *iv, 
				   int mode, 
				   unsigned int encrypt,
				   unsigned int blocklen);






extern void 
AES_DestroyContext(AESContext *cx, PRBool freeit);












extern SECStatus 
AES_Encrypt(AESContext *cx, unsigned char *output,
            unsigned int *outputLen, unsigned int maxOutputLen,
            const unsigned char *input, unsigned int inputLen);












extern SECStatus 
AES_Decrypt(AESContext *cx, unsigned char *output,
            unsigned int *outputLen, unsigned int maxOutputLen,
            const unsigned char *input, unsigned int inputLen);













extern AESKeyWrapContext *
AESKeyWrap_CreateContext(const unsigned char *key, const unsigned char *iv, 
                         int encrypt, unsigned int keylen);
extern AESKeyWrapContext * AESKeyWrap_AllocateContext(void);
extern SECStatus  
     AESKeyWrap_InitContext(AESKeyWrapContext *cx, 
				   const unsigned char *key, 
				   unsigned int keylen,
				   const unsigned char *iv, 
				   int ,
				   unsigned int encrypt,
				   unsigned int );






extern void 
AESKeyWrap_DestroyContext(AESKeyWrapContext *cx, PRBool freeit);












extern SECStatus 
AESKeyWrap_Encrypt(AESKeyWrapContext *cx, unsigned char *output,
            unsigned int *outputLen, unsigned int maxOutputLen,
            const unsigned char *input, unsigned int inputLen);












extern SECStatus 
AESKeyWrap_Decrypt(AESKeyWrapContext *cx, unsigned char *output,
            unsigned int *outputLen, unsigned int maxOutputLen,
            const unsigned char *input, unsigned int inputLen);

 









extern CamelliaContext *
Camellia_CreateContext(const unsigned char *key, const unsigned char *iv, 
		       int mode, int encrypt, unsigned int keylen);

extern CamelliaContext *Camellia_AllocateContext(void);
extern SECStatus   Camellia_InitContext(CamelliaContext *cx,
					const unsigned char *key, 
					unsigned int keylen, 
					const unsigned char *iv, 
					int mode, 
					unsigned int encrypt,
					unsigned int unused);





extern void 
Camellia_DestroyContext(CamelliaContext *cx, PRBool freeit);












extern SECStatus 
Camellia_Encrypt(CamelliaContext *cx, unsigned char *output,
		 unsigned int *outputLen, unsigned int maxOutputLen,
		 const unsigned char *input, unsigned int inputLen);












extern SECStatus 
Camellia_Decrypt(CamelliaContext *cx, unsigned char *output,
		 unsigned int *outputLen, unsigned int maxOutputLen,
		 const unsigned char *input, unsigned int inputLen);










extern SECStatus MD5_Hash(unsigned char *dest, const char *src);




extern SECStatus MD5_HashBuf(unsigned char *dest, const unsigned char *src,
			     PRUint32 src_length);




extern MD5Context *MD5_NewContext(void);







extern void MD5_DestroyContext(MD5Context *cx, PRBool freeit);




extern void MD5_Begin(MD5Context *cx);







extern void MD5_Update(MD5Context *cx,
		       const unsigned char *input, unsigned int inputLen);









extern void MD5_End(MD5Context *cx, unsigned char *digest,
		    unsigned int *digestLen, unsigned int maxDigestLen);










extern void MD5_EndRaw(MD5Context *cx, unsigned char *digest,
		       unsigned int *digestLen, unsigned int maxDigestLen);






extern unsigned int MD5_FlattenSize(MD5Context *cx);







extern SECStatus MD5_Flatten(MD5Context *cx,unsigned char *space);







extern MD5Context * MD5_Resurrect(unsigned char *space, void *arg);
extern void MD5_Clone(MD5Context *dest, MD5Context *src);




extern void MD5_TraceState(MD5Context *cx);










extern SECStatus MD2_Hash(unsigned char *dest, const char *src);




extern MD2Context *MD2_NewContext(void);







extern void MD2_DestroyContext(MD2Context *cx, PRBool freeit);




extern void MD2_Begin(MD2Context *cx);







extern void MD2_Update(MD2Context *cx,
		       const unsigned char *input, unsigned int inputLen);









extern void MD2_End(MD2Context *cx, unsigned char *digest,
		    unsigned int *digestLen, unsigned int maxDigestLen);






extern unsigned int MD2_FlattenSize(MD2Context *cx);







extern SECStatus MD2_Flatten(MD2Context *cx,unsigned char *space);







extern MD2Context * MD2_Resurrect(unsigned char *space, void *arg);
extern void MD2_Clone(MD2Context *dest, MD2Context *src);









extern SECStatus SHA1_Hash(unsigned char *dest, const char *src);




extern SECStatus SHA1_HashBuf(unsigned char *dest, const unsigned char *src,
			      PRUint32 src_length);




extern SHA1Context *SHA1_NewContext(void);







extern void SHA1_DestroyContext(SHA1Context *cx, PRBool freeit);




extern void SHA1_Begin(SHA1Context *cx);







extern void SHA1_Update(SHA1Context *cx, const unsigned char *input,
			unsigned int inputLen);









extern void SHA1_End(SHA1Context *cx, unsigned char *digest,
		     unsigned int *digestLen, unsigned int maxDigestLen);










extern void SHA1_EndRaw(SHA1Context *cx, unsigned char *digest,
			unsigned int *digestLen, unsigned int maxDigestLen);




extern void SHA1_TraceState(SHA1Context *cx);






extern unsigned int SHA1_FlattenSize(SHA1Context *cx);







extern SECStatus SHA1_Flatten(SHA1Context *cx,unsigned char *space);







extern SHA1Context * SHA1_Resurrect(unsigned char *space, void *arg);
extern void SHA1_Clone(SHA1Context *dest, SHA1Context *src);



extern SHA224Context *SHA224_NewContext(void);
extern void SHA224_DestroyContext(SHA224Context *cx, PRBool freeit);
extern void SHA224_Begin(SHA224Context *cx);
extern void SHA224_Update(SHA224Context *cx, const unsigned char *input,
			unsigned int inputLen);
extern void SHA224_End(SHA224Context *cx, unsigned char *digest,
		     unsigned int *digestLen, unsigned int maxDigestLen);









extern void SHA224_EndRaw(SHA224Context *cx, unsigned char *digest,
			  unsigned int *digestLen, unsigned int maxDigestLen);
extern SECStatus SHA224_HashBuf(unsigned char *dest, const unsigned char *src,
				PRUint32 src_length);
extern SECStatus SHA224_Hash(unsigned char *dest, const char *src);
extern void SHA224_TraceState(SHA224Context *cx);
extern unsigned int SHA224_FlattenSize(SHA224Context *cx);
extern SECStatus SHA224_Flatten(SHA224Context *cx,unsigned char *space);
extern SHA224Context * SHA224_Resurrect(unsigned char *space, void *arg);
extern void SHA224_Clone(SHA224Context *dest, SHA224Context *src);



extern SHA256Context *SHA256_NewContext(void);
extern void SHA256_DestroyContext(SHA256Context *cx, PRBool freeit);
extern void SHA256_Begin(SHA256Context *cx);
extern void SHA256_Update(SHA256Context *cx, const unsigned char *input,
			unsigned int inputLen);
extern void SHA256_End(SHA256Context *cx, unsigned char *digest,
		     unsigned int *digestLen, unsigned int maxDigestLen);









extern void SHA256_EndRaw(SHA256Context *cx, unsigned char *digest,
			  unsigned int *digestLen, unsigned int maxDigestLen);
extern SECStatus SHA256_HashBuf(unsigned char *dest, const unsigned char *src,
				PRUint32 src_length);
extern SECStatus SHA256_Hash(unsigned char *dest, const char *src);
extern void SHA256_TraceState(SHA256Context *cx);
extern unsigned int SHA256_FlattenSize(SHA256Context *cx);
extern SECStatus SHA256_Flatten(SHA256Context *cx,unsigned char *space);
extern SHA256Context * SHA256_Resurrect(unsigned char *space, void *arg);
extern void SHA256_Clone(SHA256Context *dest, SHA256Context *src);



extern SHA512Context *SHA512_NewContext(void);
extern void SHA512_DestroyContext(SHA512Context *cx, PRBool freeit);
extern void SHA512_Begin(SHA512Context *cx);
extern void SHA512_Update(SHA512Context *cx, const unsigned char *input,
			unsigned int inputLen);









extern void SHA512_EndRaw(SHA512Context *cx, unsigned char *digest,
			  unsigned int *digestLen, unsigned int maxDigestLen);
extern void SHA512_End(SHA512Context *cx, unsigned char *digest,
		     unsigned int *digestLen, unsigned int maxDigestLen);
extern SECStatus SHA512_HashBuf(unsigned char *dest, const unsigned char *src,
				PRUint32 src_length);
extern SECStatus SHA512_Hash(unsigned char *dest, const char *src);
extern void SHA512_TraceState(SHA512Context *cx);
extern unsigned int SHA512_FlattenSize(SHA512Context *cx);
extern SECStatus SHA512_Flatten(SHA512Context *cx,unsigned char *space);
extern SHA512Context * SHA512_Resurrect(unsigned char *space, void *arg);
extern void SHA512_Clone(SHA512Context *dest, SHA512Context *src);



extern SHA384Context *SHA384_NewContext(void);
extern void SHA384_DestroyContext(SHA384Context *cx, PRBool freeit);
extern void SHA384_Begin(SHA384Context *cx);
extern void SHA384_Update(SHA384Context *cx, const unsigned char *input,
			unsigned int inputLen);
extern void SHA384_End(SHA384Context *cx, unsigned char *digest,
		     unsigned int *digestLen, unsigned int maxDigestLen);









extern void SHA384_EndRaw(SHA384Context *cx, unsigned char *digest,
			  unsigned int *digestLen, unsigned int maxDigestLen);
extern SECStatus SHA384_HashBuf(unsigned char *dest, const unsigned char *src,
				PRUint32 src_length);
extern SECStatus SHA384_Hash(unsigned char *dest, const char *src);
extern void SHA384_TraceState(SHA384Context *cx);
extern unsigned int SHA384_FlattenSize(SHA384Context *cx);
extern SECStatus SHA384_Flatten(SHA384Context *cx,unsigned char *space);
extern SHA384Context * SHA384_Resurrect(unsigned char *space, void *arg);
extern void SHA384_Clone(SHA384Context *dest, SHA384Context *src);





extern SECStatus
TLS_PRF(const SECItem *secret, const char *label, SECItem *seed, 
         SECItem *result, PRBool isFIPS);

extern SECStatus
TLS_P_hash(HASH_HashType hashAlg, const SECItem *secret, const char *label,
           SECItem *seed, SECItem *result, PRBool isFIPS);















extern SECStatus RNG_RNGInit(void);





extern SECStatus RNG_RandomUpdate(const void *data, size_t bytes);





extern SECStatus RNG_GenerateGlobalRandomBytes(void *dest, size_t len);





extern void  RNG_RNGShutdown(void);

extern void RNG_SystemInfoForRNG(void);













extern SECStatus
FIPS186Change_GenerateX(unsigned char *XKEY,
                        const unsigned char *XSEEDj,
                        unsigned char *x_j);









extern SECStatus
FIPS186Change_ReduceModQForDSA(const unsigned char *w,
                               const unsigned char *q,
                               unsigned char *xj);





extern SECStatus
PRNGTEST_Instantiate(const PRUint8 *entropy, unsigned int entropy_len, 
		const PRUint8 *nonce, unsigned int nonce_len,
		const PRUint8 *personal_string, unsigned int ps_len);

extern SECStatus
PRNGTEST_Reseed(const PRUint8 *entropy, unsigned int entropy_len, 
		  const PRUint8 *additional, unsigned int additional_len);

extern SECStatus
PRNGTEST_Generate(PRUint8 *bytes, unsigned int bytes_len, 
		  const PRUint8 *additional, unsigned int additional_len);

extern SECStatus
PRNGTEST_Uninstantiate(void);

extern SECStatus
PRNGTEST_RunHealthTests(void);







extern SECStatus
PQG_ParamGen(unsigned int j, 	   
             PQGParams **pParams,  
	     PQGVerify **pVfy);    








extern SECStatus
PQG_ParamGenSeedLen(
             unsigned int j, 	     
	     unsigned int seedBytes, 
             PQGParams **pParams,    
	     PQGVerify **pVfy);      























extern SECStatus
PQG_ParamGenV2(
             unsigned int L, 	     
             unsigned int N, 	     
	     unsigned int seedBytes, 
             PQGParams **pParams,    
	     PQGVerify **pVfy);      

















extern SECStatus   PQG_VerifyParams(const PQGParams *params, 
                                    const PQGVerify *vfy, SECStatus *result);

extern void PQG_DestroyParams(PQGParams *params);

extern void PQG_DestroyVerify(PQGVerify *vfy);







extern void BL_Cleanup(void);


extern void BL_Unload(void);




PRBool BLAPI_SHVerify(const char *name, PRFuncPtr addr);




PRBool BLAPI_SHVerifyFile(const char *shName);




PRBool BLAPI_VerifySelf(const char *name);


extern const SECHashObject * HASH_GetRawHashObject(HASH_HashType hashType);

extern void BL_SetForkState(PRBool forked);

#ifndef NSS_DISABLE_ECC



extern SECStatus EC_FillParams(PLArenaPool *arena,
                               const SECItem *encodedParams, ECParams *params);
extern SECStatus EC_DecodeParams(const SECItem *encodedParams,
                                 ECParams **ecparams);
extern SECStatus EC_CopyParams(PLArenaPool *arena, ECParams *dstParams,
                               const ECParams *srcParams);
#endif

SEC_END_PROTOS

#endif 
