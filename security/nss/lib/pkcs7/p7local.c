












































#include "p7local.h"

#include "cryptohi.h" 
#include "secasn1.h"
#include "secoid.h"
#include "secitem.h"
#include "pk11func.h"
#include "secpkcs5.h"
#include "secerr.h"






typedef SECStatus (*sec_pkcs7_cipher_function) (void *,
						unsigned char *,
						unsigned *,
						unsigned int,
						const unsigned char *,
						unsigned int);
typedef SECStatus (*sec_pkcs7_cipher_destroy) (void *, PRBool);

#define BLOCK_SIZE 4096

struct sec_pkcs7_cipher_object {
    void *cx;
    sec_pkcs7_cipher_function doit;
    sec_pkcs7_cipher_destroy destroy;
    PRBool encrypt;
    int block_size;
    int pad_size;
    int pending_count;
    unsigned char pending_buf[BLOCK_SIZE];
};

SEC_ASN1_MKSUB(CERT_IssuerAndSNTemplate)
SEC_ASN1_MKSUB(CERT_SetOfSignedCrlTemplate)
SEC_ASN1_MKSUB(SECOID_AlgorithmIDTemplate)
SEC_ASN1_MKSUB(SEC_OctetStringTemplate)
SEC_ASN1_MKSUB(SEC_SetOfAnyTemplate)













sec_PKCS7CipherObject *
sec_PKCS7CreateDecryptObject (PK11SymKey *key, SECAlgorithmID *algid)
{
    sec_PKCS7CipherObject *result;
    SECOidTag algtag;
    void *ciphercx;
    CK_MECHANISM_TYPE cryptoMechType;
    PK11SlotInfo *slot;
    SECItem *param = NULL;

    result = (struct sec_pkcs7_cipher_object*)
      PORT_ZAlloc (sizeof(struct sec_pkcs7_cipher_object));
    if (result == NULL)
	return NULL;

    ciphercx = NULL;
    algtag = SECOID_GetAlgorithmTag (algid);

    if (SEC_PKCS5IsAlgorithmPBEAlg(algid)) {
	SECItem *pwitem;

	pwitem = (SECItem *)PK11_GetSymKeyUserData(key);
	if (!pwitem) {
	    PORT_Free(result);
	    return NULL;
	}

	cryptoMechType = PK11_GetPBECryptoMechanism(algid, &param, pwitem);
	if (cryptoMechType == CKM_INVALID_MECHANISM) {
	    PORT_Free(result);
	    SECITEM_FreeItem(param,PR_TRUE);
	    return NULL;
	}
    } else {
	cryptoMechType = PK11_AlgtagToMechanism(algtag);
	param = PK11_ParamFromAlgid(algid);
	if (param == NULL) {
	    PORT_Free(result);
	    return NULL;
	}
    }

    result->pad_size = PK11_GetBlockSize(cryptoMechType, param);
    slot = PK11_GetSlotFromKey(key);
    result->block_size = PK11_IsHW(slot) ? BLOCK_SIZE : result->pad_size;
    PK11_FreeSlot(slot);
    ciphercx = PK11_CreateContextBySymKey(cryptoMechType, CKA_DECRYPT, 
					  key, param);
    SECITEM_FreeItem(param,PR_TRUE);
    if (ciphercx == NULL) {
	PORT_Free (result);
	return NULL;
    }

    result->cx = ciphercx;
    result->doit =  (sec_pkcs7_cipher_function) PK11_CipherOp;
    result->destroy = (sec_pkcs7_cipher_destroy) PK11_DestroyContext;
    result->encrypt = PR_FALSE;
    result->pending_count = 0;

    return result;
}














