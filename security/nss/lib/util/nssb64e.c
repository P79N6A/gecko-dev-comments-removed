









#include "nssb64.h"
#include "nspr.h"
#include "secitem.h"
#include "secerr.h"





 







struct PLBase64EncodeStateStr {
    unsigned chunks;
    unsigned saved;
    unsigned char buf[3];
};




typedef struct PLBase64EncoderStr PLBase64Encoder;

















PR_BEGIN_EXTERN_C




struct PLBase64EncoderStr {
    




    unsigned char in_buffer[2];
    int in_buffer_count;

    









 
    PRUint32 line_length;
    PRUint32 current_column;

    





    PRInt32 (*output_fn) (void *output_arg, const char *buf, PRInt32 size);
    void *output_arg;

    




    char *output_buffer;
    PRUint32 output_buflen;	
    PRUint32 output_length;	
};

PR_END_EXTERN_C





static unsigned char base64_valuetocode[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define B64_PAD	'='
#define B64_CR	'\r'
#define B64_LF	'\n'

static PRStatus
pl_base64_encode_buffer (PLBase64Encoder *data, const unsigned char *in,
			 PRUint32 size)
{
    const unsigned char *end = in + size;
    char *out = data->output_buffer + data->output_length;
    unsigned int i = data->in_buffer_count;
    PRUint32 n = 0;
    int off;
    PRUint32 output_threshold;

    
    if (size < (3 - i)) {
	data->in_buffer[i++] = in[0];
	if (size > 1)
	    data->in_buffer[i++] = in[1];
	PR_ASSERT(i < 3);
	data->in_buffer_count = i;
	return PR_SUCCESS;
    }

    
    if (i > 0) {
	n = data->in_buffer[0];
	if (i > 1)
	    n = (n << 8) | data->in_buffer[1];
	data->in_buffer_count = 0;
    }

    
    off = (size + i) % 3;
    if (off > 0) {
	size -= off;
	data->in_buffer[0] = in[size];
	if (off > 1)
	    data->in_buffer[1] = in[size + 1];
	data->in_buffer_count = off;
	end -= off;
    }

    output_threshold = data->output_buflen - 3;

    



    while (in < end) {
	int j, k;

	while (i < 3) {
	    n = (n << 8) | *in++;
	    i++;
	}
	i = 0;

	if (data->line_length > 0) {
	    if (data->current_column >= data->line_length) {
		data->current_column = 0;
		*out++ = B64_CR;
		*out++ = B64_LF;
		data->output_length += 2;
	    }
	    data->current_column += 4;	
	}

	for (j = 18; j >= 0; j -= 6) {
	    k = (n >> j) & 0x3F;
	    *out++ = base64_valuetocode[k];
	}
	n = 0;
	data->output_length += 4;

	if (data->output_length >= output_threshold) {
	    PR_ASSERT(data->output_length <= data->output_buflen);
	    if (data->output_fn != NULL) {
		PRInt32 output_result;

		output_result = data->output_fn (data->output_arg,
						 data->output_buffer,
						 (PRInt32) data->output_length);
		if (output_result < 0)
		    return PR_FAILURE;

		out = data->output_buffer;
		data->output_length = 0;
	    } else {
		




		PR_ASSERT(in == end);
		if (in < end) {
		    PR_SetError (PR_BUFFER_OVERFLOW_ERROR, 0);
		    return PR_FAILURE;
		}
	    }
	}
    }

    return PR_SUCCESS;
}

static PRStatus
pl_base64_encode_flush (PLBase64Encoder *data)
{
    int i = data->in_buffer_count;

    if (i == 0 && data->output_length == 0)
	return PR_SUCCESS;

    if (i > 0) {
	char *out = data->output_buffer + data->output_length;
	PRUint32 n;
	int j, k;

	n = ((PRUint32) data->in_buffer[0]) << 16;
	if (i > 1)
	    n |= ((PRUint32) data->in_buffer[1] << 8);

	data->in_buffer_count = 0;

	if (data->line_length > 0) {
	    if (data->current_column >= data->line_length) {
		data->current_column = 0;
		*out++ = B64_CR;
		*out++ = B64_LF;
		data->output_length += 2;
	    }
	}

	




	for (j = 18; j >= 0; j -= 6) {
	    k = (n >> j) & 0x3F;
	    *out++ = base64_valuetocode[k];
	}

	
	if (i == 1)
	    out[-2] = B64_PAD;
	out[-1] = B64_PAD;

	data->output_length += 4;
    }

    if (data->output_fn != NULL) {
	PRInt32 output_result;

	output_result = data->output_fn (data->output_arg, data->output_buffer,
					 (PRInt32) data->output_length);
	data->output_length = 0;

	if (output_result < 0)
	    return PR_FAILURE;
    }

    return PR_SUCCESS;
}








static PRUint32
PL_Base64MaxEncodedLength (PRUint32 size, PRUint32 line_length)
{
    PRUint32 tokens, tokens_per_line, full_lines, line_break_chars, remainder;

    tokens = (size + 2) / 3;

    if (line_length == 0)
	return tokens * 4;

    if (line_length < 4)	
	line_length = 4;

    tokens_per_line = line_length / 4;
    full_lines = tokens / tokens_per_line;
    remainder = (tokens - (full_lines * tokens_per_line)) * 4;
    line_break_chars = full_lines * 2;
    if (remainder == 0)
	line_break_chars -= 2;

    return (full_lines * tokens_per_line * 4) + line_break_chars + remainder;
}












static PLBase64Encoder *
pl_base64_create_encoder (PRUint32 line_length, char *output_buffer,
			  PRUint32 output_buflen)
{
    PLBase64Encoder *data;
    PRUint32 line_tokens;

    data = PR_NEWZAP(PLBase64Encoder);
    if (data == NULL)
	return NULL;

    if (line_length > 0 && line_length < 4)	
	line_length = 4;

    line_tokens = line_length / 4;
    data->line_length = line_tokens * 4;

    if (output_buffer == NULL) {
	if (output_buflen == 0) {
	    if (data->line_length > 0)	
		output_buflen = data->line_length + 2;
	    else
		output_buflen = 64;		
	}

	output_buffer = (char *) PR_Malloc(output_buflen);
	if (output_buffer == NULL) {
	    PR_Free(data);
	    return NULL;
	}
    }

    data->output_buffer = output_buffer;
    data->output_buflen = output_buflen;
    return data;
}









static PLBase64Encoder *
PL_CreateBase64Encoder (PRInt32 (*output_fn) (void *, const char *, PRInt32),
			void *output_arg, PRUint32 line_length)
{
    PLBase64Encoder *data;

    if (output_fn == NULL) {
	PR_SetError (PR_INVALID_ARGUMENT_ERROR, 0);
	return NULL;
    }

    data = pl_base64_create_encoder (line_length, NULL, 0);
    if (data == NULL)
	return NULL;

    data->output_fn = output_fn;
    data->output_arg = output_arg;

    return data;
}






static PRStatus
PL_UpdateBase64Encoder (PLBase64Encoder *data, const unsigned char *buffer,
			PRUint32 size)
{
    
    if (data == NULL || buffer == NULL || size == 0) {
	PR_SetError (PR_INVALID_ARGUMENT_ERROR, 0);
	return PR_FAILURE;
    }

    return pl_base64_encode_buffer (data, buffer, size);
}







static PRStatus
PL_DestroyBase64Encoder (PLBase64Encoder *data, PRBool abort_p)
{
    PRStatus status = PR_SUCCESS;

    
    if (data == NULL) {
	PR_SetError (PR_INVALID_ARGUMENT_ERROR, 0);
	return PR_FAILURE;
    }

    
    if (!abort_p)
	status = pl_base64_encode_flush (data);

    if (data->output_buffer != NULL)
	PR_Free(data->output_buffer);
    PR_Free(data);

    return status;
}



















static char *
PL_Base64EncodeBuffer (const unsigned char *src, PRUint32 srclen,
		       PRUint32 line_length, char *dest, PRUint32 maxdestlen,
		       PRUint32 *output_destlen)
{
    PRUint32 need_length;
    PLBase64Encoder *data = NULL;
    PRStatus status;

    PR_ASSERT(srclen > 0);
    if (srclen == 0)
	return dest;

    


    need_length = PL_Base64MaxEncodedLength (srclen, line_length);

    


    if (dest != NULL) {
	PR_ASSERT(maxdestlen >= need_length);
	if (maxdestlen < need_length) {
	    PR_SetError(PR_BUFFER_OVERFLOW_ERROR, 0);
	    return NULL;
	}
    } else {
	maxdestlen = need_length;
    }

    data = pl_base64_create_encoder(line_length, dest, maxdestlen);
    if (data == NULL)
	return NULL;

    status = pl_base64_encode_buffer (data, src, srclen);

    



    if (status == PR_SUCCESS)
	status = pl_base64_encode_flush (data);

    if (status != PR_SUCCESS) {
	(void) PL_DestroyBase64Encoder (data, PR_TRUE);
	return NULL;
    }

    dest = data->output_buffer;

    
    data->output_buffer = NULL;

    *output_destlen = data->output_length;
    status = PL_DestroyBase64Encoder (data, PR_FALSE);
    if (status == PR_FAILURE) {
	PR_Free(dest);
	return NULL;
    }

    return dest;
}














PR_BEGIN_EXTERN_C





struct NSSBase64EncoderStr {
    PLBase64Encoder *pl_data;
};

PR_END_EXTERN_C





NSSBase64Encoder *
NSSBase64Encoder_Create (PRInt32 (*output_fn) (void *, const char *, PRInt32),
			 void *output_arg)
{
    PLBase64Encoder *pl_data;
    NSSBase64Encoder *nss_data;

    nss_data = PORT_ZNew(NSSBase64Encoder);
    if (nss_data == NULL)
	return NULL;

    pl_data = PL_CreateBase64Encoder (output_fn, output_arg, 64);
    if (pl_data == NULL) {
	PORT_Free(nss_data);
	return NULL;
    }

    nss_data->pl_data = pl_data;
    return nss_data;
}






SECStatus
NSSBase64Encoder_Update (NSSBase64Encoder *data, const unsigned char *buffer,
			 PRUint32 size)
{
    PRStatus pr_status;

    
    if (data == NULL) {
	PORT_SetError (SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    pr_status = PL_UpdateBase64Encoder (data->pl_data, buffer, size);
    if (pr_status == PR_FAILURE)
	return SECFailure;

    return SECSuccess;
}







SECStatus
NSSBase64Encoder_Destroy (NSSBase64Encoder *data, PRBool abort_p)
{
    PRStatus pr_status;

    
    if (data == NULL) {
	PORT_SetError (SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    pr_status = PL_DestroyBase64Encoder (data->pl_data, abort_p);

    PORT_Free(data);

    if (pr_status == PR_FAILURE)
	return SECFailure;

    return SECSuccess;
}


















char *
NSSBase64_EncodeItem (PRArenaPool *arenaOpt, char *outStrOpt,
		      unsigned int maxOutLen, SECItem *inItem)
{
    char *out_string = outStrOpt;
    PRUint32 max_out_len;
    PRUint32 out_len;
    void *mark = NULL;
    char *dummy;

    PORT_Assert(inItem != NULL && inItem->data != NULL && inItem->len != 0);
    if (inItem == NULL || inItem->data == NULL || inItem->len == 0) {
	PORT_SetError (SEC_ERROR_INVALID_ARGS);
	return NULL;
    }

    max_out_len = PL_Base64MaxEncodedLength (inItem->len, 64);

    if (arenaOpt != NULL)
	mark = PORT_ArenaMark (arenaOpt);

    if (out_string == NULL) {
	if (arenaOpt != NULL)
	    out_string = PORT_ArenaAlloc (arenaOpt, max_out_len + 1);
	else
	    out_string = PORT_Alloc (max_out_len + 1);

	if (out_string == NULL) {
	    if (arenaOpt != NULL)
		PORT_ArenaRelease (arenaOpt, mark);
	    return NULL;
	}
    } else {
	if ((max_out_len + 1) > maxOutLen) {
	    PORT_SetError (SEC_ERROR_OUTPUT_LEN);
	    return NULL;
	}
	max_out_len = maxOutLen;
    }


    dummy = PL_Base64EncodeBuffer (inItem->data, inItem->len, 64,
				   out_string, max_out_len, &out_len);
    if (dummy == NULL) {
	if (arenaOpt != NULL) {
	    PORT_ArenaRelease (arenaOpt, mark);
	} else {
	    PORT_Free (out_string);
	}
	return NULL;
    }

    if (arenaOpt != NULL)
	PORT_ArenaUnmark (arenaOpt, mark);

    out_string[out_len] = '\0';
    return out_string;
}




















#include "base64.h"





char *
BTOA_DataToAscii(const unsigned char *data, unsigned int len)
{
    SECItem binary_item;

    binary_item.data = (unsigned char *)data;
    binary_item.len = len;

    return NSSBase64_EncodeItem (NULL, NULL, 0, &binary_item);
}




char *
BTOA_ConvertItemToAscii (SECItem *binary_item)
{
    return NSSBase64_EncodeItem (NULL, NULL, 0, binary_item);
}
