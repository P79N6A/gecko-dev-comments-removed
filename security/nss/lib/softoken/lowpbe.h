



#ifndef _SECPKCS5_H_
#define _SECPKCS5_H_

#include "plarena.h"
#include "secitem.h"
#include "seccomon.h"
#include "secoidt.h"
#include "hasht.h"

typedef SECItem * (* SEC_PKCS5GetPBEPassword)(void *arg);

 
typedef enum {
    pbeBitGenIDNull = 0,
    pbeBitGenCipherKey = 0x01,
    pbeBitGenCipherIV = 0x02,
    pbeBitGenIntegrityKey = 0x03
} PBEBitGenID;

typedef enum {
    NSSPKCS5_PBKDF1 = 0,
    NSSPKCS5_PBKDF2 = 1,
    NSSPKCS5_PKCS12_V2 = 2
} NSSPKCS5PBEType;

typedef struct NSSPKCS5PBEParameterStr NSSPKCS5PBEParameter;

struct NSSPKCS5PBEParameterStr {
    PRArenaPool *poolp;
    SECItem	salt;		
    SECItem	iteration;	
    SECItem	keyLength;	

    
    int		iter;
    int 	keyLen;
    int		ivLen;
    unsigned char *ivData;
    HASH_HashType hashType;
    NSSPKCS5PBEType pbeType;
    SECAlgorithmID  prfAlg;	
    PBEBitGenID	keyID;
    SECOidTag	encAlg;
    PRBool	is2KeyDES;
};


SEC_BEGIN_PROTOS









extern SECAlgorithmID *
nsspkcs5_CreateAlgorithmID(PRArenaPool *arena, SECOidTag algorithm, 
						NSSPKCS5PBEParameter *pbe);






NSSPKCS5PBEParameter *
nsspkcs5_AlgidToParam(SECAlgorithmID *algid);






NSSPKCS5PBEParameter *
nsspkcs5_NewParam(SECOidTag alg, SECItem *salt, int iterator);











extern SECItem *
nsspkcs5_CipherData(NSSPKCS5PBEParameter *, SECItem *pwitem,
		    SECItem *src, PRBool encrypt, PRBool *update);

extern SECItem *
nsspkcs5_ComputeKeyAndIV(NSSPKCS5PBEParameter *, SECItem *pwitem,
		    			SECItem *iv, PRBool faulty3DES);


extern void
nsspkcs5_DestroyPBEParameter(NSSPKCS5PBEParameter *param);

HASH_HashType HASH_FromHMACOid(SECOidTag oid);

SEC_END_PROTOS

#endif