sec_PKCS7CipherObject *
sec_PKCS7CreateEncryptObject (PRArenaPool *poolp, PK11SymKey *key,
			      SECOidTag algtag, SECAlgorithmID *algid)
{
    sec_PKCS7CipherObject *result;
    void *ciphercx;
    SECStatus rv;
    CK_MECHANISM_TYPE cryptoMechType;
    PK11SlotInfo *slot;
    SECItem *param = NULL;
    PRBool needToEncodeAlgid = PR_FALSE;

    result = (struct sec_pkcs7_cipher_object*)
	      PORT_ZAlloc (sizeof(struct sec_pkcs7_cipher_object));
    if (result == NULL)
	return NULL;

    ciphercx = NULL;
    if (SEC_PKCS5IsAlgorithmPBEAlg(algid)) {
	SECItem *pwitem;

	pwitem = (SECItem *)PK11_GetSymKeyUserData(key);
	if (!pwitem) {
	    PORT_Free(result);
	    return NULL;
	}

	cryptoMechType = PK11_GetPBECryptoMechanism(algid, &param, pwitem);
	if (cryptoMechType == CKM_INVALID_MECHANISM) {
	    PORT_Free(result);
	    SECITEM_FreeItem(param,PR_TRUE);
	    return NULL;
	}
    } else {
	cryptoMechType = PK11_AlgtagToMechanism(algtag);
	param = PK11_GenerateNewParam(cryptoMechType, key);
	if (param == NULL) {
	    PORT_Free(result);
	    return NULL;
	}
	needToEncodeAlgid = PR_TRUE;
    }

    result->pad_size = PK11_GetBlockSize(cryptoMechType,param);
    slot = PK11_GetSlotFromKey(key);
    result->block_size = PK11_IsHW(slot) ? BLOCK_SIZE : result->pad_size;
    PK11_FreeSlot(slot);
    ciphercx = PK11_CreateContextBySymKey(cryptoMechType, CKA_ENCRYPT, 
    					  key, param);
    if (ciphercx == NULL) {
	PORT_Free (result);
        SECITEM_FreeItem(param,PR_TRUE);
	return NULL;
    }

    




    if (needToEncodeAlgid) {
	rv = PK11_ParamToAlgid(algtag,param,poolp,algid);
	if(rv != SECSuccess) {
	    PORT_Free (result);
            SECITEM_FreeItem(param,PR_TRUE);
	    return NULL;
	}
    }
    SECITEM_FreeItem(param,PR_TRUE);

    result->cx = ciphercx;
    result->doit = (sec_pkcs7_cipher_function) PK11_CipherOp;
    result->destroy = (sec_pkcs7_cipher_destroy) PK11_DestroyContext;
    result->encrypt = PR_TRUE;
    result->pending_count = 0;

    return result;
}





static void
sec_pkcs7_destroy_cipher (sec_PKCS7CipherObject *obj)
{
    (* obj->destroy) (obj->cx, PR_TRUE);
    PORT_Free (obj);
}

void
sec_PKCS7DestroyDecryptObject (sec_PKCS7CipherObject *obj)
{
    PORT_Assert (obj != NULL);
    if (obj == NULL)
	return;
    PORT_Assert (! obj->encrypt);
    sec_pkcs7_destroy_cipher (obj);
}

void
sec_PKCS7DestroyEncryptObject (sec_PKCS7CipherObject *obj)
{
    PORT_Assert (obj != NULL);
    if (obj == NULL)
	return;
    PORT_Assert (obj->encrypt);
    sec_pkcs7_destroy_cipher (obj);
}
























unsigned int
sec_PKCS7DecryptLength (sec_PKCS7CipherObject *obj, unsigned int input_len,
			PRBool final)
{
    int blocks, block_size;

    PORT_Assert (! obj->encrypt);

    block_size = obj->block_size;

    



    if (block_size == 0)
	return input_len;

    







    if (final)
	return obj->pending_count + input_len;

    









    blocks = (obj->pending_count + input_len - 1) / block_size;
    return blocks * block_size;
}













