



#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "secport.h"
#include "hasht.h"
#include "blapit.h"
#include "hmacct.h"
#include "secerr.h"



#define MAX_HASH_BIT_COUNT_BYTES 16










#define DUPLICATE_MSB_TO_ALL(x) ( (unsigned int)( (int)(x) >> (sizeof(int)*8-1) ) )
#define DUPLICATE_MSB_TO_ALL_8(x) ( (unsigned char)(DUPLICATE_MSB_TO_ALL(x)) )



static unsigned char
constantTimeGE(unsigned int a, unsigned int b)
{
    a -= b;
    return DUPLICATE_MSB_TO_ALL(~a);
}


static unsigned char
constantTimeEQ8(unsigned char a, unsigned char b)
{
    unsigned int c = a ^ b;
    c--;
    return DUPLICATE_MSB_TO_ALL_8(c);
}
















static SECStatus
MAC(unsigned char *mdOut,
    unsigned int *mdOutLen,
    unsigned int mdOutMax,
    const SECHashObject *hashObj,
    const unsigned char *macSecret,
    unsigned int macSecretLen,
    const unsigned char *header,
    unsigned int headerLen,
    const unsigned char *data,
    unsigned int dataLen,
    unsigned int dataTotalLen,
    unsigned char isSSLv3)
{
    void *mdState = hashObj->create();
    const unsigned int mdSize = hashObj->length;
    const unsigned int mdBlockSize = hashObj->blocklength;
    






    const unsigned int mdLengthSize = mdBlockSize == 64 ? 8 : 16;

    const unsigned int sslv3PadLen = hashObj->type == HASH_AlgMD5 ? 48 : 40;

    















    unsigned int varianceBlocks = isSSLv3 ? 2 : 6;
    


    const unsigned int len = dataTotalLen + headerLen;
    

    const unsigned int maxMACBytes = len - mdSize - 1;
    
    const unsigned int numBlocks =
	(maxMACBytes + 1 + mdLengthSize + mdBlockSize - 1) / mdBlockSize;
    

    const unsigned int macEndOffset = dataLen + headerLen - mdSize;
    

    const unsigned int c = macEndOffset % mdBlockSize;
    

    const unsigned int indexA = macEndOffset / mdBlockSize;
    

    const unsigned int indexB = (macEndOffset + mdLengthSize) / mdBlockSize;
    


    unsigned int bits;
    





    unsigned int numStartingBlocks = 0;
    

    unsigned int k = 0;
    unsigned char lengthBytes[MAX_HASH_BIT_COUNT_BYTES];
    
    unsigned char hmacPad[HASH_BLOCK_LENGTH_MAX];
    unsigned char firstBlock[HASH_BLOCK_LENGTH_MAX];
    unsigned char macOut[HASH_LENGTH_MAX];
    unsigned i, j;

    

    if (numBlocks > varianceBlocks + (isSSLv3 ? 1 : 0)) {
	numStartingBlocks = numBlocks - varianceBlocks;
	k = mdBlockSize*numStartingBlocks;
    }

    bits = 8*macEndOffset;
    hashObj->begin(mdState);
    if (!isSSLv3) {
	


	bits += 8*mdBlockSize;
	memset(hmacPad, 0, mdBlockSize);
	PORT_Assert(macSecretLen <= sizeof(hmacPad));
	memcpy(hmacPad, macSecret, macSecretLen);
	for (i = 0; i < mdBlockSize; i++)
	    hmacPad[i] ^= 0x36;
	hashObj->update(mdState, hmacPad, mdBlockSize);
    }

    j = 0;
    memset(lengthBytes, 0, sizeof(lengthBytes));
    if (mdLengthSize == 16) {
	j = 8;
    }
    if (hashObj->type == HASH_AlgMD5) {
	
	for (i = 0; i < 4; i++) {
	    lengthBytes[i+j] = bits >> (8*i);
	}
    } else {
	
	for (i = 0; i < 4; i++) {
	    lengthBytes[4+i+j] = bits >> (8*(3-i));
	}
    }

    if (k > 0) {
	if (isSSLv3) {
	    



	    const unsigned int overhang = headerLen-mdBlockSize;
	    hashObj->update(mdState, header, mdBlockSize);
	    memcpy(firstBlock, header + mdBlockSize, overhang);
	    memcpy(firstBlock + overhang, data, mdBlockSize-overhang);
	    hashObj->update(mdState, firstBlock, mdBlockSize);
	    for (i = 1; i < k/mdBlockSize - 1; i++) {
		hashObj->update(mdState, data + mdBlockSize*i - overhang,
				mdBlockSize);
	    }
	} else {
	    
	    memcpy(firstBlock, header, 13);
	    memcpy(firstBlock+13, data, mdBlockSize-13);
	    hashObj->update(mdState, firstBlock, mdBlockSize);
	    for (i = 1; i < k/mdBlockSize; i++) {
		hashObj->update(mdState, data + mdBlockSize*i - 13,
				mdBlockSize);
	    }
	}
    }

    memset(macOut, 0, sizeof(macOut));

    



    for (i = numStartingBlocks; i <= numStartingBlocks+varianceBlocks; i++) {
	unsigned char block[HASH_BLOCK_LENGTH_MAX];
	unsigned char isBlockA = constantTimeEQ8(i, indexA);
	unsigned char isBlockB = constantTimeEQ8(i, indexB);
	for (j = 0; j < mdBlockSize; j++) {
	    unsigned char isPastC = isBlockA & constantTimeGE(j, c);
	    unsigned char isPastCPlus1 = isBlockA & constantTimeGE(j, c+1);
	    unsigned char b = 0;
	    if (k < headerLen) {
		b = header[k];
	    } else if (k < dataTotalLen + headerLen) {
		b = data[k-headerLen];
	    }
	    k++;

	    


	    b = (b&~isPastC) | (0x80&isPastC);
	    


	    b = b&~isPastCPlus1;
	    



	    b &= ~isBlockB | isBlockA;

	    
	    if (j >= mdBlockSize - mdLengthSize) {
		
		b = (b&~isBlockB) |
		    (isBlockB&lengthBytes[j-(mdBlockSize-mdLengthSize)]);
	    }
	    block[j] = b;
	}

	hashObj->update(mdState, block, mdBlockSize);
	hashObj->end_raw(mdState, block, NULL, mdSize);
	
	for (j = 0; j < mdSize; j++) {
	    macOut[j] |= block[j]&isBlockB;
	}
    }

    hashObj->begin(mdState);

    if (isSSLv3) {
	
	for (i = 0; i < sslv3PadLen; i++)
	    hmacPad[i] = 0x5c;

	hashObj->update(mdState, macSecret, macSecretLen);
	hashObj->update(mdState, hmacPad, sslv3PadLen);
	hashObj->update(mdState, macOut, mdSize);
    } else {
	
	for (i = 0; i < mdBlockSize; i++)
	    hmacPad[i] ^= 0x6a;

	hashObj->update(mdState, hmacPad, mdBlockSize);
	hashObj->update(mdState, macOut, mdSize);
    }

    hashObj->end(mdState, mdOut, mdOutLen, mdOutMax);
    hashObj->destroy(mdState, PR_TRUE);

    return SECSuccess;
}

