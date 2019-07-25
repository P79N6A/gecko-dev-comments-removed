









































#include "cmslocal.h"

#include "secoid.h"
#include "secitem.h"
#include "pk11func.h"
#include "secerr.h"
#include "secpkcs5.h"






typedef SECStatus (*nss_cms_cipher_function) (void *, unsigned char *, unsigned int *,
					unsigned int, const unsigned char *, unsigned int);
typedef SECStatus (*nss_cms_cipher_destroy) (void *, PRBool);

#define BLOCK_SIZE 4096

struct NSSCMSCipherContextStr {
    void *		cx;			
    nss_cms_cipher_function doit;
    nss_cms_cipher_destroy destroy;
    PRBool		encrypt;		
    int			block_size;		
    int			pad_size;
    int			pending_count;		
    unsigned char	pending_buf[BLOCK_SIZE];
};










NSSCMSCipherContext *
NSS_CMSCipherContext_StartDecrypt(PK11SymKey *key, SECAlgorithmID *algid)
{
    NSSCMSCipherContext *cc;
    void *ciphercx;
    CK_MECHANISM_TYPE cryptoMechType;
    PK11SlotInfo *slot;
    SECOidTag algtag;
    SECItem *param = NULL;

    algtag = SECOID_GetAlgorithmTag(algid);

    
    if (SEC_PKCS5IsAlgorithmPBEAlg(algid)) {
	SECItem *pwitem;

	pwitem = PK11_GetSymKeyUserData(key);
	if (!pwitem) 
	    return NULL;

	cryptoMechType = PK11_GetPBECryptoMechanism(algid, &param, pwitem);
	if (cryptoMechType == CKM_INVALID_MECHANISM) {
	    SECITEM_FreeItem(param,PR_TRUE);
	    return NULL;
	}

    } else {
	cryptoMechType = PK11_AlgtagToMechanism(algtag);
	if ((param = PK11_ParamFromAlgid(algid)) == NULL)
	    return NULL;
    }

    cc = (NSSCMSCipherContext *)PORT_ZAlloc(sizeof(NSSCMSCipherContext));
    if (cc == NULL) {
	SECITEM_FreeItem(param,PR_TRUE);
	return NULL;
    }

    
    cc->pad_size = PK11_GetBlockSize(cryptoMechType, param);
    slot = PK11_GetSlotFromKey(key);
    cc->block_size = PK11_IsHW(slot) ? BLOCK_SIZE : cc->pad_size;
    PK11_FreeSlot(slot);

    
    ciphercx = PK11_CreateContextBySymKey(cryptoMechType, CKA_DECRYPT, 
					  key, param);
    SECITEM_FreeItem(param, PR_TRUE);
    if (ciphercx == NULL) {
	PORT_Free (cc);
	return NULL;
    }

    cc->cx = ciphercx;
    cc->doit =  (nss_cms_cipher_function) PK11_CipherOp;
    cc->destroy = (nss_cms_cipher_destroy) PK11_DestroyContext;
    cc->encrypt = PR_FALSE;
    cc->pending_count = 0;

    return cc;
}










NSSCMSCipherContext *
NSS_CMSCipherContext_StartEncrypt(PRArenaPool *poolp, PK11SymKey *key, SECAlgorithmID *algid)
{
    NSSCMSCipherContext *cc;
    void *ciphercx;
    SECStatus rv;
    CK_MECHANISM_TYPE cryptoMechType;
    PK11SlotInfo *slot;
    SECItem *param = NULL;
    PRBool needToEncodeAlgid = PR_FALSE;
    SECOidTag algtag = SECOID_GetAlgorithmTag(algid);

    
    if (SEC_PKCS5IsAlgorithmPBEAlg(algid)) {
	SECItem *pwitem;

	pwitem = PK11_GetSymKeyUserData(key);
	if (!pwitem) 
	    return NULL;

	cryptoMechType = PK11_GetPBECryptoMechanism(algid, &param, pwitem);
	if (cryptoMechType == CKM_INVALID_MECHANISM) {
	    SECITEM_FreeItem(param,PR_TRUE);
	    return NULL;
	}
    } else {
	cryptoMechType = PK11_AlgtagToMechanism(algtag);
	if ((param = PK11_GenerateNewParam(cryptoMechType, key)) == NULL)
	    return NULL;
	needToEncodeAlgid = PR_TRUE;
    }

    cc = (NSSCMSCipherContext *)PORT_ZAlloc(sizeof(NSSCMSCipherContext));
    if (cc == NULL) {
	goto loser;
    }

    
    cc->pad_size = PK11_GetBlockSize(cryptoMechType, param);
    slot = PK11_GetSlotFromKey(key);
    cc->block_size = PK11_IsHW(slot) ? BLOCK_SIZE : cc->pad_size;
    PK11_FreeSlot(slot);

    
    ciphercx = PK11_CreateContextBySymKey(cryptoMechType, CKA_ENCRYPT, 
					  key, param);
    if (ciphercx == NULL) {
	PORT_Free(cc);
	cc = NULL;
	goto loser;
    }

    







    if (needToEncodeAlgid) {
	rv = PK11_ParamToAlgid(algtag, param, poolp, algid);
	if(rv != SECSuccess) {
	    PORT_Free(cc);
	    cc = NULL;
	    goto loser;
	}
    }

    cc->cx = ciphercx;
    cc->doit = (nss_cms_cipher_function)PK11_CipherOp;
    cc->destroy = (nss_cms_cipher_destroy)PK11_DestroyContext;
    cc->encrypt = PR_TRUE;
    cc->pending_count = 0;

loser:
    SECITEM_FreeItem(param, PR_TRUE);

    return cc;
}