unsigned int
sec_PKCS7EncryptLength (sec_PKCS7CipherObject *obj, unsigned int input_len,
			PRBool final)
{
    int blocks, block_size;
    int pad_size;

    PORT_Assert (obj->encrypt);

    block_size = obj->block_size;
    pad_size = obj->pad_size;

    



    if (block_size == 0)
	return input_len;

    





    if (final) {
	if (pad_size == 0) {
    	    return obj->pending_count + input_len;
	} else {
    	    blocks = (obj->pending_count + input_len) / pad_size;
	    blocks++;
	    return blocks*pad_size;
	}
    }

    


    blocks = (obj->pending_count + input_len) / block_size;


    return blocks * block_size;
}





















 
SECStatus
sec_PKCS7Decrypt (sec_PKCS7CipherObject *obj, unsigned char *output,
		  unsigned int *output_len_p, unsigned int max_output_len,
		  const unsigned char *input, unsigned int input_len,
		  PRBool final)
{
    int blocks, bsize, pcount, padsize;
    unsigned int max_needed, ifraglen, ofraglen, output_len;
    unsigned char *pbuf;
    SECStatus rv;

    PORT_Assert (! obj->encrypt);

    



    max_needed = sec_PKCS7DecryptLength (obj, input_len, final);
    PORT_Assert (max_output_len >= max_needed);
    if (max_output_len < max_needed) {
	
	return SECFailure;
    }

    



    bsize = obj->block_size;
    padsize = obj->pad_size;

    



    if (bsize == 0) {
	return (* obj->doit) (obj->cx, output, output_len_p, max_output_len,
			      input, input_len);
    }

    pcount = obj->pending_count;
    pbuf = obj->pending_buf;

    output_len = 0;

    if (pcount) {
	



	while (input_len && pcount < bsize) {
	    pbuf[pcount++] = *input++;
	    input_len--;
	}
	





	if (input_len == 0 && !final) {
	    obj->pending_count = pcount;
	    if (output_len_p)
		*output_len_p = 0;
	    return SECSuccess;
	}
	




	PORT_Assert ((padsize == 0) || (pcount % padsize) == 0);
	if ((padsize != 0) && (pcount % padsize) != 0) {
	    PORT_Assert (final);	
	    PORT_SetError (SEC_ERROR_BAD_DATA);
	    return SECFailure;
	}
	


	rv = (* obj->doit) (obj->cx, output, &ofraglen, max_output_len,
			    pbuf, pcount);
	if (rv != SECSuccess)
	    return rv;

	




	PORT_Assert (ofraglen == pcount);

	


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
	    PORT_SetError (SEC_ERROR_BAD_DATA);
	    return SECFailure;
	}
    } else {
	blocks = (input_len - 1) / bsize;
	ifraglen = blocks * bsize;
	PORT_Assert (ifraglen < input_len);

	pcount = input_len - ifraglen;
	PORT_Memcpy (pbuf, input + ifraglen, pcount);
	obj->pending_count = pcount;
    }

    if (ifraglen) {
	rv = (* obj->doit) (obj->cx, output, &ofraglen, max_output_len,
			    input, ifraglen);
	if (rv != SECSuccess)
	    return rv;

	




	PORT_Assert (ifraglen == ofraglen);
	if (ifraglen != ofraglen) {
	    PORT_SetError (SEC_ERROR_BAD_DATA);
	    return SECFailure;
	}

	output_len += ofraglen;
    } else {
	ofraglen = 0;
    }

    



    if (final && (padsize != 0)) {
	unsigned int padlen = *(output + ofraglen - 1);
	if (padlen == 0 || padlen > padsize) {
	    PORT_SetError (SEC_ERROR_BAD_DATA);
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
sec_PKCS7Encrypt (sec_PKCS7CipherObject *obj, unsigned char *output,
		  unsigned int *output_len_p, unsigned int max_output_len,
		  const unsigned char *input, unsigned int input_len,
		  PRBool final)
{
    int blocks, bsize, padlen, pcount, padsize;
    unsigned int max_needed, ifraglen, ofraglen, output_len;
    unsigned char *pbuf;
    SECStatus rv;

    PORT_Assert (obj->encrypt);

    



    max_needed = sec_PKCS7EncryptLength (obj, input_len, final);
    PORT_Assert (max_output_len >= max_needed);
    if (max_output_len < max_needed) {
	
	return SECFailure;
    }

    bsize = obj->block_size;
    padsize = obj->pad_size;

    



    if (bsize == 0) {
	return (* obj->doit) (obj->cx, output, output_len_p, max_output_len,
			      input, input_len);
    }

    pcount = obj->pending_count;
    pbuf = obj->pending_buf;

    output_len = 0;

    if (pcount) {
	



	while (input_len && pcount < bsize) {
	    pbuf[pcount++] = *input++;
	    input_len--;
	}
	



	if (pcount < bsize && !final) {
	    obj->pending_count = pcount;
	    if (output_len_p != NULL)
		*output_len_p = 0;
	    return SECSuccess;
	}
	


	if ((padsize == 0) || (pcount % padsize) == 0) {
	    rv = (* obj->doit) (obj->cx, output, &ofraglen, max_output_len,
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
	    rv = (* obj->doit) (obj->cx, output, &ofraglen, max_output_len,
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
	rv = (* obj->doit) (obj->cx, output, &ofraglen, max_output_len,
			    pbuf, pcount+padlen);
	if (rv != SECSuccess)
	    return rv;

	




	PORT_Assert (ofraglen == (pcount+padlen));
	output_len += ofraglen;
    } else {
	obj->pending_count = pcount;
    }

    PORT_Assert (output_len_p != NULL || output_len == 0);
    if (output_len_p != NULL)
	*output_len_p = output_len;

    return SECSuccess;
}
























SEC_PKCS7Attribute *
sec_PKCS7FindAttribute (SEC_PKCS7Attribute **attrs, SECOidTag oidtag,
			PRBool only)
{
    SECOidData *oid;
    SEC_PKCS7Attribute *attr1, *attr2;

    if (attrs == NULL)
	return NULL;

    oid = SECOID_FindOIDByTag(oidtag);
    if (oid == NULL)
	return NULL;

    while ((attr1 = *attrs++) != NULL) {
	if (attr1->type.len == oid->oid.len && PORT_Memcmp (attr1->type.data,
							    oid->oid.data,
							    oid->oid.len) == 0)
	    break;
    }

    if (attr1 == NULL)
	return NULL;

    if (!only)
	return attr1;

    while ((attr2 = *attrs++) != NULL) {
	if (attr2->type.len == oid->oid.len && PORT_Memcmp (attr2->type.data,
							    oid->oid.data,
							    oid->oid.len) == 0)
	    break;
    }

    if (attr2 != NULL)
	return NULL;

    return attr1;
}







SECItem *
sec_PKCS7AttributeValue(SEC_PKCS7Attribute *attr)
{
    SECItem *value;

    if (attr == NULL)
	return NULL;

    value = attr->values[0];

    if (value == NULL || value->data == NULL || value->len == 0)
	return NULL;

    if (attr->values[1] != NULL)
	return NULL;

    return value;
}

static const SEC_ASN1Template *
sec_attr_choose_attr_value_template(void *src_or_dest, PRBool encoding)
{
    const SEC_ASN1Template *theTemplate;

    SEC_PKCS7Attribute *attribute;
    SECOidData *oiddata;
    PRBool encoded;

    PORT_Assert (src_or_dest != NULL);
    if (src_or_dest == NULL)
	return NULL;

    attribute = (SEC_PKCS7Attribute*)src_or_dest;

    if (encoding && attribute->encoded)
	return SEC_ASN1_GET(SEC_AnyTemplate);

    oiddata = attribute->typeTag;
    if (oiddata == NULL) {
	oiddata = SECOID_FindOID(&attribute->type);
	attribute->typeTag = oiddata;
    }

    if (oiddata == NULL) {
	encoded = PR_TRUE;
	theTemplate = SEC_ASN1_GET(SEC_AnyTemplate);
    } else {
	switch (oiddata->offset) {
	  default:
	    encoded = PR_TRUE;
	    theTemplate = SEC_ASN1_GET(SEC_AnyTemplate);
	    break;
	  case SEC_OID_PKCS9_EMAIL_ADDRESS:
	  case SEC_OID_RFC1274_MAIL:
	  case SEC_OID_PKCS9_UNSTRUCTURED_NAME:
	    encoded = PR_FALSE;
	    theTemplate = SEC_ASN1_GET(SEC_IA5StringTemplate);
	    break;
	  case SEC_OID_PKCS9_CONTENT_TYPE:
	    encoded = PR_FALSE;
	    theTemplate = SEC_ASN1_GET(SEC_ObjectIDTemplate);
	    break;
	  case SEC_OID_PKCS9_MESSAGE_DIGEST:
	    encoded = PR_FALSE;
	    theTemplate = SEC_ASN1_GET(SEC_OctetStringTemplate);
	    break;
	  case SEC_OID_PKCS9_SIGNING_TIME:
	    encoded = PR_FALSE;
            theTemplate = SEC_ASN1_GET(CERT_TimeChoiceTemplate);
	    break;
	  
	}
    }

    if (encoding) {
	






	PORT_Assert (!encoded);
    } else {
	



	attribute->encoded = encoded;
    }
    return theTemplate;
}

static const SEC_ASN1TemplateChooserPtr sec_attr_chooser
	= sec_attr_choose_attr_value_template;

static const SEC_ASN1Template sec_pkcs7_attribute_template[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SEC_PKCS7Attribute) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(SEC_PKCS7Attribute,type) },
    { SEC_ASN1_DYNAMIC | SEC_ASN1_SET_OF,
	  offsetof(SEC_PKCS7Attribute,values),
	  &sec_attr_chooser },
    { 0 }
};

static const SEC_ASN1Template sec_pkcs7_set_of_attribute_template[] = {
    { SEC_ASN1_SET_OF, 0, sec_pkcs7_attribute_template },
};








SECItem *
sec_PKCS7EncodeAttributes (PRArenaPool *poolp, SECItem *dest, void *src)
{
    return SEC_ASN1EncodeItem (poolp, dest, src,
			       sec_pkcs7_set_of_attribute_template);
}






SECStatus
sec_PKCS7ReorderAttributes (SEC_PKCS7Attribute **attrs)
{
    PRArenaPool *poolp;
    int num_attrs, i, pass, besti;
    unsigned int j;
    SECItem **enc_attrs;
    SEC_PKCS7Attribute **new_attrs;

    



    PORT_Assert (attrs != NULL);
    if (attrs == NULL)
	return SECSuccess;

    


    num_attrs = 0;
    while (attrs[num_attrs] != NULL)
	num_attrs++;

    




    PORT_Assert (num_attrs);
    if (num_attrs == 0 || num_attrs == 1)
	return SECSuccess;

    



    poolp = PORT_NewArena (1024);	
    if (poolp == NULL)
	return SECFailure;		

    



    enc_attrs=(SECItem**)PORT_ArenaZAlloc(poolp, num_attrs*sizeof(SECItem *));
    new_attrs = (SEC_PKCS7Attribute**)PORT_ArenaZAlloc (poolp,
				  num_attrs * sizeof(SEC_PKCS7Attribute *));
    if (enc_attrs == NULL || new_attrs == NULL) {
	PORT_FreeArena (poolp, PR_FALSE);
	return SECFailure;
    }

    


    for (i = 0; i < num_attrs; i++) {
	enc_attrs[i] = SEC_ASN1EncodeItem (poolp, NULL, attrs[i],
					   sec_pkcs7_attribute_template);
	if (enc_attrs[i] == NULL) {
	    PORT_FreeArena (poolp, PR_FALSE);
	    return SECFailure;
	}
    }

    




    for (pass = 0; pass < num_attrs; pass++) {
	



	for (i = 0; i < num_attrs; i++) {
	    if (enc_attrs[i] != NULL)
		break;
	}
	PORT_Assert (i < num_attrs);
	besti = i;

	










	for (i = besti + 1; i < num_attrs; i++) {
	    if (enc_attrs[i] == NULL)	
		continue;

	    if (enc_attrs[i]->len != enc_attrs[besti]->len) {
		if (enc_attrs[i]->len < enc_attrs[besti]->len)
		    besti = i;
		continue;
	    }

	    for (j = 0; j < enc_attrs[i]->len; j++) {
		if (enc_attrs[i]->data[j] < enc_attrs[besti]->data[j]) {
		    besti = i;
		    break;
		}
	    }

	    





	    PORT_Assert (j < enc_attrs[i]->len);
	}

	



	new_attrs[pass] = attrs[besti];
	enc_attrs[besti] = NULL;
    }

    



    for (i = 0; i < num_attrs; i++)
	attrs[i] = new_attrs[i];

    PORT_FreeArena (poolp, PR_FALSE);
    return SECSuccess;
}












static const SEC_ASN1Template *
sec_pkcs7_choose_content_template(void *src_or_dest, PRBool encoding);

static const SEC_ASN1TemplateChooserPtr sec_pkcs7_chooser
	= sec_pkcs7_choose_content_template;

const SEC_ASN1Template sec_PKCS7ContentInfoTemplate[] = {
    { SEC_ASN1_SEQUENCE | SEC_ASN1_MAY_STREAM,
	  0, NULL, sizeof(SEC_PKCS7ContentInfo) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(SEC_PKCS7ContentInfo,contentType) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_DYNAMIC | SEC_ASN1_MAY_STREAM
     | SEC_ASN1_EXPLICIT | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
	  offsetof(SEC_PKCS7ContentInfo,content),
	  &sec_pkcs7_chooser },
    { 0 }
};



static const SEC_ASN1Template SEC_PKCS7SignerInfoTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SEC_PKCS7SignerInfo) },
    { SEC_ASN1_INTEGER,
	  offsetof(SEC_PKCS7SignerInfo,version) },
    { SEC_ASN1_POINTER | SEC_ASN1_XTRN,
	  offsetof(SEC_PKCS7SignerInfo,issuerAndSN),
	  SEC_ASN1_SUB(CERT_IssuerAndSNTemplate) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(SEC_PKCS7SignerInfo,digestAlg),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
	  offsetof(SEC_PKCS7SignerInfo,authAttr),
	  sec_pkcs7_set_of_attribute_template },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(SEC_PKCS7SignerInfo,digestEncAlg),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_OCTET_STRING,
	  offsetof(SEC_PKCS7SignerInfo,encDigest) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 1,
	  offsetof(SEC_PKCS7SignerInfo,unAuthAttr),
	  sec_pkcs7_set_of_attribute_template },
    { 0 }
};

