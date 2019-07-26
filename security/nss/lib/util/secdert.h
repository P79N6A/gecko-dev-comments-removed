



#ifndef _SECDERT_H_
#define _SECDERT_H_







#include "utilrename.h"
#include "seccomon.h"

typedef struct DERTemplateStr DERTemplate;









struct DERTemplateStr {
    


    unsigned long kind;

    



    unsigned int offset;

    



    DERTemplate *sub;

    










    unsigned long arg;
};




#define DER_DEFAULT_CHUNKSIZE (2048)




#define DER_TAG_MASK		0xff







#define DER_TAGNUM_MASK		0x1f
#define DER_BOOLEAN		0x01
#define DER_INTEGER		0x02
#define DER_BIT_STRING		0x03
#define DER_OCTET_STRING	0x04
#define DER_NULL		0x05
#define DER_OBJECT_ID		0x06
#define DER_SEQUENCE		0x10
#define DER_SET			0x11
#define DER_PRINTABLE_STRING	0x13
#define DER_T61_STRING		0x14
#define DER_IA5_STRING		0x16
#define DER_UTC_TIME		0x17
#define DER_VISIBLE_STRING	0x1a
#define DER_HIGH_TAG_NUMBER	0x1f






#define DER_METHOD_MASK		0x20
#define DER_PRIMITIVE		0x00
#define DER_CONSTRUCTED		0x20

#define DER_CLASS_MASK		0xc0
#define DER_UNIVERSAL		0x00
#define DER_APPLICATION		0x40
#define DER_CONTEXT_SPECIFIC	0x80
#define DER_PRIVATE		0xc0






#define DER_OPTIONAL		0x00100
#define DER_EXPLICIT		0x00200
#define DER_ANY			0x00400
#define DER_INLINE		0x00800
#define DER_POINTER		0x01000
#define DER_INDEFINITE		0x02000
#define DER_DERPTR		0x04000
#define DER_SKIP		0x08000
#define DER_FORCE		0x10000
#define DER_OUTER		0x40000 /* for DER_DERPTR */





#define DER_ConvertBitString(item)	  \
{					  \
    (item)->len = ((item)->len + 7) >> 3; \
}

#endif 