SECStatus
HMAC_ConstantTime(
    unsigned char *result,
    unsigned int *resultLen,
    unsigned int maxResultLen,
    const SECHashObject *hashObj,
    const unsigned char *secret,
    unsigned int secretLen,
    const unsigned char *header,
    unsigned int headerLen,
    const unsigned char *body,
    unsigned int bodyLen,
    unsigned int bodyTotalLen)
{
    if (hashObj->end_raw == NULL)
	return SECFailure;
    return MAC(result, resultLen, maxResultLen, hashObj, secret, secretLen,
	       header, headerLen, body, bodyLen, bodyTotalLen,
	       0 );
}

SECStatus
SSLv3_MAC_ConstantTime(
    unsigned char *result,
    unsigned int *resultLen,
    unsigned int maxResultLen,
    const SECHashObject *hashObj,
    const unsigned char *secret,
    unsigned int secretLen,
    const unsigned char *header,
    unsigned int headerLen,
    const unsigned char *body,
    unsigned int bodyLen,
    unsigned int bodyTotalLen)
{
    if (hashObj->end_raw == NULL)
	return SECFailure;
    return MAC(result, resultLen, maxResultLen, hashObj, secret, secretLen,
	       header, headerLen, body, bodyLen, bodyTotalLen,
	       1 );
}