static const SEC_ASN1Template SEC_PKCS7SignedDataTemplate[] = {
    { SEC_ASN1_SEQUENCE | SEC_ASN1_MAY_STREAM,
	  0, NULL, sizeof(SEC_PKCS7SignedData) },
    { SEC_ASN1_INTEGER,
	  offsetof(SEC_PKCS7SignedData,version) },
    { SEC_ASN1_SET_OF | SEC_ASN1_XTRN,
	  offsetof(SEC_PKCS7SignedData,digestAlgorithms),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_INLINE,
	  offsetof(SEC_PKCS7SignedData,contentInfo),
	  sec_PKCS7ContentInfoTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC  |
      SEC_ASN1_XTRN | 0,
	  offsetof(SEC_PKCS7SignedData,rawCerts),
	  SEC_ASN1_SUB(SEC_SetOfAnyTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC  |
      SEC_ASN1_XTRN | 1,
	  offsetof(SEC_PKCS7SignedData,crls),
	  SEC_ASN1_SUB(CERT_SetOfSignedCrlTemplate) },
    { SEC_ASN1_SET_OF,
	  offsetof(SEC_PKCS7SignedData,signerInfos),
	  SEC_PKCS7SignerInfoTemplate },
    { 0 }
};

static const SEC_ASN1Template SEC_PointerToPKCS7SignedDataTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_PKCS7SignedDataTemplate }
};

