








































#include "blapi.h"
#include "softoken.h"
#include "sechash.h"

#include "lowkeyi.h"
#include "secerr.h"

#define RSA_BLOCK_MIN_PAD_LEN		8
#define RSA_BLOCK_FIRST_OCTET		0x00
#define RSA_BLOCK_PRIVATE0_PAD_OCTET	0x00
#define RSA_BLOCK_PRIVATE_PAD_OCTET	0xff
#define RSA_BLOCK_AFTER_PAD_OCTET	0x00

#define OAEP_SALT_LEN		8
#define OAEP_PAD_LEN		8
#define OAEP_PAD_OCTET		0x00

#define FLAT_BUFSIZE 512	/* bytes to hold flattened SHA1Context. */

static SHA1Context *
SHA1_CloneContext(SHA1Context *original)
{
    SHA1Context *  clone	= NULL;
    unsigned char *pBuf;
    int            sha1ContextSize = SHA1_FlattenSize(original);
    SECStatus      frv;
    unsigned char  buf[FLAT_BUFSIZE];

    PORT_Assert(sizeof buf >= sha1ContextSize);
    if (sizeof buf >= sha1ContextSize) {
    	pBuf = buf;
    } else {
        pBuf = PORT_Alloc(sha1ContextSize);
	if (!pBuf)
	    goto done;
    }

    frv = SHA1_Flatten(original, pBuf);
    if (frv == SECSuccess) {
	clone = SHA1_Resurrect(pBuf, NULL);
	memset(pBuf, 0, sha1ContextSize);
    }
done:
    if (pBuf != buf)
    	PORT_Free(pBuf);
    return clone;
}




static SECStatus
oaep_xor_with_h1(unsigned char *data, unsigned int datalen,
		 unsigned char *salt, unsigned int saltlen)
{
    SHA1Context *sha1cx;
    unsigned char *dp, *dataend;
    unsigned char end_octet;

    sha1cx = SHA1_NewContext();
    if (sha1cx == NULL) {
	return SECFailure;
    }

    



    SHA1_Begin (sha1cx);
    SHA1_Update (sha1cx, salt, saltlen);
    end_octet = 0;

    dp = data;
    dataend = data + datalen;

    while (dp < dataend) {
	SHA1Context *sha1cx_h1;
	unsigned int sha1len, sha1off;
	unsigned char sha1[SHA1_LENGTH];

	


	sha1cx_h1 = SHA1_CloneContext (sha1cx);
	SHA1_Update (sha1cx_h1, &end_octet, 1);
	SHA1_End (sha1cx_h1, sha1, &sha1len, sizeof(sha1));
	SHA1_DestroyContext (sha1cx_h1, PR_TRUE);
	PORT_Assert (sha1len == SHA1_LENGTH);

	




	sha1off = 0;
	if ((dataend - dp) < SHA1_LENGTH)
	    sha1off = SHA1_LENGTH - (dataend - dp);
	while (sha1off < SHA1_LENGTH)
	    *dp++ ^= sha1[sha1off++];

	


	end_octet++;
    }

    SHA1_DestroyContext (sha1cx, PR_TRUE);
    return SECSuccess;
}




static SECStatus
oaep_xor_with_h2(unsigned char *salt, unsigned int saltlen,
		 unsigned char *data, unsigned int datalen)
{
    unsigned char sha1[SHA1_LENGTH];
    unsigned char *psalt, *psha1, *saltend;
    SECStatus rv;

    


    rv = SHA1_HashBuf (sha1, data, datalen);
    if (rv != SECSuccess) {
	return rv;
    }

    


    PORT_Assert (saltlen <= SHA1_LENGTH);
    saltend = salt + saltlen;
    psalt = salt;
    psha1 = sha1 + SHA1_LENGTH - saltlen;
    while (psalt < saltend) {
	*psalt++ ^= *psha1++;
    }

    return SECSuccess;
}





