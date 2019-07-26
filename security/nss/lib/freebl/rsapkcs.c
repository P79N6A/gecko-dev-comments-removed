







#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "secerr.h"

#include "blapi.h"
#include "secitem.h"
#include "blapii.h"

#define RSA_BLOCK_MIN_PAD_LEN            8
#define RSA_BLOCK_FIRST_OCTET            0x00
#define RSA_BLOCK_PRIVATE_PAD_OCTET      0xff
#define RSA_BLOCK_AFTER_PAD_OCTET        0x00








typedef enum {
    RSA_BlockUnused = 0,    
    RSA_BlockPrivate = 1,   
    RSA_BlockPublic = 2,    
    RSA_BlockRaw = 4,       
    RSA_BlockTotal
} RSA_BlockType;


static const unsigned char eightZeros[] = { 0, 0, 0, 0, 0, 0, 0, 0 };





static unsigned char constantTimeEQ8(unsigned char a, unsigned char b) {
    unsigned char c = ~((a - b) | (b - a));
    c >>= 7;
    return c;
}





static unsigned char constantTimeCompare(const unsigned char *a,
                                         const unsigned char *b,
                                         unsigned int len) {
    unsigned char tmp = 0;
    unsigned int i;
    for (i = 0; i < len; ++i, ++a, ++b)
        tmp |= *a ^ *b;
    return constantTimeEQ8(0x00, tmp);
}





static unsigned int constantTimeCondition(unsigned int c,
                                          unsigned int a,
                                          unsigned int b)
{
    return (~(c - 1) & a) | ((c - 1) & b);
}

static unsigned int
rsa_modulusLen(SECItem * modulus)
{
    unsigned char byteZero = modulus->data[0];
    unsigned int modLen = modulus->len - !byteZero;
    return modLen;
}





static unsigned char *
rsa_FormatOneBlock(unsigned modulusLen,
                   RSA_BlockType blockType,
                   SECItem * data)
{
    unsigned char *block;
    unsigned char *bp;
    int padLen;
    int i, j;
    SECStatus rv;

    block = (unsigned char *) PORT_Alloc(modulusLen);
    if (block == NULL)
        return NULL;

    bp = block;

    



    *bp++ = RSA_BLOCK_FIRST_OCTET;
    *bp++ = (unsigned char) blockType;

    switch (blockType) {

      


      case RSA_BlockPrivate:	 
        




        padLen = modulusLen - data->len - 3;
        PORT_Assert(padLen >= RSA_BLOCK_MIN_PAD_LEN);
        if (padLen < RSA_BLOCK_MIN_PAD_LEN) {
            PORT_Free(block);
            return NULL;
        }
        PORT_Memset(bp, RSA_BLOCK_PRIVATE_PAD_OCTET, padLen);
        bp += padLen;
        *bp++ = RSA_BLOCK_AFTER_PAD_OCTET;
        PORT_Memcpy(bp, data->data, data->len);
        break;

      


      case RSA_BlockPublic:
        











        padLen = modulusLen - (data->len + 3);
        PORT_Assert(padLen >= RSA_BLOCK_MIN_PAD_LEN);
        if (padLen < RSA_BLOCK_MIN_PAD_LEN) {
            PORT_Free(block);
            return NULL;
        }
        j = modulusLen - 2;
        rv = RNG_GenerateGlobalRandomBytes(bp, j);
        if (rv == SECSuccess) {
            for (i = 0; i < padLen; ) {
                unsigned char repl;
                
                if (bp[i] != RSA_BLOCK_AFTER_PAD_OCTET) {
                    ++i;
                    continue;
                }
                if (j <= padLen) {
                    rv = RNG_GenerateGlobalRandomBytes(bp + padLen,
                                          modulusLen - (2 + padLen));
                    if (rv != SECSuccess)
                        break;
                    j = modulusLen - 2;
                }
                do {
                    repl = bp[--j];
                } while (repl == RSA_BLOCK_AFTER_PAD_OCTET && j > padLen);
                if (repl != RSA_BLOCK_AFTER_PAD_OCTET) {
                    bp[i++] = repl;
                }
            }
        }
        if (rv != SECSuccess) {
            PORT_Free(block);
            PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
            return NULL;
        }
        bp += padLen;
        *bp++ = RSA_BLOCK_AFTER_PAD_OCTET;
        PORT_Memcpy(bp, data->data, data->len);
        break;

      default:
        PORT_Assert(0);
        PORT_Free(block);
        return NULL;
    }

    return block;
}

