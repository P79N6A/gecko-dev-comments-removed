








































#ifndef _CRYPTOHI_H_
#define _CRYPTOHI_H_

#include "blapit.h"

#include "seccomon.h"
#include "secoidt.h"
#include "secdert.h"
#include "cryptoht.h"
#include "keyt.h"
#include "certt.h"


SEC_BEGIN_PROTOS











extern SECStatus DSAU_EncodeDerSig(SECItem *dest, SECItem *src);
extern SECItem *DSAU_DecodeDerSig(const SECItem *item);













extern SECStatus DSAU_EncodeDerSigWithLen(SECItem *dest, SECItem *src, 
					  unsigned int len);
extern SECItem *DSAU_DecodeDerSigToLen(const SECItem *item, unsigned int len);











extern SGNContext *SGN_NewContext(SECOidTag alg, SECKEYPrivateKey *privKey);






extern void SGN_DestroyContext(SGNContext *cx, PRBool freeit);





extern SECStatus SGN_Begin(SGNContext *cx);







extern SECStatus SGN_Update(SGNContext *cx, unsigned char *input,
			   unsigned int inputLen);









extern SECStatus SGN_End(SGNContext *cx, SECItem *result);











extern SECStatus SEC_SignData(SECItem *result, unsigned char *buf, int len,
			     SECKEYPrivateKey *pk, SECOidTag algid);









extern SECStatus SGN_Digest(SECKEYPrivateKey *privKey,
                SECOidTag algtag, SECItem *result, SECItem *digest);












extern SECStatus SEC_DerSignData(PLArenaPool *arena, SECItem *result,
				unsigned char *buf, int len,
				SECKEYPrivateKey *pk, SECOidTag algid);






extern void SEC_DestroySignedData(CERTSignedData *sd, PRBool freeit);






extern SECOidTag SEC_GetSignatureAlgorithmOidTag(KeyType keyType,
                                                 SECOidTag hashAlgTag);


















extern VFYContext *VFY_CreateContext(SECKEYPublicKey *key, SECItem *sig,
				     SECOidTag sigAlg, void *wincx);


















extern VFYContext *VFY_CreateContextDirect(const SECKEYPublicKey *key,
					   const SECItem *sig,
	     				   SECOidTag pubkAlg, 
					   SECOidTag hashAlg, 
					   SECOidTag *hash, void *wincx);












extern VFYContext *VFY_CreateContextWithAlgorithmID(const SECKEYPublicKey *key, 
				     const SECItem *sig,
				     const SECAlgorithmID *algid, 
				     SECOidTag *hash,
				     void *wincx);






extern void VFY_DestroyContext(VFYContext *cx, PRBool freeit);

extern SECStatus VFY_Begin(VFYContext *cx);









extern SECStatus VFY_Update(VFYContext *cx, const unsigned char *input,
			    unsigned int inputLen);








extern SECStatus VFY_End(VFYContext *cx);












extern SECStatus VFY_EndWithSignature(VFYContext *cx, SECItem *sig);















extern SECStatus VFY_VerifyDigest(SECItem *dig, SECKEYPublicKey *key,
				  SECItem *sig, SECOidTag sigAlg, void *wincx);













extern SECStatus VFY_VerifyDigestDirect(const SECItem *dig, 
					const SECKEYPublicKey *key,
					const SECItem *sig, SECOidTag pubkAlg, 
					SECOidTag hashAlg, void *wincx);














extern SECStatus VFY_VerifyDigestWithAlgorithmID(const SECItem *dig, 
				const SECKEYPublicKey *key, const SECItem *sig,
				const SECAlgorithmID *algid, SECOidTag hash,
				void *wincx);














extern SECStatus VFY_VerifyData(unsigned char *buf, int len,
				SECKEYPublicKey *key, SECItem *sig,
				SECOidTag sigAlg, void *wincx);



















extern SECStatus VFY_VerifyDataDirect(const unsigned char *buf, int len,
				      const SECKEYPublicKey *key, 
				      const SECItem *sig,
				      SECOidTag pubkAlg, SECOidTag hashAlg, 
				      SECOidTag *hash, void *wincx);














extern SECStatus VFY_VerifyDataWithAlgorithmID(const unsigned char *buf, 
				int len, const SECKEYPublicKey *key,
				 const SECItem *sig,
				const SECAlgorithmID *algid, SECOidTag *hash,
				void *wincx);


SEC_END_PROTOS

#endif 