static unsigned char *
rsa_FormatOneBlock(unsigned modulusLen, RSA_BlockType blockType,
		   SECItem *data)
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

      


      case RSA_BlockPrivate0: 
      case RSA_BlockPrivate:	 
	




	padLen = modulusLen - data->len - 3;
	PORT_Assert (padLen >= RSA_BLOCK_MIN_PAD_LEN);
	if (padLen < RSA_BLOCK_MIN_PAD_LEN) {
	    PORT_Free (block);
	    return NULL;
	}
	PORT_Memset (bp,
		   blockType == RSA_BlockPrivate0
			? RSA_BLOCK_PRIVATE0_PAD_OCTET
			: RSA_BLOCK_PRIVATE_PAD_OCTET,
		   padLen);
	bp += padLen;
	*bp++ = RSA_BLOCK_AFTER_PAD_OCTET;
	PORT_Memcpy (bp, data->data, data->len);
	break;

      


      case RSA_BlockPublic:

	











	padLen = modulusLen - (data->len + 3);
	PORT_Assert (padLen >= RSA_BLOCK_MIN_PAD_LEN);
	if (padLen < RSA_BLOCK_MIN_PAD_LEN) {
	    PORT_Free (block);
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
	    sftk_fatalError = PR_TRUE;
	    PORT_Free (block);
	    return NULL;
	}
	bp += padLen;
	*bp++ = RSA_BLOCK_AFTER_PAD_OCTET;
	PORT_Memcpy (bp, data->data, data->len);
	break;

      



      case RSA_BlockOAEP:
	




















	


	rv = RNG_GenerateGlobalRandomBytes(bp, OAEP_SALT_LEN);
	if (rv != SECSuccess) {
	    sftk_fatalError = PR_TRUE;
	    PORT_Free (block);
	    return NULL;
	}
	bp += OAEP_SALT_LEN;

	


	PORT_Memset (bp, OAEP_PAD_OCTET, OAEP_PAD_LEN);
	bp += OAEP_PAD_LEN;

	


	PORT_Memcpy (bp, data->data, data->len);
	bp += data->len;

	


	if (bp < (block + modulusLen)) {
	    rv = RNG_GenerateGlobalRandomBytes(bp, block - bp + modulusLen);
	    if (rv != SECSuccess) {
		sftk_fatalError = PR_TRUE;
		PORT_Free (block);
		return NULL;
	    }
	}

	







	if (oaep_xor_with_h1(block + 2 + OAEP_SALT_LEN,
			     modulusLen - 2 - OAEP_SALT_LEN,
			     block + 2, OAEP_SALT_LEN) != SECSuccess) {
	    PORT_Free (block);
	    return NULL;
	}

	





	if (oaep_xor_with_h2(block + 2, OAEP_SALT_LEN,
			     block + 2 + OAEP_SALT_LEN,
			     modulusLen - 2 - OAEP_SALT_LEN) != SECSuccess) {
	    PORT_Free (block);
	    return NULL;
	}

	break;

      default:
	PORT_Assert (0);
	PORT_Free (block);
	return NULL;
    }

    return block;
}

static SECStatus
rsa_FormatBlock(SECItem *result, unsigned modulusLen,
		RSA_BlockType blockType, SECItem *data)
{
    












    switch (blockType) {
      case RSA_BlockPrivate0:
      case RSA_BlockPrivate:
      case RSA_BlockPublic:
	





	PORT_Assert (data->len <= (modulusLen - (3 + RSA_BLOCK_MIN_PAD_LEN)));

	result->data = rsa_FormatOneBlock(modulusLen, blockType, data);
	if (result->data == NULL) {
	    result->len = 0;
	    return SECFailure;
	}
	result->len = modulusLen;

	break;

      case RSA_BlockOAEP:
	






	PORT_Assert (data->len <= (modulusLen - (2 + OAEP_SALT_LEN
						 + OAEP_PAD_LEN)));

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
	PORT_Memcpy(result->data+(modulusLen-data->len),data->data,data->len);
	break;

      default:
	PORT_Assert (0);
	result->data = NULL;
	result->len = 0;
	return SECFailure;
    }

    return SECSuccess;
}