static SECStatus
rsa_FormatBlock(SECItem * result,
                unsigned modulusLen,
                RSA_BlockType blockType,
                SECItem * data)
{
    switch (blockType) {
      case RSA_BlockPrivate:
      case RSA_BlockPublic:
        





        PORT_Assert(data->len <= (modulusLen - (3 + RSA_BLOCK_MIN_PAD_LEN)));

        result->data = rsa_FormatOneBlock(modulusLen, blockType, data);
        if (result->data == NULL) {
            result->len = 0;
            return SECFailure;
        }
        result->len = modulusLen;

        break;

      case RSA_BlockRaw:
        




        if (data->len > modulusLen ) {
            return SECFailure;
        }
        result->data = (unsigned char*)PORT_ZAlloc(modulusLen);
        result->len = modulusLen;
        PORT_Memcpy(result->data + (modulusLen - data->len),
                    data->data, data->len);
        break;

      default:
        PORT_Assert(0);
        result->data = NULL;
        result->len = 0;
        return SECFailure;
    }

    return SECSuccess;
}




static SECStatus
MGF1(HASH_HashType hashAlg,
     unsigned char * mask,
     unsigned int maskLen,
     const unsigned char * mgfSeed,
     unsigned int mgfSeedLen)
{
    unsigned int digestLen;
    PRUint32 counter;
    PRUint32 rounds;
    unsigned char * tempHash;
    unsigned char * temp;
    const SECHashObject * hash;
    void * hashContext;
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
        if (counter != (rounds - 1)) {
            (*hash->end)(hashContext, tempHash, &digestLen, hash->length);
        } else { 
            temp = (unsigned char *)PORT_Alloc(hash->length);
            (*hash->end)(hashContext, temp, &digestLen, hash->length);
            PORT_Memcpy(tempHash, temp, maskLen - counter * hash->length);
            PORT_Free(temp);
        }
    }
    (*hash->destroy)(hashContext, PR_TRUE);

    return SECSuccess;
}


SECStatus
RSA_SignRaw(RSAPrivateKey * key,
            unsigned char * output,
            unsigned int * outputLen,
            unsigned int maxOutputLen,
            const unsigned char * data,
            unsigned int dataLen)
{
    SECStatus rv = SECSuccess;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    SECItem formatted;
    SECItem unformatted;

    if (maxOutputLen < modulusLen)
        return SECFailure;

    unformatted.len  = dataLen;
    unformatted.data = (unsigned char*)data;
    formatted.data   = NULL;
    rv = rsa_FormatBlock(&formatted, modulusLen, RSA_BlockRaw, &unformatted);
    if (rv != SECSuccess)
        goto done;

    rv = RSA_PrivateKeyOpDoubleChecked(key, output, formatted.data);
    *outputLen = modulusLen;

done:
    if (formatted.data != NULL)
        PORT_ZFree(formatted.data, modulusLen);
    return rv;
}


SECStatus
RSA_CheckSignRaw(RSAPublicKey * key,
                 const unsigned char * sig,
                 unsigned int sigLen,
                 const unsigned char * hash,
                 unsigned int hashLen)
{
    SECStatus rv;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    unsigned char * buffer;

    if (sigLen != modulusLen)
        goto failure;
    if (hashLen > modulusLen)
        goto failure;

    buffer = (unsigned char *)PORT_Alloc(modulusLen + 1);
    if (!buffer)
        goto failure;

    rv = RSA_PublicKeyOp(key, buffer, sig);
    if (rv != SECSuccess)
        goto loser;

    


    
    
    if (PORT_Memcmp(buffer + (modulusLen - hashLen), hash, hashLen) != 0)
        goto loser;

    PORT_Free(buffer);
    return SECSuccess;

loser:
    PORT_Free(buffer);
failure:
    return SECFailure;
}


