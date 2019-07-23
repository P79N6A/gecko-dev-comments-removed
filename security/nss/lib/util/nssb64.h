








































#ifndef _NSSB64_H_
#define _NSSB64_H_

#include "utilrename.h"
#include "seccomon.h"
#include "nssb64t.h"

SEC_BEGIN_PROTOS





extern NSSBase64Decoder *
NSSBase64Decoder_Create (PRInt32 (*output_fn) (void *, const unsigned char *,
					       PRInt32),
			 void *output_arg);

extern NSSBase64Encoder *
NSSBase64Encoder_Create (PRInt32 (*output_fn) (void *, const char *, PRInt32),
			 void *output_arg);






extern SECStatus
NSSBase64Decoder_Update (NSSBase64Decoder *data, const char *buffer,
			 PRUint32 size);

extern SECStatus
NSSBase64Encoder_Update (NSSBase64Encoder *data, const unsigned char *buffer,
			 PRUint32 size);







extern SECStatus
NSSBase64Decoder_Destroy (NSSBase64Decoder *data, PRBool abort_p);

extern SECStatus
NSSBase64Encoder_Destroy (NSSBase64Encoder *data, PRBool abort_p);














extern SECItem *
NSSBase64_DecodeBuffer (PLArenaPool *arenaOpt, SECItem *outItemOpt,
			const char *inStr, unsigned int inLen);

















extern char *
NSSBase64_EncodeItem (PLArenaPool *arenaOpt, char *outStrOpt,
		      unsigned int maxOutLen, SECItem *inItem);

SEC_END_PROTOS

#endif 
