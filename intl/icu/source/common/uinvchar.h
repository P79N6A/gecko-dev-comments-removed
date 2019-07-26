


















#ifndef __UINVCHAR_H__
#define __UINVCHAR_H__

#include "unicode/utypes.h"











U_INTERNAL UBool U_EXPORT2
uprv_isInvariantString(const char *s, int32_t length);











U_INTERNAL UBool U_EXPORT2
uprv_isInvariantUString(const UChar *s, int32_t length);






#if U_CHARSET_FAMILY==U_ASCII_FAMILY
#   define U_UPPER_ORDINAL(x) ((x)-'A')
#elif U_CHARSET_FAMILY==U_EBCDIC_FAMILY
#   define U_UPPER_ORDINAL(x) (((x) < 'J') ? ((x)-'A') : \
                              (((x) < 'S') ? ((x)-'J'+9) : \
                               ((x)-'S'+18)))
#else
#   error Unknown charset family!
#endif





U_INTERNAL int32_t U_EXPORT2
uprv_compareInvEbcdicAsAscii(const char *s1, const char *s2);






#if U_CHARSET_FAMILY==U_ASCII_FAMILY
#   define uprv_compareInvCharsAsAscii(s1, s2) uprv_strcmp(s1, s2)
#elif U_CHARSET_FAMILY==U_EBCDIC_FAMILY
#   define uprv_compareInvCharsAsAscii(s1, s2) uprv_compareInvEbcdicAsAscii(s1, s2)
#else
#   error Unknown charset family!
#endif





U_INTERNAL char U_EXPORT2
uprv_ebcdicToLowercaseAscii(char c);






#if U_CHARSET_FAMILY==U_ASCII_FAMILY
#   define uprv_invCharToLowercaseAscii uprv_asciitolower
#elif U_CHARSET_FAMILY==U_EBCDIC_FAMILY
#   define uprv_invCharToLowercaseAscii uprv_ebcdicToLowercaseAscii
#else
#   error Unknown charset family!
#endif






U_INTERNAL uint8_t* U_EXPORT2
uprv_aestrncpy(uint8_t *dst, const uint8_t *src, int32_t n);







U_INTERNAL uint8_t* U_EXPORT2
uprv_eastrncpy(uint8_t *dst, const uint8_t *src, int32_t n);



#endif