SECStatus
RSA_CheckSignRecoverRaw(RSAPublicKey * key,
                        unsigned char * data,
                        unsigned int * dataLen,
                        unsigned int maxDataLen,
                        const unsigned char * sig,
                        unsigned int sigLen)
{
    SECStatus rv;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);

    if (sigLen != modulusLen)
        goto failure;
    if (maxDataLen < modulusLen)
        goto failure;

    rv = RSA_PublicKeyOp(key, data, sig);
    if (rv != SECSuccess)
        goto failure;

    *dataLen = modulusLen;
    return SECSuccess;

failure:
    return SECFailure;
}


SECStatus
RSA_EncryptRaw(RSAPublicKey * key,
               unsigned char * output,
               unsigned int * outputLen,
               unsigned int maxOutputLen,
               const unsigned char * input,
               unsigned int inputLen)
{
    SECStatus rv;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    SECItem formatted;
    SECItem unformatted;

    formatted.data = NULL;
    if (maxOutputLen < modulusLen)
        goto failure;

    unformatted.len  = inputLen;
    unformatted.data = (unsigned char*)input;
    formatted.data   = NULL;
    rv = rsa_FormatBlock(&formatted, modulusLen, RSA_BlockRaw, &unformatted);
    if (rv != SECSuccess)
        goto failure;

    rv = RSA_PublicKeyOp(key, output, formatted.data);
    if (rv != SECSuccess)
        goto failure;

    PORT_ZFree(formatted.data, modulusLen);
    *outputLen = modulusLen;
    return SECSuccess;

failure:
    if (formatted.data != NULL)
        PORT_ZFree(formatted.data, modulusLen);
    return SECFailure;
}


SECStatus
RSA_DecryptRaw(RSAPrivateKey * key,
               unsigned char * output,
               unsigned int * outputLen,
               unsigned int maxOutputLen,
               const unsigned char * input,
               unsigned int inputLen)
{
    SECStatus rv;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);

    if (modulusLen > maxOutputLen)
        goto failure;
    if (inputLen != modulusLen)
        goto failure;

    rv = RSA_PrivateKeyOp(key, output, input);
    if (rv != SECSuccess)
        goto failure;

    *outputLen = modulusLen;
    return SECSuccess;

failure:
    return SECFailure;
}










static SECStatus
eme_oaep_decode(unsigned char * output,
                unsigned int * outputLen,
                unsigned int maxOutputLen,
                const unsigned char * input,
                unsigned int inputLen,
                HASH_HashType hashAlg,
                HASH_HashType maskHashAlg,
                const unsigned char * label,
                unsigned int labelLen)
{
    const SECHashObject * hash;
    void * hashContext;
    SECStatus rv = SECFailure;
    unsigned char labelHash[HASH_LENGTH_MAX];
    unsigned int i;
    unsigned int maskLen;
    unsigned int paddingOffset;
    unsigned char * mask = NULL;
    unsigned char * tmpOutput = NULL;
    unsigned char isGood;
    unsigned char foundPaddingEnd;

    hash = HASH_GetRawHashObject(hashAlg);

    
    if (inputLen < (hash->length * 2) + 2) {
        PORT_SetError(SEC_ERROR_INPUT_LEN);
        return SECFailure;
    }

    
    hashContext = (*hash->create)();
    if (hashContext == NULL) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    (*hash->begin)(hashContext);
    if (labelLen > 0)
        (*hash->update)(hashContext, label, labelLen);
    (*hash->end)(hashContext, labelHash, &i, sizeof(labelHash));
    (*hash->destroy)(hashContext, PR_TRUE);

    tmpOutput = (unsigned char*)PORT_Alloc(inputLen);
    if (tmpOutput == NULL) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto done;
    }

    maskLen = inputLen - hash->length - 1;
    mask = (unsigned char*)PORT_Alloc(maskLen);
    if (mask == NULL) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto done;
    }

    PORT_Memcpy(tmpOutput, input, inputLen);

    
    MGF1(maskHashAlg, mask, hash->length, &tmpOutput[1 + hash->length],
         inputLen - hash->length - 1);
    
    for (i = 0; i < hash->length; ++i)
        tmpOutput[1 + i] ^= mask[i];

    
    MGF1(maskHashAlg, mask, maskLen, &tmpOutput[1], hash->length);
    
    for (i = 0; i < maskLen; ++i)
        tmpOutput[1 + hash->length + i] ^= mask[i];

    



    paddingOffset = 0;
    isGood = 1;
    foundPaddingEnd = 0;

    
    isGood &= constantTimeEQ8(0x00, tmpOutput[0]);

    
    isGood &= constantTimeCompare(&labelHash[0],
                                  &tmpOutput[1 + hash->length],
                                  hash->length);

    

    for (i = 1 + (hash->length * 2); i < inputLen; ++i) {
        unsigned char isZero = constantTimeEQ8(0x00, tmpOutput[i]);
        unsigned char isOne = constantTimeEQ8(0x01, tmpOutput[i]);
        



        paddingOffset = constantTimeCondition(isOne & ~foundPaddingEnd, i,
                                              paddingOffset);
        








        foundPaddingEnd = constantTimeCondition(isOne, 1, foundPaddingEnd);
        









        isGood = constantTimeCondition(~foundPaddingEnd & ~isZero, 0, isGood);
    }

    




    if (!(isGood & foundPaddingEnd)) {
        PORT_SetError(SEC_ERROR_BAD_DATA);
        goto done;
    }

    

    ++paddingOffset; 

    *outputLen = inputLen - paddingOffset;
    if (*outputLen > maxOutputLen) {
        PORT_SetError(SEC_ERROR_OUTPUT_LEN);
        goto done;
    }

    if (*outputLen)
        PORT_Memcpy(output, &tmpOutput[paddingOffset], *outputLen);
    rv = SECSuccess;

