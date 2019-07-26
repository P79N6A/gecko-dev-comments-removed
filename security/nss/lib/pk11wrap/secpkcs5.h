


#ifndef _SECPKCS5_H_
#define _SECPKCS5_H_
#include "seccomon.h"
#include "secmodt.h"


typedef enum {
    pbeBitGenIDNull = 0,
    pbeBitGenCipherKey = 0x01,
    pbeBitGenCipherIV = 0x02,
    pbeBitGenIntegrityKey = 0x03
} PBEBitGenID;

typedef struct PBEBitGenContextStr PBEBitGenContext;

SEC_BEGIN_PROTOS


SECAlgorithmID *
sec_pkcs5CreateAlgorithmID(SECOidTag algorithm, SECOidTag cipherAlgorithm,
			SECOidTag prfAlg, SECOidTag *pPbeAlgorithm,
			int keyLengh, SECItem *salt, int iteration);








SECItem *
SEC_PKCS5GetIV(SECAlgorithmID *algid, SECItem *pwitem, PRBool faulty3DES);

SECOidTag SEC_PKCS5GetCryptoAlgorithm(SECAlgorithmID *algid);
PRBool SEC_PKCS5IsAlgorithmPBEAlg(SECAlgorithmID *algid);
PRBool SEC_PKCS5IsAlgorithmPBEAlgTag(SECOidTag algTag);
SECOidTag SEC_PKCS5GetPBEAlgorithm(SECOidTag algTag, int keyLen);
int SEC_PKCS5GetKeyLength(SECAlgorithmID *algid);






PBEBitGenContext *
PBE_CreateContext(SECOidTag hashAlgorithm, PBEBitGenID bitGenPurpose,
        SECItem *pwitem, SECItem *salt, unsigned int bitsNeeded,
        unsigned int iterations);

void
PBE_DestroyContext(PBEBitGenContext *context);


SECItem *
PBE_GenerateBits(PBEBitGenContext *context);

SEC_END_PROTOS

#endif 
