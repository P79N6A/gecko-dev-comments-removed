





































































#ifndef _GBKU_H__
#define _GBKU_H__


#define  UCS2_NO_MAPPING ((PRUnichar) 0xfffd)
#define UINT8_IN_RANGE(a, b, c) \
 (((PRUint8)(a) <= (PRUint8)(b))&&((PRUint8)(b) <= (PRUint8)(c)))
#define UNICHAR_IN_RANGE(a, b, c) \
 (((PRUnichar)(a) <= (PRUnichar)(b))&&((PRUnichar)(b) <= (PRUnichar)(c)))
#define CAST_CHAR_TO_UNICHAR(a) ((PRUnichar)((unsigned char)(a)))
#define CAST_UNICHAR_TO_CHAR(a) ((char)a)

#define IS_ASCII(a) (0==(0xff80 & (a)))
#define IS_GBK_EURO(c) ((char)0x80 == (c))
#define UCS2_EURO  ((PRUnichar) 0x20ac)

#include "nsGBKConvUtil.h"

#endif 