done:
    if (mask)
        PORT_ZFree(mask, maskLen);
    if (tmpOutput)
        PORT_ZFree(tmpOutput, inputLen);
    return rv;
}







static SECStatus
eme_oaep_encode(unsigned char * em,
                unsigned int emLen,
                const unsigned char * input,
                unsigned int inputLen,
                HASH_HashType hashAlg,
                HASH_HashType maskHashAlg,
                const unsigned char * label,
                unsigned int labelLen,
                const unsigned char * seed,
                unsigned int seedLen)
{
    const SECHashObject * hash;
    void * hashContext;
    SECStatus rv;
    unsigned char * mask;
    unsigned int reservedLen;
    unsigned int dbMaskLen;
    unsigned int i;

    hash = HASH_GetRawHashObject(hashAlg);
    PORT_Assert(seed == NULL || seedLen == hash->length);

    
    reservedLen = (2 * hash->length) + 2;
    if (emLen < reservedLen || inputLen > (emLen - reservedLen)) {
        PORT_SetError(SEC_ERROR_INPUT_LEN);
        return SECFailure;
    }

    





















    *em = 0x00;

    
    hashContext = (*hash->create)();
    if (hashContext == NULL) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    (*hash->begin)(hashContext);
    if (labelLen > 0)
        (*hash->update)(hashContext, label, labelLen);
    (*hash->end)(hashContext, &em[1 + hash->length], &i, hash->length);
    (*hash->destroy)(hashContext, PR_TRUE);

    
    if (emLen - reservedLen - inputLen > 0) {
        PORT_Memset(em + 1 + (hash->length * 2), 0x00,
                    emLen - reservedLen - inputLen);
    }

    




    em[emLen - inputLen - 1] = 0x01;
    if (inputLen)
        PORT_Memcpy(em + emLen - inputLen, input, inputLen);

    if (seed == NULL) {
        
        rv = RNG_GenerateGlobalRandomBytes(em + 1, hash->length);
        if (rv != SECSuccess) {
            return rv;
        }
    } else {
        
        PORT_Memcpy(em + 1, seed, seedLen);
    }

    
    dbMaskLen = emLen - hash->length - 1;
    mask = (unsigned char*)PORT_Alloc(dbMaskLen);
    if (mask == NULL) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    MGF1(maskHashAlg, mask, dbMaskLen, em + 1, hash->length);
    
    for (i = 0; i < dbMaskLen; ++i)
        em[1 + hash->length + i] ^= mask[i];

    
    MGF1(maskHashAlg, mask, hash->length, &em[1 + hash->length], dbMaskLen);
    
    for (i = 0; i < hash->length; ++i)
        em[1 + i] ^= mask[i];

    PORT_ZFree(mask, dbMaskLen);
    return SECSuccess;
}

