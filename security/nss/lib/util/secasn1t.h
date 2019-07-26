










#ifndef _SECASN1T_H_
#define _SECASN1T_H_

#include "utilrename.h"










typedef struct sec_ASN1Template_struct {
    


    unsigned long kind;

    



    unsigned long offset;

    










    const void *sub;

    





    unsigned int size;
} SEC_ASN1Template;




#define SEC_ASN1_DEFAULT_ARENA_SIZE	(2048)




#define SEC_ASN1_TAG_MASK		0xff










#define SEC_ASN1_TAGNUM_MASK		0x1f
#define SEC_ASN1_BOOLEAN		0x01
#define SEC_ASN1_INTEGER		0x02
#define SEC_ASN1_BIT_STRING		0x03
#define SEC_ASN1_OCTET_STRING		0x04
#define SEC_ASN1_NULL			0x05
#define SEC_ASN1_OBJECT_ID		0x06
#define SEC_ASN1_OBJECT_DESCRIPTOR      0x07

#define SEC_ASN1_REAL                   0x09
#define SEC_ASN1_ENUMERATED		0x0a
#define SEC_ASN1_EMBEDDED_PDV           0x0b
#define SEC_ASN1_UTF8_STRING		0x0c



#define SEC_ASN1_SEQUENCE		0x10
#define SEC_ASN1_SET			0x11
#define SEC_ASN1_NUMERIC_STRING         0x12
#define SEC_ASN1_PRINTABLE_STRING	0x13
#define SEC_ASN1_T61_STRING		0x14
#define SEC_ASN1_VIDEOTEX_STRING        0x15
#define SEC_ASN1_IA5_STRING		0x16
#define SEC_ASN1_UTC_TIME		0x17
#define SEC_ASN1_GENERALIZED_TIME	0x18
#define SEC_ASN1_GRAPHIC_STRING         0x19
#define SEC_ASN1_VISIBLE_STRING		0x1a
#define SEC_ASN1_GENERAL_STRING         0x1b
#define SEC_ASN1_UNIVERSAL_STRING	0x1c

#define SEC_ASN1_BMP_STRING		0x1e
#define SEC_ASN1_HIGH_TAG_NUMBER	0x1f
#define SEC_ASN1_TELETEX_STRING 	SEC_ASN1_T61_STRING






#define SEC_ASN1_METHOD_MASK		0x20
#define SEC_ASN1_PRIMITIVE		0x00
#define SEC_ASN1_CONSTRUCTED		0x20

#define SEC_ASN1_CLASS_MASK		0xc0
#define SEC_ASN1_UNIVERSAL		0x00
#define SEC_ASN1_APPLICATION		0x40
#define SEC_ASN1_CONTEXT_SPECIFIC	0x80
#define SEC_ASN1_PRIVATE		0xc0







#define SEC_ASN1_OPTIONAL	0x00100
#define SEC_ASN1_EXPLICIT	0x00200
#define SEC_ASN1_ANY		0x00400
#define SEC_ASN1_INLINE		0x00800
#define SEC_ASN1_POINTER	0x01000
#define SEC_ASN1_GROUP		0x02000	/* with SET or SEQUENCE means
					 * SET OF or SEQUENCE OF */
#define SEC_ASN1_DYNAMIC	0x04000 /* subtemplate is found by calling
					 * a function at runtime */
#define SEC_ASN1_SKIP		0x08000 /* skip a field; only for decoding */
#define SEC_ASN1_INNER		0x10000	/* with ANY means capture the
					 * contents only (not the id, len,
					 * or eoc); only for decoding */
#define SEC_ASN1_SAVE		0x20000 /* stash away the encoded bytes first;
					 * only for decoding */
#define SEC_ASN1_MAY_STREAM	0x40000	/* field or one of its sub-fields may
					 * stream in and so should encode as
					 * indefinite-length when streaming
					 * has been indicated; only for
					 * encoding */
#define SEC_ASN1_SKIP_REST	0x80000	/* skip all following fields;
					   only for decoding */
#define SEC_ASN1_CHOICE        0x100000 /* pick one from a template */
#define SEC_ASN1_NO_STREAM     0X200000 /* This entry will not stream
                                           even if the sub-template says
                                           streaming is possible.  Helps
                                           to solve ambiguities with potential
                                           streaming entries that are 
                                           optional */
#define SEC_ASN1_DEBUG_BREAK   0X400000 /* put this in your template and the
                                           decoder will assert when it
                                           processes it. Only for use with
                                           SEC_QuickDERDecodeItem */

                                          


#define SEC_ASN1_SEQUENCE_OF	(SEC_ASN1_GROUP | SEC_ASN1_SEQUENCE)
#define SEC_ASN1_SET_OF		(SEC_ASN1_GROUP | SEC_ASN1_SET)
#define SEC_ASN1_ANY_CONTENTS	(SEC_ASN1_ANY | SEC_ASN1_INNER)


#define SEC_ASN1D_MAX_DEPTH 32






typedef const SEC_ASN1Template * SEC_ASN1TemplateChooser(void *arg, PRBool enc);
typedef SEC_ASN1TemplateChooser * SEC_ASN1TemplateChooserPtr;

#if defined(_WIN32) || defined(ANDROID)
#define SEC_ASN1_GET(x)        NSS_Get_##x(NULL, PR_FALSE)
#define SEC_ASN1_SUB(x)        &p_NSS_Get_##x
#define SEC_ASN1_XTRN          SEC_ASN1_DYNAMIC
#define SEC_ASN1_MKSUB(x) \
static const SEC_ASN1TemplateChooserPtr p_NSS_Get_##x = &NSS_Get_##x;
#else
#define SEC_ASN1_GET(x)        x
#define SEC_ASN1_SUB(x)        x
#define SEC_ASN1_XTRN          0
#define SEC_ASN1_MKSUB(x) 
#endif

#define SEC_ASN1_CHOOSER_DECLARE(x) \
extern const SEC_ASN1Template * NSS_Get_##x (void *arg, PRBool enc);

#define SEC_ASN1_CHOOSER_IMPLEMENT(x) \
const SEC_ASN1Template * NSS_Get_##x(void * arg, PRBool enc) \
{ return x; }




typedef struct sec_DecoderContext_struct SEC_ASN1DecoderContext;




typedef struct sec_EncoderContext_struct SEC_ASN1EncoderContext;







typedef enum {
    SEC_ASN1_Identifier = 0,
    SEC_ASN1_Length = 1,
    SEC_ASN1_Contents = 2,
    SEC_ASN1_EndOfContents = 3
} SEC_ASN1EncodingPart;




 
typedef void (* SEC_ASN1NotifyProc)(void *arg, PRBool before,
				    void *dest, int real_depth);






























 
typedef void (* SEC_ASN1WriteProc)(void *arg,
				   const char *data, unsigned long len,
				   int depth, SEC_ASN1EncodingPart data_kind);

#endif 