SECStatus
RSA_Sign(NSSLOWKEYPrivateKey *key, 
         unsigned char *      output, 
	 unsigned int *       output_len,
         unsigned int         maxOutputLen, 
	 unsigned char *      input, 
	 unsigned int         input_len)
{
    SECStatus     rv          = SECSuccess;
    unsigned int  modulus_len = nsslowkey_PrivateModulusLen(key);
    SECItem       formatted;
    SECItem       unformatted;

    if (maxOutputLen < modulus_len) 
    	return SECFailure;
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey)
    	return SECFailure;

    unformatted.len  = input_len;
    unformatted.data = input;
    formatted.data   = NULL;
    rv = rsa_FormatBlock(&formatted, modulus_len, RSA_BlockPrivate,
			 &unformatted);
    if (rv != SECSuccess) 
    	goto done;

    rv = RSA_PrivateKeyOpDoubleChecked(&key->u.rsa, output, formatted.data);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
	sftk_fatalError = PR_TRUE;
    }
    *output_len = modulus_len;

    goto done;

done:
    if (formatted.data != NULL) 
    	PORT_ZFree(formatted.data, modulus_len);
    return rv;
}


SECStatus
RSA_CheckSign(NSSLOWKEYPublicKey *key,
              unsigned char *     sign, 
	      unsigned int        sign_len, 
	      unsigned char *     hash, 
	      unsigned int        hash_len)
{
    SECStatus       rv;
    unsigned int    modulus_len = nsslowkey_PublicModulusLen(key);
    unsigned int    i;
    unsigned char * buffer;

    modulus_len = nsslowkey_PublicModulusLen(key);
    if (sign_len != modulus_len) 
    	goto failure;
    





    if (hash_len > modulus_len - (3 + RSA_BLOCK_MIN_PAD_LEN)) 
    	goto failure;
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey)
    	goto failure;

    buffer = (unsigned char *)PORT_Alloc(modulus_len + 1);
    if (!buffer)
    	goto failure;

    rv = RSA_PublicKeyOp(&key->u.rsa, buffer, sign);
    if (rv != SECSuccess)
	goto loser;

    


    if (buffer[0] != 0 || buffer[1] != 1) 
    	goto loser;
    for (i = 2; i < modulus_len - hash_len - 1; i++) {
	if (buffer[i] != 0xff) 
	    goto loser;
    }
    if (buffer[i] != 0) 
	goto loser;

    


    if (PORT_Memcmp(buffer + modulus_len - hash_len, hash, hash_len) != 0)
	goto loser;

    PORT_Free(buffer);
    return SECSuccess;

loser:
    PORT_Free(buffer);
failure:
    return SECFailure;
}


SECStatus
RSA_CheckSignRecover(NSSLOWKEYPublicKey *key,
                     unsigned char *     data,
                     unsigned int *      data_len, 
		     unsigned int        max_output_len, 
		     unsigned char *     sign,
		     unsigned int        sign_len)
{
    SECStatus       rv;
    unsigned int    modulus_len = nsslowkey_PublicModulusLen(key);
    unsigned int    i;
    unsigned char * buffer;

    if (sign_len != modulus_len) 
    	goto failure;
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey)
    	goto failure;

    buffer = (unsigned char *)PORT_Alloc(modulus_len + 1);
    if (!buffer)
    	goto failure;

    rv = RSA_PublicKeyOp(&key->u.rsa, buffer, sign);
    if (rv != SECSuccess)
    	goto loser;
    *data_len = 0;

    


    if (buffer[0] != 0 || buffer[1] != 1) 
    	goto loser;
    for (i = 2; i < modulus_len; i++) {
	if (buffer[i] == 0) {
	    *data_len = modulus_len - i - 1;
	    break;
	}
	if (buffer[i] != 0xff) 
	    goto loser;
    }
    if (*data_len == 0) 
    	goto loser;
    if (*data_len > max_output_len) 
    	goto loser;

    


    PORT_Memcpy(data,buffer + modulus_len - *data_len, *data_len);

    PORT_Free(buffer);
    return SECSuccess;