SECStatus
RSA_EncryptOAEP(RSAPublicKey * key,
                HASH_HashType hashAlg,
                HASH_HashType maskHashAlg,
                const unsigned char * label,
                unsigned int labelLen,
                const unsigned char * seed,
                unsigned int seedLen,
                unsigned char * output,
                unsigned int * outputLen,
                unsigned int maxOutputLen,
                const unsigned char * input,
                unsigned int inputLen)
{
    SECStatus rv = SECFailure;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    unsigned char * oaepEncoded = NULL;

    if (maxOutputLen < modulusLen) {
        PORT_SetError(SEC_ERROR_OUTPUT_LEN);
        return SECFailure;
    }

    if ((hashAlg == HASH_AlgNULL) || (maskHashAlg == HASH_AlgNULL)) {
        PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
        return SECFailure;
    }

    if ((labelLen == 0 && label != NULL) ||
        (labelLen > 0 && label == NULL)) {
        PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
        return SECFailure;
    }

    oaepEncoded = (unsigned char *)PORT_Alloc(modulusLen);
    if (oaepEncoded == NULL) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    rv = eme_oaep_encode(oaepEncoded, modulusLen, input, inputLen,
                         hashAlg, maskHashAlg, label, labelLen, seed, seedLen);
    if (rv != SECSuccess)
        goto done;

    rv = RSA_PublicKeyOp(key, output, oaepEncoded);
    if (rv != SECSuccess)
        goto done;
    *outputLen = modulusLen;

done:
    PORT_Free(oaepEncoded);
    return rv;
}

SECStatus
RSA_DecryptOAEP(RSAPrivateKey * key,
                HASH_HashType hashAlg,
                HASH_HashType maskHashAlg,
                const unsigned char * label,
                unsigned int labelLen,
                unsigned char * output,
                unsigned int * outputLen,
                unsigned int maxOutputLen,
                const unsigned char * input,
                unsigned int inputLen)
{
    SECStatus rv = SECFailure;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    unsigned char * oaepEncoded = NULL;

    if ((hashAlg == HASH_AlgNULL) || (maskHashAlg == HASH_AlgNULL)) {
        PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
        return SECFailure;
    }

    if (inputLen != modulusLen) {
        PORT_SetError(SEC_ERROR_INPUT_LEN);
        return SECFailure;
    }

    if ((labelLen == 0 && label != NULL) ||
        (labelLen > 0 && label == NULL)) {
        PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
        return SECFailure;
    }

    oaepEncoded = (unsigned char *)PORT_Alloc(modulusLen);
    if (oaepEncoded == NULL) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }

    rv = RSA_PrivateKeyOpDoubleChecked(key, oaepEncoded, input);
    if (rv != SECSuccess) {
        goto done;
    }
    rv = eme_oaep_decode(output, outputLen, maxOutputLen, oaepEncoded,
                         modulusLen, hashAlg, maskHashAlg, label,
                         labelLen);

done:
    if (oaepEncoded)
        PORT_ZFree(oaepEncoded, modulusLen);
    return rv;
}


SECStatus
RSA_EncryptBlock(RSAPublicKey * key,
                 unsigned char * output,
                 unsigned int * outputLen,
                 unsigned int maxOutputLen,
                 const unsigned char * input,
                 unsigned int inputLen)
{
    SECStatus rv;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    SECItem formatted;
    SECItem unformatted;

    formatted.data = NULL;
    if (maxOutputLen < modulusLen)
        goto failure;

    unformatted.len  = inputLen;
    unformatted.data = (unsigned char*)input;
    formatted.data   = NULL;
    rv = rsa_FormatBlock(&formatted, modulusLen, RSA_BlockPublic,
                         &unformatted);
    if (rv != SECSuccess)
        goto failure;

    rv = RSA_PublicKeyOp(key, output, formatted.data);
    if (rv != SECSuccess)
        goto failure;

    PORT_ZFree(formatted.data, modulusLen);
    *outputLen = modulusLen;
    return SECSuccess;

failure:
    if (formatted.data != NULL)
        PORT_ZFree(formatted.data, modulusLen);
    return SECFailure;
}