void
NSS_CMSCipherContext_Destroy(NSSCMSCipherContext *cc)
{
    PORT_Assert(cc != NULL);
    if (cc == NULL)
	return;
    (*cc->destroy)(cc->cx, PR_TRUE);
    PORT_Free(cc);
}






















unsigned int
NSS_CMSCipherContext_DecryptLength(NSSCMSCipherContext *cc, unsigned int input_len, PRBool final)
{
    int blocks, block_size;

    PORT_Assert (! cc->encrypt);

    block_size = cc->block_size;

    



    if (block_size == 0)
	return input_len;

    







    if (final)
	return cc->pending_count + input_len;

    









    blocks = (cc->pending_count + input_len - 1) / block_size;
    return blocks * block_size;
}


















unsigned int
NSS_CMSCipherContext_EncryptLength(NSSCMSCipherContext *cc, unsigned int input_len, PRBool final)
{
    int blocks, block_size;
    int pad_size;

    PORT_Assert (cc->encrypt);

    block_size = cc->block_size;
    pad_size = cc->pad_size;

    



    if (block_size == 0)
	return input_len;

    





    if (final) {
	if (pad_size == 0) {
    	    return cc->pending_count + input_len;
	} else {
    	    blocks = (cc->pending_count + input_len) / pad_size;
	    blocks++;
	    return blocks*pad_size;
	}
    }

    


    blocks = (cc->pending_count + input_len) / block_size;


    return blocks * block_size;
}































 
SECStatus
NSS_CMSCipherContext_Decrypt(NSSCMSCipherContext *cc, unsigned char *output,
		  unsigned int *output_len_p, unsigned int max_output_len,
		  const unsigned char *input, unsigned int input_len,
		  PRBool final)
{
    int blocks, bsize, pcount, padsize;
    unsigned int max_needed, ifraglen, ofraglen, output_len;
    unsigned char *pbuf;
    SECStatus rv;

    PORT_Assert (! cc->encrypt);

    



    max_needed = NSS_CMSCipherContext_DecryptLength(cc, input_len, final);
    PORT_Assert (max_output_len >= max_needed);
    if (max_output_len < max_needed) {
	
	return SECFailure;
    }

    



    bsize = cc->block_size;
    padsize = cc->pad_size;

    



    if (bsize == 0) {
	return (* cc->doit) (cc->cx, output, output_len_p, max_output_len,
			      input, input_len);
    }

    pcount = cc->pending_count;
    pbuf = cc->pending_buf;

    output_len = 0;

    if (pcount) {
	



	while (input_len && pcount < bsize) {
	    pbuf[pcount++] = *input++;
	    input_len--;
	}
	





	if (input_len == 0 && !final) {
	    cc->pending_count = pcount;
	    if (output_len_p)
		*output_len_p = 0;
	    return SECSuccess;
	}
	




	if ((padsize != 0) && (pcount % padsize) != 0) {
	    PORT_Assert (final);	
	    PORT_SetError (SEC_ERROR_BAD_DATA);
	    return SECFailure;
	}
	


	rv = (*cc->doit)(cc->cx, output, &ofraglen, max_output_len,
			    pbuf, pcount);
	if (rv != SECSuccess)
	    return rv;

	




	PORT_Assert(ofraglen == pcount);

	


	max_output_len -= ofraglen;
	output_len += ofraglen;
	output += ofraglen;
    }

    












    if (final) {
	if (padsize) {
	    blocks = input_len / padsize;
	    ifraglen = blocks * padsize;
	} else ifraglen = input_len;
	PORT_Assert (ifraglen == input_len);

	if (ifraglen != input_len) {
	    PORT_SetError(SEC_ERROR_BAD_DATA);
	    return SECFailure;
	}
    } else {
	blocks = (input_len - 1) / bsize;
	ifraglen = blocks * bsize;
	PORT_Assert (ifraglen < input_len);

	pcount = input_len - ifraglen;
	PORT_Memcpy (pbuf, input + ifraglen, pcount);
	cc->pending_count = pcount;
    }

    if (ifraglen) {
	rv = (* cc->doit)(cc->cx, output, &ofraglen, max_output_len,
			    input, ifraglen);
	if (rv != SECSuccess)
	    return rv;

	




	PORT_Assert (ifraglen == ofraglen);
	if (ifraglen != ofraglen) {
	    PORT_SetError(SEC_ERROR_BAD_DATA);
	    return SECFailure;
	}

	output_len += ofraglen;
    } else {
	ofraglen = 0;
    }

    



    if (final && (padsize != 0)) {
	unsigned int padlen = *(output + ofraglen - 1);

	if (padlen == 0 || padlen > padsize) {
	    PORT_SetError(SEC_ERROR_BAD_DATA);
	    return SECFailure;
	}
	output_len -= padlen;
    }

    PORT_Assert (output_len_p != NULL || output_len == 0);
    if (output_len_p != NULL)
	*output_len_p = output_len;

    return SECSuccess;
}



































 
SECStatus
NSS_CMSCipherContext_Encrypt(NSSCMSCipherContext *cc, unsigned char *output,
		  unsigned int *output_len_p, unsigned int max_output_len,
		  const unsigned char *input, unsigned int input_len,
		  PRBool final)
{
    int blocks, bsize, padlen, pcount, padsize;
    unsigned int max_needed, ifraglen, ofraglen, output_len;
    unsigned char *pbuf;
    SECStatus rv;

    PORT_Assert (cc->encrypt);

    



    max_needed = NSS_CMSCipherContext_EncryptLength (cc, input_len, final);
    PORT_Assert (max_output_len >= max_needed);
    if (max_output_len < max_needed) {
	
	return SECFailure;
    }

    bsize = cc->block_size;
    padsize = cc->pad_size;

    



    if (bsize == 0) {
	return (*cc->doit)(cc->cx, output, output_len_p, max_output_len,
			      input, input_len);
    }

    pcount = cc->pending_count;
    pbuf = cc->pending_buf;

    output_len = 0;

    if (pcount) {
	



	while (input_len && pcount < bsize) {
	    pbuf[pcount++] = *input++;
	    input_len--;
	}
	



	if (pcount < bsize && !final) {
	    cc->pending_count = pcount;
	    if (output_len_p != NULL)
		*output_len_p = 0;
	    return SECSuccess;
	}
	


	if ((padsize == 0) || (pcount % padsize) == 0) {
	    rv = (* cc->doit) (cc->cx, output, &ofraglen, max_output_len,
				pbuf, pcount);
	    if (rv != SECSuccess)
		return rv;

	    




	    PORT_Assert (ofraglen == pcount);

	    


	    max_output_len -= ofraglen;
	    output_len += ofraglen;
	    output += ofraglen;

	    pcount = 0;
	}
    }

    if (input_len) {
	PORT_Assert (pcount == 0);

	blocks = input_len / bsize;
	ifraglen = blocks * bsize;

	if (ifraglen) {
	    rv = (* cc->doit) (cc->cx, output, &ofraglen, max_output_len,
				input, ifraglen);
	    if (rv != SECSuccess)
		return rv;

	    




	    PORT_Assert (ifraglen == ofraglen);

	    max_output_len -= ofraglen;
	    output_len += ofraglen;
	    output += ofraglen;
	}

	pcount = input_len - ifraglen;
	PORT_Assert (pcount < bsize);
	if (pcount)
	    PORT_Memcpy (pbuf, input + ifraglen, pcount);
    }

    if (final) {
	padlen = padsize - (pcount % padsize);
	PORT_Memset (pbuf + pcount, padlen, padlen);
	rv = (* cc->doit) (cc->cx, output, &ofraglen, max_output_len,
			    pbuf, pcount+padlen);
	if (rv != SECSuccess)
	    return rv;

	




	PORT_Assert (ofraglen == (pcount+padlen));
	output_len += ofraglen;
    } else {
	cc->pending_count = pcount;
    }

    PORT_Assert (output_len_p != NULL || output_len == 0);
    if (output_len_p != NULL)
	*output_len_p = output_len;

    return SECSuccess;
}