static const SEC_ASN1Template SEC_PKCS7RecipientInfoTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SEC_PKCS7RecipientInfo) },
    { SEC_ASN1_INTEGER,
	  offsetof(SEC_PKCS7RecipientInfo,version) },
    { SEC_ASN1_POINTER | SEC_ASN1_XTRN,
	  offsetof(SEC_PKCS7RecipientInfo,issuerAndSN),
	  SEC_ASN1_SUB(CERT_IssuerAndSNTemplate) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(SEC_PKCS7RecipientInfo,keyEncAlg),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_OCTET_STRING,
	  offsetof(SEC_PKCS7RecipientInfo,encKey) },
    { 0 }
};

static const SEC_ASN1Template SEC_PKCS7EncryptedContentInfoTemplate[] = {
    { SEC_ASN1_SEQUENCE | SEC_ASN1_MAY_STREAM,
	  0, NULL, sizeof(SEC_PKCS7EncryptedContentInfo) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(SEC_PKCS7EncryptedContentInfo,contentType) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(SEC_PKCS7EncryptedContentInfo,contentEncAlg),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_MAY_STREAM | SEC_ASN1_CONTEXT_SPECIFIC |
      SEC_ASN1_XTRN | 0,
	  offsetof(SEC_PKCS7EncryptedContentInfo,encContent),
	  SEC_ASN1_SUB(SEC_OctetStringTemplate) },
    { 0 }
};