SECStatus
RSA_DecryptBlock(RSAPrivateKey * key,
                 unsigned char * output,
                 unsigned int * outputLen,
                 unsigned int maxOutputLen,
                 const unsigned char * input,
                 unsigned int inputLen)
{
    SECStatus rv;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    unsigned int i;
    unsigned char * buffer;

    if (inputLen != modulusLen)
        goto failure;

    buffer = (unsigned char *)PORT_Alloc(modulusLen + 1);
    if (!buffer)
        goto failure;

    rv = RSA_PrivateKeyOp(key, buffer, input);
    if (rv != SECSuccess)
        goto loser;

    
    if (buffer[0] != RSA_BLOCK_FIRST_OCTET ||
        buffer[1] != (unsigned char)RSA_BlockPublic) {
        goto loser;
    }
    *outputLen = 0;
    for (i = 2; i < modulusLen; i++) {
        if (buffer[i] == RSA_BLOCK_AFTER_PAD_OCTET) {
            *outputLen = modulusLen - i - 1;
            break;
        }
    }
    if (*outputLen == 0)
        goto loser;
    if (*outputLen > maxOutputLen)
        goto loser;

    PORT_Memcpy(output, buffer + modulusLen - *outputLen, *outputLen);

    PORT_Free(buffer);
    return SECSuccess;

loser:
    PORT_Free(buffer);
failure:
    return SECFailure;
}










static SECStatus
emsa_pss_encode(unsigned char * em,
                unsigned int emLen,
                const unsigned char * mHash,
                HASH_HashType hashAlg,
                HASH_HashType maskHashAlg,
                const unsigned char * salt,
                unsigned int saltLen)
{
    const SECHashObject * hash;
    void * hash_context;
    unsigned char * dbMask;
    unsigned int dbMaskLen;
    unsigned int i;
    SECStatus rv;

    hash = HASH_GetRawHashObject(hashAlg);
    dbMaskLen = emLen - hash->length - 1;

    
    if (emLen < hash->length + saltLen + 2) {
        PORT_SetError(SEC_ERROR_OUTPUT_LEN);
        return SECFailure;
    }

    
    if (salt == NULL) {
        rv = RNG_GenerateGlobalRandomBytes(&em[dbMaskLen - saltLen], saltLen);
        if (rv != SECSuccess) {
            return rv;
        }
    } else {
        PORT_Memcpy(&em[dbMaskLen - saltLen], salt, saltLen);
    }

    
    
    hash_context = (*hash->create)();
    if (hash_context == NULL) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    (*hash->begin)(hash_context);
    (*hash->update)(hash_context, eightZeros, 8);
    (*hash->update)(hash_context, mHash, hash->length);
    (*hash->update)(hash_context, &em[dbMaskLen - saltLen], saltLen);
    (*hash->end)(hash_context, &em[dbMaskLen], &i, hash->length);
    (*hash->destroy)(hash_context, PR_TRUE);

    
    PORT_Memset(em, 0, dbMaskLen - saltLen - 1);
    em[dbMaskLen - saltLen - 1] = 0x01;

    
    dbMask = (unsigned char *)PORT_Alloc(dbMaskLen);
    if (dbMask == NULL) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    MGF1(maskHashAlg, dbMask, dbMaskLen, &em[dbMaskLen], hash->length);

    
    for (i = 0; i < dbMaskLen; i++)
        em[i] ^= dbMask[i];
    PORT_Free(dbMask);

    
    em[0] &= 0x7f;

    
    em[emLen - 1] = 0xbc;

    return SECSuccess;
}