loser:
    PORT_Free(buffer);
failure:
    return SECFailure;
}


SECStatus
RSA_EncryptBlock(NSSLOWKEYPublicKey *key, 
                 unsigned char *     output, 
		 unsigned int *      output_len,
                 unsigned int        max_output_len, 
		 unsigned char *     input, 
		 unsigned int        input_len)
{
    SECStatus     rv;
    unsigned int  modulus_len = nsslowkey_PublicModulusLen(key);
    SECItem       formatted;
    SECItem       unformatted;

    formatted.data = NULL;
    if (max_output_len < modulus_len) 
    	goto failure;
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey)
    	goto failure;

    unformatted.len  = input_len;
    unformatted.data = input;
    formatted.data   = NULL;
    rv = rsa_FormatBlock(&formatted, modulus_len, RSA_BlockPublic,
			 &unformatted);
    if (rv != SECSuccess) 
	goto failure;

    rv = RSA_PublicKeyOp(&key->u.rsa, output, formatted.data);
    if (rv != SECSuccess) 
    	goto failure;

    PORT_ZFree(formatted.data, modulus_len);
    *output_len = modulus_len;
    return SECSuccess;

failure:
    if (formatted.data != NULL) 
	PORT_ZFree(formatted.data, modulus_len);
    return SECFailure;
}


SECStatus
RSA_DecryptBlock(NSSLOWKEYPrivateKey *key, 
                 unsigned char *      output, 
		 unsigned int *       output_len,
                 unsigned int         max_output_len, 
		 unsigned char *      input, 
		 unsigned int         input_len)
{
    SECStatus       rv;
    unsigned int    modulus_len = nsslowkey_PrivateModulusLen(key);
    unsigned int    i;
    unsigned char * buffer;

    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey)
    	goto failure;
    if (input_len != modulus_len)
    	goto failure;

    buffer = (unsigned char *)PORT_Alloc(modulus_len + 1);
    if (!buffer)
    	goto failure;

    rv = RSA_PrivateKeyOp(&key->u.rsa, buffer, input);
    if (rv != SECSuccess) {
	if (PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
	    sftk_fatalError = PR_TRUE;
	}
    	goto loser;
    }

    if (buffer[0] != 0 || buffer[1] != 2) 
    	goto loser;
    *output_len = 0;
    for (i = 2; i < modulus_len; i++) {
	if (buffer[i] == 0) {
	    *output_len = modulus_len - i - 1;
	    break;
	}
    }
    if (*output_len == 0) 
    	goto loser;
    if (*output_len > max_output_len) 
    	goto loser;

    PORT_Memcpy(output, buffer + modulus_len - *output_len, *output_len);

    PORT_Free(buffer);
    return SECSuccess;

loser:
    PORT_Free(buffer);
failure:
    return SECFailure;
}






SECStatus
RSA_SignRaw(NSSLOWKEYPrivateKey *key, 
            unsigned char *      output, 
	    unsigned int *       output_len,
            unsigned int         maxOutputLen, 
	    unsigned char *      input, 
	    unsigned int         input_len)
{
    SECStatus    rv          = SECSuccess;
    unsigned int modulus_len = nsslowkey_PrivateModulusLen(key);
    SECItem      formatted;
    SECItem      unformatted;

    if (maxOutputLen < modulus_len) 
    	return SECFailure;
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey)
    	return SECFailure;

    unformatted.len  = input_len;
    unformatted.data = input;
    formatted.data   = NULL;
    rv = rsa_FormatBlock(&formatted, modulus_len, RSA_BlockRaw, &unformatted);
    if (rv != SECSuccess) 
    	goto done;

    rv = RSA_PrivateKeyOpDoubleChecked(&key->u.rsa, output, formatted.data);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
	sftk_fatalError = PR_TRUE;
    }
    *output_len = modulus_len;

done:
    if (formatted.data != NULL) 
    	PORT_ZFree(formatted.data, modulus_len);
    return rv;
}