static const SEC_ASN1Template SEC_PKCS7EnvelopedDataTemplate[] = {
    { SEC_ASN1_SEQUENCE | SEC_ASN1_MAY_STREAM,
	  0, NULL, sizeof(SEC_PKCS7EnvelopedData) },
    { SEC_ASN1_INTEGER,
	  offsetof(SEC_PKCS7EnvelopedData,version) },
    { SEC_ASN1_SET_OF,
	  offsetof(SEC_PKCS7EnvelopedData,recipientInfos),
	  SEC_PKCS7RecipientInfoTemplate },
    { SEC_ASN1_INLINE,
	  offsetof(SEC_PKCS7EnvelopedData,encContentInfo),
	  SEC_PKCS7EncryptedContentInfoTemplate },
    { 0 }
};

static const SEC_ASN1Template SEC_PointerToPKCS7EnvelopedDataTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_PKCS7EnvelopedDataTemplate }
};

static const SEC_ASN1Template SEC_PKCS7SignedAndEnvelopedDataTemplate[] = {
    { SEC_ASN1_SEQUENCE | SEC_ASN1_MAY_STREAM,
	  0, NULL, sizeof(SEC_PKCS7SignedAndEnvelopedData) },
    { SEC_ASN1_INTEGER,
	  offsetof(SEC_PKCS7SignedAndEnvelopedData,version) },
    { SEC_ASN1_SET_OF,
	  offsetof(SEC_PKCS7SignedAndEnvelopedData,recipientInfos),
	  SEC_PKCS7RecipientInfoTemplate },
    { SEC_ASN1_SET_OF | SEC_ASN1_XTRN,
	  offsetof(SEC_PKCS7SignedAndEnvelopedData,digestAlgorithms),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_INLINE,
	  offsetof(SEC_PKCS7SignedAndEnvelopedData,encContentInfo),
	  SEC_PKCS7EncryptedContentInfoTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
      SEC_ASN1_XTRN | 0,
	  offsetof(SEC_PKCS7SignedAndEnvelopedData,rawCerts),
	  SEC_ASN1_SUB(SEC_SetOfAnyTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
      SEC_ASN1_XTRN | 1,
	  offsetof(SEC_PKCS7SignedAndEnvelopedData,crls),
	  SEC_ASN1_SUB(CERT_SetOfSignedCrlTemplate) },
    { SEC_ASN1_SET_OF,
	  offsetof(SEC_PKCS7SignedAndEnvelopedData,signerInfos),
	  SEC_PKCS7SignerInfoTemplate },
    { 0 }
};

static const SEC_ASN1Template
SEC_PointerToPKCS7SignedAndEnvelopedDataTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_PKCS7SignedAndEnvelopedDataTemplate }
};