static SECStatus
emsa_pss_verify(const unsigned char * mHash,
                const unsigned char * em,
                unsigned int emLen,
                HASH_HashType hashAlg,
                HASH_HashType maskHashAlg,
                unsigned int saltLen)
{
    const SECHashObject * hash;
    void * hash_context;
    unsigned char * db;
    unsigned char * H_;  
    unsigned int i;
    unsigned int dbMaskLen;
    SECStatus rv;

    hash = HASH_GetRawHashObject(hashAlg);
    dbMaskLen = emLen - hash->length - 1;

    
    if ((emLen < (hash->length + saltLen + 2)) ||
        (em[emLen - 1] != 0xbc) ||
        ((em[0] & 0x80) != 0)) {
        PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
        return SECFailure;
    }

    
    db = (unsigned char *)PORT_Alloc(dbMaskLen);
    if (db == NULL) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    
    MGF1(maskHashAlg, db, dbMaskLen, &em[dbMaskLen], hash->length);

    
    for (i = 0; i < dbMaskLen; i++) {
        db[i] ^= em[i];
    }

    
    db[0] &= 0x7f;

    
    for (i = 0; i < (dbMaskLen - saltLen - 1); i++) {
        if (db[i] != 0) {
            PORT_Free(db);
            PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
            return SECFailure;
        }
    }
    if (db[dbMaskLen - saltLen - 1] != 0x01) {
        PORT_Free(db);
        PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
        return SECFailure;
    }

    
    H_ = (unsigned char *)PORT_Alloc(hash->length);
    if (H_ == NULL) {
        PORT_Free(db);
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    hash_context = (*hash->create)();
    if (hash_context == NULL) {
        PORT_Free(db);
        PORT_Free(H_);
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    (*hash->begin)(hash_context);
    (*hash->update)(hash_context, eightZeros, 8);
    (*hash->update)(hash_context, mHash, hash->length);
    (*hash->update)(hash_context, &db[dbMaskLen - saltLen], saltLen);
    (*hash->end)(hash_context, H_, &i, hash->length);
    (*hash->destroy)(hash_context, PR_TRUE);

    PORT_Free(db);

    
    if (PORT_Memcmp(H_, &em[dbMaskLen], hash->length) != 0) {
        PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
        rv = SECFailure;
    } else {
        rv = SECSuccess;
    }

    PORT_Free(H_);
    return rv;
}

SECStatus
RSA_SignPSS(RSAPrivateKey * key,
            HASH_HashType hashAlg,
            HASH_HashType maskHashAlg,
            const unsigned char * salt,
            unsigned int saltLength,
            unsigned char * output,
            unsigned int * outputLen,
            unsigned int maxOutputLen,
            const unsigned char * input,
            unsigned int inputLen)
{
    SECStatus rv = SECSuccess;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    unsigned char *pssEncoded = NULL;

    if (maxOutputLen < modulusLen) {
        PORT_SetError(SEC_ERROR_OUTPUT_LEN);
        return SECFailure;
    }

    if ((hashAlg == HASH_AlgNULL) || (maskHashAlg == HASH_AlgNULL)) {
        PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
        return SECFailure;
    }

    pssEncoded = (unsigned char *)PORT_Alloc(modulusLen);
    if (pssEncoded == NULL) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    rv = emsa_pss_encode(pssEncoded, modulusLen, input, hashAlg,
                         maskHashAlg, salt, saltLength);
    if (rv != SECSuccess)
        goto done;

    rv = RSA_PrivateKeyOpDoubleChecked(key, output, pssEncoded);
    *outputLen = modulusLen;

done:
    PORT_Free(pssEncoded);
    return rv;
}

SECStatus
RSA_CheckSignPSS(RSAPublicKey * key,
                 HASH_HashType hashAlg,
                 HASH_HashType maskHashAlg,
                 unsigned int saltLength,
                 const unsigned char * sig,
                 unsigned int sigLen,
                 const unsigned char * hash,
                 unsigned int hashLen)
{
    SECStatus rv;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    unsigned char * buffer;

    if (sigLen != modulusLen) {
        PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
        return SECFailure;
    }

    if ((hashAlg == HASH_AlgNULL) || (maskHashAlg == HASH_AlgNULL)) {
        PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
        return SECFailure;
    }

    buffer = (unsigned char *)PORT_Alloc(modulusLen);
    if (!buffer) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }

    rv = RSA_PublicKeyOp(key, buffer, sig);
    if (rv != SECSuccess) {
        PORT_Free(buffer);
        PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
        return SECFailure;
    }

    rv = emsa_pss_verify(hash, buffer, modulusLen, hashAlg,
                         maskHashAlg, saltLength);
    PORT_Free(buffer);

    return rv;
}


SECStatus
RSA_Sign(RSAPrivateKey * key,
         unsigned char * output,
         unsigned int * outputLen,
         unsigned int maxOutputLen,
         const unsigned char * input,
         unsigned int inputLen)
{
    SECStatus rv = SECSuccess;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    SECItem formatted;
    SECItem unformatted;

    if (maxOutputLen < modulusLen)
        return SECFailure;

    unformatted.len  = inputLen;
    unformatted.data = (unsigned char*)input;
    formatted.data   = NULL;
    rv = rsa_FormatBlock(&formatted, modulusLen, RSA_BlockPrivate,
                         &unformatted);
    if (rv != SECSuccess)
        goto done;

    rv = RSA_PrivateKeyOpDoubleChecked(key, output, formatted.data);
    *outputLen = modulusLen;

    goto done;

done:
    if (formatted.data != NULL)
        PORT_ZFree(formatted.data, modulusLen);
    return rv;
}


SECStatus
RSA_CheckSign(RSAPublicKey * key,
              const unsigned char * sig,
              unsigned int sigLen,
              const unsigned char * data,
              unsigned int dataLen)
{
    SECStatus rv;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    unsigned int i;
    unsigned char * buffer;

    if (sigLen != modulusLen)
        goto failure;
    





    if (dataLen > modulusLen - (3 + RSA_BLOCK_MIN_PAD_LEN))
        goto failure;

    buffer = (unsigned char *)PORT_Alloc(modulusLen + 1);
    if (!buffer)
        goto failure;

    rv = RSA_PublicKeyOp(key, buffer, sig);
    if (rv != SECSuccess)
        goto loser;

    


    if (buffer[0] != RSA_BLOCK_FIRST_OCTET ||
        buffer[1] != (unsigned char)RSA_BlockPrivate) {
        goto loser;
    }
    for (i = 2; i < modulusLen - dataLen - 1; i++) {
        if (buffer[i] != RSA_BLOCK_PRIVATE_PAD_OCTET)
            goto loser;
    }
    if (buffer[i] != RSA_BLOCK_AFTER_PAD_OCTET)
        goto loser;

    


    if (PORT_Memcmp(buffer + modulusLen - dataLen, data, dataLen) != 0)
        goto loser;

    PORT_Free(buffer);
    return SECSuccess;

loser:
    PORT_Free(buffer);
failure:
    return SECFailure;
}


SECStatus
RSA_CheckSignRecover(RSAPublicKey * key,
                     unsigned char * output,
                     unsigned int * outputLen,
                     unsigned int maxOutputLen,
                     const unsigned char * sig,
                     unsigned int sigLen)
{
    SECStatus rv;
    unsigned int modulusLen = rsa_modulusLen(&key->modulus);
    unsigned int i;
    unsigned char * buffer;

    if (sigLen != modulusLen)
        goto failure;

    buffer = (unsigned char *)PORT_Alloc(modulusLen + 1);
    if (!buffer)
        goto failure;

    rv = RSA_PublicKeyOp(key, buffer, sig);
    if (rv != SECSuccess)
        goto loser;
    *outputLen = 0;

    


    if (buffer[0] != RSA_BLOCK_FIRST_OCTET ||
        buffer[1] != (unsigned char)RSA_BlockPrivate) {
        goto loser;
    }
    for (i = 2; i < modulusLen; i++) {
        if (buffer[i] == RSA_BLOCK_AFTER_PAD_OCTET) {
            *outputLen = modulusLen - i - 1;
            break;
        }
        if (buffer[i] != RSA_BLOCK_PRIVATE_PAD_OCTET)
            goto loser;
    }
    if (*outputLen == 0)
        goto loser;
    if (*outputLen > maxOutputLen)
        goto loser;

    PORT_Memcpy(output, buffer + modulusLen - *outputLen, *outputLen);

    PORT_Free(buffer);
    return SECSuccess;

loser:
    PORT_Free(buffer);
failure:
    return SECFailure;
}
