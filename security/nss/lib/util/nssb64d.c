










































#include "nssb64.h"
#include "nspr.h"
#include "secitem.h"
#include "secerr.h"































typedef struct PLBase64DecoderStr PLBase64Decoder;

















PR_BEGIN_EXTERN_C




struct PLBase64DecoderStr {
    
    unsigned char token[4];
    int token_size;

    





    PRInt32 (*output_fn) (void *output_arg, const unsigned char *buf,
			  PRInt32 size);
    void *output_arg;

    




    unsigned char *output_buffer;
    PRUint32 output_buflen;	
    PRUint32 output_length;	
};

PR_END_EXTERN_C










static unsigned char base64_codetovaluep1[256] = {
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	  0,	  0,	  0,	 63,	  0,	  0,	  0,	 64,
	 53,	 54,	 55,	 56,	 57,	 58,	 59,	 60,
	 61,	 62,	  0,	  0,	  0,	  0,	  0,	  0,
	  0,	  1,	  2,	  3,	  4,	  5,	  6,	  7,
	  8,	  9,	 10,	 11,	 12,	 13,	 14,	 15,
	 16,	 17,	 18,	 19,	 20,	 21,	 22,	 23,
	 24,	 25,	 26,	  0,	  0,	  0,	  0,	  0,
	  0,	 27,	 28,	 29,	 30,	 31,	 32,	 33,
	 34,	 35,	 36,	 37,	 38,	 39,	 40,	 41,
	 42,	 43,	 44,	 45,	 46,	 47,	 48,	 49,
	 50,	 51,	 52,	  0,	  0,	  0,	  0,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0

};

#define B64_PAD	'='






static int
pl_base64_decode_4to3 (const unsigned char *in, unsigned char *out)
{
    int j;
    PRUint32 num = 0;
    unsigned char bits;

    for (j = 0; j < 4; j++) {
	bits = base64_codetovaluep1[in[j]];
	if (bits == 0)
	    return -1;
	num = (num << 6) | (bits - 1);
    }

    out[0] = (unsigned char) (num >> 16);
    out[1] = (unsigned char) ((num >> 8) & 0xFF);
    out[2] = (unsigned char) (num & 0xFF);

    return 3;
}





static int
pl_base64_decode_3to2 (const unsigned char *in, unsigned char *out)
{
    PRUint32 num = 0;
    unsigned char bits1, bits2, bits3;

    bits1 = base64_codetovaluep1[in[0]];
    bits2 = base64_codetovaluep1[in[1]];
    bits3 = base64_codetovaluep1[in[2]];

    if ((bits1 == 0) || (bits2 == 0) || (bits3 == 0))
	return -1;

    num = ((PRUint32)(bits1 - 1)) << 10;
    num |= ((PRUint32)(bits2 - 1)) << 4;
    num |= ((PRUint32)(bits3 - 1)) >> 2;

    out[0] = (unsigned char) (num >> 8);
    out[1] = (unsigned char) (num & 0xFF);

    return 2;
}





static int
pl_base64_decode_2to1 (const unsigned char *in, unsigned char *out)
{
    PRUint32 num = 0;
    unsigned char bits1, bits2;

    bits1 = base64_codetovaluep1[in[0]];
    bits2 = base64_codetovaluep1[in[1]];

    if ((bits1 == 0) || (bits2 == 0))
	return -1;

    num = ((PRUint32)(bits1 - 1)) << 2;
    num |= ((PRUint32)(bits2 - 1)) >> 4;

    out[0] = (unsigned char) num;

    return 1;
}





static int
pl_base64_decode_token (const unsigned char *in, unsigned char *out)
{
    if (in[3] != B64_PAD)
	return pl_base64_decode_4to3 (in, out);

    if (in[2] == B64_PAD)
	return pl_base64_decode_2to1 (in, out);

    return pl_base64_decode_3to2 (in, out);
}

static PRStatus
pl_base64_decode_buffer (PLBase64Decoder *data, const unsigned char *in,
			 PRUint32 length)
{
    unsigned char *out = data->output_buffer;
    unsigned char *token = data->token;
    int i, n = 0;

    i = data->token_size;
    data->token_size = 0;

    while (length > 0) {
	while (i < 4 && length > 0) {
	    








	    if (base64_codetovaluep1[*in] > 0 || *in == B64_PAD)
		token[i++] = *in;
	    in++;
	    length--;
	}

	if (i < 4) {
	    
	    data->token_size = i;
	    break;
	}
	i = 0;

	PR_ASSERT((out - data->output_buffer + 3) <= data->output_buflen);

	













	n = pl_base64_decode_4to3 (token, out);
	if (n < 0)
	    break;

	
	out += n;
	n = 0;
    }

    






    if (n < 0) {
	n = pl_base64_decode_token (token, out);
	if (n < 0)
	    return PR_FAILURE;

	out += n;
    }

    







    while (length > 0) {
	if (base64_codetovaluep1[*in] > 0)
	    return PR_FAILURE;
	in++;
	length--;
    }

    
    data->output_length = (PRUint32) (out - data->output_buffer);
    return PR_SUCCESS;
}







