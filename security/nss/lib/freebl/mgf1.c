








































#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "blapi.h"
#include "hasht.h"

SECStatus
MGF1(HASH_HashType hashAlg, unsigned char *mask, unsigned int maskLen,
     const unsigned char *mgfSeed, unsigned int mgfSeedLen)
{
    unsigned int digestLen;
    PRUint32 counter, rounds;
    unsigned char *tempHash, *temp;
    const SECHashObject *hash;
    void *hashContext;
    unsigned char C[4];

    hash = HASH_GetRawHashObject(hashAlg);
    if (hash == NULL)
        return SECFailure;

    hashContext = (*hash->create)();
    rounds = (maskLen + hash->length - 1) / hash->length;
    for (counter = 0; counter < rounds; counter++) {
        C[0] = (unsigned char)((counter >> 24) & 0xff);
        C[1] = (unsigned char)((counter >> 16) & 0xff);
        C[2] = (unsigned char)((counter >> 8) & 0xff);
        C[3] = (unsigned char)(counter & 0xff);

        

        (*hash->begin)(hashContext);
        (*hash->update)(hashContext, mgfSeed, mgfSeedLen); 
        (*hash->update)(hashContext, C, sizeof C);

        tempHash = mask + counter * hash->length;
        if (counter != (rounds-1)) {
            (*hash->end)(hashContext, tempHash, &digestLen, hash->length);
        } else { 
            temp = PORT_Alloc(hash->length);
            (*hash->end)(hashContext, temp, &digestLen, hash->length);
            PORT_Memcpy(tempHash, temp, maskLen - counter * hash->length);
            PORT_Free(temp);
        }
    }
    (*hash->destroy)(hashContext, PR_TRUE);

    return SECSuccess;
}
