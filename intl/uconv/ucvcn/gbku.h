



































#ifndef _GBKU_H__
#define _GBKU_H__


#define  UCS2_NO_MAPPING ((char16_t) 0xfffd)
#define UINT8_IN_RANGE(a, b, c) \
 (((uint8_t)(a) <= (uint8_t)(b))&&((uint8_t)(b) <= (uint8_t)(c)))
#define UNICHAR_IN_RANGE(a, b, c) \
 (((char16_t)(a) <= (char16_t)(b))&&((char16_t)(b) <= (char16_t)(c)))
#define CAST_CHAR_TO_UNICHAR(a) ((char16_t)((unsigned char)(a)))
#define CAST_UNICHAR_TO_CHAR(a) ((char)a)

#define IS_ASCII(a) (0==(0xff80 & (a)))
#define IS_GBK_EURO(c) ((char)0x80 == (c))
#define UCS2_EURO  ((char16_t) 0x20ac)

#include "nsGBKConvUtil.h"

#endif 