static PRStatus
pl_base64_decode_flush (PLBase64Decoder *data)
{
    int count;

    




    if (data->token_size == 0 || data->token[0] == B64_PAD)
	return PR_SUCCESS;

    



    while (data->token_size < 4)
	data->token[data->token_size++] = B64_PAD;

    data->token_size = 0;	

    count = pl_base64_decode_token (data->token,
				    data->output_buffer + data->output_length);
    if (count < 0)
	return PR_FAILURE;

    




    if (data->output_fn != NULL) {
	PRInt32 output_result;

	PR_ASSERT(data->output_length == 0);
	output_result = data->output_fn (data->output_arg,
					 data->output_buffer,
					 (PRInt32) count);
	if (output_result < 0)
	    return  PR_FAILURE;
    } else {
	data->output_length += count;
    }

    return PR_SUCCESS;
}






static PRUint32
PL_Base64MaxDecodedLength (PRUint32 size)
{
    return ((size * 3) / 4);
}








static PLBase64Decoder *
pl_base64_create_decoder (void)
{
    return PR_NEWZAP(PLBase64Decoder);
}





static PLBase64Decoder *
PL_CreateBase64Decoder (PRInt32 (*output_fn) (void *, const unsigned char *,
					      PRInt32),
			void *output_arg)
{
    PLBase64Decoder *data;

    if (output_fn == NULL) {
	PR_SetError (PR_INVALID_ARGUMENT_ERROR, 0);
	return NULL;
    }

    data = pl_base64_create_decoder ();
    if (data != NULL) {
	data->output_fn = output_fn;
	data->output_arg = output_arg;
    }
    return data;
}






static PRStatus
PL_UpdateBase64Decoder (PLBase64Decoder *data, const char *buffer,
			PRUint32 size)
{
    PRUint32 need_length;
    PRStatus status;

    
    if (data == NULL || buffer == NULL || size == 0) {
	PR_SetError (PR_INVALID_ARGUMENT_ERROR, 0);
	return PR_FAILURE;
    }

    


    need_length = PL_Base64MaxDecodedLength (size + data->token_size);

    


    if (need_length > data->output_buflen) {
	unsigned char *output_buffer = data->output_buffer;

	if (output_buffer != NULL)
	    output_buffer = (unsigned char *) PR_Realloc(output_buffer,
							 need_length);
	else
	    output_buffer = (unsigned char *) PR_Malloc(need_length);

	if (output_buffer == NULL)
	    return PR_FAILURE;

	data->output_buffer = output_buffer;
	data->output_buflen = need_length;
    }

    
    PR_ASSERT(data->output_length == 0);
    data->output_length = 0;

    status = pl_base64_decode_buffer (data, (const unsigned char *) buffer,
				      size);

    
    if (status == PR_SUCCESS && data->output_length > 0) {
	PRInt32 output_result;

	PR_ASSERT(data->output_fn != NULL);
	output_result = data->output_fn (data->output_arg,
					 data->output_buffer,
					 (PRInt32) data->output_length);
	if (output_result < 0)
	    status = PR_FAILURE;
    }

    data->output_length = 0;
    return status;
}







static PRStatus
PL_DestroyBase64Decoder (PLBase64Decoder *data, PRBool abort_p)
{
    PRStatus status = PR_SUCCESS;

    
    if (data == NULL) {
	PR_SetError (PR_INVALID_ARGUMENT_ERROR, 0);
	return PR_FAILURE;
    }

    
    if (!abort_p)
	status = pl_base64_decode_flush (data);

    if (data->output_buffer != NULL)
	PR_Free(data->output_buffer);
    PR_Free(data);

    return status;
}














static unsigned char *
PL_Base64DecodeBuffer (const char *src, PRUint32 srclen, unsigned char *dest,
		       PRUint32 maxdestlen, PRUint32 *output_destlen)
{
    PRUint32 need_length;
    unsigned char *output_buffer = NULL;
    PLBase64Decoder *data = NULL;
    PRStatus status;

    if (srclen == 0) {
	*output_destlen = 0;
	if (dest == NULL) {
	    
	    return (unsigned char *) PR_Malloc(1);
	}
	return dest;
    }

    


    need_length = PL_Base64MaxDecodedLength (srclen);

    



    if (dest != NULL) {
	PR_ASSERT(maxdestlen >= need_length);
	if (maxdestlen < need_length) {
	    PR_SetError(PR_BUFFER_OVERFLOW_ERROR, 0);
	    goto loser;
	}
	output_buffer = dest;
    } else {
	output_buffer = (unsigned char *) PR_Malloc(need_length);
	if (output_buffer == NULL)
	    goto loser;
	maxdestlen = need_length;
    }

    data = pl_base64_create_decoder();
    if (data == NULL)
	goto loser;

    data->output_buflen = maxdestlen;
    data->output_buffer = output_buffer;

    status = pl_base64_decode_buffer (data, (const unsigned char *) src,
				      srclen);

    



    if (status == PR_SUCCESS)
	status = pl_base64_decode_flush (data);

    
    data->output_buffer = NULL;

    if (status == PR_SUCCESS) {
	*output_destlen = data->output_length;
	status = PL_DestroyBase64Decoder (data, PR_FALSE);
	data = NULL;
	if (status == PR_FAILURE)
	    goto loser;
	return output_buffer;
    }

loser:
    if (dest == NULL && output_buffer != NULL)
	PR_Free(output_buffer);
    if (data != NULL)
	(void) PL_DestroyBase64Decoder (data, PR_TRUE);
    return NULL;
}