static const SEC_ASN1Template SEC_PKCS7DigestedDataTemplate[] = {
    { SEC_ASN1_SEQUENCE | SEC_ASN1_MAY_STREAM,
	  0, NULL, sizeof(SEC_PKCS7DigestedData) },
    { SEC_ASN1_INTEGER,
	  offsetof(SEC_PKCS7DigestedData,version) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(SEC_PKCS7DigestedData,digestAlg),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_INLINE,
	  offsetof(SEC_PKCS7DigestedData,contentInfo),
	  sec_PKCS7ContentInfoTemplate },
    { SEC_ASN1_OCTET_STRING,
	  offsetof(SEC_PKCS7DigestedData,digest) },
    { 0 }
};

static const SEC_ASN1Template SEC_PointerToPKCS7DigestedDataTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_PKCS7DigestedDataTemplate }
};

static const SEC_ASN1Template SEC_PKCS7EncryptedDataTemplate[] = {
    { SEC_ASN1_SEQUENCE | SEC_ASN1_MAY_STREAM,
	  0, NULL, sizeof(SEC_PKCS7EncryptedData) },
    { SEC_ASN1_INTEGER,
	  offsetof(SEC_PKCS7EncryptedData,version) },
    { SEC_ASN1_INLINE,
	  offsetof(SEC_PKCS7EncryptedData,encContentInfo),
	  SEC_PKCS7EncryptedContentInfoTemplate },
    { 0 }
};