SECStatus
RSA_CheckSignRaw(NSSLOWKEYPublicKey *key,
                 unsigned char *     sign, 
		 unsigned int        sign_len, 
		 unsigned char *     hash, 
		 unsigned int        hash_len)
{
    SECStatus       rv;
    unsigned int    modulus_len = nsslowkey_PublicModulusLen(key);
    unsigned char * buffer;

    if (sign_len != modulus_len) 
    	goto failure;
    if (hash_len > modulus_len) 
    	goto failure;
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey)
    	goto failure;

    buffer = (unsigned char *)PORT_Alloc(modulus_len + 1);
    if (!buffer)
    	goto failure;

    rv = RSA_PublicKeyOp(&key->u.rsa, buffer, sign);
    if (rv != SECSuccess)
	goto loser;

    


    
    if (PORT_Memcmp(buffer + (modulus_len-hash_len), hash, hash_len) != 0)
	goto loser;

    PORT_Free(buffer);
    return SECSuccess;

loser:
    PORT_Free(buffer);
failure:
    return SECFailure;
}


SECStatus
RSA_CheckSignRecoverRaw(NSSLOWKEYPublicKey *key,
                        unsigned char *     data,
                        unsigned int *      data_len, 
			unsigned int        max_output_len, 
			unsigned char *     sign,
			unsigned int        sign_len)
{
    SECStatus      rv;
    unsigned int   modulus_len = nsslowkey_PublicModulusLen(key);

    if (sign_len != modulus_len) 
    	goto failure;
    if (max_output_len < modulus_len) 
    	goto failure;
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey)
    	goto failure;

    rv = RSA_PublicKeyOp(&key->u.rsa, data, sign);
    if (rv != SECSuccess)
	goto failure;

    *data_len = modulus_len;
    return SECSuccess;

failure:
    return SECFailure;
}



SECStatus
RSA_EncryptRaw(NSSLOWKEYPublicKey *key, 
	       unsigned char *     output, 
	       unsigned int *      output_len,
               unsigned int        max_output_len, 
	       unsigned char *     input, 
	       unsigned int        input_len)
{
    SECStatus rv;
    unsigned int  modulus_len = nsslowkey_PublicModulusLen(key);
    SECItem       formatted;
    SECItem       unformatted;

    formatted.data = NULL;
    if (max_output_len < modulus_len) 
    	goto failure;
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey)
    	goto failure;

    unformatted.len  = input_len;
    unformatted.data = input;
    formatted.data   = NULL;
    rv = rsa_FormatBlock(&formatted, modulus_len, RSA_BlockRaw, &unformatted);
    if (rv != SECSuccess)
	goto failure;

    rv = RSA_PublicKeyOp(&key->u.rsa, output, formatted.data);
    if (rv != SECSuccess) 
    	goto failure;

    PORT_ZFree(formatted.data, modulus_len);
    *output_len = modulus_len;
    return SECSuccess;

failure:
    if (formatted.data != NULL) 
	PORT_ZFree(formatted.data, modulus_len);
    return SECFailure;
}


SECStatus
RSA_DecryptRaw(NSSLOWKEYPrivateKey *key, 
               unsigned char *      output, 
	       unsigned int *       output_len,
               unsigned int         max_output_len, 
	       unsigned char *      input, 
	       unsigned int         input_len)
{
    SECStatus     rv;
    unsigned int  modulus_len = nsslowkey_PrivateModulusLen(key);

    if (modulus_len <= 0) 
    	goto failure;
    if (modulus_len > max_output_len) 
    	goto failure;
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey)
    	goto failure;
    if (input_len != modulus_len) 
    	goto failure;

    rv = RSA_PrivateKeyOp(&key->u.rsa, output, input);
    if (rv != SECSuccess) {
	if (PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
	    sftk_fatalError = PR_TRUE;
	}
    	goto failure;
    }

    *output_len = modulus_len;
    return SECSuccess;

failure:
    return SECFailure;
}