PR_BEGIN_EXTERN_C





struct NSSBase64DecoderStr {
    PLBase64Decoder *pl_data;
};

PR_END_EXTERN_C





NSSBase64Decoder *
NSSBase64Decoder_Create (PRInt32 (*output_fn) (void *, const unsigned char *,
					       PRInt32),
			 void *output_arg)
{
    PLBase64Decoder *pl_data;
    NSSBase64Decoder *nss_data;

    nss_data = PORT_ZNew(NSSBase64Decoder);
    if (nss_data == NULL)
	return NULL;

    pl_data = PL_CreateBase64Decoder (output_fn, output_arg);
    if (pl_data == NULL) {
	PORT_Free(nss_data);
	return NULL;
    }

    nss_data->pl_data = pl_data;
    return nss_data;
}






SECStatus
NSSBase64Decoder_Update (NSSBase64Decoder *data, const char *buffer,
			 PRUint32 size)
{
    PRStatus pr_status;

    
    if (data == NULL) {
	PORT_SetError (SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    pr_status = PL_UpdateBase64Decoder (data->pl_data, buffer, size);
    if (pr_status == PR_FAILURE)
	return SECFailure;

    return SECSuccess;
}







SECStatus
NSSBase64Decoder_Destroy (NSSBase64Decoder *data, PRBool abort_p)
{
    PRStatus pr_status;

    
    if (data == NULL) {
	PORT_SetError (SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    pr_status = PL_DestroyBase64Decoder (data->pl_data, abort_p);

    PORT_Free(data);

    if (pr_status == PR_FAILURE)
	return SECFailure;

    return SECSuccess;
}















SECItem *
NSSBase64_DecodeBuffer (PRArenaPool *arenaOpt, SECItem *outItemOpt,
			const char *inStr, unsigned int inLen)
{
    SECItem *out_item = outItemOpt;
    PRUint32 max_out_len = PL_Base64MaxDecodedLength (inLen);
    PRUint32 out_len;
    void *mark = NULL;
    unsigned char *dummy;

    PORT_Assert(outItemOpt == NULL || outItemOpt->data == NULL);

    if (arenaOpt != NULL)
	mark = PORT_ArenaMark (arenaOpt);

    out_item = SECITEM_AllocItem (arenaOpt, outItemOpt, max_out_len);
    if (out_item == NULL) {
	if (arenaOpt != NULL)
	    PORT_ArenaRelease (arenaOpt, mark);
	return NULL;
    }

    dummy = PL_Base64DecodeBuffer (inStr, inLen, out_item->data,
				   max_out_len, &out_len);
    if (dummy == NULL) {
	if (arenaOpt != NULL) {
	    PORT_ArenaRelease (arenaOpt, mark);
	    if (outItemOpt != NULL) {
		outItemOpt->data = NULL;
		outItemOpt->len = 0;
	    }
	} else {
	    SECITEM_FreeItem (out_item,
			      (outItemOpt == NULL) ? PR_TRUE : PR_FALSE);
	}
	return NULL;
    }

    if (arenaOpt != NULL)
	PORT_ArenaUnmark (arenaOpt, mark);
    out_item->len = out_len;
    return out_item;
}




















#include "base64.h"





unsigned char *
ATOB_AsciiToData(const char *string, unsigned int *lenp)
{
    SECItem binary_item, *dummy;

    binary_item.data = NULL;
    binary_item.len = 0;

    dummy = NSSBase64_DecodeBuffer (NULL, &binary_item, string,
				    (PRUint32) PORT_Strlen(string));
    if (dummy == NULL)
	return NULL;

    PORT_Assert(dummy == &binary_item);

    *lenp = dummy->len;
    return dummy->data;
}
 



SECStatus
ATOB_ConvertAsciiToItem(SECItem *binary_item, char *ascii)
{
    SECItem *dummy;

    if (binary_item == NULL) {
	PORT_SetError (SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    






    binary_item->data = NULL;
    binary_item->len = 0;

    dummy = NSSBase64_DecodeBuffer (NULL, binary_item, ascii,
				    (PRUint32) PORT_Strlen(ascii));

    if (dummy == NULL)
	return SECFailure;

    return SECSuccess;
}