static const SEC_ASN1Template SEC_PointerToPKCS7EncryptedDataTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_PKCS7EncryptedDataTemplate }
};

static const SEC_ASN1Template *
sec_pkcs7_choose_content_template(void *src_or_dest, PRBool encoding)
{
    const SEC_ASN1Template *theTemplate;
    SEC_PKCS7ContentInfo *cinfo;
    SECOidTag kind;

    PORT_Assert (src_or_dest != NULL);
    if (src_or_dest == NULL)
	return NULL;

    cinfo = (SEC_PKCS7ContentInfo*)src_or_dest;
    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      default:
	theTemplate = SEC_ASN1_GET(SEC_PointerToAnyTemplate);
	break;
      case SEC_OID_PKCS7_DATA:
	theTemplate = SEC_ASN1_GET(SEC_PointerToOctetStringTemplate);
	break;
      case SEC_OID_PKCS7_SIGNED_DATA:
	theTemplate = SEC_PointerToPKCS7SignedDataTemplate;
	break;
      case SEC_OID_PKCS7_ENVELOPED_DATA:
	theTemplate = SEC_PointerToPKCS7EnvelopedDataTemplate;
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	theTemplate = SEC_PointerToPKCS7SignedAndEnvelopedDataTemplate;
	break;
      case SEC_OID_PKCS7_DIGESTED_DATA:
	theTemplate = SEC_PointerToPKCS7DigestedDataTemplate;
	break;
      case SEC_OID_PKCS7_ENCRYPTED_DATA:
	theTemplate = SEC_PointerToPKCS7EncryptedDataTemplate;
	break;
    }
    return theTemplate;
}





